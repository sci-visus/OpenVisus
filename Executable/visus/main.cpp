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
#include <Visus/Access.h>
#include <Visus/IdxFile.h>
#include <Visus/DatasetFilter.h>
#include <Visus/CloudStorage.h>
#include <Visus/IdxDataset.h>
#include <Visus/File.h>
#include <Visus/Encoders.h>
#include <Visus/Idx.h>
#include <Visus/Array.h>
#include <Visus/ModVisus.h>
#include <Visus/Path.h>
#include <Visus/ThreadPool.h>
#include <Visus/NetService.h>
#include <Visus/Utils.h>
#include <Visus/ApplicationInfo.h>
#include <Visus/IdxDiskAccess.h>
#include <Visus/IdxMosaicAccess.h>
#include <Visus/IdxMultipleDataset.h>
#include <Visus/MultiplexAccess.h>

#include <Visus/PythonEngine.h>
#include <pydebug.h>

using namespace Visus;

void execTestIdx(int max_seconds);

///////////////////////////////////////////////////////////
class ConvertStep
{
public:


  //destructor
  virtual ~ConvertStep()
  {}

  //getHelp
  virtual String getHelp(std::vector<String> args)=0;

  //exec
  virtual Array exec(Array data,std::vector<String> args)=0;


};

///////////////////////////////////////////////////////////
class CreateIdx : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out<<args[0]
      << " <filename.idx>" << std::endl
      << "   [--box <NdBox>]" << std::endl
      << "   [--fields <string>]" << std::endl
      << "   [--bitmask <string>]" << std::endl
      << "   [--bitsperblock <int>]" << std::endl
      << "   [--blocksperfile <int>]" << std::endl
      << "   [--filename_template <string>]" << std::endl
      << "   [--time from to template]" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() < 2)
      ThrowException(StringUtils::format() << args[0] <<"  syntax error");

    String filename = args[1];

    IdxFile idxfile;

    for (int I = 2; I < (int)args.size(); I++)
    {
      if (args[I] == "--box")
      {
        auto sbox = args[++I];
        int pdim = (int)StringUtils::split(sbox, " ", true).size() / 2; VisusAssert(pdim > 0);
        idxfile.box = NdBox::parseFromOldFormatString(pdim, sbox);
      }

      else if (args[I] == "--fields")
        idxfile.fields = IdxFile::parseFields(args[++I]);

      else if (args[I] == "--bits" || args[I] == "--bitmask_pattern" || args[I] == "--bitmask")
        idxfile.bitmask = DatasetBitmask(args[++I]);

      else if (args[I] == "--bitsperblock")
        idxfile.bitsperblock = cint(args[++I]);

      else if (args[I] == "--blocksperfile")
        idxfile.blocksperfile = cint(args[++I]);

      else if (args[I] == "--filename_template")
        idxfile.filename_template = args[++I];

      else if (args[I] == "--time")
      {
        idxfile.timesteps = DatasetTimesteps();
        String from = args[++I];
        String to = args[++I];
        String templ = args[++I];

        if (from == "*" && to == "*")
          idxfile.timesteps = DatasetTimesteps::star();
        else
          idxfile.timesteps.addTimesteps(cdouble(from), cdouble(to), 1.0);

        idxfile.time_template = templ;
      }
      else
      {
        //just ignore
      }
    }

    if (!IdxDataset::create(filename, data, idxfile))
    {
      VisusError() << "IdxDataset::create(" << filename << ") failed";
      return Array();
    }

    return data;
  }
};

///////////////////////////////////////////////////////////////////////
class StartVisusServer : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out <<args[0]
      << " [--port value]" << std::endl
      << "Example: " << args[0] << " --port 10000 ";
    return out.str();
  }

  //exec
  virtual Array exec(Array data,std::vector<String> args) override
  {
    int    port=10000;

    for (int I=1;I<(int)args.size();I++)
    {
      if (args[I]=="--port" || args[I]=="-p") 
      {
        port=cint(args[++I]);
        continue;
      }
      
      ThrowException(StringUtils::format() << args[0] <<"  Invalid argument " << args[I]);
    }

    auto modvisus=std::make_shared<ModVisus>();
    modvisus->configureDatasets();
  
    auto netserver=std::make_shared<NetServer>(port, modvisus);
    netserver->runInThisThread();

    return data;
  }

};

///////////////////////////////////////////////////////////
class ApplyFilters : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <filename>" << std::endl
      << "   [--window-size <int>]" << std::endl
      << "   [--field <field>]" << std::endl
      << "   [--time <time>]" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size()<2)
      ThrowException(StringUtils::format() << args[0] <<"  syntax error");

    String filename = args[1];

    auto dataset = Dataset::loadDataset(filename);
    if (!dataset)
      ThrowException(StringUtils::format() << args[0] <<"  Dataset::loadDataset(" << filename << ") failed");

    int window_size = 4096;
    double time = dataset->getDefaultTime();
    Field field = dataset->getDefaultField();

    int pdim = dataset->getPointDim();
    NdPoint sliding_box = NdPoint::one(pdim);

    for (int I = 2; I<(int)args.size(); I++)
    {
      if (args[I] == "--window-size")
      {
        window_size = cint(args[++I]);

        if (!Utils::isPowerOf2(window_size))
          ThrowException(StringUtils::format() << args[0] <<" window size must be power of two");

        continue;
      }

      if (args[I] == "--field")
      {
        String fieldname = args[++I];
        field = dataset->getFieldByName(fieldname);
        if (!field.valid())
          ThrowException(StringUtils::format() << args[0] <<"  Invalid --field " << fieldname);

        continue;
      }

      if (args[I] == "--time")
      {
        time = cdouble(args[++I]);
        continue;
      }
      
      ThrowException(StringUtils::format() << args[0] <<"  unknown argument: " << args[I]);
    }

    //the filter will be applied using this sliding window
    for (int D = 0; D<dataset->getPointDim(); D++)
      sliding_box[D] = window_size;

    auto access = dataset->createAccess();
    auto filter = dataset->createQueryFilter(field);
    VisusInfo() << "starting conversion...";
    filter->computeFilter(time, field, access, sliding_box);
    return data;
  }
};

