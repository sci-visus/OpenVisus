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


  Position              logic_position;

  PointNi               npoints;
  SharedPtr<HeapMemory> points = std::make_shared<HeapMemory>();

  int                   start_resolution = 0; //always zero
  int                   cur_resolution = -1;
  int                   end_resolution = -1;
  std::vector<int>      end_resolutions;

  //blockid -> (point-cloud-offset, block-offset)*
  typedef std::vector< std::pair<BigInt, BigInt> > Offsets;
  std::map< BigInt, SharedPtr< Offsets > > offsets;

  //constructor
  PointQuery() {
  }

  //getNumberOfPoints
  PointNi getNumberOfPoints() const {
    return npoints;
  }

  //getByteSize
  Int64 getByteSize() const {
    return field.dtype.getByteSize(getNumberOfPoints());
  }

  //setPoints
  bool setPoints(PointNi nsamples);


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

};


} //namespace Visus

#endif //__VISUS_POINTQUERY_H

