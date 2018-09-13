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

#include <Visus/CloudStorage.h>
#include <Visus/NetService.h>
#include <Visus/Log.h>
#include <Visus/Path.h>
#include <Visus/UUID.h>

#include <cctype>
#include <Visus/json.hpp>

#if WIN32
#pragma warning(disable:4996)
#endif

namespace Visus {

//////////////////////////////////////////////////////////////////////////////// 
class AzureCloudStorage : public CloudStorage
{
public:

  Url    url;
  String account_name;
  String access_key;

  String container;

  //constructor
  AzureCloudStorage(Url url) 
  {
    this->access_key = url.getParam("access_key");
    VisusAssert(!access_key.empty());
    this->access_key = StringUtils::base64Decode(access_key);

    this->account_name =url.getHostname().substr(0, url.getHostname().find('.'));

    this->url = url.getProtocol() + "://" + url.getHostname();
  }

  //destructor
  virtual ~AzureCloudStorage() {
  }

  //signRequest
  void signRequest(NetRequest& request)
  {
    String canonicalized_resource = "/" + this->account_name + request.url.getPath();

    if (!request.url.params.empty())
    {
      std::ostringstream out;
      for (auto it = request.url.params.begin(); it != request.url.params.end(); ++it)
        out << "\n" << it->first << ":" << it->second;
      canonicalized_resource += out.str();
    }

    char date_GTM[256];
    time_t t; time(&t);
    struct tm *ptm = gmtime(&t);
    strftime(date_GTM, sizeof(date_GTM), "%a, %d %b %Y %H:%M:%S GMT", ptm);

    request.setHeader("x-ms-version", "2018-03-28");
    request.setHeader("x-ms-date", date_GTM);

    String canonicalized_headers;
    {
      std::ostringstream out;
      int N = 0; for (auto it = request.headers.begin(); it != request.headers.end(); it++)
      {
        if (StringUtils::startsWith(it->first, "x-ms-"))
          out << (N++ ? "\n" : "") << StringUtils::toLower(it->first) << ":" << it->second;
      }
      canonicalized_headers = out.str();
    }

    /*
    In the current version, the Content-Length field must be an empty string if the content length of the request is zero. 
    In version 2014-02-14 and earlier, the content length was included even if zero. 
    See below for more information on the old behavior
    */
    String content_length = request.getHeader("Content-Length");
    if (cint(content_length) == 0)
      content_length = "";

    String signature;
    signature += request.method + "\n";// Verb
    signature += request.getHeader("Content-Encoding") + "\n";
    signature += request.getHeader("Content-Language") + "\n";
    signature += content_length + "\n";
    signature += request.getHeader("Content-MD5") + "\n";
    signature += request.getHeader("Content-Type") + "\n";
    signature += request.getHeader("Date") + "\n";
    signature += request.getHeader("If-Modified-Since") + "\n";
    signature += request.getHeader("If-Match") + "\n";
    signature += request.getHeader("If-None-Match") + "\n";
    signature += request.getHeader("If-Unmodified-Since") + "\n";
    signature += request.getHeader("Range") + "\n";
    signature += canonicalized_headers + "\n";
    signature += canonicalized_resource;

    //if something wrong happens open a "telnet hostname 80", copy and paste what's the request made by curl (setting  CURLOPT_VERBOSE to 1)
    //and compare what azure is signing from what you are using
    //VisusInfo() << signature;

    signature = StringUtils::base64Encode(StringUtils::sha256(signature, this->access_key));

    request.setHeader("Authorization", "SharedKey " + account_name + ":" + signature);
  }

