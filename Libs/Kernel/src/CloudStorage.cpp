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

  String container_url;
  String storage_account_name;
  String access_key;

  //constructor
  AzureCloudStorage(Url url) 
  {
    this->access_key = url.getParam("access_key");
    VisusAssert(!access_key.empty());
    this->access_key = StringUtils::base64Decode(access_key);

    this->storage_account_name =url.getHostname().substr(0, url.getHostname().find('.'));

    String path = url.getPath();
    int index = (int)path.find('/', 1);
    String container_name = (index >= 0) ? path.substr(1, index - 1) : path.substr(1);

    this->container_url = url.getProtocol() + "://" + url.getHostname() + "/" + container_name;
  }

  //destructor
  virtual ~AzureCloudStorage() {
  }

  //signRequest
  void signRequest(NetRequest& request)
  {
    String canonicalized_resource = "/" + this->storage_account_name + request.url.getPath();

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

    request.setHeader("Authorization", "SharedKey " + storage_account_name + ":" + signature);
  }

  // createContainer
  virtual bool createContainer() override
  {
    NetRequest request(container_url, "PUT");
    request.url.params.setValue("restype", "container");
    request.setContentLength(0);
    //request.setHeader("x-ms-prop-publicaccess", "container"); IF YOU WANT PUBLIC
    signRequest(request);

    auto response = NetService::getNetResponse(request);
    if (!response.isSuccessful())
    {
      VisusWarning() << "ERROR. Cannot create container status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
      return false;
    }
    VisusInfo() << "createContainer done";
    return true;
  }

  // deleteContainer
  virtual bool deleteContainer() override
  {
    NetRequest request(container_url, "DELETE");
    request.url.params.setValue("restype", "container");
    signRequest(request);

    auto response = NetService::getNetResponse(request);
    if (!response.isSuccessful())
    {
      VisusWarning() << "ERROR. Cannot delete container status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
      return false;
    }
    VisusInfo() << "deleteContainer done";
    return true;
  }

  // addBlobRequest 
  virtual NetRequest addBlobRequest(String blob_name, Blob blob) override
  {
    NetRequest request(container_url + "/" + blob_name, "PUT");
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
      if (StringUtils::contains(name,"-"))
        name=StringUtils::replaceAll(name, "-", "_");

      request.setHeader("x-ms-meta-" + name, value);
    }

    signRequest(request);

    return request;
  }

  // deleteBlob
  virtual bool deleteBlob(String blob_name) override
  {
    NetRequest request(container_url + "/" + blob_name, "DELETE");
    signRequest(request);

    auto response = NetService::getNetResponse(request);
    if (!response.isSuccessful())
    {
      VisusWarning() << "ERROR Cannot delete block status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
      return false;
    }
    VisusInfo() << "deleteBlob done";
    return true;
  }

  // getBlobRequest 
  virtual NetRequest getBlobRequest(String blob_name) override
  {
    NetRequest request(container_url + "/" + blob_name, "GET");
    signRequest(request);
    return request;
  }

  //parseMetadata
  virtual StringMap parseMetadata(NetResponse response) override
  {
    StringMap ret;

    String metatata_prefix = "x-ms-meta-";
    for (auto it = response.headers.begin(); it != response.headers.end(); it++)
    {
      String name = it->first;
      if (StringUtils::startsWith(name, metatata_prefix))
      {
        name = name.substr(metatata_prefix.length());

        //trick: azure does not allow the "-" 
        if (StringUtils::contains(name,"_"))
          name = StringUtils::replaceAll(name, "_", "-"); 

        ret.setValue(name, it->second);
      }
    }

    return ret;
  }


};


////////////////////////////////////////////////////////////////////////////////
class AmazonCloudStorage : public CloudStorage
{
public:

  VISUS_CLASS(AmazonCloudStorage)

  String container_url;
  String username;
  String password;

