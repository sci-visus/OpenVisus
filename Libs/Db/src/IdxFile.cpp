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

#include <Visus/IdxFile.h>
#include <Visus/Dataset.h>
#include <Visus/CloudStorage.h>
#include <Visus/Path.h>
#include <Visus/CriticalSection.h>
#include <Visus/File.h>
#include <Visus/Encoder.h>
#include <Visus/IdxHzOrder.h>
#include <Visus/IdxDiskAccess.h>
#include <Visus/NetService.h>
#include <Visus/StringTree.h>

namespace Visus {

////////////////////////////////////////////////////////////////////
String IdxFile::guessFilenameTemplate(String url)
{
  //this is the bits for block number
  int nbits_blocknumber = (this->bitmask.getMaxResolution() - bitsperblock);

  //"./" means where the directory where there is the idx
  //note: for remote I really don't care what's the template
  
  std::ostringstream out;

  String basename;
  
  Url __url__(url);
  if (__url__.valid() && __url__.isFile())
    basename=Path(__url__.getPath()).getFileNameWithoutExtension();

  if (basename.empty())
    basename = "visus_data";

  out << "./" + basename;

  while (nbits_blocknumber > 16)
  {
    out << "/%02x";  //256 subdirectories
    nbits_blocknumber -= 8;
  }

  out << "/%04x.bin"; //max 65536  files
  return out.str();
    
}

//////////////////////////////////////////////////////////////////////////////
void IdxFile::validate(String url)
{
  //version
  if (this->version == 0)
    this->version = 6;

  if (version <= 0)
  {
    PrintInfo("Wrong version",version);
    this->version = -1;
    return;
  }

  //box
  if (!logic_box.isFullDim())
  {
    PrintWarning("wrong box",logic_box.toOldFormatString());
    this->version = -1;
    return;
  }

  //bitmask, default guess is blocks not full res
  if (bitmask.empty())
  {
    bitmask = DatasetBitmask::guess('V', logic_box.p2);
  }
  else
  {
    //used specified only the first letter (V | F)
    auto pattern = bitmask.toString();
    if (pattern.size() == 1 && std::isalpha(pattern[0]))
    {
      auto first_letter = pattern[0];
      bitmask = DatasetBitmask::guess(first_letter, logic_box.p2);
    }
    else
    {
      if (!bitmask.valid())
      {
        PrintWarning("invalid bitmask");
        this->version = -1;
        return;
      }
    }
  }

  auto pdim = bitmask.getPointDim();

  if (!this->bounds.valid())
    this->bounds = this->logic_box;

  if (bounds.getSpaceDim() != (pdim + 1) || bounds.getPointDim() != pdim)
  {
    PrintWarning("invalid bounds");
    this->version = -1;
    return;
  }

  //check fields
  if (fields.empty())
  {
    PrintWarning("no fields");
    this->version = -1;
    return;
  }

  //parse layout
  for (int N = 0; N < (int)this->fields.size(); N++)
  {
    Field& field = this->fields[N];
    field.index = cstring(N);

    if (!field.valid())
    {
      PrintWarning("wrong field", field.name);
      this->version = -1;
      return;
    }

    //for historical reason '0' me
    if (field.default_layout == "0" || StringUtils::contains(field.default_layout, "hz") || version < 6)
      field.default_layout = "hzorder";
    else
      field.default_layout = "";
  }

  //bitsperblock
  if (bitsperblock == 0)
    bitsperblock = std::min(bitmask.getMaxResolution(), 16);

  if (bitsperblock<=0)
  {
    PrintWarning("wrong bitsperblock", bitsperblock);
    this->version=-1;
    return;
  }

  if (bitsperblock>bitmask.getMaxResolution())
  {
    PrintWarning("bitsperblock",bitsperblock,"is greater than max_resolution",bitmask.getMaxResolution());
    this->bitsperblock=bitmask.getMaxResolution();
  }

  Int64 totblocks = ((Int64)1) << (bitmask.getMaxResolution() - bitsperblock);

  //one file per dataset
  if (blocksperfile == -1)
  {
    blocksperfile = (int)totblocks;
    VisusAssert(blocksperfile == totblocks);
  }
  //user specified a filename template which is one file only
  else if (blocksperfile == 0 && !filename_template.empty() && !StringUtils::contains(filename_template, "%"))
  {
    blocksperfile = (int)totblocks;
    VisusAssert(blocksperfile == totblocks);
  }
  //need to guess blocksperfile
  else if (blocksperfile == 0)
  {
    //compute overall blockdim (ignoring the headers)
    int overall_blockdim = 0;
    for (auto field : this->fields)
      overall_blockdim += (int)field.dtype.getByteSize((Int64)1 << bitsperblock);

    //probably the file will be compressed and I will have a compression ratio at least of 50%, so the file size won't be larger than 16mb
    const double likely_compression_ratio = 0.5;
    const int mb = 1024 * 1024;
    const int target_compressed_filesize = 16 * mb;
    const int target_uncompressed_filesize = (int)(target_compressed_filesize / likely_compression_ratio);

    blocksperfile = target_uncompressed_filesize / overall_blockdim;

    if (blocksperfile > totblocks)
      blocksperfile = (int)totblocks;
  }

  if (blocksperfile<=0)
  {
    PrintWarning("wrong blockperfile",blocksperfile);
    this->version=-1;
    return;
  }

  if (blocksperfile>(((BigInt)1)<<(bitmask.getMaxResolution()-bitsperblock)))
  {
    //this can happen with PIDX
    //PrintWarning("wrong blockperfile",blocksperfile,"with bitmask.getMaxResolution()",bitmask.getMaxResolution(),"bitsperblock",bitsperblock);
    
    if (this->version<=6)
    {
      //VisusAssert(false);
      ; //DO NOTHING... there are idx files around with a wrong blocksperfile (and a bigger header)
    }
    else
    {
      this->blocksperfile= (int)totblocks;
      VisusAssert(this->blocksperfile==totblocks);
    }
  }

  //filename_template
  if (filename_template.empty())
    filename_template = guessFilenameTemplate(url);
}

//////////////////////////////////////////////////////////////////////////////
IdxFile IdxFile::createNewOne(String filename) const
{
  VisusReleaseAssert(!FileUtils::existsFile(filename));
  IdxFile ret = *this;
  ret.version = 6;
  ret.block_interleaving = 0;
  ret.filename_template = ret.guessFilenameTemplate(filename);

  //store in row major
  for (auto& field : ret.fields)
    field.default_layout = "";

  ret.save(filename);
  return ret;
}

//////////////////////////////////////////////////////////////////////////////
std::vector<Field> IdxFile::parseFields(String fields_content)
{
  //split using + (need to take care if I'm inside some paranthesis)
  std::vector<String> v;
  {
    int nopen=0;
    String last="";
    while (!fields_content.empty())
    {
      int ch=fields_content[0];fields_content=fields_content.substr(1);
      if (ch=='+' && nopen==0) 
      {
        last=StringUtils::trim(last);
        if (!last.empty()) v.push_back(last);
        last="";
        continue;
      }

      if (ch=='(' || ch=='[' || ch=='{')  nopen++;
      if (ch==')' || ch==']' || ch=='}')  nopen--;
      last.push_back(ch);
      continue;
    }

    if (!(last=StringUtils::trim(last)).empty()) v.push_back(last);
    last="";
  }

  std::vector<Field> ret;

  for (int I=0;I<(int)v.size();I++)
  {
    auto field = Field::fromString(v[I]);
    
    if (!field.valid())
      return std::vector<Field>();

    ret.push_back(field);
  }

  return ret;
}

//////////////////////////////////////////////////////////////////////////////
IdxFile::IdxFile(int version_) : version(version_)
{
  timesteps.addTimestep(0); //default is to have one timestep with 0 value
}

//////////////////////////////////////////////////////////////////////////////
void IdxFile::load(String url,String& TypeName)
{
  String content=Utils::loadTextDocument(url);

  if (content.empty())
    ThrowException("empty content");

  auto ar = StringTree::fromString(content);
  if (ar.valid())
  {
    ar.read("typename",TypeName);
    ar.readObject("idxfile",*this);
  }
  else
  {
    TypeName = "IdxDataset";
    this->readFromOldFormat(content);
  }

  this->validate(url);
}

//////////////////////////////////////////////////////////////////////////////
void IdxFile::save(String filename)
{
  if (filename.empty())
    ThrowException("invalid name");

  //the user is trying to create a new IdxFile... help him by guessing and checking some values
  if (version == 0)
    validate(filename);

  String content;

  //backward compatible
  bool bPreferOldFormat = true;

  if (bPreferOldFormat)
  {
    content=writeToOldFormat();
  }
  else
  {
    Archive ar("dataset");
    ar.writeObject("idxfile", *this);
    content = ar.toString();
  } 

  //save the file (using file locks... there could be tons of visus running!)
  {
    FileUtils::lock(filename);
    Utils::saveTextDocument(filename, content);
    FileUtils::unlock(filename);
  }
}


/////////////////////////////////////////////////////////////////////////////
String IdxFile::writeToOldFormat() const
{
  std::ostringstream out;

  out<<"(version)\n"<<this->version<<"\n";
  out<<"(box)\n"<< this->logic_box.toOldFormatString()<<"\n";

  auto logic_to_physic = Position::computeTransformation(this->bounds,this->logic_box);
  if (!logic_to_physic.isIdentity())
    out << "(logic_to_physic)\n" << logic_to_physic.toString() << "\n";
  
  //dump fields
  out<<"(fields)\n";
  for (int i=0;i<(int)this->fields.size();i++) 
  {
    const Field& field=this->fields[i];

    out<<(i?"+ ":"");
    out<<field.name<<" ";
    out<<field.dtype.toString()<<" ";

    //compression
    if (!field.default_compression.empty())
    {
      if (version<6)
        out<<"compressed"<<" "; //old format, compressed means zip, consider that v12345 only supports zip
      else
        out<<"default_compression("<<field.default_compression <<")"<<" ";
    }

    //default_layout(...) (1 means row major, 0 means hzorder)
    out << "default_layout("<< field.default_layout <<") ";

    //default_value
    out<<"default_value("<<field.default_value<<")"<<" ";

    //filter(...)
    if (!field.filter.empty())
      out<<"filter("<<field.filter<<")"<<" ";

    //min/max
    std::vector<String> vmin,vmax;
    for (int C=0;C<field.dtype.ncomponents();C++)
    {
      Range range=field.dtype.getDTypeRange(C);
      vmin.push_back(range.delta()>0? cstring(range.from) : "0");
      vmax.push_back(range.delta()>0? cstring(range.to  ) : "0");
    }
    out<<"min("<<StringUtils::join(vmin," ")<<") ";
    out<<"max("<<StringUtils::join(vmax," ")<<") ";

    out<<"\n";
  }
      
  out<<"(bits)\n"<<this->bitmask.toString()<<"\n";
  out<<"(bitsperblock)\n"<<this->bitsperblock<<"\n";
  out<<"(blocksperfile)\n"<<this->blocksperfile<<"\n";
  out<<"(interleave block)\n"<<this->block_interleaving<<"\n";
  
  if (!this->time_template.empty())
  {
    //STAR FORMAT
    //* * time_template 
    if (this->timesteps==DatasetTimesteps::star())
    {
      out<<"(time)\n"<<"*"<<" "<<"*"<<" "<<this->time_template<<"\n";
    }
    else
    {
      //OLD FORMAT
      //From To time_template
      if (this->timesteps==DatasetTimesteps(this->timesteps.getMin(),this->timesteps.getMax(),1.0))
      {
        out<<"(time)\n"<<this->timesteps.getMin()<<" "<<this->timesteps.getMax()<<" "<<this->time_template<<"\n";
      }
      //NEW FORMAT
      //time_template (From,To,Step) (From,To,Step) (From,To,Step)
      else
      {
        out<<"(time)\n"<<this->time_template<<" ";
        for (int N=0;N<this->timesteps.size();N++)
        {
          const DatasetTimesteps::IRange& irange=this->timesteps.getAt(N);
          out<<"("<<irange.a<<","<<irange.b<<","<<irange.step<<") ";
        }
        out<<"\n";
      }
    }
  }

  out << "(filename_template)\n" << filename_template << "\n";
  out << "(missing_blocks)\n" << missing_blocks << "\n";

  //write other medatata
  for (auto it : this->metadata)
    out << "(" << it.first << ")\n" << it.second<< "\n"; 

  return out.str();
}



//////////////////////////////////////////////////////////////////////////////
void IdxFile::readFromOldFormat(const String& content)
{
  //parse the idx text format
  StringMap map;

  std::vector<String> v = StringUtils::getLinesAndPurgeComments(content, "#");
  String key, value;
  for (int I = 0; I < (int)v.size(); I++)
  {
    String line = StringUtils::trim(v[I]);

    //ignore empty strings
    if (line.empty())
      continue;

    //comment
    if (StringUtils::startsWith(line, "#"))
      continue;

    if (bool bIsKey = StringUtils::startsWith(line, "("))
    {
      //flush previous
      if (!key.empty())
        map.setValue(key, StringUtils::trim(value));

      key = StringUtils::trim(line);
      value = String();
    }
    else
    {
      value = value + " " + line;
    }
  }

  if (!key.empty())
    map.setValue(key, StringUtils::trim(value));

  //trim the values
  for (auto it = map.begin(); it != map.end(); it++)
    it->second = StringUtils::trim(it->second);


#if 0

  //there is a very old format which I am not sure has a binary compatible format.
  //it does not have (version) and first line is (dimensions)
  //it also has (rootpath) and (filepath)
  //my first attempts to enable it didn't work during the read-blocks
    if (map.hasValue("(dimensions)") && map.hasValue("(filepath)"))
    {
      auto dimensions = PointNi::fromString(map.getValue("(dimensions)"));
      auto boundingbox = BoxNi::fromString(map.getValue("(boundingbox)"));
      auto samplesize = cint(map.getValue("(samplesize)"));
      auto samplesperblock = cint(map.getValue("(samplesperblock)"));
      auto blocksperfile = cint(map.getValue("(blocksperfile)"));
      auto rootpath = map.getValue("(rootpath)");
      auto filepath = map.getValue("(filepath)");
    
      auto bitsperblock = Utils::getLog2(samplesperblock);
      auto filename_template = StringUtils::replaceAll(rootpath + "/" + filepath, ":", "");
      auto field = concatenate("DATA uint8[", samplesize, "] format(0) default_value(0) min(0) max(0)");
      auto bits = DatasetBitmask::guess('V', dimensions,/*regular_as_soon_as possible*/true); //not sure about this
    
      map.setValue("(version)", "1");
      map.setValue("(box)", boundingbox.toString());
      map.setValue("(fields)", field);
      map.setValue("(bits)", bits.toString());
      map.setValue("(bitsperblock)", cstring(bitsperblock));
      map.setValue("(blocksperfile)", cstring(blocksperfile));
      map.setValue("(filename_template)", filename_template);
    }
#endif

  this->version = cint(map.getValue("(version)"));
  if (!(this->version >= 1 && this->version <= 6))
    ThrowException("invalid version");
  map.eraseValue("(version)");

  this->bitmask = DatasetBitmask::fromString(map.getValue("(bits)"));
  map.eraseValue("(bits)");

  this->logic_box = BoxNi::parseFromOldFormatString(this->bitmask.getPointDim(), map.getValue("(box)"));
  map.eraseValue("(box)");

  auto pdim = this->bitmask.getPointDim();

  if (map.hasValue("(physic_box)"))
  {
    this->bounds = BoxNd::fromString(map.getValue("(physic_box)"));
    this->bounds.setSpaceDim(pdim + 1);
    map.eraseValue("(physic_box)");
  }

  else if (map.hasValue("(logic_to_physic)"))
  {
    auto logic_to_physic = Matrix::fromString(map.getValue("(logic_to_physic)"));
    this->bounds = Position(logic_to_physic, this->logic_box);
    this->bounds.setSpaceDim(pdim + 1);
    map.eraseValue("(logic_to_physic)");
  }
  else
  {
    this->bounds = this->logic_box;
  }

  //parse fields
  if (map.hasValue("(fields)"))
  {
    this->fields = parseFields(map.getValue("(fields)"));
    map.eraseValue("(fields)");
  }

  this->bitsperblock = cint(map.getValue("(bitsperblock)")); 
  map.eraseValue("(bitsperblock)");

  this->blocksperfile = cint(map.getValue("(blocksperfile)"));
  map.eraseValue("(blocksperfile)");

  this->filename_template = map.getValue("(filename_template)");
  map.eraseValue("(filename_template)");

  this->missing_blocks = cint(map.getValue("(missing_blocks)"));
  map.eraseValue("(missing_blocks)");

  if (map.hasValue("(interleave)"))
  {
    this->block_interleaving = cint(map.getValue("(interleave)"));
    map.eraseValue("(interleave)");
  }

  if (map.hasValue("(interleave block)"))
  {
    this->block_interleaving = cint(map.getValue("(interleave block)"));
    map.eraseValue("(interleave block)");
  }


  if (map.hasValue("(time)"))
  {
    std::vector<String> vtime = StringUtils::split(map.getValue("(time)"), " ");
    map.eraseValue("(time)");

    if (vtime.size() < 2)
      ThrowException("invalid (time)");

    this->timesteps = DatasetTimesteps();
    double parse_time = 0;

    ///star format (means I don't know in advance the timesteps)
    //* * time_template 
    if (vtime[0] == "*")
    {
      bool bGood = vtime.size() == 3 && vtime[1] == "*";
      if (!bGood)
        ThrowException("idx (time) is wrong");

      this->time_template = vtime[2];
    }
    //old format
    //From To time_template
    else if (StringUtils::tryParse(vtime[0], parse_time))
    {
      bool bGood = vtime.size() == 3 && StringUtils::tryParse(vtime[1], parse_time);
      if (!bGood)
        ThrowException("idx (time) is wrong");
      double From = cdouble(vtime[0]);
      double To = cdouble(vtime[1]);
      this->timesteps.addTimesteps(From, To, 1.0);
      this->time_template = vtime[2];
    }
    //new format
    //time_template (From,To,Step) (From,To,Step) (From,To,Step)
    else
    {
      this->time_template = vtime[0];
      for (int I = 1; I < (int)vtime.size(); I++)
      {
        bool bGood = StringUtils::startsWith(vtime[I], "(") && StringUtils::find(vtime[I], ")");
        if (!bGood)
          ThrowException("idx (time) is wrong");

        std::vector<String> vrange = StringUtils::split(vtime[I].substr(1, vtime[I].size() - 2), ",", true);
        double From = vrange.size() >= 1 ? cdouble(vrange[0]) : 0;
        double To = vrange.size() >= 2 ? cdouble(vrange[1]) : From;
        double Step = vrange.size() >= 3 ? cdouble(vrange[2]) : 1;
        this->timesteps.addTimesteps(From, To, Step);
      }
    }
  }

  //read all other metadata
  for (auto it : map)
  {
    auto key = it.first; 
    auto value = it.second;
    key = key.substr(1, key.size() - 2); //remove "(...)"
    this->metadata.setValue(key, value);
  }

}



//////////////////////////////////////////////////////////////////////////////
void IdxFile::write(Archive& ar) const
{
  ar.addChild("version")->write("value", version);
  ar.addChild("bitmask")->write("value", bitmask);
  ar.addChild("box")->write("value", logic_box);
  ar.addChild("bitsperblock")->write("value", bitsperblock);
  ar.addChild("blocksperfile")->write("value", blocksperfile);
  ar.addChild("block_interleaving")->write("value", block_interleaving);
  ar.addChild("filename_template")->write("value", filename_template);
  ar.addChild("missing_blocks")->write("value", missing_blocks);
  ar.addChild("time_template")->write("value", time_template);

  auto logic_to_physic = Position::computeTransformation(this->bounds, this->logic_box);
  if (logic_to_physic.isIdentity())
    ;
  else if (logic_to_physic.isOnlyScale())
    ar.addChild("physic_box")->write("value", this->bounds.toAxisAlignedBox());
  else
    ar.addChild("logic_to_physic")->write("value", logic_to_physic);

  for (auto field : fields)
    ar.writeObject("field", field);

  //write timesteps
  timesteps.write(ar);

  for (auto it : metadata)
  {
    auto key = it.first;
    auto value = it.second;
    ar.addChild("metadata")->write("key", key).write("value", value);
  }
  
}

//////////////////////////////////////////////////////////////////////////////
void IdxFile::read(Archive& ar)
{
  ar.getChild("version")->read("value", version);
  ar.getChild("bitmask")->read("value", bitmask);
  ar.getChild("box")->read("value", logic_box);
  ar.getChild("bitsperblock")->read("value", bitsperblock);
  ar.getChild("blocksperfile")->read("value", blocksperfile);
  ar.getChild("block_interleaving")->read("value", block_interleaving);
  ar.getChild("filename_template")->read("value", filename_template);
  ar.getChild("missing_blocks")->read("value", missing_blocks);
  ar.getChild("time_template")->read("value", time_template);

  if (ar.hasAttribute("physic_box"))
  {
    BoxNd value;
    ar.read("physic_box", value);
    this->bounds = Position(value);
  }
  else if (auto child = ar.getChild("physic_box"))
  {
    BoxNd physic_box;
    child->read("value", physic_box);
    this->bounds = Position(physic_box);
  }
  else if (auto child=ar.getChild("logic_to_physic"))
  {
    Matrix logic_to_physic;
    child->read("value", logic_to_physic);
    this->bounds = Position(logic_to_physic, logic_box);
  }
  else
  {
    this->bounds = Position(logic_box);
  }

  for (auto child : ar.getChilds("field"))
  {
    Field field;
    field.read(*child);
    VisusReleaseAssert(field.valid());
    this->fields.push_back(field);
  }

  //read timestep
  this->timesteps.read(ar);


  for (auto it : ar.getChilds("metadata"))
  {
    String key, value;
    it->read("key", key);
    it->read("value", value);
    this->metadata.setValue(key, value);
  }
}


} //namespace Visus
