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
IdxDataset2::IdxDataset2() {
}

//////////////////////////////////////////////////////////////////////
IdxDataset2::~IdxDataset2() {
}


//////////////////////////////////////////////////////////////////////
void IdxDataset2::GetDecodeParams(idx2::params& P, SharedPtr<BoxQuery> query, int H)
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

  P.Action = idx2::action::Decode;
  P.OutMode = idx2::params::out_mode::RegularGridMem;
  P.DecodeExtent = idx2::extent(Cast(query->logic_box.p1), Cast(query->logic_box.size())); //what is the logic_box/logic_samples in OpenVisus
  P.DownsamplingFactor3 = idx2::v3i(0, 0, 0); //get information at full resolution
  P.InputFile = this->input_file.c_str();
  P.InDir = this->in_dir.c_str();
  P.DecodeTolerance = 0.01;//TODO
  for (int I = MaxH; I > H; I--)
  {
    auto bit = bitmask[I];
    ++P.DownsamplingFactor3[bit];
  }
}

//////////////////////////////////////////////////////////////////////
bool IdxDataset2::setBoxQueryEndResolution(SharedPtr<BoxQuery> query, int H)
{
  PrintInfo("IdxDataset2::setBoxQueryEndResolution");
  auto bitmask = getBitmask();
  auto MaxH = bitmask.getMaxResolution();

  VisusReleaseAssert(H >= 0 && H < MaxH);
  VisusReleaseAssert(query->end_resolution < H);

  auto pdim = getPointDim();
  VisusAssert(pdim == 3); //todo 2d to

  idx2::params P;
  GetDecodeParams(P, query, H);

  //I just need to know the Grid
  idx2::idx2_file Idx2;
  DoAtExit do_at_exit([&]() {idx2::Dealloc(&Idx2); });
  idx2::InitFromBuffer(&Idx2, P, idx2::buffer((const idx2::byte*)this->metafile.c_str(), this->metafile.size()));
  idx2::grid OutGrid = idx2::GetGrid(Idx2, P.DecodeExtent);
  idx2::v3i from = idx2::From(OutGrid);
  idx2::v3i dims = idx2::Dims(OutGrid);
  idx2::v3i strd = idx2::Strd(OutGrid);


  if (false)
  {
    PrintInfo("//////////////////////////////////////////");
    PrintInfo("Bitmask", this->bitmask);
    PrintInfo("logic_box", query->logic_box);
    PrintInfo("H", H, "MaxH", MaxH, "DownsamplingFactor3 ", Cast(P.DownsamplingFactor3));
    PrintInfo("from", Cast(from), "Dims", Cast(dims), "stride", Cast(strd));
    PrintInfo("Tolerance", P.DecodeTolerance);
  }

  auto logic_samples = LogicSamples(BoxNi(Cast(from), Cast(from + dims * strd)), Cast(strd));
  if (!logic_samples.valid())
    return false;

  VisusReleaseAssert(logic_samples.logic_box.p1 == Cast(from));
  VisusReleaseAssert(logic_samples.nsamples == Cast(dims));
  VisusReleaseAssert(logic_samples.delta == Cast(strd));

  query->logic_samples = logic_samples;
  query->end_resolution = H;
  return true;
}