///////////////////////////////////////////////////////////
class CopyDataset : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <filename.xml> OR" << std::endl
      << " <src_dataset> <dst_dataset>" << std::endl
      << "" << std::endl
      << ""
      << " XML file example (to specify access):" << std::endl
      << " <CopyDataset>" << std::endl
      << "   <src url='E:/google_sci/visus_dataset/2kbit1/lz4/rowmajor/visus.idx' />" << std::endl
      << "   <dst url='E:/google_sci/visus_dataset/2kbit1/lz4/rowmajor/visus.idx'  >" << std::endl
      << "     <access type = 'CloudStorageAccess' url='http://visus.s3.wasabisys.com/2kbit1?username=NOXCLNFH3Y64J3ET7Z4B&amp;password=XXXXXXX' />" << std::endl
      << "   </dst>" << std::endl
      << " </CopyDataset>" << std::endl;

    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    String Surl; StringTree Sconfig;
    String Durl; StringTree Dconfig;

    if (args.size() == 2)
    {
      //xml file with url and xml for access creation
      StringTree stree;
      if (!stree.loadFromXml(Utils::loadTextDocument(args[1])))
      {
        VisusAssert(false);
        return Array();
      }

      if (auto src = stree.findChildWithName("src")) {
        Surl = src->readString("url");
        if (auto access = src->findChildWithName("access"))
          Sconfig = *access;
      }

      if (auto dst = stree.findChildWithName("dst")) {
        Durl = dst->readString("url");
        if (auto access = dst->findChildWithName("access"))
          Dconfig = *access;
      }
    }
    else if (args.size() == 3)
    {
      //simply source url and dest url
      Surl = args[1]; Sconfig = StringTree();
      Durl = args[2]; Dconfig = StringTree();
    }
    else
    {
      ThrowException(StringUtils::format() << args[0] << " syntax error, wrong arguments");
    }

    auto Svf = Dataset::loadDataset(Surl);
    auto Dvf = Dataset::loadDataset(Durl);

    if (!Svf)
      ThrowException(StringUtils::format() << args[0] << "  Dataset::loadDataset(" << Surl << ") failed");

    if (!Dvf)
      ThrowException(StringUtils::format() << args[0] << "  Dataset::loadDataset(" << Durl << ") failed");

    if (Svf->getTimesteps() != Dvf->getTimesteps())
      ThrowException(StringUtils::format() << args[0] << " Time range not compatible");

    std::vector<double> timesteps = Svf->getTimesteps().asVector();

    std::vector<Field> Sfields = Svf->getFields();
    std::vector<Field> Dfields = Dvf->getFields();

    if (Sfields.size() != Dfields.size())
      ThrowException(StringUtils::format() << args[0] << " Fieldnames not compatible");

    auto Saccess = Svf->createAccessForBlockQuery(Sconfig);
    auto Daccess = Dvf->createAccessForBlockQuery(Dconfig);

    for (int time_id = 0; time_id<(int)timesteps.size(); time_id++)
    {
      double timestep = timesteps[time_id];
      for (int F = 0; F<(int)Sfields.size(); F++)
      {
        Dataset::copyDataset(
          Dvf.get(), Daccess, Dfields[F], timestep,
          Svf.get(), Saccess, Sfields[F], timestep);
      }
    }

    return data;
  }
};

///////////////////////////////////////////////////////////
class CompressDataset : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <dataset.xml> <compression>" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() != 3)
      ThrowException(StringUtils::format() << args[0] <<"  syntax error, needed 3 arguments");

    String url = args[1]; auto dataset = Dataset::loadDataset(url); 
    if (!dataset)  
      ThrowException(StringUtils::format() << args[0] <<"  Dataset::loadDataset(" << url << ") failed");

    String compression = args[2];
    if (!Encoders::getSingleton()->getEncoder(compression))
      ThrowException(StringUtils::format() << args[0] <<"  encoder(" << compression << ") does not exists");

    if (!dataset->compress(compression))
      ThrowException(StringUtils::format() << args[0] <<" Compression failed");

    return data;
  }
};

///////////////////////////////////////////////////////////
class FixDatasetRange : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <filename> " << std::endl
      << "   [--field <field>]" << std::endl
      << "   [--time <double>]" << std::endl
      << "   [--from <block_from>]" << std::endl
      << "   [--to <block_to>]" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size()<2)
      ThrowException(StringUtils::format() << args[0] <<"  syntax error, needed filename");

    VisusInfo() << "FixDatasetRange starting...";

    String filename = args[1];
    auto vf = IdxDataset::loadDataset(filename);
    if (!vf)
      ThrowException(StringUtils::format() << args[0] <<"  failed to read IDX dataset (Dataset::loadDataset(" << filename << ") failed)");

    auto idxfile = vf->idxfile;

    int maxh = vf->getMaxResolution();
    HzOrder hzorder(idxfile.bitmask, maxh);
    BigInt last_block = (hzorder.getAddress(hzorder.getLevelP2Included(maxh)) >> idxfile.bitsperblock) + 1;
    int    samplesperblock = 1 << idxfile.bitsperblock;

    auto access = vf->createAccessForBlockQuery();
    if (!access)
      ThrowException(StringUtils::format() << args[0] << " cannot create access (vf->createAccess() failed)");

    bool       filter_field = false; String field_filtered = "";
    bool       filter_time = false; double time_filtered = 0;
    BigInt     block_from = 0;
    BigInt     block_to = last_block;
    for (int I = 2; I<(int)args.size(); I++)
    {
      if (args[I] == "--field") { filter_field = true; field_filtered = (args[++I]); }
      else if (args[I] == "--time") { filter_time = true; time_filtered = cdouble(args[++I]); }
      else if (args[I] == "--from") { block_from = cint64(args[++I]); }
      else if (args[I] == "--to") { block_to = cint64(args[++I]); }
    }

    VisusInfo() << "Calculating minmax for " << (filter_field ? "field(" + field_filtered + ")" : "all fields");
    VisusInfo() << "Calculating minmax for " << (filter_time ? "time(" + cstring(time_filtered) + ")" : "all timesteps");
    VisusInfo() << "Calculating minmax in the block range [" << block_from << "," << block_to << ")";

    Time t1 = Time::now();

    Aborted aborted;

    access->beginRead();

    for (int F = 0; F<(int)idxfile.fields.size(); F++)
    {
      Field& field = idxfile.fields[F];

      if (filter_field && field.name != field_filtered)
      {
        VisusInfo() << "ignoring field(" << field.name << "), does not match with --field argument";
        continue;
      }

      //trivial case... don't want to read all the dataset!
      DType dtype = field.dtype;
      if (dtype.isVectorOf(DTypes::INT8))
      {
        VisusInfo() << "range for field(" << field.name << ") of type 'int8' quickly guessed (skipped the reading from disk)";
        continue;
      }

      if (dtype.isVectorOf(DTypes::UINT8))
      {
        VisusInfo() << "range for field(" << field.name << ") of type 'uint8' quickly guessed (skipped the reading from disk)";
        continue;
      }

      for (int C = 0; C<field.dtype.ncomponents(); C++)
      {
        auto sub = field.dtype.get(C);
        Range range = Range::invalid();

        if (sub == DTypes::INT8)
          range = Range(-128, 127,1);

        else if (sub == DTypes::UINT8)
          range = Range(0, 255,1);

        field.setDTypeRange(range, C);
      }

      std::vector<double> timesteps = vf->getTimesteps().asVector();
      for (int time_id = 0; time_id<(int)timesteps.size(); time_id++)
      {
        double time = timesteps[time_id];

        if (filter_time && time != time_filtered)
        {
          VisusInfo() << "ignoring timestep(" << time << ") ,does not match with --time argument";
          continue;
        }

        for (BigInt nblock = block_from; nblock<block_to; nblock++)
        {
          auto read_block = std::make_shared<BlockQuery>(field, time, access->getStartAddress(nblock), access->getEndAddress(nblock), aborted);
          if (!vf->readBlockAndWait(access, read_block))
            continue;

          //need to calculate since I already know it's invalid!
          for (int C = 0; C<field.dtype.ncomponents(); C++)
          {
            auto sub = field.dtype.get(C);

            if (sub == DTypes::INT8 || sub == DTypes::UINT8)
              continue;

            auto range = field.dtype.getDTypeRange(C);
            range = range.getUnion(ArrayUtils::computeRange(read_block->buffer, C));
            field.setDTypeRange(range, C);
          }

          //estimation
          if (t1.elapsedMsec()>5000)
          {
            VisusInfo()
              << "RANGE time(" << time << ")"
              << " field(" << field.name << ")"
              << " nblock/from/to(" << nblock << "/" << block_from << "/" << block_to << ")";

            for (int C = 0; C<field.dtype.ncomponents(); C++)
              VisusInfo() << "  what(" << C << ") " << field.dtype.getDTypeRange(C).toString();

            idxfile.fields[F] = field;

            //try to save the intermediate file (NOTE: internally using locks so it should be fine to run in parallel)
            if (!idxfile.save(filename))
            {
              VisusWarning() << "cannot save the INTERMEDIATE min-max in IDX dataset (vf->idxfile.save(" << filename << ") failed)";
              VisusWarning() << "Continuing anyway hoping to solve the problem saving the file later";
            }

            t1 = Time::now();
          }
        }
      }

      VisusInfo() << "done minmax for field(" << field.name << ")";

      for (int C = 0; C<field.dtype.ncomponents(); C++)
        VisusInfo() << "  what(" << C << ") " << field.dtype.getDTypeRange(C).toString();
    }

    access->endRead();

    //finally save the file
    if (!idxfile.save(filename))
      ThrowException(StringUtils::format() << args[0] <<"  cannot save the FINAL min-max in IDX dataset (IdxFile::save(" << filename << ") failed)");

    VisusInfo() << "done fixFieldsRange";

    for (auto field : vf->getFields())
      VisusInfo() << field.toString();

    return data;
  }
};

