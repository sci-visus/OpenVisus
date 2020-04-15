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

#ifndef __VISUS_DB_DATASET_H
#define __VISUS_DB_DATASET_H

#include <Visus/Db.h>
#include <Visus/StringMap.h>
#include <Visus/Frustum.h>
#include <Visus/Access.h>
#include <Visus/BlockQuery.h>
#include <Visus/BoxQuery.h>
#include <Visus/PointQuery.h>
#include <Visus/DatasetBitmask.h>
#include <Visus/DatasetTimesteps.h>
#include <Visus/Path.h>
#include <Visus/NetMessage.h>
#include <Visus/Annotation.h>

namespace Visus {

//predeclaration
class RamAccess;


////////////////////////////////////////////////////////
class VISUS_DB_API KdQueryMode
{
public:

  enum
  {
    NotSpecified = 0x00,
    UseBlockQuery = 0x01,
    UseBoxQuery = 0x02
  };

  //fromString
  static int fromString(String value)
  {
    value = StringUtils::trim(StringUtils::toLower(value));

    if (value == "block")
      return KdQueryMode::UseBlockQuery;

    if (value == "box")
      return KdQueryMode::UseBoxQuery;

    if (cbool(value))
      return KdQueryMode::UseBlockQuery;

    return KdQueryMode::NotSpecified;
  }

  //toString
  static String toString(int value)
  {
    VisusAssert(value >= 0 && value < 3);
    switch (value)
    {
    case 0: return "";
    case 1: return "block";
    case 2: return "box";
    default: return "";
    }
  }

private:

  KdQueryMode() = delete;
};

////////////////////////////////////////////////////////
class VISUS_DB_API Dataset 
{
public:

  //this is needed for midx
  Color color;


  //this is needed for midx
  std::map<String, SharedPtr<Dataset> > down_datasets;


  //this is needed for midx
  Matrix logic_to_LOGIC;

  //annotations
  SharedPtr<Annotations> annotations;

  //constructor
  Dataset() {
  }

  //destructor
  virtual ~Dataset() {
  }

  //getTypeName
  virtual String getTypeName() const = 0;

  //cloneclone
  virtual SharedPtr<Dataset> clone() const = 0;

  //copyDataset
  static void copyDataset(
    Dataset* Dvf, SharedPtr<Access> Daccess, Field Dfield, double Dtime,
    Dataset* Svf, SharedPtr<Access> Saccess, Field Sfield, double Stime);

  //isServerMode
  bool isServerMode() const {
    return bServerMode;
  }

  //setServerMode
  void setServerMode(bool value) {
    this->bServerMode = value;
  }

  //getTimesteps
  const DatasetTimesteps& getTimesteps() const {
    return timesteps;
  }

  //setTimesteps
  void setTimesteps(const DatasetTimesteps& value) {
    this->timesteps = value;
  }

  //getDefaultTime
  double getDefaultTime() const {
    return getTimesteps().getDefault();
  }

  //getAccessConfigs
  std::vector< SharedPtr<StringTree> > getAccessConfigs() const {
    return getDatasetBody().getChilds("access");
  }

  //getDefaultAccessConfig
  StringTree getDefaultAccessConfig() const  {
    auto v = getAccessConfigs();
    return v.empty() ? StringTree() : *v[0];
  }

  //getKdQueryMode
  int getKdQueryMode() const {
    return kdquery_mode;
  }

  //setKdQueryMode
  void setKdQueryMode(int value) {
    kdquery_mode = value;
  }

  //getDefaultField
  Field getDefaultField() const {
    return fields.empty()? Field() : fields.front();
  }
  
  //getDatasetBody
  StringTree& getDatasetBody() {
    return dataset_body;
  } 

  //getDatasetBody
  const StringTree& getDatasetBody() const {
    return dataset_body;
  }

  //setDatasetBody
  void setDatasetBody(const StringTree& value) {
    this->dataset_body = value;
  }

  //getUrl
  String getUrl() const {
    return getDatasetBody().getAttribute("url");
  }

  // getDatasetInfos
  String getDatasetInfos() const;

public:

  //clearFields
  void clearFields() {
    this->fields.clear();
    this->find_field.clear();
  }

  //getFields
  std::vector<Field>& getFields() {
    return fields;
  }