//////////////////////////////////////////////////////////////////////
void IdxDataset2::enableExternalRead(idx2::idx2_file& Idx2, SharedPtr<Access> access, Aborted aborted)
{
  Idx2.external_read = [this, access, aborted](const idx2::idx2_file& Idx2, idx2::buffer& Buf, idx2::u64 ChunkAddress) -> std::future<bool>
  {
    VisusReleaseAssert(static_cast<idx2::u64>(static_cast<Visus::BigInt>(ChunkAddress)) == ChunkAddress);

    //OLD version for debugging
    //std::ostringstream filename; filename << Visus::StringUtils::rtrim(std::string(Idx2.Dir.Ptr, Idx2.Dir.Size), "/") << "/" << Idx2.Name << "/" << Idx2.Field << "/" << ChunkAddress;
    //auto heap = Visus::Utils::loadBinaryDocument(filename.str());
    //AllocBuf(&buff, heap->c_size());
    //memcpy(buff.Data, heap->c_ptr(), heap->c_size());

    auto query = createBlockQuery(ChunkAddress, getField(), getTime(), 'r', aborted);

    SharedPtr< std::promise<bool> > p = std::make_shared< std::promise<bool> >( );
    std::future<bool>  f = p->get_future();

    query->done.get_promise()->addWhenDoneListener(std::function<void(Void)>([query,&Buf, p](Void) {

      if (!query->ok())
      {
        p->set_value(false);
      }
      else
      {
        idx2::AllocBuf(&Buf, query->buffer.c_size());
        memcpy(Buf.Data, query->buffer.c_ptr(), query->buffer.c_size());

        //if you want to debug..
# if 0
        auto checksum = Visus::StringUtils::hexdigest(Visus::StringUtils::md5(std::string((const char*)Buf.Data, Buf.Bytes)));
        PrintInfo("ExternalRead ChunkAddress", ChunkAddress, " Bytes=", Buf.Bytes, " checksum", checksum);
#endif

        p->set_value(true);
      }

    }));

    executeBlockQuery(access, query);
    return f;
  };
}

//////////////////////////////////////////////////////////////////////
void IdxDataset2::enableExternalWrite(idx2::idx2_file& Idx2, SharedPtr<Access> access, Aborted aborted)
{
  Idx2.external_write = [this, access, aborted](const idx2::idx2_file& Idx2, idx2::buffer& Buf, idx2::u64 ChunkAddress) -> bool {
#if 0
    std::ostringstream filename;
    auto dir = Visus::StringUtils::rtrim(std::string(Idx2.Dir.Ptr, Idx2.Dir.Size), "/");
    filename << dir << "/" << Idx2.Name << "/" << Idx2.Field << "/" << ChunkAddress;
    auto heap = Visus::Utils::loadBinaryDocument(filename.str());
    AllocBuf(&buff, heap->c_size());
    memcpy(buff.Data, heap->c_ptr(), heap->c_size());
    return true;
#else
    VisusReleaseAssert(static_cast<idx2::u64>(static_cast<Visus::BigInt>(ChunkAddress))==ChunkAddress);
    auto query = createBlockQuery(ChunkAddress, getField(), getTime(), 'w', aborted);
    query->buffer = Visus::Array(Buf.Bytes, Visus::DTypes::UINT8, Visus::HeapMemory::createUnmanaged(Buf.Data, Buf.Bytes));
    if (!executeBlockQueryAndWait(access, query))
      return false;
#endif

    if (false)
    {
      auto checksum = Visus::StringUtils::hexdigest(Visus::StringUtils::md5(std::string((const char*)Buf.Data, Buf.Bytes)));
      PrintInfo("ExternalWrite ChunkAddress", ChunkAddress, " Bytes=", Buf.Bytes," checksum", checksum);
    }

    return true;
  };
}

//////////////////////////////////////////////////////////////////////
SharedPtr<BlockQuery> IdxDataset2::createBlockQuery(BigInt blockid, Field field, double time, int mode, Aborted aborted)  {

  auto ret = std::make_shared<BlockQuery>();
  ret->dataset = this;
  ret->field = field;
  ret->time = time;
  ret->mode = mode; VisusAssert(mode == 'r' || mode == 'w');
  ret->aborted = aborted;
  ret->blockid = blockid;
  ret->logic_samples = LogicSamples::invalid(); // I don't have any valid LogicSamples (and I should not care);
  return ret;
}


