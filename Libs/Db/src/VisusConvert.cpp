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

#include <Visus/VisusConvert.h>
#include <Visus/Access.h>
#include <Visus/IdxFile.h>
#include <Visus/DatasetFilter.h>
#include <Visus/CloudStorage.h>
#include <Visus/IdxDataset.h>
#include <Visus/File.h>
#include <Visus/Encoder.h>
#include <Visus/Db.h>
#include <Visus/Array.h>
#include <Visus/ModVisus.h>
#include <Visus/Path.h>
#include <Visus/ThreadPool.h>
#include <Visus/NetService.h>
#include <Visus/Utils.h>
#include <Visus/ApplicationInfo.h>
#include <Visus/IdxDiskAccess.h>
#include <Visus/IdxMultipleDataset.h>
#include <Visus/MultiplexAccess.h>

namespace Visus {

  ///////////////////////////////////////////////////////////
class VISUS_DB_API VisusConvert::Step
{
public:

  //destructor
  virtual ~Step() {
  }

  //getHelp
  virtual String getHelp(std::vector<String> args) = 0;

  //exec
  virtual Array exec(Array data, std::vector<String> args) = 0;

};

namespace Private {


///////////////////////////////////////////////////////////
class CreateIdx : public VisusConvert::Step
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <filename.idx>" << std::endl
      << "   [--box <BoxNi>]" << std::endl
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
      ThrowException(args[0], "syntax error");

    String filename = args[1];

    IdxFile idxfile;
    if (data && data.getTotalNumberOfSamples())
    {
      idxfile.logic_box = BoxNi(PointNi(data.getPointDim()), data.dims);
      idxfile.fields.push_back(Field("myfield", data.dtype));
    }

    for (int I = 2; I < (int)args.size(); I++)
    {
      if (args[I] == "--box")
      {
        auto sbox = args[++I];
        int pdim = (int)StringUtils::split(sbox, " ", true).size() / 2; VisusAssert(pdim > 0);
        idxfile.logic_box = BoxNi::parseFromOldFormatString(pdim, sbox);
      }

      else if (args[I] == "--fields")
        idxfile.fields = IdxFile::parseFields(args[++I]);

      else if (args[I] == "--bits" || args[I] == "--bitmask_pattern" || args[I] == "--bitmask")
        idxfile.bitmask = DatasetBitmask::fromString(args[++I]);

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
        String time_template = args[++I];

        if (from == "*" && to == "*")
          idxfile.timesteps = DatasetTimesteps::star();
        else
          idxfile.timesteps.addTimesteps(cdouble(from), cdouble(to), 1.0);

        idxfile.time_template = time_template;
      }
      else
      {
        //just ignore
      }
    }

    idxfile.save(filename);

    //no need to write data
    if (!data)
      return data;

    //try to write data
    auto dataset = LoadDataset<IdxDataset>(filename);

    //write all data to RAM
    auto ram_access = dataset->createRamAccess(/* no memory limit*/0);
    ram_access->bDisableWriteLocks = true; //only one process is writing in sync
    if (!dataset->writeFullResolutionData(ram_access, dataset->getDefaultField(), dataset->getDefaultTime(), data))
      ThrowException("Failed to write full res data");

    for (auto& field : dataset->getFields())
      field.default_compression = "zip";

    dataset->idxfile.save(filename);

    //read all data from RAM and write to DISK
    //for each timestep...
    for (auto time : dataset->getTimesteps().asVector())
    {
      //for each field...
      for (auto& field : dataset->getFields())
      {
        auto r_access = ram_access;
        auto w_access = dataset->createAccess();

        r_access->beginRead();
        w_access->beginWrite();

        for (BigInt blockid = 0, TotBlocks = dataset->getTotalNumberOfBlocks(); blockid < TotBlocks; blockid++)
        {
          auto hz1 = w_access->getStartAddress(blockid);
          auto hz2 = w_access->getEndAddress(blockid);
          auto read_block = std::make_shared<BlockQuery>(dataset.get(), field, time, hz1, hz2, 'r', Aborted());
          if (dataset->executeBlockQueryAndWait(r_access, read_block))
          {
            auto write_block = std::make_shared<BlockQuery>(dataset.get(), field, time, hz1, hz2, 'w', Aborted());
            write_block->buffer = read_block->buffer;
            if (!dataset->executeBlockQueryAndWait(w_access, write_block))
              ThrowException("Failed to write block");
          }
        }

        r_access->endRead();
        w_access->endWrite();
      }
    }

