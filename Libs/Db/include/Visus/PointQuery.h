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

#ifndef __VISUS_POINTQUERY_H
#define __VISUS_POINTQUERY_H

#include <Visus/Db.h>
#include <Visus/Query.h>
#include <Visus/Frustum.h>

namespace Visus {

//predeclaration
class Dataset;

////////////////////////////////////////////////////////
class VISUS_DB_API PointQuery : public Query
{
public:

  VISUS_NON_COPYABLE_CLASS(PointQuery)

  int                        start_resolution = 0;
  std::vector<int>           end_resolutions;

  Position                   logic_position;
  Frustum                    logic_to_screen;
  Position                   logic_clipping;

  PointNi                    nsamples; //available only after beginQuery

  int                        cur_resolution = -1;
  int                        running_cursor = -1;

  // for point queries
  SharedPtr<HeapMemory>      points;

  //constructor
  PointQuery(Dataset* dataset, Field field, double time, int mode, Aborted aborted = Aborted())
    : Query(dataset, field, time, mode, aborted)
  {
    this->points = std::make_shared<HeapMemory>();
  }

  //destructor
  virtual ~PointQuery() {
  }

  //getNumberOfSamples
  virtual PointNi getNumberOfSamples() const override {
    VisusAssert(isRunning());
    return nsamples;
  }

  //setStatus
  virtual void setStatus(QueryStatus status) override
  {
    Query::setStatus(status);
    if (status == QueryOk || status == QueryFailed)
      this->running_cursor = -1;
  }

  //canNext
  bool canNext() const {
    return isRunning() && cur_resolution == end_resolutions[running_cursor];
  }

  //canExecute
  bool canExecute() const {
    return isRunning() && cur_resolution < end_resolutions[running_cursor];
  }

  //getEndResolution
  int getEndResolution() const {

    if (!isRunning()) return -1;
    VisusAssert(running_cursor >= 0 && running_cursor < end_resolutions.size());
    return end_resolutions[running_cursor];
  }

  //setCurrentLevelReady
  void setCurrentLevelReady();

};


} //namespace Visus

#endif //__VISUS_POINTQUERY_H

