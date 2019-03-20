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

#include <Visus/Dataflow.h>
#include <Visus/DataflowMessage.h>
#include <Visus/DataflowNode.h>

namespace Visus {


///////////////////////////////////////////////////////////////////////////////
class ReturnReceipt::Waiting
{
public:
  SharedPtr<Semaphore> semaphore;

  //constructor
  Waiting(SharedPtr<Semaphore> semaphore_) : semaphore(semaphore_) 
  {}
  
  //destructor
  ~Waiting() 
  {semaphore->up();}

private:

  VISUS_NON_COPYABLE_CLASS(Waiting)
};


///////////////////////////////////////////////////////////////////////////////
class ReturnReceipt::ScopedSignature
{
public:
  SharedPtr<ReturnReceipt> target;Signer* signer;

  //constructor
  ScopedSignature(SharedPtr<ReturnReceipt> target_,Signer* signer_) : target(target_),signer(signer_) 
  {}
  
  //destructor
  ~ScopedSignature() 
  {target->addSignature(signer);}

private:

  VISUS_NON_COPYABLE_CLASS(ScopedSignature)
};

///////////////////////////////////////////////////////////////////////////////
ReturnReceipt::ReturnReceipt() 
{}

///////////////////////////////////////////////////////////////////////////////
ReturnReceipt::~ReturnReceipt()
{
  //could be that I didn't get some addSignature... 
  //nevermind, the ReturnReceipt is ready with some missing signatures
  waiting.clear();
  scoped_signature.clear();
}

///////////////////////////////////////////////////////////////////////////////
bool ReturnReceipt::isReady()
{
  ScopedLock lock(this->lock);
  return need_signature.empty();
}

///////////////////////////////////////////////////////////////////////////////
void ReturnReceipt::waitReady(SharedPtr<Semaphore> ready_semaphore)
{
  {
    ScopedLock lock(this->lock);
    if (need_signature.empty()) return;
    this->waiting.push_back(std::make_shared<Waiting>(ready_semaphore));
  }
  ready_semaphore->down();
}

 


///////////////////////////////////////////////////////////////////////////////
void ReturnReceipt::needSignature(Signer* signer)
{
  ScopedLock lock(this->lock);
  VisusAssert(need_signature.find(signer)==need_signature.end());
  need_signature.insert(signer);
}

///////////////////////////////////////////////////////////////////////////////
void ReturnReceipt::addSignature(Signer* signer)
{
  ScopedLock lock(this->lock);
  VisusAssert(need_signature.find(signer)!=need_signature.end());
  need_signature.erase(signer);
  if (need_signature.empty())
  {
    waiting.clear();
    scoped_signature.clear();
  }
}


///////////////////////////////////////////////////////////////////////////////
SharedPtr<ReturnReceipt> ReturnReceipt::createPassThroughtReceipt(Node* node)
{
  // next message to publish will depend on "data" input
  auto B=std::make_shared<ReturnReceipt>();

  for (auto it=node->inputs.begin();it!=node->inputs.end();it++)
  {
    DataflowPort* iport=it->second;
    if (DataflowPortValue* preview_value=iport->previewValue())
    {
      if (SharedPtr<ReturnReceipt> A=preview_value->return_receipt)
      {
        //B will sign A when B is ready
        //ScopedLock lock(B->lock);  don't need the signature here... I know for sure no other thread is using B
        A->needSignature(B.get());
        B->scoped_signature.push_back(std::make_shared<ScopedSignature>(A,B.get()));
      }
    }
  }
  
  return B;
}



} //namespace Visus 