    return data;
  }

};

///////////////////////////////////////////////////////////
class Zeros : public VisusConvert::Step
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << "   [--dims <BoxNi>]" << std::endl
      << "   [--dtype <dtype>]" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() < 2)
      ThrowException(args[0], "syntax error");

    auto dims = data.dims;
    auto dtype = data.dtype;

    for (int I = 1; I < (int)args.size(); I++)
    {
      if (args[I] == "--dims")
      {
        dims = PointNi::fromString(args[++I]);
        continue;
      }

      if (args[I] == "--dtype")
      {
        dtype = DType::fromString(args[++I]);
        continue;
      }

      ThrowException(args[0], "Invalid arguments", args[I]);
    }

    data = Array(dims, dtype);
    return data;
  }

};

///////////////////////////////////////////////////////////
class CompressDataset : public VisusConvert::Step
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <dataset_filename> <compression>" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() != 3)
      ThrowException(args[0], "syntax error, needed 3 arguments");

    String url = args[1];
    auto dataset = LoadDataset(url);

    String compression = args[2];

    if (!dataset->compressDataset(compression))
      ThrowException(args[0], "Compression failed");

    return data;
  }
};

///////////////////////////////////////////////////////////
class FixDatasetRange : public VisusConvert::Step
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
    if (args.size() < 2)
      ThrowException(args[0], "syntax error, needed filename");

    PrintInfo("FixDatasetRange starting...");

    String filename = args[1];
    auto vf = LoadDataset<IdxDataset>(filename);

    auto idxfile = vf->idxfile;

    HzOrder hzorder(idxfile.bitmask);
    BigInt last_block = vf->getTotalNumberOfBlocks();
    int    samplesperblock = 1 << idxfile.bitsperblock;

    auto access = vf->createAccessForBlockQuery();
    if (!access)
      ThrowException(args[0], "cannot create access vf->createAccess() failed");

    bool       filter_field = false; String field_filtered = "";
    bool       filter_time = false; double time_filtered = 0;
    BigInt     block_from = 0;
    BigInt     block_to = last_block;
    for (int I = 2; I < (int)args.size(); I++)
    {
      if (args[I] == "--field") { filter_field = true; field_filtered = (args[++I]); }
      else if (args[I] == "--time") { filter_time = true; time_filtered = cdouble(args[++I]); }
      else if (args[I] == "--from") { block_from = cint64(args[++I]); }
      else if (args[I] == "--to") { block_to = cint64(args[++I]); }
    }

    PrintInfo("Calculating minmax for", filter_field ? "field" + field_filtered : String("all fields"));
    PrintInfo("Calculating minmax for", filter_time ? "time" + cstring(time_filtered) : String("all timesteps"));
    PrintInfo("Calculating minmax in the block range [", block_from, ",", block_to);

    Time t1 = Time::now();

    Aborted aborted;

    access->beginRead();

    for (int F = 0; F < (int)idxfile.fields.size(); F++)
    {
      Field& field = idxfile.fields[F];

      if (filter_field && field.name != field_filtered)
      {
        PrintInfo("ignoring field", field.name, "does not match with --field argument");
        continue;
      }

      //trivial case... don't want to read all the dataset!
      DType dtype = field.dtype;
      if (dtype.isVectorOf(DTypes::INT8))
      {
        PrintInfo("range for field", field.name, "of type 'int8' quickly guessed (skipped the reading from disk)");
        continue;
      }

      if (dtype.isVectorOf(DTypes::UINT8))
      {
        PrintInfo("range for field", field.name, "of type 'uint8' quickly guessed (skipped the reading from disk)");
        continue;
      }

      for (int C = 0; C < field.dtype.ncomponents(); C++)
      {
        auto sub = field.dtype.get(C);
        Range range = Range::invalid();

        if (sub == DTypes::INT8)
          range = Range(-128, 127, 1);

        else if (sub == DTypes::UINT8)
          range = Range(0, 255, 1);

        field.setDTypeRange(range, C);
      }

      std::vector<double> timesteps = vf->getTimesteps().asVector();
      for (int time_id = 0; time_id < (int)timesteps.size(); time_id++)
      {
        double time = timesteps[time_id];

        if (filter_time && time != time_filtered)
        {
          PrintInfo("ignoring timestep", time, "does not match with --time argument");
          continue;
        }

        for (BigInt nblock = block_from; nblock < block_to; nblock++)
        {
          auto read_block = std::make_shared<BlockQuery>(vf.get(), field, time, access->getStartAddress(nblock), access->getEndAddress(nblock), 'r', aborted);
          if (!vf->executeBlockQueryAndWait(access, read_block))
            continue;

          //need to calculate since I already know it's invalid!
          for (int C = 0; C < field.dtype.ncomponents(); C++)
          {
            auto sub = field.dtype.get(C);

            if (sub == DTypes::INT8 || sub == DTypes::UINT8)
              continue;

            auto range = field.dtype.getDTypeRange(C);
            range = range.getUnion(ArrayUtils::computeRange(read_block->buffer, C));
            field.setDTypeRange(range, C);
          }

          //estimation
          if (t1.elapsedMsec() > 5000)
          {
            PrintInfo("RANGE time", time,
              "field", field.name,
              "nblock/from/to", nblock, "/", block_from, "/", block_to);

            for (int C = 0; C < field.dtype.ncomponents(); C++)
              PrintInfo("  what", C, field.dtype.getDTypeRange(C));

            idxfile.fields[F] = field;

            //try to save the intermediate file (NOTE: internally using locks so it should be fine to run in parallel)
            try
            {
              idxfile.save(filename);
            }
            catch (std::runtime_error)
            {
              PrintWarning("cannot save the INTERMEDIATE min-max in IDX dataset (vf->idxfile.save", filename, "failed");
              PrintWarning("Continuing anyway hoping to solve the problem saving the file later");
            }

            t1 = Time::now();
          }
        }
      }

      PrintInfo("done minmax for field", field.name);

      for (int C = 0; C < field.dtype.ncomponents(); C++)
        PrintInfo("  what", C, field.dtype.getDTypeRange(C));
    }

    access->endRead();

    //finally save the file
    idxfile.save(filename);

    PrintInfo("done fixFieldsRange");

    StringTree ar("fields");
    for (auto field : vf->getFields())
      ar.writeObject("field", field);
    PrintInfo(ar);

    return data;
  }
};

