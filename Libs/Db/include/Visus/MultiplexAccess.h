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

#ifndef __VISUS_DB_MULTIPLEXACCESS_H
#define __VISUS_DB_MULTIPLEXACCESS_H

#include <Visus/Db.h>
#include <Visus/Access.h>
#include <Visus/ThreadPool.h>
#include <Visus/CriticalSection.h>

namespace Visus {

//predeclaration
class Dataset;

  ///////////////////////////////////////////////////////////////////////////////////////
class VISUS_DB_API MultiplexAccess : public Access
{
public:

  VISUS_NON_COPYABLE_CLASS(MultiplexAccess)

  Dataset* dataset = nullptr;

  //all the dw_access
  std::vector< SharedPtr<Access> > dw_access;

  //constructor
  MultiplexAccess(Dataset* dataset, StringTree config = StringTree());

  //destructor 
  virtual ~MultiplexAccess();

  //addChild
  void addChild(SharedPtr<Access> child, StringTree child_config);

  //readBlock 
  virtual void readBlock(SharedPtr<BlockQuery> up_query) override {
    scheduleOp('r', 0, up_query);
  }

  //writeBlock
  virtual void writeBlock(SharedPtr<BlockQuery> up_query) override {

    VisusAssert(false);//not supported
    writeFailed(up_query,"not supported");
  }

  //printStatistics
  virtual void printStatistics() override;

private:

  class Pending
  {
  public:
    int index = 0;
    SharedPtr<BlockQuery> up_query;
    SharedPtr<BlockQuery> dw_query;
  };

  std::vector<Pending> pendings;
  CriticalSection      lock;
  Semaphore            something_happened;
  bool                 bExit = false;

  class Filter
  {
  public:
    BigInt A=0,B=0,F=0,S=0;
  };

  std::vector<Filter> dw_filter;

  SharedPtr<std::thread> thread;

  //isGoodIndex
  bool isGoodIndex(int index) const {
    return index >= 0 && index < (int)dw_access.size();
  }

  //passThought
  bool passThought(int index, BigInt blockid);

  //scheduleOp
  void scheduleOp(int mode, int index, SharedPtr<BlockQuery> up_query);
  
  //runInBackground
  void runInBackground();


};

} //namespace Visus

#endif //__VISUS_DB_MULTIPLEXACCESS_H

