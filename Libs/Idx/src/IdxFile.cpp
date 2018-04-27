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
#include <Visus/Encoders.h>
#include <Visus/IdxHzOrder.h>
#include <Visus/IdxDiskAccess.h>
#include <Visus/NetService.h>
#include <Visus/VisusConfig.h>

#define VISUS_IDX_FILE_DEFAULT_VERSION 6

namespace Visus {

  
////////////////////////////////////////////////////////////////////
String IdxFile::guessFilenameTemplate(Url url)
{
  if (!url.valid())
    return "";

  if (this->version!=6) 
  {
    VisusAssert(false);
    return "";
  }

  auto maxh=this->bitmask.getMaxResolution();

  //this is the bits for block number
  int nbits_blocknumber = (maxh - bitsperblock);

  //"./" means where the directory where there is the idx
  //note: for remote I really don't care what's the template
  String ret = url.isFile()? "./" + Path(url.getPath()).getFileNameWithoutExtension() : "./visus_data";

  //can happen if I have only only one block
  if (nbits_blocknumber==0)
  {
    ret += "/%01x.bin";
  }
  else
  {
    //approximate to 4 bits
    if (nbits_blocknumber % 4)
    {
      nbits_blocknumber += (4 - (nbits_blocknumber % 4));
      VisusAssert(!(nbits_blocknumber % 4));
    }

    if (nbits_blocknumber <= 8)
    {
      ret += "/%02x.bin";  //no directories, 256 files
    }
    else if (nbits_blocknumber <= 12)
    {
      ret += "/%03x.bin"; //no directories, 4096 files
    }
    else if (nbits_blocknumber <= 16)
    {
      ret += "/%04x.bin"; //no directories, 65536  files
    }
    else
    {
      while (nbits_blocknumber > 16)
      {
        ret += "/%02x";  //256 subdirectories
        nbits_blocknumber -= 8;
      }

      ret += "/%04x.bin"; //max 65536  files
      nbits_blocknumber -= 16;
      VisusAssert(nbits_blocknumber <= 0);
    }
  }

  return ret;
    
}

//////////////////////////////////////////////////////////////////////////////
void IdxFile::validate(Url url)
{
  //version
  if (this->version==0)
    this->version=VISUS_IDX_FILE_DEFAULT_VERSION;

  if (version<=0)
  {
    VisusInfo()<<"Wrong version("<<version<<")";
    this->version=-1;
    return;
  }

  //box
  if (!box.isFullDim())
  {
    VisusWarning()<<"wrong box("<< box.toOldFormatString()<<")";
    this->version=-1;
    return;
  }

  //bitmask
  if (bitmask.empty())
    bitmask=DatasetBitmask::guess(box.p2);

  if (!bitmask.valid())
  {
    VisusWarning()<<"invalid bitmask";
    this->version=-1;
    return;
  }

  //bitsperblock
  if (bitsperblock==0)
    bitsperblock=std::min(bitmask.getMaxResolution(),16);

  if (bitsperblock<=0)
  {
    VisusWarning()<<"wrong bitsperblock("<<bitsperblock<<")";
    this->version=-1;
    return;
  }

  if (bitsperblock>bitmask.getMaxResolution())
  {
    VisusWarning()<<"bitsperblock="<<bitsperblock<<" is greater than max_resolution="<<bitmask.getMaxResolution();
    this->bitsperblock=bitmask.getMaxResolution();
  }

  //blocksperfile
  if (blocksperfile==0)
  {
    Int64 totblocks=((Int64)1)<<(bitmask.getMaxResolution()-bitsperblock);
    blocksperfile=(int)std::min(totblocks,(Int64)256);
  }

  if (blocksperfile<=0)
  {
    VisusWarning()<<"wrong blockperfile("<<blocksperfile<<")";
    this->version=-1;
    return;
  }

  if (!bitmask.hasRegExpr() && blocksperfile>(((BigInt)1)<<(bitmask.getMaxResolution()-bitsperblock)))
  {
    VisusWarning()<<"wrong blockperfile("<<blocksperfile<<"), with bitmask.getMaxResolution()="<<bitmask.getMaxResolution()<<" and bitsperblock("<<bitsperblock<<")";
    
    if (this->version<=6)
    {
      //VisusAssert(false);
      ; //DO NOTHING... there are idx files around with a wrong blocksperfile (and a bigger header)
    }
    else
    {
      this->blocksperfile=1<<(bitmask.getMaxResolution()-bitsperblock);  
    }
  }

  //check fields
  if (fields.empty())
  {
    VisusWarning()<<"no fields";
    this->version=-1;
    return;
  }

  for (int N=0;N<(int)this->fields.size();N++)
  {
    Field& field=this->fields[N];

    //old format stores multiple fields in the same file
    if (version>=1 && version<=6)
      field.index=cstring(N);

    if (!field.valid())
    {
      VisusWarning()<<"wrong field("<<field.name<<")";
      this->version=-1;
      return;
    }

    //if not specified it means hzorder
    if (field.default_layout.empty() || field.default_layout=="hzorder" || field.default_layout=="0")
    {
      field.default_layout="hzorder";
    }
    //use "rowmajor" or "1" to force row major
    else if (field.default_layout=="rowmajor" || field.default_layout=="1")
    {
      field.default_layout=""; //empty will mean rowmajor
    }
    else
    {
      VisusAssert(false);
      VisusWarning()<<"unknown field.default_layout("<<field.default_layout<<")";
      field.default_layout="hzorder";
    }

    //check that if we use a LOSSY (for example jpg) I need QueryData::RowMajor
    bool bRowMajor=field.default_layout.empty();
    if (!bRowMajor && Encoders::getSingleton()->getEncoder(field.default_compression)->isLossy())
    {
      VisusWarning()<<"The field "<<field.name<<" has default_compression!=zip but !field.default_layout.empty()";
      field.default_compression="zip";
    }
  }

  //filename_template
  if (filename_template.empty())
    filename_template=guessFilenameTemplate(url);

  if (filename_template.empty())
  {
    VisusWarning()<<"wrong filename_template("<<filename_template<<")";
    this->version=-1;
    return;
  }

  //replace some alias
  if (url.valid() && url.isFile() && (StringUtils::contains(filename_template,"$(CurrentFileDirectory)") || StringUtils::startsWith(filename_template,"./")))
  {
    String CurrentFileDirectory=Path(url.getPath()).getParent().toString();
    if (!CurrentFileDirectory.empty())
    {
      if (StringUtils::startsWith(this->filename_template,"./"))
        this->filename_template = StringUtils::replaceFirst(this->filename_template,"./",CurrentFileDirectory + "/");

      this->filename_template = StringUtils::replaceAll(this->filename_template,"$(CurrentFileDirectory)",CurrentFileDirectory);
    }
  }  
}

//////////////////////////////////////////////////////////////////////////////
static String parseRoundBracketArgument(String s,String name)
{
  String arg=StringUtils::nextToken(s,name+"(");
  return arg.empty()?"":StringUtils::trim(StringUtils::split(arg,")")[0]);
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
    if (!(last=StringUtils::trim(last)).empty()) v.push_back(last);last="";
  }

