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
  this->can_read  = StringUtils::find(config.readString("chmod", DefaultChMod), "r") >= 0;
  this->can_write = StringUtils::find(config.readString("chmod", DefaultChMod), "w") >= 0;
  this->bitsperblock = cint(config.readString("bitsperblock", cstring(dataset->getDefaultBitsPerBlock()))); VisusAssert(this->bitsperblock>0);
  this->url = config.readString("url", dataset->getUrl()); VisusAssert(url.valid());

  this->compression = config.readString("compression", "zip");

  this->config.write("url", url.toString());

  this->num_queries_per_request = cint(this->config.readString("num_queries_per_request", "8"));

  if (this->num_queries_per_request>1)
  {
    //I may have some extra params I want to keep!
    auto request=NetRequest(url.withPath(url.getPath()));
    request.url.setParam("action", "ping");

    auto response = NetService::getNetResponse(request);
    bool bSupportAggregation = cbool(response.getHeader("block-query-support-aggregation", "0"));
    if (!bSupportAggregation)
    {
      PrintInfo("Server does not support block-query-support-aggregation, so I'm overriding num_queries_per_request to be 1");
      this->num_queries_per_request = 1;
    }
    else
    {
      PrintInfo("Server supports block query aggregration","num_queries_per_request", this->num_queries_per_request);
    }
  }


  bool disable_async = dataset->isServerMode() || config.readBool("disable_async", false);
  if (!disable_async)
  {
    int nconnections = config.readInt("nconnections", 6);
    this->netservice = std::make_shared<NetService>(nconnections);
  }

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
      query->field.name == batch[0]->field.name &&
      query->time       == batch[0]->time &&
      query->aborted    == batch[0]->aborted;

    if (!bCompatible)
      flushBatch();
  }

  batch.push_back(query);

  //reached the number of queries per batch?
  if (batch.size() >= num_queries_per_request)
    flushBatch();
}


//////////////////////////////////////////////////////////////////////////////////////
void ModVisusAccess::flushBatch()
{
  if (batch.empty())
    return;

  Batch batch;
  std::swap(batch,this->batch);

  auto compression = getCompression();

  Url URL(this->url.withPath(url.getPath()));
  URL.setParam("action"      , "rangequery");
  URL.setParam("dataset"     , this->url.getParam("dataset"));
  URL.setParam("compression" , compression);
  URL.setParam("field"       , batch[0]->field.name);
  URL.setParam("time"        , cstring(batch[0]->time));

  //send blockid to the server
  {
    std::vector<String> v;
    for (auto query : batch)
      v.push_back(cstring(query->blockid));
    URL.setParam("block", StringUtils::join(v));
  }

  auto REQUEST=NetRequest(URL);
  REQUEST.aborted=batch[0]->aborted;

  NetService::push(netservice, REQUEST).when_ready([this,batch](NetResponse RESPONSE)
  {
    std::vector<NetResponse> responses = NetResponse::decompose(RESPONSE);
    responses.resize(batch.size(), NetResponse(HttpStatus::STATUS_CANCELLED));

    for (int I = 0; I < batch.size(); I++)
    {
      auto query    = batch[I];
      auto response = responses[I];

      if (!response.hasHeader("visus-dtype"))
        response.setHeader("visus-dtype", query->field.dtype.toString());

      if (!response.hasHeader("visus-nsamples"))
        response.setHeader("visus-nsamples", query->getNumberOfSamples().toString());

      if (query->aborted()) {
        readFailed(query,"aborted");
        continue;
      }
      
      if (!response.isSuccessful())
      {
        readFailed(query,"response not valid");
        continue;
      }

      auto decoded = response.getCompatibleArrayBody(query->getNumberOfSamples(), query->field.dtype);
      if (!decoded.valid())
      {
        readFailed(query,"cannot decode array");
        continue;
      }

      query->buffer = decoded;

      readOk(query);
    }
  });

}


} //namespace Visus 

