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

#ifndef __VISUS_DB_GOOGLE_MAPS_DATASET_H
#define __VISUS_DB_GOOGLE_MAPS_DATASET_H

#include <Visus/Db.h>
#include <Visus/Dataset.h>

namespace Visus {


////////////////////////////////////////////////////////
class VISUS_DB_API GoogleMapsDataset : public Dataset
{
public:

  VISUS_NON_COPYABLE_CLASS(GoogleMapsDataset)

  String           tiles;
  DType            dtype;
  Int64            tile_width=0;
  Int64            tile_height = 0;
  int              nlevels = 22;
  String           compression;

  //constructor
  GoogleMapsDataset() {
  }

  //destructor
  virtual ~GoogleMapsDataset() {
  }

  //castFrom
  static SharedPtr<GoogleMapsDataset> castFrom(SharedPtr<Dataset> db) {
    return std::dynamic_pointer_cast<GoogleMapsDataset>(db);
  }

  //getDatasetTypeName
  virtual String getDatasetTypeName() const override {
    return "GoogleMapsDataset";
  }

  //getBlockCoordinate
  Point3i getBlockCoordinate(BigInt blockid);

public:

  //readDatasetFromArchive 
  virtual void readDatasetFromArchive(Archive& ar) override;

  //createAccess
  virtual SharedPtr<Access> createAccess(StringTree config=StringTree(), bool bForBlockQuery = false) override;

  //getBlockSamples
  virtual LogicSamples getBlockSamples(BigInt blockid) override;

public:

  //beginBoxQuery
  virtual void beginBoxQuery(SharedPtr<BoxQuery> query) override;

  //nextBoxQuery
  virtual void nextBoxQuery(SharedPtr<BoxQuery> query) override;

  //executeBoxQuery
  virtual bool executeBoxQuery(SharedPtr<Access> access,SharedPtr<BoxQuery> query) override;

  //mergeBoxQueryWithBlockQuery
  virtual bool mergeBoxQueryWithBlockQuery(SharedPtr<BoxQuery> query,SharedPtr<BlockQuery> blockquery) override;

private:

  //getLevelSamples
  LogicSamples getLevelSamples(int H);

  //setBoxQueryEndResolution
  bool setBoxQueryEndResolution(SharedPtr<BoxQuery> query,int value);

  //kdTraverse
  void kdTraverse(std::vector< SharedPtr<BlockQuery> >& block_queries,SharedPtr<BoxQuery> query,BoxNi box,BigInt id,int H,int end_resolution);

};


} //namespace Visus

#endif //__VISUS_DB_GOOGLE_MAPS_DATASET_H


