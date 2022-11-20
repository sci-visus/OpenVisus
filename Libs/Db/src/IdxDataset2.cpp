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


#include <Visus/Db.h>
#include <Visus/IdxDataset2.h>

#if VISUS_IDX2

#include <idx2.h>

namespace Visus {

//////////////////////////////////////////////////////////////////////
inline idx2::v3i Cast(PointNi v) {
  return idx2::v3i((Int32)v[0], (Int32)v[1], (Int32)v[2]);
}

inline PointNi Cast(idx2::v3i v) {
  return PointNi(v[0], v[1], v[2]);
}


//////////////////////////////////////////////////////////////////////
IdxDataset2::IdxDataset2() : Idx2(nullptr) {
}

//////////////////////////////////////////////////////////////////////
IdxDataset2::~IdxDataset2() {

  if (Idx2)
  {
    idx2::Dealloc(Idx2);
    delete Idx2;
    this->Idx2 = nullptr;
  }

}

//////////////////////////////////////////////////////////////////////
SharedPtr<Access> IdxDataset2::createAccess(StringTree config, bool for_block_query)  {
  return std::make_shared<IdxDiskAccess2>();
}

//////////////////////////////////////////////////////////////////////
SharedPtr<BlockQuery> IdxDataset2::createBlockQuery(BigInt blockid, Field field, double time, int mode, Aborted aborted)  {
  ThrowException("TODO");
  return SharedPtr<BlockQuery>();
}


//////////////////////////////////////////////////////////////////////
LogicSamples IdxDataset2::getBlockQuerySamples(BigInt blockid, int& H)
{
  ThrowException("TODO");
  return LogicSamples();
}

//////////////////////////////////////////////////////////////////////
void IdxDataset2::executeBlockQuery(SharedPtr<Access> access, SharedPtr<BlockQuery> query) 
{
  ThrowException("TODO");
}

//////////////////////////////////////////////////////////////////////
bool IdxDataset2::convertBlockQueryToRowMajor(SharedPtr<BlockQuery> block_query)  {
  ThrowException("TODO");
  return true;
}

//////////////////////////////////////////////////////////////////////
SharedPtr<BoxQuery> IdxDataset2::createEquivalentBoxQuery(int mode, SharedPtr<BlockQuery> block_query) 
{
  ThrowException("TODO");
  return SharedPtr<BoxQuery>();
}

//////////////////////////////////////////////////////////////////////
SharedPtr<BoxQuery> IdxDataset2::createBoxQuery(BoxNi logic_box, Field field, double time, int mode, Aborted aborted) 
{
  return Dataset::createBoxQuery(logic_box, field, time, mode, aborted); //should be the same
}

//////////////////////////////////////////////////////////////////////
int IdxDataset2::guessBoxQueryEndResolution(Frustum logic_to_screen, Position logic_position) 
{
  return Dataset::guessBoxQueryEndResolution(logic_to_screen, logic_position);
}

//////////////////////////////////////////////////////////////////////
LogicSamples IdxDataset2::getDecodeParams(idx2::params* P, SharedPtr<BoxQuery> query, int H)
{
  //euristic to map the resolution 'H' to level and mask (e.g. V012012 resolutions in the range [0,6] with 0  -  the first V)
  /*
      000000000011111111112222222  IDX2 LEVEL
      012345678901234567890123456  IDX2 MASK
      ---------------------------
      V01012012012012012012012012  IDX1

      Level 0, mask 128 = 256 x 256 x 256\n"); 0 HALF DIM
      Level 0, mask 64  = 256 x 256 x 128\n"); 1 HALF DIM
      Level 0, mask 32  = 256 x 128 x 256\n"); 1 HALF DIM
      Level 0, mask 16  = 128 x 256 x 256\n"); 1 HALF DIM
      Level 0, mask 8   = 256 x 128 x 128\n"); 2 HALF DIM
      Level 0, mask 4   = 128 x 256 x 128\n"); 2 HALF DIM
      Level 0, mask 2   = 128 x 128 x 256\n"); 2 HALF DIM
      Level 0, mask 1   = 128 x 128 x 128\n"); 3 HALF DIM
      Level 1, mask 128 = 128 x 128 x 128\n"); ALL HALF DIMENSION
  */

  auto MaxH = getMaxResolution();

  P->Action = idx2::action::Decode;
  P->OutMode = idx2::params::out_mode::RegularGridMem; //in memory!

  //what is the logic_box/logic_samples in OpenVisus
  P->DecodeExtent = idx2::extent(Cast(query->logic_box.p1), Cast(query->logic_box.size())); //first, dims

  //switch from defaiult access to OpenVisus access
  auto url = this->getUrl();
  P->enable_visus = cbool(Url(url).getParam("visus","0"));

  auto pdim = getPointDim();
  VisusAssert(pdim == 3); //todo 2d to

  P->DownsamplingFactor3 = idx2::v3i(0, 0, 0);

  auto bitmask = getBitmask();
  auto& down = P->DownsamplingFactor3;
  for (int I = MaxH; I > H; I--)
  {
    auto bit = bitmask[I];
    down[bit] = std::max(1, down[bit]) << 1;
  }

  //since IDX2 seems to want a char* I need some storage here
  this->idx2_input_file = Url(getUrl()).getPath();
  P->InputFile = this->idx2_input_file.c_str(); //keep in memory

  // <whatever>/Miranda/Viscosity/...data...
  // <whatever>/Miranda/Viscosity.idx2
  // i need to go just after <whatever>
  this->idx2_int_dir = StringUtils::replaceAll(Path(this->idx2_input_file).getParent().getParent().toString(), "\\", "/");
  P->InDir = idx2_int_dir.c_str();

  //todo
  P->DecodeAccuracy = query->accuracy;
  idx2::Init(this->Idx2, *P);

  idx2::grid OutGrid = idx2::GetGrid(*this->Idx2, P->DecodeExtent);
  idx2::v3i from = idx2::From(OutGrid);
  idx2::v3i dims = idx2::Dims(OutGrid); //is this the number of samples?
  idx2::v3i strd = idx2::Strd(OutGrid);
  PrintInfo("//////////////////////////////////////////");
  PrintInfo("Bitmask", this->bitmask);
  PrintInfo("logic_box", query->logic_box);
  PrintInfo("H", H, "MaxH", MaxH, "down ", Cast(down));
  PrintInfo("from", Cast(from), "Dims", Cast(dims), "stride", Cast(strd));
  PrintInfo("Accuracy", P->DecodeAccuracy);

  auto logic_samples = LogicSamples(BoxNi(Cast(from), Cast(from + dims * strd)), Cast(strd));
  VisusReleaseAssert(logic_samples.logic_box.p1 == Cast(from));
  VisusReleaseAssert(logic_samples.nsamples == Cast(dims));
  VisusReleaseAssert(logic_samples.delta == Cast(strd));
  return logic_samples;
}

//////////////////////////////////////////////////////////////////////
bool IdxDataset2::setBoxQueryEndResolution(SharedPtr<BoxQuery> query, int H) 
{
  PrintInfo("IdxDataset2::setBoxQueryEndResolution");
  auto MaxH = this->bitmask.getMaxResolution();

  VisusReleaseAssert(H >= 0 && H < MaxH);
  VisusReleaseAssert(query->end_resolution < H);

  idx2::params P;
  auto logic_samples = getDecodeParams(&P, query, H);
  if (!logic_samples.valid())
    return false;

  query->logic_samples = logic_samples;
  query->end_resolution = H;
  return true;
}

//////////////////////////////////////////////////////////////////////
void IdxDataset2::beginBoxQuery(SharedPtr<BoxQuery> query)
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

