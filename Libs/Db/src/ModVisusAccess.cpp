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

  this->config.write("url", url.toString());

  this->num_queries_per_request = cint(this->config.readString("num_queries_per_request", "8"));

  if (num_queries_per_request>1)
  {
    Url url(this->url.getProtocol() + "://" + this->url.getHostname() + ":" + cstring(this->url.getPort()) + "/mod_visus");
    url.setParam("action", "ping");

    auto request = NetRequest(url);
    auto response = NetService::getNetResponse(request);
    bool bSupportAggregation = cbool(response.getHeader("block-query-support-aggregation", "0"));
    if (!bSupportAggregation)
    {
      VisusInfo() << "Server does not support block-query-support-aggregation, so I'm overriding num_queries_per_request to be 1";
      num_queries_per_request = 1;
    }
  }

  bool disable_async = dataset->isServerMode() || config.readBool("disable_async", false);
  if (!disable_async)
  {
    int nconnections = config.readInt("nconnections", (num_queries_per_request == 1)? (8) : (2));
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

  std::vector<String> from, to;
  for (auto query : batch)
  {
    from.push_back(cstring(query->start_address));
    to.push_back(cstring(query->end_address));
  }

  Url URL(this->url.getProtocol() + "://" + this->url.getHostname() + ":" + cstring(this->url.getPort()) + "/mod_visus");
  URL.setParam("action"      , "rangequery");
  URL.setParam("dataset"     , this->url.getParam("dataset"));
  URL.setParam("compression" , this->compression);
  URL.setParam("field"       , batch[0]->field.name);
  URL.setParam("time"        , cstring(batch[0]->time));
  URL.setParam("from"        , StringUtils::join(from," "));
  URL.setParam("to"          , StringUtils::join(to  ," "));

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

      if (query->aborted() || !response.isSuccessful())
      {
        readFailed(query);
        continue;
      }

      auto decoded = response.getArrayBody();
      if (!decoded)
      {
        readFailed(query);
        continue;
      }

      VisusAssert(decoded.dims  == query->getNumberOfSamples());
      VisusAssert(decoded.dtype == query->field.dtype);
      query->buffer = decoded;

      readOk(query);
    }
  });

}


} //namespace Visus 