  //addField
  void addField(String name, Field field) {
    fields.push_back(field);
    find_field[name] = field;
  }

  //addField
  void addField(Field field) {
    addField(field.name, field);
  }

  // getFieldByNameThrowEx
  virtual Field getFieldByNameThrowEx(String name) const;

  // getFieldByName
  Field getFieldByName(String name) const;

public:

  //getPointDim
  int getPointDim() const {
    return bitmask.getPointDim();
  }

  //getBitmask
  const DatasetBitmask& getBitmask() const {
    return bitmask;
  }

  //setLogicBitmask
  void setBitmask(const DatasetBitmask& value) {
    this->bitmask = value;
  }

  //getBox
  const BoxNi& getLogicBox() const {
    return logic_box;
  }

  //setBox
  void setLogicBox(const BoxNi& value) {
    this->logic_box = value;
    VisusAssert(!dataset_bounds.valid());
  }

public:

  //getDatasetBounds
  Position getDatasetBounds() const {
    return dataset_bounds;
  }

  //setDatasetBounds (to call after setLogicBox())
  void setDatasetBounds(Position value) {
    VisusAssert(this->logic_box.valid());
    this->dataset_bounds = value;
    this->logic_to_physic = Position::computeTransformation(value, getLogicBox());
    this->physic_to_logic = this->logic_to_physic.invert();
  }

  //logicToPhysic
  Matrix logicToPhysic() const {
    return logic_to_physic;
  }

  //physicToLogic
  Matrix physicToLogic() const {
    return physic_to_logic;
  }

  //logicToPhysic
  Position logicToPhysic(Position logic) const {
    return Position(logicToPhysic(),logic);
  }

  //physicToLogic
  Position physicToLogic(Position physic) const {
    return Position(physicToLogic(), physic);
  }

  //logicToScreen
  Frustum logicToScreen(Frustum physic_to_screen) const {
    return Frustum(physic_to_screen, logicToPhysic());
  }

  //physicToScreen
  Frustum physicToScreen(Frustum logic_to_screen) const {
    return Frustum(logic_to_screen, physicToLogic());
  }

public:

  //getDefaultBitsPerBlock
  int getDefaultBitsPerBlock() const {
    return default_bitsperblock;
  }

  //setDefaultBitsPerBlock
  void setDefaultBitsPerBlock(int value) {
    this->default_bitsperblock = value;
  }

  //getTotalNumberOfBlocks
  BigInt getTotalNumberOfBlocks() const {
    return (((BigInt)1) << getMaxResolution()) / (((Int64)1) << getDefaultBitsPerBlock());
  }

  //getMaxResolution
  int getMaxResolution() const {
    return bitmask.getMaxResolution();
  }

  //getLevelSamples
  virtual LogicSamples getLevelSamples(int H) = 0;

  //getAddressRangeSamples
  virtual LogicSamples getAddressRangeSamples(BigInt start_address, BigInt end_address) = 0;

public:

  //read 
  virtual void read(Archive& ar) = 0;

  //compressDataset
  virtual bool compressDataset(String compression) {
    ThrowException("compression not enabled");
    return false;
  }

  //getInnerDatasets
  virtual std::map<String,SharedPtr<Dataset> > getInnerDatasets() const  {
    return std::map<String,SharedPtr<Dataset> >();
  }

public:

  //createAccess
  virtual SharedPtr<Access> createAccess(StringTree config = StringTree(), bool bForBlockQuery = false);
  
  //createAccessForBlockQuery
  SharedPtr<Access> createAccessForBlockQuery(StringTree config = StringTree()) {
    return createAccess(config, true);
  }

  //createRamAccess
  SharedPtr<Access> createRamAccess(Int64 available, bool can_read = true, bool can_write = true);

  //readBlock  
  virtual Future<Void> executeBlockQuery(SharedPtr<Access> access, SharedPtr<BlockQuery> query);

  //executeBlockQueryAndWait
  bool executeBlockQueryAndWait(SharedPtr<Access> access, SharedPtr<BlockQuery> query) {
    executeBlockQuery(access, query).get(); return query->ok();
  }

  //convertBlockQueryToRowMajor
  virtual bool convertBlockQueryToRowMajor(SharedPtr<BlockQuery> block_query) {
    return false;
  }

