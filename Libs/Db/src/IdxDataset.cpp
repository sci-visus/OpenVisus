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
#include <Visus/ModVisusAccess.h>
#include <Visus/File.h>
#include <Visus/DirectoryIterator.h>
#include <Visus/DatasetFilter.h>
#include <Visus/StringTree.h>
#include <Visus/OnDemandAccess.h>
#include <Visus/IdxHzOrder.h>

#ifdef WIN32
#pragma warning(disable:4996) // 'sprintf': This function or variable may be unsafe
#endif

namespace Visus {


//box query type
typedef struct
{
  int    H;
  BoxNi  box;
}
FastLoopStack;

//////////////////////////////////////////////////////////////////////////////
class IdxMandelbrotAccess : public Access
{
public:

  IdxDataset* dataset=nullptr;

  //create
  IdxMandelbrotAccess(IdxDataset* dataset,StringTree config) 
  {
    this->dataset=dataset;
    this->can_read       = true;
    this->can_write      = false;
    this->bitsperblock   = dataset->getDefaultBitsPerBlock();
  }

  //destructor 
  virtual ~IdxMandelbrotAccess(){
  }

  //readBlock
  virtual void readBlock(SharedPtr<BlockQuery> query) override
  {
    const Field& field=query->field;

    //wrong field (so far only 1*float32)
    if (field.dtype!=(DTypes::FLOAT32)) 
      return readFailed(query);

    Time t1=Time::now();
    VisusAssert(this->bitsperblock==dataset->getDefaultBitsPerBlock());

    int samplesperblock=getSamplesPerBlock();
    int blockdim    = (int)(field.dtype.getByteSize(samplesperblock));

    DatasetBitmask bitmask=dataset->getBitmask();
    int pdim = bitmask.getPointDim();

    int fromh = HzOrder::getAddressResolution(bitmask,query->start_address);
    int toh   = HzOrder::getAddressResolution(bitmask,query->end_address-1);

    LogicSamples logic_samples=query->logic_samples;
    if (!logic_samples.valid())
      return readFailed(query);

    if (!query->allocateBufferIfNeeded())
      return readFailed(query);

    auto& buffer=query->buffer;
    buffer.layout="";

    BoxNi   box = dataset->getLogicBox();
    PointNi dim = box.size();

    Float32* ptr=(Float32*)query->buffer.c_ptr();
    for (auto loc = ForEachPoint(buffer.dims); !loc.end(); loc.next())
    {
      PointNi pos= logic_samples.pixelToLogic(loc.pos);
      double x=(pos[0]-box.p1[0])/(double)(dim[0]);
      double y=(pos[1]-box.p1[1])/(double)(dim[1]);
      *ptr++=(Float32)Mandelbrot(x,y);
    }

    return readOk(query);
  }

  //writeBlock
  virtual void writeBlock(SharedPtr<BlockQuery> query)  override
  {
    VisusAssert(false);
    return writeFailed(query);
  }

  //http://nuclear.mutantstargoat.com/articles/sdr_fract/
  static inline double Mandelbrot(double x,double y) 
  {
    const double scale=2;
    const double center_x=0;
    const double center_y=0;
    const int iter=48;

    double c_x = 1.3333 * (x-0.5) * scale - center_x ;
    double c_y =          (y-0.5) * scale - center_y;
    double z_x = c_x;
    double z_y = c_y;
    int i;for(i=0;i<iter;i++,z_x=x,z_y=y) 
    {
      x = (z_x*z_x-z_y*z_y) + c_x;
      y = (z_y*z_x+z_x*z_y) + c_y;
      if((x*x+y*y)>4.0) return double(i)/iter;
    }
    return 0.0; 
  }

}; 

////////////////////////////////////////////////////////
class IdxBoxQueryHzAddressConversion 
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
      return sizeof(PointNi)*(this->num);
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
  IdxBoxQueryHzAddressConversion(const DatasetBitmask& bitmask_) : bitmask(bitmask_)  
  {
    int maxh = bitmask.getMaxResolution();
    while (maxh >= this->levels.size())
      this->levels.push_back(std::make_shared<Level>(bitmask, (int)this->levels.size()));
  }


};

////////////////////////////////////////////////////////
class IdxPointQueryHzAddressConversion 
{
public:

  Int64 memsize=0;

  std::vector< std::pair<BigInt,Int32>* > loc;

  //create
  IdxPointQueryHzAddressConversion(DatasetBitmask bitmask)
  {
    //todo cases for which I'm using regexp
    auto MaxH = bitmask.getMaxResolution();
    BigInt last_bitmask = ((BigInt)1)<<MaxH;

    HzOrder hzorder(bitmask);
    int pdim = bitmask.getPointDim();
    loc.resize(pdim);
  
    for (int D=0;D<pdim;D++)
    {
      Int64 dim= bitmask.getPow2Dims()[D]; 

      this->loc[D]=new std::pair<BigInt,Int32>[dim];

      BigInt one=1;
      for (int i=0;i<dim;i++)
      {
        PointNi p(pdim);
        p[D]=i;
        auto& pair=this->loc[D][i];
        pair.first=hzorder.interleave(p);
        pair.second=0;
        BigInt temp =pair.first | last_bitmask;
        while ((one & temp)==0) {pair.second++;temp >>= 1;}
        temp >>= 1;
        pair.second++;
      }
    }
  }

  //destructor
  ~IdxPointQueryHzAddressConversion() {
    for (auto it : loc)
      delete[] it;
  }

};

///////////////////////////////////////////////////////////////////////////
class InsertBlockQueryHzOrderSamplesToBoxQuery
{
public:

  template <class Sample>
  bool execute(IdxDataset*  vf,BoxQuery* query,BlockQuery* block_query)
  {
    #define PUSH()  (*((stack)++))=(item)
    #define POP()   (item)=(*(--(stack)))
    #define EMPTY() ((stack)==(STACK))

    VisusAssert(query->field.dtype==block_query->buffer.dtype);
    VisusAssert(block_query->buffer.layout=="hzorder");

    bool bInvertOrder=query->mode=='w';

    DatasetBitmask bitmask=vf->getBitmask();
    int            max_resolution=vf->getMaxResolution();
    BigInt         HzFrom=block_query->start_address;
    BigInt         HzTo  =block_query->end_address;
    HzOrder        hzorder(bitmask);
    int            hstart=std::max(query->getCurrentResolution()+1 ,HzOrder::getAddressResolution(bitmask,HzFrom));
    int            hend  =std::min(query->getEndResolution(),HzOrder::getAddressResolution(bitmask,HzTo-1));
    int            samplesperblock=(int)cint64(HzTo-HzFrom);
    int            bitsperblock= Utils::getLog2(samplesperblock);

    VisusAssert(HzFrom==0 || hstart==hend);

    auto Wbox=GetSamples<Sample>(      query->buffer);
    auto Rbox=GetSamples<Sample>(block_query->buffer);

    if (bInvertOrder)
      std::swap(Wbox,Rbox);

    auto address_conversion = vf->hzaddress_conversion_boxquery;
    VisusReleaseAssert(address_conversion);

    int              numused=0;
    int              bit;
    Int64 delta;
    BoxNi            logic_box        = query->logic_samples.logic_box;
    PointNi          stride           = query->getNumberOfSamples().stride();
    PointNi          qshift           = query->logic_samples.shift;
    BigInt           numpoints;
    Aborted          aborted=query->aborted;

    FastLoopStack item,*stack=NULL;
    FastLoopStack STACK[DatasetBitmaskMaxLen+1];

    //layout of the block
    auto block_logic_box= block_query->getLogicBox();
    if (!block_logic_box.valid())
      return false;

    //deltas
    std::vector<Int64> fldeltas(max_resolution+1);
    for (int H = 0; H <= max_resolution; H++)
      fldeltas[H] = H? (hzorder.getLevelDelta(H)[bitmask[H]] >> 1) : 0;

    for (int H=hstart;H<=hend;H++)
    {
      if (aborted())
        return false;

      LogicSamples Lsamples=vf->getLevelSamples(H);
      PointNi  lshift= Lsamples.shift;

      BoxNi   zbox = (HzFrom!=0)? block_logic_box : Lsamples.logic_box;
      BigInt  hz   = hzorder.getAddress(zbox.p1);

      BoxNi user_box= logic_box.getIntersection(zbox);
      BoxNi box= Lsamples.alignBox(user_box);
      if (!box.isFullDim())
        continue;
     
      VisusAssert(address_conversion->levels[H]);
      const auto& fllevel = *(address_conversion->levels[H]);
      int cachable = std::min(fllevel.num,samplesperblock);
    
      //i need this to "split" the fast loop in two chunks
      VisusAssert(cachable>0 && cachable<=samplesperblock);
    
      //push root in the kdtree
      {
        item.box     = zbox;
        item.H       = H? std::max(1,H-bitsperblock) : (0);
        stack        = STACK;
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
          hz+=(((BigInt)1)<<(H-item.H));
          continue;
        }

        // all intersection and all the samples are contained in the FastLoopCache
        numpoints = ((BigInt)1)<<(H-item.H);
        if (numpoints<=cachable && box.containsBox(item.box))
        {
          Int64    hzfrom = cint64(hz-HzFrom);
          Int64    num    = cint64(numpoints);
          PointNi* cc     = (PointNi*)fllevel.cached_points->c_ptr();
          const PointNi  query_p1= logic_box.p1;
          PointNi  P=item.box.p1;

          ++numused;

          VisusAssert(hz>=HzFrom && hz<HzTo);

          Int64 from = stride.dotProduct((P-query_p1).rightShift(qshift));

          auto& Windex=bInvertOrder? hzfrom :   from;
          auto& Rindex=bInvertOrder?   from : hzfrom;

          auto shift = (lshift - qshift);

          //slow version (enable it if you have problems)
#if 0
          for (;num--;++hzfrom,++cc)
          {
            from = stride.dotProduct((P-query_p1).rightShift(qshift));
            Wbox[Windex]=Rbox[Rindex];
            P+=(cc->leftShift(lshift));
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

          hz+=numpoints;
          continue;
        }

        //kd-traversal code
        bit   = bitmask   [item.H];
        delta = fldeltas  [item.H];
        ++item.H;
        item.box.p1[bit]+=delta                        ; VisusAssert(item.box.isFullDim());PUSH();
        item.box.p1[bit]-=delta;item.box.p2[bit]-=delta; VisusAssert(item.box.isFullDim());PUSH();
      }
    }

    VisusAssert (numused>0);
    return true;

    #undef PUSH
    #undef POP
    #undef EMPTY
  }

};

/////////////////////////////////////////////////////////////////////////////////////////////
class InsertBlockQuerySamplesIntoPointQuery 
{
public:

  //operator()
  template <class Sample>
  bool execute(IdxDataset* vf,PointQuery* query,BlockQuery* block_query,std::pair<BigInt,Int32>* A,std::pair<BigInt,Int32>* B,Aborted aborted)
  {
    BigInt HzFrom = block_query->start_address;
    BigInt HzTo   = block_query->end_address;

    int sample_bitsize=query->field.dtype.getBitSize();

    if (!query->allocateBufferIfNeeded())
      return false;

    auto& Wbuffer=      query->buffer; auto write=GetSamples<Sample>(Wbuffer);
    auto& Rbuffer=block_query->buffer; auto read =GetSamples<Sample>(Rbuffer);

    if (bool bIsHzOrder=block_query->buffer.layout=="hzorder")
    {
      for (auto cursor=A;!aborted() && cursor<B;cursor++)
        write[cursor->second]=read[cint64(cursor->first-HzFrom)];
      return !aborted();
    }

    VisusAssert(Rbuffer.layout.empty());
    int            maxh          = vf->getMaxResolution();
    DatasetBitmask bitmask       = vf->getBitmask();
    int            pdim          = vf->getPointDim();
    HzOrder        hzorder         (bitmask, maxh);
    PointNi        depth_mask    = hzorder.getLevelP2Included(query->end_resolution);

    LogicSamples block_samples= block_query->logic_samples;
    if (!block_samples.valid())
      return false;

    PointNi stride = Rbuffer.dims.stride();
    PointNi p0     = block_samples.logic_box.p1;
    PointNi shift  = block_samples.shift;

    auto points = (Int64*)query->points.c_ptr();

    switch (pdim)
    {
      #define HZOFFSET(__coord__) (stride[__coord__] * ((((points+cursor->second*pdim)[__coord__] & depth_mask[__coord__])-p0[__coord__])>>shift[__coord__]))
      case 1:
        for (auto cursor=A;!aborted() && cursor<B;cursor++)
          write[cursor->second]=read[HZOFFSET(0)];
        return !aborted();

      case 2:
        for (auto cursor=A;!aborted() && cursor<B;cursor++)
          write[cursor->second]=read[HZOFFSET(0)+HZOFFSET(1)];
        return !aborted();

      case 3:
        for (auto cursor=A;!aborted() && cursor<B;cursor++)
          write[cursor->second]=read[HZOFFSET(0)+HZOFFSET(1)+HZOFFSET(2)];
        return !aborted();

      case 4:
        for (auto cursor=A;!aborted() && cursor<B;cursor++)
          write[cursor->second]=read[HZOFFSET(0)+HZOFFSET(1)+HZOFFSET(2)+HZOFFSET(3)];
        return !aborted();

      case 5:
        for (auto cursor=A;!aborted() && cursor<B;cursor++)
          write[cursor->second]=read[HZOFFSET(0)+HZOFFSET(1)+HZOFFSET(2)+HZOFFSET(3)+HZOFFSET(4)];
        return !aborted();

      default:
        ThrowException("todo");
        return false;

      #undef HZOFFSET
    }

    VisusAssert(false);
    return false;
  }

};