  //constructor
  AmazonCloudStorage(Url url)  
  {
    this->username = url.getParam("username");
    VisusAssert(!this->username.empty());

    this->password = url.getParam("password");
    VisusAssert(!this->password.empty());
    this->password = StringUtils::base64Decode(password);

    //http://host.something.com/container_name/blob_name path=/container_name/blob_name
    String path = url.getPath();
    VisusAssert(!path.empty() && path[0] == '/');
    int index = (int)path.find('/', 1);
    String container_name = (index >= 0) ? path.substr(1, index - 1) : path.substr(1);

    this->container_url = url.getProtocol() + "://" + url.getHostname() + "/" + container_name;
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

  // createContainerRequest
  virtual bool createContainer() override
  {
    NetRequest request(this->container_url, "PUT");
    request.url.setPath(request.url.getPath() + "/"); //IMPORTANT the "/" to indicate is a container! see http://www.bucketexplorer.com/documentation/amazon-s3--how-to-create-a-folder.html
    signRequest(request);

    auto response = NetService::getNetResponse(request);
    if (!response.isSuccessful())
    {
      VisusWarning() << "ERROR. Cannot create container status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
      return false;
    }
    VisusInfo() << "createContainer done";
    return true;
  }

  // deleteContainer
  virtual bool deleteContainer() override
  {
    NetRequest request(this->container_url, "DELETE");
    request.url.setPath(request.url.getPath() + "/"); //IMPORTANT the "/" to indicate is a container!
    signRequest(request);

    auto response = NetService::getNetResponse(request);
    if (!response.isSuccessful())
    {
      VisusWarning() << "ERROR. Cannot delete container status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
      return false;
    }
    VisusInfo() << "deleteContainer done";
    return true;
  }

  // addBlobRequest 
  virtual NetRequest addBlobRequest(String blob_name, Blob blob) override
  {
    NetRequest request(this->container_url+ "/" + blob_name, "PUT");
    request.body = blob.body;
    request.setContentLength(blob.body->c_size());
    request.setContentType(blob.content_type);

    //metadata
    for (auto it : blob.metadata)
      request.setHeader("x-amz-meta-" + it.first, it.second);

    signRequest(request);

    return request;
  }

  // deleteBlob
  virtual bool deleteBlob(String blob_name) override
  {
    NetRequest request(this->container_url + "/" + blob_name, "DELETE");
    signRequest(request);

    auto response = NetService::getNetResponse(request);
    if (!response.isSuccessful())
    {
      VisusWarning() << "ERROR Cannot delete block status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
      return false;
    }
    VisusInfo() << "deleteBlob done";
    return true;
  }

  // getBlobRequest 
  virtual NetRequest getBlobRequest(String blob_name) override
  {
    NetRequest request(this->container_url + "/" + blob_name, "GET");
    signRequest(request);
    return request;
  }

  //parseMetadata
  virtual StringMap parseMetadata(NetResponse response) override
  {
    StringMap ret;
    String metatata_prefix = "x-amz-meta-";
    for (auto it = response.headers.begin(); it != response.headers.end(); it++)
    {
      String name = it->first;
      if (StringUtils::startsWith(name, metatata_prefix))
      {
        name = StringUtils::replaceAll(name.substr(metatata_prefix.length()), "_", "-"); //backward compatibility
        ret.setValue(name, it->second);
      }
    }
    return ret;
  }


};


////////////////////////////////////////////////////////////////////////////////
class GoogleDriveStorage : public CloudStorage
{
public:

  VISUS_CLASS(GoogleDriveStorage)

  String container_id;

  String client_id;
  String client_secret;
  String access_token;
  String refresh_token;

  //constructor
  GoogleDriveStorage(Url url) 
  {
    String path = url.getPath();
    VisusAssert(!path.empty() && path[0] == '/');
    int index = (int)path.find('/', 1);
    String container_name = (index >= 0) ? path.substr(1, index - 1) : path.substr(1);
    this->container_id = getContainerId(container_name);

    this->access_token   = url.getParam("access_token"); VisusAssert(!access_token.empty());
    this->refresh_token  = url.getParam("refresh_token"); VisusAssert(!refresh_token.empty());
    this->client_id      = url.getParam("client_id"); VisusAssert(!client_id.empty());
    this->client_secret  = url.getParam("client_secret"); VisusAssert(!client_secret.empty());
  }

  //destructor
  virtual ~GoogleDriveStorage() {
  }

  //signRequest
  void signRequest(NetRequest& request)
  {
    request.headers.setValue("Authorization", "Bearer " + access_token);
  }

  //getContainerId
  String getContainerId(String container_name)
  {
    NetRequest request("https://www.googleapis.com/drive/v3/files?q=name='" + container_name + "'", "GET");
    signRequest(request);
    auto response=NetService::getNetResponse(request);
    if (!response.isSuccessful())
      return "";

    auto json = nlohmann::json::parse(response.getTextBody());
    auto container_id =json["files"][0]["id"].get<std::string>();
    return container_id;
  }

