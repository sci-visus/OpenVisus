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

#include <Visus/Idx.h>
#include <Visus/IdxDataset.h>
#include <Visus/Color.h>

namespace Visus {

class VISUS_IDX_API PythonEnginePool;

//////////////////////////////////////////////////////////////////////
class VISUS_IDX_API IdxMultipleDataset  : public IdxDataset
{
public:

  VISUS_NON_COPYABLE_CLASS(IdxMultipleDataset)

  //___________________________________________________
  class VISUS_IDX_API Child
  {
  public:
    String                name;
    Color                 color;
    Matrix                M; //transformation matrix up <- dw
    SharedPtr<IdxDataset> dataset;
    String                mosaic_filename_template;
  };

  //bMosaic
  bool bMosaic = false;

  std::map<String , Child > childs;

  //constructor
  IdxMultipleDataset();

  //destructor
  virtual ~IdxMultipleDataset();

  //getTypeName
  virtual String getTypeName() const override {
    return "IdxMultipleDataset";
  }

  //getChild
  Child getChild(String name) const {
    auto it = childs.find(name); 
    return it == childs.end() ? Child() : it->second;
  }

  //getFirstDataset
  SharedPtr<Dataset> getFirstDataset() const {
    return childs.empty()? SharedPtr<Dataset>() : childs.begin()->second.dataset;
  }

  //addChild
  void addChild(Child value);

  //computeDefaultFields
  void computeDefaultFields();

public:

  //openFromUrl 
  virtual bool openFromUrl(Url URL) override;

  //getInnerDatasets
  virtual std::map<String, SharedPtr<Dataset> > getInnerDatasets() const override;

  // getFieldByNameThrowEx
  virtual Field getFieldByNameThrowEx(String name) const override;

  //createAccess
  virtual SharedPtr<Access> createAccess(StringTree CONFIG=StringTree(), bool bForBlockQuery = false) override;

  //createQueryFilter (not supported?!)
  virtual SharedPtr<DatasetFilter> createQueryFilter(const Field& FIELD) override {
    return SharedPtr<DatasetFilter>();
  }

  //beginQuery
  virtual bool beginQuery(SharedPtr<Query> QUERY) override {
    return IdxDataset::beginQuery(QUERY);
  }

  //executeQuery
  virtual bool executeQuery(SharedPtr<Access> ACCESS,SharedPtr<Query> QUERY) override;

  //nextQuery
  virtual bool nextQuery(SharedPtr<Query> QUERY) override;

  //sameLogicSpace
  bool sameLogicSpace(Child& child) const
  {
    auto vf = child.dataset;
    return child.M.isIdentity() && this->getBox() == vf->getBox() && this->getBitmask() == vf->getBitmask();
  }

public:

  //createIdxFile
  bool createIdxFile(String idx_filename, Field idx_field) const;

private:

  friend class QueryInputTerm;

  SharedPtr<PythonEnginePool> python_engine_pool;

  //getInputName
  String getInputName(String name, String fieldname,bool bIsVarName=false);

  //createField
  Field createField(String operation_name);

  //parseDataset
  void parseDataset(ObjectStream& istream, Matrix4 T);

  //parseDatasets
  void parseDatasets(ObjectStream& istream,Matrix4 T);

  //removeAliases
  String removeAliases(String url);


};

} //namespace Visus

#endif //__VISUS_IDX_MULTIPLE_DATASET_H

