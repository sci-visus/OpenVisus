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

#ifndef _VISUS_IDX_DATASET_2_H__
#define _VISUS_IDX_DATASET_2_H__

#include <Visus/Db.h>
#include <Visus/Dataset.h>
#include <Visus/IdxFile.h>
#include <Visus/IdxHzOrder.h>

#if VISUS_IDX2

namespace idx2 {
  struct idx2_file;
  struct params;
};


namespace Visus {

//////////////////////////////////////////////////////////////////////
class VISUS_DB_API IdxDiskAccess2 : public Access
{
public:

  //constructor
  IdxDiskAccess2() {
  }

  //destructor
  virtual ~IdxDiskAccess2() {
  }

  //readBlock
  virtual void readBlock(SharedPtr<BlockQuery> query)
  {
    ThrowException("TODO");
  }

  //writeBlock
  virtual void writeBlock(SharedPtr<BlockQuery> query)
  {
    ThrowException("TODO");
  }


};

//////////////////////////////////////////////////////////////////////
class VISUS_DB_API IdxDataset2 : public Dataset
{
public:

  //default constructor
  IdxDataset2();

  //destructor
  virtual ~IdxDataset2();

  //castFrom
  static SharedPtr<IdxDataset2> castFrom(SharedPtr<Dataset> db) {
    return std::dynamic_pointer_cast<IdxDataset2>(db);
  }

  //getDatasetTypeName
  virtual String getDatasetTypeName() const override {
    return "IdxDataset2";
  }
  
  //UseLegacyFileFormat
  static bool UseLegacyFileFormat() {
    return cbool(Utils::getEnv("VISUS_IDX2_USE_LEGACY_FILE_FORMAT", "0"));
  }

  //useLegacyFileFormat
  bool useLegacyFileFormat() const {
    Url url = this->getUrl();
    return IdxDataset2::UseLegacyFileFormat() || cbool(url.getParam("legacy", "0"));
  }

public:

  //block query stuff

  //createBlockQuery
  virtual SharedPtr<BlockQuery> createBlockQuery(BigInt blockid, Field field, double time, int mode = 'r', Aborted aborted = Aborted()) override;


  //getBlockQuerySamples
  LogicSamples getBlockQuerySamples(BigInt blockid, int& H) {
    ThrowException("not supported");
    return LogicSamples();
  }

  //executeBlockQuery
  void executeBlockQuery(SharedPtr<Access> access, SharedPtr<BlockQuery> query) override;

  //convertBlockQueryToRowMajor
  virtual bool convertBlockQueryToRowMajor(SharedPtr<BlockQuery> block_query) override {
    ThrowException("not supported");
    return false;
  }

  //createEquivalentBoxQuery
  virtual SharedPtr<BoxQuery> createEquivalentBoxQuery(int mode, SharedPtr<BlockQuery> block_query) override {
    ThrowException("not supported");
    return SharedPtr<BoxQuery>();
  }

public:

  //box query stuff

  //createBoxQuery
  virtual SharedPtr<BoxQuery> createBoxQuery(BoxNi logic_box, Field field, double time, int mode = 'r', Aborted aborted = Aborted()) override {
    return Dataset::createBoxQuery(logic_box, field, time, mode, aborted);
  }

  //guessBoxQueryEndResolution
  virtual int guessBoxQueryEndResolution(Frustum logic_to_screen, Position logic_position) override {
    return Dataset::guessBoxQueryEndResolution(logic_to_screen, logic_position);
  }

  //setBoxQueryEndResolution
  virtual bool setBoxQueryEndResolution(SharedPtr<BoxQuery> query, int H) override;

  //beginBoxQuery
  virtual void beginBoxQuery(SharedPtr<BoxQuery> query) override {
    return Dataset::beginBoxQuery(query);
  }

  //executeBoxQuery
  virtual bool executeBoxQuery(SharedPtr<Access> access, SharedPtr<BoxQuery> query) override;


  //nextBoxQuery
  virtual void nextBoxQuery(SharedPtr<BoxQuery> query) override;

  //createBlockQueriesForBoxQuery
  virtual std::vector<BigInt> createBlockQueriesForBoxQuery(SharedPtr<BoxQuery> query) override {
    ThrowException("not supported");
    return {};
  }

  //mergeBoxQueryWithBlockQuery
  virtual bool mergeBoxQueryWithBlockQuery(SharedPtr<BoxQuery> query, SharedPtr<BlockQuery> block_query) override {
    ThrowException("not supported");
    return false;
  }

  //createBoxQueryRequest
  virtual NetRequest createBoxQueryRequest(SharedPtr<BoxQuery> query) override {
    ThrowException("not supported");
    return NetRequest();
  }

  //executeBoxQueryOnServer
  virtual bool executeBoxQueryOnServer(SharedPtr<BoxQuery> query) override {
    ThrowException("not supported");
    return false;
  }


public:

  //point query stuff

  //constructor
  virtual SharedPtr<PointQuery> createPointQuery(Position logic_position, Field field, double time, Aborted aborted = Aborted()) {
    ThrowException("not supported");
    return SharedPtr<PointQuery>();
  }

  //createBlockQueriesForPointQuery
  virtual std::vector<BigInt> createBlockQueriesForPointQuery(SharedPtr<PointQuery> query) {
    ThrowException("not supported");
    return {};
  }


public:

  //enableExternalRead
  void enableExternalRead(idx2::idx2_file& Idx2, SharedPtr<Access> access, Aborted aborted=Aborted());


  //enableExternalRead
  void enableExternalWrite(idx2::idx2_file& Idx2, SharedPtr<Access> access, Aborted aborted = Aborted());

public:

  //readDatasetFromArchive
  virtual void readDatasetFromArchive(Archive& ar) override;

private:

  String           input_file;
  String           in_dir;
  String           metafile;

  void GetDecodeParams(idx2::params& P,SharedPtr<BoxQuery> query, int H);

};


} //namespace Visus


#endif 


#endif //_VISUS_IDX_DATASET_2_H__

