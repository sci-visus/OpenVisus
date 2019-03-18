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
#include <Visus/BlockQuery.h>
#include <Visus/Frustum.h>

namespace Visus {

//predeclaration
class DatasetFilter;
class Dataset;


////////////////////////////////////////////////////////
class VISUS_DB_API Query 
{
public:

  VISUS_NON_COPYABLE_CLASS(Query)

  //see mergeQueries
  enum MergeMode
  {
    InsertSamples,
    InterpolateSamples
  };


  Aborted    aborted;

  Field      field;
  double     time = 0;

  Array      buffer;

  NdPoint    nsamples;
  LogicBox   logic_box;

  //-1 guess progression
  //0 means that you want to see only the final resolution
  //>0 set some progression
  enum Progression
  {
    GuessProgression=-1,
    NoProgression=0
  };

  //I estimate internally (see estimateEndH) what is the max resolution the user can see on the screen (depending also on the view frustum)
  //you can ask to see less samples than the estimation by using a negative number
  //you can ask to see more samples than the estimation by using a positive number
  enum Quality
  {
    DefaultQuality=0,
  };

  int                        mode='r';
  MergeMode                  merge_mode=InsertSamples;
  Position                   position;
  Frustum                    viewdep;
  Position                   clipping;
  std::function<void(Array)> incrementalPublish;

  /*
    DatasetBitmask bitmask("V012{012}*);
    V012012012012012012012012012012012...... 
    0-----------------------------max
             |         |
    start    current   end
             |.......->|    
  */
  
  int              start_resolution=0;
  int              cur_resolution=-1;
  std::vector<int> end_resolutions;
  int              max_resolution=0;
  int              query_cursor=-1; 

  class VISUS_DB_API Filter
  {
  public:
    bool                     enabled = false;
    SharedPtr<DatasetFilter> value;
    NdBox                    domain;
  };

  Filter filter;

  //aligned_box (internal use only)
  NdBox aligned_box;

  //filter_query (internal use only)
  SharedPtr<Query> filter_query;

  //(internal use only)
#if !SWIG
  class AddressConversion {
  public:
    virtual ~AddressConversion() {}
  };
  CriticalSection              address_conversion_lock;
  SharedPtr<AddressConversion> address_conversion;
#endif

  struct
  {
    String  name;
    Array   BUFFER;
    Array   ALPHA;
    Matrix  PIXEL_TO_LOGIC;
    Matrix  LOGIC_TO_PIXEL;
    Point3d logic_centroid;

    SharedPtr<Access> access;
  }
  down_info;

  std::map<String, SharedPtr<Query> >  down_queries;

  // for point queries
  SharedPtr<HeapMemory> point_coordinates;

  //constructor
  Query(Dataset* dataset,int mode);

  //destructor
  virtual ~Query() {
  }

  //getByteSize
  Int64 getByteSize() const {
    return field.dtype.getByteSize(nsamples);
  }


  //isPointQuery
  bool isPointQuery() const {
    return point_coordinates ? true : false;
  }

  //isRunning
  bool isRunning() const {
    return status==QueryRunning;
  }

  //setRunning
  void setRunning() {
    VisusAssert(status==QueryCreated);
    this->status=QueryRunning;
  }

  //failed
  bool failed() const {
    return status==QueryFailed;
  }

  //getLastErrorMsg
  String getLastErrorMsg() const {
    return errormsg;
  }

  //ok
  bool ok() const {
    return status==QueryOk;
  }

  //setFailed
  void setFailed(String msg) 
  {
    if (failed()) return;
    VisusAssert(status==QueryCreated || status==QueryRunning || status==QueryOk);
    this->query_cursor=(int)end_resolutions.size();
    this->status=QueryFailed;
    this->errormsg=msg;
  }

  //setOk
  void setOk()
  {
    VisusAssert(status==QueryRunning);
    this->query_cursor=(int)end_resolutions.size();
    this->status=QueryOk;
    this->errormsg="";
  }

  //canBegin
  bool canBegin() const {
    return status==QueryCreated;
  }

  //canNext
  bool canNext() const {
    return status==QueryRunning && cur_resolution==end_resolutions[query_cursor];
  }

  //canExecute
  bool canExecute() const {
    return status==QueryRunning && cur_resolution<end_resolutions[query_cursor];
  }

  //getEndResolution
  int getEndResolution() const {

    if (status!=QueryRunning) return -1;
    VisusAssert(query_cursor>=0 && query_cursor<end_resolutions.size());
    return end_resolutions[query_cursor];
  }

  //currentLevelReady
  void currentLevelReady() 
  {
    VisusAssert(status==QueryRunning);
    VisusAssert(this->buffer.dims==this->nsamples);
    VisusAssert(query_cursor>=0 && query_cursor<end_resolutions.size());
    this->buffer.bounds   = this->position;
    this->buffer.clipping = this->clipping;
    this->cur_resolution=end_resolutions[query_cursor];
  }

  //allocateBufferIfNeeded
  bool allocateBufferIfNeeded();

public:

  //mergeSamples
  static bool mergeSamples(LogicBox wbox, Array& wbuffer, LogicBox rbox, Array rbuffer, int merge_mode, Aborted aborted);

  //mergeSamples
  static bool mergeSamples(Query& write, Query& read, int merge_mode, Aborted aborted) {
    return mergeSamples(write.logic_box, write.buffer, read.logic_box, read.buffer, merge_mode, aborted);
  }

private:

  String errormsg="";

  QueryStatus status = QueryCreated;

};



} //namespace Visus

#endif //__VISUS_QUERY_H