  std::vector<Field> ret;

  for (int I=0;I<(int)v.size();I++)
  {
    String sfield=v[I];
    std::istringstream ss(sfield);

    Field field;

    //name
    {
      ss>>field.name;
      if (field.name.empty())
        return std::vector<Field>();
    }

    //dtype
    {
      String string_dtype;
      ss>>string_dtype;
      field.dtype=DType::fromString(string_dtype);
      if (!field.dtype.valid()) 
        return std::vector<Field>();
    }

    //description
    field.description=parseRoundBracketArgument(sfield,"description");

    //default_compression
    {
      field.default_compression=parseRoundBracketArgument(sfield,"default_compression");

      //backward compatible: compressed(...) | compressed
      if (field.default_compression.empty())
      {
        field.default_compression=parseRoundBracketArgument(sfield,"compressed");

        if (field.default_compression.empty() && StringUtils::contains(sfield,"compressed"))
          field.default_compression="zip";
      }
    }

    //default_layout
    {
      field.default_layout=parseRoundBracketArgument(sfield,"default_layout");

      //backward compatible: format(...)
      if (field.default_layout.empty())
        field.default_layout=parseRoundBracketArgument(sfield,"format");
    }

    //default_value
    {
      field.default_value=cint(parseRoundBracketArgument(sfield,"default_value"));
    }

    //filter(...)
    field.filter=parseRoundBracketArgument(sfield,"filter");

    //min(...) max(...)
    {
      auto vmin = StringUtils::split(parseRoundBracketArgument(sfield,"min"));
      auto vmax = StringUtils::split(parseRoundBracketArgument(sfield,"max"));
      if (!vmin.empty() && !vmax.empty())
      {
        for (int C=0;C<field.dtype.ncomponents();C++) 
        {
          auto From=cdouble(vmin[C<vmin.size()? C : vmin.size()-1]);
          auto To  =cdouble(vmax[C<vmax.size()? C : vmax.size()-1]);
          field.setDTypeRange(Range(From,To,0),C);
        }
      }
    }

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
IdxFile IdxFile::openFromUrl(Url url)
{
  String content;
  {
    //special case for cloud storage, I need to sign the request
    if (url.isRemote() && !StringUtils::contains(url.toString(),"mod_visus"))
    {
      UniquePtr<CloudStorage> cloud_storage(CloudStorage::createInstance(url)); VisusAssert(cloud_storage);
      if (cloud_storage)
        content=NetService::getNetResponse(cloud_storage->createGetBlobRequest(url)).getTextBody();
    }
    else
    {
      content=Utils::loadTextDocument(url.toString());
    }
  }

  if (content.empty())
    return IdxFile::invalid();

  IdxFile idxfile;

  //old text format
  if (StringUtils::startsWith(content,"(version)"))
  {
    //parse the idx text format
    StringMap map;

    std::vector<String> v=StringUtils::getLinesAndPurgeComments(content,"#");
    String key,value;
    for (int I=0;I<(int)v.size();I++)
    {
      String line=StringUtils::trim(v[I]);
      if (line.empty()) continue;
      if (StringUtils::startsWith(line,"("))
        {if (!key.empty()) map.setValue(key,StringUtils::trim(value));key=line;value=String();}
      else
        {value=value + " " + line;}
    }
    if (!key.empty()) 
      map.setValue(key,StringUtils::trim(value));

    //trim the values
    for (auto it=map.begin();it!=map.end();it++)
      it->second=StringUtils::trim(it->second);

    idxfile.version     = cint(map.getValue("(version)")); VisusAssert(idxfile.version>=1 && idxfile.version<=6);
    idxfile.bitmask     = DatasetBitmask(map.getValue("(bits)"));
    idxfile.box         = NdBox::parseFromOldFormatString(idxfile.bitmask.getPointDim(),map.getValue("(box)"));

    //parse fields
    if (map.hasValue("(fields)"))
      idxfile.fields=parseFields(map.getValue("(fields)"));

    idxfile.bitsperblock      = cint(map.getValue("(bitsperblock)"));
    idxfile.blocksperfile     = cint(map.getValue("(blocksperfile)"));
    idxfile.filename_template = map.getValue("(filename_template)");
    idxfile.scene             = map.getValue("(scene)");

    if (map.hasValue("(interleave)"))
      idxfile.block_interleaving=cint(map.getValue("(interleave)"));
  
    if (map.hasValue("(time)"))
    {
      std::vector<String> vtime=StringUtils::split(map.getValue("(time)")," "); 
      VisusAssert(vtime.size()>=2);

      idxfile.timesteps=DatasetTimesteps();
      double parse_time=0;

      ///star format (means I don't know in advance the timesteps)
      //* * time_template 
      if (vtime[0]=="*")
      {
        bool bGood=vtime.size()==3 && vtime[1]=="*";
        if (!bGood) {VisusInfo()<<"idx (time) is wrong";VisusAssert(false);return IdxFile::invalid();}
        idxfile.time_template =vtime[2];
      }
      //old format
      //From To time_template
      else if (StringUtils::tryParse(vtime[0],parse_time))
      {
        bool bGood=vtime.size()==3 && StringUtils::tryParse(vtime[1],parse_time);
        if (!bGood) {VisusInfo()<<"idx (time) is wrong";VisusAssert(false);return IdxFile::invalid();}
        double From=cdouble(vtime[0]);
        double To  =cdouble(vtime[1]);
        idxfile.timesteps.addTimesteps(From,To,1.0);
        idxfile.time_template =vtime[2];
      }
      //new format
      //time_template (From,To,Step) (From,To,Step) (From,To,Step)
      else
      {
        idxfile.time_template =vtime[0];
        for (int I=1;I<(int)vtime.size();I++)
        {
          bool bGood=StringUtils::startsWith(vtime[I],"(") && StringUtils::find(vtime[I],")");
          if (!bGood) {VisusInfo()<<"idx (time) is wrong";VisusAssert(false);return IdxFile::invalid();}
          std::vector<String> vrange=StringUtils::split(vtime[I].substr(1,vtime[I].size()-2),",",true);
          double From=vrange.size()>=1? cdouble(vrange[0]) : 0;
          double To  =vrange.size()>=2? cdouble(vrange[1]) : From;
          double Step=vrange.size()>=3? cdouble(vrange[2]) : 1;
          idxfile.timesteps.addTimesteps(From,To,Step);
        }
      }
    }

    idxfile.validate(url);
  }
  //new xml format
  else
  {
    StringTree stree;
    if (!stree.loadFromXml(content))
    {
      VisusInfo()<<"idx file is wrong";
      VisusAssert(false);
      return IdxFile::invalid();
    }

    ObjectStream istream(stree,'r');
    idxfile.readFromObjectStream(istream);
    idxfile.validate(url);
    if (!idxfile.valid())
    {
      VisusInfo()<<"idx file is wrong ";
      return IdxFile::invalid();
    }
  }

  return idxfile;
}

//////////////////////////////////////////////////////////////////////////////
bool IdxFile::save(String filename)
{
  if (filename.empty())
    return false;

  //the user is trying to create a new IdxFile... help him by guessing and checking some values
  if (version==0)
    validate(Url(filename));

  if (!valid())
    return false;

  this->saving_filename=filename;
  String content=toString();
  this->saving_filename="";
  if (content.empty()) 
    return false;

  //save the file (using file locks... there could be tons of visus running!)
  {
    FileUtils::lock(filename);

    bool bOk = Utils::saveTextDocument(filename, content);
    FileUtils::unlock(filename);

    if (!bOk)
    {
      VisusWarning()<<"Utils::saveTextDocument("<<filename<<",content) failed";
      return false;
    }
  }

  return true;
}



/////////////////////////////////////////////////////////////////////////////
String IdxFile::toString() const
{
  if (!valid())
    return "";

  //old idx file format
  if (version>=1 && version<=6)
  {
    std::ostringstream out;

    out<<"(version)\n"<<this->version<<"\n";
    out<<"(box)\n"<< this->box.toOldFormatString()<<"\n";
  
    //dump fields
    out<<"(fields)\n";
    for (int i=0;i<(int)this->fields.size();i++) 
    {
      const Field& field=this->fields[i];

      out<<(i?"+ ":"");
      out<<field.name<<" ";
      out<<field.dtype.toString()<<" ";

      //compressed | compressed(...)  
      if (!field.default_compression.empty())
      {
        if (field.default_compression=="zip")
          out<<"compressed"<<" ";
        else
          out<<"compressed("<<field.default_compression<<")"<<" ";
      }

      //format(...)
      out<<"format("<<(field.default_layout.empty()?"1":"0")<<")"<<" ";

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
      //star format
      //* * time_template 
      if (this->timesteps==DatasetTimesteps::star())
      {
        out<<"(time)\n"<<"*"<<" "<<"*"<<" "<<this->time_template<<"\n";
      }
      else
      {
        //old format
        //From To time_template
        if (this->timesteps==DatasetTimesteps(this->timesteps.getMin(),this->timesteps.getMax(),1.0))
        {
          out<<"(time)\n"<<this->timesteps.getMin()<<" "<<this->timesteps.getMax()<<" "<<this->time_template<<"\n";
        }
        //new format
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

    String filename_template=this->filename_template;

    //fix absolute path -> ./
    if (!saving_filename.empty())
    {
      String saving_directory=Path(saving_filename).getParent().toString();
      if (StringUtils::startsWith(filename_template,saving_directory))
        filename_template=StringUtils::replaceFirst(filename_template,saving_directory,".");
    }
  
    if(this->scene!="")
      out<<"(scene)\n"<<this->scene<<"\n";
    
    out<<"(filename_template)\n"<<filename_template<<"\n";

    return out.str();
  }
  else
  {
    StringTree stree("IdxFile");
    ObjectStream ostream(stree,'w');
    const_cast<IdxFile*>(this)->writeToObjectStream(ostream);
    return stree.toString();
  }
}

//////////////////////////////////////////////////////////////////////////////
void IdxFile::writeToObjectStream(ObjectStream& ostream)
{
 if (!this->valid())
    ThrowException("internal error");

  ostream.write("version",cstring(this->version));
  ostream.write("box", box.toOldFormatString());
  ostream.write("bitmask",this->bitmask.toString());
  ostream.write("bitsperblock",cstring(this->bitsperblock));
  ostream.write("blocksperfile",cstring(this->blocksperfile));
  ostream.write("block_interleaving",cstring(this->block_interleaving));
  if(scene!="")
    ostream.write("scene",scene);
  ostream.write("filename_template",filename_template);
  
  ostream.pushContext("fields");
  for (int I=0;I<(int)fields.size();I++)
  {
    ostream.pushContext("field");
    fields[I].writeToObjectStream(ostream);
    ostream.popContext("field");
  }
  ostream.popContext("fields");

  if (!this->time_template.empty())
  {
    ostream.pushContext("Timesteps");
    ostream.write("filename_template",this->time_template);
    timesteps.writeToObjectStream(ostream);
    ostream.popContext("Timesteps");
  }
}


//////////////////////////////////////////////////////////////////////////////
void IdxFile::readFromObjectStream(ObjectStream& istream)
{
  this->version           = cint(istream.read("version"));
  this->bitmask = DatasetBitmask(istream.read("bitmask"));
  this->box               = NdBox::parseFromOldFormatString(this->bitmask.getPointDim(),istream.read("box"));
  this->bitsperblock      = cint(istream.read("bitsperblock"));
  this->blocksperfile     = cint(istream.read("blocksperfile"));
  this->block_interleaving= cint(istream.read("block_interleaving"));
  this->scene             = istream.read("scene");
  this->filename_template = istream.read("filename_template");

  this->fields.clear();
  istream.pushContext("fields");
  while (istream.pushContext("field"))
  {
    Field field;field.readFromObjectStream(istream);
    if (!field.valid())
      ThrowException("field not valid");

    this->fields.push_back(field);
    istream.popContext("field");
  }
  istream.popContext("fields");

  if (istream.pushContext("Timesteps"))
  {
    this->time_template=istream.read("filename_template");
    this->timesteps.readFromObjectStream(istream);
    istream.popContext("Timesteps");
  }
}


} //namespace Visus