///////////////////////////////////////////////////////////////////////////////
class ConvertMidxToIdx : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <midx_filename> <idx_filename> [--tile-size value] [--field value]" << std::endl
      << "Example: " << args[0] << " D:/Google Drive sci.utah.edu/visus_dataset/slam/Alfalfa/visus.midx D:/Google Drive sci.utah.edu/visus_dataset/slam/Alfalfa/visus.idx";
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    srand(0);

    String midx_filename = args[1];
    String idx_filename = args[2];
    int TileSize = 4 * 1024;
    String fieldname = "output=voronoi()";

    for (int I = 1; I < args.size(); I++)
    {
      if (args[I] == "--tile-size")
        TileSize = cint(args[++I]);

      else if (args[I] == "--field")
        fieldname = args[++I];
    }

    auto midx = std::dynamic_pointer_cast<IdxMultipleDataset>(Dataset::loadDataset(midx_filename)); VisusReleaseAssert(midx);
    auto midx_access = midx->createAccess();

    bool bOk = midx->createIdxFile(idx_filename, Field("DATA", midx->getFieldByName(fieldname).dtype, "rowmajor"));
    VisusReleaseAssert(bOk);

    auto idx = IdxDataset::loadDataset(idx_filename);
    auto idx_access = idx->createAccess();

    auto tiles = midx->generateTiles(TileSize);

    auto T1 = Time::now();
    for (int TileId = 0; TileId<tiles.size(); TileId++)
    {
      auto tile = tiles[TileId];

      auto t1 = Time::now();
      auto buffer = midx->readMaxResolutionData(midx_access, midx->getFieldByName(fieldname), midx->getDefaultTime(), tile);
      int msec_read = (int)t1.elapsedMsec();
      if (!buffer)
        continue;

      t1 = Time::now();
      idx->writeMaxResolutionData(idx_access, tile, buffer);
      int msec_write = (int)t1.elapsedMsec();

      VisusInfo() << "done " << TileId << " of " << tiles.size() << " msec_read(" << msec_read << ") msec_write(" << msec_write << ")";

      //ArrayUtils::saveImage(StringUtils::format() << "tile_" << TileId << ".png", read->buffer);
    }

    VisusInfo() << "ALL DONE IN " << T1.elapsedMsec();
    return data;
  }

};


///////////////////////////////////////////////////////////
class ImportData : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <filename> " << std::endl
      << "   [load_args]*" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size()<2)
      ThrowException(StringUtils::format() << args[0] <<"  syntax error, needed filename");

    String filename = args[1];

    auto ret = ArrayUtils::loadImage(filename, args);
    if (!ret)
      ThrowException(StringUtils::format() << args[0] <<"  cannot load image " << filename);

    return ret;
  }
};

///////////////////////////////////////////////////////////
class ExportData : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      <<" <filename> [save_args]*" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size()<2)
      ThrowException(StringUtils::format() << args[0] <<"  syntax error");

    String filename = args[1];

    if (!ArrayUtils::saveImage(filename, data, args))
      ThrowException(StringUtils::format() << args[0] <<"  saveImage failed " << filename);

    return data;
  }
};

///////////////////////////////////////////////////////////
class PasteData : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <filename> " << std::endl
      << "   [--source-box      <NdBox>]" << std::endl
      << "   [--destination-box <NdBox>]" << std::endl
      << "   [load_args]*" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size()<2)
      ThrowException(StringUtils::format() << args[0]<< " syntax error");

    String filename = args[1];

    Array to_paste = ArrayUtils::loadImage(filename, args);
    if (!to_paste)
      ThrowException(StringUtils::format() << args[0] <<"  Cannot load " << filename);

    int pdim = data.getPointDim();

    //embedding in case I'm missing point-dims (see interleaving)
    if (pdim>to_paste.dims.getPointDim())
      to_paste.dims.setPointDim(pdim);

    NdBox Dbox(NdPoint(pdim), data.dims);
    NdBox Sbox(NdPoint(pdim), to_paste.dims);
    for (int I = 2; I<(int)args.size(); I++)
    {
      if (args[I] == "--destination-box")
        Dbox = NdBox::parseFromOldFormatString(pdim,args[++I]);

      else if (args[I] == "--source-box") {
        Sbox = NdBox::parseFromOldFormatString(pdim, args[++I]);
        if (pdim > Sbox.getPointDim()) {
          Sbox.p1.setPointDim(pdim);
          Sbox.p2.setPointDim(pdim);
        }
      }
    }

    if (!ArrayUtils::paste(data, Dbox, to_paste, Sbox))
      ThrowException(StringUtils::format() << args[0] <<" paste of image failed");

    return data;
  }
};

///////////////////////////////////////////////////////////
class GetComponent : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <expression>" << std::endl
      << "Example: " << args[0] << " 0";
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() != 2)
      ThrowException(StringUtils::format() << args[0] << " syntax error");

    int C = cint(args[1]);
    return data.getComponent(C);
  }
};

///////////////////////////////////////////////////////////
class Cast : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      <<" <dtype>" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() != 2)
      ThrowException(StringUtils::format() << args[0] << " syntax error");

    DType dtype = DType::fromString(args[1]);
    return ArrayUtils::cast(data, dtype);
  }
};

///////////////////////////////////////////////////////////
class SmartCast : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      <<" <dtype>" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() != 2)
      ThrowException(StringUtils::format() << args[0] << " syntax error");

    DType dtype = DType::fromString(args[1]);
    return ArrayUtils::smartCast(data, dtype);
  }
};

