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
#include <Visus/LogicBox.h>
#include <Visus/Async.h>

namespace Visus {


//////////////////////////////////////////////////////////////////////////
enum QueryStatus
{
  QueryCreated = 0,
  QueryRunning,
  QueryFailed,
  QueryOk
};


///////////////////////////////////////////////////////////////////////////////////////
class VISUS_DB_API BlockQuery 
{
public:

  VISUS_NON_COPYABLE_CLASS(BlockQuery)

  Aborted    aborted;

  Field      field;
  double     time = 0;

  Array      buffer;

  NdPoint    nsamples;
  LogicBox   logic_box;

  Future<Void> done;

  BigInt start_address=0;
  BigInt end_address=0;

  //constructor
  BlockQuery(Field field_,double time_,BigInt start_address_,BigInt end_address_,Aborted aborted_) 
    : field(field_), time(time_), aborted(aborted_),start_address(start_address_),end_address(end_address_) {
    done = Promise<Void>().get_future();
  }

  //destructor
  virtual ~BlockQuery() {
  }


  //getByteSize
  Int64 getByteSize() const {
    return field.dtype.getByteSize(nsamples);
  }


  //getBlockNumber
  BigInt getBlockNumber(int bitsperblock) const {
    return start_address >> bitsperblock;
  }

  //setRunning
  void setRunning() {
    VisusAssert(status == QueryCreated);
    this->status = QueryRunning;
  }

  //ok
  bool ok() const {
    VisusAssert(status == QueryOk || status == QueryFailed); //call only when the blockquery is done
    return status == QueryOk;
  }

  //setOk
  void setOk() {
    VisusAssert(this->status == QueryCreated || this->status == QueryRunning);
    this->status = QueryOk;
    this->done.get_promise()->set_value(Void());
  }

  //failed
  bool failed() const {
    VisusAssert(status == QueryOk || status == QueryFailed); //call only when the blockquery is done
    return status == QueryFailed;
  }

  //setOk
  void setFailed() {
    VisusAssert(this->status == QueryCreated || this->status == QueryRunning);
    this->status = QueryFailed;
    this->done.get_promise()->set_value(Void());
  }

  //allocateBufferIfNeeded
  bool allocateBufferIfNeeded();

private:

  QueryStatus status = QueryCreated;

};


//predeclaration
class Dataset;
class Access;




} //namespace Visus

#endif //__VISUS_BLOCK_QUERY_H

