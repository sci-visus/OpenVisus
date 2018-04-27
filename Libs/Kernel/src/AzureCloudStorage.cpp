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

#include <Visus/AzureCloudStorage.h>

#include <cctype>

namespace Visus {

//////////////////////////////////////////////////////////////////////////////////
bool AzureCloudStorage::signRequest(NetRequest& request)
{
  String password=needRequestParam(request,"password",/*bBaseDecodeBase64*/true,/*bRemoveParamFromRequest*/true);
  if (password.empty()) return false;

  String username=request.url.getHostname().substr(0,request.url.getHostname().find('.'));

  String canonicalized_resource="/" + username + request.url.getPath();
  if (!request.url.params.empty())
  {
    std::ostringstream out;
    for(auto it=request.url.params.begin();it!=request.url.params.end();++it)
      out<<"\n"<<it->first<<":"<<it->second;    
    canonicalized_resource+=out.str();
  }

  String date_GTM=getDateGTM();
  request.setHeader("x-ms-version","2011-08-18");
  request.setHeader("x-ms-date"   ,date_GTM);

  String canonicalized_headers;
  {
    std::ostringstream out;
    int N=0;for (auto it=request.headers.begin();it!=request.headers.end();it++)
    {
      if (StringUtils::startsWith(it->first,"x-ms-"))
        out<<(N++?"\n":"")<<StringUtils::toLower(it->first)<<":"<<it->second;
    }
    canonicalized_headers=out.str();
  }

  String signature;
  signature += request.method+"\n";// Verb
  signature += request.getHeader("Content-Encoding"   ) + "\n";
  signature += request.getHeader("Content-Language"   ) + "\n";
  signature += request.getHeader("Content-Length"     ) + "\n";
  signature += request.getHeader("Content-MD5"        ) + "\n";
  signature += request.getHeader("Content-Type"       ) + "\n";
  signature += request.getHeader("Date"               ) + "\n";
  signature += request.getHeader("If-Modified-Since"  ) + "\n";
  signature += request.getHeader("If-Match"           ) + "\n";
  signature += request.getHeader("If-None-Match"      ) + "\n";
  signature += request.getHeader("If-Unmodified-Since") + "\n";
  signature += request.getHeader("Range"              ) + "\n";
  signature += canonicalized_headers                    + "\n";
  signature += canonicalized_resource;

  signature=StringUtils::base64Encode(StringUtils::sha256(signature,password));

  request.setHeader("Authorization","SharedKey " + username + ":" + signature);
  return true;
}


//////////////////////////////////////////////////////////////////////////////////
NetRequest AzureCloudStorage::createListOfContainersRequest(Url url)
{
  url.params.setValue("comp","list");
  NetRequest request(url);
  signRequest(request);
  return request;
}

//////////////////////////////////////////////////////////////////////////////////
StringTree AzureCloudStorage::getListOfContainers(NetResponse response)
{
  VisusAssert(response.isSuccessful());
  StringTree ret;
  if (!ret.loadFromXml(response.getTextBody())) return StringTree();
  return ret;
}

//////////////////////////////////////////////////////////////////////////////////
NetRequest AzureCloudStorage::createListOfBlobsRequest(Url url)
{
  url.params.setValue("comp","list");
  url.params.setValue("restype","container");
  NetRequest request(url);
  signRequest(request);
  return request;
}

//////////////////////////////////////////////////////////////////////////////////
StringTree AzureCloudStorage::getListOfBlobs(NetResponse response)
{
  VisusAssert(response.isSuccessful());
  StringTree ret;
  if (!ret.loadFromXml(response.getTextBody())) return StringTree();
  return ret;
}


//////////////////////////////////////////////////////////////////////////////////
NetRequest AzureCloudStorage::createCreateContainerRequest(Url url,bool isPublic)
{
  VisusAssert(isGoodContainerName(getContainerName(url)));
  url.params.setValue("restype","container");
  NetRequest request(url);
  request.method="PUT";
  request.setContentLength(0);
  if (isPublic) request.setHeader("x-ms-prop-publicaccess","true");
  signRequest(request);
  return request;
}

//////////////////////////////////////////////////////////////////////////////////
NetRequest AzureCloudStorage::createDeleteContainerRequest(Url url)
{
  url.params.setValue("restype","container");
  NetRequest request(url);
  request.method="DELETE";
  signRequest(request);
  return request;
}

//////////////////////////////////////////////////////////////////////////////////
NetRequest AzureCloudStorage::createDeleteBlobRequest(Url url)
{
  NetRequest request(url);
  request.method="DELETE";
  signRequest(request);
  return request;
}

//////////////////////////////////////////////////////////////////////////////////
NetRequest AzureCloudStorage::createGetBlobRequest(Url url)
{
  NetRequest request(url);
  request.method="GET";
  signRequest(request);
  return request;
}

//////////////////////////////////////////////////////////////////////////////////
NetRequest AzureCloudStorage::createHasBlobRequest(Url url)
{
  NetRequest request(url);
  request.method="HEAD";
  signRequest(request);
  return request;
}


//////////////////////////////////////////////////////////////////////////////////
NetRequest AzureCloudStorage::createAddBlobRequest(Url url, SharedPtr<HeapMemory> blob, StringMap metadata, String content_type)
{
  VisusAssert(isGoodContainerName(getContainerName(url)));
  VisusAssert(isGoodBlobName     (getBlobName     (url)));
  NetRequest request(url);
  request.method="PUT";
  request.body=blob;
  request.setContentLength(blob->c_size());
  request.setHeader("x-ms-blob-type","BlockBlob");
  request.setContentType(content_type);

  for (auto it=metadata.begin();it!=metadata.end();it++)
  {
    String meta_name =it->first;
    String meta_value=it->second;
    VisusAssert(isGoodMetaName(meta_name));
    request.setHeader(getMetaDataPrefix()+meta_name,meta_value);
  }

  signRequest(request);
  return request;
}


//////////////////////////////////////////////////////////////////////////////////
StringMap AzureCloudStorage::getMetadata(NetResponse response) 
{
  StringMap ret;
  String metatata_prefix=getMetaDataPrefix();
  for (auto it=response.headers.begin();it!=response.headers.end();it++)
  {
    String name=it->first;
    if (StringUtils::startsWith(name,metatata_prefix))
    {
      name=StringUtils::replaceAll(name.substr(metatata_prefix.length()),"_","-"); //trick: azure does not allow the "-" 
      ret.setValue(name,it->second);
    }
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
bool AzureCloudStorage::isGoodMetaName(String meta_name)
{
  //for azure : StringMap names must adhere to the naming rules for C# identifiers
  //amazon  does not seem to have any particular rule

  for (int K=0;K<(int)meta_name.size();K++)
    if (!(std::isalpha(meta_name[K]) || (K>0 && std::isdigit(meta_name[K])) || (K>0 && meta_name[K]=='_'))) return false;
  return true;
}

//////////////////////////////////////////////////////////////////////////////////
void AzureCloudStorage::setMetadata(NetRequest& request,const StringMap& metadata)
{
  for (auto it=metadata.begin();it!=metadata.end();it++)
  {
    String meta_name =StringUtils::replaceAll(it->first,"-","_"); //trick: azure does not allow the "-" 
    String meta_value=it->second;
    VisusAssert(isGoodMetaName(meta_name));
    request.setHeader(getMetaDataPrefix()+meta_name,meta_value);
  }
}


} //namespace Visus
