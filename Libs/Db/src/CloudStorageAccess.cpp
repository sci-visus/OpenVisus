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
#include <Visus/File.h>

namespace Visus {

///////////////////////////////////////////////////////////////////////////////////////
CloudStorageAccess::CloudStorageAccess(Dataset* dataset,StringTree config_)
  : config(config_)
{ 
  this->dataset = dataset;

  this->name = this->name = config.readString("name", "CloudStorageAccess");
  this->can_read  = StringUtils::find(config.readString("chmod", DefaultChMod), "r") >= 0;
  this->can_write = StringUtils::find(config.readString("chmod", DefaultChMod), "w") >= 0;
  this->bitsperblock = cint(config.readString("bitsperblock", cstring(dataset->getDefaultBitsPerBlock()))); VisusAssert(this->bitsperblock>0);

  this->url = config.readString("url", dataset->getUrl()); VisusAssert(url.valid());
  this->config.write("url", url.toString());

  this->compression = config.readString("compression","zip");

  this->layout = config.readString("layout", this->url.getParam("layout","")); //row major is default
  this->reverse_filename = config.readBool("reverse_filename", cbool(this->url.getParam("reverse_filename","0")));

  bool disable_async = config.readBool("disable_async", cbool(this->url.getParam("disable_async",cstring(dataset->isServerMode()))));

  int nconnections = disable_async ? 0 : config.readInt("nconnections", cint(this->url.getParam("nconnections", cstring(64))));

  if (nconnections)
    this->netservice = std::make_shared<NetService>(nconnections);

  this->cloud_storage=CloudStorage::createInstance(url);

  //example: "s3://bucket-name/whatever/$(time)/$(field)/$(block:%016x:%04x).$(compression)";
  //NOTE 16x is enough for 16*4 bits==64 bit for block number
  //     splitting by 4 means 2^16= 64K files inside a directory with max 64/16=4 levels of directories
  this->filename_template= config.readString("filename_template");

  if (this->filename_template.empty())
  { 
    //guess fromm *.idx filename
    auto path = Path(this->url.getPath());
    auto path_no_ext = path.withoutExtension();
    String blob_extension = this->url.getParam("blob_extension", ".bin");
    this->filename_template = path_no_ext + "/$(time)/$(field)/$(block:%016x:%04x)" + blob_extension; //example for Pania blob_extension=".bin.zz"
  }

  VisusReleaseAssert(!this->filename_template.empty());

  PrintInfo("Created CloudStorageAccess", "url", url, "filename_template", filename_template, "compression", this->compression, "nconnections", nconnections);
}

///////////////////////////////////////////////////////////////////////////////////////
CloudStorageAccess::~CloudStorageAccess()
{
}



///////////////////////////////////////////////////////////////////////////////////////
String CloudStorageAccess::getFilename(Field field, double time, BigInt blockid) const
{    
  auto compression = getCompression();
  auto ret = getBlockFilename(this->dataset, this->bitsperblock, this->filename_template, field, time, compression, blockid, reverse_filename);

  //s3://bucket/... -> /bucket/...
  if (StringUtils::startsWith(ret,"s3://"))
    ret = ret.substr(4);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////
void CloudStorageAccess::readBlock(SharedPtr<BlockQuery> query)
{
  //relaxing conditions for VISUS_IDX2 (until I get the layout)
  //VisusAssert((int)query->getNumberOfSamples().innerProduct() == (1 << bitsperblock));

  auto blob_name = getFilename(query->field, query->time, query->blockid);

  cloud_storage->getBlob(netservice, blob_name, /*head*/false, /*range*/{0,0}, query->aborted).when_ready([this, query](SharedPtr<CloudStorageItem> blob) {

    if (!blob || !blob->valid())
      return readFailed(query, query->aborted()? "query aborted" : "blob not valid");

    auto compression = getCompression();

    //special case for idx2 where I just want to data as it is
    if (!query->getNumberOfSamples().innerProduct())
    {
      blob->metadata.setValue("visus-compression", compression);
      blob->metadata.setValue("visus-dtype", DTypes::UINT8.toString());
      blob->metadata.setValue("visus-nsamples", cstring(blob->body->c_size()));
      blob->metadata.setValue("visus-layout", this->layout);
    }
    else
    {
      VisusAssert((int)query->getNumberOfSamples().innerProduct() == (1 << bitsperblock));
      blob->metadata.setValue("visus-compression", compression);
      blob->metadata.setValue("visus-dtype", query->field.dtype.toString());
      blob->metadata.setValue("visus-nsamples", query->getNumberOfSamples().toString());
      blob->metadata.setValue("visus-layout", this->layout);
    }

    auto decoded = ArrayUtils::decodeArray(blob->metadata, blob->body);
    if (!decoded.valid())
      return readFailed(query, "cannot decode array");

    //relaxing a little for VISUS_IDX2 (until I get the layout)
    //VisusAssert(decoded.dims == query->getNumberOfSamples());
    //VisusAssert(decoded.dtype == query->field.dtype);
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
