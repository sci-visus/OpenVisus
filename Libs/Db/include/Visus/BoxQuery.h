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

#ifndef __VISUS_BOXQUERY_H
#define __VISUS_BOXQUERY_H

#include <Visus/Db.h>
#include <Visus/Query.h>
#include <Visus/Frustum.h>

namespace Visus {

//predeclaration
class IdxFilter;
class Dataset;
class Access;

//-1 guess progression
//0 means that you want to see only the final resolution
//>0 set some progression
enum QueryProgression
{
  QueryGuessProgression = -1,
  QueryNoProgression = 0
};

//I estimate internally (see estimateEndH) what is the max resolution the user can see on the screen (depending also on the view frustum)
//you can ask to see less samples than the estimation by using a negative number
//you can ask to see more samples than the estimation by using a positive number
enum QueryQuality
{
  QueryDefaultQuality = 0,
};


////////////////////////////////////////////////////////
class VISUS_DB_API BoxQuery : public Query
{
public:

  VISUS_NON_COPYABLE_CLASS(BoxQuery)

  BoxNi                      logic_box;
  LogicSamples               logic_samples;

  int                        start_resolution = 0;
  int                        cur_resolution = -1;
  int                        end_resolution = -1;
  std::vector<int>           end_resolutions;

  //for idx
#if !SWIG
  struct
  {
    bool                     enabled = false;
    SharedPtr<IdxFilter>     dataset_filter;
    BoxNi                    domain;
    BoxNi                    adjusted_logic_box;
    SharedPtr<BoxQuery>      query;
  }
  filter;
#endif

  //for midx
#if !SWIG
  std::map<String, SharedPtr<BoxQuery> >  down_queries;
#endif

  //internal use only
#if !SWIG
  std::function<void(Array)> incrementalPublish;
#endif

  //internal use only

  //constructor
  BoxQuery() {
  }
  
  //canExecute
  bool canExecute() const {
    return status == QueryRunning && cur_resolution < end_resolution;
  }

  //getNumberOfSamples
  PointNi getNumberOfSamples() const {
    return
      canExecute() ? logic_samples.nsamples : buffer.dims;
  }

  //allocateBufferIfNeeded
  bool allocateBufferIfNeeded();

  //getByteSize
  Int64 getByteSize() const {
    return field.dtype.getByteSize(getNumberOfSamples());
  }

  //getCurrentResolution
  int getCurrentResolution() const {
    return this->cur_resolution;
  }

  //setCurrentResolution
  void setCurrentResolution(int value) {
    this->cur_resolution = value;
  }

  //getEndResolution
  int getEndResolution() const {
    return end_resolution;
  }

  //setResolutionRange
  void setResolutionRange(int A,int B) {
    this->start_resolution = A;
    this->end_resolutions = { B };
  }

  //disableFilters
  void disableFilters() {
    this->filter.enabled = false;
  }
  
  //enableFilters
  void enableFilters() {
    this->filter.enabled = true;
  }

};



} //namespace Visus

#endif //__VISUS_BOXQUERY_H

