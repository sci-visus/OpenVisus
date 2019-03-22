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

#ifndef __VISUS_IDX_FILE_H
#define __VISUS_IDX_FILE_H

#include <Visus/Idx.h>
#include <Visus/DatasetBitmask.h>
#include <Visus/Field.h>
#include <Visus/DatasetTimesteps.h>
#include <Visus/Url.h>

namespace Visus {


//////////////////////////////////////////////////////////////////////
class VISUS_IDX_API IdxFile 
{
public:

  VISUS_CLASS(IdxFile)

  //version
  int version=0;

  //bitmask (example: 010101{01}*)
  DatasetBitmask bitmask;

  // bounding box of data stored in the file, with both box.from and box.to included (see Visus(Set/Get)Box) 
  NdBox box;

  // the collection of fields stored inside this dataset (note if this is empty you can still use Dataset::getFieldByName(name) for dynamic fields)
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
  
  //in case there is scene
  String scene;

  //constructor
  IdxFile(int version_=0);

  //invalid
  static IdxFile invalid() {
    return IdxFile(-1);
  }

  //openFromUrl
  static IdxFile openFromUrl(Url url);

  //valid
  bool valid() const {
    return this->version>0;
  }

  //save
  bool save(String filename);

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


  //validate
  void validate(Url url);

  //toString
  String toString() const ;

  //writeToObjectStream
  void writeToObjectStream(ObjectStream& ostream) ;

  //readFromObjectStream
  void readFromObjectStream(ObjectStream& istream) ; 

private:

  //guessFilenameTemplate
  String guessFilenameTemplate(Url url);

  //saving_filename
  String saving_filename;

};

} //namespace Visus

#endif //__VISUS_IDX_FILE_H