///////////////////////////////////////////////////////////
class ResizeData : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << "   [--dtype <DType>]" << std::endl
      << "   [--dims <NdPoint>]" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size()<2)
      ThrowException(StringUtils::format() << args[0] << " syntax error");

    DType   dtype = data.dtype;
    NdPoint dims = data.dims;
    for (int I = 1; I<(int)args.size(); I++)
    {
      if (args[I] == "--dtype")
      {
        dtype = DType::fromString(args[++I]);
        continue;
      }

      if (args[I] == "--dims")
      {
        dims = NdPoint::parseDims(args[++I]);
        continue;
      }
      
      ThrowException(StringUtils::format() << args[0] << " Invalid arguments " << args[I]);
    }

    if (!data.resize(dims, dtype, __FILE__, __LINE__))
      ThrowException(StringUtils::format() << args[0] << " resize failed");

    return data;
  }
};

///////////////////////////////////////////////////////////
class CropData : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <NdBox>" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() != 2)
      ThrowException(StringUtils::format() << args[0] << " syntax error");

    int pdim = data.getPointDim();
    String sbox = args[1];

    NdBox box = NdBox::parseFromOldFormatString(pdim,sbox);
    if (!box.isFullDim())
      ThrowException(StringUtils::format() << args[0] <<"  Invalid box " << sbox);

    return ArrayUtils::crop(data, box);
  }
};

///////////////////////////////////////////////////////////
class MirrorData : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <int>" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() != 2)
      ThrowException(StringUtils::format() << args[0] <<"  syntax error");

    int axis = cint(args[1]);
    if (axis<0)
      ThrowException(StringUtils::format() << args[0] <<"  Invalid axis " << args[1]);

    return ArrayUtils::mirror(data, axis);
  }
};

///////////////////////////////////////////////////////////
class ComputeComponentRange : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <expression>" << std::endl
      << "Example: " << args[0] << " 0";
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() != 2)
      ThrowException(StringUtils::format() << args[0] <<"  syntax error");

    int C = cint(args[1]);
    Range range = ArrayUtils::computeRange(data, C);
    VisusInfo() << "Range of component " << C << " is " << range.toString();

    return data;
  }
};

///////////////////////////////////////////////////////////
class InterleaveData : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() != 1)
      ThrowException(StringUtils::format() << args[0] <<"  syntax error");

    if (data.dtype.ncomponents()>1 && !Utils::isByteAligned(data.dtype.get(0).getBitSize()))
      ThrowException(StringUtils::format() << args[0] <<" request to --interleave but a sample is not byte aligned");

    //need to interleave (the input for example is RRRRR...GGGGG...BBBBB and I really need RGBRGBRGB)
    int ncomponents = data.dtype.ncomponents();
    if (ncomponents <= 1)
      return data;

    std::vector<Array> v;
    for (int I = 0; I<ncomponents; I++)
    {
      DType component_dtype = data.dtype.get(I);
      Int64 offset = I*component_dtype.getByteSize(data.dims);
      v.push_back(Array::createView(data, data.dims, component_dtype, offset));
    }

    return ArrayUtils::interleave(v);
  }
};

///////////////////////////////////////////////////////////
class PrintInfo : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <filename>";
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() != 2)
      ThrowException(StringUtils::format() << args[0] <<"  syntax error");

    String filename = args[1];
    StringTree info = ArrayUtils::statImage(filename);
    if (info.empty())
      ThrowException(StringUtils::format() << args[0] <<"  Could not open " << filename);

    VisusInfo() << std::endl << info.toString();

    auto dataset = Dataset::loadDataset(filename);
    return data;
  }
};

///////////////////////////////////////////////////////////
class DumpData : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() != 1)
      ThrowException(StringUtils::format() << args[0] <<"  syntax error");

    VisusInfo() << "Buffer dims(" << data.dims.toString() << ") dtype(" << data.dtype.toString() << ")";

    Uint8* SRC = data.c_ptr();
    Int64 N = data.c_size();
    std::ostringstream out;

    for (int I = 0; I<N; I++)
    {
      out << std::setfill('0') << std::hex << "0x" << std::setw(2) << (int)(*SRC++);
      if (I != N - 1) out << ",";
      if ((I % 16) == 15) out << std::endl;
    }
    out << std::endl;
    out << std::endl;
    VisusInfo() << "\n" << out.str();
    return data;
  }
};

///////////////////////////////////////////////////////////
class ReadWriteBlock : public ConvertStep
{
public:

  //constructor
  ReadWriteBlock(bool bWriting_) : bWriting(bWriting_) {
  }
  
  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <filename>" << std::endl
      << "   [--block <int>]" << std::endl
      << "   [--field <field>]" << std::endl
      << "   [--time <time>]" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size()<2)
      ThrowException(StringUtils::format() << args[0] <<"  syntax error");

    String url = args[1];

    auto dataset = Dataset::loadDataset(url);
    if (!dataset)
      ThrowException(StringUtils::format() << args[0] <<"  Dataset::loadDataset(" << url << ") failed");

    BigInt block_id = 0;
    Field  field = dataset->getDefaultField();
    double time = dataset->getDefaultTime();

    for (int I = 2; I<(int)args.size(); I++)
    {
      if (args[I] == "--block")
      {
        block_id = cbigint(args[++I]);
        continue;
      }

      if (args[I] == "--field")
      {
        String fieldname = args[++I];
        field = dataset->getFieldByName(fieldname);
        if (!field.valid())
          ThrowException(StringUtils::format() << args[0] <<"  specified field (" << fieldname << ") is wrong");
        continue;
      }

      if (args[I] == "--time")
      {
        double time = cdouble(args[++I]);
        if (!dataset->getTimesteps().containsTimestep(time))
          ThrowException(StringUtils::format() << args[0] <<"  specified time (" << time << ") is wrong");
        continue;
      }

      ThrowException(StringUtils::format() << args[0] <<"  Invalid argument " << args[I]);
    }

    VisusInfo() << "url(" << url << ") block(" << block_id << ") field(" << field.name << ") time(" << time << ")";

    auto access=dataset->createAccessForBlockQuery();

    auto block_query = std::make_shared<BlockQuery>(field, time, block_id *(((Int64)1) << access->bitsperblock), (block_id + 1)*(((Int64)1) << access->bitsperblock), Aborted());
    ApplicationStats::io.readValues(true);

    auto t1 = Time::now();

    Array ret;
    if (bWriting)
    {
      access->beginWrite();
      bool bOk = dataset->writeBlockAndWait(access, block_query);
      access->endWrite();
      if (!bOk)
        ThrowException(StringUtils::format() << args[0] <<" Failed to write block");
      ret=data;
    }
    else
    {
      access->beginRead();
      bool bOk = dataset->readBlockAndWait(access, block_query);
      access->endRead();
      if (!bOk)
        ThrowException(StringUtils::format() << args[0] <<" Failed to write block");
      ret=block_query->buffer;
    }


    auto stats=ApplicationStats::io.readValues(true);
    VisusInfo() << (bWriting?"Wrote":"Read")<< " block("<<block_id<< ") in msec(" << t1.elapsedMsec() << ") nopen(" << stats.nopen << ") rbytes(" << StringUtils::getStringFromByteSize(stats.rbytes) << ") wbytes(" << StringUtils::getStringFromByteSize(stats.wbytes) << ")";
    return ret;
  }

private:

  bool bWriting;
};





