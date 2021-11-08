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

#ifndef __VISUS_QUERY_H
#define __VISUS_QUERY_H

#include <Visus/Db.h>
#include <Visus/LogicSamples.h>
#include <Visus/Async.h>

namespace Visus {

//predeclaration
class Dataset;

///////////////////////////////////////////////////////////////////////////////////////
class VISUS_DB_API Query
{
public:

  enum
  {
    QueryCreated = 0,
    QueryRunning,
    QueryFailed,
    QueryOk
  };

  Dataset*     dataset = nullptr;
  int          mode = 0;
  Field        field;
  double       time = 0;
  double       accuracy = 0.0; //for idx2
  Aborted      aborted;

  Array        buffer;
  int          status = QueryCreated;
  String       errormsg;

  Future<Void> done;

  //constructor
  Query() {
    this->done = Promise<Void>().get_future();
  }

  //destructor
  ~Query() {
  }

  //getStatus
  int getStatus() const {
    return status;
  }

  //setStatus
  void setStatus(int value)
  {
    if (this->status == value) 
      return;

    this->status = value;

    //final status?
    if ((this->status == QueryOk) || (this->status == QueryFailed))
      this->done.get_promise()->set_value(Void());
  }

  //ok
  bool ok() const {
    return status == QueryOk;
  }

  //failed
  bool failed() const {
    return status == QueryFailed;
  }

  //running
  bool isRunning() const {
    return status == QueryRunning;
  }

  //setRunning
  void setRunning() {
    setStatus(QueryRunning);
  }

  //setOk
  void setOk() {
    setStatus(QueryOk);
  }

  //setOk
  void setFailed(String error_msg) {
    this->errormsg = error_msg;
    setStatus(QueryFailed);
  }


};

} //namespace Visus

#endif //__VISUS_QUERY_H

