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
    auto dataset = LoadIdxDataset(filename);

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
          auto read_block = dataset->createBlockQuery(blockid, field, time);
          if (dataset->executeBlockQueryAndWait(r_access, read_block))
          {
            auto write_block = dataset->createBlockQuery(blockid, field, time, 'w', Aborted());
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

} //namespace Private

//////////////////////////////////////////////////////////////////////////////
VisusConvert::VisusConvert()
{
  using namespace Private;

  addAction("create", []() {return std::make_shared<CreateIdx>(); });
  addAction("zeros", []() {return std::make_shared<Zeros>(); });
  addAction("import", []() {return std::make_shared<ImportData>(); });
  addAction("export", []() {return std::make_shared<ExportData>(); });
  addAction("paste", []() {return std::make_shared<PasteData>(); });
  addAction("cast", []() {return std::make_shared<Cast>(); });
  addAction("smart-cast", []() {return std::make_shared<SmartCast>(); });
  addAction("crop", []() {return std::make_shared<CropData>(); });
  addAction("mirror", []() {return std::make_shared<MirrorData>(); });
  addAction("interleave", []() {return std::make_shared<InterleaveData>(); });
  addAction("resize", []() {return std::make_shared<ResizeData>(); });
  addAction("resample", []() {return std::make_shared<ResampleData>(); });
  addAction("get-component", []() {return std::make_shared<GetComponent>(); });
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

    //begin of a new command
    if (actions.find(name) != actions.end())
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
