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
#include <Visus/IdxFile.h>

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

    if (value.empty())
      return KdQueryMode::NotSpecified;

    if (StringUtils::contains(value,"block") || value == "1" || value == "true")
      return KdQueryMode::UseBlockQuery;

    if (StringUtils::contains(value, "box"))
      return KdQueryMode::UseBoxQuery;

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

  //idxfile
  IdxFile idxfile;

  //this is needed for midx
  Color color;

  //this is needed for midx
  Matrix logic_to_LOGIC;

  //annotations
  SharedPtr<Annotations> annotations;

  //internal use only
  std::vector<LogicSamples> level_samples;

  //internal use only
  std::vector<LogicSamples> block_samples;

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

  //blocksFullRes()
  bool blocksFullRes() const {
    auto s = bitmask.toString();
    return !s.empty() && s[0] == 'F';
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

  //getDefaultAccuracy
  double getDefaultAccuracy() const {
    return this->default_accuracy;
  }

  //setDefaultAccuracy
  void setDefaultAccuracy(double value) {
    this->default_accuracy = value;
  }

  //getTotalNumberOfBlocks
  BigInt getTotalNumberOfBlocks() const {
    return (((BigInt)1) << getMaxResolution()) / (((Int64)1) << getDefaultBitsPerBlock());
  }

  //setEnableAnnotations
  void setEnableAnnotations(bool value) {
    annotations->enabled = value;
    if (!annotations) return;
  }

  //getEnableAnnotations
  bool getEnableAnnotations() const {
    return annotations ? annotations->enabled : false;
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

  //for python
  int getNumberOfLevelSamples() const {
    return (int)level_samples.size();
  }

  //getLevelSamples
  LogicSamples getLevelSamples(int lvl) const  {
    return level_samples[lvl];
  }


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

  //createAccess (to be used for box query)
  virtual SharedPtr<Access> createAccess(StringTree config = StringTree(), bool for_block_query = false);

  //createAccessForBlockQuery
  SharedPtr<Access> createAccessForBlockQuery(StringTree config = StringTree());


public:

  //________________________________________________
  //block query stuff

  //createBlockQuery
  virtual SharedPtr<BlockQuery> createBlockQuery(BigInt blockid, Field field, double time, int mode = 'r', Aborted aborted = Aborted());

  //createBlockQuery
  SharedPtr<BlockQuery> createBlockQuery(BigInt blockid, int mode = 'r', Aborted aborted = Aborted()) {
    return createBlockQuery(blockid, getField(), getTime(), mode, aborted);
  }

  //getBlockQuerySamples
  LogicSamples getBlockQuerySamples(BigInt blockid, int& H);

  //getBlockQuerySamples (for swig)
  LogicSamples getBlockQuerySamples(BigInt blockid) {
    int H=0; auto samples=getBlockQuerySamples(blockid,H); return samples;
  }

  //getBlockQueryLevel (for swig)
  int getBlockQueryLevel(BigInt blockid) {
    int H=0; auto samples = getBlockQuerySamples(blockid, H); return H;
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
  virtual bool convertBlockQueryToRowMajor(SharedPtr<BlockQuery> block_query);

  //createEquivalentBoxQuery
  virtual SharedPtr<BoxQuery> createEquivalentBoxQuery(int mode, SharedPtr<BlockQuery> block_query)
  {
    auto ret = createBoxQuery(block_query->logic_samples.logic_box, block_query->field, block_query->time, mode, block_query->aborted);
    ret->setResolutionRange(block_query->blockid == 0 ? 0 : block_query->H, block_query->H);
    return ret;
  }

public:

  //________________________________________________
  //box query stuff

  //createBoxQuery
  virtual SharedPtr<BoxQuery> createBoxQuery(BoxNi logic_box, Field field, double time, int mode = 'r', Aborted aborted = Aborted());

  //createBoxQuery
  inline SharedPtr<BoxQuery> createBoxQuery(BoxNi logic_box, int mode = 'r', Aborted aborted = Aborted()) {
    return createBoxQuery(logic_box, getField(), getTime(), mode, aborted);
  }

  //createBlockQueriesForBoxQuery
  virtual std::vector<BigInt> createBlockQueriesForBoxQuery(SharedPtr<BoxQuery> query);

  //guessBoxQueryEndResolution
  virtual int guessBoxQueryEndResolution(Frustum logic_to_screen, Position logic_position);

  //beginBoxQuery
  virtual void beginBoxQuery(SharedPtr<BoxQuery> query);

  //nextBoxQuery
  virtual void nextBoxQuery(SharedPtr<BoxQuery> query);

  //executeBoxQuery
  virtual bool executeBoxQuery(SharedPtr<Access> access, SharedPtr<BoxQuery> query);

  //mergeBoxQueryWithBlockQuery
  virtual bool mergeBoxQueryWithBlockQuery(SharedPtr<BoxQuery> query, SharedPtr<BlockQuery> block_query);

  //setBoxQueryEndResolution
  virtual bool setBoxQueryEndResolution(SharedPtr<BoxQuery> query, int value);

  //createBoxQueryRequest
  virtual NetRequest createBoxQueryRequest(SharedPtr<BoxQuery> query);

  //executeBoxQueryOnServer
  virtual bool executeBoxQueryOnServer(SharedPtr<BoxQuery> query);

public:

  //________________________________________________
  //point query stuff

  //constructor
  virtual SharedPtr<PointQuery> createPointQuery(Position logic_position, Field field, double time, Aborted aborted = Aborted()) ;

  //createPointQuery
  inline SharedPtr<PointQuery> createPointQuery(Position logic_position, Aborted aborted = Aborted()) {
    return createPointQuery(logic_position, getField(), getTime(), aborted);
  }

  //createBlockQueriesForPointQuery
  virtual std::vector<BigInt> createBlockQueriesForPointQuery(SharedPtr<PointQuery> query) {
    return {};
  }

  //guessPointQueryEndResolution
  virtual int guessPointQueryEndResolution(Frustum logic_to_screen, Position logic_position) ;

  //guessPointQueryNumberOfSamples
  virtual PointNi guessPointQueryNumberOfSamples(Frustum logic_to_screen, Position logic_position, int end_resolution) ;

  //beginPointQuery
  virtual void beginPointQuery(SharedPtr<PointQuery> query) ;

  //executeBoxQuery
  virtual bool executePointQuery(SharedPtr<Access> access, SharedPtr<PointQuery> query);

  //mergePointQueryWithBlockQuery
  virtual bool mergePointQueryWithBlockQuery(SharedPtr<PointQuery> query, SharedPtr<BlockQuery> block_query);

  //nextPointQuery
  virtual void nextPointQuery(SharedPtr<PointQuery> query) ;

  //createPointQueryRequest
  virtual NetRequest createPointQueryRequest(SharedPtr<PointQuery> query);

  //executePointQueryOnServer
  virtual bool executePointQueryOnServer(SharedPtr<PointQuery> query);

public:

  //adjustBoxQueryFilterBox
  virtual BoxNi adjustBoxQueryFilterBox(BoxQuery* query, IdxFilter* filter, BoxNi box, int H);

  //createFilter
  virtual SharedPtr<IdxFilter> createFilter(const Field& field);

  //computeFilter
  virtual bool computeFilter(SharedPtr<IdxFilter> filter, double time, Field field, SharedPtr<Access> access, PointNi SlidingWindow, bool bVerbose = false);

  //computeFilter
  virtual void computeFilter(const Field& field, int window_size, bool bVerbose = false);

  //executeBlockQuerWithFilters
  virtual bool executeBlockQuerWithFilters(SharedPtr<Access> access, SharedPtr<BoxQuery> query, SharedPtr<IdxFilter> filter);

public:

  //insertSamples
  static bool insertSamples(
    LogicSamples Wsamples, Array Wbuffer,
    LogicSamples Rsamples, Array Rbuffer, Aborted aborted);

  //getFilenames
  std::vector<String> getFilenames(int timestep = -1,String field="");

  //compressDataset
  virtual void compressDataset(std::vector<String> compression, Array data = Array());;

public:

  //readDatasetFromArchive 
  virtual void readDatasetFromArchive(Archive& ar);

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
  bool                    missing_blocks = false;
  double                  default_accuracy = 0.0; //for idx2

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

VISUS_DB_API SharedPtr<Dataset> LoadDataset(String url, String cache_dir = "");

} //namespace Visus


#endif //__VISUS_DB_DATASET_H