  //getBlobId
  String getBlobId(String blob_name)
  {
    NetRequest request("https://www.googleapis.com/drive/v3/files?q=name='" + blob_name + "' container_id in parents", "GET");
    signRequest(request);
    auto response = NetService::getNetResponse(request);
    if (!response.isSuccessful())
      return "";

    auto json = nlohmann::json::parse(response.getTextBody());
    auto blob_id = json["files"][0]["id"].get<std::string>();
    return blob_id;
  }

  // createContainer
  virtual bool createContainer() override
  {
    NetRequest request(Url("https://www.googleapis.com/drive/v3/files"),"POST");
    request.setHeader("Content-Type","application/json");
    request.setTextBody("{'name':'Folder1000','mimeType':'application/vnd.google-apps.folder'}");
    signRequest(request);

    auto response = NetService::getNetResponse(request);
    if (!response.isSuccessful())
    {
      VisusWarning() << "ERROR. Cannot create container status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
      return false;
    }

    auto json = nlohmann::json::parse(response.getTextBody());
    this->container_id = json["id"].get<std::string>();

    VisusInfo() << "createContainer done";
    return true;
  }

  // deleteContainer
  virtual bool deleteContainer() override
  {
    if (container_id.empty())
      return false;

    NetRequest request(Url("https://www.googleapis.com/drive/v3/files/"+container_id),"DELETE");
    signRequest(request);

    auto response = NetService::getNetResponse(request);
    if (!response.isSuccessful())
    {
      VisusWarning() << "ERROR. Cannot delete container status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
      return false;
    }
    VisusInfo() << "deleteContainer done";
    return true;
  }

  // addBlobRequest 
  virtual NetRequest addBlobRequest(String blob_name, Blob blob) override
  {
    NetRequest request("aaa");
    request.method = "PUT";
    request.body = blob.body;
    request.setContentLength(blob.body->c_size());
    request.setContentType(blob.content_type);

    //metadata
    for (auto it : blob.metadata)
      request.setHeader("x-amz-meta-" + it.first, it.second);

    signRequest(request);
    return request;
  }

  // deleteBlob
  virtual bool deleteBlob(String blob_name) override
  {
    auto blob_id=getBlobId(blob_name);
    if (blob_id.empty())
      return false;

    NetRequest request(Url("https://www.googleapis.com/drive/v3/files/" + blob_id), "DELETE");
    signRequest(request);

    auto response = NetService::getNetResponse(request);
    if (!response.isSuccessful())
    {
      VisusWarning() << "ERROR. Cannot delete container status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
      return false;
    }
    VisusInfo() << "deleteContainer done";
    return true;
  }

  // getBlobRequest 
  virtual NetRequest getBlobRequest(String blob_name) override
  {
    auto blob_id = getBlobId(blob_name);
    NetRequest request(Url("https://www.googleapis.com/drive/v3/files/" + blob_id + "?fields=id,name,mimeType,properties"),"GET");
    signRequest(request);

    //NetRequest request(Url("https://www.googleapis.com/drive/v3/files/" + blob_id + "?alt=media"), "GET");
    //signRequest(request);

    return request;
  }


  //parseMetadata
  virtual StringMap parseMetadata(NetResponse response) override
  {
    return StringMap();
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


/////////////////////////////////////////////////////////////////////////////////////////////////////
bool CloudStorage::addBlob(String blob_name, Blob blob)
{
  auto response = NetService::getNetResponse(addBlobRequest(blob_name, blob));
  if (!response.isSuccessful())
  {
    VisusWarning() << "ERROR. Cannot create blob status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
    return false;
  }
  VisusInfo() << "addBlob done";
  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
CloudStorage::Blob CloudStorage::getBlob(String name)
{
  auto response = NetService::getNetResponse(getBlobRequest(name));
  if (!response.isSuccessful())
  {
    VisusWarning() << "ERROR Cannot get blob status(" << response.status << "),errormsg(" << response.getErrorMessage() << ")";
    return CloudStorage::Blob();
  }

  Blob ret;
  ret.body = response.body;
  ret.metadata = parseMetadata(response);
  return ret;
}


} //namespace Visus