////////////////////////////////////////////////
void IdxDataset2::executeBlockQuery(SharedPtr<Access> access, SharedPtr<BlockQuery> query)
{
  VisusAssert(access->isReading() || access->isWriting());

  int mode = query->mode;
  auto failed = [&](String reason) {

    if (!access)
      query->setFailed(reason);
    else
      mode == 'r' ? access->readFailed(query, reason) : access->writeFailed(query, reason);

    if (!reason.empty())
      PrintInfo("executeBlockQUery failed", reason);

    return;
  };

  if (!access)
    return failed("no access");

  if (!query->field.valid())
    return failed("field not valid");

  if (query->blockid < 0)
    return failed("address range not valid");

  if ((mode == 'r' && !access->can_read) || (mode == 'w' && !access->can_write))
    return failed("rw not enabled");

#if 0
  if (!query->logic_samples.valid())
    return failed("logic_samples not valid");

  //scrgiorgio: add this optimization to avoid empty blocks
  if (!query->logic_samples.logic_box.intersect(this->getLogicBox()))
    return failed("");//"no intersection with logic box" (TOO many messages with )
#endif

  if (mode == 'w' && !query->buffer.valid())
    return failed("no buffer to write");

  // override time  from from field
  if (query->field.hasParam("time"))
    query->time = cdouble(query->field.getParam("time"));

  query->setRunning();

  if (mode == 'r')
  {
    access->readBlock(query);
    BlockQuery::readBlockEvent();
  }
  else
  {
    access->writeBlock(query);
    BlockQuery::writeBlockEvent();
  }

  return;
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

  query->allocateBufferIfNeeded();
  VisusAssert(query->buffer.dims == query->getNumberOfSamples());

  idx2::params P;
  GetDecodeParams(P, query, query->end_resolution);

  idx2::idx2_file Idx2; DoAtExit do_at_exit([&]() {idx2::Dealloc(&Idx2); });
  idx2::InitFromBuffer(&Idx2, P, idx2::buffer((const idx2::byte*)this->metafile.c_str(), this->metafile.size()));

  //want to use OpenVisus::Access for IDX block access?
  Url url = this->getUrl();

  //in case you want to debug with IDX2 file format
  if (!cbool(url.getParam("DISABLE_EXTERNAL_ACCESS", "0")))
    enableExternalRead(Idx2, access, query->aborted);

  auto query_buffer = idx2::buffer((const idx2::byte*)query->buffer.c_ptr(), query->buffer.c_size());
  //idx2::Decode(Idx2, P, &query_buffer);  //TODO: no aborted?
  idx2::ParallelDecode(Idx2, P, &query_buffer);  //TODO: no aborted?
  query->setCurrentResolution(query->end_resolution);

  return true;
}


//////////////////////////////////////////////////////////////////////
void IdxDataset2::readDatasetFromArchive(Archive& ar)
{
  //TODO: only local file so far (with *.idx2 extension)
  String url = ar.readString("url"); //remove any params

  //I need to keep this in memory
  // <whatever>/Miranda/Viscosity/...data...
  // <whatever>/Miranda/Viscosity.idx2
  // i need to go just after <whatever>
  this->input_file = Url(url).getPath();
  this->in_dir = StringUtils::replaceAll(Path(this->input_file).getParent().getParent().toString(), "\\", "/");
  PrintInfo("idx2::input_file", this->input_file);
  PrintInfo("idx2::in_dir"    , this->in_dir);

  idx2::idx2_file Idx2;
  DoAtExit do_at_exit([&]() {idx2::Dealloc(&Idx2); });
  idx2::SetDir(&Idx2, this->in_dir.c_str());

  this->metafile = Utils::loadTextDocument(url);
  VisusReleaseAssert(!metafile.empty());
  VisusReleaseAssert(idx2::ReadMetaFileFromBuffer(&Idx2, idx2::buffer((const idx2::byte*)metafile.c_str(), metafile.size())));

  DType dtype;
  if      (Idx2.DType == idx2::dtype::float32) dtype = DTypes::FLOAT32;
  else if (Idx2.DType == idx2::dtype::float64) dtype = DTypes::FLOAT64;
  VisusReleaseAssert(dtype.valid());

  //convert to idx1 and Dataset class
#if 1
  PointNi dims(Idx2.Dims3[0], Idx2.Dims3[1], Idx2.Dims3[2]);

  IdxFile idx1;
  idx1.version = 20;
  idx1.logic_box = BoxNi(PointNi(0, 0, 0), dims);
  idx1.bounds = idx1.logic_box;
  idx1.fields.push_back(Field(Idx2.Field, dtype));
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

} //namespace Visus


#endif