  //  time from field
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
    if (!(it >= 0 && it < this->getMaxResolution()))
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


//////////////////////////////////////////////////////////////////////
bool IdxDataset2::executeBoxQuery(SharedPtr<Access> access, SharedPtr<BoxQuery> query) 
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

  idx2::params P;
  auto logic_samples = getDecodeParams(&P, query, query->end_resolution);
  if (!logic_samples.valid())
    return false;

  VisusReleaseAssert(logic_samples == query->logic_samples);

  query->allocateBufferIfNeeded();
  VisusAssert(query->buffer.dims == query->getNumberOfSamples());

  auto buff = idx2::buffer((const idx2::byte*)query->buffer.c_ptr(), query->buffer.c_size());
  idx2::Init(this->Idx2, P);
  idx2::Decode(*this->Idx2, P, &buff); //TODO: no aborted?
  query->setCurrentResolution(query->end_resolution);
  return true;
}


//////////////////////////////////////////////////////////////////////
void IdxDataset2::nextBoxQuery(SharedPtr<BoxQuery> query) 
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

//////////////////////////////////////////////////////////////////////
std::vector<BigInt> IdxDataset2::createBlockQueriesForBoxQuery(SharedPtr<BoxQuery> query)  {
  ThrowException("not supported");
  return {};
}

//////////////////////////////////////////////////////////////////////
bool IdxDataset2::mergeBoxQueryWithBlockQuery(SharedPtr<BoxQuery> query, SharedPtr<BlockQuery> block_query)
{
  ThrowException("not supported");
  return false;
}

//////////////////////////////////////////////////////5////////////////
NetRequest IdxDataset2::createBoxQueryRequest(SharedPtr<BoxQuery> query) 
{
  ThrowException("not supported");
  return NetRequest();
}

//////////////////////////////////////////////////////////////////////
bool IdxDataset2::executeBoxQueryOnServer(SharedPtr<BoxQuery> query) 
{
  ThrowException("not supported");
  return false;
}

//////////////////////////////////////////////////////////////////////
SharedPtr<PointQuery> IdxDataset2::createPointQuery(Position logic_position, Field field, double time, Aborted)
{
  ThrowException("not supported");
  return SharedPtr<PointQuery>();
}

//////////////////////////////////////////////////////////////////////
std::vector<BigInt> IdxDataset2::createBlockQueriesForPointQuery(SharedPtr<PointQuery> query) {
  ThrowException("not supported");
  return {};
}

//////////////////////////////////////////////////////////////////////
void IdxDataset2::readDatasetFromArchive(Archive& ar) 
{
  //TODO: only local file so far (with *.idx2 extension)
  String url = Url(ar.readString("url")).getPath(); //remove any params

  VisusReleaseAssert(!this->Idx2);
  this->Idx2 = new idx2::idx2_file; 
  idx2::SetDir(this->Idx2, "./");

  if (!idx2::ReadMetaFile(this->Idx2, url.c_str()))
    ThrowException("problem");

  //if (!idx2::Finalize(this->Idx2))
  //  ThrowException("problem");

  DType dtype;
  if (this->Idx2->DType == idx2::dtype::float32) dtype = DTypes::FLOAT32;
  else if (this->Idx2->DType == idx2::dtype::float64) dtype = DTypes::FLOAT64;
  else ThrowException("internal error");

  //convert to idx1 and Dataset class
#if 1
  PointNi dims(this->Idx2->Dims3[0], this->Idx2->Dims3[1], this->Idx2->Dims3[2]);

  IdxFile idx1;
  idx1.version = 20;
  idx1.logic_box = BoxNi(PointNi(0, 0, 0), dims);
  idx1.bounds = idx1.logic_box;
  idx1.fields.push_back(Field(this->Idx2->Field, dtype));
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



} //namespace Visus


#endif 






