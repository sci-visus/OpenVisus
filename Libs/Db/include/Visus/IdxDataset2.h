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

#ifndef __VISUS_DB_IDX_DATASET2_H
#define __VISUS_DB_IDX_DATASET2_H


#if VISUS_IDX2

#include <Visus/Db.h>
#include <Visus/Dataset.h>
#include <Visus/IdxFile.h>
#include <Visus/IdxHzOrder.h>

#include <idx2_lib.h>

//TODO: cannot expose here
using namespace idx2;

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
  IdxDataset2() : idx2(nullptr) {
  }

  //destructor
  virtual ~IdxDataset2() {

    if (idx2)
    {
      Dealloc(idx2);
      delete idx2;
      idx2 = nullptr;
    }

  }

  //castFrom
  static SharedPtr<IdxDataset2> castFrom(SharedPtr<Dataset> db) {
    return std::dynamic_pointer_cast<IdxDataset2>(db);
  }

  //getDatasetTypeName
  virtual String getDatasetTypeName() const override {
    return "IdxDataset2";
  }

public:

  //createAccess (right now not needed)
  virtual SharedPtr<Access> createAccess(StringTree config = StringTree(), bool bForBlockQuery = false) override {
    return std::make_shared<IdxDiskAccess2>();
  }

public:

  //________________________________________________
  //block query stuff

  //createBlockQuery
  virtual SharedPtr<BlockQuery> createBlockQuery(BigInt blockid, Field field, double time, int mode = 'r', Aborted aborted = Aborted()) override {
    ThrowException("TODO");
    return SharedPtr<BlockQuery>();
  }


  //getBlockQuerySamples
  LogicSamples getBlockQuerySamples(BigInt blockid, int& H)
  {
    ThrowException("TODO");
    return LogicSamples();
  }

  //readBlock  
  virtual void executeBlockQuery(SharedPtr<Access> access, SharedPtr<BlockQuery> query) override 
  {
    ThrowException("TODO");
  }

  //convertBlockQueryToRowMajor
  virtual bool convertBlockQueryToRowMajor(SharedPtr<BlockQuery> block_query) override {
    ThrowException("TODO");
    return true;
  }

  //createEquivalentBoxQuery
  virtual SharedPtr<BoxQuery> createEquivalentBoxQuery(int mode, SharedPtr<BlockQuery> block_query) override
  {
    ThrowException("TODO");
    return SharedPtr<BoxQuery>();
  }

