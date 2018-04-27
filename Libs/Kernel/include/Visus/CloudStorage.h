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

#ifndef VISUS_CLOUD_STORAGE_
#define VISUS_CLOUD_STORAGE_

#include <Visus/Kernel.h>
#include <Visus/NetMessage.h>

namespace Visus {


////////////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API CloudStorage 
{
public:

  VISUS_CLASS(CloudStorage)

  //Typical url syntax: host/container/blob 
  //where for example:
  //  host      = visus.blob.core.windows.net | visus.s3.amazonaws.com
  //  container = 2kbit1
  //  blob      = block0001.bin

  enum CloudStorageType
  {
    DoNotUseCloudStorage=0,
    UseAzureCloudStorage,
    UseAmazonCloudStorage,
  };

  //destructor
  virtual ~CloudStorage()
  {}

  //createInstance
  static VISUS_NEWOBJECT(CloudStorage*) createInstance(CloudStorageType type);

  //guessType
  static CloudStorageType guessType(Url url);

  //createInstance
  static VISUS_NEWOBJECT(CloudStorage*) createInstance(Url url) {
    return createInstance(guessType(url));
  }
  
  // createListOfContainersRequest 
  virtual NetRequest createListOfContainersRequest(Url url)=0;
  
  //createCreateContainerRequest
  virtual NetRequest createListOfBlobsRequest(Url url)=0;
  
  // createCreateContainerRequest
  virtual NetRequest createCreateContainerRequest(Url url,bool isPublic=true)=0;

  // createDeleteContainerRequest
  virtual NetRequest createDeleteContainerRequest(Url url)=0;

  // createAddBlobRequest 
  virtual NetRequest createAddBlobRequest(Url url,SharedPtr<HeapMemory> blob,StringMap metadata,String content_type = "application/octet-stream")=0;

  // createDeleteBlobRequest
  virtual NetRequest createDeleteBlobRequest(Url url)=0;

  // createHasBlobRequest 
  virtual NetRequest createHasBlobRequest(Url url)=0;

  // createGetBlobRequest 
  virtual NetRequest createGetBlobRequest(Url url)=0;

  //getListOfContainers
  virtual StringTree getListOfContainers(NetResponse response)=0;

  //getListOfBlobs
  virtual StringTree getListOfBlobs(NetResponse response)=0;

  //getMetadata
  virtual StringMap getMetadata(NetResponse response)=0;

  //setMetadata
  virtual void setMetadata(NetRequest& request,const StringMap& metadata)=0;

public:

  //createContainer
  bool createContainer(Url url);

  //deleteContainer
  bool deleteContainer(Url url);

  //listContainers
  StringTree listContainers(Url url);

  //addBlob
  bool addBlob(Url dst_url,SharedPtr<HeapMemory> blob,StringMap meta_data);

  //deleteBlob
  bool deleteBlob(Url url);

  //getBlob
  SharedPtr<HeapMemory> getBlob(Url url,StringMap& meta_data);

  //listOfBlobs
  StringTree listOfBlobs(Url url);

protected:

  //getMetaDataPrefix
  virtual String getMetaDataPrefix()=0;

  //isGoodMetaName
  virtual bool isGoodMetaName(String meta_name)=0;

  //signRequest
  virtual bool signRequest(NetRequest& request)=0;

  //getDateGTM
  static String getDateGTM(const char* format="%a, %d %b %Y %H:%M:%S GMT");

  //isGoodContainerName
  static bool isGoodContainerName(String container_name);

  //isGoodBlobName
  static bool isGoodBlobName(String blob_name);
  
  //getContainerName
  static String getContainerName(Url url);

  //getBlobName
  static String getBlobName(Url url);

  //needRequestParam
  static String needRequestParam(NetRequest& request,String name,bool bBaseDecodeBase64,bool bRemoveParamFromRequest);

};





} //namespace Visus


#endif //VISUS_CLOUD_STORAGE_




