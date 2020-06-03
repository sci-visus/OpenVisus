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

#ifndef VISUS_GOOGLE_CLOUD_STORAGE_
#define VISUS_GOOGLE_CLOUD_STORAGE_

#include <Visus/Kernel.h>
#include <Visus/CloudStorage.h>

#define JSON_SKIP_UNSUPPORTED_COMPILER_CHECK 1
#include <Visus/json.hpp>

namespace Visus {

////////////////////////////////////////////////////////////////////////////////
class GoogleDriveStorage : public CloudStorage
{
public:

  VISUS_CLASS(GoogleDriveStorage)

  //constructor
  GoogleDriveStorage(Url url)
  {
    this->client_id = url.getParam("client_id"); 
    this->client_secret = url.getParam("client_secret"); 
    this->refresh_token = url.getParam("refresh_token"); 

    VisusReleaseAssert(!client_id.empty());
    VisusReleaseAssert(!client_secret.empty());
    VisusReleaseAssert(!refresh_token.empty());
  }

  //destructor
  virtual ~GoogleDriveStorage() {
  }

  // getBlob 
  virtual Future<CloudStorageBlob> getBlob(SharedPtr<NetService> service, String blob_name, Aborted aborted = Aborted()) override
  {
    auto ret = Promise<CloudStorageBlob>().get_future();

    NetRequest request("https://storage.googleapis.com" + blob_name +"?alt=media", "GET");
    request.aborted = aborted;
    signRequest(request);

    NetService::push(service, request).when_ready([ret, aborted](NetResponse response) {

      if (!response.isSuccessful())
      {
        PrintWarning("ERROR. Cannot get blob status",response.status,"errormsg",response.getErrorMessage());
        ret.get_promise()->set_value(CloudStorageBlob());
        return;
      }

      CloudStorageBlob blob;
      blob.body = response.body;
      ret.get_promise()->set_value(blob);
    });

    return ret;
  }
  
private:

  String client_id;
  String client_secret;
  String refresh_token;

  struct
  {
    String value;
    Time   t1;
    double expires_in = 0;
  }
  access_token;

  //signRequest
  void signRequest(NetRequest& request)
  {
    //need to regenerate access token?
    if (this->access_token.value.empty() || this->access_token.t1.elapsedSec() > 0.85 * this->access_token.expires_in)
    {
      this->access_token.value = "";

      NetRequest request(Url("https://www.googleapis.com/oauth2/v3/token"), "POST");

      request.setTextBody(concatenate(
             "client_id=", this->client_id,
        "&", "client_secret=", this->client_secret,
        "&", "refresh_token=", this->refresh_token,
        "&", "grant_type=refresh_token"));

      auto response = NetService::getNetResponse(request);
      if (response.isSuccessful())
      {
        auto json = nlohmann::json::parse(response.getTextBody());
        this->access_token.t1 = Time::now();
        this->access_token.value = json["access_token"].get<std::string>();
        this->access_token.expires_in = json["expires_in"].get<int>();
      }
    }

    request.headers.setValue("Authorization", "Bearer " + this->access_token.value);
  }

};

}//namespace

#endif //VISUS_GOOGLE_CLOUD_STORAGE_

