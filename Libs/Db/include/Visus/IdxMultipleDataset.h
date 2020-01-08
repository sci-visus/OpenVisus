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

#ifndef __VISUS_DB_IDX_MULTIPLE_DATASET_H
#define __VISUS_DB_IDX_MULTIPLE_DATASET_H

#include <Visus/Db.h>
#include <Visus/IdxDataset.h>
#include <Visus/Color.h>

namespace Visus {

#if VISUS_PYTHON
class VISUS_DB_API PythonEnginePool;
#endif

//////////////////////////////////////////////////////////////////////
class VISUS_DB_API IdxMultipleDataset  : public IdxDataset
{
public:

  //bMosaic
  bool is_mosaic = false;

  //constructor
  IdxMultipleDataset();

  //destructor
  virtual ~IdxMultipleDataset();

  //getTypeName
  virtual String getTypeName() const override {
    return "IdxMultipleDataset";
  }

  //clone
  virtual SharedPtr<Dataset> clone() const override {
    auto ret = std::make_shared<IdxMultipleDataset>();
    *ret = *this;
    return ret;
  }

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

  //computeDefaultFields
  void computeDefaultFields();

public:

  //read 
  virtual void read(Archive& ar) override;

  //getInnerDatasets
  virtual std::map<String, SharedPtr<Dataset> > getInnerDatasets() const override {
    return down_datasets;
  }

  // getFieldByNameThrowEx
  virtual Field getFieldByNameThrowEx(String name) const override;

  //createAccess
  virtual SharedPtr<Access> createAccess(StringTree CONFIG=StringTree(), bool bForBlockQuery = false) override;

  //createQueryFilter (not supported?!)
  virtual SharedPtr<DatasetFilter> createFilter(const Field& FIELD) override {
    return SharedPtr<DatasetFilter>();
  }

  //beginQuery
  virtual void beginQuery(SharedPtr<BoxQuery> query) override;

  //nextQuery
  virtual void nextQuery(SharedPtr<BoxQuery> QUERY) override;

  //executeQuery
  virtual bool executeQuery(SharedPtr<Access> ACCESS,SharedPtr<BoxQuery> QUERY) override;

public:

  //createIdxFile
  void createIdxFile(String idx_filename, Field idx_field) const;

private:

  friend class QueryInputTerm;

  enum DebugMode
  {
    DebugSaveImages = 0x01,
    DebugSkipReading=0x02,
    DebugAll=0xff
  };

  int debug_mode = 0;

#if VISUS_PYTHON
  SharedPtr<PythonEnginePool> python_engine_pool;
#endif

  //getInputName
  String getInputName(String name, String fieldname);

  //createField
  Field createField(String operation_name);


  //removeAliases
  String removeAliases(String url);

  //parseDataset
  void parseDataset(StringTree& cur, Matrix T);

  //parseDatasets
  void parseDatasets(StringTree& cur,Matrix T);


};

} //namespace Visus

#endif //__VISUS_DB_IDX_MULTIPLE_DATASET_H