///////////////////////////////////////////////////////////////////////////////
class ConvertMidxToIdx : public VisusConvert::Step
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

    auto midx = std::dynamic_pointer_cast<IdxMultipleDataset>(LoadDataset(midx_filename));
    auto midx_access = midx->createAccess();

    midx->createIdxFile(idx_filename, Field("DATA", midx->getFieldByName(fieldname).dtype, "rowmajor"));

    auto idx = LoadDataset<IdxDataset>(idx_filename);
    auto idx_access = idx->createAccess();

    auto tiles = midx->generateTiles(TileSize);

    auto T1 = Time::now();
    for (int TileId = 0; TileId < tiles.size(); TileId++)
    {
      auto tile = tiles[TileId];

      auto t1 = Time::now();
      auto buffer = midx->readFullResolutionData(midx_access, midx->getFieldByName(fieldname), midx->getDefaultTime(), tile);
      int msec_read = (int)t1.elapsedMsec();
      if (!buffer)
        continue;

      t1 = Time::now();
      idx->writeFullResolutionData(idx_access, idx->getDefaultField(), idx->getDefaultTime(), buffer, tile);
      int msec_write = (int)t1.elapsedMsec();

      PrintInfo("done", TileId, "of", tiles.size(), "msec_read", msec_read, "msec_write", msec_write);

      //ArrayUtils::saveImage(concatenate("tile_",TileId,".png"),read->buffer));
    }

    //finally compress
    idx->compressDataset("zip");

    PrintInfo("ALL DONE IN", T1.elapsedMsec());
    return data;
  }

};

///////////////////////////////////////////////////////////
class ImportData : public VisusConvert::Step
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
    if (args.size() < 2)
      ThrowException(args[0], "syntax error, needed filename");

    String filename = args[1];

    auto ret = ArrayUtils::loadImage(filename, args);
    if (!ret)
      ThrowException(args[0], "cannot load image", filename);

    return ret;
  }
};

