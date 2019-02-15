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
#include <Visus/NetService.h>

namespace Visus {


////////////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API CloudStorage 
{
public:

  VISUS_CLASS(CloudStorage)

  //Typical url syntax: <host>/<container_name>/<blob_name> 
  //where for example:
  //  <host>      = visus.blob.core.windows.net | visus.s3.amazonaws.com
  //  <container> = 2kbit1
  //  <blob>      = block0001.bin


  //_____________________________________________________
  class VISUS_KERNEL_API Blob
  {
  public:
    SharedPtr<HeapMemory> body;
    StringMap metadata; 
    String content_type;

    //constructor
    Blob(SharedPtr<HeapMemory> body_= SharedPtr<HeapMemory>(), StringMap metadata_ = StringMap(), String content_type_= "application/octet-stream")
    : metadata(metadata_), body(body_), content_type(content_type_) {
    }

    //valid
    bool valid() const { 
      return body ? true : false; 
    }

    //operator==
    bool operator==(const Blob& b) const
    {
      const Blob& a = *this;
      return 
        (a.content_type == b.content_type) &&
        (a.metadata == b.metadata) &&
        ((!a.body && !b.body) || (a.body && b.body && a.body->c_size() == b.body->c_size() && memcmp(a.body->c_ptr(), b.body->c_ptr(), (size_t)a.body->c_size()) == 0));
    }
    
    //operator==
    bool operator!=(const Blob& b) const {
      return !(operator==(b));
    }

  };

  //constructor
  CloudStorage(){
  }

  //destructor
  virtual ~CloudStorage() {
  }

  //createInstance
  static SharedPtr<CloudStorage> createInstance(Url url);
  
  //addBlob
  virtual Future<bool> addBlob(SharedPtr<NetService> service, String name, Blob blob, Aborted aborted = Aborted())=0;

  //getBlob
  virtual Future<Blob> getBlob(SharedPtr<NetService> service, String name, Aborted aborted = Aborted()) = 0;

  // deleteBlob
  virtual Future<bool> deleteBlob(SharedPtr<NetService> service, String name, Aborted aborted = Aborted()) = 0;

};

} //namespace Visus


#endif //VISUS_CLOUD_STORAGE_




