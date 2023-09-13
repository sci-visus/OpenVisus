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

#ifndef __VISUS_DB_IDXFILE_H
#define __VISUS_DB_IDXFILE_H

#include <Visus/Db.h>
#include <Visus/DatasetBitmask.h>
#include <Visus/Field.h>
#include <Visus/DatasetTimesteps.h>
#include <Visus/Matrix.h>
#include <Visus/Position.h>

namespace Visus {


//////////////////////////////////////////////////////////////////////
class VISUS_DB_API IdxFile 
{
public:

  VISUS_CLASS(IdxFile)

  //version
  int version=0;

  //bitmask (example: V010101)
  DatasetBitmask bitmask;

  // bounding box of data stored in the file, with both box.from and box.to included (see Visus(Set/Get)BoxNd) 
  BoxNi logic_box;

  //physic bounds
  Position bounds;

  //for python/bokeh
  String axis;

  // the collection of fields stored inside this dataset (note if this is empty you can still use Dataset::getField(name) for dynamic fields)
  std::vector<Field> fields;

  //timesteps
  DatasetTimesteps timesteps;

  //bitsperblock
  int bitsperblock=0;

  //blocksperfile
  int blocksperfile=0;

  //block interleaving (this is only for a very old format)
  int block_interleaving=0;

  //template for generating filename
  String filename_template;

  //in case there is time
  String time_template;

  //missing_blocks (i.e. some fine blocks could be missing and we need first to interpolate the coarse resolutions, and the insert the fine samples if they exists)
  bool missing_blocks = false;

  //allows other metadata 
  StringMap metadata;

  // adding support for arco
  int arco = 0;
  
  //constructor
  IdxFile(int version_=0);

  //for swig
  static IdxFile clone(const IdxFile& src) {
    IdxFile dst;
    dst = src;
    return dst;
  }

  //setDefaultCompression
  void setDefaultCompression(String value) {
    for (auto& field : this->fields)
      field.default_compression = value;
  }

  //getMaxFieldSize
  int getMaxFieldSize() const {
    int ret = 0;
    for (auto field : this->fields)
      ret = Utils::max(ret, field.dtype.getByteSize());
    return ret;
  }

  //load
  void load(String url,String& TypeName);

  void load(String url) {
    String Typename; return load(url, Typename);
  }

  //save
  void save(String filename);

  //parseFields
  static std::vector<Field> parseFields(String content);

  //getBlockPositionInFile
  Int64 getBlockPositionInFile(BigInt blockid) const
  {
    VisusAssert(blockid >= 0);
    return cint64((blockid / std::max(1, this->block_interleaving)) % this->blocksperfile);
  }

  //getFirstBlockInFile
  BigInt getFirstBlockInFile(BigInt blockid) const
  {
    if (blockid < 0) return -1;
    return blockid - std::max(1, this->block_interleaving)*getBlockPositionInFile(blockid);
  }

  //guessFilenameTemplate
  String guessFilenameTemplate(String url);

  //validate
  void validate(String url);

public:

  //read
  void read(Archive& ar);

  //write
  void write(Archive& ar) const;

  //toString
  String toString() const {
    Archive ar("idxfile");
    write(ar);
    return ar.toString();
  }

  //readFromOldFormat
  void readFromOldFormat(const String& content);

  //writeToOldFormat
  String writeToOldFormat() const;

};

} //namespace Visus

#endif //__VISUS_DB_IDXFILE_H

