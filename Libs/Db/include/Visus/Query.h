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
class Access;


////////////////////////////////////////////////////////
class VISUS_DB_API Query 
{
public:

  VISUS_NON_COPYABLE_CLASS(Query)

  /*
    DatasetBitmask
    V012012012012012012012012012012012
    0-----------------------------max
              |         |
    start    current   end
              |.......->|
  */

  //-1 guess progression
  //0 means that you want to see only the final resolution
  //>0 set some progression
  enum Progression
  {
    GuessProgression = -1,
    NoProgression = 0
  };

  //I estimate internally (see estimateEndH) what is the max resolution the user can see on the screen (depending also on the view frustum)
  //you can ask to see less samples than the estimation by using a negative number
  //you can ask to see more samples than the estimation by using a positive number
  enum Quality
  {
    DefaultQuality = 0,
  };

  //see mergeQueries
  enum MergeMode
  {
    InsertSamples,
    InterpolateSamples
  };


  Dataset*                   dataset = nullptr;
  Field                      field;
  double                     time = 0;
  int                        mode = 0;
  Aborted                    aborted;

  int                        start_resolution = 0;
  std::vector<int>           end_resolutions;
  MergeMode                  merge_mode = InsertSamples;
  std::function<void(Array)> incrementalPublish;

  Position                   logic_position;
  Frustum                    logic_to_screen;
  Position                   logic_clipping;

  PointNi                    nsamples; //available only after beginQuery
  Array                      buffer;
  int                        cur_resolution = -1;
  int                        running_cursor = -1;

  // for box queries
#if !SWIG
  struct
  {
    BoxNi    logic_aligned_box;
    LogicBox logic_box;
  }
  box_query;
#endif

  // for point queries
#if !SWIG
  struct
  {
    SharedPtr<HeapMemory> coordinates;
  }
  point_query;
#endif

  //for idx
#if !SWIG
  struct
  {
    bool                     enabled = false;
    SharedPtr<DatasetFilter> dataset_filter;
    BoxNi                    domain;

    //filter_query (internal use only)
    SharedPtr<Query> query;
  }
  filter;
#endif

  //for midx
#if !SWIG
  struct
  {
    String  name;
    Array   BUFFER;
    Matrix  PIXEL_TO_LOGIC;
    Matrix  LOGIC_TO_PIXEL;
    PointNd logic_centroid;

    SharedPtr<Access> access;
  }
  down_info;
  std::map<String, SharedPtr<Query> >  down_queries;
#endif
  
  //constructor
  Query(Dataset* dataset, Field field,double time, int mode,Aborted aborted=Aborted());

  //constructor
  Query(Dataset* dataset,int mode, Aborted aborted = Aborted());

  //destructor
  virtual ~Query() {
  }

  //getByteSize
  Int64 getByteSize() const {
    return field.dtype.getByteSize(nsamples);
  }

  //isPointQuery
  bool isPointQuery() const;

  //isBoxQuery
  bool isBoxQuery() const {
    return !isPointQuery();
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
  bool setFailed(String msg) 
  {
    if (failed()) return false;
    VisusAssert(status==QueryCreated || status==QueryRunning || status==QueryOk);
    this->running_cursor =-1;
    this->status=QueryFailed;
    this->errormsg=msg;
    return false;
  }

  //setOk
  bool setOk()
  {
    VisusAssert(status==QueryRunning);
    this->running_cursor = -1;
    this->status=QueryOk;
    this->errormsg="";
    return true;
  }

  //canBegin
  bool canBegin() const {
    return status==QueryCreated;
  }

  //canNext
  bool canNext() const {
    return status==QueryRunning && cur_resolution==end_resolutions[running_cursor];
  }

  //canExecute
  bool canExecute() const {
    return status==QueryRunning && cur_resolution<end_resolutions[running_cursor];
  }

  //getEndResolution
  int getEndResolution() const {

    if (status!=QueryRunning) return -1;
    VisusAssert(running_cursor >=0 && running_cursor <end_resolutions.size());
    return end_resolutions[running_cursor];
  }

  //setCurrentLevelReady
  void setCurrentLevelReady();

  //allocateBufferIfNeeded
  bool allocateBufferIfNeeded();

public:

  //mergeSamples
  static bool mergeSamples(LogicBox wbox, Array& wbuffer, LogicBox rbox, Array rbuffer, int merge_mode, Aborted aborted);

  //mergeSamples
  static bool mergeSamples(Query& write, Query& read, int merge_mode, Aborted aborted) {
    return mergeSamples(write.box_query.logic_box, write.buffer, read.box_query.logic_box, read.buffer, merge_mode, aborted);
  }

private:

  String errormsg="";

  QueryStatus status = QueryCreated;

};



} //namespace Visus

#endif //__VISUS_QUERY_H