  // addContainer
  Future<bool> addContainer(SharedPtr<NetService> service, String container_name, Aborted aborted = Aborted()) 
  {
    VisusAssert(!StringUtils::contains(container_name, "/"));
    auto ret = Promise<bool>().get_future();

    NetRequest request(this->url.toString() + "/" + container_name, "PUT");
    request.aborted = aborted;
    request.url.params.setValue("restype", "container");
    request.setContentLength(0);
    //request.setHeader("x-ms-prop-publicaccess", "container"); IF YOU WANT PUBLIC
    signRequest(request);

    NetService::push(service, request).when_ready([ret](NetResponse response) {
      bool bAlreadyExits = response.status == 409;
      ret.get_promise()->set_value(response.isSuccessful() || bAlreadyExits);
    });

    return ret;
  }

  // deleteContainer
  Future<bool> deleteContainer(SharedPtr<NetService> service, String container_name, Aborted aborted = Aborted())
  {
    VisusAssert(!StringUtils::contains(container_name, "/"));
    NetRequest request(this->url.toString() + "/" + container_name, "DELETE");
    request.aborted = aborted;
    request.url.params.setValue("restype", "container");
    signRequest(request);

    auto ret = Promise<bool>().get_future();

    NetService::push(service, request).when_ready([ret](NetResponse response) {
      ret.get_promise()->set_value(response.isSuccessful());
    });

    return ret;
  }

  //setContainer
  Future<bool> setContainer(SharedPtr<NetService> service, String blob_name, Aborted aborted = Aborted())
  {
    auto ret = Promise<bool>().get_future();

    auto v = StringUtils::split(blob_name,"/");
    if (v.empty() || v[0].empty())
    {
      ret.get_promise()->set_value(false);
      return ret;
    }

    //no need to set it again 
    auto container = v[0];
    if (!this->container.empty() && container == this->container) {
      ret.get_promise()->set_value(true);
      return ret;
    }

    //try to add
    addContainer(service, container, aborted).when_ready([this,container,ret](bool bOk) {
      if (bOk) 
        this->container = container;
      ret.get_promise()->set_value(bOk);
    });

    return ret;
  }

  // addBlob
  virtual Future<bool> addBlob(SharedPtr<NetService> service, String blob_name, Blob blob,Aborted aborted = Aborted()) override
  {
    auto ret = Promise<bool>().get_future();

    setContainer(service, blob_name,aborted).when_ready([this,ret, service, blob,blob_name,aborted](bool bOk)
    {
      if (!bOk)
      {
        ret.get_promise()->set_value(false);
        return;
      }

      NetRequest request(this->url.toString() + blob_name, "PUT");
      request.aborted = aborted;
      request.body = blob.body;
      request.setContentLength(blob.body->c_size());
      request.setHeader("x-ms-blob-type", "BlockBlob");
      request.setContentType(blob.content_type);

      for (auto it : blob.metadata)
      {
        auto name = it.first;
        auto value = it.second;

        //name must be a C# variable name
        VisusAssert(!StringUtils::contains(name, "_"));
        if (StringUtils::contains(name, "-"))
          name = StringUtils::replaceAll(name, "-", "_");

        request.setHeader("x-ms-meta-" + name, value);
      }

      signRequest(request);

      NetService::push(service, request).when_ready([ret](NetResponse response) {
        ret.get_promise()->set_value(response.isSuccessful());
      });

    });

    return ret;
  }

  // getBlob 
  virtual Future<Blob> getBlob(SharedPtr<NetService> service, String blob_name, Aborted aborted = Aborted()) override
  {
    auto ret = Promise<Blob>().get_future();

    setContainer(service, blob_name, aborted).when_ready([this, ret, service, blob_name, aborted](bool bOk) {

      if (!bOk)
      {
        ret.get_promise()->set_value(Blob());
        return;
      }

      NetRequest request(this->url.toString() + blob_name, "GET");
      request.aborted = aborted;
      signRequest(request);

      NetService::push(service, request).when_ready([ret](NetResponse response) {

        if (!response.isSuccessful())
        {
          ret.get_promise()->set_value(Blob());
          return;
        }

        //parse metadata
        Blob blob;
        String metatata_prefix = "x-ms-meta-";
        for (auto it = response.headers.begin(); it != response.headers.end(); it++)
        {
          String name = it->first;
          if (StringUtils::startsWith(name, metatata_prefix))
          {
            name = name.substr(metatata_prefix.length());

            //trick: azure does not allow the "-" 
            if (StringUtils::contains(name, "_"))
              name = StringUtils::replaceAll(name, "_", "-");

            blob.metadata.setValue(name, it->second);
          }
        }

        blob.body = response.body;

        auto content_type = response.getContentType();
        if (!content_type.empty())
          blob.content_type = content_type;

        ret.get_promise()->set_value(blob);
      });
    });

    return ret;
  }


