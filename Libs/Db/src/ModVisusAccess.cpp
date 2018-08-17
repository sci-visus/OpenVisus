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

#include <Visus/ModVisusAccess.h>
#include <Visus/Dataset.h>
#include <Visus/NetService.h>

namespace Visus {


///////////////////////////////////////////////////////////////////////////////////////
ModVisusAccess::ModVisusAccess(Dataset* dataset,StringTree config_) 
  : config(config_)
{
  this->name = "ModVisusAccess";
  this->can_read = StringUtils::find(config.readString("chmod", "rw"), "r") >= 0;
  this->can_write = StringUtils::find(config.readString("chmod", "rw"), "w") >= 0;
  this->bitsperblock = cint(config.readString("bitsperblock", cstring(dataset->getDefaultBitsPerBlock()))); VisusAssert(this->bitsperblock>0);
  this->url = config.readString("url", dataset->getUrl().toString()); VisusAssert(url.valid());
  this->compression = config.readString("compression", url.getParam("compression", "zip"));  //TODO: should I swith to lz4?

  this->config.writeString("url", url.toString());

  bool disable_async = config.readBool("disable_async", dataset->bServerMode);

  if (int nconnections = disable_async ? 0 : config.readInt("nconnections", 8))
    this->netservice = std::make_shared<NetService>(nconnections);

  this->num_queries_per_request=cint(this->config.readString("num_queries_per_request","1"));
  VisusAssert(num_queries_per_request > 0);
}

//////////////////////////////////////////////////////////////////////////////////////
ModVisusAccess::~ModVisusAccess() {
}


///////////////////////////////////////////////////////////////////////////////////////
void ModVisusAccess::readBlock(SharedPtr<BlockQuery> query)
{
  if (!batch.empty())
  {
    bool bCompatible =
      query->field.name          == batch[0]->field.name &&
      query->time                == batch[0]->time &&
      query->aborted.inner_value == batch[0]->aborted.inner_value;

    if (!bCompatible)
      flushBatch();
  }

  batch.push_back(query);

  //reached the number of queries per batch?
  if (batch.size() >= num_queries_per_request)
    flushBatch();
}

///////////////////////////////////////////////////////////////////////////////////////
void ModVisusAccess::onNetResponse(NetResponse response,SharedPtr<BlockQuery> query)
{
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

  //done but status not set yet
  query->future.get_promise()->set_value(true);
}

///////////////////////////////////////////////////////////////////////////////////////
void ModVisusAccess::onNetResponse(NetResponse RESPONSE,Batch batch)
{
  std::vector<NetResponse> responses=NetResponse::decompose(RESPONSE);
  responses.resize(batch.size(),NetResponse(HttpStatus::STATUS_CANCELLED));

  for (int I=0;I<batch.size();I++)
    onNetResponse(responses[I],batch[I]);
}

//////////////////////////////////////////////////////////////////////////////////////
void ModVisusAccess::flushBatch()
{
  if (batch.empty())
    return;

  Batch batch;
  std::swap(batch,this->batch);

  Url url(this->url.getProtocol() + "://" + this->url.getHostname() + ":" + cstring(this->url.getPort()) + "/mod_visus");
  url.setParam("action"     ,"rangequery");
  url.setParam("dataset"    ,this->url.getParam("dataset"));
  url.setParam("compression",this->compression);

  url.setParam("field"      ,batch[0]->field.name);
  url.setParam("time"       ,cstring(batch[0]->time));

  std::vector<String> from,to; 
  for (auto query : batch) 
  {
    from.push_back(cstring(query->start_address));
    to  .push_back(cstring(query->end_address  ));
  }

  url.setParam("from",StringUtils::join(from," "));
  url.setParam("to"  ,StringUtils::join(to  ," "));

  auto request=NetRequest(url);
  request.aborted=batch[0]->aborted;

  if (bool bAsync= this->netservice?true:false)
  {
    auto FUTURE_RESPONSE= this->netservice->asyncNetworkIO(request);
    FUTURE_RESPONSE.when_ready([this, FUTURE_RESPONSE, batch]() {
      onNetResponse(FUTURE_RESPONSE.get(),batch);
    });
  }
  else
  {
    auto RESPONSE=NetService::getNetResponse(request);
    onNetResponse(RESPONSE,batch);
  }

}


} //namespace Visus 

