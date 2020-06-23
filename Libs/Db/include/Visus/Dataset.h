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
  Matrix logic_to_LOGIC;

  //annotations
  SharedPtr<Annotations> annotations;

  //constructor
  Dataset() {
  }

  //destructor
  virtual ~Dataset() {
  }

  //getDatasetTypeName
  virtual String getDatasetTypeName() const = 0;

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

  //getTime
  double getTime() const {
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

  //getDefaultBitsPerBlock
  int getDefaultBitsPerBlock() const {
    return default_bitsperblock;
  }

  //setDefaultBitsPerBlock
  void setDefaultBitsPerBlock(int value) {
    this->default_bitsperblock = value;
  }


  //getMaxResolution
  int getMaxResolution() const {
    return bitmask.getMaxResolution();
  }

  //getTotalNumberOfBlocks
  BigInt getTotalNumberOfBlocks() const {
    return (((BigInt)1) << getMaxResolution()) / (((Int64)1) << getDefaultBitsPerBlock());
  }

public:

  //________________________________________________
  //position stuff

  //getBox
  const BoxNi& getLogicBox() const {
    return logic_box;
  }

  //setBox
  void setLogicBox(const BoxNi& value) {
    this->logic_box = value;
    VisusAssert(!dataset_bounds.valid());
  }

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
    return Position(logicToPhysic(), logic);
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

  //________________________________________________
  //fields stuff

  //getFields
  std::vector<Field>& getFields() {
    return fields;
  }

  //getField
  Field getField() const {
    return fields.empty() ? Field() : fields.front();
  }

  // getField
  Field getField(String name) const;

  // getFieldEx
  virtual Field getFieldEx(String name) const;

  //addField
  void addField(String name, Field field) {
    fields.push_back(field);
    find_field[name] = field;
  }

  //addField
  void addField(Field field) {
    addField(field.name, field);
  }

  //clearFields
  void clearFields() {
    this->fields.clear();
    this->find_field.clear();
  }

public:

  //________________________________________________
  //block query stuff

  //createAccess
  virtual SharedPtr<Access> createAccess(StringTree config = StringTree(), bool bForBlockQuery = false);
  
  //createAccessForBlockQuery
  SharedPtr<Access> createAccessForBlockQuery(StringTree config = StringTree()) {
    return createAccess(config, true);
  }

  //createRamAccess
  SharedPtr<Access> createRamAccess(Int64 available, bool can_read = true, bool can_write = true);

  //getBlockSamples
  virtual LogicSamples getBlockSamples(BigInt blockid) = 0;

  //createBlockQuery
  SharedPtr<BlockQuery> createBlockQuery(BigInt blockid, Field field, double time, int mode = 'r', Aborted aborted = Aborted());

  //createBlockQuery
  SharedPtr<BlockQuery> createBlockQuery(BigInt blockid, int mode = 'r', Aborted aborted = Aborted()) {
    return createBlockQuery(blockid, getField(), getTime(), mode, aborted);
  }

  //readBlock  
  virtual void executeBlockQuery(SharedPtr<Access> access, SharedPtr<BlockQuery> query);

  //executeBlockQueryAndWait
  bool executeBlockQueryAndWait(SharedPtr<Access> access, SharedPtr<BlockQuery> query) {
    executeBlockQuery(access, query);
    query->done.get(); 
    return query->ok();
  }

  //convertBlockQueryToRowMajor
  virtual bool convertBlockQueryToRowMajor(SharedPtr<BlockQuery> block_query) {
    return false;
  }

public:

  //________________________________________________
  //box query stuff

  //createBoxQuery
  SharedPtr<BoxQuery> createBoxQuery(BoxNi logic_box, Field field, double time, int mode = 'r', Aborted aborted = Aborted());

  //createBoxQuery
  SharedPtr<BoxQuery> createBoxQuery(BoxNi logic_box, int mode = 'r', Aborted aborted = Aborted()) {
    return createBoxQuery(logic_box, getField(), getTime(), mode, aborted);
  }

  virtual void beginBoxQuery(SharedPtr<BoxQuery> query) {
  }

  //nextBoxQuery
  virtual void nextBoxQuery(SharedPtr<BoxQuery> query) {
  }

  //executeBoxQuery
  virtual bool executeBoxQuery(SharedPtr<Access> access,SharedPtr<BoxQuery> query) {
    return false;
  }

  //mergeBoxQueryWithBlockQuery
  virtual bool mergeBoxQueryWithBlockQuery(SharedPtr<BoxQuery> query, SharedPtr<BlockQuery> block_query){
    return false;
  }

  //createBoxQueryRequest
  virtual NetRequest createBoxQueryRequest(SharedPtr<BoxQuery> query) {
    return NetRequest();
  }

  //executeBoxQueryOnServer
  virtual bool executeBoxQueryOnServer(SharedPtr<BoxQuery> query);

public:

  //________________________________________________
  //point query stuff

  //guessPointQueryNumberOfSamples
  PointNi guessPointQueryNumberOfSamples(const Frustum& logic_to_screen, Position logic_position, int end_resolution);

  //constructor
  SharedPtr<PointQuery> createPointQuery(Position logic_position, Field field, double time, Aborted aborted = Aborted());

  //createPointQuery
  SharedPtr<PointQuery> createPointQuery(Position logic_position, Aborted aborted=Aborted()) {
    return createPointQuery(logic_position, getField(), getTime(), aborted);
  }

  //beginPointQuery
  virtual void beginPointQuery(SharedPtr<PointQuery> query) {
  }

  //executePointQuery
  virtual bool executePointQuery(SharedPtr<Access> access, SharedPtr<PointQuery> query) {
    return false;
  }

  //createPointQueryRequest
  virtual NetRequest createPointQueryRequest(SharedPtr<PointQuery> query) {
    return NetRequest();
  }

  //executePointQueryOnServer
  virtual bool executePointQueryOnServer(SharedPtr<PointQuery> query);

public:

  //insertSamples
  static bool insertSamples(
    LogicSamples Wsamples, Array Wbuffer,
    LogicSamples Rsamples, Array Rbuffer, Aborted aborted);

  static bool interpolateSamples(
    LogicSamples Wsamples, Array Wbuffer,
    LogicSamples Rsamples, Array Rbuffer, Aborted aborted);

public:

  //readDatasetFromArchive 
  virtual void readDatasetFromArchive(Archive& ar) = 0;

protected:

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

  //registerDatasetType
  void registerDatasetType(String TypeName, CreateInstance createInstance) {
    map[TypeName] = createInstance;
  }

  //createInstance
  SharedPtr<Dataset> createInstance(String TypeName) {
    auto it = map.find(TypeName);
    return it == map.end() ? SharedPtr<Dataset>() : it->second();
  }

private:

  std::map<String, CreateInstance> map;

  DatasetFactory(){}

};

VISUS_DB_API StringTree FindDatasetConfig(StringTree ar, String url);

//LoadDatasetEx
VISUS_DB_API SharedPtr<Dataset> LoadDatasetEx(StringTree ar);

VISUS_DB_API SharedPtr<Dataset> LoadDataset(String url);

} //namespace Visus


#endif //__VISUS_DB_DATASET_H