  // deleteBlob
  virtual Future<bool> deleteBlob(SharedPtr<NetService> service, String blob_name, Aborted aborted) override
  {
    auto ret = Promise<bool>().get_future();

    NetRequest request(this->url.toString() + blob_name, "DELETE");
    request.aborted = aborted;
    signRequest(request);

    NetService::push(service, request).when_ready([ret](NetResponse response) {
      ret.get_promise()->set_value(response.isSuccessful());
    });

    return ret;
  }

};


////////////////////////////////////////////////////////////////////////////////
class AmazonCloudStorage : public CloudStorage
{
public:

  VISUS_CLASS(AmazonCloudStorage)

  Url    url;
  String username;
  String password;

  String container;

  //constructor
  AmazonCloudStorage(Url url)  
  {
    this->username = url.getParam("username");
    VisusAssert(!this->username.empty());

    this->password = url.getParam("password");
    VisusAssert(!this->password.empty());
    this->password = StringUtils::base64Decode(password);

    this->url = url.getProtocol() + "://" + url.getHostname();
  }

  //destructor
  virtual ~AmazonCloudStorage() {
  }

  //signRequest
  void signRequest(NetRequest& request)
  {
    String canonicalized_resource = request.url.getPath();
    int find_bucket = StringUtils::find(request.url.getHostname(), "s3.amazonaws.com"); VisusAssert(find_bucket >= 0);
    String bucket_name = request.url.getHostname().substr(0, find_bucket);
    bucket_name = StringUtils::rtrim(bucket_name, ".");
    if (!bucket_name.empty())
      canonicalized_resource = "/" + bucket_name + canonicalized_resource;

    String canonicalized_headers;
    {
      std::ostringstream out;
      for (auto it = request.headers.begin(); it != request.headers.end(); it++)
      {
        if (StringUtils::startsWith(it->first, "x-amz-"))
          out << StringUtils::toLower(it->first) << ":" << it->second << "\n";
      }
      canonicalized_headers = out.str();
    }

    char date_GTM[256];
    time_t t; time(&t);
    struct tm *ptm = gmtime(&t);
    strftime(date_GTM, sizeof(date_GTM), "%a, %d %b %Y %H:%M:%S GMT", ptm);

    String signature = request.method + "\n";
    signature += request.getHeader("Content-MD5") + "\n";
    signature += request.getContentType() + "\n";
    signature += String(date_GTM) + "\n";
    signature += canonicalized_headers;
    signature += canonicalized_resource;
    signature = StringUtils::base64Encode(StringUtils::sha1(signature, password));
    request.setHeader("Host", request.url.getHostname());
    request.setHeader("Date", date_GTM);
    request.setHeader("Authorization", "AWS " + username + ":" + signature);
  }

  // addContainer
  Future<int> addContainer(SharedPtr<NetService> service,String container_name,Aborted aborted=Aborted()) 
  {
    auto ret = Promise<int>().get_future();

    VisusAssert(!StringUtils::contains(container_name, "/"));
    NetRequest request(this->url.toString() + "/" + container_name, "PUT");
    request.aborted = aborted;
    request.url.setPath(request.url.getPath() + "/"); //IMPORTANT the "/" to indicate is a container! see http://www.bucketexplorer.com/documentation/amazon-s3--how-to-create-a-folder.html
    signRequest(request);

    NetService::push(service, request).when_ready([ret](NetResponse response) {
      ret.get_promise()->set_value(response.isSuccessful());
    });

    return ret;
  }

