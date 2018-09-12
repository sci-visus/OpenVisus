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
#include <Visus/VisusConfig.h>
#include <Visus/OnDemandAccess.h>

#ifdef WIN32
#pragma warning(disable:4996) // 'sprintf': This function or variable may be unsafe
#endif

namespace Visus {


//box query type
typedef struct
{
  int    H;
  NdBox  box;
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
    int maxh  = std::max(bitmask.getMaxResolution(),toh);

    LogicBox logic_box=query->logic_box;
    if (!logic_box.valid())
      return readFailed(query);

    auto& buffer=query->buffer;
    buffer.layout="";

    NdBox   box = bitmask.upgradeBox(dataset->getBox(),maxh);
    NdPoint dim = box.size();

    Float32* ptr=(Float32*)query->buffer.c_ptr();
    for (auto loc = ForEachPoint(buffer.dims); !loc.end(); loc.next())
    {
      NdPoint pos=logic_box.pixelToLogic(loc.pos);
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
class IdxBoxQueryHzAddressConversion : public Query::AddressConversion
{
public:

  //_______________________________________________________________
  class Level : public Object
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

      if (!cached_points->resize(this->num * sizeof(NdPoint), __FILE__, __LINE__))
      {
        this->clear();
        return;
      }

      cached_points->fill(0);

      NdPoint* ptr = (NdPoint*)cached_points->c_ptr();

      HzOrder hzorder(bitmask, H);

      //create the delta for points  
      for (BigInt zaddress = 0; zaddress < (this->num - 1); zaddress++, ptr++)
      {
        NdPoint Pcur = hzorder.deinterleave(zaddress + 0);
        NdPoint Pnex = hzorder.deinterleave(zaddress + 1);

        (*ptr) = NdPoint(pdim);

        //store the delta
        for (int D = 0; D < pdim; D++)
          (*ptr)[D] = Pnex[D] - Pcur[D];
      }

      //i want the last (FAKE and unused) element to be zero
      (*ptr) = NdPoint(pdim);
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
      return sizeof(NdPoint)*(this->num);
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
      addLevel();
  }


  //createCloneConversion
  IdxBoxQueryHzAddressConversion(const IdxBoxQueryHzAddressConversion& other,int maxh) 
  {
    this->bitmask = other.bitmask;
    this->levels = other.levels;

    while (maxh>=this->levels.size())
      addLevel();
  }

  //addLevel
  void addLevel() {
    this->levels.push_back(std::make_shared<Level>(bitmask, (int)this->levels.size()));
  }


};

////////////////////////////////////////////////////////
class IdxPointQueryHzAddressConversion : public Query::AddressConversion
{
public:

  Int64 memsize=0;

  std::vector< std::pair<BigInt,Int32>* > loc;

  //create
  IdxPointQueryHzAddressConversion(IdxDataset* vf)
  {
    //todo cases for which I'm using regexp
    int    MaxH         = vf->getMaxResolution();
    BigInt last_bitmask = ((BigInt)1)<<MaxH;

    DatasetBitmask bitmask=vf->getBitmask();
    HzOrder hzorder(bitmask,MaxH);
    int pdim = bitmask.getPointDim();
    loc.resize(pdim);
  
    for (int D=0;D<pdim;D++)
    {
      Int64 dim= bitmask.getPow2Dims()[D]; 

      this->loc[D]=new std::pair<BigInt,Int32>[dim];

      BigInt one=1;
      for (int i=0;i<dim;i++)
      {
        NdPoint p(pdim);
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
class InsertBlockQueryHzOrderSamples 
{
public:

  template <class Sample>
  bool execute(IdxDataset*  vf,Query* query,BlockQuery* block_query)
  {
    #define PUSH()  (*((stack)++))=(item)
    #define POP()   (item)=(*(--(stack)))
    #define EMPTY() ((stack)==(STACK))

    VisusAssert(query->field.dtype==block_query->buffer.dtype);
    VisusAssert(block_query->buffer.layout=="hzorder");

    bool bInvertOrder=query->mode=='w';

    DatasetBitmask bitmask=vf->getBitmask();
    int            max_resolution=query->max_resolution;
    BigInt         HzFrom=block_query->start_address;
    BigInt         HzTo  =block_query->end_address;
    HzOrder        hzorder(bitmask,max_resolution);
    int            hstart=std::max(query->cur_resolution+1 ,HzOrder::getAddressResolution(bitmask,HzFrom));
    int            hend  =std::min(query->getEndResolution(),HzOrder::getAddressResolution(bitmask,HzTo-1));
    int            samplesperblock=(int)cint64(HzTo-HzFrom);
    int            bitsperblock= Utils::getLog2(samplesperblock);

    VisusAssert(HzFrom==0 || hstart==hend);

    auto Wbox=GetSamples<Sample>(      query->buffer);
    auto Rbox=GetSamples<Sample>(block_query->buffer);

    if (bInvertOrder)
      std::swap(Wbox,Rbox);

    if (!query->address_conversion)
    {
      ScopedLock lock(query->address_conversion_lock);
      query->address_conversion = std::make_shared<IdxBoxQueryHzAddressConversion>(*vf->hzaddress_conversion_boxquery,max_resolution);
    }

    int              numused=0;
    int              bit;
    NdPoint::coord_t delta;
    NdBox            query_box        = query->logic_box;
    NdPoint          stride           = query->nsamples.stride();
    NdPoint          qshift           = query->logic_box.shift;
    BigInt           numpoints;
    Aborted          aborted=query->aborted;

    FastLoopStack item,*stack=NULL;
    FastLoopStack STACK[DatasetBitmaskMaxLen+1];

    //layout of the block
    LogicBox Bbox=vf->getAddressRangeBox(block_query->start_address,block_query->end_address,max_resolution);
    if (!Bbox.valid())
      return false;

    //deltas
    std::vector<NdPoint::coord_t> fldeltas(max_resolution+1);
    for (int H = 0; H <= max_resolution; H++)
      fldeltas[H] = H? (hzorder.getLevelDelta(H)[bitmask[H]] >> 1) : 0;

    for (int H=hstart;H<=hend;H++)
    {
      if (aborted())
        return false;

      LogicBox Lbox=vf->getLevelBox(hzorder,H);
      NdPoint  lshift=Lbox.shift;

      NdBox   zbox = (HzFrom!=0)? Bbox : Lbox;
      BigInt  hz   = hzorder.getAddress(zbox.p1);

      NdBox user_box= query_box.getIntersection(zbox);
      NdBox box=Lbox.alignBox(user_box);
      if (!box.isFullDim())
        continue;
     
      auto conv = std::dynamic_pointer_cast<IdxBoxQueryHzAddressConversion>(query->address_conversion);
      VisusAssert(conv->levels[H]);
      const auto& fllevel = *(conv->levels[H]);
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
          NdPoint* cc     = (NdPoint*)fllevel.cached_points->c_ptr();
          const NdPoint  query_p1=query_box.p1;
          NdPoint  P=item.box.p1;

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
  bool execute(IdxDataset* vf,Query* query,BlockQuery* block_query,std::pair<BigInt,Int32>* A,std::pair<BigInt,Int32>* B,Aborted aborted)
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
    DatasetBitmask bitmask       = vf->getBitmask();
    int            pdim          = vf->getPointDim();
    HzOrder        hzorder         (bitmask,query->max_resolution);
    NdPoint        depth_mask    = hzorder.getLevelP2Included(query->getEndResolution());

    LogicBox Bbox=vf->getAddressRangeBox(block_query->start_address,block_query->end_address,query->max_resolution);
    if (!Bbox.valid())
      return false;

    NdPoint stride = Rbuffer.dims.stride();
    NdPoint p0     = Bbox.p1;
    NdPoint shift  = Bbox.shift;

    const auto points = (NdPoint::coord_t*)query->point_coordinates->c_ptr();

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
LogicBox IdxDataset::getLevelBox(HzOrder& hzorder, int H)
{
  NdPoint delta = hzorder.getLevelDelta(H);
  NdBox box(hzorder.getLevelP1(H),hzorder.getLevelP2Included(H) + delta);
  auto ret=LogicBox(box, delta);
  VisusAssert(ret.valid());
  return ret;
}


///////////////////////////////////////////////////////////
void IdxDataset::tryRemoveLockAndCorruptedBinaryFiles(String directory)
{
  if (directory.empty())
  {
    directory=VisusConfig::readString("Configuration/IdxDataset/RemoveLockFiles/directory");
    if (directory.empty())
      return;
  }

  VisusInfo()<<"Trying to remove locks and corrupted binary files in directory "<<directory<<"...";

  std::vector<String> lock_files;
  DirectoryIterator::findAllFilesEndingWith(lock_files,directory,".lock");

  for (int I=0;I<(int)lock_files.size();I++)
  {
    String lock_filename=lock_files[I];
    {
      bool bOk=FileUtils::removeFile(lock_filename);
      VisusInfo()<<"Removing lock_filename("<<lock_filename<<") "<<(bOk?"ok":"ERROR");
    }

    String bin_filename =lock_files[I].substr(0,lock_filename.length()-5);
    if (FileUtils::existsFile(bin_filename)) 
    {
      bool bOk=FileUtils::removeFile(bin_filename);
      VisusInfo() <<"Removing bin_filename("+bin_filename+") "<<(bOk?"ok":"ERROR");
    }
  }
}

////////////////////////////////////////////////////////////////////////
void IdxDataset::removeFiles(int maxh)
{
  if (maxh<0) 
    maxh=idxfile.bitmask.getMaxResolution();

  HzOrder hzorder(idxfile.bitmask,maxh);
  BigInt tot_blocks= hzorder.getAddress(hzorder.getLevelP2Included(maxh))>>idxfile.bitsperblock;
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
bool IdxDataset::compress(String compression) 
{
  // for future version: here I'm making the assumption that a file contains multiple fields
  if (idxfile.version != 6) {
    VisusAssert(false); 
    return false;
  }
  
  //save the new idx file
  {
    for (auto& field : idxfile.fields)
      field.default_compression=compression;

    String filename=this->getUrl().getPath();
    if (!idxfile.save(filename)){
      VisusError()<<"Cannot save the new idxfile "<<filename;
      VisusAssert(false);
      return false;
    }
  }

  Time T1=Time::now();
  Time t1=T1;

  auto access = std::make_shared<IdxDiskAccess>(this);

  Aborted aborted; 

  //for each time
  auto timesteps=idxfile.timesteps.asVector();
  for (auto time : timesteps)
  {
    //for each file...
    BigInt tot_files = getTotalnumberOfBlocks() / idxfile.blocksperfile;
    for (BigInt fileid = 0; fileid < tot_files; fileid++)
    {
      std::vector< std::vector<Array> > file_blocks;
      file_blocks.resize(idxfile.fields.size(), std::vector<Array>(idxfile.blocksperfile));

      std::set<String> filenames;

      //read file blocks
      access->beginRead();
      for (int F = 0; F < idxfile.fields.size(); F++)
      {
        auto field = idxfile.fields[F];

        auto blockid = fileid*idxfile.blocksperfile;
        for (BigInt B = 0; B < idxfile.blocksperfile; B++, blockid++)
        {
          auto filename=access->getFilename(field,time,blockid);
          if (!FileUtils::existsFile(filename))
            continue;

          auto read_block = std::make_shared<BlockQuery>(field, time, access->getStartAddress(blockid), access->getEndAddress(blockid), aborted);
          if (readBlockAndWait(access, read_block))
          {
            file_blocks[F][B] = read_block->buffer;
            filenames.insert(filename);
          }
        }
      }
      access->endRead();

      //rename old files (in case the conversion fails)
      for (auto filename : filenames) 
      {
        String tmp_filename=filename+".tmp~";
         if(std::rename(filename.c_str(), tmp_filename.c_str())!=0) 
         { 
           String error_msg=StringUtils::format()<<"Cannot std::rename("+filename <<","<<tmp_filename<<")";
           std::perror(error_msg.c_str()); 
           VisusAssert(false);
           return false; 
         }
      }

      //write file blocks
      access->beginWrite();
      for (int F = 0; F < idxfile.fields.size(); F++)
      {
        auto field = idxfile.fields[F];

        auto blockid = fileid*idxfile.blocksperfile;
        for (BigInt B = 0; B < idxfile.blocksperfile; B++, blockid++)
        {
          if (auto buffer = file_blocks[F][B])
          {
            auto write_block = std::make_shared<BlockQuery>(field, time, access->getStartAddress(blockid), access->getEndAddress(blockid), aborted);
            write_block->buffer = buffer;

            if (!writeBlockAndWait(access, write_block))
            {
              VisusError()<<"Fatal error writing field(" << F << ") block(" << blockid << ")";
              VisusAssert(false);
              access->endIO();
              return false;
            }
          }
        }
      }
      access->endWrite();

      //safe to delete the old file
      for (auto filename : filenames) 
      {
        String old_filename=filename+".tmp~";
        Int64 old_size=FileUtils::getFileSize(old_filename);
        Int64 new_size=FileUtils::getFileSize(    filename);
        double ratio=100.0*(new_size/(double)old_size);

        if(std::remove(old_filename.c_str())!=0)  
        {
          String error_msg=StringUtils::format()<<"Cannot std::remove("+old_filename<<"). ";
          std::perror(error_msg.c_str()); 
          VisusAssert(false);
          return false;
        }

        VisusInfo()<<"Done "<<filename<<" time("<<time<<"/"<<timesteps.size()<<" fileid("<<fileid<<"/"<<tot_files<<") ratio("<<ratio<<")";
      }
    }
  }

  VisusInfo()<<"Dataset compressed in "<<T1.elapsedSec()<<"sec";
  return true;
}


///////////////////////////////////////////////////////////////////////////////////
NdBox IdxDataset::adjustFilterBox(Query* query,DatasetFilter* filter,NdBox user_box,int H) 
{
  int MaxH=query->max_resolution;

  //there are some case when I need alignment with pow2 box, for example when doing kdquery=box with filters
  HzOrder hzorder(bitmask,MaxH);
  int pdim = bitmask.getPointDim();

  NdPoint delta=hzorder.getLevelDelta(H);

  NdBox domain = bitmask.upgradeBox(query->filter.domain,MaxH);

  //important! for the filter alignment
  NdBox box= user_box.getIntersection(domain);

  if (!box.isFullDim())
    return box;

  NdPoint filterstep=filter->getFilterStep(H,MaxH);

  for (int D=0;D<pdim;D++) 
  {
    //what is the world step of the filter at the current resolution
    NdPoint::coord_t FILTERSTEP=filterstep[D];

    //means only one sample so no alignment
    if (FILTERSTEP==1) 
      continue;

    box.p1[D]=Utils::alignLeft(box.p1[D]  ,(NdPoint::coord_t)0,FILTERSTEP);
    box.p2[D]=Utils::alignLeft(box.p2[D]-1,(NdPoint::coord_t)0,FILTERSTEP)+FILTERSTEP; 
  }

  //since I've modified the box I need to do the intersection with the box again
  //important: this intersection can cause a misalignment, but applyToQuery will handle it (see comments)
  box= box.getIntersection(domain);
  return box;
}



//////////////////////////////////////////////////////////////////
LogicBox IdxDataset::getAddressRangeBox(BigInt HzFrom,BigInt HzTo,int max_resolution)
{
  const DatasetBitmask& bitmask=this->idxfile.bitmask;
  int pdim = bitmask.getPointDim();

  if (max_resolution<0)
  {
    int fromh = HzOrder::getAddressResolution(bitmask,HzFrom);
    int toh   = HzOrder::getAddressResolution(bitmask,HzTo-1);
    max_resolution = std::max(bitmask.getMaxResolution(),toh);
  }

  HzOrder hzorder(bitmask,max_resolution);

  int start_resolution=HzOrder::getAddressResolution(bitmask,HzFrom);
  int end_resolution  =HzOrder::getAddressResolution(bitmask,HzTo-1);
  VisusAssert((HzFrom>0 && start_resolution==end_resolution) || (HzFrom==0 && start_resolution==0));

  NdPoint delta(pdim);
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

  NdBox box(hzorder.getPoint(HzFrom),hzorder.getPoint(HzTo-1)+delta);

  auto ret=LogicBox(box,delta);
  VisusAssert(ret.nsamples==HzOrder::getAddressRangeNumberOfSamples(bitmask,HzFrom,HzTo));
  VisusAssert(ret.valid());
  return ret;
}


////////////////////////////////////////////////////////////////////////
SharedPtr<Query> IdxDataset::createEquivalentQuery(int mode,SharedPtr<BlockQuery> block_query)
{
  int fromh = HzOrder::getAddressResolution(bitmask,block_query->start_address);
  int toh   = HzOrder::getAddressResolution(bitmask,block_query->end_address-1);
  int maxh  = std::max(bitmask.getMaxResolution(),toh);

  auto logic_box=block_query->logic_box;
  VisusAssert(logic_box.nsamples.innerProduct()==(block_query->end_address-block_query->start_address));
  VisusAssert(fromh==toh || block_query->start_address==0);

  auto ret=std::make_shared<Query>(this,mode);
  ret->aborted=block_query->aborted;
  ret->time=block_query->time;
  ret->field=block_query->field;
  ret->position=logic_box;
  ret->start_resolution=fromh;
  ret->end_resolutions={toh};
  ret->max_resolution=maxh;
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

  auto query=createEquivalentQuery('r',block_query);
  if (!this->beginQuery(query)) 
  {
    VisusAssert(false);
    return false;
  }

  //as the query has not already been executed!
  query->cur_resolution=query->start_resolution-1;
  query->buffer=row_major; 
  
  if (!mergeQueryWithBlock(query,block_query))
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
  VisusAssert(this->valid());

  if (config.empty())
    config = getDefaultAccessConfig();

  String type =StringUtils::toLower(config.readString("type"));

  //no type, create default
  if (type.empty()) 
  {
    Url url = config.readString("url", this->url.toString());

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
bool IdxDataset::openFromUrl(Url url) 
{
  auto idxfile = IdxFile::openFromUrl(url);
  if (!idxfile.valid()) {
    this->invalidate();
    return false;
  }

  this->url = url;
  this->dataset_body = idxfile.toString();
  setIdxFile(idxfile);
  return true;
}


////////////////////////////////////////////////////////////
SharedPtr<IdxDataset> IdxDataset::cloneForMosaic() const
{
  SharedPtr<IdxDataset> ret = std::make_shared<IdxDataset>();

  ret->BaseDataset::operator=(*this);

  ret->idxfile = this->idxfile;
  ret->hzaddress_conversion_boxquery = this->hzaddress_conversion_boxquery;
  ret->hzaddress_conversion_pointquery = this->hzaddress_conversion_pointquery;

  return ret;
}


////////////////////////////////////////////////////////////
void IdxDataset::setIdxFile(IdxFile value)
{
  VisusAssert(value.valid());
  this->idxfile=value;

  this->bitmask= value.bitmask;
  this->default_bitsperblock= value.bitsperblock;
  this->box= value.box;
  this->timesteps= value.timesteps;
  
  this->default_scene  = value.scene;
    
  if (StringUtils::startsWith(this->default_scene, "."))
  {
    auto dir = this->url.getPath().substr(0, this->url.getPath().find_last_of("/"));
    this->default_scene = dir  + "/" + value.scene;
  }

  for (auto field : value.fields)
    addField(field);

  this->hzaddress_conversion_boxquery=std::make_shared<IdxBoxQueryHzAddressConversion>(this->bitmask);

  //create the loc-cache only for 3d data, in 2d I know I'm not going to use it!
  //instead in 3d I will use it a lot (consider a slice in odd position)
  if (value.bitmask.getPointDim()>=3 && !this->hzaddress_conversion_pointquery)
    this->hzaddress_conversion_pointquery=std::make_shared<IdxPointQueryHzAddressConversion>(this);
}


//////////////////////////////////////////////////////////////////////////////////////////
bool IdxDataset::mergeQueryWithBlock(SharedPtr<Query> query,SharedPtr<BlockQuery> block_query)
{
  if (query->isPointQuery())
  {
    VisusAssert(false);
    return false;
  }

  if (!query->allocateBufferIfNeeded())
    return false;

  if (bool bRowMajor=block_query->buffer.layout.empty())
  {
    VisusAssert(query->field.dtype==block_query->field.dtype);

    DatasetBitmask bitmask=this->getBitmask();
    int            max_resolution=query->max_resolution;
    BigInt         HzFrom=block_query->start_address;
    BigInt         HzTo  =block_query->end_address;
    int            hstart=std::max(query->cur_resolution+1  ,HzOrder::getAddressResolution(bitmask,HzFrom));
    int            hend  =std::min(query->getEndResolution(),HzOrder::getAddressResolution(bitmask,HzTo-1));

    //layout of the block
#if 1
    auto Bbox=this->getAddressRangeBox(block_query->start_address,block_query->end_address,max_resolution);
#else
    auto Bbox=block_query->samples;
#endif

    if (!Bbox.valid())
      return false;

    auto Wbox=query->mode=='w'?        Bbox : query->logic_box;
    auto Rbox=query->mode=='w'? query->logic_box :        Bbox;

    auto& Wbuffer =query->mode=='w'?  block_query->buffer :       query->buffer;
    auto& Rbuffer =query->mode=='w'?        query->buffer : block_query->buffer;

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

      HzOrder hzorder(bitmask,max_resolution);

      for (int H=hstart; !query->aborted() && H<=hend ; ++H)
      {
        auto Lbox    = getLevelBox(hzorder,H);
        auto Lbuffer = Array(Lbox.nsamples,block_query->field.dtype);

        /*
        NOTE the pipeline is:
             Wbox <- Lbox <- Rbox 

         but since it can be that Rbox writes only a subset of Lbox 
         i.e.  I allocate Lbox buffer and one of its samples at position P is not written by Rbox
               that sample P will overwrite some Wbox P' 

         For this reason I do at the beginning:

          Lbox <- Wbox

        int this way I'm sure that all Wbox P' are left unmodified

        Note also that merge can fail simply because there are no samples to merge at a certain level
        */

        Query::mergeSamples(Lbox,Lbuffer,Wbox,Wbuffer,Query::InsertSamples,query->aborted);
        Query::mergeSamples(Lbox,Lbuffer,Rbox,Rbuffer,Query::InsertSamples,query->aborted);
        Query::mergeSamples(Wbox,Wbuffer,Lbox,Lbuffer,Query::InsertSamples,query->aborted);
      }

      return query->aborted()? false : true;
    }
    else
    {
      VisusAssert(hstart==hend);

      return Query::mergeSamples(Wbox,Wbuffer,Rbox,Rbuffer,Query::InsertSamples,query->aborted);
    }
  }
  else
  {
    InsertBlockQueryHzOrderSamples op;
    return NeedToCopySamples(op,query->field.dtype,this,query.get(),block_query.get());
  }
}

/////////////////////////////////////////////////////////////////////////
NetRequest IdxDataset::createPureRemoteQueryNetRequest(SharedPtr<Query> query)
{
  //todo writing
  VisusAssert(query->mode=='r');

  /* 
    *****NOTE FOR REMOTE QUERIES:*****

    I always restart from scratch so I will do Query0[0,end_resolutions[0]], Query1[0,end_resolutions[1]], Query2[0,end_resolutions[2]]  without any merging
    In this way I transfer a little more data on the network (without compression in the worst case the ratio is 2.0)  
    but I can use lossy compression and jump levels 
    in the old code I was using:

      Query0[0,end_resolutions[0]  ]
      Query1[0,end_resolutions[0]+1] <-- by merging prev_single and Query[end_resolutions[0]+1,end_resolutions[0]+1]
      Query1[0,end_resolutions[0]+2] <-- by merging prev_single and Query[end_resolutions[0]+2,end_resolutions[0]+2]
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
        (*) not all datasets support the merging (see IdxMultipleDataset and LegacyDataset)
        (*) the filters (example wavelets) are applied always on the server side (in the old code filters were applied on the server only for Q0)
  */


  //important for IdxMultipleDataset... the url can be different
  Url url=this->getUrl();

  NetRequest ret;
  ret.url=url.getProtocol() + "://" + url.getHostname() + ":" + cstring(url.getPort()) + "/mod_visus";
  ret.url.params=url.params;  //I may have some extra params I want to keep!
  ret.url.setParam("dataset"    ,url.getParam("dataset"));
  ret.url.setParam("time"       ,url.getParam("time",cstring(query->time)));
  ret.url.setParam("compression",url.getParam("compression",query->field.default_compression));
  ret.url.setParam("field"      ,query->field.name);
  ret.url.setParam("fromh"      ,cstring(query->start_resolution));
  ret.url.setParam("toh"        ,cstring(query->getEndResolution()));
  ret.url.setParam("maxh"       ,cstring(query->max_resolution));

  if (query->isPointQuery())
  {
    ret.url.setParam("action"  ,"pointquery");
    ret.url.setParam("matrix"  ,query->position.getTransformation().toString());
    ret.url.setParam("box"     ,query->position.getBox().toString());
    ret.url.setParam("nsamples",query->nsamples.toString());
  }
  else
  {
    VisusAssert(query->position.getTransformation().isIdentity()); //todo
    ret.url.setParam("action","boxquery");
    ret.url.setParam("box"   , query->position.getNdBox().toOldFormatString());
  }

  ret.aborted=query->aborted;
  return ret;
}

//*********************************************************************
// valerio's algorithm, find the final view dependent resolution (endh)
// (the default endh is the maximum resolution available)
//*********************************************************************

NdPoint IdxDataset::guessPointQueryNumberOfSamples(Position position,const Frustum& viewdep,int end_resolution)
{
  const int unit_box_edges[12][2]=
  {
    {0,1}, {1,2}, {2,3}, {3,0},
    {4,5}, {5,6}, {6,7}, {7,4},
    {0,4}, {1,5}, {2,6}, {3,7}
  };

  std::vector<Point3d> points;
  for (int I=0;I<8;I++)
  {
    Point3d p=position.getTransformation() * position.getBox().getPoint(I);
    points.push_back(p);
  }

  std::vector<Point2d> screen_points;
  if (viewdep.valid())
  {
    FrustumMap map(viewdep);
    for (int I=0;I<8;I++)
      screen_points.push_back(map.projectPoint(points[I]));
  }

  int pdim = bitmask.getPointDim();

  NdPoint virtual_worlddim=NdPoint::one(pdim);
  for (int H=1;H<=end_resolution;H++)
  {
    int bit=bitmask[H];
    virtual_worlddim[bit]<<=1;
  }

  NdPoint nsamples=NdPoint::one(pdim);
  for (int E=0;E<12;E++)
  {
    int query_axis=(E>=8)?2:(E&1?1:0);
    Point3d P1=points[unit_box_edges[E][0]];
    Point3d P2=points[unit_box_edges[E][1]];
    Point3d edge_size=(P2-P1).abs();

    NdPoint idx_size   = this->getBox().size();

    // need to project onto IJK  axis
    // I'm using this formula: x/virtual_worlddim[dataset_axis] = factor = edge_size[dataset_axis]/idx_size[dataset_axis]
    for (int dataset_axis=0;dataset_axis<3;dataset_axis++)
    {
      double factor=(double)edge_size[dataset_axis]/(double)idx_size[dataset_axis];
      NdPoint::coord_t x=(NdPoint::coord_t)(virtual_worlddim[dataset_axis]*factor);
      nsamples[query_axis]=std::max(nsamples[query_axis],x);
    }
  }
    
  //view dependent, limit the nsamples to what the user can see on the screen!
  if (!screen_points.empty())
  {
    NdPoint view_dependent_dims = NdPoint::one(pdim);
    for (int E=0;E<12;E++)
    {
      int query_axis=(E>=8)?2:(E&1?1:0);
      Point2d p1=screen_points[unit_box_edges[E][0]];
      Point2d p2=screen_points[unit_box_edges[E][1]];
      double pixel_distance_on_screen=(p2-p1).module();
      view_dependent_dims[query_axis]=std::max(view_dependent_dims[query_axis],(NdPoint::coord_t)pixel_distance_on_screen);
    }

    nsamples[0]=std::min(view_dependent_dims[0],nsamples[0]);
    nsamples[1]=std::min(view_dependent_dims[1],nsamples[1]);
    nsamples[2]=std::min(view_dependent_dims[2],nsamples[2]);
  }

  return nsamples;
}

/////////////////////////////////////////////////////////////////////////
bool IdxDataset::setPointQueryCurrentEndResolution(SharedPtr<Query> query)
{
  int end_resolution = query->getEndResolution();
  if (end_resolution<0)
    return false;

  Position position=query->position;

  const Matrix& T=position.getTransformation();
  Box3d box        =position.getBox();

  int pdim=this->getPointDim();
  VisusAssert(pdim==3); //why I need point queries in 2d... I'm asserting this because I do not create Query for 2d datasets 

  if (!(query->start_resolution==0 && end_resolution<=query->max_resolution && this->getMaxResolution()==query->max_resolution))
  {
    VisusAssert(false);
    return false;
  }

  auto nsamples=guessPointQueryNumberOfSamples(position,query->viewdep,end_resolution);

  //no samples or overflow
  if (nsamples.innerProduct()<=0)
    return false;

  //only 1-2-3 dim
  Int64 tot = nsamples.innerProduct();
  if (tot !=(nsamples[0]*nsamples[1]*nsamples[2]))
    return false;

  if (!query->point_coordinates->resize(tot*pdim*sizeof(NdPoint::coord_t),__FILE__,__LINE__))
    return false;

  //definition of a point query!
  //P'=T* (P0 + I* X/nsamples.x +  J * Y/nsamples.y + K * Z/nsamples.z)
  //P'=T*P0 +(T*Stepx)*I + (T*Stepy)*J + (T*Stepz)*K
    
  Point4d P0(box.p1.x,box.p1.y,box.p1.z,1.0);
  Point4d X(1,0,0,0); X[0]=box.p2[0]-box.p1.x; Point4d DX=X*(1.0 / (double)nsamples[0]); VisusAssert(X.w==0.0 && DX.w==0.0);
  Point4d Y(0,1,0,0); Y[1]=box.p2[1]-box.p1.y; Point4d DY=Y*(1.0 / (double)nsamples[1]); VisusAssert(Y.w==0.0 && DY.w==0.0);
  Point4d Z(0,0,1,0); Z[2]=box.p2[2]-box.p1.z; Point4d DZ=Z*(1.0 / (double)nsamples[2]); VisusAssert(Z.w==0.0 && DZ.w==0.0);

  Point4d TP0_4d = T*P0;                             Point3d TP0  = TP0_4d.dropHomogeneousCoordinate();
  Point4d TDX_4d = T*DX; VisusAssert(TDX_4d.w==0.0); Point3d TDX  = TDX_4d.dropW();
  Point4d TDY_4d = T*DY; VisusAssert(TDY_4d.w==0.0); Point3d TDY  = TDY_4d.dropW();
  Point4d TDZ_4d = T*DZ; VisusAssert(TDZ_4d.w==0.0); Point3d TDZ  = TDZ_4d.dropW();

  auto point_p = (NdPoint::coord_t*)query->point_coordinates->c_ptr();
  Point3d PZ=TP0; for (int K=0;K<nsamples[2];++K,PZ+=TDZ) {
  Point3d PY =PZ; for (int J=0;J<nsamples[1];++J,PY+=TDY) {
  Point3d PX =PY; for (int I=0;I<nsamples[0];++I,PX+=TDX) {
    *point_p++=(NdPoint::coord_t)(PX.x);
    *point_p++=(NdPoint::coord_t)(PX.y);
    *point_p++=(NdPoint::coord_t)(PX.z);
  }}}

  //note: point queries are not mergeable, so it's box is invalid!
  query->nsamples=nsamples;
  query->logic_box=LogicBox();
  query->buffer=Array();
  return true;
}


//////////////////////////////////////////////////////////////////////////////////////////
bool IdxDataset::setBoxQueryCurrentEndResolution(SharedPtr<Query> query)
{
  int end_resolution = query->getEndResolution();
  if (end_resolution<0)
    return false;

  VisusAssert(query->position.getTransformation().isIdentity());
  query->aligned_box=query->position.getNdBox();

  if (!query->aligned_box.isFullDim())
    return false;

  int start_resolution = query->start_resolution;
  int max_resolution   = query->max_resolution;

  if (!(0<=start_resolution && start_resolution<=end_resolution && end_resolution<=max_resolution && this->getMaxResolution()<=max_resolution))
  {
    //AAG: why force a crash here? VisusAssert(false);
    return false;
  }

  if (start_resolution>0 && start_resolution!=end_resolution)
  {
    VisusAssert(false);
    return false;
  }

  //special case for query with filters
  //I need to go level by level [0,1,2,...] in order to reconstruct the original data
  if (auto filter=query->filter.value)
  {
    //important to return the "final" number of samples (see execute for loop)
    query->aligned_box=this->adjustFilterBox(query.get(),filter.get(), query->aligned_box,end_resolution);
  }

  //coompute the samples I will get
  DatasetBitmask bitmask=this->idxfile.bitmask;
  HzOrder hzorder(bitmask,max_resolution);

  int pdim = bitmask.getPointDim();

  //I get twice the samples of the samples!
  NdPoint DELTA=hzorder.getLevelDelta(end_resolution);
  if (start_resolution==0 && end_resolution>0)
    DELTA[bitmask[end_resolution]]>>=1;

  bool bGotSamples=false;
  NdPoint P1incl(pdim),P2incl(pdim);
  for (int H=start_resolution;H<=end_resolution;H++)
  {
    int bit=bitmask[H];

    LogicBox Lbox=this->getLevelBox(hzorder,H);

    NdBox box=Lbox.alignBox(query->aligned_box);
    if (!box.isFullDim())
      continue;

    NdPoint p1incl=box.p1;
    NdPoint p2incl=box.p2-Lbox.delta;
    P1incl=bGotSamples? NdPoint::min(P1incl,p1incl) : p1incl;
    P2incl=bGotSamples? NdPoint::max(P2incl,p2incl) : p2incl;
    bGotSamples=true;
  } 

  //probably overflow
  if (!bGotSamples)
    return false;

  NdBox BOX(P1incl,P2incl+DELTA);

  auto logic_box=LogicBox(BOX,DELTA); 
  VisusAssert(logic_box.valid());

  query->nsamples=logic_box.nsamples;
  query->logic_box=logic_box;
  query->buffer=Array();
  return true;
}


//////////////////////////////////////////////////////////////////////////////////////////
bool IdxDataset::beginQuery(SharedPtr<Query> query)
{
  if (!Dataset::beginQuery(query))
    return false;

  bool bPointQuery = query->position.getPointDim() < this->getPointDim();
  query->point_coordinates = bPointQuery ? std::make_shared<HeapMemory>() : SharedPtr<HeapMemory>();

  if (!bPointQuery)
  {
    Matrix T = query->position.getTransformation();
    if (!T.isIdentity())
    {
      //clipping...
      if (this->getPointDim() == 3)
        query->clipping = query->position;

      query->position = query->position.withoutTransformation().getNdBox().getIntersection(this->getBox());
    }

    if (query->filter.enabled)
    {
      query->filter.value = this->createQueryFilter(query->field);
      if (!query->filter.value)
        query->filter.enabled = false;
    }
  }

  if (!query->position.valid())
  {
    query->setFailed("position is wrong");
    return false;
  }

  query->setRunning();

  std::vector<int> end_resolutions=query->end_resolutions;
  for (query->query_cursor=0;query->query_cursor<(int)end_resolutions.size();query->query_cursor++)
  {
    if (this->setCurrentEndResolution(query))
      return true;
  }

  query->setFailed("cannot find an initial resolution");
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////
bool IdxDataset::executeQuery(SharedPtr<Access> access,SharedPtr<Query> query)  
{
  if (!Dataset::executeQuery(access,query))
    return false;

  if (access)
  {
    return query->isPointQuery() ?
      executePointQueryWithAccess(access,query) : 
      executeBoxQueryWithAccess(access,query);
  }
  //pure remote query 
  else
  {
    return Dataset::executePureRemoteQuery(query);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool IdxDataset::executePointQueryWithAccess(SharedPtr<Access> access,SharedPtr<Query> query)
{
  VisusAssert(query->mode=='r');
  VisusAssert(access);

  int end_resolution=query->getEndResolution();

  int             pdim               = this->getPointDim();
  int             maxh               = query->max_resolution;
  NdBox           bounds             = bitmask.upgradeBox(this->getBox(),maxh);
  BigInt          last_bitmask       = ((BigInt)1)<<(maxh);
  HzOrder         hzorder            (bitmask,maxh);
  NdPoint         depth_mask         = hzorder.getLevelP2Included(end_resolution);
  int             bitsperblock       = access->bitsperblock;
  Aborted         aborted            = query->aborted;

  if (!query->allocateBufferIfNeeded())
    return false;

  Int64 tot=query->nsamples.innerProduct();

  const auto points = (NdPoint::coord_t*)query->point_coordinates->c_ptr();
  if (!query->point_coordinates || !query->point_coordinates->c_size() || !tot)
    return false;

  VisusAssert(access);
  VisusAssert(pdim<=3);//todo: other cases
  VisusAssert(query->start_resolution==0);//todo: othercases
  VisusAssert(maxh==this->getMaxResolution());//todo other cases!
  VisusAssert((Int64)query->point_coordinates->c_size()>=query->nsamples.innerProduct()*(Int64)sizeof(NdPoint::coord_t)*pdim);

  //first BigInt is hzaddress, second Int32 is offset inside buffer
  auto hzaddresses=std::vector< std::pair<BigInt,Int32> >(tot,std::make_pair(-1,0)); 

  NdPoint p(pdim);

  //if this is not available I use the slower conversion p->zaddress->Hz
  if (!this->hzaddress_conversion_pointquery)
  {
    VisusWarning()<<"The hzaddress_conversion_pointquery has not been created, so loc-by-loc queries will be a lot slower!!!!";

    //so you investigate why it's happening! .... I think only for the iphone could make sense....
    #if WIN32 && _DEBUG
    VisusAssert(false);
    #endif

    const NdPoint::coord_t* points_p=points;
    for (int N=0;N<tot;N++,points_p+=pdim) 
    {
      if (aborted()) return false;
      if (pdim>=1) {p[0]=points_p[0]; if (!(p[0]>=bounds.p1[0] && p[0]<bounds.p2[0])) continue; p[0] &= depth_mask[0];}
      if (pdim>=2) {p[1]=points_p[1]; if (!(p[1]>=bounds.p1[1] && p[1]<bounds.p2[1])) continue; p[1] &= depth_mask[1];}
      if (pdim>=3) {p[2]=points_p[2]; if (!(p[2]>=bounds.p1[2] && p[2]<bounds.p2[2])) continue; p[2] &= depth_mask[2];}
      if (pdim>=4) {p[3]=points_p[3]; if (!(p[3]>=bounds.p1[3] && p[3]<bounds.p2[3])) continue; p[3] &= depth_mask[3];}
      if (pdim>=5) {p[4]=points_p[4]; if (!(p[4]>=bounds.p1[4] && p[4]<bounds.p2[4])) continue; p[4] &= depth_mask[4];}
      hzaddresses[N]=std::make_pair(hzorder.getAddress(p),N);
    }
  }
  //the conversion from point to Hz will be faster
  else
  {
    BigInt zaddress;
    int    shift;
    auto   points_p=points;
    auto   loc=this->hzaddress_conversion_pointquery->loc;

    for (int N=0;N<tot;N++,points_p+=pdim) 
    {
      if (aborted()) 
        return false;
      if (pdim>=1) {p[0]=points_p[0]; if (!(p[0]>=bounds.p1[0] && p[0]<bounds.p2[0])) continue; p[0] &= depth_mask[0]; shift=        (      loc[0][p[0]].second) ; zaddress =loc[0][p[0]].first;}
      if (pdim>=2) {p[1]=points_p[1]; if (!(p[1]>=bounds.p1[1] && p[1]<bounds.p2[1])) continue; p[1] &= depth_mask[1]; shift=std::min(shift,loc[1][p[1]].second) ; zaddress|=loc[1][p[1]].first;}
      if (pdim>=3) {p[2]=points_p[2]; if (!(p[2]>=bounds.p1[2] && p[2]<bounds.p2[2])) continue; p[2] &= depth_mask[2]; shift=std::min(shift,loc[2][p[2]].second) ; zaddress|=loc[2][p[2]].first;}
      if (pdim>=4) {p[3]=points_p[3]; if (!(p[3]>=bounds.p1[3] && p[3]<bounds.p2[3])) continue; p[3] &= depth_mask[3]; shift=std::min(shift,loc[3][p[3]].second) ; zaddress|=loc[3][p[3]].first;}
      if (pdim>=5) {p[4]=points_p[4]; if (!(p[4]>=bounds.p1[4] && p[4]<bounds.p2[4])) continue; p[4] &= depth_mask[4]; shift=std::min(shift,loc[4][p[4]].second) ; zaddress|=loc[4][p[4]].first;}
      hzaddresses[N]=std::make_pair(((zaddress | last_bitmask)>>shift),N);
    }
  }

  //sort the address 
  std::sort(hzaddresses.begin(),hzaddresses.end());

  //do the for loop block aligned
  WaitAsync< Future<SharedPtr<BlockQuery> >, std::tuple<SharedPtr<BlockQuery>, int, int> > wait_async;

  access->beginRead();
  for (int A=0,B=0 ; !aborted() &&  A< (int)hzaddresses.size() ; A=B)
  {
    if (hzaddresses[A].first<0) {
      while (B<(int)hzaddresses.size() && hzaddresses[B].first<0)
        ++B;
      continue;
    }

    //align to the block
    BigInt HzFrom    = (hzaddresses[A].first>>bitsperblock)<<bitsperblock;
    BigInt HzTo      = HzFrom+(((BigInt)1)<<bitsperblock);
    VisusAssert(hzaddresses[A].first>=HzFrom && hzaddresses[A].first<HzTo);

    //end of the block
    while (B<(int)hzaddresses.size() && (hzaddresses[B].first>=HzFrom && hzaddresses[B].first<HzTo))
      ++B;

    auto block_query=std::make_shared<BlockQuery>(query->field,query->time,HzFrom,HzTo,aborted);
    wait_async.pushRunning(block_query->future, std::make_tuple(block_query,A,B));

    this->readBlock(access,block_query);
  }
  access->endRead();

  //wait for the block query to be ready
  for (int I=0,N= wait_async.size();I<N;I++)
  {
    auto popped= wait_async.popReady().second;

    auto block_query=std::get<0>(popped); VisusAssert(block_query);
    auto A=std::get<1>(popped);
    auto B=std::get<2>(popped);

    if (aborted() || block_query->getStatus()!=QueryOk) 
      continue;

    InsertBlockQuerySamplesIntoPointQuery op;
    NeedToCopySamples(op,query->field.dtype,this,query.get(),block_query.get(),&hzaddresses[0]+A,&hzaddresses[0]+B,aborted);
  }

  if (aborted())
    return false;

  query->currentLevelReady();
  return true;
}

////////////////////////////////////////////////////////////////////////
bool IdxDataset::executeBoxQueryWithAccess(SharedPtr<Access> access,SharedPtr<Query> query) 
{
  VisusAssert(access);

  #define PUSH()  (*((stack)++))=(item)
  #define POP()   (item)=(*(--(stack)))
  #define EMPTY() ((stack)==(STACK))

  int           bReading     = query->mode=='r';
  const Field&  field        = query->field;
  double        time         = query->time;

  int cur_resolution=query->cur_resolution;
  int end_resolution=query->getEndResolution();
  int max_resolution=query->max_resolution;

  if (!query->allocateBufferIfNeeded())
    return false;

  //filter enabled... need to go level by level
  if (auto filter=query->filter.value)
  {
    VisusAssert(bReading);

    Query::MergeMode merge_mode=Query::InsertSamples;

    //need to go level by level to rebuild the original data (top-down)
    for (int H=cur_resolution+1; H<=end_resolution; H++)
    {
      NdBox adjusted_box = adjustFilterBox(query.get(),filter.get(),query->aligned_box,H);

      auto Wquery=std::make_shared<Query>(this,'r');
      Wquery->time=query->time;
      Wquery->field=query->field;
      Wquery->position=adjusted_box;
      Wquery->end_resolutions={H};
      Wquery->max_resolution=max_resolution;
      Wquery->filter.enabled=false;
      Wquery->merge_mode=merge_mode;
      Wquery->aborted=query->aborted;

      //cannot get samples yet
      if (!this->beginQuery(Wquery))
      {
        VisusAssert(cur_resolution==-1);
        VisusAssert(!query->filter_query);
        continue;
      }

      //try to merge previous results
      if (auto Rquery= query->filter_query)
      {
        //auto allocate buffers
        if (!Wquery->allocateBufferIfNeeded())
          return false;

        if (!Query::mergeSamples(*Wquery,*Rquery,merge_mode,query->aborted))
        {
          VisusAssert(query->aborted());
          return false;
        }

        Wquery->cur_resolution = Rquery->cur_resolution;
      }

      if (!this->executeQuery(access, Wquery))
        return false;

      if (!filter->computeFilter(Wquery.get(),true))
        return false;

      query->filter_query= Wquery;
    }

    //cannot get samples yet... returning failed as a query without filters...
    if (!query->filter_query)
    {
      VisusAssert(false);//technically this should not happen since the outside query has got some samples
      return false;
    }

    query->nsamples  = query->filter_query->nsamples;
    query->logic_box = query->filter_query->logic_box;
    query->buffer    = query->filter_query->buffer;
  }
  //execute with access
  else 
  {
    int bitsperblock = access->bitsperblock;
    VisusAssert(bitsperblock);
  
    FastLoopStack  item,*stack=NULL;
    FastLoopStack  STACK[DatasetBitmaskMaxLen+1];

    DatasetBitmask bitmask = this->getBitmask();
    HzOrder hzorder(bitmask, query->max_resolution);

    std::vector<NdPoint::coord_t> fldeltas(max_resolution+1);
    for (int H = 0; H <= max_resolution; H++)
      fldeltas[H] = H? (hzorder.getLevelDelta(H)[bitmask[H]] >> 1) : 0;

    WaitAsync< Future< SharedPtr<BlockQuery> > , SharedPtr<BlockQuery> > wait_async;

    if (bReading)
      access->beginRead();
    else
      access->beginReadWrite();

    //do the loop up to the end or until a stop signal
    Aborted& aborted = query->aborted;
    for (int H=cur_resolution+1;!aborted() && H<=end_resolution;H++)
    {
      LogicBox Lbox=this->getLevelBox(hzorder,H);
      NdBox box=Lbox.alignBox(query->logic_box);
      if (!box.isFullDim())
        continue;

      //push first item
      BigInt hz = hzorder.getAddress(Lbox.p1);
      {
        item.box = Lbox;
        item.H   = H?1:0;
        stack    = STACK;
        PUSH();
      }
    
      while (!EMPTY())
      {
        POP();
        
        // no intersection
        if (!item.box.strictIntersect(box)) 
        {
          hz+=(((BigInt)1)<<(H-item.H));
          continue;
        }

        // intersection with hz-block!
        if ((H-item.H)<=bitsperblock)
        {
          BigInt HzFrom = (hz>>bitsperblock)<<bitsperblock;
          BigInt HzTo   = HzFrom+(((BigInt)1)<<bitsperblock);
          VisusAssert(hz>=HzFrom && hz<HzTo);

          if (aborted())
            break;

          auto read_block=std::make_shared<BlockQuery>(field,time,HzFrom,HzTo,aborted);

          if (bReading)
          {
            wait_async.pushRunning(read_block->future, read_block);
            readBlock(access,read_block);
          }
          else
          {
            //need a lease... so that I can read/merge/write like in a transaction mode
            access->acquireWriteLock(read_block);

            //need to read and wait the block
            readBlockAndWait(access, read_block);

            //WRITE block
            auto write_block=std::make_shared<BlockQuery>(field,time,HzFrom,HzTo,aborted);

           //read ok
            if (read_block->getStatus()==QueryOk)
            {
              write_block->buffer=read_block->buffer;
            }
            //I don't care if it fails... maybe does not exist
            else
            {
              write_block->nsamples=read_block->nsamples;
              write_block->logic_box=read_block->logic_box;
              write_block->buffer.layout=query->field.default_layout;
              write_block->buffer.resize(write_block->nsamples,write_block->field.dtype,__FILE__,__LINE__);
              write_block->buffer.fillWithValue(write_block->field.default_value);
            }

            mergeQueryWithBlock(query,write_block);

            //need to write and wait for the block
            writeBlockAndWait(access, write_block);

            //important! all writings are with a lease!
            access->releaseWriteLock(read_block);

            if (aborted() || write_block->getStatus() != QueryOk) {
              bReading? access->endRead() : access->endReadWrite();
              return false;
            }
          }

          // I know that block 0 convers several hz-levels from [0 to bitsperblock]
          if (HzFrom==0)
          {
            H=bitsperblock;
            break;
          }

          hz+=((BigInt)1)<<(H-item.H);
          continue;
        }

        //kd-traversal code
        int bit   = bitmask  [item.H];
        NdPoint::coord_t delta = fldeltas [item.H];
        ++item.H;
        item.box.p1[bit]+=delta;                         VisusAssert(item.box.isFullDim());PUSH();
        item.box.p1[bit]-=delta;item.box.p2[bit]-=delta; VisusAssert(item.box.isFullDim());PUSH();
         
      } //while (stack!=STACK)

    } //for 

    bReading ? access->endRead() : access->endReadWrite();

    //wait for block query completition
    if (bReading)
    {
      for (int I=0,N= wait_async.size();I<N;I++)
      {
        auto block_query= wait_async.popReady().second;
        VisusAssert(block_query);

        //I don't care if the read fails...
        if (!aborted() && block_query->getStatus()==QueryOk)
          mergeQueryWithBlock(query,block_query);
      }
    }

    //set the query status
    if (aborted())
      return false;
  }

  query->currentLevelReady();
  return true;

  #undef PUSH 
  #undef POP  
  #undef EMPTY
}

//////////////////////////////////////////////////////////////////////////////////////////
bool IdxDataset::nextQuery(SharedPtr<Query> query)
{
  if (!Dataset::nextQuery(query))
    return false;

  auto Rcurrent_resolution=query->cur_resolution;
  auto Rbox    =query->logic_box;
  auto Rbuffer =query->buffer;
  auto Rfilter_query= query->filter_query;

  //merging will happen only in certain cases
  query->buffer=Array();

  if (!this->setCurrentEndResolution(query))
  {
    query->setFailed("cannot set end resolution");
    return false;
  }

  //try to erge
  auto& Wbox=query->logic_box;
  if (Wbox.valid() && Rbox.valid())
  {
    if (!query->allocateBufferIfNeeded())
      return false;

    auto Wbuffer=query->buffer;
    if (!Query::mergeSamples(Wbox,Wbuffer,Rbox,Rbuffer,query->merge_mode,query->aborted))
    {
      if (query->aborted())
      {
        query->setFailed("query aborted");
        return false;
      }
      else
      {
        VisusAssert(false);
        query->setFailed("Merging failed for unknown reasons");
        return false;
      }
    }
  }

  query->filter_query=Rfilter_query;
  query->cur_resolution=(Rcurrent_resolution);
  return true;
}

/////////////////////////////////////////////////////////////////////////
SharedPtr<IdxDataset> IdxDataset::create(String filename, Array data, IdxFile idxfile)
{
  //override box
  if (idxfile.box == NdBox() && data && data.getTotalNumberOfSamples())
    idxfile.box = NdBox(NdPoint(data.getPointDim()), data.dims);

  //append field
  if (idxfile.fields.empty() && data.dtype.valid())
    idxfile.fields.push_back(Field("data", data.dtype));

  if (!idxfile.save(filename))
  {
    VisusError() << "idxfile.save(" << filename << ") failed";
    return SharedPtr<IdxDataset>();
  }

  auto vf = IdxDataset::loadDataset(filename);
  if (!vf)
  {
    VisusError() << "Dataset::loadDataset(" << filename << ") failed";
    return SharedPtr<IdxDataset>();
  }

  //write the data
  if (data && data.getTotalNumberOfSamples())
  {
    auto query = std::make_shared<Query>(vf.get(), 'w');
    query->time = vf->getDefaultTime();
    query->field = vf->getDefaultField();
    query->position = NdBox(NdPoint(data.getPointDim()), data.dims);
    query->start_resolution = 0;
    query->end_resolutions = { vf->getMaxResolution() };
    query->max_resolution = vf->getMaxResolution();

    if (!vf->beginQuery(query))
      return SharedPtr<IdxDataset>();

    VisusAssert(query->nsamples == data.dims);
    query->buffer = data;

    auto access = vf->createAccess();
    if (!vf->executeQuery(access, query))
      return SharedPtr<IdxDataset>();
  }

  return vf;
}


///////////////////////////////////////////////////
#if 0
bool IdxDataset::upgradeBoxQueryMaxResolution(int maxh)
{
  VisusAssert(false);//this function is not used anymore

  int query_maxh=query->max_resolution;

  //necessary condition
  VisusAssert(vf->idxfile.bitmask.hasRegExpr());
  VisusAssert(vf->idxfile.bitmask.getMaxResolution()<query_maxh && query_maxh<maxh);


  int pdim = vf->idxfile.bitmask.getPointDim();
  
  //WRONG : I should not change box here
  NdBox box = query->position.getNdBox();

  while (query_maxh<maxh)
  {
    ++query_maxh;
    int bit=vf->idxfile.bitmask[query_maxh];

    box.from[bit]<<=1;
    box.to  [bit]<<=1;

    this->h_box.from[bit]<<=1;
    this->h_box.to  [bit]<<=1;

    query->max_resolution=(query_maxh); //WRONG! I cannot change max resolution here
  }

  this->shift=LogicBox::getShift(this->h_box,query->nsamples);
  this->delta=NdPoint::one(pdim)<<this->shift;

  return true;
}
#endif

} //namespace Visus