///////////////////////////////////////////////////////////
class ExportData : public VisusConvert::Step
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <filename> [save_args]*" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() < 2)
      ThrowException(args[0], "syntax error");

    String filename = args[1];

    if (!ArrayUtils::saveImage(filename, data, args))
      ThrowException(args[0], "saveImage failed", filename);

    return data;
  }
};

///////////////////////////////////////////////////////////
class PasteData : public VisusConvert::Step
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <filename> " << std::endl
      << "   [--source-box      <BoxNi>]" << std::endl
      << "   [--destination-box <BoxNi>]" << std::endl
      << "   [load_args]*" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() < 2)
      ThrowException(args[0], "syntax error");

    String filename = args[1];

    Array to_paste = ArrayUtils::loadImage(filename, args);
    if (!to_paste)
      ThrowException(args[0], "Cannot load", filename);

    int pdim = data.getPointDim();

    //embedding in case I'm missing point-dims (see interleaving)
    if (pdim > to_paste.dims.getPointDim())
      to_paste.dims.setPointDim(pdim, 1);

    BoxNi Dbox(PointNi(pdim), data.dims);
    BoxNi Sbox(PointNi(pdim), to_paste.dims);
    for (int I = 2; I < (int)args.size(); I++)
    {
      if (args[I] == "--destination-box")
        Dbox = BoxNi::parseFromOldFormatString(pdim, args[++I]);

      else if (args[I] == "--source-box") {
        Sbox = BoxNi::parseFromOldFormatString(pdim, args[++I]);
        if (pdim > Sbox.getPointDim()) {
          Sbox.p1.setPointDim(pdim, 0);
          Sbox.p2.setPointDim(pdim, 1);
        }
      }
    }

    if (!ArrayUtils::paste(data, Dbox, to_paste, Sbox))
      ThrowException(args[0], "paste of image failed");

    return data;
  }
};

///////////////////////////////////////////////////////////
class GetComponent : public VisusConvert::Step
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
      ThrowException(args[0], "syntax error");

    int C = cint(args[1]);
    return data.getComponent(C);
  }
};

///////////////////////////////////////////////////////////
class Cast : public VisusConvert::Step
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <dtype>" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() != 2)
      ThrowException(args[0], "syntax error");

    DType dtype = DType::fromString(args[1]);
    return ArrayUtils::cast(data, dtype);
  }
};

///////////////////////////////////////////////////////////
class SmartCast : public VisusConvert::Step
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <dtype>" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() != 2)
      ThrowException(args[0], "syntax error");

    DType dtype = DType::fromString(args[1]);
    return ArrayUtils::smartCast(data, dtype);
  }
};

///////////////////////////////////////////////////////////
class ResizeData : public VisusConvert::Step
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << "   [--dtype <DType>]" << std::endl
      << "   [--dims <PointNi>]" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() < 2)
      ThrowException(args[0], "syntax error");

    DType   dtype = data.dtype;
    PointNi dims = data.dims;
    for (int I = 1; I < (int)args.size(); I++)
    {
      if (args[I] == "--dtype")
      {
        dtype = DType::fromString(args[++I]);
        continue;
      }

      if (args[I] == "--dims")
      {
        dims = PointNi::fromString(args[++I]);
        continue;
      }

      ThrowException(args[0], "Invalid arguments", args[I]);
    }

    if (!data.resize(dims, dtype, __FILE__, __LINE__))
      ThrowException(args[0], "resize failed");

    return data;
  }
};

///////////////////////////////////////////////////////////
class ResampleData : public VisusConvert::Step
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << "   [--dims <PointNi>]" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() < 2)
      ThrowException(args[0], "syntax error");


    PointNi target_dims = data.dims;
    for (int I = 1; I < (int)args.size(); I++)
    {
      if (args[I] == "--dims")
      {
        target_dims = PointNi::fromString(args[++I]);
        continue;
      }

      ThrowException(args[0], "Invalid arguments", args[I]);
    }

    return ArrayUtils::resample(target_dims, data);
  }
};

///////////////////////////////////////////////////////////
class CropData : public VisusConvert::Step
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <BoxNi>" << std::endl;
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() != 2)
      ThrowException(args[0], "syntax error");

    int pdim = data.getPointDim();
    String sbox = args[1];

    BoxNi box = BoxNi::parseFromOldFormatString(pdim, sbox);
    if (!box.isFullDim())
      ThrowException(args[0], "Invalid box", sbox);

    return ArrayUtils::crop(data, box);
  }
};