  // deleteContainer
  Future<bool> deleteContainer(SharedPtr<NetService> service, String container_name, Aborted aborted = Aborted()) 
  {
    VisusAssert(!StringUtils::contains(container_name, "/"));

    auto ret = Promise<bool>().get_future();

    NetRequest request(this->url.toString() + "/" + container_name, "DELETE");
    request.aborted = aborted;
    request.url.setPath(request.url.getPath() + "/"); //IMPORTANT the "/" to indicate is a container!
    signRequest(request);

    NetService::push(service, request).when_ready([ret](NetResponse response) {
      ret.get_promise()->set_value(response.isSuccessful());
    });

    return ret;
  }


  //setContainer
  Future<bool> setContainer(SharedPtr<NetService> service, String blob_name, Aborted aborted = Aborted())
  {
    auto ret = Promise<bool>().get_future();

    auto v = StringUtils::split(blob_name, "/");
    if (v.empty() || v[0].empty())
    {
      ret.get_promise()->set_value(false);
      return ret;
    }

    //no need to set it again 
    auto container = v[0];
    if (!this->container.empty() && container == this->container) {
      ret.get_promise()->set_value(true);
      return ret;
    }

    //try to add
    addContainer(service, container, aborted).when_ready([this, container, ret](bool bOk) {

      if (bOk) 
        this->container = container;
      ret.get_promise()->set_value(bOk);
    });

    return ret;
  }

  // addBlob 
  virtual Future<bool> addBlob(SharedPtr<NetService> service, String blob_name, Blob blob, Aborted aborted = Aborted()) override
  {
    auto ret = Promise<bool>().get_future();

    setContainer(service, blob_name, aborted).when_ready([this, ret, service, blob, blob_name, aborted](bool bOk)
    {
      if (!bOk)
      {
        ret.get_promise()->set_value(false);
        return;
      }

      NetRequest request(this->url.toString() + blob_name, "PUT");
      request.aborted = aborted;
      request.body = blob.body;
      request.setContentLength(blob.body->c_size());
      request.setContentType(blob.content_type);

      //metadata
      for (auto it : blob.metadata)
        request.setHeader("x-amz-meta-" + it.first, it.second);

      signRequest(request);

      NetService::push(service, request).when_ready([ret](NetResponse response) {
        bool bOk = response.isSuccessful();
        ret.get_promise()->set_value(bOk);
      });
    });

    return ret;
  }

  // getBlob 
  virtual Future<Blob> getBlob(SharedPtr<NetService> service, String blob_name, Aborted aborted = Aborted()) override
  {
    auto ret = Promise<Blob>().get_future();

    setContainer(service, blob_name, aborted).when_ready([this, ret, service, blob_name, aborted](bool bOk) {

      if (!bOk)
      {
        ret.get_promise()->set_value(Blob());
        return;
      }

      NetRequest request(this->url.toString() + blob_name, "GET");
      request.aborted = aborted;
      signRequest(request);

      NetService::push(service, request).when_ready([ret](NetResponse response) {

        Blob blob;

        if (response.isSuccessful())
        {
          //parse metadata
          String metatata_prefix = "x-amz-meta-";
          for (auto it = response.headers.begin(); it != response.headers.end(); it++)
          {
            String name = it->first;
            if (StringUtils::startsWith(name, metatata_prefix))
            {
              name = StringUtils::replaceAll(name.substr(metatata_prefix.length()), "_", "-"); //backward compatibility
              blob.metadata.setValue(name, it->second);
            }
          }

          blob.body = response.body;

          auto content_type = response.getContentType();
          if (!content_type.empty())
            blob.content_type = content_type;
        }

        ret.get_promise()->set_value(blob);
      });
    });

    return ret;
  }

