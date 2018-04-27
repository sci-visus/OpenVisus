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
#include <Visus/AmazonCloudStorage.h>
#include <Visus/AzureCloudStorage.h>
#include <Visus/NetService.h>
#include <Visus/Log.h>

#include <cctype>

#if WIN32
#pragma warning(disable:4996)
#endif

namespace Visus {


/////////////////////////////////////////////////////////////////////////////////////////////////////
CloudStorage* CloudStorage::createInstance(CloudStorageType type)
{
  if (type==UseAzureCloudStorage ) return new AzureCloudStorage ();
  if (type==UseAmazonCloudStorage) return new AmazonCloudStorage();
  return nullptr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
CloudStorage::CloudStorageType CloudStorage::guessType(Url url)
{
  CloudStorageType ret=DoNotUseCloudStorage;
  if      (StringUtils::contains(url.getHostname(),"core.windows")) ret=UseAzureCloudStorage;
  else if (StringUtils::contains(url.getHostname(),"s3.amazonaws")) ret=UseAmazonCloudStorage;
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
String CloudStorage::getDateGTM(const char* format)
{
  char ret[256];
  time_t t;time(&t);
  struct tm *ptm= gmtime (&t);
  strftime(ret,sizeof(ret),format,ptm);
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
String CloudStorage::needRequestParam(NetRequest& request,String name,bool bBaseDecodeBase64,bool bRemoveParamFromRequest)
{
  String ret=request.url.getParam(name); 
  if (ret.empty()) {VisusAssert(false);return "";}
  if (bRemoveParamFromRequest) request.url.params.eraseValue(name);
  return bBaseDecodeBase64? StringUtils::base64Decode(ret) : ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
String CloudStorage::getContainerName(Url url)
{
  //http://host.something.com/container_name/blob_name path=/container_name/blob_name
  String path=url.getPath(); 
  VisusAssert(!path.empty() && path[0]=='/');
  int index=(int)path.find('/',1); 
  String ret=(index>=0)? path.substr(1,index-1) : path.substr(1);
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
String CloudStorage::getBlobName(Url url)
{
  String path=url.getPath(); 
  VisusAssert(!path.empty() && path[0]=='/');
  int index=(int)path.find('/',1); VisusAssert(index>=0);
  String ret=path.substr(index+1);
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
bool CloudStorage::isGoodContainerName(String container_name)
{
  //isGoodContainerName
  // for azure  see: http://stackoverflow.com/questions/16446568/how-to-evaluate-valid-azure-blob-storage-container-names
  // for amazon see : ?

  bool ret=true;
  ret=ret && (container_name.size()>=3 && container_name.size()<=63);
  ret=ret && (StringUtils::toLower(container_name)==container_name);
  ret=ret && (std::isalnum(container_name[0]));
  ret=ret && (!StringUtils::startsWith(container_name,"-") && StringUtils::find(container_name,"--")<0 && !StringUtils::endsWith(container_name,"-"));
  for (int I=0;I<(int)container_name.size();I++) 
    ret=ret && (std::isalnum(container_name[I]) || container_name[I]=='-');
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
bool CloudStorage::isGoodBlobName(String blob_name)
{
  bool ret=true;
  ret=ret && (blob_name.size()>=1 && blob_name.size()<1024);
  ret=ret && (!StringUtils::endsWith(blob_name,"."));
  ret=ret && (!StringUtils::endsWith(blob_name,"/"));
  ret=ret && (StringUtils::find(blob_name,"..")<0);
  ret=ret && (StringUtils::find(blob_name,"//")<0);
  ret=ret && (StringUtils::find(blob_name,"./")<0);
  ret=ret && (StringUtils::find(blob_name,"/.")<0);
  return ret;
}


//////////////////////////////////////////////////////////////////////////////
bool CloudStorage::createContainer(Url url)
{
  auto request =createCreateContainerRequest(url);
  auto response=NetService::getNetResponse(request);
  if (!response.isSuccessful())
    {VisusWarning()<<"ERROR. Cannot create container status(" <<response.status<<"),errormsg("<<response.getErrorMessage()<<")";return false;}    
  VisusInfo()<<"createContainer done";
  return true;
}

//////////////////////////////////////////////////////////////////////////////
bool CloudStorage::deleteContainer(Url url)
{
  auto request =createDeleteContainerRequest(url);
  auto response=NetService::getNetResponse(request);
  if (!response.isSuccessful())
    {VisusWarning()<<"ERROR. Cannot delete container status(" <<response.status<<"),errormsg("<<response.getErrorMessage()<<")";return false;}
  VisusInfo()<<"deleteContainer done";
  return true;
}

//////////////////////////////////////////////////////////////////////////////
StringTree CloudStorage::listContainers(Url url)
{
  auto request =createListOfContainersRequest(url);
  auto response=NetService::getNetResponse(request);
  if (!response.isSuccessful())
    {VisusWarning()<<"ERROR Cannot list containers status(" <<response.status<<"),errormsg("<<response.getErrorMessage()<<")";return StringTree();}
  VisusInfo()<<"listContainers done";
  return getListOfContainers(response);
}

//////////////////////////////////////////////////////////////////////////////
bool CloudStorage::addBlob(Url dst_url,SharedPtr<HeapMemory> blob,StringMap metadata)
{
  auto request =createAddBlobRequest(dst_url,blob,metadata);
  auto response=NetService::getNetResponse(request);
  if (!response.isSuccessful())
    {VisusWarning()<<"ERROR. Cannot create blob status(" <<response.status<<"),errormsg("<<response.getErrorMessage()<<")";return false;}
  VisusInfo()<<"addBlob done";
  return true;
}


//////////////////////////////////////////////////////////////////////////////
bool CloudStorage::deleteBlob(Url url)
{
  auto request =createDeleteBlobRequest(url);
  auto response=NetService::getNetResponse(request);
  if (!response.isSuccessful())
    {VisusWarning()<<"ERROR Cannot delete block status(" <<response.status<<"),errormsg("<<response.getErrorMessage()<<")";return false;}
  VisusInfo()<<"deleteBlob done";
  return true;
}

//////////////////////////////////////////////////////////////////////////////
SharedPtr<HeapMemory> CloudStorage::getBlob(Url url,StringMap& meta_data)
{
  auto request =createGetBlobRequest(url);
  auto response=NetService::getNetResponse(request);

  if (!response.isSuccessful())
    {VisusWarning()<<"ERROR Cannot get blob status(" <<response.status<<"),errormsg("<<response.getErrorMessage()<<")";return SharedPtr<HeapMemory>();}

  meta_data=getMetadata(response);

  VisusInfo()<<"getBlob done";
  return response.body;
}

//////////////////////////////////////////////////////////////////////////////
StringTree CloudStorage::listOfBlobs(Url url)
{
  auto request =createListOfBlobsRequest(url);
  auto response=NetService::getNetResponse(request);
  if (!response.isSuccessful())
    {VisusWarning()<<"ERROR Cannot list blobs status(" <<response.status<<"),errormsg("<<response.getErrorMessage()<<")";return StringTree();}

  VisusInfo()<<"getListOfBlobs done";
  return getListOfBlobs(response);
}
  

} //namespace Visus
