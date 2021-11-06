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

#include <Visus/Dataset.h>
#include <Visus/DiskAccess.h>
#include <Visus/MultiplexAccess.h>
#include <Visus/CloudStorageAccess.h>
#include <Visus/RamAccess.h>
#include <Visus/FilterAccess.h>
#include <Visus/NetService.h>
#include <Visus/StringTree.h>
#include <Visus/Polygon.h>
#include <Visus/IdxFile.h>
#include <Visus/File.h>
#include <Visus/GoogleMapsDataset.h>
#include <Visus/GoogleMapsAccess.h>
#include <Visus/IdxHzOrder.h>
#include <Visus/IdxMultipleDataset.h>
#include <Visus/OnDemandAccess.h>
#include <Visus/ModVisusAccess.h>
#include <Visus/MandelbrotAccess.h>
#include <Visus/IdxMultipleAccess.h>
#include <Visus/IdxDiskAccess.h>
#include <Visus/IdxFilter.h>

namespace Visus {

VISUS_IMPLEMENT_SINGLETON_CLASS(DatasetFactory)

/////////////////////////////////////////////////////////////////////////////////////////////
class InsertIntoPointQuery
{
public:

  //operator()
  template <class Sample>
  bool execute(Dataset* db, PointQuery* query, BlockQuery* block_query)
  {
    //only row major supported
    VisusReleaseAssert(block_query->buffer.layout.empty());

    if (block_query->mode == 'r')
    {
      auto query_samples = GetSamples<Sample>(query->buffer);
      auto block_samples = GetSamples<Sample>(block_query->buffer);
      for (auto it : *query->offsets[block_query->blockid])
        query_samples[it.first] = block_samples[it.second];
    }
    else
    {
      auto block_samples = GetSamples<Sample>(block_query->buffer);
      auto query_samples = GetSamples<Sample>(query->buffer);
      for (auto it : *query->offsets[block_query->blockid])
        block_samples[it.second] = query_samples[it.first];
    }

    return true;
  }
};

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
        for (Wpixel[1] = 0; Wpixel[1] < Wbuffer.dims[1]; Wpixel[1]++) { Rpixel[1] = W2R(1); Rpos[1] = Rpixel[1] * Rstride[1];
        for (Wpixel[0] = 0; Wpixel[0] < Wbuffer.dims[0]; Wpixel[0]++) { Rpixel[0] = W2R(0); Rpos[0] = Rpixel[0] * Rstride[0] + Rpos[1];
          W[Wpos++] = R[Rpos[0]];
        }}
      }
      else if (pdim == 3)
      {
        for (Wpixel[2] = 0; Wpixel[2] < Wbuffer.dims[2]; Wpixel[2]++) { Rpixel[2] = W2R(2); Rpos[2] = Rpixel[2] * Rstride[2];
        for (Wpixel[1] = 0; Wpixel[1] < Wbuffer.dims[1]; Wpixel[1]++) { Rpixel[1] = W2R(1); Rpos[1] = Rpixel[1] * Rstride[1] + Rpos[2];
        for (Wpixel[0] = 0; Wpixel[0] < Wbuffer.dims[0]; Wpixel[0]++) { Rpixel[0] = W2R(0); Rpos[0] = Rpixel[0] * Rstride[0] + Rpos[1];
          W[Wpos++] = R[Rpos[0]];
        }}}
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


////////////////////////////////////////////////////////////////////////////////////
StringTree FindDatasetConfig(StringTree ar, String url)
{
  auto all_datasets = ar.getAllChilds("dataset");
  for (auto it : all_datasets)
  {
    if (it->readString("name") == url) {
      VisusAssert(it->hasAttribute("url"));
      return *it;
    }
  }

  for (auto it : all_datasets)
  {
    if (it->readString("url") == url)
      return *it;
  }

  auto ret = StringTree("dataset");
  ret.write("url", url);
  return ret;
}

/////////////////////////////////////////////////////////////////////////
SharedPtr<Dataset> LoadDatasetEx(StringTree ar)
{
  String url = ar.readString("url");

  //could the 'ar' self contained (as for GoogleMapsDatasets)
  if (!url.empty())
  {
    if (!Url(url).valid())
      ThrowException("LoadDataset", url, "failed. Not a valid url");

    String content = Utils::loadTextDocument(url);

    if (content.empty())
      ThrowException("empty content");

    //enrich ar by loaded document
    auto doc = StringTree::fromString(content);

    // backward compatible, old idx text format that is not xml
    if (!doc.valid())
    {
      //TODO: handle this better
      if (StringUtils::contains(url, ".idx2"))
      {
        ar.write("typename", "IdxDataset2");
        ar.write("url", url);
      }
      else
      {
        ar.write("typename", "IdxDataset");

        IdxFile old_format;
        old_format.readFromOldFormat(content);
        ar.writeObject("idxfile", old_format);
      }
    }
    else
    {
      //backward compatible
      if (doc.name == "midx")
      {
        doc.name = "dataset";
        doc.write("typename", "IdxMultipleDataset");
      }

      //example <dataset tyname="IdxMultipleDataset">...</dataset>
      //note: ar has precedence
      StringTree::merge(ar, doc);
    }
  }

  auto TypeName = ar.getAttribute("typename");
  VisusReleaseAssert(!TypeName.empty());
  auto ret = DatasetFactory::getSingleton()->createInstance(TypeName);
  if (!ret)
  {
    ThrowException("LoadDataset", url, "failed. Cannot DatasetFactory::getSingleton()->createInstance", TypeName);
    return SharedPtr<Dataset>();
  }

  ret->readDatasetFromArchive(ar);
  return ret;
}

////////////////////////////////////////////////
SharedPtr<Dataset> LoadDataset(String url) {
  auto ar = FindDatasetConfig(*DbModule::getModuleConfig(), url);
  return LoadDatasetEx(ar);
}

///////////////////////////////////////////////////////////
Field Dataset::getFieldEx(String fieldname) const
{
  //remove any params (they will be used in queries)
  ParseStringParams parse(fieldname);

  auto it = find_field.find(parse.without_params);
  if (it != find_field.end())
  {
    Field ret = it->second;
    ret.name = fieldname; //important to keep the params! example "temperature?time=30"
    ret.params = parse.params;
    return ret;
  }

  //not found
  return Field();
}

///////////////////////////////////////////////////////////
Field Dataset::getField(String name) const {
  try {
    return getFieldEx(name);
  }
  catch (std::exception ex) {
    return Field();
  }
}


////////////////////////////////////////////////////////////////////
SharedPtr<Access> Dataset::createAccess(StringTree config,bool bForBlockQuery)
{
  if (!config.valid())
    config = getDefaultAccessConfig();

  String type =StringUtils::toLower(config.readString("type"));

  //I always need an access
  if (auto google = dynamic_cast<GoogleMapsDataset*>(this))
  {
    if (type.empty() || type == "GoogleMapsAccess")
    {
      SharedPtr<NetService> netservice;
      if (!bServerMode)
      {
        int nconnections = config.readInt("nconnections", 8);
        netservice = std::make_shared<NetService>(nconnections);
      }
      return std::make_shared<GoogleMapsAccess>(this, google->tiles_url, netservice);
    }
  }

  if (auto idx = dynamic_cast<IdxDataset*>(this))
  {
    //consider I can have thousands of childs (NOTE: this attribute should be "inherited" from child)
    auto midx = dynamic_cast<IdxMultipleDataset*>(this);
    if (midx)
      config.write("disable_async", true);

    String type = StringUtils::toLower(config.readString("type"));

    //no type, create default
    if (type.empty())
    {
      Url url = config.readString("url", getUrl());

      //local disk access
      if (url.isFile())
      {
        if (midx)
          return std::make_shared<IdxMultipleAccess>(midx, config);
        else
          return std::make_shared<IdxDiskAccess>(idx, config);
      }
      else
      {
        VisusAssert(url.isRemote());

        if (bool is_cloud = !CloudStorage::guessType(url).empty())
          return std::make_shared<CloudStorageAccess>(this, config);

        if (bForBlockQuery)
          return std::make_shared<ModVisusAccess>(this, config);
        else
          //I can execute box/point queries on the remote server
          return SharedPtr<Access>();
      }
    }

    //IdxDiskAccess
    if (type == "disk" || type == "idxdiskaccess")
      return std::make_shared<IdxDiskAccess>(idx, config);

    //IdxMultipleAccess
    if (type == "idxmultipleaccess" || type == "midx" || type == "multipleaccess")
    {
      VisusReleaseAssert(midx);
      return std::make_shared<IdxMultipleAccess>(midx, config);
    }

    //IdxMandelbrotAccess
    if (type == "idxmandelbrotaccess" || type == "mandelbrotaccess")
      return std::make_shared<MandelbrotAccess>(this, config);

    //ondemandaccess
    if (type == "ondemandaccess")
      return std::make_shared<OnDemandAccess>(this, config);
  }

  if (!config.valid()) {
    VisusAssert(!bForBlockQuery);
    return SharedPtr<Access>(); //pure remote query
  }

  if (type.empty()) {
    VisusAssert(false); //please handle this case in your dataset since I dont' know how to handle this situation here
    return SharedPtr<Access>();
  }

  //DiskAccess
  if (type=="diskaccess")
    return std::make_shared<DiskAccess>(this, config);

  // MULTIPLEX 
  if (type=="multiplex" || type=="multiplexaccess")
    return std::make_shared<MultiplexAccess>(this, config);
  
  // RAM CACHE 
  if (type == "lruam" || type == "ram" || type == "ramaccess")
  {
    auto ret = std::make_shared<RamAccess>(getDefaultBitsPerBlock());
    ret->can_read = StringUtils::contains(config.readString("chmod", Access::DefaultChMod), "r");
    ret->can_write = StringUtils::contains(config.readString("chmod", Access::DefaultChMod), "w");
    ret->setAvailableMemory(StringUtils::getByteSizeFromString(config.readString("available", "128mb")));
    return ret;
  }

  // NETWORK 
  if (type=="network" || type=="modvisusaccess")
    return std::make_shared<ModVisusAccess>(this, config);

  //CloudStorageAccess
  if (type=="cloudstorageaccess")
    return std::make_shared<CloudStorageAccess>(this, config);
  
  // FILTER 
  if (type=="filter" || type=="filteraccess")
    return std::make_shared<FilterAccess>(this, config);

  //problem here
  VisusAssert(false);
  return SharedPtr<Access>();
}

///////////////////////////////////////////////////////////////////////////////////
void Dataset::compressDataset(std::vector<String> compression, Array data)
{
  auto idx = dynamic_cast<IdxDataset*>(this);
  VisusReleaseAssert(idx);

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

    auto Waccess = std::make_shared<IdxDiskAccess>(idx);
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

          //NOTE: the layout will remain the same, I am not switching it]

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

    auto compressed_idx_file = idx->idxfile;
    compressed_idx_file.filename_template = idxfile.filename_template + suffix;
    compressed_idx_file.save(compressed_idx_filename);

    auto Waccess = std::make_shared<IdxDiskAccess>(idx, compressed_idx_file);
    auto Raccess = std::make_shared<IdxDiskAccess>(idx, idxfile);

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

        if (!prev_filename.empty())
          PrintInfo("Compressed file", prev_filename);

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

          //NOTE: the layout will be the same

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

////////////////////////////////////////////////////////////////////////////////////
bool Dataset::insertSamples(LogicSamples Wsamples, Array Wbuffer, LogicSamples Rsamples, Array Rbuffer, Aborted aborted)
{
  if (!Wsamples.valid() || !Rsamples.valid())
    return false;

  if (Wbuffer.dtype != Rbuffer.dtype || Wbuffer.dims != Wsamples.nsamples || Rbuffer.dims != Rsamples.nsamples)
  {
    VisusAssert(false);
    return false;
  }

  //cannot find intersection (i.e. no sample to merge)
  BoxNi box = Wsamples.logic_box.getIntersection(Rsamples.logic_box);
  if (!box.isFullDim())
    return false;

  /*
  Example of the problem to solve:

  Wsamples:=-2 + kw*6         -2,          4,          10,            *16*,            22,            28,            34,            40,            *46*,            52,            58,  ...)
  Rsamples:=-4 + kr*5    -4,         1,        6,         11,         *16*,          21,       25,            ,31         36,          41,         *46*,          51,         56,       ...)

  give kl,kw,kr independent integers all >=0

  leastCommonMultiple(2,6,5)= 2*3*5 =30

  First "common" value (i.e. minimum value satisfying all 3 conditions) is 16
  */

  int pdim = Rbuffer.getPointDim();
  PointNi delta(pdim);
  for (int D = 0; D < pdim; D++)
  {
    Int64 lcm = Utils::leastCommonMultiple(Rsamples.delta[D], Wsamples.delta[D]);

    Int64 P1 = box.p1[D];
    Int64 P2 = box.p2[D];

    while (
      !Utils::isAligned(P1, Wsamples.logic_box.p1[D], Wsamples.delta[D]) ||
      !Utils::isAligned(P1, Rsamples.logic_box.p1[D], Rsamples.delta[D]))
    {
      //NOTE: if the value is already aligned, alignRight does nothing
      P1 = Utils::alignRight(P1, Wsamples.logic_box.p1[D], Wsamples.delta[D]);
      P1 = Utils::alignRight(P1, Rsamples.logic_box.p1[D], Rsamples.delta[D]);

      //cannot find any alignment, going beyond the valid range
      if (P1 >= P2)
        return false;

      //continue in the search IIF it's not useless
      if ((P1 - box.p1[D]) >= lcm)
      {
        //should be acceptable to be here, it just means that there are no samples to merge... 
        //but since 99% of the time Visus has pow-2 alignment it is high unlikely right now... adding the VisusAssert just for now
        VisusAssert(false);
        return false;
      }
    }

    delta[D] = lcm;
    P2 = Utils::alignRight(P2, P1, delta[D]);
    box.p1[D] = P1;
    box.p2[D] = P2;
  }

  VisusAssert(box.isFullDim());

  auto wfrom = Wsamples.logicToPixel(box.p1); auto wto = Wsamples.logicToPixel(box.p2); auto wstep = delta.rightShift(Wsamples.shift);
  auto rfrom = Rsamples.logicToPixel(box.p1); auto rto = Rsamples.logicToPixel(box.p2); auto rstep = delta.rightShift(Rsamples.shift);

  VisusAssert(PointNi::max(wfrom, PointNi(pdim)) == wfrom);
  VisusAssert(PointNi::max(rfrom, PointNi(pdim)) == rfrom);

  wto = PointNi::min(wto, Wbuffer.dims); wstep = PointNi::min(wstep, Wbuffer.dims);
  rto = PointNi::min(rto, Rbuffer.dims); rstep = PointNi::min(rstep, Rbuffer.dims);

  //first insert samples in the right position!
  if (!ArrayUtils::insert(Wbuffer, wfrom, wto, wstep, Rbuffer, rfrom, rto, rstep, aborted))
    return false;

  return true;
}



////////////////////////////////////////////////////////////////////
SharedPtr<BlockQuery> Dataset::createBlockQuery(BigInt blockid, Field field, double time, int mode, Aborted aborted)
{
  auto ret = std::make_shared<BlockQuery>();
  ret->dataset = this;
  ret->field = field;
  ret->time = time;
  ret->mode = mode; VisusAssert(mode == 'r' || mode == 'w');
  ret->aborted = aborted;
	ret->blockid = blockid;
	ret->logic_samples = getBlockQuerySamples(blockid, ret->H);
  return ret;
}

////////////////////////////////////////////////
void Dataset::executeBlockQuery(SharedPtr<Access> access,SharedPtr<BlockQuery> query)
{
  VisusAssert(access->isReading() || access->isWriting());

  int mode = query->mode; 
  auto failed = [&](String reason) {

    if (!access)
      query->setFailed(reason);
    else
      mode == 'r'? access->readFailed(query, reason) : access->writeFailed(query, reason);
   
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

  if (!query->logic_samples.valid())
    return failed("logic_samples not valid");

  //scrgiorgio: add this optimization to avoid empty blocks
  if (!query->logic_samples.logic_box.intersect(this->getLogicBox()))
    return failed("");//"no intersection with logic box" (TOO many messages with )

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

////////////////////////////////////////////////////////////////////
LogicSamples Dataset::getBlockQuerySamples(BigInt blockid, int& H)
{
  auto bitmask = getBitmask();
  auto bitsperblock = getDefaultBitsPerBlock();
  auto samplesperblock = 1 << bitsperblock;

  PointNi p0, delta;
  if (blocksFullRes())
  {
    H = blockid == 0 ? bitsperblock : bitsperblock + 0 + Utils::getLog2(1 + blockid);
    delta = block_samples[H].delta;
    Int64 first_block_in_level = (((Int64)1) << (H - bitsperblock)) - 1;
    auto coord = bitmask.deinterleave(blockid - first_block_in_level, H - bitsperblock);
    p0 = coord.innerMultiply(block_samples[H].logic_box.size());
  }
  else
  {
    H = blockid == 0 ? bitsperblock : bitsperblock + 1 + Utils::getLog2(0 + blockid);
    delta = block_samples[H].delta;

    //for first block I get twice the samples sice the blocking '1' can be '0' considering all previous levels 
    if (blockid == 0)
      delta[bitmask[H]] >>= 1;

    p0 = HzOrder(bitmask).getPoint(blockid * samplesperblock);
  }

  auto ret = LogicSamples(block_samples[H].logic_box.translate(p0), delta);
  VisusAssert(ret.valid());
  return ret;
}


////////////////////////////////////////////////
SharedPtr<BoxQuery> Dataset::createBoxQuery(BoxNi logic_box, Field field, double time, int mode, Aborted aborted)
{
  auto ret = std::make_shared<BoxQuery>();
  ret->dataset = this;
  ret->field = field;
  ret->time = time;
  ret->mode = mode; VisusAssert(mode == 'r' || mode == 'w');
  ret->aborted = aborted;
  ret->logic_box = logic_box;
  ret->filter.domain = this->getLogicBox();
  return ret;
}

//////////////////////////////////////////////////////////////
void Dataset::beginBoxQuery(SharedPtr<BoxQuery> query)
{
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

  //google has only even resolution
  if (auto google = dynamic_cast<GoogleMapsDataset*>(this))
  {
    std::set<int> good;
    for (auto it : query->end_resolutions)
    {
      auto value = (it >> 1) << 1;
      good.insert(Utils::clamp(value, getDefaultBitsPerBlock(), getMaxResolution()));
    }

    query->end_resolutions = std::vector<int>(good.begin(), good.end());
  }

  //end resolution
  for (auto it : query->end_resolutions)
  {
    if (it <0 || it> this->getMaxResolution())
      return query->setFailed("wrong end resolution");
  }

  //start_resolution
  if (query->start_resolution > 0)
  {
    if (query->end_resolutions.size() != 1 || query->start_resolution != query->end_resolutions[0])
      return query->setFailed("wrong query start resolution");
  }

  //filters?
  if (query->filter.enabled)
  {
    if (!query->filter.dataset_filter)
    {
      query->filter.dataset_filter = createFilter(query->field);

      if (!query->filter.dataset_filter)
        query->disableFilters();
    }
  }

  for (auto it : query->end_resolutions)
  {
    if (setBoxQueryEndResolution(query, it))
      return query->setRunning();
  }

  query->setFailed("cannot find a good end_resolution to start with");
}

//////////////////////////////////////////////////////////////
bool Dataset::executeBoxQuery(SharedPtr<Access> access, SharedPtr<BoxQuery> query)
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

  if (!query->allocateBufferIfNeeded())
  {
    query->setFailed("cannot allocate buffer");
    return false;
  }

  if (!access)
  {
    if (auto google=dynamic_cast<GoogleMapsDataset*>(this))
      access = createAccessForBlockQuery(); //the google server cannot handle pure remote queries (i.e. compose the tiles on server side)
    else
      return executeBoxQueryOnServer(query); //pure remote query
  }

  //filter enabled... need to go level by level
  if (auto filter = query->filter.dataset_filter)
    return executeBlockQuerWithFilters(access, query, filter);

  int nread = 0, nwrite = 0;

  //example, say each block is 32kb -> 512*32kb==16MB
  WaitAsync< Future<Void> > wait_async(/*max_running*/512);

  auto blocks = createBlockQueriesForBoxQuery(query);

  if (query->aborted())
    return false;

  //rehentrant call...(just to not close the file too soon)
  bool bEndIO = false;
	if (query->mode == 'w')
	{
    if (!access->isWriting())
    {
      bEndIO = true;
      access->beginWrite();
    }
	}
	else
	{
    if (!access->isReading())
    {
      bEndIO = true;
      access->beginRead();
    }
	}

  for (auto blockid : blocks)
  {
    if (query->aborted())
      break;

    nread++;

    auto read_block = createBlockQuery(blockid, query->field, query->time, 'r', query->aborted);

		if (query->mode == 'r')
    {
      executeBlockQuery(access, read_block);
      wait_async.pushRunning(read_block->done, [this, query, read_block](Void) {
        //I don't care if the read fails...
        if (!query->aborted() && read_block->ok())
          mergeBoxQueryWithBlockQuery(query, read_block);
        });
    }
    else
    {
      //need a lease... so that I can read/merge/write like in a transaction mode
      access->acquireWriteLock(read_block);

      //need to read and wait the block
      executeBlockQueryAndWait(access, read_block);

      auto write_block = createBlockQuery(read_block->blockid, query->field, query->time, 'w', query->aborted);

      //read ok
      if (read_block->ok())
        write_block->buffer = read_block->buffer;
      //I don't care if it fails... maybe does not exist
      else
        write_block->allocateBufferIfNeeded();

      //here a change in the layout (hzorder) can happen
      mergeBoxQueryWithBlockQuery(query, write_block);

      //need to write and wait for the block
      executeBlockQueryAndWait(access, write_block);
      nwrite++;

      //important! all writings are with a lease!
      access->releaseWriteLock(read_block);

      if (query->aborted() || write_block->failed()) {
        if (bEndIO) 
          access->endIO();
        return false;
      }
    }
  }

  if (bEndIO) 
    access->endIO();

  wait_async.waitAllDone();

  //PrintInfo("aysnc read",concatenate(nread, "/", block_queries.size()),"...");
  //PrintInfo("Query finished", "nread", nread, "nwrite", nwrite);

  //set the query status
  if (query->aborted())
    return false;

  VisusAssert(query->buffer.dims == query->getNumberOfSamples());
  query->setCurrentResolution(query->end_resolution);
  return true;

}

//////////////////////////////////////////////////////////////
void Dataset::nextBoxQuery(SharedPtr<BoxQuery> query)
{
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

  auto Rcurrent_resolution = query->getCurrentResolution();
  auto Rsamples = query->logic_samples;
  auto Rbuffer = query->buffer;
  auto Rfilter_query = query->filter.query;

  if (!setBoxQueryEndResolution(query, query->end_resolutions[Utils::find(query->end_resolutions, query->end_resolution) + 1]))
    VisusReleaseAssert(false);

  //asssume no merging
  query->buffer = Array();

  //merge with previous results
  if (this->missing_blocks)
  {
    if (!query->allocateBufferIfNeeded())
      return failed("out of memory");

    //cannot merge, scrgiorgio: can it really happen??? (maybe when I produce numpy arrays by projecting script...)
    VisusReleaseAssert(Rsamples.valid() && Rsamples.nsamples == Rbuffer.dims);

    //NOTE: this op is slow (like 2sec for 1GB data)
    auto t1 = Time::now();
    InterpolateBufferOperation op;
    if (!ExecuteOnCppSamples(op, query->buffer.dtype, query->logic_samples, query->buffer, Rsamples, Rbuffer, query->aborted))
      return failed("interpolate samples failed");
    auto msec = t1.elapsedMsec();
    if (msec > 100)
      PrintInfo("Interpolation of buffer", StringUtils::getStringFromByteSize(query->buffer.dtype.getByteSize(query->buffer.dims)), "done in", msec, "msec");
  }
  else if (!blocksFullRes())
  {
    if (!query->allocateBufferIfNeeded())
      return failed("out of memory");

    //cannot merge, scrgiorgio: can it really happen??? (maybe when I produce numpy arrays by projecting script...)
    VisusReleaseAssert(Rsamples.valid() && Rsamples.nsamples == Rbuffer.dims);

    //I must be sure that 'inserted samples' from Rbuffer must be untouched in Wbuffer
    //this is for wavelets where I need the coefficients to be right
    auto t1 = Time::now();
    if (!insertSamples(query->logic_samples, query->buffer, Rsamples, Rbuffer, query->aborted))
      return failed("insert samples failed");
    auto msec = t1.elapsedMsec();
    if (msec > 100)
      PrintInfo("Insert samples", StringUtils::getStringFromByteSize(query->buffer.dtype.getByteSize(query->buffer.dims)), "done in", msec, "msec");
  }
  else
  {
    //new blocks will replace all samples
    VisusReleaseAssert(blocksFullRes() && !this->missing_blocks);
  }

  query->filter.query = Rfilter_query;
  query->setCurrentResolution(Rcurrent_resolution);
}

////////////////////////////////////////////////
int Dataset::guessBoxQueryEndResolution(Frustum logic_to_screen, Position logic_position)
{
  if (!logic_position.valid())
    return {};
  
  auto minh = getDefaultBitsPerBlock();
  auto maxh = getMaxResolution();
  auto endh = maxh;
  auto pdim = getPointDim();

  if (logic_to_screen.valid())
  {
    //important to work with orthogonal box
    auto logic_box = logic_position.toAxisAlignedBox();

    FrustumMap map(logic_to_screen);

    std::vector<Point2d> screen_points;
    for (auto p : logic_box.getPoints())
      screen_points.push_back(map.projectPoint(p.toPoint3()));

    //project on the screen
    std::vector<double> screen_distance = { 0,0,0 };

    for (auto edge : BoxNi::getEdges(pdim))
    {
      auto axis = edge.axis;
      auto s0 = screen_points[edge.index0];
      auto s1 = screen_points[edge.index1];
      auto Sd = s0.distance(s1);
      screen_distance[axis] = std::max(screen_distance[axis], Sd);
    }

    const int max_3d_texture_size = 2048;

    auto nsamples = logic_box.size().toPoint3();
    while (endh > 0)
    {
      std::vector<double> samples_per_pixel = {
        nsamples[0] / screen_distance[0],
        nsamples[1] / screen_distance[1],
        nsamples[2] / screen_distance[2]
      };

      std::sort(samples_per_pixel.begin(), samples_per_pixel.end());

      auto quality = sqrt(samples_per_pixel[0] * samples_per_pixel[1]);

      //note: in 2D samples_per_pixel[2] is INF; in 3D with an ortho view XY samples_per_pixel[2] is INF (see std::sort)
      bool bGood = quality < 1.0;

      if (pdim == 3 && bGood)
        bGood =
        nsamples[0] <= max_3d_texture_size &&
        nsamples[1] <= max_3d_texture_size &&
        nsamples[2] <= max_3d_texture_size;

      if (bGood)
        break;

      //by decreasing resolution I will get half of the samples on that axis
      auto bit = bitmask[endh];
      nsamples[bit] *= 0.5;
      --endh;
    }
  }

  return Utils::clamp(endh, minh, maxh);
}

//////////////////////////////////////////////////////////////
std::vector<BigInt> Dataset::createBlockQueriesForBoxQuery(SharedPtr<BoxQuery> query)
{
  VisusReleaseAssert(blocksFullRes());
  std::vector<BigInt> ret;

  auto bitsperblock = this->getDefaultBitsPerBlock();

  BoxNi box = this->getBitmask().getPow2Box();
  std::stack< std::tuple<BoxNi, BigInt, int> > stack; // box / blockid / H
  stack.push(std::make_tuple( box ,0, bitsperblock ));
  while (!stack.empty() && !query->aborted())
  {
    auto top = stack.top();
    stack.pop();

    if (query->aborted())
      return {};

    auto box     = std::get<0>(top);
    auto blockid = std::get<1>(top);
    auto H       = std::get<2>(top);

    if (!box.getIntersection(query->logic_box).isFullDim())
      continue;

    //is the resolution I need?
    if (H == query->end_resolution)
    {
      ret.push_back(blockid);
    }
    else
    {
      auto split_bit = bitmask[1 + H - bitsperblock];
      auto middle = (box.p1[split_bit] + box.p2[split_bit]) >> 1;
      auto rbox = box; rbox.p1[split_bit] = middle; stack.push(std::make_tuple(rbox, blockid * 2 + 2, H + 1));
      auto lbox = box; lbox.p2[split_bit] = middle; stack.push(std::make_tuple(lbox, blockid * 2 + 1, H + 1));
    }
  }

  return ret;
}

//////////////////////////////////////////////////////////////
bool Dataset::mergeBoxQueryWithBlockQuery(SharedPtr<BoxQuery> query, SharedPtr<BlockQuery> block_query)
{
  VisusAssert(block_query->buffer.layout.empty());

  VisusAssert(query->field.dtype == block_query->field.dtype);

  if (!block_query->logic_samples.valid())
    return false;

  auto Wsamples = query->logic_samples;
  auto Wbuffer  = query->buffer;

  auto Rsamples = block_query->logic_samples;
  auto Rbuffer  = block_query->buffer;

  if (query->mode == 'w')
  {
    std::swap(Wsamples, Rsamples);
    std::swap(Wbuffer,  Rbuffer);
  }

  /* Important note
  Merging directly the block is wrong. In fact there are queries with filters that need to go level by level (coarse to fine).
  If I do the wrong way:

  filter-query:=
    Q(H=0)             <- I would directly merge block 0
    Q(H=1)             <- I would directly merge block 0. WRONG!!! I cannot overwrite samples belonging to H=0 since they have the filter already applied
    ...
    Q(H=bitsperblock)
  */
  if (!blocksFullRes() && block_query->blockid == 0)
  {
    auto hstart = std::max(query->getCurrentResolution() + 1, block_query->blockid == 0 ? 0 : block_query->H);
    auto hend   = std::min(query->getEndResolution()        ,                                 block_query->H);
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

        Lsamples <- Wsamples

      int this way I'm sure that all Wsamples P' are left unmodified

      Note also that merge can fail simply because there are no samples to merge at a certain level
      */

      insertSamples(Lsamples, Lbuffer, Wsamples, Wbuffer, query->aborted);
      insertSamples(Lsamples, Lbuffer, Rsamples, Rbuffer, query->aborted);
      insertSamples(Wsamples, Wbuffer, Lsamples, Lbuffer, query->aborted);
    }

    return query->aborted() ? false : true;
  }

  return insertSamples(Wsamples, Wbuffer, Rsamples, Rbuffer, query->aborted);
}

//////////////////////////////////////////////////////////////
bool Dataset::convertBlockQueryToRowMajor(SharedPtr<BlockQuery> block_query) 
{
  //already in row major
  if (block_query->buffer.layout.empty())
    return true;

  //idx full res is row major only
  VisusReleaseAssert(!blocksFullRes());

  //note I cannot use buffer of block_query because I need them to execute other queries
  Array row_major = block_query->buffer.clone();

  auto query = createEquivalentBoxQuery('r', block_query);
  beginBoxQuery(query);
  if (!query->isRunning())
  {
    VisusAssert(false);
    return false;
  }

  //as the query has not already been executed!
  query->setCurrentResolution(query->start_resolution - 1);
  query->buffer = row_major;

  if (!mergeBoxQueryWithBlockQuery(query, block_query))
  {
    VisusAssert(block_query->aborted());
    return false;
  }

  //now block query it's row major
  block_query->buffer = row_major;
  block_query->buffer.layout = "";
  return true;
}

//////////////////////////////////////////////////////////////
bool Dataset::setBoxQueryEndResolution(SharedPtr<BoxQuery> query, int value)
{
  VisusAssert(query->end_resolution < value);
  query->end_resolution = value;

  auto bitmask          = this->idxfile.bitmask;
  auto start_resolution = query->start_resolution;
  auto end_resolution   = query->end_resolution;

  BoxNi logic_box = query->logic_box.withPointDim(this->getPointDim());

  //special case for query with filters
  //I need to go level by level [0,1,2,...] in order to reconstruct the original data
  if (auto filter = query->filter.dataset_filter)
  {
    //important to return the "final" number of samples (see execute for loop)
    logic_box = this->adjustBoxQueryFilterBox(query.get(), filter.get(), logic_box, end_resolution);
    query->filter.adjusted_logic_box = logic_box;
  }

  auto delta = this->level_samples[end_resolution].delta;

  if (blocksFullRes())
  {
    logic_box = query->logic_box.getIntersection(this->getLogicBox());
    logic_box = level_samples[end_resolution].alignBox(logic_box);
  }
  else
  {
    //I get twice the samples of the samples because I'm allowing the blocking '1' bit to be anything coming from previous levels
    if (start_resolution == 0 && end_resolution > 0)
      delta[bitmask[end_resolution]] >>= 1;

    PointNi P1incl, P2incl;
    for (int H = start_resolution; H <= end_resolution; H++)
    {
      BoxNi box = level_samples[H].alignBox(logic_box);
      if (!box.isFullDim()) continue;
      PointNi p1incl = box.p1;
      PointNi p2incl = box.p2 - level_samples[H].delta;
      P1incl = P1incl.getPointDim() ? PointNi::min(P1incl, p1incl) : p1incl;
      P2incl = P2incl.getPointDim() ? PointNi::max(P2incl, p2incl) : p2incl;
    }

    if (!P1incl.getPointDim())
      return false;

    logic_box = BoxNi(P1incl, P2incl + delta);
  }

  if (!logic_box.isFullDim()) 
    return false;

  query->logic_samples = LogicSamples(logic_box, delta);
  VisusAssert(query->logic_samples.valid());
  return true;
}

/////////////////////////////////////////////////////////////////////////
NetRequest Dataset::createBoxQueryRequest(SharedPtr<BoxQuery> query)
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

  //I may have some extra params I want to keep!
  auto ret = NetRequest(url.withPath("/mod_visus"));
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

////////////////////////////////////////////////////////////////////
bool Dataset::executeBoxQueryOnServer(SharedPtr<BoxQuery> query)
{
  auto request = createBoxQueryRequest(query);
  if (!request.valid())
  {
    query->setFailed("cannot create box query request");
    return false;
  }

  auto response = NetService::getNetResponse(request);
  if (!response.isSuccessful())
  {
    query->setFailed(cstring("network request failed", cnamed("errormsg", response.getErrorMessage())));
    return false;
  }

  auto decoded = response.getCompatibleArrayBody(query->getNumberOfSamples(), query->field.dtype);
  if (!decoded.valid()) {
    query->setFailed("failed to decode body");
    return false;
  }

  query->buffer = decoded;
  query->setCurrentResolution(query->end_resolution);
  return true;
}



/////////////////////////////////////////////////////////
SharedPtr<PointQuery> Dataset::createPointQuery(Position logic_position, Field field, double time, Aborted aborted)
{
  VisusAssert(!blocksFullRes());
  auto ret = std::make_shared<PointQuery>();
  ret->dataset = this;
  ret->field = field = field;
  ret->time = time;
  ret->mode = 'r';
  ret->aborted = aborted;
  ret->logic_position = logic_position;
  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////
void Dataset::beginPointQuery(SharedPtr<PointQuery> query)
{
  VisusAssert(!blocksFullRes());

  if (!query)
    return;

  if (query->getStatus() != Query::QueryCreated)
    return;

  //if you want to set a buffer for 'w' queries, please do it after begin
  VisusAssert(!query->buffer.valid());

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

  query->end_resolution = query->end_resolutions.front();
  query->setRunning();
}

///////////////////////////////////////////////////////////////////////////////////////
bool Dataset::executePointQuery(SharedPtr<Access> access, SharedPtr<PointQuery> query)
{
  if (!query)
    return false;

  auto pdim = this->getPointDim();

  VisusReleaseAssert(!blocksFullRes());//todo

  if (!(query->isRunning() && query->getCurrentResolution() < query->getEndResolution()))
    return false;

  if (query->aborted())
  {
    query->setFailed("query aborted");
    return false;
  }

  if (!access)
    return executePointQueryOnServer(query);

  if (query->buffer.dims != query->getNumberOfPoints())
  {
    //solve the problem of missing blocks
    if (query->buffer.valid())
    {
      query->buffer = ArrayUtils::resample(query->getNumberOfPoints(), query->buffer);
      if (!query->buffer.valid())
      {
        query->setFailed("out of memory");
        return false;
      }
    }
    else
    {
      if (!query->buffer.resize(query->getNumberOfPoints(), query->field.dtype, __FILE__, __LINE__))
      {
        query->setFailed("out of memory");
        return false;
      }

      query->buffer.fillWithValue(query->field.default_value);
    }
  }

  VisusAssert(query->buffer.dtype == query->field.dtype);
  VisusAssert(query->buffer.c_size() == query->getByteSize());
  VisusAssert(query->buffer.dims == query->getNumberOfPoints());
  VisusAssert((Int64)query->points->c_size() == query->getNumberOfPoints().innerProduct() * sizeof(Int64) * pdim);

  auto blocks = createBlockQueriesForPointQuery(query);
  if (query->aborted() || blocks.empty())
    return false;

  //rehentrant call...(just to not close the file too soon)
  bool bEndIO = false;
  if (query->mode == 'w')
  {
    if (!access->isWriting())
    {
      bEndIO = true;
      access->beginWrite();
    }
  }
  else
  {
    if (!access->isReading())
    {
      bEndIO = true;
      access->beginRead();
    }
  }

  int nread = 0, nwrite = 0;

  //512*32kb=16MB (32KB IS A reasonable block size)
  WaitAsync< Future<Void> > wait_async(/*max_running*/512);
  for (auto blockid : blocks)
  {
    auto block_query = createBlockQuery(blockid, query->field, query->time, query->mode, query->aborted);

    if (query->mode=='r')
    {
      ++nread;
      executeBlockQuery(access, block_query);
      wait_async.pushRunning(block_query->done,[this, query, block_query](Void) {
        mergePointQueryWithBlockQuery(query, block_query);
      });
    }
    else
    {
      VisusReleaseAssert(false);//todo...
    }
  }

  if (bEndIO) 
    access->endIO();
  
  wait_async.waitAllDone();

  //PrintInfo("aysnc read",concatenate(nread, "/", block_queries.size()),"...");
  //PrintInfo("Query finished", "nread", nread, "nwrite", nwrite);

  //set the query status
  if (query->aborted()) {
    query->setFailed("query aborted");
    return false;
  }

  query->setCurrentResolution(query->end_resolution);
  return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
void Dataset::nextPointQuery(SharedPtr<PointQuery> query)
{
  VisusAssert(!blocksFullRes());

  if (!query)
    return;

  if (!(query->isRunning() && query->getCurrentResolution() == query->getEndResolution()))
    return;

  //reached the end? 
  if (query->end_resolution == query->end_resolutions.back())
    return query->setOk();

  int index = Utils::find(query->end_resolutions, query->end_resolution);
  query->end_resolution = query->end_resolutions[index + 1];
}

//////////////////////////////////////////////////////////////////////////////////////////
bool Dataset::mergePointQueryWithBlockQuery(SharedPtr<PointQuery> query, SharedPtr<BlockQuery> block_query)
{
  if (query->aborted() || block_query->failed())
    return false;

  if (!convertBlockQueryToRowMajor(block_query))
    return false;

  InsertIntoPointQuery op;
  return NeedToCopySamples(op, query->field.dtype, this, query.get(), block_query.get());
}

/// ///////////////////////////////////////////////////////////////////////////
int Dataset::guessPointQueryEndResolution(Frustum logic_to_screen, Position logic_position)
{
  VisusAssert(!blocksFullRes());

  if (!logic_position.valid())
    return {};

  auto minh = getDefaultBitsPerBlock();
  auto maxh = getMaxResolution();
  auto endh = maxh;
  auto pdim = getPointDim();

  if (logic_to_screen.valid())
  {
    std::vector<Point3d> logic_points;
    std::vector<Point2d> screen_points;
    FrustumMap map(logic_to_screen);
    for (auto p : logic_position.getPoints())
    {
      auto logic_point = p.toPoint3();
      logic_points.push_back(logic_point);
      screen_points.push_back(map.projectPoint(logic_point));
    }

    // valerio's algorithm, find the final view dependent resolution (endh)
    // (the default endh is the maximum resolution available)
    BoxNi::Edge longest_edge;
    double longest_screen_distance = NumericLimits<double>::lowest();
    for (auto edge : BoxNi::getEdges(pdim))
    {
      double screen_distance = (screen_points[edge.index1] - screen_points[edge.index0]).module();

      if (screen_distance > longest_screen_distance)
      {
        longest_edge = edge;
        longest_screen_distance = screen_distance;
      }
    }

    //I match the highest resolution on dataset axis (it's just an euristic!)
    for (int A = 0; A < pdim; A++)
    {
      double logic_distance = fabs(logic_points[longest_edge.index0][A] - logic_points[longest_edge.index1][A]);
      double samples_per_pixel = logic_distance / longest_screen_distance;
      Int64  num = Utils::getPowerOf2((Int64)samples_per_pixel);
      while (num > samples_per_pixel)
        num >>= 1;

      int H = maxh;
      for (; num > 1 && H >= 0; H--)
      {
        if (bitmask[H] == A)
          num >>= 1;
      }

      endh = std::min(endh, H);
    }
  }

  return Utils::clamp(endh, minh, maxh);
}

/// ///////////////////////////////////////////////////////////////////////////
PointNi Dataset::guessPointQueryNumberOfSamples(Frustum logic_to_screen, Position logic_position, int end_resolution)
{
  VisusAssert(!blocksFullRes());

  //*********************************************************************
  // valerio's algorithm, find the final view dependent resolution (endh)
  // (the default endh is the maximum resolution available)
  //*********************************************************************

  auto bitmask = this->idxfile.bitmask;
  int pdim = bitmask.getPointDim();

  if (!logic_position.valid())
    return PointNi(pdim);

  const int unit_box_edges[12][2] =
  {
    {0,1}, {1,2}, {2,3}, {3,0},
    {4,5}, {5,6}, {6,7}, {7,4},
    {0,4}, {1,5}, {2,6}, {3,7}
  };

  std::vector<Point3d> logic_points;
  for (auto p : logic_position.getPoints())
    logic_points.push_back(p.toPoint3());

  std::vector<Point2d> screen_points;
  if (logic_to_screen.valid())
  {
    FrustumMap map(logic_to_screen);
    for (int I = 0; I < 8; I++)
      screen_points.push_back(map.projectPoint(logic_points[I]));
  }

  PointNi virtual_worlddim = PointNi::one(pdim);
  for (int H = 1; H <= end_resolution; H++)
  {
    int bit = bitmask[H];
    virtual_worlddim[bit] <<= 1;
  }

  PointNi nsamples = PointNi::one(pdim);
  for (int E = 0; E < 12; E++)
  {
    int query_axis = (E >= 8) ? 2 : (E & 1 ? 1 : 0);
    Point3d P1 = logic_points[unit_box_edges[E][0]];
    Point3d P2 = logic_points[unit_box_edges[E][1]];
    Point3d edge_size = (P2 - P1).abs();

    PointNi idx_size = this->getLogicBox().size();

    // need to project onto IJK  axis
    // I'm using this formula: x/virtual_worlddim[dataset_axis] = factor = edge_size[dataset_axis]/idx_size[dataset_axis]
    for (int dataset_axis = 0; dataset_axis < 3; dataset_axis++)
    {
      double factor = (double)edge_size[dataset_axis] / (double)idx_size[dataset_axis];
      Int64 x = (Int64)(virtual_worlddim[dataset_axis] * factor);
      nsamples[query_axis] = std::max(nsamples[query_axis], x);
    }
  }

  //view dependent, limit the nsamples to what the user can see on the screen!
  if (!screen_points.empty())
  {
    PointNi view_dependent_dims = PointNi::one(pdim);
    for (int E = 0; E < 12; E++)
    {
      int query_axis = (E >= 8) ? 2 : (E & 1 ? 1 : 0);
      Point2d p1 = screen_points[unit_box_edges[E][0]];
      Point2d p2 = screen_points[unit_box_edges[E][1]];
      double pixel_distance_on_screen = (p2 - p1).module();
      view_dependent_dims[query_axis] = std::max(view_dependent_dims[query_axis], (Int64)pixel_distance_on_screen);
    }

    nsamples[0] = std::min(view_dependent_dims[0], nsamples[0]);
    nsamples[1] = std::min(view_dependent_dims[1], nsamples[1]);
    nsamples[2] = std::min(view_dependent_dims[2], nsamples[2]);
  }

  //important
  nsamples = nsamples.compactDims();
  nsamples.setPointDim(3, 1);

  return nsamples;
}

//////////////////////////////////////////////////////////////////////////////////////////
NetRequest Dataset::createPointQueryRequest(SharedPtr<PointQuery> query)
{
  Url url = this->getUrl();

  //I may have some extra params I want to keep!
  auto request=NetRequest(url.withPath("/mod_visus"));

  request.url.setParam("action", "pointquery");
  request.url.setParam("dataset", url.getParam("dataset"));
  request.url.setParam("time", url.getParam("time", cstring(query->time)));
  request.url.setParam("compression", url.getParam("compression", "zip")); //for networking I prefer to use zip
  request.url.setParam("field", query->field.name);
  request.url.setParam("fromh", cstring(0)); //backward compatible
  request.url.setParam("toh", cstring(query->end_resolution));
  request.url.setParam("maxh", cstring(getMaxResolution())); //backward compatible
  request.url.setParam("matrix", query->logic_position.getTransformation().toString());
  request.url.setParam("box", query->logic_position.getBoxNd().toBox3().toString(/*bInterleave*/false));
  request.url.setParam("nsamples", query->getNumberOfPoints().toString());

  request.aborted = query->aborted;

  return request;
}

//////////////////////////////////////////////////////////////////////////////////////////
bool Dataset::executePointQueryOnServer(SharedPtr<PointQuery> query)
{
  auto request = createPointQueryRequest(query);
  if (!request.valid())
  {
    query->setFailed("cannot create point query request");
    return false;
  }

  PrintInfo(request.url);
  auto response = NetService::getNetResponse(request);
  if (!response.isSuccessful())
  {
    query->setFailed(cstring("network request failed ", cnamed("errormsg", response.getErrorMessage())));
    return false;
  }

  auto decoded = response.getCompatibleArrayBody(query->getNumberOfPoints(), query->field.dtype);
  if (!decoded.valid()) {
    query->setFailed("failed to decode body");
    return false;
  }

  query->buffer = decoded;

  if (query->aborted()) {
    query->setFailed("query aborted");
    return false;
  }

  query->cur_resolution = query->end_resolution;
  return true;
}




///////////////////////////////////////////////////////////////////////////////////
BoxNi Dataset::adjustBoxQueryFilterBox(BoxQuery* query, IdxFilter* filter, BoxNi user_box, int H)
{
  VisusAssert(!blocksFullRes());

  //there are some case when I need alignment with pow2 box, for example when doing kdquery=box with filters
  auto bitmask = idxfile.bitmask;
  int pdim = bitmask.getPointDim();

  PointNi delta = this->level_samples[H].delta;

  BoxNi domain = query->filter.domain;

  //important! for the filter alignment
  BoxNi box = user_box.getIntersection(domain);

  if (!box.isFullDim())
    return box;

  PointNi filterstep = filter->getFilterStep(H);

  for (int D = 0; D < pdim; D++)
  {
    //what is the world step of the filter at the current resolution
    Int64 FILTERSTEP = filterstep[D];

    //means only one sample so no alignment
    if (FILTERSTEP == 1)
      continue;

    box.p1[D] = Utils::alignLeft(box.p1[D], (Int64)0, FILTERSTEP);
    box.p2[D] = Utils::alignLeft(box.p2[D] - 1, (Int64)0, FILTERSTEP) + FILTERSTEP;
  }

  //since I've modified the box I need to do the intersection with the box again
  //important: this intersection can cause a misalignment, but applyToQuery will handle it (see comments)
  box = box.getIntersection(domain);
  return box;
}

///////////////////////////////////////////////////////////////////////////////
bool Dataset::computeFilter(SharedPtr<IdxFilter> filter, double time, Field field, SharedPtr<Access> access, PointNi SlidingWindow, bool bVerbose)
{
  VisusAssert(!blocksFullRes());

  //this works only for filter_size==2, otherwise the building of the sliding_window is very difficult
  VisusAssert(filter->size == 2);

  DatasetBitmask bitmask = this->idxfile.bitmask;
  BoxNi          box = this->getLogicBox();

  int pdim = bitmask.getPointDim();

  //the window size must be multiple of 2, otherwise I loose the filter alignment
  for (int D = 0; D < pdim; D++)
  {
    Int64 size = SlidingWindow[D];
    VisusAssert(size == 1 || (size / 2) * 2 == size);
  }

  //convert the dataset  FINE TO COARSE, this can be really time consuming!!!!
  for (int H = this->getMaxResolution(); H >= 1; H--)
  {
    if (bVerbose)
      PrintInfo("Applying filter to dataset resolution", H);

    int bit = bitmask[H];

    Int64 FILTERSTEP = filter->getFilterStep(H)[bit];

    //need to align the from so that the first sample is filter-aligned
    PointNi From = box.p1;

    if (!Utils::isAligned(From[bit], (Int64)0, FILTERSTEP))
      From[bit] = Utils::alignLeft(From[bit], (Int64)0, FILTERSTEP) + FILTERSTEP;

    PointNi To = box.p2;
    for (auto P = ForEachPoint(From, To, SlidingWindow); !P.end(); P.next())
    {
      //this is the sliding window
      BoxNi sliding_window(P.pos, P.pos + SlidingWindow);

      //important! crop to the stored world box to be sure that the alignment with the filter is correct!
      sliding_window = sliding_window.getIntersection(box);

      //no valid box since do not intersect with box 
      if (!sliding_window.isFullDim())
        continue;

      //I'm sure that since the From is filter-aligned, then P must be already aligned
      VisusAssert(Utils::isAligned(sliding_window.p1[bit], (Int64)0, FILTERSTEP));

      //important, i'm not using adjustBox because I'm sure it is already correct!
      auto read = createBoxQuery(sliding_window, field, time, 'r');
      read->setResolutionRange(0, H);

      beginBoxQuery(read);

      if (!executeBoxQuery(access, read))
        return false;

      //if you want to debug step by step...
#if 0
      {
        PrintInfo("Before");
        int nx = (int)read->buffer->dims.x;
        int ny = (int)read->buffer->dims.y;
        Uint8* SRC = read->buffer->c_ptr();
        std::ostringstream out;
        for (int Y = 0; Y < ny; Y++)
        {
          for (int X = 0; X < nx; X++)
          {
            out << std::setw(3) << (int)SRC[((ny - Y - 1) * nx + X) * 2] << " ";
          }
          out << std::endl;
        }
        out << std::endl;
        PrintInfo("\n" << out.str();
      }
#endif

      filter->internalComputeFilter(read.get(),/*bInverse*/false);

      //if you want to debug step by step...
#if 0 
      {
        PrintInfo("After");
        int nx = (int)read->buffer->dims.x;
        int ny = (int)read->buffer->dims.y;
        Uint8* SRC = read->buffer->c_ptr();
        std::ostringstream out;
        for (int Y = 0; Y < ny; Y++)
        {
          for (int X = 0; X < nx; X++)
          {
            out << std::setw(3) << (int)SRC[((ny - Y - 1) * nx + X) * 2] << " ";
          }
          out << std::endl;
        }
        out << std::endl;
        PrintInfo("\n", out.str());
      }
#endif

      auto write = createBoxQuery(sliding_window, field, time, 'w');
      write->setResolutionRange(0, H);

      beginBoxQuery(write);

      if (!write->isRunning())
        return false;

      write->buffer = read->buffer;

      if (!executeBoxQuery(access, write))
        return false;
    }

    //I'm going to write the next resolution, double the dimension along the current axis
    //in this way I have the same number of samples!
    SlidingWindow[bit] <<= 1;
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////
void Dataset::computeFilter(const Field& field, int window_size, bool bVerbose)
{
  VisusAssert(!blocksFullRes());

  if (bVerbose)
    PrintInfo("starting filter computation...");

  auto filter = createFilter(field);

  //the filter will be applied using this sliding window
  PointNi sliding_box = PointNi::one(getPointDim());
  for (int D = 0; D < getPointDim(); D++)
    sliding_box[D] = window_size;

  auto acess = createAccess();
  for (auto time : getTimesteps().asVector())
    computeFilter(filter, time, field, acess, sliding_box, bVerbose);
}

///////////////////////////////////////////////////////////////////////////////////////
bool Dataset::executeBlockQuerWithFilters(SharedPtr<Access> access, SharedPtr<BoxQuery> query, SharedPtr<IdxFilter> filter)
{
  VisusAssert(!blocksFullRes());
  VisusAssert(filter);
  VisusAssert(query->mode == 'r');

  int cur_resolution = query->getCurrentResolution();
  int end_resolution = query->end_resolution;

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




/// ////////////////////////////////////////////////////////////////
void Dataset::readDatasetFromArchive(Archive& ar)
{
  IdxFile idxfile;
  ar.readObject("idxfile", idxfile);

  String url = ar.readString("url");
  idxfile.validate(url);

  this->dataset_body = ar;
  this->kdquery_mode = KdQueryMode::fromString(ar.readString("kdquery", Url(url).getParam("kdquery")));
  this->idxfile = idxfile;
  this->bitmask = idxfile.bitmask;
  this->default_bitsperblock = idxfile.bitsperblock;
  this->logic_box = idxfile.logic_box;
  this->timesteps = idxfile.timesteps;
  this->missing_blocks = idxfile.missing_blocks;

  setDatasetBounds(idxfile.bounds);

  //idxfile.fields -> Dataset::fields 
  if (this->fields.empty())
  {
    for (auto field : idxfile.fields)
    {
      if (field.name != "__fake__")
        addField(field);
    }
  }

  //create samples for levels and blocks
  {
    //bitmask = DatasetBitmask::fromString("V0011");
    //bitsperblock = 2;

    int bitsperblock = getDefaultBitsPerBlock();
    int pdim = bitmask.getPointDim();
    auto MaxH = bitmask.getMaxResolution();

    level_samples.clear(); level_samples.push_back(LogicSamples(bitmask.getPow2Box(), bitmask.getPow2Dims()));
    block_samples.clear(); block_samples.push_back(LogicSamples(bitmask.getPow2Box(), bitmask.getPow2Dims()));

    for (int H = 1; H <= MaxH; H++)
    {
      BoxNi logic_box;
      auto delta = PointNi::one(pdim);
      auto block_nsamples = PointNi::one(pdim);
      auto level_nsamples = PointNi::one(pdim);

      if (blocksFullRes())
      {
        //goint right to left (0,H] counting the bits
        for (int K = H; K > 0; K--)
          level_nsamples[bitmask[K]] *= 2;

        //delta.go from right to left up to the free '0' excluded
        for (int K = MaxH; K > H; K--)
          delta[bitmask[K]] *= 2;

        //block_nsamples. go from free '0' included to left up to bitsperblock
				for (int K = H, I = 0; I < bitsperblock && K>0; I++, K--)
					block_nsamples[bitmask[K]] *= 2;

        //logic_box
        logic_box = bitmask.getPow2Box();
      }
      else
      {
        HzOrder hzorder(bitmask);

        //goint right to left (0,H-1] counting the bits
        for (int K = H-1; K > 0; K--)
          level_nsamples[bitmask[K]] *= 2;

        //compute delta. go from right to left up to the blocking '1' included
        for (int K = MaxH; K >= H; K--)
          delta[bitmask[K]] *= 2;

        //block_nsamples. go from blocking '1' excluded to left up to bitsperblock
        for (int K = H-1, I = 0; I < bitsperblock && K>0; I++, K--)
          block_nsamples[bitmask[K]] *= 2;

				logic_box = BoxNi(hzorder.getLevelP1(H), hzorder.getLevelP2Included(H) + delta);
      }

      level_samples.push_back(LogicSamples(logic_box, delta)); 
      block_samples.push_back(LogicSamples(BoxNi(PointNi::zero(pdim), block_nsamples.innerMultiply(delta)), delta));

      VisusReleaseAssert(level_samples.back().nsamples==level_nsamples);


#if 0
      if (H >= bitsperblock)
      {
        PrintInfo("H", H,
          "level_samples[H].delta", level_samples[H].delta,
          "block_samples[H].nsamples", block_samples[H].nsamples);
      }
#endif
    }
  }
}

} //namespace Visus 