  // deleteBlob
  virtual Future<bool> deleteBlob(SharedPtr<NetService> service, String blob_name, Aborted aborted = Aborted()) override
  {
    NetRequest request(this->url.toString() + blob_name, "DELETE");
    request.aborted = aborted;
    signRequest(request);

    auto ret = Promise<bool>().get_future();

    NetService::push(service, request).when_ready([ret](NetResponse response) {
      ret.get_promise()->set_value(response.isSuccessful());
    });

    return ret;
  }


};


////////////////////////////////////////////////////////////////////////////////
class GoogleDriveStorage : public CloudStorage
{
public:

  VISUS_CLASS(GoogleDriveStorage)

  String container_name;
  String container_id;

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

  //constructor
  GoogleDriveStorage(Url url) 
  {
    this->refresh_token = url.getParam("refresh_token"); VisusAssert(!refresh_token.empty());
    this->client_id = url.getParam("client_id"); VisusAssert(!client_id.empty());
    this->client_secret = url.getParam("client_secret"); VisusAssert(!client_secret.empty());

    String path = url.getPath();
    VisusAssert(!path.empty() && path[0] == '/');
    int index = (int)path.find('/', 1);
    this->container_name = (index >= 0) ? path.substr(1, index - 1) : path.substr(1);

    generateAccessToken();

    this->container_id = getContainerId();
  }

  //destructor
  virtual ~GoogleDriveStorage() {
  }

  //accessTokenValid
  bool accessTokenValid() const {
    return !this->access_token.value.empty() && this->access_token.t1.elapsedSec() < 0.85* this->access_token.expires_in;
  }

  //generateAccessToken
  void generateAccessToken()
  {
    this->access_token.value.clear();

    NetRequest request(Url("https://www.googleapis.com/oauth2/v3/token"), "POST");
    request.setTextBody(StringUtils::format()
      << "client_id=" << this->client_id
      << "&client_secret=" << this->client_secret
      << "&refresh_token=" << this->refresh_token
      << "&grant_type=refresh_token");

    auto response = NetService::getNetResponse(request);
    if (!response.isSuccessful())
      return;

    auto json = nlohmann::json::parse(response.getTextBody());
    std::cout << json <<std::endl;

    this->access_token.t1 = Time::now();
    this->access_token.value = json["access_token"].get<std::string>();
    this->access_token.expires_in = json["expires_in"].get<int>();
  }


  //signRequest
  void signRequest(NetRequest& request)
  {
    if (!accessTokenValid())
      generateAccessToken();
    request.headers.setValue("Authorization", "Bearer " + this->access_token.value);
  }

  //getContainerId
  String getContainerId()
  {
    NetRequest request("https://www.googleapis.com/drive/v3/files?q=name='" + container_name + "'", "GET");
    signRequest(request);
    auto response=NetService::getNetResponse(request);

    //could be that does not exist
    if (!response.isSuccessful())
      return "";

    auto json = nlohmann::json::parse(response.getTextBody());
    auto ret = json["files"].size()? json["files"].at(0)["id"].get<std::string>() : String();
    return ret;
  }

  // addContainer
  Future<bool> addContainer(SharedPtr<NetService> service, Aborted aborted=Aborted()) 
  {
    NetRequest request(Url("https://www.googleapis.com/drive/v3/files"),"POST");
    request.aborted = aborted;
    request.setHeader("Content-Type","application/json");
    request.setTextBody("{'name':'"+this->container_name+"','mimeType':'application/vnd.google-apps.folder'}");
    signRequest(request);

    auto ret = Promise<bool>().get_future();

    NetService::push(service, request).when_ready([this,ret](NetResponse response) {

      bool bOk = response.isSuccessful();

      if (!bOk)
        VisusWarning() << "ERROR. Cannot create container status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
      else
      {
        VisusInfo() << "addContainer done";

        auto json = nlohmann::json::parse(response.getTextBody());
        this->container_id = json["id"].get<std::string>();
      }

      ret.get_promise()->set_value(bOk);
    });

    return ret;
  }

