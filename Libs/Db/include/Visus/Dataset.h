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
#include <Visus/Query.h>
#include <Visus/DatasetBitmask.h>
#include <Visus/DatasetTimesteps.h>
#include <Visus/Path.h>
#include <Visus/NetMessage.h>

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
    UseQuery = 0x02
  };

  //fromString
  static int fromString(String value)
  {
    value = StringUtils::trim(StringUtils::toLower(value));

    if (value == "block")
      return KdQueryMode::UseBlockQuery;

    if (value == "box")
      return KdQueryMode::UseQuery;

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
class VISUS_DB_API BaseDataset
{
public:

  //NOTE: I can copy the base inforation

  Url                     url;
  String                  dataset_body;
  DatasetBitmask          bitmask;
  int                     default_bitsperblock = 0;
  NdBox                   box;
  DatasetTimesteps        timesteps;
  String                  default_scene;

  StringTree              config;
  SharedPtr<RamAccess>    ram_access;
  int                     kdquery_mode = KdQueryMode::NotSpecified;

  std::vector<Field>      fields;
  std::map<String, Field> find_field;

  bool                    bServerMode = false;

};

////////////////////////////////////////////////////////
class VISUS_DB_API Dataset : public BaseDataset
{
public:

  VISUS_NON_COPYABLE_CLASS(Dataset)

  //______________________________________________
  class VISUS_DB_API Info
  {
  public:
    String     name;
    Url        url;
    String     TypeName;
    StringTree config;
    bool valid() const { return !name.empty(); }
  };

  //constructor
  Dataset() {
  }

  //destructor
  virtual ~Dataset() {
  }

  //getTypeName
  virtual String getTypeName() const = 0;

  //copyDataset
  static void copyDataset(Dataset* Dvf, SharedPtr<Access> Daccess, Field Dfield, double Dtime,
    Dataset* Svf, SharedPtr<Access> Saccess, Field Sfield, double Stime);


  //findDatasetInVisusConfig
  static Info findDatasetInVisusConfig(String name);

  //getDefaultDatasetInVisusConfig
  static String getDefaultDatasetInVisusConfig();

  //loadDataset
  static SharedPtr<Dataset> loadDataset(String name);

  //valid
  bool valid() const {
    return getUrl().valid();
  }

  //invalidate
  void invalidate() {
    this->url = Url();
  }

  //getUrl
  Url getUrl() const {
    return url;
  }

  //setUrl (internal use only)
  void setUrl(Url value) {
    this->url = value;
  }

  //setDatasetBody (internal use only)
  void setDatasetBody(String value) {
    this->dataset_body = value;
  }

  //getBitmask
  const DatasetBitmask& getBitmask() const {
    return bitmask;
  }

  //getBox
  NdBox getBox() const {
    return box;
  }

  //getTimesteps
  const DatasetTimesteps& getTimesteps() const {
    return timesteps;
  }

  //getDefaultBitsPerBlock
  int getDefaultBitsPerBlock() const {
    return default_bitsperblock;
  }

  //getTotalnumberOfBlocks
  BigInt getTotalnumberOfBlocks() const {
    return (((BigInt)1) << getMaxResolution()) / (((Int64)1) << getDefaultBitsPerBlock());
  }

  //getConfig
  const StringTree& getConfig() const {
    return config;
  }

  //getDefaultTime
  double getDefaultTime() const
  {
    Url url = getUrl();
    if (url.hasParam("time")) return cdouble(url.getParam("time"));
    return getTimesteps().getDefault();
  }

  //getAccessConfigs
  std::vector<StringTree*> getAccessConfigs() const {
    return config.findAllChildsWithName("access", false);
  }

  //getDefaultAccessConfig
  StringTree getDefaultAccessConfig() const  {
    auto v = getAccessConfigs();
    return v.empty() ? StringTree() : *v[0];
  }

  //getMaxResolution
  int getMaxResolution() const {
    return getBitmask().getMaxResolution();
  }

  //getPointDim
  int getPointDim() const {
    return getBitmask().getPointDim();
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
  
  String getDefaultScene() const {
    return default_scene.empty()? "" : default_scene;
  }

  //getFields
  std::vector<Field> getFields() const {
    return fields;
  }

  //getDatasetBody
  String getDatasetBody() const {
    return dataset_body;
  } 

  // getDatasetInfos
  String getDatasetInfos() const;

  //extractLevelImage
  Array extractLevelImage(SharedPtr<Access> access, Field field, double time, int H);

public:

  //openFromUrl 
  virtual bool openFromUrl(Url url) = 0;

  //compress
  virtual bool compress(String compression) {
    VisusAssert(false);
    return false;
  }

  //getInnerDatasets
  virtual std::map<String,SharedPtr<Dataset> > getInnerDatasets() const  {
    return std::map<String,SharedPtr<Dataset> >();
  }

  //guessEndResolutions
  virtual std::vector<int> guessEndResolutions(const Frustum& viewdep, Position position, Query::Quality quality=Query::DefaultQuality, Query::Progression progression=Query::GuessProgression);

  // getFieldByNameThrowEx
  virtual Field getFieldByNameThrowEx(String name) const;

  // getFieldByName
  Field getFieldByName(String name) const {
    try {
      return getFieldByNameThrowEx(name);
    }
    catch (std::exception ex) {
      return Field();
    }
  }

  //createAccess
  virtual SharedPtr<Access> createAccess(StringTree config=StringTree(), bool bForBlockQuery = false);

  //createAccessForBlockQuery
  SharedPtr<Access> createAccessForBlockQuery(StringTree config= StringTree()) {
    return createAccess(config,true);
  }

  //getLevelBox
  virtual LogicBox getLevelBox(int H)=0;

  //getAddressRangeBox
  virtual LogicBox getAddressRangeBox(BigInt start_address,BigInt end_address)=0;

  //readBlock  
  virtual Future<Void> readBlock(SharedPtr<Access> access,SharedPtr<BlockQuery> query);

  //writeBlock  
  virtual Future<Void> writeBlock(SharedPtr<Access> access, SharedPtr<BlockQuery> query);

  //readBlockAndWait
  bool readBlockAndWait(SharedPtr<Access> access, SharedPtr<BlockQuery> query) {
    readBlock(access, query).get(); return query->ok();
  }

  //writeBlockAndWait
  bool writeBlockAndWait(SharedPtr<Access> access, SharedPtr<BlockQuery> query) {
    writeBlock(access, query).get(); return query->ok();
  }

  //convertBlockQueryToRowMajor
  virtual bool convertBlockQueryToRowMajor(SharedPtr<BlockQuery> block_query) {
    return false;
  }

  //createQueryFilter (default: not supported)
  virtual SharedPtr<DatasetFilter> createQueryFilter(const Field& field) {
    return SharedPtr<DatasetFilter>();
  }

  //beginQuery
  virtual bool beginQuery(SharedPtr<Query> query);

  //executeQuery
  virtual bool executeQuery(SharedPtr<Access> access,SharedPtr<Query> query);

  //nextQuery
  virtual bool nextQuery(SharedPtr<Query> query);

  //mergeQueryWithBlock
  virtual bool mergeQueryWithBlock(SharedPtr<Query> query, SharedPtr<BlockQuery> block_query){
    return false;
  }

  //createPureRemoteQueryNetRequest
  virtual NetRequest createPureRemoteQueryNetRequest(SharedPtr<Query> query) {
    return NetRequest();
  }

public:

  //addField
  void addField(String name, Field field) {
    fields.push_back(field);
    find_field[name] = field;
  }

  //addField
  void addField(Field field) {
    addField(field.name, field);
  }

  //executePureRemoteQuery
  bool executePureRemoteQuery(SharedPtr<Query> query);

  //generateTiles (useful for conversion)
  std::vector<NdBox> generateTiles(int TileSize) const;

  //readMaxResolutionData
  Array readMaxResolutionData(SharedPtr<Access> access, Field field, double time, NdBox box);

  //readMaxResolutionData
  Array readMaxResolutionData(SharedPtr<Access> access, NdBox box) {
    return readMaxResolutionData(access, getDefaultField(), getDefaultTime(),box);
  }

  //writeMaxResolutionData
  bool writeMaxResolutionData(SharedPtr<Access> access, Field field, double time, NdBox box, Array buffer);

  //writeMaxResolutionData
  bool writeMaxResolutionData(SharedPtr<Access> access, NdBox box, Array buffer) {
    return writeMaxResolutionData(access,getDefaultField(),getDefaultTime(),box,buffer);
  }

public:
  
  //toString
  String toString() const {
    return dataset_body.empty() ? getUrl().toString() : dataset_body;
  }

  //writeToObjectStream
  void writeToObjectStream(ObjectStream& ostream);

  //readFromObjectStream
  void readFromObjectStream(ObjectStream& istream);

private:

  //DO NOT ADD VARIABLE here... add to BaseDataset which is copy-able

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
    String extension; 
    String TypeName;
    CreateInstance createInstance;

  };

  //registerDatasetType
  void registerDatasetType(String extension,String TypeName, CreateInstance createInstance)
  {
    RegisteredDataset item;
    item.extension = extension;
    item.TypeName = TypeName;
    item.createInstance = createInstance;
    v.push_back(item);
  }

  //getDatasetTypeNameFromExtension
  String getDatasetTypeNameFromExtension(String extension) {
    for (const auto& it : v) {
      if (it.extension == extension)
        return it.TypeName;
    }
    return "";
  }

  //createInstance
  SharedPtr<Dataset> createInstance(String TypeName) {
    for (const auto& it : v) {
      if (it.TypeName == TypeName)
        return it.createInstance();
    }
    return SharedPtr<Dataset>();
  }

private:

  //extension -> TypeName
  std::vector<RegisteredDataset> v;

  DatasetFactory(){}

};

} //namespace Visus


#endif //__VISUS_DB_DATASET_H