///////////////////////////////////////////////////////////
class MirrorData : public VisusConvert::Step
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
      ThrowException(args[0], "syntax error");

    int axis = cint(args[1]);
    if (axis < 0)
      ThrowException(args[0], "Invalid axis", args[1]);

    return ArrayUtils::mirror(data, axis);
  }
};

///////////////////////////////////////////////////////////
class ComputeComponentRange : public VisusConvert::Step
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
      ThrowException(args[0], "syntax error");

    int C = cint(args[1]);
    Range range = ArrayUtils::computeRange(data, C);
    PrintInfo("Range of component", C, "is", range);

    return data;
  }
};

///////////////////////////////////////////////////////////
class InterleaveData : public VisusConvert::Step
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
      ThrowException(args[0], "syntax error");

    if (data.dtype.ncomponents() > 1 && !Utils::isByteAligned(data.dtype.get(0).getBitSize()))
      ThrowException(args[0], "request to --interleave but a sample is not byte aligned");

    //need to interleave (the input for example is RRRRR...GGGGG...BBBBB and I really need RGBRGBRGB)
    return ArrayUtils::interleave(data);
  }
};

///////////////////////////////////////////////////////////
class PrintInfo : public VisusConvert::Step
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
      ThrowException(args[0], "syntax error");

    String filename = args[1];
    StringTree info = ArrayUtils::statImage(filename);
    if (!info.valid())
      ThrowException(args[0], "Could not open", filename);

    PrintInfo("\n", info);

    auto dataset = LoadDataset(filename);
    return data;
  }
};


///////////////////////////////////////////////////////////////////////
class TestQuerySpeed : public VisusConvert::Step
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    std::ostringstream out;
    out << args[0]
      << " <filename> [--query-dim value]" << std::endl
      << "Example: " << args[0] << " E:/google_sci/visus_dataset/2kbit1/zip/hzorder/visus.idx --query-dim 512";
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    if (args.size() < 2)
      ThrowException(args[0], "syntax error");

    String filename = args[1];
    auto dataset = LoadDataset(filename);

    int    query_dim = dataset->getPointDim() == 2 ? 2048 : 512;

    for (int I = 2; I < (int)args.size(); I++)
    {
      if (args[I] == "--query-dim")
      {
        String s_query_dim = args[++I];

        if (!StringUtils::tryParse(s_query_dim, query_dim) || query_dim <= 0)
          ThrowException(args[0], "Invalid --query-dim ", s_query_dim);

        continue;
      }

      ThrowException(args[0], "Invalid args", args[I]);
    }

    srand((unsigned int)Time::now().getTimeStamp());

    PrintInfo("Testing query...");
    auto access = dataset->createAccess();

    auto tiles = dataset->generateTiles(query_dim);

    Time T1 = Time::now();
    Time Tstats = Time::now();

    for (int TileId = 0; TileId < tiles.size(); TileId++)
    {
      auto tile = tiles[TileId];
      auto buffer = dataset->readFullResolutionData(access, dataset->getDefaultField(), dataset->getDefaultTime(), tile);
      if (!buffer)
        continue;

      PrintInfo("Done", TileId, "of", tiles.size());

      if (Tstats.elapsedSec() > 3.0)
      {
        auto sec = Tstats.elapsedSec();

        auto nopen = (int)ApplicationStats::io.nopen;
        auto rbytes = (int)ApplicationStats::io.rbytes;
        auto wbytes = (int)ApplicationStats::io.wbytes;

        PrintInfo("ndone", TileId, "/", tiles.size(),
          "io.nopen", nopen, "/", Int64(nopen / sec),
          "io.rbytes", StringUtils::getStringFromByteSize(rbytes), StringUtils::getStringFromByteSize(Int64(rbytes / sec)),
          "io.wbytes", StringUtils::getStringFromByteSize(wbytes), StringUtils::getStringFromByteSize(Int64(wbytes / sec)));
        ApplicationStats::io.reset();
        Tstats = Time::now();
      }
    }

    PrintInfo("Test done in", T1.elapsedSec());
    return data;
  }
};