  // deleteContainer
  Future<bool> deleteContainer(SharedPtr<NetService> service, Aborted aborted = Aborted())  
  {
    auto ret = Promise<bool>().get_future();

    auto failed = [&] {
      ret.get_promise()->set_value(false);
      return ret;
    };

    if (container_id.empty())
      return failed();

    NetRequest request(Url("https://www.googleapis.com/drive/v3/files/"+container_id),"DELETE");
    request.aborted = aborted;
    signRequest(request);

    NetService::push(service, request).when_ready([this,ret](NetResponse response) {
      bool bOk = response.isSuccessful();
      if (!bOk)
        VisusWarning() << "ERROR. Cannot delete container status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
      else
      {
        VisusInfo() << "deleteContainer done";
        this->container_id = "";
      }

      ret.get_promise()->set_value(bOk);
    });

    return ret;
  }

  // addBlobRequest 
  virtual Future<bool> addBlob(SharedPtr<NetService> service, String name, Blob blob, Aborted aborted = Aborted()) override
  {
    auto ret = Promise<bool>().get_future();

    if (container_id.empty())
    {
      ret.get_promise()->set_value(false);
      return ret;
    }

    NetRequest request(Url("https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart"), "POST");
    request.aborted = aborted;

    String boundary = UUIDGenerator::getSingleton()->create();
    request.setHeader("Expect", "");
    request.setContentType("multipart/form-data; boundary=" + boundary);

    //write multiparts
    {
      request.body = std::make_shared<HeapMemory>();

      OutputBinaryStream out(*request.body);

      //multipart 1
      out << "--" << boundary << "\r\n";
      out << "Content-Disposition: form-data; name=\"metadata\"\r\n";
      out << "Content-Type: application/json;charset=UTF-8;\r\n";
      out << "\r\n";
      out << "{'name':'" << name << "','parents':['" << container_id << "'], 'properties': {";
      bool need_sep = false; 
      for (auto it : blob.metadata)
      {
        out << (need_sep? "," : "" ) << "'" << it.first << "': '" << it.second << "'";
        need_sep=true;
      }
      out << "}}" << "\r\n";

      //multipart2
      out << "--" << boundary << "\r\n";
      out << "Content-Disposition: form-data; name=\"file\"\r\n";
      out << "Content-Type: " + blob.content_type + "\r\n";
      out << "\r\n";
      out << *blob.body << "\r\n";

      //multipart end
      out << "--" << boundary << "--";

      request.setContentLength(request.body->c_size());
    }

    signRequest(request);

    NetService::push(service, request).when_ready([this, ret](NetResponse response) {
      bool bOk = response.isSuccessful();
      if (!bOk)
        VisusWarning() << "ERROR. Cannot addBlob status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
      ret.get_promise()->set_value(bOk);
    });

    return ret;
  }

