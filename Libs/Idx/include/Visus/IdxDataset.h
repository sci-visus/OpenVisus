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

#ifndef __VISUS_IDX_DATASET_H
#define __VISUS_IDX_DATASET_H

#include <Visus/Idx.h>
#include <Visus/Dataset.h>
#include <Visus/IdxFile.h>
#include <Visus/IdxHzOrder.h>

namespace Visus {

#if !SWIG
class IdxBoxQueryHzAddressConversion;
class IdxPointQueryHzAddressConversion;
#endif

  //////////////////////////////////////////////////////////////////////
class VISUS_IDX_API IdxDataset  : public Dataset 
{
public:

  VISUS_NON_COPYABLE_CLASS(IdxDataset)

  //idxfile
  IdxFile idxfile;

  SharedPtr<IdxBoxQueryHzAddressConversion> hzaddress_conversion_boxquery;

  // to speed up point queries
  // But keep in mind that this "preprocessing" is slow and can consume  a lot of memory. 
  // So use only when stricly necessary! 
  SharedPtr<IdxPointQueryHzAddressConversion> hzaddress_conversion_pointquery;

  //default constructor
  IdxDataset();

  //destructor
  virtual ~IdxDataset();

  //getTypeName
  virtual String getTypeName() const override {
    return "IdxDataset";
  }

  //loadDataset
  static SharedPtr<IdxDataset> loadDataset(String url) {
    return std::dynamic_pointer_cast<IdxDataset>(Dataset::loadDataset(url));
  }

  //tryRemoveLockAndCorruptedBinaryFiles
  static void tryRemoveLockAndCorruptedBinaryFiles(String directory = "");

  // removeFiles all files bolonging to this visus file 
  void removeFiles(int maxh = -1);

  //compress
  virtual bool compress(String compression) override;

  //getAddressRangeBox
  LogicBox getAddressRangeBox(BigInt start_address,BigInt end_address, int max_resolution);

  //getLevelBox
  LogicBox getLevelBox(HzOrder& hzorder, int H);

  //getLevelBox
  virtual LogicBox getLevelBox(int H) override {
    auto hzorder=HzOrder(bitmask,getMaxResolution());
    return getLevelBox(hzorder,H);
  }

  //adjustFilterBox
  NdBox adjustFilterBox(Query* query,DatasetFilter* filter,NdBox box,int H);

  //createEquivalentQuery
  SharedPtr<Query> createEquivalentQuery(int mode,SharedPtr<BlockQuery> block_query);

  //create
  static SharedPtr<IdxDataset> create(String filename, Array data, IdxFile idxfile = IdxFile());

  //setIdxFile
  void setIdxFile(IdxFile value);

  //special function for mosaic idx
  SharedPtr<IdxDataset> cloneForMosaic() const;

public:

  //openFromUrl
  virtual bool openFromUrl(Url url) override;

  //createAccess
  virtual SharedPtr<Access> createAccess(StringTree config=StringTree(), bool bForBlockQuery = false) override;

  //getAddressRangeBox
  virtual LogicBox getAddressRangeBox(BigInt start_address,BigInt end_address) override {
    return getAddressRangeBox(start_address,end_address,-1);
  }

  //convertBlockQueryToRowMajor
  virtual bool convertBlockQueryToRowMajor(SharedPtr<BlockQuery> block_query) override;

  //createQueryFilter
  virtual SharedPtr<DatasetFilter> createQueryFilter(const Field& field) override;

  //beginQuery
  virtual bool beginQuery(SharedPtr<Query> query) override;

  //executeQuery
  virtual bool executeQuery(SharedPtr<Access> access,SharedPtr<Query> query) override;

  //nextQuery
  virtual bool nextQuery(SharedPtr<Query> query) override;

  //mergeQueryWithBlock
  virtual bool mergeQueryWithBlock(SharedPtr<Query> query,SharedPtr<BlockQuery> block_query) override;

  //createNetRequest
  virtual NetRequest createPureRemoteQueryNetRequest(SharedPtr<Query> query) override;

private:

  //guessPointQueryNumberOfSamples
  NdPoint guessPointQueryNumberOfSamples(Position position, const Frustum& viewdep, int end_resolution);

  //setCurrentEndResolution
  bool setCurrentEndResolution(SharedPtr<Query> query) {
    return query->isPointQuery()?
      setPointQueryCurrentEndResolution(query) : 
      setBoxQueryCurrentEndResolution(query);
  }

  //setBoxQueryCurrentEndResolution
  bool setBoxQueryCurrentEndResolution(SharedPtr<Query> query);

  //setPointQueryCurrentEndResolution
  bool setPointQueryCurrentEndResolution(SharedPtr<Query> query);

  //executeBoxQuery
  bool executeBoxQueryWithAccess(SharedPtr<Access> access,SharedPtr<Query> query) ;

  //executePointQuery
  bool executePointQueryWithAccess(SharedPtr<Access> access,SharedPtr<Query> query) ;

};

} //namespace Visus


#endif //__VISUS_IDX_DATASET_H