//////////////////////////////////////////////////////////////////////////////////////////
IdxDataset::IdxDataset() {
}

//////////////////////////////////////////////////////////////////////////////////////////
IdxDataset::~IdxDataset(){
}

///////////////////////////////////////////////////////////
LogicSamples IdxDataset::getLevelSamples(int H)
{
  HzOrder hzorder(getBitmask());
  PointNi delta = hzorder.getLevelDelta(H);
  BoxNi box(hzorder.getLevelP1(H),hzorder.getLevelP2Included(H) + delta);
  auto ret=LogicSamples(box, delta);
  VisusAssert(ret.valid());
  return ret;
}


///////////////////////////////////////////////////////////
void IdxDataset::tryRemoveLockAndCorruptedBinaryFiles(String directory)
{
  PrintInfo("Trying to remove locks and corrupted binary files in directory",directory,"...");

  std::vector<String> lock_files;
  DirectoryIterator::findAllFilesEndingWith(lock_files,directory,".lock");

  for (int I=0;I<(int)lock_files.size();I++)
  {
    String lock_filename=lock_files[I];
    {
      bool bOk=FileUtils::removeFile(lock_filename);
      PrintInfo("Removing lock_filename", lock_filename,bOk?"ok":"ERROR");
    }

    String bin_filename =lock_files[I].substr(0,lock_filename.length()-5);
    if (FileUtils::existsFile(bin_filename)) 
    {
      bool bOk=FileUtils::removeFile(bin_filename);
      PrintInfo("Removing bin_filename", bin_filename, bOk?"ok":"ERROR");
    }
  }
}

