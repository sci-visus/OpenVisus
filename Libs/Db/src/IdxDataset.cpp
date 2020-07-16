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

#include <Visus/IdxDataset.h>
#include <Visus/IdxDiskAccess.h>
#include <Visus/IdxHzOrder.h>
#include <Visus/IdxFilter.h>
#include <Visus/RamAccess.h>
#include <Visus/NetService.h>

namespace Visus {

////////////////////////////////////////////////////////
class BoxQueryHzConversion
{
public:

  //_______________________________________________________________
  class Level
  {
  public:

    int                   num = 0;// total number of cachable elements
    int                   pdim = 0;// what is the space I need to work
    SharedPtr<HeapMemory> cached_points = std::make_shared<HeapMemory>();

    //constructor
    Level(const DatasetBitmask& bitmask, int H, int numbits = 10)
    {
      VisusAssert(bitmask.valid());
      --H; //yes! it is correct

      numbits = std::max(0, std::min(numbits, H));

      //number of bits which can be cached for current H
      this->num = 1 << numbits; VisusAssert(this->num);
      this->pdim = bitmask.getPointDim();

      if (!cached_points->resize(this->num * sizeof(PointNi), __FILE__, __LINE__))
      {
        this->clear();
        return;
      }

      cached_points->fill(0);

      PointNi* ptr = (PointNi*)cached_points->c_ptr();

      HzOrder hzorder(bitmask, H);

      //create the delta for points  
      for (BigInt zaddress = 0; zaddress < (this->num - 1); zaddress++, ptr++)
      {
        PointNi Pcur = hzorder.deinterleave(zaddress + 0);
        PointNi Pnex = hzorder.deinterleave(zaddress + 1);

        (*ptr) = PointNi(pdim);

        //store the delta
        for (int D = 0; D < pdim; D++)
          (*ptr)[D] = Pnex[D] - Pcur[D];
      }

      //i want the last (FAKE and unused) element to be zero
      (*ptr) = PointNi(pdim);
    }

    //destructor
    inline ~Level() {
      clear();
    }

    //valid
    bool valid() {
      return this->num > 0;
    }

    //memsize
    inline int memsize() const {
      return sizeof(PointNi) * (this->num);
    }

    //clear
    inline void clear() {
      this->cached_points.reset();
      this->num = 0;
      this->pdim = 0;
    }
  };

  DatasetBitmask bitmask;
  std::vector< SharedPtr<Level> > levels;

  //constructor
  BoxQueryHzConversion(const DatasetBitmask& bitmask_) : bitmask(bitmask_)
  {
    int maxh = bitmask.getMaxResolution();
    while (maxh >= this->levels.size())
      this->levels.push_back(std::make_shared<Level>(bitmask, (int)this->levels.size()));
  }


};

////////////////////////////////////////////////////////
class PointQueryHzConversion
{
public:

  Int64 memsize = 0;

  std::vector< std::pair<BigInt, Int32>* > loc;

  //create
  PointQueryHzConversion(DatasetBitmask bitmask)
  {
    //todo cases for which I'm using regexp
    auto MaxH = bitmask.getMaxResolution();
    BigInt last_bitmask = ((BigInt)1) << MaxH;

    HzOrder hzorder(bitmask);
    int pdim = bitmask.getPointDim();
    loc.resize(pdim);

    for (int D = 0; D < pdim; D++)
    {
      Int64 dim = bitmask.getPow2Dims()[D];

      this->loc[D] = new std::pair<BigInt, Int32>[dim];

      BigInt one = 1;
      for (int i = 0; i < dim; i++)
      {
        PointNi p(pdim);
        p[D] = i;
        auto& pair = this->loc[D][i];
        pair.first = hzorder.interleave(p);
        pair.second = 0;
        BigInt temp = pair.first | last_bitmask;
        while ((one & temp) == 0) { pair.second++; temp >>= 1; }
        temp >>= 1;
        pair.second++;
      }
    }
  }

  //destructor
  ~PointQueryHzConversion() {
    for (auto it : loc)
      delete[] it;
  }

};

////////////////////////////////////////////////////////
class HzAddressConversion
{
public:

  struct
  {
    CriticalSection                                       lock;
    std::map<String, SharedPtr<BoxQueryHzConversion> >    map;
  }
  boxquery;

  struct
  {
    CriticalSection                                      lock;
    std::map<String, SharedPtr<PointQueryHzConversion> > map;
  }
  pointquery;

};

static HzAddressConversion hz_address_conversion;

///////////////////////////////////////////////////////////////////////////
class ConvertHzOrderSamples
{
public:

  //execute
  template <class Sample>
  bool execute(IdxDataset* vf, BoxQuery* query, BlockQuery* block_query)
  {
#define PUSH()  (*((stack)++))=(item)
#define POP()   (item)=(*(--(stack)))
#define EMPTY() ((stack)==(STACK))

    VisusAssert(query->field.dtype == block_query->buffer.dtype);

    VisusAssert(block_query->buffer.layout == "hzorder");

    bool bInvertOrder = query->mode == 'w';

    auto bitsperblock = vf->getDefaultBitsPerBlock();
    int  samplesperblock = 1 << bitsperblock;

    DatasetBitmask bitmask = vf->idxfile.bitmask;
    int            max_resolution = vf->getMaxResolution();
    BigInt         HzFrom = block_query->blockid * samplesperblock;
    BigInt         HzTo = HzFrom + samplesperblock;
    HzOrder        hzorder(bitmask);
    int            hstart = std::max(query->getCurrentResolution() + 1, block_query->blockid == 0 ? 0 : block_query->H);
    int            hend = std::min(query->getEndResolution(), block_query->H);

    VisusAssert(HzFrom == 0 || hstart == hend);

    auto Wbox = GetSamples<Sample>(query->buffer);
    auto Rbox = GetSamples<Sample>(block_query->buffer);

    if (bInvertOrder)
      std::swap(Wbox, Rbox);

    auto address_conversion = vf->hzaddress_conversion_boxquery;
    VisusReleaseAssert(address_conversion);

    int              numused = 0;
    int              bit;
    Int64 delta;
    BoxNi            logic_box = query->logic_samples.logic_box;
    PointNi          stride = query->getNumberOfSamples().stride();
    PointNi          qshift = query->logic_samples.shift;
    BigInt           numpoints;
    Aborted          aborted = query->aborted;

    typedef struct { int    H; BoxNi  box; } FastLoopStack;
    FastLoopStack item, * stack = NULL;
    FastLoopStack STACK[DatasetBitmaskMaxLen + 1];

    //layout of the block
    auto block_logic_box = block_query->getLogicBox();
    if (!block_logic_box.valid())
      return false;

    //deltas
    std::vector<Int64> fldeltas(max_resolution + 1);
    for (int H = 0; H <= max_resolution; H++)
      fldeltas[H] = H ? (vf->level_samples[H].delta[bitmask[H]] >> 1) : 0;

    for (int H = hstart; H <= hend; H++)
    {
      if (aborted())
        return false;

      LogicSamples Lsamples = vf->level_samples[H];
      PointNi  lshift = Lsamples.shift;

      BoxNi   zbox = (HzFrom != 0) ? block_logic_box : Lsamples.logic_box;
      BigInt  hz = hzorder.pointToHzAddress(zbox.p1);

      BoxNi user_box = logic_box.getIntersection(zbox);
      BoxNi box = Lsamples.alignBox(user_box);
      if (!box.isFullDim())
        continue;

      VisusAssert(address_conversion->levels[H]);
      const auto& fllevel = *(address_conversion->levels[H]);
      int cachable = std::min(fllevel.num, samplesperblock);

      //i need this to "split" the fast loop in two chunks
      VisusAssert(cachable > 0 && cachable <= samplesperblock);

      //push root in the kdtree
      {
        item.box = zbox;
        item.H = H ? std::max(1, H - bitsperblock) : (0);
        stack = STACK;
        PUSH();
      }

      while (!EMPTY())
      {
        if (aborted())
          return false;

        POP();

        // no intersection
        if (!item.box.strictIntersect(box))
        {
          hz += (((BigInt)1) << (H - item.H));
          continue;
        }

        // all intersection and all the samples are contained in the FastLoopCache
        numpoints = ((BigInt)1) << (H - item.H);
        if (numpoints <= cachable && box.containsBox(item.box))
        {
          Int64    hzfrom = cint64(hz - HzFrom);
          Int64    num = cint64(numpoints);
          PointNi* cc = (PointNi*)fllevel.cached_points->c_ptr();
          const PointNi  query_p1 = logic_box.p1;
          PointNi  P = item.box.p1;

          ++numused;

          VisusAssert(hz >= HzFrom && hz < HzTo);

          Int64 from = stride.dotProduct((P - query_p1).rightShift(qshift));

          auto& Windex = bInvertOrder ? hzfrom : from;
          auto& Rindex = bInvertOrder ? from : hzfrom;

          auto shift = (lshift - qshift);

          //slow version (enable it if you have problems)
#if 0
          for (; num--; ++hzfrom, ++cc)
          {
            from = stride.dotProduct((P - query_p1).rightShift(qshift));
            Wbox[Windex] = Rbox[Rindex];
            P += (cc->leftShift(lshift));
          }
          //fast version
#else

#define EXPRESSION(num) stride[num] * ((*cc)[num] << shift[num]) 
          switch (fllevel.pdim) {
          case 2: for (; num--; ++hzfrom, ++cc) { Wbox[Windex] = Rbox[Rindex]; from += EXPRESSION(0) + EXPRESSION(1); } break;
          case 3: for (; num--; ++hzfrom, ++cc) { Wbox[Windex] = Rbox[Rindex]; from += EXPRESSION(0) + EXPRESSION(1) + EXPRESSION(2); } break;
          case 4: for (; num--; ++hzfrom, ++cc) { Wbox[Windex] = Rbox[Rindex]; from += EXPRESSION(0) + EXPRESSION(1) + EXPRESSION(2) + EXPRESSION(3); } break;
          case 5: for (; num--; ++hzfrom, ++cc) { Wbox[Windex] = Rbox[Rindex]; from += EXPRESSION(0) + EXPRESSION(1) + EXPRESSION(2) + EXPRESSION(3) + EXPRESSION(4); } break;
          default: ThrowException("internal error"); break;
          }
#undef EXPRESSION
#endif

          hz += numpoints;
          continue;
        }

        //kd-traversal code
        bit = bitmask[item.H];
        delta = fldeltas[item.H];
        ++item.H;
        item.box.p1[bit] += delta; VisusAssert(item.box.isFullDim()); PUSH();
        item.box.p1[bit] -= delta; item.box.p2[bit] -= delta; VisusAssert(item.box.isFullDim()); PUSH();
      }
    }

    VisusAssert(numused > 0);
    return true;

#undef PUSH
#undef POP
#undef EMPTY
  }

};


//////////////////////////////////////////////////////////////////////////////////////////
IdxDataset::IdxDataset() {

  //historycally are not full res
  bBlocksAreFullRes = false;
}

//////////////////////////////////////////////////////////////////////////////////////////
IdxDataset::~IdxDataset(){
}

///////////////////////////////////////////////////////////////////////////////////
void IdxDataset::compressDataset(std::vector<String> compression, Array data)
{
  // for future version: here I'm making the assumption that a file contains multiple fields
  if (idxfile.version != 6)
    ThrowException("unsupported");

  //PrintInfo("Compressing dataset", StringUtils::join(compression));

  int nlevels = getMaxResolution() + 1;
  VisusReleaseAssert(compression.size() <= nlevels);

  // example ["zip","jpeg","jpeg"] means last level "jpeg", last-level-minus-one "jpeg" all others zip
  while (compression.size() < nlevels)
    compression.insert(compression.begin(), compression.front());

  //save the new idx file oonly if compression is equal for all levels
  if (std::set<String>(compression.begin(), compression.end()).size() == 1)
  {
    for (auto& field : idxfile.fields)
      field.default_compression = compression[0];

    String filename = Url(getUrl()).getPath();
    idxfile.save(filename);
  }

  if (data.valid())
  {
    //data will replace current data
    //write BoxQuery(==data) to BlockQuery(==RAM)
    auto query = createBoxQuery(getLogicBox(), 'w');
    beginBoxQuery(query);
    VisusReleaseAssert(query->isRunning());
    VisusAssert(query->getNumberOfSamples() == data.dims);
    query->buffer = data;

    auto Waccess = std::make_shared<IdxDiskAccess>(this);
    Waccess->disableWriteLock();
    Waccess->disableAsync();

    auto Raccess = std::make_shared<RamAccess>(getDefaultBitsPerBlock());
    Raccess->setAvailableMemory(/* no memory limit*/0);

    Raccess->disableWriteLock();
    VisusReleaseAssert(executeBoxQuery(Raccess, query));

    //read blocks are in RAM assuming there is no file yet stored on disk
    Raccess->beginRead();
    Waccess->beginWrite();
    for (auto time : idxfile.timesteps.asVector())
    {
      for (BigInt blockid = 0, total_blocks = getTotalNumberOfBlocks(); blockid < total_blocks; blockid++)
      {
        for (auto Rfield : idxfile.fields)
        {
          auto read_block = createBlockQuery(blockid, Rfield, time, 'r');
          if (!executeBlockQueryAndWait(Raccess, read_block))
            continue;

          //compression can depend on level
          int H = read_block->H;
          VisusReleaseAssert(H >= 0 && H < compression.size());

          auto Wfield = Rfield;
          Wfield.default_compression = compression[H];
          auto write_block = createBlockQuery(read_block->blockid, Wfield, read_block->time, 'w');
          write_block->buffer = read_block->buffer;
          VisusReleaseAssert(executeBlockQueryAndWait(Waccess, write_block));
        }
      }
    }
    Raccess->endRead();
    Waccess->endWrite();
  }
  else
  {
    String suffix = ".~compressed";

    String idx_filename = getUrl();
    VisusReleaseAssert(FileUtils::existsFile(idx_filename));
    String compressed_idx_filename = idx_filename + suffix;

    auto compressed_idx_file = this->idxfile;
    compressed_idx_file.filename_template = idxfile.filename_template + suffix;
    compressed_idx_file.save(compressed_idx_filename);

    auto Waccess = std::make_shared<IdxDiskAccess>(this, compressed_idx_file);
    auto Raccess = std::make_shared<IdxDiskAccess>(this, idxfile);

    Raccess->disableAsync();
    Raccess->disableWriteLock();

    Waccess->disableWriteLock();
    Waccess->disableAsync();

    for (auto time : idxfile.timesteps.asVector())
    {
      String prev_filename;
      auto setFilename = [&](String value)
      {
        //wait for a filename change
        if (prev_filename == value)
          return;

        // close any read file handle only if it's the last read op
        if (value.empty())
          Raccess->endRead();

        //close any write file handle
        Waccess->endWrite();

        //mv filename.~compressed -> filename
        if (!prev_filename.empty())
        {
          VisusReleaseAssert(FileUtils::removeFile(prev_filename));
          VisusReleaseAssert(FileUtils::moveFile(prev_filename + suffix, prev_filename));
        }

        prev_filename = value;

        if (value.empty())
        {
          Raccess->beginRead();
        }
        else
        {
          //remove any file coming from an old compression process
          FileUtils::removeFile(value + suffix);
        }

        Waccess->beginWrite();
      };

      Raccess->beginRead();
      Waccess->beginWrite();
      for (BigInt blockid = 0, total_blocks = getTotalNumberOfBlocks(); blockid < total_blocks; blockid++)
      {
        for (auto Rfield : idxfile.fields)
        {
          auto filename = Raccess->getFilename(Rfield, time, blockid);
          auto read_block = createBlockQuery(blockid, Rfield, time, 'r');

          //could fail because block does not exist
          if (!executeBlockQueryAndWait(Raccess, read_block))
            continue;

          //prepare for writing... 
          setFilename(filename);

          //compression can depend on level
          int H = read_block->H;
          VisusReleaseAssert(H >= 0 && H < compression.size());

          auto Wfield = Rfield;
          Wfield.default_compression = compression[H];
          auto write_block = createBlockQuery(read_block->blockid, Wfield, read_block->time, 'w');
          write_block->buffer = read_block->buffer;
          VisusReleaseAssert(executeBlockQueryAndWait(Waccess, write_block));
        }
      }
      setFilename("");
      Raccess->endRead();
      Waccess->endWrite();
    }
    
    VisusReleaseAssert(FileUtils::existsFile(compressed_idx_filename));
    FileUtils::removeFile(compressed_idx_filename);
  }
}



////////////////////////////////////////////////////////
bool IdxDataset::convertBlockQueryToRowMajor(SharedPtr<BlockQuery> block_query) 
{
  //already in row major
  if (block_query->buffer.layout.empty())
    return true;

  //note I cannot use buffer of block_query because I need them to execute other queries
  Array row_major= block_query->buffer.clone();

  auto query= createEquivalentBoxQuery('r',block_query);
  beginBoxQuery(query);

  if (!query->isRunning())
  {
    VisusAssert(false);
    return false;
  }

  //as the query has not already been executed!
  query->setCurrentResolution(query->start_resolution-1);
  query->buffer=row_major; 
  
  if (!query->allocateBufferIfNeeded())
    return false;

  ConvertHzOrderSamples op;
  if (!NeedToCopySamples(op, query->field.dtype, this, query.get(), block_query.get()))
  {
    VisusAssert(block_query->aborted());
    return false;
  }

  //now block query it's row major
  block_query->buffer=row_major;
  block_query->buffer.layout=""; 
  return true;
}





////////////////////////////////////////////////////////////////////////////////
class InterpolateBufferOperation
{
public:

  //execute
  template <class CppType>
  bool execute(LogicSamples Wsamples, Array Wbuffer, LogicSamples Rsamples, Array Rbuffer, Aborted aborted)
  {
    if (!Wsamples.valid() || !Rsamples.valid())
      return false;

    if (Wbuffer.dtype != Rbuffer.dtype || Wbuffer.dims != Wsamples.nsamples || Rbuffer.dims != Rsamples.nsamples)
    {
      VisusAssert(false);
      return false;
    }

    auto pdim = Wbuffer.getPointDim(); VisusAssert(Rbuffer.getPointDim() == pdim);
    auto zero = PointNi(pdim);
    auto one = PointNi(pdim);

    auto Wstride = Wbuffer.dims.stride();
    auto Rstride = Rbuffer.dims.stride();

    VisusReleaseAssert(Wbuffer.dtype == Rbuffer.dtype);
    int N = Wbuffer.dtype.ncomponents();

    //for each component...
    for (int C = 0; C < N; C++)
    {
      if (aborted())
        return false;

      GetComponentSamples<CppType> W(Wbuffer, C); PointNi Wpixel(pdim); Int64   Wpos = 0;
      GetComponentSamples<CppType> R(Rbuffer, C); PointNi Rpixel(pdim); PointNi Rpos(pdim);

#define W2R(I) (Utils::clamp<Int64>(((Wsamples.logic_box.p1[I] + (Wpixel[I] << Wsamples.shift[I])) - Rsamples.logic_box.p1[I]) >> Rsamples.shift[I], 0, Rbuffer.dims[I] - 1))

      if (pdim == 2)
      {
        for (Wpixel[1] = 0; Wpixel[1] < Wbuffer.dims[1]; Wpixel[1]++) {
          Rpixel[1] = W2R(1); Rpos[1] = Rpixel[1] * Rstride[1];
          for (Wpixel[0] = 0; Wpixel[0] < Wbuffer.dims[0]; Wpixel[0]++) {
            Rpixel[0] = W2R(0); Rpos[0] = Rpixel[0] * Rstride[0] + Rpos[1];
            W[Wpos++] = R[Rpos[0]];
          }
        }
      }
      else if (pdim == 3)
      {
        for (Wpixel[2] = 0; Wpixel[2] < Wbuffer.dims[2]; Wpixel[2]++) {
          Rpixel[2] = W2R(2); Rpos[2] = Rpixel[2] * Rstride[2];
          for (Wpixel[1] = 0; Wpixel[1] < Wbuffer.dims[1]; Wpixel[1]++) {
            Rpixel[1] = W2R(1); Rpos[1] = Rpixel[1] * Rstride[1] + Rpos[2];
            for (Wpixel[0] = 0; Wpixel[0] < Wbuffer.dims[0]; Wpixel[0]++) {
              Rpixel[0] = W2R(0); Rpos[0] = Rpixel[0] * Rstride[0] + Rpos[1];
              W[Wpos++] = R[Rpos[0]];
            }
          }
        }
      }
      else
      {
        for (auto it = ForEachPoint(Wbuffer.dims); !it.end(); it.next())
        {
          Wpixel = it.pos;
          Rpixel = PointNi::clamp(Rsamples.logicToPixel(Wsamples.pixelToLogic(Wpixel)), zero, Rbuffer.dims - one);
          W[Wpos++] = R[Rpixel.dot(Rstride)];
        }
      }
    }
    return true;
  }
};

//////////////////////////////////////////////////////////////////////////////////////////
void IdxDataset::beginBoxQuery(SharedPtr<BoxQuery> query) 
{
  if (!query)
    return;

  if (query->getStatus() != Query::QueryCreated)
    return;

  if (query->aborted())
    return query->setFailed("query aborted");

  if (!query->field.valid())
    return query->setFailed("field not valid");

  if (!query->logic_box.valid())
    return query->setFailed("position not valid");

  // override time from field
  if (query->field.hasParam("time"))
    query->time = cdouble(query->field.getParam("time"));

  if (!getTimesteps().containsTimestep(query->time))
    return query->setFailed("wrong time");

  if (query->end_resolutions.empty())
    query->end_resolutions = { this->getMaxResolution() };

  for (int I = 0; I < (int)query->end_resolutions.size(); I++)
  {
    if (query->end_resolutions[I] <0 || query->end_resolutions[I]> this->getMaxResolution())
      return query->setFailed("wrong end resolution");
  }

  if (query->start_resolution > 0 && (query->end_resolutions.size() != 1 || query->start_resolution != query->end_resolutions[0]))
    return query->setFailed("wrong query start resolution");

  if (!query->logic_box.valid())
    return query->setFailed("query logic_position is wrong");

  if (query->filter.enabled)
  {
    if (!query->filter.dataset_filter)
    {
      query->filter.dataset_filter = createFilter(query->field);

      if (!query->filter.dataset_filter)
        query->disableFilters();
    }
  }

  for (auto end_resolution : query->end_resolutions)
  {
    if (setBoxQueryEndResolution(query, end_resolution))
    {
      query->setRunning();
      return;
    }
  }

  query->setFailed();
}

///////////////////////////////////////////////////////////////////////////////////////
bool IdxDataset::executeBoxQuery(SharedPtr<Access> access, SharedPtr<BoxQuery> query)
{
  if (!query)
    return false;

  if (!(query->isRunning() && query->getCurrentResolution() < query->getEndResolution()))
    return false;

  if (query->aborted())
  {
    query->setFailed("query aborted");
    return false;
  }

  //for 'r' queries I can postpone the allocation
  if (query->mode == 'w' && !query->buffer.valid())
  {
    query->setFailed("write buffer not set");
    return false;
  }

  if (!access)
    return executeBoxQueryOnServer(query);

  VisusAssert(access);

  bool bWriting = query->mode == 'w';
  bool bReading = query->mode == 'r';

  const Field& field = query->field;
  double        time = query->time;

  int cur_resolution = query->getCurrentResolution();
  int end_resolution = query->end_resolution;

  if (!query->allocateBufferIfNeeded())
    return false;

  //filter enabled... need to go level by level
  if (auto filter = query->filter.dataset_filter)
  {
    VisusAssert(bReading);

    //need to go level by level to rebuild the original data (top-down)
    for (int H = cur_resolution + 1; H <= end_resolution; H++)
    {
      BoxNi adjusted_logic_box = adjustBoxQueryFilterBox(query.get(), filter.get(), query->filter.adjusted_logic_box, H);

      auto Wquery = createBoxQuery(adjusted_logic_box, query->field, query->time, 'r', query->aborted);
      Wquery->setResolutionRange(0, H);
      Wquery->disableFilters();

      beginBoxQuery(Wquery);

      //cannot get samples yet
      if (!Wquery->isRunning())
      {
        VisusAssert(cur_resolution == -1);
        VisusAssert(!query->filter.query);
        continue;
      }

      //try to merge previous results
      if (auto Rquery = query->filter.query)
      {
        if (!Wquery->allocateBufferIfNeeded())
          return false;

        //interpolate samples seems to be wrong here, produces a lot of artifacts (see david_wavelets)! 
        //could be that I'm inserting wrong coefficients?
        //for now insertSamples seems to work just fine, "interpolating" missing blocks/samples
        if (!insertSamples(Wquery->logic_samples, Wquery->buffer, Rquery->logic_samples, Rquery->buffer, Wquery->aborted))
          return false;

        //note: start_resolution/end_resolution do not change
        Wquery->setCurrentResolution(Rquery->getCurrentResolution());
      }

      if (!this->executeBoxQuery(access, Wquery))
        return false;

      filter->internalComputeFilter(Wquery.get(), /*bInverse*/true);

      query->filter.query = Wquery;
    }

    //cannot get samples yet... returning failed as a query without filters...
    if (!query->filter.query)
    {
      VisusAssert(false);//technically this should not happen since the outside query has got some samples
      return false;
    }

    query->logic_samples = query->filter.query->logic_samples;
    query->buffer = query->filter.query->buffer;

    VisusAssert(query->buffer.dims == query->getNumberOfSamples());
    query->setCurrentResolution(query->end_resolution);
    return true;
  }

  //execute with access
  int bitsperblock = access->bitsperblock;
  VisusAssert(bitsperblock);

  typedef struct { int    H; BoxNi  box; } FastLoopStack;
  FastLoopStack  item, * stack = NULL;
  FastLoopStack  STACK[DatasetBitmaskMaxLen + 1];

  DatasetBitmask bitmask = this->idxfile.bitmask;
  
  int max_resolution = getMaxResolution();
  std::vector<Int64> fldeltas(max_resolution + 1);
  for (auto H = 0; H <= max_resolution; H++)
    fldeltas[H] = H ? (this->level_samples[H].delta[bitmask[H]] >> 1) : 0;

  auto aborted = query->aborted;

  HzOrder hzorder(bitmask);
  //collect blocks
  std::vector<BigInt> blocks;
  for (int H = cur_resolution + 1; H <= end_resolution; H++)
  {
    if (aborted())
      return false;

    LogicSamples Lsamples = this->level_samples[H];
    BoxNi box = Lsamples.alignBox(query->logic_samples.logic_box);
    if (!box.isFullDim())
      continue;

    //push first item
    BigInt hz = hzorder.pointToHzAddress(Lsamples.logic_box.p1);

    item.box = Lsamples.logic_box;
    item.H = H ? 1 : 0;
    stack = STACK;
    *(stack++) = item;

    while (stack != STACK)
    {
      item = *(--stack);

      // no intersection
      if (!item.box.strictIntersect(box))
      {
        hz += (((BigInt)1) << (H - item.H));
        continue;
      }

      // intersection with hz-block!
      if ((H - item.H) <= bitsperblock)
      {
        auto blockid = hz >> bitsperblock;
        blocks.push_back(blockid);

        // I know that block 0 convers several hz-levels from [0 to bitsperblock]
        if (blockid == 0)
        {
          H = bitsperblock;
          break;
        }

        hz += ((BigInt)1) << (H - item.H);
        continue;
      }

      //kd-traversal code
      int bit = bitmask[item.H];
      Int64 delta = fldeltas[item.H];
      ++item.H;
      item.box.p1[bit] += delta;                            *(stack++) = item;
      item.box.p1[bit] -= delta; item.box.p2[bit] -= delta; *(stack++) = item;

    } //while 

  } //for levels


  if (aborted())
    return false;

  int NREAD = 0;
  int NWRITE = 0;
  WaitAsync< Future<Void> > async_read;

  //waitAllDone
  auto  waitAsyncRead = [&]()
  {
    async_read.waitAllDone();
    //PrintInfo("aysnc read",concatenate(NREAD, "/", blocks.size()),"...");
  };

  //PrintInfo("Executing query...");

  //rehentrant call...(just to not close the file too soon)
  bool bWasWriting = access->isWriting();
  bool bWasReading = access->isReading();

  if (bWriting)
  {
    if (!bWasWriting)
      access->beginWrite();
  }
  else
  {
    if (!bWasReading)
      access->beginRead();
  }

  for (auto blockid : blocks)
  {
    if (aborted())
      break;

    //flush previous
    if (async_read.getNumRunning() > 1024)
      waitAsyncRead();

    auto read_block = createBlockQuery(blockid, field, time, 'r', aborted);
    NREAD++;

    if (bReading)
    {
      executeBlockQuery(access, read_block);
      async_read.pushRunning(read_block->done).when_ready([this, query, read_block, aborted](Void)
      {
        //I don't care if the read fails...
        if (!aborted() && read_block->ok())
          mergeBoxQueryWithBlockQuery(query, read_block);
      });
    }
    else
    {
      //need a lease... so that I can read/merge/write like in a transaction mode
      access->acquireWriteLock(read_block);

      //need to read and wait the block
      executeBlockQueryAndWait(access, read_block);

      //WRITE block
      auto write_block = createBlockQuery(blockid, field, time, 'w', aborted);

      //read ok
      if (read_block->ok())
        write_block->buffer = read_block->buffer;
      //I don't care if it fails... maybe does not exist
      else
        write_block->allocateBufferIfNeeded();

      mergeBoxQueryWithBlockQuery(query, write_block);

      //need to write and wait for the block
      executeBlockQueryAndWait(access, write_block);
      NWRITE++;

      //important! all writings are with a lease!
      access->releaseWriteLock(read_block);

      if (aborted() || write_block->failed()) {
        if (!bWasWriting)
          access->endWrite();
        return false;
      }
    }
  }

  if (bWriting && !bWasWriting)
    access->endWrite();

  if (bReading && !bWasReading)
    access->endRead();

  waitAsyncRead();
  //PrintInfo("Query finished", "NREAD", NREAD, "NWRITE", NWRITE);

  //set the query status
  if (aborted())
    return false;

  VisusAssert(query->buffer.dims == query->getNumberOfSamples());
  query->setCurrentResolution(query->end_resolution);
  return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
void IdxDataset::nextBoxQuery(SharedPtr<BoxQuery> query)
{
  if (!query)
    return;

  if (!(query->isRunning() && query->getCurrentResolution() == query->getEndResolution()))
    return;

  //reached the end? 
  if (query->end_resolution == query->end_resolutions.back())
    return query->setOk();

  auto Rcurrent_resolution = query->getCurrentResolution();
  auto Rsamples = query->logic_samples;
  auto Rbuffer = query->buffer;
  auto Rfilter_query = query->filter.query;

  int index = Utils::find(query->end_resolutions, query->end_resolution) + 1;
  int end_resolution = query->end_resolutions[index];
  VisusReleaseAssert(setBoxQueryEndResolution(query,end_resolution));

  //asssume no merging
  query->buffer = Array();

  if (!query->allocateBufferIfNeeded())
    return query->setFailed(query->aborted() ? "query aborted" : "out of memory");

  //cannot merge, scrgiorgio: can it really happen??? (maybe when I produce numpy arrays by projecting script...)
  if (!Rsamples.valid() || Rsamples.nsamples != Rbuffer.dims)
    return;

  //solve the problem of missing blocks here...
  InterpolateBufferOperation op;
  if (!ExecuteOnCppSamples(op, query->buffer.dtype, query->logic_samples, query->buffer, Rsamples, Rbuffer, query->aborted))
    return query->setFailed(query->aborted() ? "query aborted" : "merge of samples (interpolate) failed");

  //I must be sure that 'inserted samples' from Rbuffer must be untouched in Wbuffer
  //this is for wavelets where I need the coefficients to be right
  if (!insertSamples(query->logic_samples, query->buffer, Rsamples, Rbuffer, query->aborted))
    return query->setFailed(query->aborted() ? "query aborted" : "merge of samples (insert) failed");

  query->filter.query = Rfilter_query;
  query->setCurrentResolution(Rcurrent_resolution);
}

//////////////////////////////////////////////////////////////////////////////////////////
bool IdxDataset::setBoxQueryEndResolution(SharedPtr<BoxQuery> query, int value)
{
  auto bitmask = this->idxfile.bitmask;

  VisusAssert(query->end_resolution < value);
  query->end_resolution = value;

  auto start_resolution = query->start_resolution;
  auto end_resolution   = query->end_resolution;
  auto logic_box = query->logic_box.withPointDim(this->getPointDim());

  //special case for query with filters
  //I need to go level by level [0,1,2,...] in order to reconstruct the original data
  if (auto filter = query->filter.dataset_filter)
  {
    //important to return the "final" number of samples (see execute for loop)
    logic_box = this->adjustBoxQueryFilterBox(query.get(), filter.get(), logic_box, end_resolution);
    query->filter.adjusted_logic_box = logic_box;
  }
  
  PointNi delta = this->level_samples[end_resolution].delta;

  //I get twice the samples of the samples because I'm allowing the blocking '1' bit to be anything coming from previous levels
  if (start_resolution == 0 && end_resolution > 0)
    delta[bitmask[end_resolution]] >>= 1;

  PointNi P1incl, P2incl;
  for (int H = start_resolution; H <= end_resolution; H++)
  {
    int bit = bitmask[H];

    LogicSamples Lsamples = this->level_samples[H];

    BoxNi box = Lsamples.alignBox(logic_box);
    if (!box.isFullDim())
      continue;

    PointNi p1incl = box.p1;
    PointNi p2incl = box.p2 - Lsamples.delta;
    P1incl = P1incl.getPointDim() ? PointNi::min(P1incl, p1incl) : p1incl;
    P2incl = P2incl.getPointDim() ? PointNi::max(P2incl, p2incl) : p2incl;
  }

  if (!P1incl.getPointDim())
    return false;

  logic_box = BoxNi(P1incl, P2incl + delta);
  query->logic_samples = LogicSamples(logic_box, delta);
  VisusAssert(query->logic_samples.valid());
  return true;

}

//////////////////////////////////////////////////////////////////////////////////////////
bool IdxDataset::mergeBoxQueryWithBlockQuery(SharedPtr<BoxQuery> query, SharedPtr<BlockQuery> block_query)
{
  if (!query->allocateBufferIfNeeded())
    return false;

  if (bool bRowMajor = block_query->buffer.layout.empty())
  {
    VisusAssert(query->field.dtype == block_query->field.dtype);

    auto bitsperblock = getDefaultBitsPerBlock();
    DatasetBitmask bitmask = this->idxfile.bitmask;
    int            hstart = std::max(query->getCurrentResolution() + 1, block_query->blockid == 0 ? 0 : block_query->H);
    int            hend = std::min(query->getEndResolution(), block_query->H);

    auto Bsamples = block_query->logic_samples;

    if (!Bsamples.valid())
      return false;

    auto Wsamples = query->logic_samples;
    auto Wbuffer = query->buffer;

    auto Rsamples = Bsamples;
    auto Rbuffer = block_query->buffer;

    if (query->mode == 'w')
    {
      std::swap(Wsamples, Rsamples);
      std::swap(Wbuffer, Rbuffer);
    }

    if (block_query->blockid == 0)
    {
      /* Important note
      Merging directly the block:

        return query->mode=='w'?
          merge(Bbox,query_samples):
          merge(query_samples,Bbox);

      is wrong. In fact there are queries with filters that need to go level by level (coarse to fine).
      If I do the wrong way:

      filter-query:=
        Q(H=0)             <- I would directly merge block 0
        Q(H=1)             <- I would directly merge block 0. WRONG!!! I cannot overwrite samples belonging to H=0 since they have the filter already applied
        ...
        Q(H=bitsperblock)

      */

      for (int H = hstart; !query->aborted() && H <= hend; ++H)
      {
        auto Lsamples = this->level_samples[H];
        auto Lbuffer = Array(Lsamples.nsamples, block_query->field.dtype);

        /*
        NOTE the pipeline is:
             Wsamples <- Lsamples <- Rsamples

         but since it can be that Rsamples writes only a subset of Lsamples
         i.e.  I allocate Lsamples buffer and one of its samples at position P is not written by Rsamples
               that sample P will overwrite some Wsamples P'

         For this reason I do at the beginning:

          LbLsamplesox <- Wsamples

        int this way I'm sure that all Wsamples P' are left unmodified

        Note also that merge can fail simply because there are no samples to merge at a certain level
        */

        insertSamples(Lsamples, Lbuffer, Wsamples, Wbuffer, query->aborted);
        insertSamples(Lsamples, Lbuffer, Rsamples, Rbuffer, query->aborted);
        insertSamples(Wsamples, Wbuffer, Lsamples, Lbuffer, query->aborted);
      }

      return query->aborted() ? false : true;
    }
    else
    {
      VisusAssert(hstart == hend);
      return insertSamples(Wsamples, Wbuffer, Rsamples, Rbuffer, query->aborted);
    }
  }
  else
  {
    //block query is hzorder, query is rowmajor
    ConvertHzOrderSamples op;
    return NeedToCopySamples(op, query->field.dtype, this, query.get(), block_query.get());
  }
}






/////////////////////////////////////////////////////////////////////////////////////////////
class InsertIntoPointQuery
{
public:

  //operator()
  template <class Sample>
  bool execute(IdxDataset* vf, PointQuery* query, BlockQuery* block_query, PointNi depth_mask, std::vector< std::pair<int, int> > v, Aborted aborted)
  {
    auto& Wbuffer = query->buffer;       auto write = GetSamples<Sample>(Wbuffer);
    auto& Rbuffer = block_query->buffer; auto read = GetSamples<Sample>(Rbuffer);

    if (block_query->buffer.layout == "hzorder")
    {
      for (auto& it : v)
        write[it.first] = read[it.second];

      return true;
    }
    else
    {
      VisusAssert(Rbuffer.layout.empty());
      PointNi stride = Rbuffer.dims.stride();
      PointNi block_origin = block_query->logic_samples.logic_box.p1;
      PointNi block_shift = block_query->logic_samples.shift;
      auto points = query->points->c_ptr<Int64*>();
      int pdim = vf->getPointDim();
      switch (pdim)
      {
#define OFFSET(I) (stride[I] * ((((points+it.first * pdim)[I] & depth_mask[I])-block_origin[I])>>block_shift[I]))
      case 1: for (auto& it : v) write[it.first] = read[OFFSET(0)]; return true;
      case 2: for (auto& it : v) write[it.first] = read[OFFSET(0) + OFFSET(1)]; return true;
      case 3: for (auto& it : v) write[it.first] = read[OFFSET(0) + OFFSET(1) + OFFSET(2)]; return true;
      case 4: for (auto& it : v) write[it.first] = read[OFFSET(0) + OFFSET(1) + OFFSET(2) + OFFSET(3)]; return true;
      case 5: for (auto& it : v) write[it.first] = read[OFFSET(0) + OFFSET(1) + OFFSET(2) + OFFSET(3) + OFFSET(4)]; return true;
#undef OFFSET
      }
      VisusAssert(false);
      return false;
    }
  }
};

///////////////////////////////////////////////////////////////////////////////////////
bool IdxDataset::executePointQuery(SharedPtr<Access> access, SharedPtr<PointQuery> query)
{
  if (!query)
    return false;

  VisusReleaseAssert(query->mode == 'r');

  if (!(query->isRunning() && query->getCurrentResolution() < query->getEndResolution()))
    return false;


  auto aborted = query->aborted;
  if (aborted())
  {
    query->setFailed("query aborted");
    return false;
  }

  //pure remote?
  if (!access)
    return executePointQueryOnServer(query);

  auto npoints = query->getNumberOfPoints();
  auto tot = npoints.innerProduct();

  if (query->buffer.dims != npoints)
  {
    //solve the problem of missing blocks
    if (query->buffer.valid())
    {
      query->buffer = ArrayUtils::resample(npoints, query->buffer);
      if (!query->buffer.valid())
      {
        query->setFailed("out of memory");
        return false;
      }
    }
    else
    {
      if (!query->buffer.resize(npoints, query->field.dtype, __FILE__, __LINE__))
      {
        query->setFailed("out of memory");
        return false;
      }

      query->buffer.fillWithValue(query->field.default_value);
    }
  }

  VisusAssert(query->buffer.dtype == query->field.dtype);
  VisusAssert(query->buffer.c_size() == query->getByteSize());
  VisusAssert(query->buffer.dims == query->npoints);
  VisusAssert((Int64)query->points->c_size() == npoints.innerProduct() * sizeof(Int64) * 3);


  //blockid-> (offset of query buffer, block offset)
  std::map<BigInt, std::vector< std::pair<int, int> > > blocks;

  int pdim = this->getPointDim();
  PointNi p(pdim);

  auto bounds = this->getLogicBox();
  auto last_bitmask = ((BigInt)1) << (getMaxResolution());
  auto hzorder = HzOrder(idxfile.bitmask);
  auto depth_mask = hzorder.getLevelP2Included(query->end_resolution);
  auto bitsperblock = access->bitsperblock;
  auto samplesperblock = 1 << bitsperblock;

  //if this is not available I use the slower conversion p->zaddress->Hz
  if (!this->hzaddress_conversion_pointquery)
  {
    PrintWarning("The hzaddress_conversion_pointquery has not been created, so loc-by-loc queries will be a lot slower!!!!");

    //so you investigate why it's happening! .... I think only for the iphone could make sense....
#if defined(_DEBUG)
    VisusAssert(false);
#endif

    auto SRC = (Int64*)query->points->c_ptr();
    BigInt hzaddress;
    for (int N = 0; N < tot; N++, SRC += pdim)
    {
      if (aborted()) {
        query->setFailed("query aborted");
        return false;
      }
      if (pdim >= 1) { p[0] = SRC[0]; if (!(p[0] >= bounds.p1[0] && p[0] < bounds.p2[0])) continue; p[0] &= depth_mask[0]; }
      if (pdim >= 2) { p[1] = SRC[1]; if (!(p[1] >= bounds.p1[1] && p[1] < bounds.p2[1])) continue; p[1] &= depth_mask[1]; }
      if (pdim >= 3) { p[2] = SRC[2]; if (!(p[2] >= bounds.p1[2] && p[2] < bounds.p2[2])) continue; p[2] &= depth_mask[2]; }
      if (pdim >= 4) { p[3] = SRC[3]; if (!(p[3] >= bounds.p1[3] && p[3] < bounds.p2[3])) continue; p[3] &= depth_mask[3]; }
      if (pdim >= 5) { p[4] = SRC[4]; if (!(p[4] >= bounds.p1[4] && p[4] < bounds.p2[4])) continue; p[4] &= depth_mask[4]; }
      hzaddress = hzorder.pointToHzAddress(p);
      blocks[hzaddress >> bitsperblock].push_back(std::make_pair(N, (int)(hzaddress % samplesperblock)));
    }
  }
  //the conversion from point to Hz will be faster
  else
  {
    BigInt zaddress;
    int    shift;
    auto   SRC = (Int64*)query->points->c_ptr();
    auto   loc = this->hzaddress_conversion_pointquery->loc;
    BigInt hzaddress;

    for (int N = 0; N < tot; N++, SRC += pdim)
    {
      if (aborted()) {
        query->setFailed("query aborted");
        return false;
      }
      if (pdim >= 1) { p[0] = SRC[0]; if (!(p[0] >= bounds.p1[0] && p[0] < bounds.p2[0])) continue; p[0] &= depth_mask[0]; shift =                (loc[0][p[0]].second); zaddress  = loc[0][p[0]].first; }
      if (pdim >= 2) { p[1] = SRC[1]; if (!(p[1] >= bounds.p1[1] && p[1] < bounds.p2[1])) continue; p[1] &= depth_mask[1]; shift = std::min(shift, loc[1][p[1]].second); zaddress |= loc[1][p[1]].first; }
      if (pdim >= 3) { p[2] = SRC[2]; if (!(p[2] >= bounds.p1[2] && p[2] < bounds.p2[2])) continue; p[2] &= depth_mask[2]; shift = std::min(shift, loc[2][p[2]].second); zaddress |= loc[2][p[2]].first; }
      if (pdim >= 4) { p[3] = SRC[3]; if (!(p[3] >= bounds.p1[3] && p[3] < bounds.p2[3])) continue; p[3] &= depth_mask[3]; shift = std::min(shift, loc[3][p[3]].second); zaddress |= loc[3][p[3]].first; }
      if (pdim >= 5) { p[4] = SRC[4]; if (!(p[4] >= bounds.p1[4] && p[4] < bounds.p2[4])) continue; p[4] &= depth_mask[4]; shift = std::min(shift, loc[4][p[4]].second); zaddress |= loc[4][p[4]].first; }
      hzaddress = ((zaddress | last_bitmask) >> shift);
      blocks[hzaddress >> bitsperblock].push_back(std::make_pair(N, int(hzaddress % samplesperblock)));
    }
  }

  //do the for loop block aligned
  WaitAsync< Future<Void> > wait_async;

  bool bWasReading = access->isReading();

  if (!bWasReading)
    access->beginRead();

  for (auto it : blocks)
  {
    auto blockid = it.first;
    auto v = it.second;
    auto block_query = createBlockQuery(blockid, query->field, query->time, 'r', aborted);
    this->executeBlockQuery(access, block_query);
    wait_async.pushRunning(block_query->done).when_ready([this, query, block_query, v, aborted, depth_mask](Void) {

      if (aborted() || block_query->failed())
        return;

      InsertIntoPointQuery op;
      NeedToCopySamples(op, query->field.dtype, this, query.get(), block_query.get(), depth_mask, v, aborted);

      if (aborted())
        return;
    });
  }

  if (!bWasReading)
    access->endRead();

  wait_async.waitAllDone();

  if (aborted()) {
    query->setFailed("query aborted");
    return false;
  }

  query->cur_resolution = query->end_resolution;
  return true;
}




////////////////////////////////////////////////////////////////////
void IdxDataset::readDatasetFromArchive(Archive& ar)
{
  Dataset::readDatasetFromArchive(ar);

  auto key = bitmask.toString();

  //create hz address conversion for box query
  {
    ScopedLock lock(hz_address_conversion.boxquery.lock);

    auto it = hz_address_conversion.boxquery.map.find(key);
    if (it != hz_address_conversion.boxquery.map.end())
      this->hzaddress_conversion_boxquery = it->second;
  }

  if (!this->hzaddress_conversion_boxquery)
  {
    this->hzaddress_conversion_boxquery = std::make_shared<BoxQueryHzConversion>(bitmask);
    {
      ScopedLock lock(hz_address_conversion.boxquery.lock);
      hz_address_conversion.boxquery.map[key] = this->hzaddress_conversion_boxquery;
    }
  }

  //create the loc-cache only for 3d data, in 2d I know I'm not going to use it!
  //instead in 3d I will use it a lot (consider a slice in odd position)
  if (bitmask.getPointDim() == 3 && !this->hzaddress_conversion_pointquery)
  {
      ScopedLock lock(hz_address_conversion.pointquery.lock);
      auto it = hz_address_conversion.pointquery.map.find(key);
      if (it != hz_address_conversion.pointquery.map.end())
        this->hzaddress_conversion_pointquery = it->second;
    }

    if (!this->hzaddress_conversion_pointquery)
    {
      this->hzaddress_conversion_pointquery = std::make_shared<PointQueryHzConversion>(bitmask);
      {
        ScopedLock lock(hz_address_conversion.pointquery.lock);
        hz_address_conversion.pointquery.map[key] = this->hzaddress_conversion_pointquery;
      }
    }
  }

} //namespace Visus
