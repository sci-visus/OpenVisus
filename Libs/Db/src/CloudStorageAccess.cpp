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
#include <Visus/Encoders.h>

namespace Visus {

///////////////////////////////////////////////////////////////////////////////////////
CloudStorageAccess::CloudStorageAccess(Dataset* dataset,StringTree config_)
  : config(config_)
{
  this->name = "CloudStorageAccess";
  this->can_read = StringUtils::find(config.readString("chmod", "rw"), "r") >= 0;
  this->can_write = StringUtils::find(config.readString("chmod", "rw"), "w") >= 0;
  this->bitsperblock = cint(config.readString("bitsperblock", cstring(dataset->getDefaultBitsPerBlock()))); VisusAssert(this->bitsperblock>0);
  this->url = config.readString("url", dataset->getUrl().toString()); VisusAssert(url.valid());
  this->compression = config.readString("compression", url.getParam("compression", "lz4")); 

  this->config.writeString("url", url.toString());

  bool disable_async = config.readBool("disable_async", dataset->bServerMode);

  if (int nconnections = disable_async ? 0 : config.readInt("nconnections", 8))
    this->netservice = std::make_shared<NetService>(nconnections);

  this->cloud_storage.reset(CloudStorage::createInstance(url)); 
}

///////////////////////////////////////////////////////////////////////////////////////
CloudStorageAccess::~CloudStorageAccess()
{
}

///////////////////////////////////////////////////////////////////////////////////////
Url CloudStorageAccess::getBlockQueryUrl(SharedPtr<BlockQuery> query) const
{
  Url ret=this->url;
  ret.setPath(StringUtils::format()
    <<url.getPath()<<"/"
    <<"time_"<<cstring((int)query->time)<<"/"
    <<"field_"<<query->field.name<<"/"
    <<"data_"<<std::setw(20) << std::setfill('0') << cstring(query->start_address));
  return ret;
}



///////////////////////////////////////////////////////////////////////////////////////
void CloudStorageAccess::readBlock(SharedPtr<BlockQuery> query)
{
  VisusAssert((int)query->nsamples.innerProduct()==(1<<bitsperblock));
  Url url=getBlockQueryUrl(query);

  auto request=cloud_storage->createGetBlobRequest(url);

  if (!request.valid())
    return readFailed(query);

  request.aborted=query->aborted;

  auto gotNetResponse=[this,query](NetResponse response)
  {
    auto metadata=cloud_storage->getMetadata(response);
    for (auto it : metadata)
      response.setHeader(it.first,it.second);

    if (!response.hasHeader("visus-dtype"))
      response.setHeader("visus-dtype",query->field.dtype.toString());

    if (!response.hasHeader("visus-nsamples"))
      response.setHeader("visus-nsamples",query->nsamples.toString());

    //I want the decoding happens in the 'client' side
    query->setClientProcessing([this,response,query]() 
    {
      if (query->aborted() || !response.isSuccessful()) 
      {
        this->statistics.rfail++;
        return QueryFailed;
      }

      auto decoded=response.getArrayBody();
      if (!decoded)
      {
        this->statistics.rfail++;
        return QueryFailed;
      }

      VisusAssert(decoded.dims==query->nsamples);
      VisusAssert(decoded.dtype==query->field.dtype);
      query->buffer=decoded;

      this->statistics.rok++;
      return QueryOk;
    });

    //set done but the status is not yet set
    query->future.get_promise()->set_value(true);
  };

  
  if (bool bAsync= this->netservice?true:false)
  {
    auto future_response= this->netservice->asyncNetworkIO(request);
    future_response.when_ready([future_response, query,gotNetResponse]() {
      gotNetResponse(future_response.get());
    });
  }
  else
  {
    gotNetResponse(NetService::getNetResponse(request));
  }
}

///////////////////////////////////////////////////////////////////////////////////////
void CloudStorageAccess::writeBlock(SharedPtr<BlockQuery> query)
{
  VisusAssert((int)query->nsamples.innerProduct()==(1<<bitsperblock));

  Url url=getBlockQueryUrl(query);
  
  auto decoded=query->buffer;
  auto encoded=ArrayUtils::encodeArray(compression,decoded);
  
  if (!encoded)
    return writeFailed(query);

  StringMap metadata;
  metadata.setValue("visus-compression"  , compression);
  metadata.setValue("visus-nsamples"     , decoded.dims.toString());
  metadata.setValue("visus-dtype"        , decoded.dtype.toString());
  metadata.setValue("visus-layout"       , decoded.layout);

  auto request=cloud_storage->createAddBlobRequest(url,encoded,metadata);

  if (!request.valid())
    return writeFailed(query);

  request.aborted=query->aborted;

  auto gotNetResponse=[this,query](NetResponse response)
  {
    if (query->aborted() || !response.isSuccessful()) 
      return writeFailed(query);

    return writeOk(query);
  };

  if (bool bAsync= this->netservice? true:false)
  {
    auto future_response= this->netservice->asyncNetworkIO(request);
    future_response.when_ready([future_response, query,gotNetResponse]()  {
      gotNetResponse(future_response.get());
    });
  }
  else
  {
    gotNetResponse(NetService::getNetResponse(request));
  }
}


} //namespace Visus 