///////////////////////////////////////////////////////////////////////////////
class TestIdxSlabSpeed : public VisusConvert::Step
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
      << " [--rowmajor   | --hzorder]" << std::endl
      << "Example: " << args[0] << " --dims \"512 512 512\" --num-slabs 128 --dtype int32";
    return out.str();
  }

  //exec
  virtual Array exec(Array data, std::vector<String> args) override
  {
    String filename = "./temp/TestIdxSlabSpeed/visus.idx";

    auto dirname = Path(filename).getParent();
    if (FileUtils::existsDirectory(dirname))
    {
      ThrowException("Please empty the directory", dirname);
      return Array();
    }

    PointNi dims = PointNi(512, 512, 512);
    int     num_slabs = 128;
    String  dtype = "int32";
    String  layout = "";

    for (int I = 1; I < (int)args.size(); I++)
    {
      if (args[I] == "--dims")
      {
        dims = PointNi::fromString(args[++I]);
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

      ThrowException("Wrong argument ", args[I]);
    }

    int slices_per_slab = (int)dims[2] / num_slabs;

    PrintInfo("--dims            ", dims);
    PrintInfo("--num-slabs       ", num_slabs);
    PrintInfo("--dtype           ", dtype);
    PrintInfo("--slices-per-slab ", slices_per_slab);

    //create the idx file
    {
      IdxFile idxfile;
      idxfile.logic_box = BoxNi(PointNi(3), dims);
      {
        Field field("myfield", DType::fromString(dtype));
        field.default_compression = ""; // no compression (in writing I should not use compression)
        field.default_layout = layout;
        idxfile.fields.push_back(field);
      }
      idxfile.save(filename);
    }

    //now create a Dataset, save it and reopen from disk
    auto dataset = LoadDataset(filename);

    //any time you need to read/write data from/to a Dataset I need a Access
    auto access = dataset->createAccess();

    //for example I want to write data by slices
    Int32 sample_id = 0;
    double SEC = 0;

    for (int Slab = 0; Slab < num_slabs; Slab++)
    {
      //this is the bounding box of the region I'm going to write
      auto Z1 = Slab * slices_per_slab;
      auto Z2 = Z1 + slices_per_slab;

      BoxNi slice_box = dataset->getLogicBox().getZSlab(Z1, Z2);

      //prepare the write query
      auto write = std::make_shared<BoxQuery>(dataset.get(), dataset->getDefaultField(), dataset->getDefaultTime(), 'w');
      write->logic_box = slice_box;
      dataset->beginQuery(write);
      VisusReleaseAssert(write->isRunning());

      int slab_num_samples = (int)(dims[0] * dims[1] * slices_per_slab);
      VisusReleaseAssert(write->getNumberOfSamples().innerProduct() == slab_num_samples);

      //fill the buffers with some fake data
      {
        Array buffer(write->getNumberOfSamples(), write->field.dtype);

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
      PrintInfo("Done", Slab, "of", num_slabs, "bbox ", slice_box.toString(/*bInterleave*/true), "in", sec, "sec");
    }

    PrintInfo("Wrote all slabs in", SEC, "sec");

    if (bool bVerify = true)
    {
      auto read = std::make_shared<BoxQuery>(dataset.get(), dataset->getDefaultField(), dataset->getDefaultTime(), 'r');
      read->logic_box = dataset->getLogicBox();
      dataset->beginQuery(read);
      VisusReleaseAssert(read->isRunning());

      Array buffer(read->getNumberOfSamples(), read->field.dtype);
      buffer.fillWithValue(0);
      read->buffer = buffer;

      auto t1 = clock();
      VisusReleaseAssert(dataset->executeQuery(access, read));
      auto t2 = clock();
      auto sec = (t2 - t1) / (float)CLOCKS_PER_SEC;

      VisusAssert(dtype == "int32");
      GetSamples<Int32> samples(buffer);

      for (int I = 0, N = (int)dims.innerProduct(); I < N; I++)
      {
        if (samples[I] != I)
          PrintInfo("Reading verification failed sample", I, "expecting", I, "got", samples[I]);
      }
    }

    return Array();
  }

};

//////////////////////////////////////////////////////////////////////////////
class TestNetworkSpeed : public VisusConvert::Step
{
public:

  //getHelp
  virtual String getHelp(std::vector<String> args) override
  {
    return cstring(args[0],
      "[--c nconnections] [--n nrequests] (url)+", "\n",
      "Example: ", args[0], "--c 8 -n 1000 http://atlantis.sci.utah.edu/mod_visus?from=0&to=65536&dataset=david_subsampled");
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

    PrintInfo("Concurrency", nconnections);
    PrintInfo("nrequest", nrequests);
    PrintInfo("urls");
    for (auto url : urls)
      PrintInfo("  ", url);

    auto net = std::make_shared<NetService>(nconnections, false);

    Time t1 = Time::now();

    WaitAsync< Future<NetResponse> > wait_async;
    for (int Id = 0; Id < nrequests; Id++)
    {
      NetRequest request(urls[Id % urls.size()]);
      wait_async.pushRunning(NetService::push(net, request)).when_ready([Id](NetResponse response) {

        if (response.status != HttpStatus::STATUS_OK)
          PrintInfo("one request failed");

        if (Id && (Id % 100) == 0)
          PrintInfo("Done ", Id, "request");
      });
    }

    wait_async.waitAllDone();

    auto sec = t1.elapsedSec();

    PrintInfo("All done in", sec, "sec");
    PrintInfo(
      "Num request/sec", double(nrequests) / sec,
      "read", StringUtils::getStringFromByteSize(ApplicationStats::net.rbytes), "bytes/sec", double(ApplicationStats::net.rbytes) / (sec),
      "write", StringUtils::getStringFromByteSize(ApplicationStats::net.wbytes), "bytes/sec", double(ApplicationStats::net.wbytes) / (sec));
    ApplicationStats::net.reset();

    return data;
  }
};

} //namespace Private

