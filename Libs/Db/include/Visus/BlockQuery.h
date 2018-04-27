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

#ifndef __VISUS_BLOCK_QUERY_H
#define __VISUS_BLOCK_QUERY_H

#include <Visus/Db.h>
#include <Visus/BaseQuery.h>
#include <Visus/Async.h>

namespace Visus {


///////////////////////////////////////////////////////////////////////////////////////
class VISUS_DB_API BlockQuery : public BaseQuery
{
public:

  VISUS_NON_COPYABLE_CLASS(BlockQuery)

  Future<bool> future;

  BigInt start_address=0;
  BigInt end_address=0;

  //constructor
  BlockQuery(Field field_,double time_,BigInt start_address_,BigInt end_address_,Aborted aborted_) 
    : BaseQuery(field_,time_,aborted_),start_address(start_address_),end_address(end_address_) {

    future = Promise<bool>().get_future();
  }

  //destructor
  virtual ~BlockQuery() {
  }

  //getBlockNumber
  BigInt getBlockNumber(int bitsperblock) const {
    return start_address >> bitsperblock;
  }

  //getStatus
  QueryStatus getStatus() 
  {  
    //client processing
    {
      ScopedLock lock(client_processing.lock);
      if (client_processing.value) 
      {
        VisusAssert(status==QueryCreated || status==QueryRunning);
        this->status=client_processing.value();
        client_processing.value=nullptr;
      }
    }
    VisusAssert(status==QueryOk || status==QueryFailed);
    return this->status;
  }

  //setClientProcessing
  void setClientProcessing(std::function<QueryStatus()> value) 
  {
    ScopedLock lock(client_processing.lock);
    VisusAssert(!client_processing.value);
    VisusAssert(status==QueryCreated || status==QueryRunning);
    client_processing.value=value;
  }

  //setStatus
  void setStatus(QueryStatus value) 
  {
    VisusAssert(!client_processing.value);

    VisusAssert(
      (status == QueryCreated && (value == QueryRunning || value == QueryFailed || value == QueryOk)) ||
      (status == QueryRunning && (                         value == QueryFailed || value == QueryOk)));

    this->status=value;

    if (status== QueryFailed || status== QueryOk)
      this->future.get_promise()->set_value(true);
  }

private:
    
  struct
  {
    CriticalSection              lock;
    std::function<QueryStatus()> value;
  }
  client_processing;
  
};


//predeclaration
class Dataset;
class Access;




} //namespace Visus

#endif //__VISUS_BLOCK_QUERY_H

