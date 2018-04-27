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

#include <Visus/AmazonCloudStorage.h>
#include <Visus/Path.h>

#include <cctype>

namespace Visus {

//////////////////////////////////////////////////////////////////////////////////
bool AmazonCloudStorage::signRequest(NetRequest& request)
{
  String username=needRequestParam(request,"username",/*bBaseDecodeBase64*/false,/*bRemoveParamFromRequest*/true); if (username.empty()) return false;
  String password=needRequestParam(request,"password",/*bBaseDecodeBase64*/true ,/*bRemoveParamFromRequest*/true); if (password.empty()) return false;

  String canonicalized_resource=request.url.getPath();
  int find_bucket=StringUtils::find(request.url.getHostname(),"s3.amazonaws.com"); VisusAssert(find_bucket>=0);
  String bucket_name=request.url.getHostname().substr(0,find_bucket);
  bucket_name=StringUtils::rtrim(bucket_name,".");
  if (!bucket_name.empty()) canonicalized_resource="/"+bucket_name + canonicalized_resource;

  String canonicalized_headers;
  {
    std::ostringstream out;
    for (auto it=request.headers.begin();it!=request.headers.end();it++)
    {
      if (StringUtils::startsWith(it->first,"x-amz-"))
        out<<StringUtils::toLower(it->first)<<":"<<it->second<<"\n";
    }
    canonicalized_headers=out.str();
  }

  String date_GTM=getDateGTM();
  String signature=request.method + "\n";
  signature += request.getHeader("Content-MD5" ) + "\n";
  signature += request.getContentType() + "\n";
  signature += date_GTM + "\n";  
  signature += canonicalized_headers;
  signature += canonicalized_resource;
  signature=StringUtils::base64Encode(StringUtils::sha1(signature,password));
  request.setHeader("Host"         ,request.url.getHostname());
  request.setHeader("Date"         ,date_GTM);
  request.setHeader("Authorization","AWS " + username + ":" + signature);
  return true;
}

//////////////////////////////////////////////////////////////////////////////////
NetRequest AmazonCloudStorage::createGetBlobRequest(Url url)
{
  NetRequest request(url);
  request.method="GET";
  signRequest(request);
  return request;
}

//////////////////////////////////////////////////////////////////////////////////
NetRequest AmazonCloudStorage::createHasBlobRequest(Url url)
{
  NetRequest request(url);
  request.method="HEAD";
  signRequest(request);
  return request;
}

//////////////////////////////////////////////////////////////////////////////////
NetRequest AmazonCloudStorage::createDeleteBlobRequest(Url url)
{
  NetRequest request(url);
  request.method="DELETE";
  signRequest(request);
  return request;
}

//////////////////////////////////////////////////////////////////////////////////
NetRequest AmazonCloudStorage::createListOfContainersRequest(Url url)
{
  //see http://docs.aws.amazon.com/AmazonS3/latest/API/RESTBucketGET.html
  url.setPath(Path("/"));
  url.params.setValue("max-keys",cstring(NumericLimits<int>::highest()));
  url.params.setValue("prefix"   ,""); //from the beginning up to the first "/"
  url.params.setValue("delimiter","/"); 
  NetRequest request(url);
  signRequest(request);
  return request;
}

//////////////////////////////////////////////////////////////////////////////////
StringTree AmazonCloudStorage::getListOfContainers(NetResponse response)
{
  VisusAssert(response.isSuccessful());
  StringTree stree;
  if (!stree.loadFromXml(response.getTextBody())) return StringTree();
  if (StringTree* IsTruncated=stree.findChildWithName("IsTruncated"))
    VisusAssert(cbool(IsTruncated->collapseTextAndCData())==false);
  return stree;
}

//////////////////////////////////////////////////////////////////////////////////
NetRequest AmazonCloudStorage::createListOfBlobsRequest(Url url)
{
  //see http://docs.aws.amazon.com/AmazonS3/latest/API/RESTBucketGET.html
  String path=url.getPath();
  VisusAssert(!path.empty() && StringUtils::startsWith(path,"/") && !StringUtils::endsWith(path,"/"));
  path=StringUtils::ltrim(path,"/") + "/";
  url.setPath(Path("/"));
  url.params.setValue("max-keys",cstring(NumericLimits<int>::highest()));
  url.params.setValue("prefix",path); //from path up to the end
  url.params.setValue("delimiter",""); 
  NetRequest request(url);
  signRequest(request);
  return request;
}

//////////////////////////////////////////////////////////////////////////////////
StringTree AmazonCloudStorage::getListOfBlobs(NetResponse response)
{
  VisusAssert(response.isSuccessful());
  StringTree ret;
  if (!ret.loadFromXml(response.getTextBody())) return StringTree();
  if (StringTree* IsTruncated=ret.findChildWithName("IsTruncated"))
    VisusAssert(cbool(IsTruncated->collapseTextAndCData())==false);
  return ret;
}

//////////////////////////////////////////////////////////////////////////////////
NetRequest AmazonCloudStorage::createCreateContainerRequest(Url url,bool isPublic)
{
  VisusAssert(isGoodContainerName(getContainerName(url)));
  NetRequest request(url);
  request.method="PUT";
  //if (isPublic) request.setHeader("x-amz-acl","public-read-write");
  request.url.setPath(url.getPath()+"/"); //IMPORTANT the "/" to indicate is a container! see http://www.bucketexplorer.com/documentation/amazon-s3--how-to-create-a-folder.html
  signRequest(request);
  return request;
}

//////////////////////////////////////////////////////////////////////////////////
NetRequest AmazonCloudStorage::createDeleteContainerRequest(Url url)
{
  NetRequest request(url);
  request.method="DELETE";
  
  request.url.setPath(url.getPath()+"/"); //IMPORTANT the "/" to indicate is a container!
  signRequest(request);
  return request;
}


//////////////////////////////////////////////////////////////////////////////////
NetRequest AmazonCloudStorage::createAddBlobRequest(Url url, SharedPtr<HeapMemory> blob, StringMap metadata,String content_type)
{
  VisusAssert(isGoodContainerName(getContainerName(url)));
  VisusAssert(isGoodBlobName     (getBlobName     (url)));

  NetRequest request(url);
  request.method="PUT";
  request.body=blob;
  request.setContentLength(blob->c_size());
  request.setContentType(content_type);
  setMetadata(request,metadata);

  signRequest(request);
  return request;
}

//////////////////////////////////////////////////////////////////////////////////
StringMap AmazonCloudStorage::getMetadata(NetResponse response) 
{
  StringMap ret;
  String metatata_prefix=getMetaDataPrefix();
  for (auto it=response.headers.begin();it!=response.headers.end();it++)
  {
    String name=it->first;
    if (StringUtils::startsWith(name,metatata_prefix))
    {
      name=StringUtils::replaceAll(name.substr(metatata_prefix.length()),"_","-"); //backward compatibility
      ret.setValue(name,it->second);
    }
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
bool AmazonCloudStorage::isGoodMetaName(String meta_name)
{
  //for azure : StringMap names must adhere to the naming rules for C# identifiers
  //amazon  does not seem to have any particular rule

  for (int K=0;K<(int)meta_name.size();K++)
    if (!(std::isalpha(meta_name[K]) || (K>0 && std::isdigit(meta_name[K])) || (K>0 && meta_name[K]=='-'))) return false;
  return true;
}

//////////////////////////////////////////////////////////////////////////////////
void AmazonCloudStorage::setMetadata(NetRequest& request,const StringMap& metadata)
{
  for (auto it=metadata.begin();it!=metadata.end();it++)
  {
    String meta_name =it->first;
    String meta_value=it->second;
    VisusAssert(isGoodMetaName(meta_name));
    request.setHeader(getMetaDataPrefix()+meta_name,meta_value);
  }
}


} //namespace Visus