////////////////////////////////////////////////////////////////////////
void IdxDataset::removeFiles()
{
  HzOrder hzorder(idxfile.bitmask);
  BigInt tot_blocks= getTotalNumberOfBlocks();
  auto  samplesperblock=1<<idxfile.bitsperblock;
  auto  access=std::make_shared<IdxDiskAccess>(this);

  std::set<String> filenames;

  for (auto time :  idxfile.timesteps.asVector())
  {
    for (auto field : idxfile.fields)
    {
      for (BigInt blockid=0;blockid<=tot_blocks;blockid++)
      {
        auto filename=access->getFilename(field,time,blockid);
        if (filenames.count(filename))
          continue;
        filenames.insert(filename);

        if (!FileUtils::existsFile(filename))
          continue;

        Path path(filename);
        FileUtils::removeFile(path);

        // this will work only if the parent directory is empty()
        FileUtils::removeDirectory(path.getParent()); 
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////
void IdxDataset::compressDataset(std::vector<String> compression)
{
  // for future version: here I'm making the assumption that a file contains multiple fields
  if (idxfile.version != 6)
    ThrowException("unsupported");
    
  PrintInfo("Compressing dataset",StringUtils::join(compression));
  	
  int nlevels=getMaxResolution() + 1;
  VisusReleaseAssert(compression.size()<=nlevels);

  // example ["zip","jpeg","jpeg"] means last level "jpeg", last-level-minus-one "jpeg" all others zip
  while (compression.size() < nlevels)
    compression.insert(compression.begin(), compression.front());
  
  //save the new idx file oonly if compression is equal for all levels
  if (std::set<String>(compression.begin(), compression.end()).size() == 1)
  {
    for (auto& field : idxfile.fields)
      field.default_compression = compression[0];

    String filename = Url(this->getUrl()).getPath();
    idxfile.save(filename);
  }

  auto Raccess = std::make_shared<IdxDiskAccess>(this);
  Raccess->disableWriteLock();
  Raccess->disableAsync();

  auto Waccess = std::make_shared<IdxDiskAccess>(this);
  Waccess->disableWriteLock();
  Waccess->disableAsync();

  Aborted aborted; 

  //for each time
  auto timesteps=idxfile.timesteps.asVector();
  Int64 overall_file_size = 0;

  int nfields = (int)idxfile.fields.size();
  for (auto Wtime : timesteps)
  {
    auto Rtime = Wtime;

    //for each file...
    BigInt total_block = getTotalNumberOfBlocks();
    BigInt tot_files = (total_block  / idxfile.blocksperfile) + ((total_block % idxfile.blocksperfile)? 1 : 0);

    std::set<String> filenames;

    for (BigInt fileid = 0; fileid < tot_files; fileid++)
    {
      PrintInfo("Compressing file", fileid, "/", tot_files);

      std::vector< std::vector<Array> > file_blocks(idxfile.blocksperfile, std::vector<Array>(nfields));

      //read file blocks
      Raccess->beginRead();
      {
        auto blockid = fileid * idxfile.blocksperfile;
        for (BigInt B = 0; B < idxfile.blocksperfile && blockid < total_block; B++, blockid++)
        {
          for (int F = 0; F < nfields; F++)
          {
            auto Rfield = idxfile.fields[F];
            auto filename = Raccess->getFilename(Rfield, Rtime, blockid);
            if (!FileUtils::existsFile(filename))
              continue;

            filenames.insert(filename);

            auto read_block = std::make_shared<BlockQuery>(this, Rfield, Rtime, Raccess->getStartAddress(blockid), Raccess->getEndAddress(blockid), 'r', aborted);

            //could fail because block does not exist
            if (executeBlockQueryAndWait(Raccess, read_block))
              file_blocks[B][F] = read_block->buffer;
          }
        }
      }
      Raccess->endRead();

      //now it's safe to remove old files
      for (auto filename : filenames)
        FileUtils::removeFile(filename);

      //write file blocks
      Waccess->beginWrite();
      {
        auto blockid = fileid * idxfile.blocksperfile;
        for (BigInt B = 0; B < idxfile.blocksperfile; B++, blockid++)
        {
          for (int F = 0; F < idxfile.fields.size(); F++)
          {
            auto Wfield = idxfile.fields[F];

            if (!file_blocks[B][F])
              continue;

            //compression can depend on level
            auto HzStart = Waccess->getStartAddress(blockid);
            auto HzEnd = Waccess->getEndAddress(blockid);
            int H = HzOrder::getAddressResolution(idxfile.bitmask, HzStart);
            VisusReleaseAssert(H >= 0 && H < compression.size());
            Wfield.default_compression = compression[H];

            auto write_block = std::make_shared<BlockQuery>(this, Wfield, Wtime, HzStart, HzEnd, 'w', aborted);
            write_block->buffer = file_blocks[B][F];

            if (!executeBlockQueryAndWait(Waccess, write_block))
              ThrowException("Fatal error writing Wfield(", F, "block", blockid);
          }
        }
      }
      Waccess->endWrite();
    }
  }
}


///////////////////////////////////////////////////////////////////////////////////
BoxNi IdxDataset::adjustFilterBox(BoxQuery* query,DatasetFilter* filter,BoxNi user_box,int H) 
{
  //there are some case when I need alignment with pow2 box, for example when doing kdquery=box with filters
  auto bitmask = getBitmask();
  HzOrder hzorder(bitmask);
  int pdim = bitmask.getPointDim();

  PointNi delta=hzorder.getLevelDelta(H);

  BoxNi domain = query->filter.domain;

  //important! for the filter alignment
  BoxNi box= user_box.getIntersection(domain);

  if (!box.isFullDim())
    return box;

  PointNi filterstep=filter->getFilterStep(H);

  for (int D=0;D<pdim;D++) 
  {
    //what is the world step of the filter at the current resolution
    Int64 FILTERSTEP=filterstep[D];

    //means only one sample so no alignment
    if (FILTERSTEP==1) 
      continue;

    box.p1[D]=Utils::alignLeft(box.p1[D]  ,(Int64)0,FILTERSTEP);
    box.p2[D]=Utils::alignLeft(box.p2[D]-1,(Int64)0,FILTERSTEP)+FILTERSTEP; 
  }

  //since I've modified the box I need to do the intersection with the box again
  //important: this intersection can cause a misalignment, but applyToQuery will handle it (see comments)
  box= box.getIntersection(domain);
  return box;
}



//////////////////////////////////////////////////////////////////
LogicSamples IdxDataset::getAddressRangeSamples(BigInt HzFrom,BigInt HzTo)
{
  const DatasetBitmask& bitmask=this->idxfile.bitmask;
  int pdim = bitmask.getPointDim();

  HzOrder hzorder(bitmask);

  int start_resolution=HzOrder::getAddressResolution(bitmask,HzFrom);
  int end_resolution  =HzOrder::getAddressResolution(bitmask,HzTo-1);
  VisusAssert((HzFrom>0 && start_resolution==end_resolution) || (HzFrom==0 && start_resolution==0));

  PointNi delta(pdim);
  if (HzFrom==0)
  {
    int H=Utils::getLog2(HzTo-HzFrom);
    delta=hzorder.getLevelDelta(H);
    delta[bitmask[H]]>>=1; //I get twice the samples...
  }
  else
  {
    delta=hzorder.getLevelDelta(start_resolution);
  }

  BoxNi box(hzorder.getPoint(HzFrom),hzorder.getPoint(HzTo-1)+delta);

  auto ret=LogicSamples(box,delta);
  VisusAssert(ret.nsamples==HzOrder::getAddressRangeNumberOfSamples(bitmask,HzFrom,HzTo));
  VisusAssert(ret.valid());
  return ret;
}


////////////////////////////////////////////////////////////////////////
SharedPtr<BoxQuery> IdxDataset::createEquivalentBoxQuery(int mode,SharedPtr<BlockQuery> block_query)
{
  auto bitmask = getBitmask();
  int fromh = HzOrder::getAddressResolution(bitmask,block_query->start_address);
  int toh   = HzOrder::getAddressResolution(bitmask,block_query->end_address-1);

  auto block_samples=block_query->logic_samples;
  VisusAssert(block_samples.nsamples.innerProduct()==(block_query->end_address-block_query->start_address));
  VisusAssert(fromh==toh || block_query->start_address==0);

  auto ret=std::make_shared<BoxQuery>(this, block_query->field, block_query->time,mode, block_query->aborted);
  ret->logic_box= block_samples.logic_box;
  ret->setResolutionRange(fromh,toh);
  return ret;
}


////////////////////////////////////////////////////////
bool IdxDataset::convertBlockQueryToRowMajor(SharedPtr<BlockQuery> block_query) 
{
  //already in row major... why are you calling me?
  if (block_query->buffer.layout.empty())
  {
    VisusAssert(false);
    return false;
  }

  //note I cannot use buffer of block_query because I need them to execute other queries
  Array row_major;
  if (!ArrayUtils::deepCopy(row_major,block_query->buffer)) 
    return false;

  auto query= createEquivalentBoxQuery('r',block_query);
  beginQuery(query);

  if (!query->isRunning())
  {
    VisusAssert(false);
    return false;
  }

  //as the query has not already been executed!
  query->setCurrentResolution(query->start_resolution-1);
  query->buffer=row_major; 
  
  if (!mergeBoxQueryWithBlock(query,block_query))
  {
    if (!block_query->aborted()) VisusAssert(false);
    return false;
  }

  //now block query it's row major
  block_query->buffer=row_major;
  block_query->buffer.layout=""; 

  return true;
}


////////////////////////////////////////////////////////////////////
SharedPtr<Access> IdxDataset::createAccess(StringTree config, bool bForBlockQuery)
{
  if (!config.valid())
    config = getDefaultAccessConfig();

  String type =StringUtils::toLower(config.readString("type"));

  //no type, create default
  if (type.empty()) 
  {
    Url url = config.readString("url", getUrl());

    //local disk access
    if (url.isFile())
    {
      return std::make_shared<IdxDiskAccess>(this,config);
    }
    else
    {
      VisusAssert(url.isRemote());

      if (bForBlockQuery)
        return std::make_shared<ModVisusAccess>(this, config);
      else
        //I can execute box/point queries on the remote server
        return SharedPtr<Access>();
    }
  }

  //IdxDiskAccess
  if (type=="disk" || type=="idxdiskaccess")
    return std::make_shared<IdxDiskAccess>(this, config);

  //IdxMandelbrotAccess
  if (type=="idxmandelbrotaccess")
    return std::make_shared<IdxMandelbrotAccess>(this, config);

  //ondemandaccess
  if (type=="ondemandaccess")
    return std::make_shared<OnDemandAccess>(this, config);

  return Dataset::createAccess(config, bForBlockQuery);
}

////////////////////////////////////////////////////////////////////
void IdxDataset::read(Archive& ar)
{
  String url = ar.readString("url");

  IdxFile idxfile;
  ar.readObject("idxfile", idxfile);
  idxfile.validate(url);

  setDatasetBody(ar);
  setKdQueryMode(KdQueryMode::fromString(ar.readString("kdquery", Url(url).getParam("kdquery"))));
  setIdxFile(idxfile);
}


static CriticalSection                                                                HZADDRESS_CONVERSION_BOXQUERY_LOCK;
static std::map<String, SharedPtr<IdxBoxQueryHzAddressConversion> >                   HZADDRESS_CONVERSION_BOXQUERY;

static CriticalSection                                                                HZADDRESS_CONVERSION_POINTQUERY_LOCK;
static std::map<std::pair<String,int> , SharedPtr<IdxPointQueryHzAddressConversion> > HZADDRESS_CONVERSION_POINTQUERY;


////////////////////////////////////////////////////////////
void IdxDataset::setIdxFile(IdxFile value)
{
  this->idxfile=value;

  auto bitmask = value.bitmask;

  setBitmask(bitmask);
  setDefaultBitsPerBlock(value.bitsperblock);
  setLogicBox(value.logic_box);
  setDatasetBounds(value.bounds);
  setTimesteps(value.timesteps);
  
  for (auto field : value.fields)
    addField(field);

  //cache address conversion
  {
    auto key = bitmask.toString();
    {
      ScopedLock lock(HZADDRESS_CONVERSION_BOXQUERY_LOCK);
      auto it = HZADDRESS_CONVERSION_BOXQUERY.find(key);
      if (it != HZADDRESS_CONVERSION_BOXQUERY.end()) 
        this->hzaddress_conversion_boxquery = it->second;
    }

    if (!this->hzaddress_conversion_boxquery)
    {
      this->hzaddress_conversion_boxquery = std::make_shared<IdxBoxQueryHzAddressConversion>(bitmask);
      {
        ScopedLock lock(HZADDRESS_CONVERSION_BOXQUERY_LOCK);
        HZADDRESS_CONVERSION_BOXQUERY[key] = this->hzaddress_conversion_boxquery;
      }
    }
  }

  //create the loc-cache only for 3d data, in 2d I know I'm not going to use it!
  //instead in 3d I will use it a lot (consider a slice in odd position)
  if (bitmask.getPointDim() >= 3 && !this->hzaddress_conversion_pointquery)
  {
    auto key = std::make_pair(bitmask.toString(), this->getMaxResolution());
    {
      ScopedLock lock(HZADDRESS_CONVERSION_POINTQUERY_LOCK);
      auto it = HZADDRESS_CONVERSION_POINTQUERY.find(key);
      if (it != HZADDRESS_CONVERSION_POINTQUERY.end())
        this->hzaddress_conversion_pointquery = it->second;
    }

    if (!this->hzaddress_conversion_pointquery)
    {
      this->hzaddress_conversion_pointquery = std::make_shared<IdxPointQueryHzAddressConversion>(bitmask);
      {
        ScopedLock lock(HZADDRESS_CONVERSION_POINTQUERY_LOCK);
        HZADDRESS_CONVERSION_POINTQUERY[key] = this->hzaddress_conversion_pointquery;
      }
    }
  }
}


//////////////////////////////////////////////////////////////////////////////////////////
bool IdxDataset::mergeBoxQueryWithBlock(SharedPtr<BoxQuery> query,SharedPtr<BlockQuery> block_query)
{
  if (!query->allocateBufferIfNeeded())
    return false;

  if (bool bRowMajor=block_query->buffer.layout.empty())
  {
    VisusAssert(query->field.dtype==block_query->field.dtype);

    DatasetBitmask bitmask=this->getBitmask();
    BigInt         HzFrom=block_query->start_address;
    BigInt         HzTo  =block_query->end_address;
    int            hstart=std::max(query->getCurrentResolution() +1  ,HzOrder::getAddressResolution(bitmask,HzFrom));
    int            hend  =std::min(query->getEndResolution(),HzOrder::getAddressResolution(bitmask,HzTo-1));

    auto Bsamples = block_query->logic_samples;

    if (!Bsamples.valid())
      return false;

    auto Wsamples= query->logic_samples;
    auto Wbuffer = query->buffer;

    auto Rsamples    = Bsamples;
    auto Rbuffer = block_query->buffer;

    if (query->mode == 'w')
    {
      std::swap(Wsamples, Rsamples);
      std::swap(Wbuffer,Rbuffer);
    }

    if (HzFrom==0)
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

      for (int H=hstart; !query->aborted() && H<=hend ; ++H)
      {
        auto Lsamples    = getLevelSamples(H);
        auto Lbuffer = Array(Lsamples.nsamples,block_query->field.dtype);

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

        LogicSamples::merge(Lsamples, Lbuffer, Wsamples,Wbuffer, InsertSamples, query->aborted);
        LogicSamples::merge(Lsamples, Lbuffer, Rsamples,Rbuffer, InsertSamples, query->aborted);
        LogicSamples::merge(Wsamples, Wbuffer, Lsamples,Lbuffer, InsertSamples, query->aborted);
      }

      return query->aborted()? false : true;
    }
    else
    {
      VisusAssert(hstart==hend);

      return LogicSamples::merge(Wsamples,Wbuffer, Rsamples,Rbuffer, InsertSamples,query->aborted);
    }
  }
  else
  {
    InsertBlockQueryHzOrderSamplesToBoxQuery op;
    return NeedToCopySamples(op,query->field.dtype,this,query.get(),block_query.get());
  }
}



/////////////////////////////////////////////////////////////////////////
NetRequest IdxDataset::createBoxQueryRequest(SharedPtr<BoxQuery> query)
{
  /*
    *****NOTE FOR REMOTE QUERIES:*****

    I always restart from scratch so I will do Query0[0,resolutions[0]], Query1[0,resolutions[1]], Query2[0,resolutions[2]]  without any merging
    In this way I transfer a little more data on the network (without compression in the worst case the ratio is 2.0)
    but I can use lossy compression and jump levels
    in the old code I was using:

      Query0[0,resolutions[0]  ]
      Query1[0,resolutions[0]+1] <-- by merging prev_single and Query[resolutions[0]+1,resolutions[0]+1]
      Query1[0,resolutions[0]+2] <-- by merging prev_single and Query[resolutions[0]+2,resolutions[0]+2]
      ...

        -----------------------------
        | overall     |single       |
        | --------------------------|
    Q0  | 2^0*T       |             |
    Q1  | 2^1*T       | 2^0*T       |
    ..  |             |             |
    Qn  | 2^n*T       | 2^(n-1)*T   |
        -----------------------------

    OLD CODE transfers singles = T*(2^0+2^1+...+2^(n-1))=T*2^n    (see http://it.wikipedia.org/wiki/Serie_geometrica)
    NEW CODE transfers overall = T*(2^0+2^1+...+2^(n  ))=T*2^n+1

    RATIO:=overall_transfer/single_transfer=2.0

    With the new code I have the following advantages:

        (*) I don't have to go level by level after Q0. By "jumping" levels I send less data
        (*) I can use lossy compression (in the old code I needed lossless compression to rebuild the data for Qi with i>0)
        (*) not all datasets support the merging (see IdxMultipleDataset and GoogleMapsDataset)
        (*) the filters (example wavelets) are applied always on the server side (in the old code filters were applied on the server only for Q0)
  */


  VisusAssert(query->mode == 'r');

  Url url = this->getUrl();

  NetRequest ret;
  ret.url = url.getProtocol() + "://" + url.getHostname() + ":" + cstring(url.getPort()) + "/mod_visus";
  ret.url.params = url.params;  //I may have some extra params I want to keep!
  ret.url.setParam("action", "boxquery");
  ret.url.setParam("dataset", url.getParam("dataset"));
  ret.url.setParam("time", url.getParam("time", cstring(query->time)));
  ret.url.setParam("compression", url.getParam("compression", "zip")); //for networking I prefer to use zip
  ret.url.setParam("field", query->field.name);
  ret.url.setParam("fromh", cstring(query->start_resolution));
  ret.url.setParam("toh", cstring(query->getEndResolution()));
  ret.url.setParam("maxh", cstring(getMaxResolution())); //backward compatible
  ret.url.setParam("box", query->logic_box.toOldFormatString());
  ret.aborted = query->aborted;
  return ret;
}

/////////////////////////////////////////////////////////////////////////
NetRequest IdxDataset::createPointQueryRequest(SharedPtr<PointQuery> query)
{
  VisusAssert(query->mode == 'r');

  Url url=this->getUrl();

  NetRequest ret;
  ret.url = url.getProtocol() + "://" + url.getHostname() + ":" + cstring(url.getPort()) + "/mod_visus";
  ret.url.params = url.params;  //I may have some extra params I want to keep!
  ret.url.setParam("action", "pointquery");
  ret.url.setParam("dataset", url.getParam("dataset"));
  ret.url.setParam("time", url.getParam("time", cstring(query->time)));
  ret.url.setParam("compression", url.getParam("compression", "zip")); //for networking I prefer to use zip
  ret.url.setParam("field", query->field.name);
  ret.url.setParam("fromh", cstring(0)); //backward compatible
  ret.url.setParam("toh", cstring(query->end_resolution));
  ret.url.setParam("maxh", cstring(getMaxResolution())); //backward compatible
  ret.url.setParam("matrix"  ,query->logic_position.getTransformation().toString());
  ret.url.setParam("box"     ,query->logic_position.getBoxNd().toBox3().toString(/*bInterleave*/false));
  ret.url.setParam("nsamples",query->getNumberOfSamples().toString());
  PrintInfo(ret.url);  
  ret.aborted = query->aborted;
  return ret;
}



//////////////////////////////////////////////////////////////////////////////////////////
bool IdxDataset::setEndResolution(SharedPtr<BoxQuery> query, int value)
{
  auto bitmask = this->idxfile.bitmask;
  HzOrder hzorder(bitmask);

  VisusAssert(query->end_resolution < value);
  query->end_resolution = value;

  auto start_resolution = query->start_resolution;
  auto end_resolution   = query->end_resolution;
  auto logic_box        = query->logic_box.withPointDim(this->getPointDim());

  //special case for query with filters
  //I need to go level by level [0,1,2,...] in order to reconstruct the original data
  if (auto filter = query->filter.dataset_filter)
  {
    //important to return the "final" number of samples (see execute for loop)
    logic_box = this->adjustFilterBox(query.get(), filter.get(), logic_box, end_resolution);
    query->filter.adjusted_logic_box = logic_box;
  }

  //I get twice the samples of the samples!
  PointNi DELTA = hzorder.getLevelDelta(end_resolution);
  if (start_resolution == 0 && end_resolution > 0)
    DELTA[bitmask[end_resolution]] >>= 1;

  PointNi P1incl, P2incl;
  for (int H = start_resolution; H <= end_resolution; H++)
  {
    int bit = bitmask[H];

    LogicSamples Lsamples = this->getLevelSamples(H);

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

  query->logic_samples = LogicSamples(BoxNi(P1incl, P2incl + DELTA), DELTA);
  VisusAssert(query->logic_samples.valid());
  return true;

}


//////////////////////////////////////////////////////////////////////////////////////////
void IdxDataset::beginQuery(SharedPtr<BoxQuery> query) 
{
  if (!query)
    return;

  if (query->getStatus() != QueryCreated)
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
    if (setEndResolution(query, end_resolution))
    {
      query->setRunning();
      return;
    }
  }

  query->setFailed();

}

//////////////////////////////////////////////////////////////////////////////////////////
void IdxDataset::nextQuery(SharedPtr<BoxQuery> query)
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
  VisusReleaseAssert(setEndResolution(query,end_resolution));

  //asssume no merging
  query->buffer = Array();

  //try to merge with previous resolution
  if (query->merge_mode != DoNotMergeSamples && Rsamples.valid() && Rsamples.nsamples == Rbuffer.dims)
  {
    if (!query->allocateBufferIfNeeded())
      return query->setFailed("out of memory");

    if (!LogicSamples::merge(query->logic_samples, query->buffer, Rsamples, Rbuffer, query->merge_mode, query->aborted))
    {
      if (query->aborted())
        return query->setFailed("query aborted");

      VisusAssert(false);
      return query->setFailed("Merging failed for unknown reasons");
    }

    query->filter.query = Rfilter_query;
    query->setCurrentResolution(Rcurrent_resolution);
  }
}


///////////////////////////////////////////////////////////////////////////////////////
bool IdxDataset::executeQuery(SharedPtr<Access> access, SharedPtr<BoxQuery> query)
{
  if (!query)
    return false;

  if (!(query->isRunning() && query->getCurrentResolution() < query->getEndResolution()))
    return false;

  if (query->aborted())
  {
    query->setFailed("query aboted");
    return false;
  }

  //for 'r' queries I can postpone the allocation
  if (query->mode == 'w' && !query->buffer)
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
      BoxNi adjusted_logic_box = adjustFilterBox(query.get(), filter.get(), query->filter.adjusted_logic_box, H);

      auto Wquery = std::make_shared<BoxQuery>(this, query->field, query->time, 'r', query->aborted);
      Wquery->logic_box = adjusted_logic_box;
      Wquery->setResolutionRange(0,H);
      Wquery->disableFilters();
      Wquery->merge_mode = InsertSamples;

      beginQuery(Wquery);

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
        if (!Wquery->mergeWith(*Rquery, query->aborted))
        {
          VisusAssert(query->aborted());
          return false;
        }
      }

      if (!this->executeQuery(access, Wquery))
        return false;

      if (!filter->computeFilter(Wquery.get(), true))
        return false;

      query->filter.query = Wquery;
    }

    //cannot get samples yet... returning failed as a query without filters...
    if (!query->filter.query)
    {
      VisusAssert(false);//technically this should not happen since the outside query has got some samples
      return false;
    }

    query->logic_samples = query->filter.query->logic_samples;
    query->buffer        = query->filter.query->buffer;

    VisusAssert(query->buffer.dims == query->getNumberOfSamples());
    query->setCurrentResolution(query->end_resolution);
    return true;
  }

  //execute with access
  int bitsperblock = access->bitsperblock;
  VisusAssert(bitsperblock);

  FastLoopStack  item, * stack = NULL;
  FastLoopStack  STACK[DatasetBitmaskMaxLen + 1];

  DatasetBitmask bitmask = this->getBitmask();
  HzOrder hzorder(bitmask);

  int max_resolution = getMaxResolution();
  std::vector<Int64> fldeltas(max_resolution + 1);
  for (auto H = 0; H <= max_resolution; H++)
    fldeltas[H] = H ? (hzorder.getLevelDelta(H)[bitmask[H]] >> 1) : 0;

  auto aborted = query->aborted;

  #define PUSH()  (*((stack)++))=(item)
  #define POP()   (item)=(*(--(stack)))
  #define EMPTY() ((stack)==(STACK))

  //collect blocks
  std::vector<BigInt> blocks;
  for (int H = cur_resolution + 1; H <= end_resolution; H++)
  {
    if (aborted())
      return false;

    LogicSamples Lsamples = this->getLevelSamples(H);
    BoxNi box = Lsamples.alignBox(query->logic_samples.logic_box);
    if (!box.isFullDim())
      continue;

    //push first item
    BigInt hz = hzorder.getAddress(Lsamples.logic_box.p1);
    {
      item.box = Lsamples.logic_box;
      item.H = H ? 1 : 0;
      stack = STACK;
      PUSH();
    }

    while (!EMPTY())
    {
      POP();

      // no intersection
      if (!item.box.strictIntersect(box))
      {
        hz += (((BigInt)1) << (H - item.H));
        continue;
      }

      // intersection with hz-block!
      if ((H - item.H) <= bitsperblock)
      {
        BigInt HzFrom = (hz >> bitsperblock) << bitsperblock;
        blocks.push_back(HzFrom);

        // I know that block 0 convers several hz-levels from [0 to bitsperblock]
        if (HzFrom == 0)
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
      item.box.p1[bit] += delta;                            PUSH();
      item.box.p1[bit] -= delta; item.box.p2[bit] -= delta; PUSH();

    } //while (stack!=STACK)

  } //for levels

  #undef PUSH 
  #undef POP  
  #undef EMPTY 

  if (aborted())
    return false;

  int NREAD  = 0;
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

  for (auto HzFrom : blocks)
  {
    if (aborted())
      break;

    //flush previous
    if (async_read.getNumRunning() > 1024)
      waitAsyncRead();

    BigInt HzTo = HzFrom + (((BigInt)1) << bitsperblock);
    auto read_block = std::make_shared<BlockQuery>(this, field, time, HzFrom, HzTo, 'r', aborted);
    NREAD++;

    if (bReading)
    {
      executeBlockQuery(access, read_block);
      async_read.pushRunning(read_block->done).when_ready([this, query, read_block, aborted](Void)
      {
        //I don't care if the read fails...
        if (!aborted() && read_block->ok())
          mergeBoxQueryWithBlock(query, read_block);
      });
    }
    else
    {
      //need a lease... so that I can read/merge/write like in a transaction mode
      access->acquireWriteLock(read_block);

      //need to read and wait the block
      executeBlockQueryAndWait(access, read_block);

      //WRITE block
      auto write_block = std::make_shared<BlockQuery>(this, field, time, HzFrom, HzTo, 'w', aborted);

      //read ok
      if (read_block->ok())
        write_block->buffer = read_block->buffer;
      //I don't care if it fails... maybe does not exist
      else
        write_block->allocateBufferIfNeeded();

      mergeBoxQueryWithBlock(query, write_block);

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


///////////////////////////////////////////////////////////////////////////////////////
void IdxDataset::beginQuery(SharedPtr<PointQuery> query)
{
  if (!query)
    return;

  if (query->getStatus() != QueryCreated)
    return;

  //if you want to set a buffer for 'w' queries, please do it after begin
  VisusAssert(!query->buffer);

  if (getPointDim() != 3)
    return query->setFailed("pointquery supported only in 3d so far");

  if (!query->field.valid())
    return query->setFailed("field not valid");

  if (!query->logic_position.valid())
    return query->setFailed("position not valid");

  // override time from field
  if (query->field.hasParam("time"))
    query->time = cdouble(query->field.getParam("time"));

  if (!getTimesteps().containsTimestep(query->time))
    return query->setFailed("wrong time");

  if (query->end_resolution < 0)
    query->end_resolution = this->getMaxResolution();

  if (query->end_resolution < 0 || query->end_resolution>getMaxResolution())
    return query->setFailed("wrong end_resolution");

  if (query->getNumberOfSamples().innerProduct() <= 0)
    return query->setFailed("wrong nsamples");

  query->setRunning();
}


///////////////////////////////////////////////////////////////////////////////////////
bool IdxDataset::executeQuery(SharedPtr<Access> access,SharedPtr<PointQuery> query)  
{
  if (!query)
    return false;

  if (!query->isRunning())
    return false;

  if (query->aborted())
  {
    query->setFailed("query aboted");
    return false;
  }

  //for 'r' queries I can postpone the allocation
  if (query->mode == 'w' && !query->buffer)
  {
    query->setFailed("write buffer not set");
    return false;
  }

  if (!access)
    return executePointQueryOnServer(query);

  //TODO
  VisusReleaseAssert(query->mode == 'r');

  auto            bitmask = getBitmask();
  BoxNi           bounds = this->getLogicBox();
  BigInt          last_bitmask = ((BigInt)1) << (getMaxResolution());
  auto            hzorder = HzOrder(bitmask);
  PointNi         depth_mask = hzorder.getLevelP2Included(query->end_resolution);
  int             bitsperblock = access->bitsperblock;
  Aborted         aborted = query->aborted;

  if (!query->allocateBufferIfNeeded())
  {
    query->setFailed("out of memory"); 
    return false;
  }

  auto nsamples = query->getNumberOfSamples();
  auto tot = nsamples.innerProduct();

  VisusAssert((Int64)query->points.c_size() == DTypes::INT64_RGB.getByteSize(nsamples));

  //first BigInt is hzaddress, second Int32 is offset inside buffer
  auto hzaddresses = std::vector< std::pair<BigInt, Int32> >(tot, std::make_pair(-1, 0));

  int pdim = this->getPointDim();
  PointNi p(pdim);

  //if this is not available I use the slower conversion p->zaddress->Hz
  if (!this->hzaddress_conversion_pointquery)
  {
    PrintWarning("The hzaddress_conversion_pointquery has not been created, so loc-by-loc queries will be a lot slower!!!!");

    //so you investigate why it's happening! .... I think only for the iphone could make sense....
#if defined(WIN32) && defined(_DEBUG)
    VisusAssert(false);
#endif

    auto SRC = (Int64*)query->points.c_ptr();
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
      hzaddresses[N] = std::make_pair(hzorder.getAddress(p), N);
    }
  }
  //the conversion from point to Hz will be faster
  else
  {
    BigInt zaddress;
    int    shift;
    auto   SRC = (Int64*)query->points.c_ptr();
    auto   loc = this->hzaddress_conversion_pointquery->loc;

    for (int N = 0; N < tot; N++, SRC += pdim)
    {
      if (aborted()) {
        query->setFailed("query aborted"); 
        return false;
      }
      if (pdim >= 1) { p[0] = SRC[0]; if (!(p[0] >= bounds.p1[0] && p[0] < bounds.p2[0])) continue; p[0] &= depth_mask[0]; shift = (loc[0][p[0]].second); zaddress = loc[0][p[0]].first; }
      if (pdim >= 2) { p[1] = SRC[1]; if (!(p[1] >= bounds.p1[1] && p[1] < bounds.p2[1])) continue; p[1] &= depth_mask[1]; shift = std::min(shift, loc[1][p[1]].second); zaddress |= loc[1][p[1]].first; }
      if (pdim >= 3) { p[2] = SRC[2]; if (!(p[2] >= bounds.p1[2] && p[2] < bounds.p2[2])) continue; p[2] &= depth_mask[2]; shift = std::min(shift, loc[2][p[2]].second); zaddress |= loc[2][p[2]].first; }
      if (pdim >= 4) { p[3] = SRC[3]; if (!(p[3] >= bounds.p1[3] && p[3] < bounds.p2[3])) continue; p[3] &= depth_mask[3]; shift = std::min(shift, loc[3][p[3]].second); zaddress |= loc[3][p[3]].first; }
      if (pdim >= 5) { p[4] = SRC[4]; if (!(p[4] >= bounds.p1[4] && p[4] < bounds.p2[4])) continue; p[4] &= depth_mask[4]; shift = std::min(shift, loc[4][p[4]].second); zaddress |= loc[4][p[4]].first; }
      hzaddresses[N] = std::make_pair(((zaddress | last_bitmask) >> shift), N);
    }
  }

  //sort the address 
  std::sort(hzaddresses.begin(), hzaddresses.end());

  //do the for loop block aligned
  WaitAsync< Future<Void> > wait_async;

  bool bWasReading = access->isReading();

  if (!bWasReading)
    access->beginRead();

  for (int A = 0, B = 0; !aborted() && A < (int)hzaddresses.size(); A = B)
  {
    if (hzaddresses[A].first < 0) {
      while (B < (int)hzaddresses.size() && hzaddresses[B].first < 0)
        ++B;
      continue;
    }

    //align to the block
    BigInt HzFrom = (hzaddresses[A].first >> bitsperblock) << bitsperblock;
    BigInt HzTo = HzFrom + (((BigInt)1) << bitsperblock);
    VisusAssert(hzaddresses[A].first >= HzFrom && hzaddresses[A].first < HzTo);

    //end of the block
    while (B < (int)hzaddresses.size() && (hzaddresses[B].first >= HzFrom && hzaddresses[B].first < HzTo))
      ++B;

    auto block_query = std::make_shared<BlockQuery>(this, query->field, query->time, HzFrom, HzTo, 'r', aborted);
    this->executeBlockQuery(access, block_query);
    wait_async.pushRunning(block_query->done).when_ready([this, query, block_query, &hzaddresses, A, B, aborted](Void) {

      if (aborted() || block_query->failed())
        return;

      InsertBlockQuerySamplesIntoPointQuery op;
      NeedToCopySamples(op, query->field.dtype, this, query.get(), block_query.get(), &hzaddresses[0] + A, &hzaddresses[0] + B, aborted);
      });
  }

  if (!bWasReading)
    access->endRead();

  wait_async.waitAllDone();

  if (aborted()) {
    query->setFailed("query aborted");
    return false;
  }

  VisusAssert(query->buffer.dims == query->getNumberOfSamples());
  query->setOk();
  return true;
}



} //namespace Visus