//////////////////////////////////////////////////////////////////////////////
VisusConvert::VisusConvert()
{
  using namespace Private;

  addAction("create", []() {return std::make_shared<CreateIdx>(); });
  addAction("zeros", []() {return std::make_shared<Zeros>(); });
  addAction("minmax", []() {return std::make_shared<FixDatasetRange>(); });
  addAction("compress-dataset", []() {return std::make_shared<CompressDataset>(); });
  addAction("midx-to-idx", []() {return std::make_shared<ConvertMidxToIdx>(); });

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
  addAction("resample", []() {return std::make_shared<ResampleData>(); });
  addAction("get-component", []() {return std::make_shared<GetComponent>(); });

  addAction("test-query-speed", []() {return std::make_shared<TestQuerySpeed>(); });
  addAction("test-idx-slab-speed", []() {return std::make_shared<TestIdxSlabSpeed>(); });
  addAction("test-network-speed", []() {return std::make_shared<TestNetworkSpeed>(); });
}

//////////////////////////////////////////////////////////////////////////////
String VisusConvert::getHelp()
{
  std::ostringstream out;
  out << "Syntax: " << std::endl << std::endl;
  for (auto it : actions)
    out << "    " << it.first << std::endl;
  out << std::endl;
  out << "For specific help: <action-name> help";
  out << std::endl;
  return out.str();
}

static String TrimDash(String value) {
  while (!value.empty() && value[0] == '-')
    value = value.substr(1);
  return value;
}


//////////////////////////////////////////////////////////////////////////////
void VisusConvert::runFromArgs(std::vector<String> args)
{
  if (args.empty())
  {
    PrintInfo(getHelp());
    return;
  }

  std::vector< std::vector<String> > steps;
  Array data;

  for (auto arg : args)
  {
    String name = TrimDash(StringUtils::toLower(arg));

    if (name == "help")
    {
      PrintInfo(getHelp());
      return;
    }

    if (name == "test-idx")
    {
      SelfTestIdx(300);
      return;
    }

    //begin of a new command
    if (bool bNewActions = (actions.find(name) != actions.end()))
      steps.push_back({ name });

    //just ignore up to a good action (needed for for example for --disable-write-locks)
    else if (!steps.empty()) 
      steps.back().push_back(arg);
  }

  for (auto step : steps)
  {
    String name = step[0];
    auto action = actions[name]();

    PrintInfo("//*** STEP ", name, "***");
    PrintInfo("Input dtype", data.dtype, "dims", data.dims, "step", StringUtils::join(step, " "));

    if (step.size() == 2 && (step[1] == "help" || step[1] == "--help" || step[1] == "-h"))
    {
      PrintInfo("\n", step[0], name, action->getHelp(step));
      return;
    }

    Time t1 = Time::now();
    data = action->exec(data, step);
    PrintInfo("STEP ", name, "done in", t1.elapsedMsec(), "msec");
  }
}

} //namespace VIsus