///////////////////////////////////////////////////////////
class CloudCopyBlob : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <url> <dst_url>" << std::endl
      << "Example: " << args[0] << " http://atlantis.sci.utah.edu/mod_visus?readdataset=2kbit1  http://visus.blob.core.windows.net/2kbit1/visus.idx?password=XXXXX";
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() != 3)
      ThrowException(StringUtils::format() << args[0] <<"  syntax error");

    String src_url = args[1];
    String dst_url = args[2];

    auto src_storage = CloudStorage::createInstance(src_url);
    auto dst_storage = CloudStorage::createInstance(dst_url);

    auto net = std::make_shared<NetService>(1);

    CloudStorage::Blob blob;
    if (src_storage)
    {
      auto blob_name = Url(src_url).getPath();
      blob = src_storage->getBlob(net, blob_name,Aborted()).get();
      if (!blob.valid())
        ThrowException(StringUtils::format() << args[0] <<"  Cloud Storage getBlob(" << src_url << ") failed");
    }
    else
    {
      auto body = Utils::loadBinaryDocument(src_url);
      if (!body)
        ThrowException(StringUtils::format() << args[0] <<"  Utils::loadBinaryDocument(" << src_url << ") failed");

      blob.body = body;
    }

    if (dst_storage)
    {
      auto blob_name = Url(dst_url).getPath();
      if (!dst_storage->addBlob(net, blob_name, blob,Aborted()).get())
        ThrowException(StringUtils::format() << args[0] <<"  Cloud Storage addBlob(" << dst_url << ") failed");
    }
    else if (!Utils::saveBinaryDocument(dst_url, blob.body))
    {
      ThrowException(StringUtils::format() << args[0] <<"  Utils::saveBinaryDocument(" << dst_url << ") failed");
    }

    return data;
  }
};


///////////////////////////////////////////////////////////
class CloudDeleteBlob : public ConvertStep
{
public:
  
  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <url>" << std::endl
      << "Example: " << args[0] << " http://visus.blob.core.windows.net/2kbit1/visus.idx?password=XXXXX";
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() != 2)
      ThrowException(StringUtils::format() << args[0] <<"  syntax error");

    auto net = std::make_shared<NetService>(1);

    Url url = args[1];
    auto cloud=CloudStorage::createInstance(url);
    if (!cloud->deleteBlob(net,url.getPath(),Aborted()).get())
      ThrowException(StringUtils::format() << args[0] <<"  cannot delete blob " << url.toString());
    return data;
  }
};

//////////////////////////////////////////////////////////////////////////////
class CloudSelfTest : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " url" << std::endl
      << "Example: " << args[0] << " http://visus.s3.amazonaws.com?username=AKIAILLJI7ANO6Y2XJNA&password=XXXXXXXXX"
      << "Example: " << args[0] << " http://visus.blob.core.windows.net?access_key=XXXXXX"
      << "Example: " << args[0] << " https://www.googleapis.com?client_id=XXXX&client_secret=YYYY&refresh_token=ZZZZZZ";
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    Url url(args[1]);

    if (!url.valid())
      ThrowException(StringUtils::format() << args[0] << ", " << args[1] << " is not a valid url");

    auto net = std::make_shared<NetService>(1);

    auto cloud = CloudStorage::createInstance(url);

    //simple blob for testing
    auto blob = CloudStorage::Blob();
    blob.body = Utils::loadBinaryDocument("datasets/cat/gray.png"); VisusReleaseAssert(blob.body);
    blob.metadata.setValue("example-meta-data", "visus-meta-data");

    {
      String blob_name = "/testing-cloud-storage/my/blob/name/visus.png";
      bool bOk = cloud->addBlob(net, blob_name, blob, Aborted()).get();
      VisusReleaseAssert(bOk);

      auto check_blob = cloud->getBlob(net, blob_name, Aborted()).get();
      
      bOk=cloud->deleteBlob(net, blob_name, Aborted()).get();
      VisusReleaseAssert(bOk);
    }

    Int64 MSEC_ADD = 0, MSEC_GET = 0;
    for (int I = 0; ; I++)
    {
      String blob_name = StringUtils::format() << "/testing-cloud-storage/speed/" << "blob."<< std::setfill('0') << std::setw(4) << I <<".bin";
      auto t1 = Time::now();
      bool bOk=cloud->addBlob(net, blob_name, blob, Aborted()).get();
      auto msec_add = t1.elapsedMsec(); MSEC_ADD += msec_add;
      VisusReleaseAssert(bOk);

      t1 = Time::now();
      auto check_blob=cloud->getBlob(net, blob_name).get();
      auto msec_get = t1.elapsedMsec(); MSEC_GET += msec_get;

      check_blob.content_type = blob.content_type; //google changes the content_type
      VisusReleaseAssert(check_blob == blob);

      VisusInfo() << "Average msec_add(" << (MSEC_ADD / (I+1)) << ") msec_get(" << (MSEC_GET / (I + 1)) << ")";
    }

    return data;
  }
};




///////////////////////////////////////////////////////////
class TestEncoderSpeed : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out<<args[0]
      << " <dataset> <compression_algorithm>" << std::endl
      << "Example: " << args[0]<< " F:/visus_dataset/2kbit1/hzorder/visus.idx zip";
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() != 3)
      ThrowException(StringUtils::format() << args[0] << "  syntax error");

    String filename = args[1];
    auto dataset = Dataset::loadDataset(filename);

    String compression = args[2];

    if (!Encoders::getSingleton()->getEncoder(compression))
      ThrowException(StringUtils::format() << args[0] << "  failed to create encoder " << compression << "");

    if (!dataset)
      ThrowException(StringUtils::format() << args[0] << "  failed to read dataset (Dataset::loadDataset(" << filename << ") failed)");

    auto access = dataset->createAccessForBlockQuery();
    auto field  = dataset->getDefaultField();
    auto time   = dataset->getDefaultTime();

    Time T1 = Time::now(), t1;
    BigInt decoded_bytes = 0; double decode_sec = 0;
    BigInt encoded_bytes = 0; double encode_sec = 0;

    Aborted aborted;
    access->beginRead();

    for (BigInt block_id = 0, nblocks = dataset->getTotalnumberOfBlocks(); ; block_id = (block_id + 1) % nblocks)
    { 
      auto read_block = std::make_shared<BlockQuery>(field, time, access->getStartAddress(block_id), access->getEndAddress(block_id), aborted);

      if (!dataset->readBlockAndWait(access, read_block))
        continue;

      auto block = read_block->buffer;

      t1 = Time::now();
      auto encoded = ArrayUtils::encodeArray(compression, block); VisusReleaseAssert(encoded);
      encode_sec += t1.elapsedSec();
      encoded_bytes += encoded->c_size();

      t1 = Time::now();
      auto decoded = ArrayUtils::decodeArray(compression, block.dims, block.dtype, encoded);VisusReleaseAssert(decoded);
      decode_sec += t1.elapsedSec();
      decoded_bytes += decoded.c_size();

      VisusReleaseAssert(block.c_size() == decoded.c_size());
      VisusReleaseAssert(memcmp(block.c_ptr(), decoded.c_ptr(), block.c_size()) == 0);

      if (T1.elapsedSec()>5)
      {
        auto encoded_mb = encoded_bytes / (1024 * 1024);
        auto decoded_mb = decoded_bytes / (1024 * 1024);
        auto ratio = 100.0*(encoded_mb / (double)decoded_mb);

        VisusInfo() << "Ratio " << ratio <<"%";
        VisusInfo() << "Encoding MByte(" << encoded_mb << ") sec(" << encode_sec << ") MByte/sec(" << (encoded_mb / encode_sec) << ")";
        VisusInfo() << "Decoding MByte(" << decoded_mb << ") sec(" << decode_sec << ") MByte/sec(" << (decoded_mb / decode_sec) << ")";
        VisusInfo();

        T1 = Time::now();
        //decoded_bytes = 0; decode_sec = 0;
        //encoded_bytes = 0; encode_sec = 0;
      }
    }

    access->endRead();

    return data;
  }
};

