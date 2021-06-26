/*-----------------------------------------------------------------------------
Copyright(c) 2010 - 2018 ViSUS L.L.C.,
Scientific Computing and Imaging Institute of the University of Utah

ViSUS L.L.C., 50 W.Broadway, Ste. 300, 84101 - 2044 Salt Lake City, UT
University of Utah, 72 S Central Campus Dr, Room 3750, 84112 Salt Lake City, UT

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

For additional information about this project contact : pascucci@acm.org
For support : support@visus.net
-----------------------------------------------------------------------------*/

#include <Visus/CloudStorageAccess.h>
#include <Visus/Dataset.h>
#include <Visus/Encoder.h>

namespace Visus {

///////////////////////////////////////////////////////////////////////////////////////
CloudStorageAccess::CloudStorageAccess(Dataset* dataset,StringTree config_)
  : config(config_)
{
  this->name = "CloudStorageAccess";
  this->can_read  = StringUtils::find(config.readString("chmod", DefaultChMod), "r") >= 0;
  this->can_write = StringUtils::find(config.readString("chmod", DefaultChMod), "w") >= 0;
  this->bitsperblock = cint(config.readString("bitsperblock", cstring(dataset->getDefaultBitsPerBlock()))); VisusAssert(this->bitsperblock>0);

  this->url = config.readString("url", dataset->getUrl()); VisusAssert(url.valid());
  this->config.write("url", url.toString());

  this->compression = config.readString("compression", this->url.getParam("compression","zip")); //zip compress more than lz4 for network.. 
  this->layout = config.readString("layout", this->url.getParam("layout","")); //row major is default
  this->reverse_filename = config.readBool("reverse_filename", cbool(this->url.getParam("reverse_filename","0")));
  bool disable_async = config.readBool("disable_async", cbool(this->url.getParam("disable_async",cstring(dataset->isServerMode()))));

  if (int nconnections = disable_async ? 0 : config.readInt("nconnections", cint(this->url.getParam("nconnections",cstring(8)))))
    this->netservice = std::make_shared<NetService>(nconnections);

  this->cloud_storage=CloudStorage::createInstance(url);
}

///////////////////////////////////////////////////////////////////////////////////////
CloudStorageAccess::~CloudStorageAccess()
{
}

///////////////////////////////////////////////////////////////////////////////////////
String CloudStorageAccess::getFilename(Field field, double time, BigInt blockid) const
{
  auto ret  = Path(url.getPath()).getParent().toString() + "/${time}/${field}/${block}";

  // new format
  if (StringUtils::contains(ret, "${"))
  {
    String fieldname = StringUtils::removeSpaces(field.name);
    ret = StringUtils::replaceFirst(ret, "${time}", StringUtils::onlyAlNum(int(time) == time ? cstring((int)time) : cstring(time)));
    ret = StringUtils::replaceFirst(ret, "${field}", fieldname.length() < 32 ? StringUtils::onlyAlNum(fieldname) : StringUtils::computeChecksum(fieldname));
    ret = StringUtils::replaceFirst(ret, "${block}", StringUtils::formatNumber("%016x", blockid));
  }

  VisusAssert(!StringUtils::contains(ret, "$"));

  if (reverse_filename)
    ret=StringUtils::reverse(ret);

  return ret;
}


///////////////////////////////////////////////////////////////////////////////////////
void CloudStorageAccess::readBlock(SharedPtr<BlockQuery> query)
{
  VisusAssert((int)query->getNumberOfSamples().innerProduct()==(1<<bitsperblock));

  auto blob_name = Access::getFilename(query);

  cloud_storage->getBlob(netservice, blob_name, query->aborted).when_ready([this, query](CloudStorageBlob blob) {

    blob.metadata.setValue("visus-compression", this->compression);
    blob.metadata.setValue("visus-dtype", query->field.dtype.toString());
    blob.metadata.setValue("visus-nsamples", query->getNumberOfSamples().toString());
    blob.metadata.setValue("visus-layout", this->layout);

    if (query->aborted())
      return readFailed(query, "query aborted");

    if (!blob.valid())
      return readFailed(query, "blob not valid");

    auto decoded = ArrayUtils::decodeArray(blob.metadata, blob.body);
    if (!decoded.valid())
      return readFailed(query, "cannot decode array");

    VisusAssert(decoded.dims == query->getNumberOfSamples());
    VisusAssert(decoded.dtype == query->field.dtype);
    query->buffer = decoded;

    return readOk(query);
  });

}

///////////////////////////////////////////////////////////////////////////////////////
void CloudStorageAccess::writeBlock(SharedPtr<BlockQuery> query)
{
  return writeFailed(query, "not supported");
}


} //namespace Visus 
