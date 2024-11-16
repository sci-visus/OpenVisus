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

#ifndef __VISUS_IDX_MULTIPLE_DATASET_H
#define __VISUS_IDX_MULTIPLE_DATASET_H

#include <Visus/Db.h>
#include <Visus/IdxDataset.h>
#include <Visus/Color.h>

namespace Visus {

//////////////////////////////////////////////////////////////////////
class VISUS_DB_API IdxMultipleDataset  : public IdxDataset
{
public:

  VISUS_NON_COPYABLE_CLASS(IdxMultipleDataset)

  enum 
  {
    DebugSaveImages = 0x01,
    DebugSkipReading = 0x02,
    DebugAll = 0xff
  };

  int debug_mode = 0;

  //this is needed for midx
  std::map<String, SharedPtr<Dataset> > down_datasets;

  //constructor
  IdxMultipleDataset();

  //destructor
  virtual ~IdxMultipleDataset();

  //castFrom
  static SharedPtr<IdxMultipleDataset> castFrom(SharedPtr<Dataset> db);

  //getDatasetTypeName
  virtual String getDatasetTypeName() const override {
    return "IdxMultipleDataset";
  }

public:

  //getChild
  SharedPtr<Dataset> getChild(String name) const {
    auto it = down_datasets.find(name); 
    return it == down_datasets.end() ? SharedPtr<Dataset>() : it->second;
  }

  //getFirstDataset
  SharedPtr<Dataset> getFirstChild() const {
    return down_datasets.empty()? SharedPtr<Dataset>() : down_datasets.begin()->second;
  }

  //addChild
  void addChild(String name, SharedPtr<Dataset> value)
  {
    VisusAssert(!down_datasets.count(name));
    down_datasets[name] = value;
  }

public:

  // getFieldEx
  virtual Field getFieldEx(String name) const override;

public:

  //beginBoxQuery
  virtual void beginBoxQuery(SharedPtr<BoxQuery> query) override;

  //nextBoxQuery
  virtual void nextBoxQuery(SharedPtr<BoxQuery> QUERY) override;

  //executeBoxQuery
  virtual bool executeBoxQuery(SharedPtr<Access> ACCESS,SharedPtr<BoxQuery> QUERY) override;

public:

  //getInputName
  static String getInputName(String dataset_name, String fieldname);

  //executeDownQuery
  Array executeDownQuery(BoxQuery* QUERY, SharedPtr<Access> ACCESS, String dataset_name, String fieldname);

  //computeOuput (to override)
  virtual Array computeOuput(BoxQuery* QUERY, SharedPtr<Access> ACCESS, Aborted aborted, String CODE) const {
    ThrowException("not supported");
    return Array();
  }

public:

  //readDatasetFromArchive 
  virtual void readDatasetFromArchive(Archive& ar) override;

private:

  //removeAliases
  String removeAliases(String url);

  //parseDataset
  void parseDataset(StringTree& cur, Matrix T);

  //parseDatasets
  void parseDatasets(StringTree& cur, Matrix T);

  //generateIdxFile
  IdxFile generateIdxFile(Archive& ar);

};

} //namespace Visus

#endif //__VISUS_IDX_MULTIPLE_DATASET_H