///////////////////////////////////////////////////////////////////////
class TestQuerySpeed : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <filename> [--query-dim value]" << std::endl
      << "Example: " << args[0] << " C:/free/visus_dataset/2kbit1/zip/hzorder/visus.idx --query-dim 512";
    return out.str();
  }
  
  //exec
  virtual Array exec(Array data,std::vector<String> args) override
  {
    if (args.size() < 2)
      ThrowException(StringUtils::format() << args[0] << " syntax error");

    String filename = args[1];
    auto dataset = Dataset::loadDataset(filename);
    if (!dataset)
      ThrowException(StringUtils::format() << args[0] << "  cannot loadDataset " << filename);

    int    query_dim=512;

    for (int I=2;I<(int)args.size();I++)
    {
      if (args[I]=="--query-dim")
      {
        String s_query_dim = args[++I];

        if (!StringUtils::tryParse(s_query_dim,query_dim) || query_dim<=0)
          ThrowException(StringUtils::format() << args[0] <<"  Invalid --query-dim " << s_query_dim);

        continue;
      }

      ThrowException(StringUtils::format() << args[0] <<" Invalid args "<<args[I]);
    }

    srand((unsigned int)Time::now().getTimeStamp());

    VisusInfo() << "Testing query...";

    auto access = dataset->createAccess();
    auto world_box = dataset->getBox();

    Time T1 = Time::now();
    for (int nqueries = 0;;nqueries++)
    {
      Time t1 = Time::now();

      auto ndbox = world_box;

      for (int I = 0; I < dataset->getPointDim(); I++)
      {
        ndbox.p1[I] = Utils::getRandInteger(0, (int)world_box.p2[I] - query_dim);
        ndbox.p2[I] = ndbox.p1[I] + query_dim;
      }

      auto query = std::make_shared<Query>(dataset.get(), 'r');
      query->position = ndbox;

      VisusReleaseAssert(dataset->beginQuery(query));
      VisusReleaseAssert(dataset->executeQuery(access, query));

      auto sec = t1.elapsedSec();
      auto stats = access ? access->statistics : Access::Statistics();
      auto io = ApplicationStats::io.readValues(true);

      VisusInfo() << "sec(" << sec << ")"
        << " avg(" << (T1.elapsedSec() / (nqueries+1)) << ")"
        << " box(" << ndbox.toString() << ") "
        << " access.rok(" << stats.rok << "/" << ((double)(stats.rok) / sec) << ") "
        << " access.rfail(" << stats.rfail << "/" << ((double)(stats.rfail) / sec) << ") "
        << " io.nopen(" << io.nopen << "/" << ((double)(io.nopen) / sec) << ") "
        << " io.rbytes(" << double(io.rbytes) / (1024 * 1024) << "kb/" << double(io.rbytes) / (sec * 1024 * 1024) << "kb) "
        << " io.wbytes(" << double(io.wbytes) / (1024 * 1024) << "kb/" << double(io.wbytes) / (sec * 1024 * 1024) << "kb) ";
    }


    return data;
  }
};

//////////////////////////////////////////////////////////////////////////////
class TestFileReadWriteSpeed : public ConvertStep
{
public:

  bool bWriting;

  //constructor
  TestFileReadWriteSpeed(bool bWriting_) : bWriting(bWriting_) {
  }

  //destructor
  virtual ~TestFileReadWriteSpeed() {
  }

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " [--filename value] [--blocksize value] [--filesize value]" << std::endl
      << "Example: " << args[0] << " --filename example.idx --blocksize 64KB --filesize 1MB";
    return out.str();
  }

  //exec
  virtual Array exec(Array data,std::vector<String> args) override
  {
    String filename="temp.bin";
    int blocksize=64*1024;
    int filesize=1*1024*1024*1024;
  
    for (int I=1;I<(int)args.size();I++)
    {
      if (args[I] == "--filename")
      {
        filename = args[++I];
        continue;
      }

      if (args[I] == "--blocksize")
      {
        blocksize = (int)StringUtils::getByteSizeFromString(args[++I]);
        continue;
      }

      if (args[I] == "--filesize")
      {
        filesize = (int)StringUtils::getByteSizeFromString(args[++I]);
        continue;
      }

      ThrowException(StringUtils::format() << "wrong argument "<<args[I]);
    }

    if (bWriting)
    {
      FileUtils::removeFile(filename);

      File file;
      if(!file.createAndOpen(filename,"w"))
        ThrowException(StringUtils::format() << args[0] <<" TestWriteIO, file.open"<<filename<<",\"wb\") failed");

      Array blockdata;
      bool bOk=blockdata.resize(blocksize,DTypes::UINT8,__FILE__,__LINE__);
      VisusReleaseAssert(bOk);

      Time t1=Time::now();
      int nwritten;
      for (nwritten=0;(nwritten+blocksize)<=filesize;nwritten+=blocksize)
      {
        if(!file.write(nwritten, blocksize, blockdata.c_ptr()))
          ThrowException(StringUtils::format() << args[0] <<" TestWriteIO write(...) failed");
      }
      file.close();
      double elapsed=t1.elapsedSec();
      int mbpersec=(int)(nwritten/(1024.0*1024.0*elapsed));
      //do not remove, I can need it for read
      //remove(filename.c_str());
      VisusInfo()<<"write("<<mbpersec<<" mb/sec) filesize("<<filesize<<") blocksize("<<blocksize<<") nwritten("<<nwritten<<") filename("<<filename<<")";
    }
    else
    {
      File file;
      if(!file.open(filename,"r"))
        ThrowException(StringUtils::format() << args[0] <<" file.open("<<filename<<",'r') failed");

      Array blockdata;
      bool bOk=blockdata.resize(blocksize,DTypes::UINT8,__FILE__,__LINE__);
      VisusReleaseAssert(bOk);
      Time t1=Time::now();
      Int64 nread=0;
      int maxcont=1000,cont=maxcont;
      while (true)
      {
        if(!file.read(nread, blocksize, blockdata.c_ptr()))
          break;

        nread+=blocksize;

        if (!--cont)
        {
          cont=maxcont;
          VisusInfo()<<"read("<<((nread/(1024*1024))/t1.elapsedSec())<<" mb/sec) blocksize("<<blocksize<<") nread("<<(nread/(1024*1024))<<" mb) filename("<<filename<<")";      
        }
      }
      file.close();
      VisusInfo()<<"read("<<((nread/(1024*1024))/t1.elapsedSec())<<" mb/sec) blocksize("<<blocksize<<") nread("<<(nread/(1024*1024))<<" mb) filename("<<filename<<")";  
    }

    return data;
  }
};

