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
  this->can_read = StringUtils::find(config.readString("chmod", "rw"), "r") >= 0;
  this->can_write = StringUtils::find(config.readString("chmod", "rw"), "w") >= 0;
  this->bitsperblock = cint(config.readString("bitsperblock", cstring(dataset->getDefaultBitsPerBlock()))); VisusAssert(this->bitsperblock>0);
  this->url = config.readString("url", dataset->getUrl()); VisusAssert(url.valid());
  this->compression = config.readString("compression", "zip"); //zip compress more than lz4 for network.. 
  this->filename_template = config.readString("filename_template", "$(prefix)/time_$(time)/$(field)/$(block).$(compression)");

  this->config.write("url", url.toString());

  bool disable_async = config.readBool("disable_async", dataset->isServerMode());

  if (int nconnections = disable_async ? 0 : config.readInt("nconnections", 8))
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
  String fieldname = StringUtils::removeSpaces(field.name);
  String ret = filename_template;
  ret = StringUtils::replaceFirst(ret, "$(prefix)", this->url.getPath());
  ret = StringUtils::replaceFirst(ret, "$(time)", StringUtils::onlyAlNum(int(time) == time ? cstring((int)time) : cstring(time)));
  ret = StringUtils::replaceFirst(ret, "$(field)", fieldname.length() < 32 ? StringUtils::onlyAlNum(fieldname) : StringUtils::computeChecksum(fieldname));
  ret = StringUtils::replaceFirst(ret, "$(block)", StringUtils::join(StringUtils::splitInChunks(StringUtils::formatNumber("%032x", blockid), 4), "/"));
  ret = StringUtils::replaceFirst(ret, "$(compression)", compression);

  //backward compatible
  if (StringUtils::contains(ret,"$(start_address)"))
    ret = StringUtils::replaceFirst(ret, "$(start_address)", StringUtils::formatNumber("%020d", blockid << bitsperblock));

  VisusAssert(!StringUtils::contains(ret, "$"));
  return ret;
}


///////////////////////////////////////////////////////////////////////////////////////
void CloudStorageAccess::readBlock(SharedPtr<BlockQuery> query)
{
  VisusAssert((int)query->getNumberOfSamples().innerProduct()==(1<<bitsperblock));

  cloud_storage->getBlob(netservice, Access::getFilename(query), query->aborted).when_ready([this, query](CloudStorageBlob blob) {

    if (!blob.metadata.hasValue("visus-compression"))
      blob.metadata.setValue("visus-compression", this->compression);

    if (!blob.metadata.hasValue("visus-dtype"))
      blob.metadata.setValue("visus-dtype", query->field.dtype.toString());

    if (!blob.metadata.hasValue("visus-nsamples"))
      blob.metadata.setValue("visus-nsamples", query->getNumberOfSamples().toString());

    if (query->aborted() || !blob.valid())
      return readFailed(query);

    auto decoded = ArrayUtils::decodeArray(blob.metadata, blob.body);
    if (!decoded)
      return readFailed(query);

    VisusAssert(decoded.dims == query->getNumberOfSamples());
    VisusAssert(decoded.dtype == query->field.dtype);
    query->buffer = decoded;

    return readOk(query);
  });

}

///////////////////////////////////////////////////////////////////////////////////////
void CloudStorageAccess::writeBlock(SharedPtr<BlockQuery> query)
{
  VisusAssert((int)query->getNumberOfSamples().innerProduct()==(1<<bitsperblock));

  auto decoded=query->buffer;
  auto encoded=ArrayUtils::encodeArray(compression,decoded);
  
  if (!encoded)
    return writeFailed(query);

  CloudStorageBlob blob;
  blob.body = encoded;

  blob.metadata.setValue("visus-compression", compression);
  blob.metadata.setValue("visus-nsamples"     , decoded.dims.toString());
  blob.metadata.setValue("visus-dtype"        , decoded.dtype.toString());
  blob.metadata.setValue("visus-layout"       , decoded.layout);

  cloud_storage->addBlob(netservice, Access::getFilename(query), blob, query->aborted).when_ready([this,query](NetResponse response) {

    if (query->aborted() || !response.isSuccessful())
      return writeFailed(query);

    return writeOk(query);
  });

}


} //namespace Visus 