  // getBlob 
  virtual Future<Blob> getBlob(SharedPtr<NetService> service, String name, Aborted aborted = Aborted()) override
  {
    auto ret = Promise<Blob> ().get_future();

    if (container_id.empty())
    {
      ret.get_promise()->set_value(Blob());
      return ret;
    }

    NetRequest get_blob_id("https://www.googleapis.com/drive/v3/files?q=name='" + name + "' and '" + container_id + "' in parents", "GET");
    get_blob_id.aborted = aborted;
    signRequest(get_blob_id);

    NetService::push(service, get_blob_id).when_ready([this, service, ret, aborted](NetResponse response) {

      if (!response.isSuccessful())
      {
        VisusWarning() << "ERROR. Cannot get blob status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
        ret.get_promise()->set_value(Blob());
        return;
      }

      auto json = nlohmann::json::parse(response.getTextBody());
      auto blob_id = json["files"].size() ? json["files"].at(0)["id"].get<std::string>() : String();
      if (blob_id.empty())
      {
        ret.get_promise()->set_value(Blob());
        return;
      }

      NetRequest get_blob_metadata(Url("https://www.googleapis.com/drive/v3/files/" + blob_id + "?fields=id,name,mimeType,properties"), "GET");
      get_blob_metadata.aborted = aborted;
      signRequest(get_blob_metadata);

      NetService::push(service, get_blob_metadata).when_ready([this, ret, service, blob_id,aborted](NetResponse response) {

        bool bOk = response.isSuccessful();
        if (!bOk)
        {
          VisusWarning() << "ERROR. Cannot get blob status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
          ret.get_promise()->set_value(Blob());
          return;
        }

        auto metadata = StringMap();
        auto json = nlohmann::json::parse(response.getTextBody());
        for (auto it : json["properties"].get<nlohmann::json::object_t>())
        {
          auto key=it.first;
          auto value= it.second.get<String>();
          metadata.setValue(key, value);
        }

        NetRequest get_blob_media(Url("https://www.googleapis.com/drive/v3/files/" + blob_id + "?alt=media"), "GET");
        get_blob_media.aborted = aborted;
        signRequest(get_blob_media);

        NetService::push(service, get_blob_media).when_ready([this, ret, aborted, metadata](NetResponse response) {

          bool bOk = response.isSuccessful();
          if (!bOk)
          {
            VisusWarning() << "ERROR. Cannot get blob status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
            ret.get_promise()->set_value(Blob());
            return;
          }

          auto content_type = response.getContentType();

          Blob blob;
          blob.metadata = metadata;
          blob.body = response.body;
          if (!content_type.empty())
            blob.content_type = content_type;

          ret.get_promise()->set_value(blob);
        });
      });

    });

    return ret;
  }


  // deleteBlob
  virtual Future<bool> deleteBlob(SharedPtr<NetService> service, String name, Aborted aborted = Aborted()) override
  {
    auto ret = Promise<bool>().get_future();

    if (container_id.empty())
    {
      ret.get_promise()->set_value(false);
      return ret;
    }

    NetRequest get_blob_id("https://www.googleapis.com/drive/v3/files?q=name='" + name + "' and '" + container_id + "' in parents", "GET");
    get_blob_id.aborted = aborted;
    signRequest(get_blob_id);

    NetService::push(service, get_blob_id).when_ready([this, service, ret, aborted](NetResponse response) {

      if (!response.isSuccessful())
      {
        VisusWarning() << "ERROR. Cannot delete blob status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
        ret.get_promise()->set_value(false);
        return;
      }

      auto json = nlohmann::json::parse(response.getTextBody());
      auto blob_id = json["files"].size() ? json["files"].at(0)["id"].get<std::string>() : String();
      if (blob_id.empty())
      {
        ret.get_promise()->set_value(false);
        return;
      }

      NetRequest delete_blob(Url("https://www.googleapis.com/drive/v3/files/" + blob_id), "DELETE");
      delete_blob.aborted = aborted;
      signRequest(delete_blob);

      NetService::push(service, delete_blob).when_ready([this, ret](NetResponse response) {

        bool bOk = response.isSuccessful();
        if (!bOk)
          VisusWarning() << "ERROR. Cannot delete container status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
        else
          VisusInfo() << "deleteBlob done";

        ret.get_promise()->set_value(bOk);
      });

    });

    return ret;
  }

};

/////////////////////////////////////////////////////////////////////////////////////////////////////
SharedPtr<CloudStorage> CloudStorage::createInstance(Url url)
{
  if (StringUtils::contains(url.getHostname(), "core.windows"))
    return std::make_shared<AzureCloudStorage>(url);

  if (StringUtils::contains(url.getHostname(), "s3.amazonaws"))
    return std::make_shared<AmazonCloudStorage>(url);

  if (StringUtils::contains(url.getHostname(), "googleapis"))
    return std::make_shared<GoogleDriveStorage>(url);

  return SharedPtr<CloudStorage>();
}



} //namespace Visus