//////////////////////////////////////////////////////////////////////////////
class IdxSelfTest : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " [--max-seconds value]" << std::endl
      << "Example: " << args[0] << " --max-seconds 300";
    return out.str();
  }

  //exec
  virtual Array exec(Array data,std::vector<String> args) override
  {
    int max_seconds=300; 
    for (int I=1;I<(int)args.size();I++)
    {
      if (args[I] == "--max-seconds")
      {
        max_seconds = cint(args[++I]);
        continue;
      }

      ThrowException(StringUtils::format() << "wrong argument " << args[I]);
    }

    execTestIdx(max_seconds);
    return data;
  }
};

///////////////////////////////////////////////////////////////////////////////
class TestIdxSlabSpeed : public ConvertStep
{
public:

  //see https://github.com/sci-visus/visus/issues/126
  /* Hana numbers (NOTE hana is using row major):
    dims 512 x 512 x 512
    num_slabs 128
    dtype int32 
    Wrote all slabs in  8.337sec
  */

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " [--dims       <dimensions>]" << std::endl
      << " [--num-slabs  <value>]" << std::endl
      << " [--dtype      <dtype>]" << std::endl
      << " [--rowmajor   | --hzorder]"<<std::endl 
      << "Example: " << args[0] << " --dims \"512 512 512\" --num-slabs 128 --dtype int32";
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    String filename = StringUtils::format()<<"./temp/TestIdxSlabSpeed/visus.idx";

    auto dirname = Path(filename).getParent();
    if (FileUtils::existsDirectory(dirname))
    {
      ThrowException(StringUtils::format() << "Please empty the directory " << dirname.toString());
      return Array();
    }

    NdPoint dims            = NdPoint::one(512, 512, 512);
    int     num_slabs       = 128;
    String  dtype           = "int32";
    String  layout   = "";

    for (int I = 1; I<(int)args.size(); I++)
    {
      if (args[I] == "--dims")
      {
        dims = NdPoint::parseDims(args[++I]);
        continue;
      }

      if (args[I] == "--num-slabs") {
        num_slabs = cint(args[++I]);
        VisusReleaseAssert((dims[2] % num_slabs) == 0);
        continue;
      }

      if (args[I] == "--dtype")
      {
        dtype = args[++I];
        continue;
      }
      
      if (args[I] == "--rowmajor")
      {
        layout = "rowmajor";
        continue;
      }

      if (args[I] == "--hzorder")
      {
        layout = "hzorder";
        continue;
      }

      ThrowException(StringUtils::format()<<"Wrong argument "<<args[I]);
    }

    int slices_per_slab = (int)dims[2] / num_slabs;

    VisusInfo() << "--dims            " << dims.toString();
    VisusInfo() << "--num-slabs       " << num_slabs;
    VisusInfo() << "--dtype           " << dtype;
    VisusInfo() << "--slices-per-slab " << slices_per_slab;

    //create the idx file
    {
      IdxFile idxfile;
      idxfile.box = NdBox(NdPoint(3),dims);
      {
        Field field("myfield", DType::fromString(dtype));
        field.default_compression = ""; // no compression (in writing I should not use compression)
        field.default_layout = layout; 
        idxfile.fields.push_back(field);
      }
      VisusReleaseAssert(idxfile.save(filename));
    }

    //now create a Dataset, save it and reopen from disk
    auto dataset = Dataset::loadDataset(filename);
    VisusReleaseAssert(dataset && dataset->valid());

    //any time you need to read/write data from/to a Dataset I need a Access
    auto access = dataset->createAccess();

    //for example I want to write data by slices
    Int32 sample_id = 0;
    double SEC = 0;

    for (int Slab = 0; Slab < num_slabs; Slab++)
    {
      //this is the bounding box of the region I'm going to write
      auto Z1 = Slab * slices_per_slab;
      auto Z2 =   Z1 + slices_per_slab;

      NdBox slice_box = dataset->getBox().getZSlab(Z1,Z2);

      //prepare the write query
      auto write = std::make_shared<Query>(dataset.get(), 'w');
      write->position = slice_box;
      VisusReleaseAssert(dataset->beginQuery(write));

      int slab_num_samples = (int)(dims[0] * dims[1] * slices_per_slab);
      VisusReleaseAssert(write->nsamples.innerProduct() == slab_num_samples);

      //fill the buffers with some fake data
      {
        Array buffer(write->nsamples, write->field.dtype);

        VisusAssert(dtype == "int32");
        GetSamples<Int32> samples(buffer);

        for (int I = 0; I < slab_num_samples; I++)
          samples[I] = sample_id++;

        write->buffer = buffer;
      }

      //execute the writing
      auto t1 = clock();
      VisusReleaseAssert(dataset->executeQuery(access, write));
      auto t2 = clock();
      auto sec = (t2 - t1) / (float)CLOCKS_PER_SEC;
      SEC += sec;
      VisusInfo() << "Done " << Slab << " of " << num_slabs << " bbox " << slice_box.toString() <<" in "<< sec <<"sec";
    }

    VisusInfo()<<"Wrote all slabs in " << SEC << "sec";

    if (bool bVerify=true)
    {
      auto read = std::make_shared<Query>(dataset.get(), 'r');
      read->position = dataset->getBox();
      VisusReleaseAssert(dataset->beginQuery(read));

      Array buffer(read->nsamples, read->field.dtype);
      buffer.fillWithValue(0);
      read->buffer = buffer;

      auto t1 = clock();
      VisusReleaseAssert(dataset->executeQuery(access, read));
      auto t2 = clock();
      auto sec = (t2 - t1) / (float)CLOCKS_PER_SEC;

      VisusAssert(dtype == "int32");
      GetSamples<Int32> samples(buffer);

      for (int I = 0, N= (int)dims.innerProduct(); I < N; I++)
      {
        if (samples[I] != I)
          VisusInfo() << "Reading verification failed sample "<<I<<" expecting "<< I <<" got "<< samples[I];
      }
    }

    return Array();
  }

};

//////////////////////////////////////////////////////////////////////////////
class TestNetworkSpeed : public ConvertStep
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " [--c nconnections] [--n nrequests] (url)+" << std::endl
      << "Example: " << args[0] << " --c 8 -n 1000 http://atlantis.sci.utah.edu/mod_visus?from=0&to=65536&dataset=david_subsampled";
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    int nconnections = 1;
    int nrequests = 1;
    std::vector<String> urls;

    for (int I = 1; I < args.size(); I++)
    {
      if (args[I] == "-c")
        nconnections = cint(args[++I]);

      else if (args[I] == "-n")
        nrequests = cint(args[++I]);

      else
        urls.push_back(args[I]);
    }

    VisusInfo() << "Concurrency " << nconnections;
    VisusInfo() << "nrequest " << nrequests;
    VisusInfo() << "urls";
    for (auto url : urls)
      VisusInfo() << "  " << url;

    auto net = std::make_shared<NetService>(nconnections, false);

    Time t1 = Time::now();

    WaitAsync< Future<NetResponse> > wait_async;
    for (int Id = 0; Id < nrequests; Id++)
    {
      NetRequest request(urls[Id % urls.size()]);
      wait_async.pushRunning(NetService::push(net, request)).when_ready([Id](NetResponse response) {

        if (response.status != HttpStatus::STATUS_OK)
          VisusInfo() << "one request failed";

        if (Id && (Id % 100) == 0)
          VisusInfo() << "Done " << Id << " request";
      });
    }

    wait_async.waitAllDone();

    auto sec = t1.elapsedSec();
    auto stats = ApplicationStats::net.readValues(true);

    VisusInfo() << "All done in " << sec << "sec";
    VisusInfo()
      << " Num request/sec " << double(nrequests) / sec << ") "
      << " read  " << StringUtils::getStringFromByteSize(stats.rbytes) << " bytes/sec " << double(stats.rbytes) / (sec) << ") "
      << " write " << StringUtils::getStringFromByteSize(stats.wbytes) << " bytes/sec " << double(stats.wbytes) / (sec) << ") ";

    return data;
  }
};


