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

  DType            dtype;
  Point2i          tile_nsamples;
  String           tile_compression;

  //constructor
  GoogleMapsDataset() {
  }

  //destructor
  virtual ~GoogleMapsDataset() {
  }

  //getTypeName
  virtual String getTypeName() const override {
    return "GoogleMapsDataset";
  }

  //getTileCoordinate
  Point3i getTileCoordinate(BigInt start_address,BigInt end_address);

public:

  //openFromUrl 
  virtual bool openFromUrl(Url url) override;

  //guessEndResolutions
  virtual std::vector<int> guessEndResolutions(const Frustum& viewdep, Position position, Query::Quality quality = Query::DefaultQuality, Query::Progression progression = Query::GuessProgression) override;

  //createAccess
  virtual SharedPtr<Access> createAccess(StringTree config=StringTree(), bool bForBlockQuery = false) override;

  //getAddressRangeBox
  virtual LogicBox getAddressRangeBox(BigInt start_address,BigInt end_address) override;

  //beginQuery
  virtual bool beginQuery(SharedPtr<Query> query) override;

  //executeQuery
  virtual bool executeQuery(SharedPtr<Access> access,SharedPtr<Query> query) override;

  //nextQuery
  virtual bool nextQuery(SharedPtr<Query> query) override;

  //mergeWithBlockQuery
  virtual bool mergeQueryWithBlock(SharedPtr<Query> query,SharedPtr<BlockQuery> blockquery) override;

  //getLevelBox
  virtual LogicBox getLevelBox(int H) override;

private:

  //setCurrentEndResolution
  bool setCurrentEndResolution(SharedPtr<Query> query);

  //kdTraverse
  void kdTraverse(std::vector< SharedPtr<BlockQuery> >& block_queries,SharedPtr<Query> query,NdBox box,BigInt id,int H,int end_resolution);

};


} //namespace Visus

#endif //__VISUS_DB_GOOGLE_MAPS_DATASET_H