public:

  //________________________________________________
  //box query stuff

  //createBoxQuery
  virtual SharedPtr<BoxQuery> createBoxQuery(BoxNi logic_box, Field field, double time, int mode = 'r', Aborted aborted = Aborted()) override
  {
    return Dataset::createBoxQuery(logic_box, field, time, mode, aborted); //should be the same
  }

  //guessBoxQueryEndResolution
  virtual int guessBoxQueryEndResolution(Frustum logic_to_screen, Position logic_position) override
  {
    return Dataset::guessBoxQueryEndResolution(logic_to_screen, logic_position);
  }

  //fillParams
  LogicSamples fillParams(params& P, SharedPtr<BoxQuery> query, int H)
  {
    auto MaxH = getMaxResolution();

    P.Action = action::Decode;
    P.OutMode = params::out_mode::KeepInMemory; //in memory!

    //what is the logic_box/logic_samples in OpenVisus
    P.DecodeExtent = extent(Cast(query->logic_box.p1), Cast(query->logic_box.size())); //first, dims

    //euristic to map the resolution 'H' to level and mask (e.g. V012012 resolutions in the range [0,6] with 0 being the first V)
    /*
      OutputLevel
        fprintf(stderr, "Provide --level (0 means full resolution)\n");
        fprintf(stderr, "The decoder will not decode levels less than this (finer resolution levels)\n");
        fprintf(stderr, "Example: --level 0\n");
    */
    /*
      Mask
        fprintf(stderr, "Provide --mask (8-bit mask, 128 (0x80) means full resolution)\n");
        fprintf(stderr, "For example, if the volume is 256 x 256 x 256 and there are 2 levels\n");
        fprintf(stderr, "Level 0, mask 128 = 256 x 256 x 256\n"); 0 HALF DIM
        fprintf(stderr, "Level 0, mask 64  = 256 x 256 x 128\n"); 1 HALF DIM
        fprintf(stderr, "Level 0, mask 32  = 256 x 128 x 256\n"); 1 HALF DIM
        fprintf(stderr, "Level 0, mask 16  = 128 x 256 x 256\n"); 1 HALF DIM
        fprintf(stderr, "Level 0, mask 8   = 256 x 128 x 128\n"); 2 HALF DIM
        fprintf(stderr, "Level 0, mask 4   = 128 x 256 x 128\n"); 2 HALF DIM
        fprintf(stderr, "Level 0, mask 2   = 128 x 128 x 256\n"); 2 HALF DIM
        fprintf(stderr, "Level 0, mask 1   = 128 x 128 x 128\n"); 3 HALF DIM
        fprintf(stderr, "Level 1, mask 128 = 128 x 128 x 128\n"); ALL HALF DIMENSION
        fprintf(stderr, "and so on, until...");
        fprintf(stderr, "Level 1, mask 1   =  64 x
    */

    {
      P.OutputLevel = (MaxH - H) / 3;

      //I should start with a lower level
      if (P.OutputLevel > this->idx2->NLevels)
        return LogicSamples::invalid();

      P.DecodeMask = (MaxH - H) % 3;

      //TODO: !!!not sure how to set it!!!
      //      in the best case I want this to be consistent with IDX1 delta
      auto remainder = (MaxH - H) % 3;
      if (remainder == 0)
        P.DecodeMask = 128;
      else if (remainder == 1)
        P.DecodeMask = 64;
      else if (remainder == 2)
        P.DecodeMask = 8;
    }

    //TODO: do i need to strdup???
    //where is the data
    P.InputFile = this->idx2_url.c_str(); //keep in memory
    P.InDir = this->idx2_dir.c_str();     //keep in memory

    //todo
    P.DecodeAccuracy = query->accuracy;

    /* swap bit 3 and 4 */
    auto Mask = P.DecodeMask;
    {
      P.DecodeMask = Mask;
      if (BitSet(Mask, 3)) P.DecodeMask = SetBit(Mask, 4); else P.DecodeMask = UnsetBit(Mask, 4);
      if (BitSet(Mask, 4)) P.DecodeMask = SetBit(P.DecodeMask, 3); else P.DecodeMask = UnsetBit(P.DecodeMask, 3);
    }

    P.DecodeLevel = P.OutputLevel;

    u8 OutMask = (P.DecodeLevel == P.OutputLevel) ? P.DecodeMask : 128;
    grid OutGrid = GetGrid(P.DecodeExtent, P.OutputLevel, OutMask, this->idx2->Subbands);

    //print some debugging info
    v3i from = From(OutGrid);
    v3i dims = Dims(OutGrid); //is this the number of samples?
    v3i strd = Strd(OutGrid);

    /*
    000000000011111111112222222
    012345678901234567890123456
    ---------------------------
    V01012012012012012012012012
    */

    PrintInfo("//////////////////////////////////////////");
    PrintInfo("Bitmask", this->bitmask);
    PrintInfo("logic_box", query->logic_box);
    PrintInfo("H", H, "MaxH", MaxH, "Level", (int)P.OutputLevel, "Mask", (int)Mask);
    PrintInfo("from", Cast(from), "Dims", Cast(dims), "stride", Cast(strd));
    PrintInfo("Accuracy", P.DecodeAccuracy);

		auto logic_samples = LogicSamples(BoxNi(Cast(from), Cast(from + dims * strd)), Cast(strd));
    VisusReleaseAssert(logic_samples.logic_box.p1 == Cast(from));
    VisusReleaseAssert(logic_samples.nsamples == Cast(dims));
    VisusReleaseAssert(logic_samples.delta == Cast(strd));
    return logic_samples;
  }

  //setBoxQueryEndResolution
  virtual bool setBoxQueryEndResolution(SharedPtr<BoxQuery> query, int H) override
  {
    PrintInfo("IdxDataset2::setBoxQueryEndResolution");
    auto MaxH = this->bitmask.getMaxResolution();

    VisusReleaseAssert(H >= 0 && H < MaxH);
    VisusReleaseAssert(query->end_resolution < H);

    params P;
    auto logic_samples = fillParams(P, query, H);
    if (!logic_samples.valid())
      return false;

    query->logic_samples = logic_samples;
    query->end_resolution = H;
    return true;
  }

  //beginBoxQuery
  virtual void beginBoxQuery(SharedPtr<BoxQuery> query) override
  {
    PrintInfo("IdxDataset2::beginBoxQuery");

    if (!query)
      return;

    if (query->getStatus() != Query::QueryCreated)
      return;

    if (query->aborted())
      return query->setFailed("query aborted");

    if (!query->field.valid())
      return query->setFailed("field not valid");

    // override time from field
    if (query->field.hasParam("time"))
      query->time = cdouble(query->field.getParam("time"));

    if (!getTimesteps().containsTimestep(query->time))
      return query->setFailed("wrong time");

    if (!query->logic_box.valid())
      return query->setFailed("query logic_box not valid");

    if (!query->logic_box.getIntersection(this->getLogicBox()).isFullDim())
      return query->setFailed("user_box not valid");

    if (query->end_resolutions.empty())
      query->end_resolutions = { this->getMaxResolution() };


    for (auto it : query->end_resolutions)
    {
      if (!(it >= 0 && it<this->getMaxResolution()))
        return query->setFailed("wrong end resolution");
    }

    if (query->start_resolution > 0 && (query->end_resolutions.size() != 1 || query->start_resolution != query->end_resolutions[0]))
      return query->setFailed("wrong query start resolution");

    for (auto it : query->end_resolutions)
    {
      if (setBoxQueryEndResolution(query, it))
        return query->setRunning();
    }

    query->setFailed("cannot find a good end_resolution to start with");
  }


  //executeBoxQuery
  virtual bool executeBoxQuery(SharedPtr<Access> access, SharedPtr<BoxQuery> query) override
  {
    PrintInfo("IdxDataset2::executeBoxQuery");

    if (!query)
      return false;

    if (!(query->isRunning() && query->getCurrentResolution() < query->getEndResolution()))
      return false;

    if (query->aborted())
    {
      query->setFailed("query aborted");
      return false;
    }

    if (!query->allocateBufferIfNeeded())
    {
      query->setFailed("cannot allocate buffer");
      return false;
    }

    params P;
    auto logic_samples = fillParams(P, query, query->end_resolution);
    if (!logic_samples.valid())
      return false;

    VisusReleaseAssert(logic_samples==query->logic_samples);

    query->allocateBufferIfNeeded();
    VisusAssert(query->buffer.dims == query->getNumberOfSamples());

    auto buff = idx2::buffer((const byte*)query->buffer.c_ptr(), query->buffer.c_size());
    Decode(*this->idx2, P, &buff);
    query->setCurrentResolution(query->end_resolution);
    return true;
  }


  //nextBoxQuery
  virtual void nextBoxQuery(SharedPtr<BoxQuery> query) override
  {
    PrintInfo("IdxDataset2::nextBoxQuery");

    if (!query)
      return;

    if (!(query->isRunning() && query->getCurrentResolution() == query->getEndResolution()))
      return;

    //reached the end?
    if (query->end_resolution == query->end_resolutions.back())
      return query->setOk();

    auto failed = [&](String reason) {
      return query->setFailed(query->aborted() ? "query aborted" : reason);
    };

    auto idx = Utils::find(query->end_resolutions, query->end_resolution);
    if (!setBoxQueryEndResolution(query, query->end_resolutions[idx + 1]))
      VisusReleaseAssert(false);

    // no merging supported (will execute the next resolution from scratch)
    query->buffer = Array();
  }

  //createBlockQueriesForBoxQuery
  virtual std::vector<BigInt> createBlockQueriesForBoxQuery(SharedPtr<BoxQuery> query) override {
    ThrowException("TODO");
    return {};
  }

  //mergeBoxQueryWithBlockQuery
  virtual bool mergeBoxQueryWithBlockQuery(SharedPtr<BoxQuery> query, SharedPtr<BlockQuery> block_query) override
  {
    ThrowException("TODO");
    return false;
  }


  //createBoxQueryRequest
  virtual NetRequest createBoxQueryRequest(SharedPtr<BoxQuery> query) override
  {
    ThrowException("TODO");
    return NetRequest();
  }

  //executeBoxQueryOnServer
  virtual bool executeBoxQueryOnServer(SharedPtr<BoxQuery> query) override
  {
    ThrowException("TODO");
    return false;
  }