using namespace Visus;

//////////////////////////////////////////////////////////////////////////////
class DoConvert : public ConvertStep
{
public:

  std::map<String, std::function<SharedPtr<ConvertStep>()> > actions;

  //constructor
  DoConvert()
  {
    addAction("create", []() {return std::make_shared<CreateIdx>(); });
    addAction("server", []() {return std::make_shared<StartVisusServer>();});
    addAction("minmax", []() {return std::make_shared<FixDatasetRange>(); });
    addAction("copy-dataset", []() {return std::make_shared<CopyDataset>(); });
    addAction("compress-dataset", []() {return std::make_shared<CompressDataset>(); });
    addAction("apply-filters", []() {return std::make_shared<ApplyFilters>(); });
    addAction("midx-to-idx", []() {return std::make_shared<ConvertMidxToIdx>(); });
    addAction("test-idx", []() {return std::make_shared<IdxSelfTest>(); });

    addAction("import", []() {return std::make_shared<ImportData>(); });
    addAction("export", []() {return std::make_shared<ExportData>(); });
    addAction("paste", []() {return std::make_shared<PasteData>(); });
    addAction("cast", []() {return std::make_shared<Cast>(); });
    addAction("smart-cast", []() {return std::make_shared<SmartCast>(); });
    addAction("crop", []() {return std::make_shared<CropData>(); });
    addAction("mirror", []() {return std::make_shared<MirrorData>(); });
    addAction("compute-range", []() {return std::make_shared<ComputeComponentRange>(); });
    addAction("info", []() {return std::make_shared<PrintInfo>(); });
    addAction("interleave", []() {return std::make_shared<InterleaveData>(); });
    addAction("resize", []() {return std::make_shared<ResizeData>(); });
    addAction("get-component", []() {return std::make_shared<GetComponent>(); });
    addAction("write-block", []() {return std::make_shared<ReadWriteBlock>(true); });
    addAction("read-block", []() {return std::make_shared<ReadWriteBlock>(false); });
    addAction("dump", []() {return std::make_shared<DumpData>(); });

    //cloud
    addAction("cloud-delete-blob", []() {return std::make_shared<CloudDeleteBlob>(); });
    addAction("cloud-copy-blob", []() {return std::make_shared<CloudCopyBlob>(); });
    addAction("cloud-self-test", []() {return std::make_shared<CloudSelfTest>(); });

    //speed
    addAction("test-query-speed", []() {return std::make_shared<TestQuerySpeed>(); });
    addAction("test-file-write-speed", []() {return std::make_shared<TestFileReadWriteSpeed>(true); });
    addAction("test-file-read-speed", []() {return std::make_shared<TestFileReadWriteSpeed>(false); });
    addAction("test-encoder-speed", []() {return std::make_shared<TestEncoderSpeed>(); });
    addAction("test-idx-slab-speed", []() {return std::make_shared<TestIdxSlabSpeed>(); });
    addAction("test-network-speed", []() {return std::make_shared<TestNetworkSpeed>(); });
  }

  //addAction
  void addAction(String name, std::function< SharedPtr<ConvertStep>()> fn)
  {
    actions[name] = fn;
  }

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0] << "Syntax: " << std::endl << std::endl;
    for (auto it : actions)
      out << "    " << it.first << std::endl;
    out << std::endl;
    out << "For specific help: " << ApplicationInfo::args[0] << " <action-name> help";
    out << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() <=1 || (args.size() == 2 && (args[1] == "help" || args[1] == "--help" || args[1] == "-h")))
    {
      VisusInfo()<<getHelp(args);
      return Array();
    }

    std::vector< std::vector<String> > parsed;

    for (int I=1;I<(int)args.size();I++)
    {
      String arg=args[I];

      String name = StringUtils::toLower(arg);
      while (name[0] == '-')
        name = name.substr(1);

      //begin of a new command
      if (actions.find(name) != actions.end())
      {
        parsed.push_back(std::vector<String>({ name }));
        continue;
      }

      if (parsed.empty())
      {
        std::ostringstream error_msg;
        error_msg << "Wrong argument " << arg << std::endl;
        error_msg << getHelp(args);
        ThrowException(error_msg.str());
      }

      parsed.back().push_back(arg);
    }

    for (auto args : parsed)
    {
      String name = args[0];

      VisusInfo() << "// *** STEP "<< name<<" ***";
      VisusInfo() << "Input dtype(" << data.dtype.toString() << ") " << "dims(" << data.dims.toString() << ") args("<<StringUtils::join(args," ")<<")";

      auto action = actions[name]();

      if (args.size() == 2 && (args[1] == "help" || args[1] == "--help" || args[1] == "-h"))
      {
        VisusInfo() << std::endl << args[0] << " " << name << " " << action->getHelp(args);
        return data;
      }

      Time t1 = Time::now();
      data = action->exec(data, args);
      VisusInfo() << "STEP "<<name<<" done in " << t1.elapsedMsec() << "msec";
    }

    return data;
  }
};


//////////////////////////////////////////////////////////////////////////////
int main(int argn, const char* argv[])
{
  //python main
  if (argn >= 2 && (String(argv[1]) == "--python" || String(argv[1]) == "-python"))
  {
    std::vector<String> args;
    for (int I = 0; I < argn; I++)
      if (I != 1) args.push_back(argv[I]);

    return PythonEngine::main(args);
  }

  Time T1 = Time::now();

  SetCommandLine(argn, argv);
  IdxModule::attach();

  if (argn >= 2 && String(argv[1]) == "--server")
  {
    auto modvisus = std::make_shared<ModVisus>();
    modvisus->configureDatasets();
    auto server = std::make_shared<NetServer>(10000, modvisus);
    server->runInThisThread();
    return 0;
  }

  Array data;
  DoConvert convert;

  //ignores all starting arguments not in actions (they will be global arguments such as --disable-write-locks)
  std::vector<String> args;
  args.push_back(ApplicationInfo::args[0]);

  for (auto it=ApplicationInfo::args.begin(); it!=ApplicationInfo::args.end();it++)
  {
    auto arg = *it;
    if (convert.actions.find(arg) != convert.actions.end())
    {
      args.insert(args.end(), it, ApplicationInfo::args.end());
      break;
    }
  }

  //VisusInfo() << StringUtils::join(args);
  
  if (ApplicationInfo::debug)
  {
    data = convert.exec(data, args);
  }
  else
  {
    try
    {
      data = convert.exec(data, args);
    }
    catch (std::exception& ex)
    {
      VisusInfo() << "ERROR: " << ex.what();
      IdxModule::detach();
      return -1;
    }
  }

  VisusInfo() << "All done in " << T1.elapsedSec()<< " seconds";

  IdxModule::detach();

  return 0;
}