  //createFilter (default: not supported)
  virtual SharedPtr<DatasetFilter> createFilter(const Field& field) {
    return SharedPtr<DatasetFilter>();
  }


public:

  ////////////////////////////////////
  //box query

  virtual void beginQuery(SharedPtr<BoxQuery> query) {
  }

  //nextQuery
  virtual void nextQuery(SharedPtr<BoxQuery> query) {
  }

  //executeQuery
  virtual bool executeQuery(SharedPtr<Access> access,SharedPtr<BoxQuery> query) {
    return false;
  }

  //mergeBoxQueryWithBlock
  virtual bool mergeBoxQueryWithBlock(SharedPtr<BoxQuery> query, SharedPtr<BlockQuery> block_query){
    return false;
  }

  //createBoxQueryRequest
  virtual NetRequest createBoxQueryRequest(SharedPtr<BoxQuery> query) {
    return NetRequest();
  }

  //executeBoxQueryOnServer
  virtual bool executeBoxQueryOnServer(SharedPtr<BoxQuery> query);

public:

  ////////////////////////////////////
  //point query

  //guessPointQueryNumberOfSamples
  PointNi guessPointQueryNumberOfSamples(const Frustum& logic_to_screen, Position logic_position, int end_resolution);

  //beginQuery
  virtual void beginQuery(SharedPtr<PointQuery> query) {
  }

  //executeQuery
  virtual bool executeQuery(SharedPtr<Access> access, SharedPtr<PointQuery> query) {
    return false;
  }

  //createPointQueryRequest
  virtual NetRequest createPointQueryRequest(SharedPtr<PointQuery> query) {
    return NetRequest();
  }

  //executePointQueryOnServer
  virtual bool executePointQueryOnServer(SharedPtr<PointQuery> query);

public:

  //generateTiles (useful for conversion)
  std::vector<BoxNi> generateTiles(int TileSize) const;

  //readFullResolutionData
  Array readFullResolutionData(SharedPtr<Access> access, Field field, double time, BoxNi logic_box =BoxNi());

  //writeFullResolutionData
  bool writeFullResolutionData(SharedPtr<Access> access, Field field, double time, Array buffer,BoxNi logic_box =BoxNi());

  //extractLevelImage
  Array extractLevelImage(SharedPtr<Access> access, Field field, double time, int H);

private:

  StringTree              dataset_body;
  DatasetTimesteps        timesteps;
  std::vector<Field>      fields;
  std::map<String, Field> find_field;
  DatasetBitmask          bitmask;
  BoxNi                   logic_box;
  Position                dataset_bounds = Position::invalid();
  Matrix                  logic_to_physic, physic_to_logic;
  int                     kdquery_mode = KdQueryMode::NotSpecified;
  bool                    bServerMode = false;
  int                     default_bitsperblock = 0;
};


////////////////////////////////////////////////////////////////
class VISUS_DB_API DatasetFactory
{
public:

  VISUS_DECLARE_SINGLETON_CLASS(DatasetFactory)

  typedef std::function< SharedPtr<Dataset>() > CreateInstance;

  class VISUS_DB_API RegisteredDataset
  {
  public:
    String TypeName;
    CreateInstance createInstance;

  };

  //registerDatasetType
  void registerDatasetType(String TypeName, CreateInstance createInstance)
  {
    RegisteredDataset item;
    item.TypeName = TypeName;
    item.createInstance = createInstance;
    v.push_back(item);
  }

  //createInstance
  SharedPtr<Dataset> createInstance(String TypeName) {
    if (TypeName.empty())
      return SharedPtr<Dataset>();

    for (const auto& it : v) {
      if (it.TypeName == TypeName)
        return it.createInstance();
    }
    return SharedPtr<Dataset>();
  }

private:

  std::vector<RegisteredDataset> v;

  DatasetFactory(){}

};

VISUS_DB_API StringTree FindDatasetConfig(StringTree ar, String url);

//LoadDatasetEx
VISUS_DB_API SharedPtr<Dataset> LoadDatasetEx(StringTree ar);

VISUS_DB_API SharedPtr<Dataset> LoadDataset(String url);


template <class T>
inline SharedPtr<T> LoadDataset(String url) {
  return std::dynamic_pointer_cast<T>(LoadDataset(url));
}

} //namespace Visus


#endif //__VISUS_DB_DATASET_H