public:

  //________________________________________________
  //point query stuff

  //constructor
  virtual SharedPtr<PointQuery> createPointQuery(Position logic_position, Field field, double time, Aborted aborted = Aborted())
  {
    ThrowException("TODO");
    return SharedPtr<PointQuery>();
  }

  //createBlockQueriesForPointQuery
  virtual std::vector<BigInt> createBlockQueriesForPointQuery(SharedPtr<PointQuery> query) {
    ThrowException("TODO");
    return {};
  }

public:

  //readDatasetFromArchive
  virtual void readDatasetFromArchive(Archive& ar) override
  {
    //TODO: only local file so far (with *.idx2 extension)
    String url = ar.readString("url");

    VisusReleaseAssert(!idx2);
    this->idx2 = new idx2_file;
    this->idx2_dir = Path(url).getParent().getParent().toString();  //keep in memory
    this->idx2_url = url;  //keep in memory
    SetDir(this->idx2, idx2_dir.c_str());

    if (!ReadMetaFile(this->idx2, url.c_str()))
      ThrowException("problem");

    if (!Finalize(this->idx2))
      ThrowException("problem");
    
    DType dtype;
    if      (this->idx2->DType == dtype::int8   ) dtype = DTypes::INT8;
    else if (this->idx2->DType == dtype::uint8  ) dtype = DTypes::UINT8;
    else if (this->idx2->DType == dtype::int16  ) dtype = DTypes::INT16;
    else if (this->idx2->DType == dtype::uint16 ) dtype = DTypes::UINT16;
    else if (this->idx2->DType == dtype::int32  ) dtype = DTypes::INT32;
    else if (this->idx2->DType == dtype::uint32 ) dtype = DTypes::UINT32;
    else if (this->idx2->DType == dtype::int64  ) dtype = DTypes::INT64;
    else if (this->idx2->DType == dtype::uint64 ) dtype = DTypes::UINT64;
    else if (this->idx2->DType == dtype::float32) dtype = DTypes::FLOAT32;
    else if (this->idx2->DType == dtype::float64) dtype = DTypes::FLOAT64;
    else ThrowException("internal error");

    //convert to idx1 and Dataset class
#if 1
    PointNi dims(this->idx2->Dims3[0], this->idx2->Dims3[1], this->idx2->Dims3[2]);

    IdxFile idx1;
    idx1.version = 20;
    idx1.logic_box = BoxNi(PointNi(0, 0, 0), dims);
    idx1.bounds = idx1.logic_box;
    idx1.fields.push_back(Field(this->idx2->Field, dtype));
    idx1.validate(url);

    this->dataset_body = StringTree();
    this->idxfile = idx1;
    this->bitmask = idx1.bitmask;
    this->default_bitsperblock = idx1.bitsperblock;
    this->logic_box = idx1.logic_box;
    this->timesteps = idx1.timesteps;

    setDatasetBounds(idxfile.bounds);

    for (auto field : idxfile.fields)
      addField(field);
#endif

    setDatasetBody(ar);
    setDefaultAccuracy(0.01);
  }

private:

  idx2_file* idx2 = nullptr;
  String     idx2_url; //keep in memory
  String     idx2_dir; //keep in memory

  static v3i Cast(PointNi v) {
    return v3i((i32)v[0], (i32)v[1], (i32)v[2]);
  }

  static PointNi Cast(v3i v) {
    return PointNi(v[0], v[1], v[2]);
  }

};


} //namespace Visus


#endif //__VISUS_DB_IDX_DATASET2_H


#endif //VISUS_IDX2