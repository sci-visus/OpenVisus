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

#include <Visus/IdxMultipleDataset.h>
#include <Visus/IdxMultipleAccess.h>
#include <Visus/Path.h>
#include <Visus/Polygon.h>

namespace Visus {

////////////////////////////////////////////////////////////////////////////////////
static bool IsGoodVariableName(String name)
{
  const std::set<String> ReservedWords =
  {
    "and", "del","from","not","while","as","elif","global","or","with","assert", "else","if",
    "pass","yield","break","except","import","print", "class","exec""in","raise","continue",
    "finally","is","return","def","for","lambda","try"
  };

  if (name.empty() || ReservedWords.count(name))
    return false;

  if (!std::isalpha(name[0]))
    return false;

  for (int I = 1; I < (int)name.length(); I++)
  {
    if (!(std::isalnum(name[I]) || name[I] == '_'))
      return false;
  }

  return true;
};


///////////////////////////////////////////////////////////////////////////////////
IdxMultipleDataset::IdxMultipleDataset() {

  this->debug_mode = 0;// DebugSkipReading;
}


///////////////////////////////////////////////////////////////////////////////////
IdxMultipleDataset::~IdxMultipleDataset() {
}


////////////////////////////////////////////////////////////////////////////////////
Field IdxMultipleDataset::getFieldEx(String FIELDNAME) const
{
  String CODE;
  if (find_field.count(FIELDNAME))
    CODE = find_field.find(FIELDNAME)->second.name;  //existing field (it's a symbolic name)
  else
    CODE = FIELDNAME; //the fieldname itself is the expression

  auto OUTPUT = computeOuput(/*QUERY*/nullptr, /*ACCESS*/SharedPtr<Access>(), Aborted(), CODE);
  return Field(CODE, OUTPUT.dtype);
}

////////////////////////////////////////////////////////////////////////////////////
String IdxMultipleDataset::getInputName(String dataset_name, String fieldname)
{
  std::ostringstream out;
  out << "input";

  if (IsGoodVariableName(dataset_name))
    out << "." << dataset_name;
  else
    out << "['" << dataset_name << "']";

  if (IsGoodVariableName(fieldname))
  {
    out << "." << fieldname;
  }
  else
  {
    if (StringUtils::contains(fieldname, "\n"))
    {
      const String triple = "\"\"\"";
      out << "[" + triple + "\n" + fieldname + triple + "]";
    }
    else
    {
      fieldname = StringUtils::replaceAll(fieldname, "'", "\\'");
      out << "['" << fieldname << "']";
    }
  }

  return out.str();
};




/////////////////////////////////////////////////////////////////////////////////////
Array IdxMultipleDataset::executeDownQuery(BoxQuery* QUERY, SharedPtr<Access> ACCESS, String dataset_name, String fieldname)
{
  IdxMultipleDataset* DATASET = this;

  auto dataset = DATASET->down_datasets[dataset_name]; 
  VisusReleaseAssert(dataset);

  auto field = dataset->getField(fieldname); 
  VisusReleaseAssert(field.valid());

  auto QUERY_LOGIC_BOX = QUERY->logic_box.castTo<BoxNd>();

  //no intersection? just skip this down query
  auto VALID_LOGIC_REGION = Position(dataset->logic_to_LOGIC, dataset->getLogicBox()).toAxisAlignedBox();
  if (!QUERY_LOGIC_BOX.intersect(VALID_LOGIC_REGION))
    return Array();

  // consider that logic_to_logic could have mat(3,0) | mat(3,1) | mat(3,2) !=0 and so I can have non-parallel axis
  // using directly the QUERY_LOGIC_BOX could result in missing pieces for example in voronoi
  // solution is to limit the QUERY_LOGIC_BOX to the valid mapped region
  QUERY_LOGIC_BOX = QUERY_LOGIC_BOX.getIntersection(VALID_LOGIC_REGION);

  auto LOGIC_to_logic = dataset->logic_to_LOGIC.invert();
  auto query_logic_box = Position(LOGIC_to_logic, QUERY_LOGIC_BOX).toDiscreteAxisAlignedBox();
  query_logic_box = query_logic_box.getIntersection(dataset->getLogicBox());

  //euristic to find delta in the hzcurve (sometimes it produces too many samples)
  auto VOLUME = Position(DATASET->getLogicBox()).computeVolume();
  auto volume = Position(dataset->logic_to_LOGIC, dataset->getLogicBox()).computeVolume();
  int delta_h = -(int)log2(VOLUME / volume);

  //query already created?
  auto key = dataset_name + "/" + fieldname;
  SharedPtr<BoxQuery> query;
  if (QUERY->down_queries.count(key))
  {
    query  = QUERY->down_queries[key];
  }
  else
  {
    query = dataset->createBoxQuery(query_logic_box, field, QUERY->time, 'r', QUERY->aborted);
    QUERY->down_queries[key] = query;

    //resolutions
    if (!QUERY->start_resolution)
      query->start_resolution = 0;
    else
      query->start_resolution = Utils::clamp(QUERY->start_resolution + delta_h, 0, dataset->getMaxResolution()); //probably a block query

    std::set<int> resolutions;
    for (auto END_RESOLUTION : QUERY->end_resolutions)
    {
      auto end_resolution = Utils::clamp(END_RESOLUTION + delta_h, 0, dataset->getMaxResolution());
      resolutions.insert(end_resolution);
    }
    query->end_resolutions = std::vector<int>(resolutions.begin(), resolutions.end());

    //skip this argument since returns empty array
    dataset->beginBoxQuery(query);
    if (!query->isRunning())
    {
      query->setFailed("cannot begin the query");
      return Array();
    }

    //ignore missing timesteps
    if (!dataset->getTimesteps().containsTimestep(query->time))
    {
      PrintInfo("Missing timestep", query->time, "for input['", concatenate(dataset_name, ".", field.name), "']...ignoring it");
      query->setFailed("wrong time");
      return Array();
    }
  }

  //if not multiple access i think it will be a pure remote query
  //NOTE I'm creating a donw-access for each key (i.e. dataset_name.field_name)
  //     this could be just dataset_name but what happens if I executeDownQuery in parallel on 2 fields of the same datasets?
  //     at least this way I could run in parallel... even if I'm not doing it
  SharedPtr<Access> access;
  if (auto multiple_access = std::dynamic_pointer_cast<IdxMultipleAccess>(ACCESS))
  {
    if (multiple_access->down_access.count(key))
    {
      access = multiple_access->down_access[key];
    }
    else
    {
      access = multiple_access->createDownAccess(dataset_name, field.name);
      multiple_access->down_access[key] = access;
    }
  }

  //already failed
  if (!query || query->failed())
    return Array();

  //NOTE if I cannot execute it probably reached max resolution for query, in that case I recycle old 'BUFFER'
  if (query->canExecute())
  {
    if (DATASET->debug_mode & IdxMultipleDataset::DebugSkipReading)
    {
      query->allocateBufferIfNeeded();
      ArrayUtils::setBufferColor(query->buffer, DATASET->down_datasets[dataset_name]->color);
      query->buffer.layout = ""; //row major
      VisusAssert(query->buffer.dims == query->getNumberOfSamples());
      query->setCurrentResolution(query->end_resolution);

    }
    else
    {
      if (!dataset->executeBoxQuery(access, query))
      {
        query->setFailed("cannot execute the query");
        return Array();
      }
    }

    //PrintInfo("MIDX up nsamples",QUERY->nsamples,"dw",dataset_name,".",field.name,"nsamples",query->buffer.dims.toString());
  }

  //create a brand new BUFFER for doing the warpPerspective
  auto ret = Array(QUERY->getNumberOfSamples(), query->buffer.dtype);
  ret.fillWithValue(query->field.default_value);

  ret.alpha = std::make_shared<Array>(ret.dims, DTypes::UINT8);
  ret.alpha->fillWithValue(0);

  auto PIXEL_TO_LOGIC = Position::computeTransformation(Position(QUERY->logic_box), ret.dims);
  auto pixel_to_logic = Position::computeTransformation(Position(query->logic_box), query->buffer.dims);

  auto LOGIC_TO_PIXEL = PIXEL_TO_LOGIC.invert();
  auto pixel_to_PIXEL = LOGIC_TO_PIXEL * dataset->logic_to_LOGIC * pixel_to_logic;
  auto LOGIC_CENTROID = dataset->logic_to_LOGIC * dataset->getLogicBox().center();

  //limit the samples to good logic domain
  //explanation: for each pixel in dims, tranform it to the logic dataset box, if inside set the pixel to 1 otherwise set the pixel to 0
  if (!query->buffer.alpha)
  {
    query->buffer.alpha = std::make_shared<Array>(query->buffer.dims, DTypes::UINT8);
    query->buffer.alpha->fillWithValue(255);
  }
  VisusReleaseAssert(query->buffer.alpha->dims == query->buffer.dims);

  ArrayUtils::warpPerspective(ret, pixel_to_PIXEL, query->buffer, QUERY->aborted);

  if (DATASET->debug_mode & IdxMultipleDataset::DebugSaveImages)
  {
    static int cont = 0;
    String tmp_dir = "tmp/debug_midx";
    ArrayUtils::saveImage(concatenate(tmp_dir, "/", cont, ".dw." + dataset_name, ".", query->field.name, ".buffer.png"), query->buffer);
    ArrayUtils::saveImage(concatenate(tmp_dir, "/", cont, ".dw." + dataset_name, ".", query->field.name, ".alpha_.png"), *query->buffer.alpha);
    ArrayUtils::saveImage(concatenate(tmp_dir, "/", cont, ".up." + dataset_name, ".", query->field.name, ".buffer.png"), ret);
    ArrayUtils::saveImage(concatenate(tmp_dir, "/", cont, ".up." + dataset_name, ".", query->field.name, ".alpha_.png"), *ret.alpha);
    //ArrayUtils::setBufferColor(query->BUFFER, DATASET->childs[dataset_name].color);
    cont++;
  }

  //this will help to find voronoi seams betweeen images
  ret.run_time_attributes.setValue("LOGIC_TO_PIXEL", LOGIC_TO_PIXEL.toString());
  ret.run_time_attributes.setValue("PIXEL_TO_LOGIC", PIXEL_TO_LOGIC.toString());
  ret.run_time_attributes.setValue("LOGIC_CENTROID", LOGIC_CENTROID.toString());

  return ret;
}

////////////////////////////////////////////////////////////////////////
bool IdxMultipleDataset::executeBoxQuery(SharedPtr<Access> ACCESS,SharedPtr<BoxQuery> QUERY)
{
  auto MULTIPLE_ACCESS = std::dynamic_pointer_cast<IdxMultipleAccess>(ACCESS);
  if (!MULTIPLE_ACCESS)
    return IdxDataset::executeBoxQuery(ACCESS, QUERY);

  if (!QUERY)
    return false;

  if (!(QUERY->isRunning() && QUERY->getCurrentResolution() < QUERY->getEndResolution()))
    return false;

  if (QUERY->aborted())
  {
    QUERY->setFailed("QUERY aboted");
    return false;
  }

  if (QUERY->mode == 'w')
  {
    QUERY->setFailed("not supported");
    return false;
  }

  //execute N-Query (independentely) and blend them
  Array  OUTPUT;
  try
  {
    OUTPUT = computeOuput(QUERY.get(), ACCESS, QUERY->aborted, QUERY->field.name);
  }
  catch (std::exception ex)
  {
    QUERY->setFailed(QUERY->aborted() ? "query aborted" : ex.what());
    return false;
  }

  QUERY->buffer = OUTPUT;
  QUERY->setCurrentResolution(QUERY->end_resolution);
  return true;
}

////////////////////////////////////////////////////////////////////////
void IdxMultipleDataset::beginBoxQuery(SharedPtr<BoxQuery> query) {
  return IdxDataset::beginBoxQuery(query);
}

////////////////////////////////////////////////////////////////////////
void IdxMultipleDataset::nextBoxQuery(SharedPtr<BoxQuery> QUERY)
{
  if (!QUERY)
    return;

  if (!(QUERY->isRunning() && QUERY->getCurrentResolution() == QUERY->getEndResolution()))
    return;

  //reached the end? 
  if (QUERY->end_resolution == QUERY->end_resolutions.back())
    return QUERY->setOk();

  IdxDataset::nextBoxQuery(QUERY);

  //finished?
  if (!QUERY->isRunning())
    return;

  for (auto it : QUERY->down_queries)
  {
    auto  query   = it.second;
    auto  dataset = query->dataset; VisusAssert(dataset);

    //can advance to the next level?
    if (query && query->isRunning() && query->getCurrentResolution() == query->getEndResolution())
      dataset->nextBoxQuery(query);
  }
}



////////////////////////////////////////////////////////////////////////////////////
String IdxMultipleDataset::removeAliases(String url)
{
  Url URL = this->getUrl();

  if (URL.isFile())
  {
    String dir = Path(URL.getPath()).getParent().toString();
    if (dir.empty())
      return url;

    if (Url(url).isFile() && StringUtils::startsWith(Url(url).getPath(), "./"))
      url = dir + Url(url).getPath().substr(1);

    if (StringUtils::contains(url, "$(CurrentFileDirectory)"))
      url = StringUtils::replaceAll(url, "$(CurrentFileDirectory)", dir);

  }
  else if (URL.isRemote())
  {
    if (StringUtils::contains(url, "$(protocol)"))
      url = StringUtils::replaceAll(url, "$(protocol)", URL.getProtocol());

    if (StringUtils::contains(url, "$(hostname)"))
      url = StringUtils::replaceAll(url, "$(hostname)", URL.getHostname());

    if (StringUtils::contains(url, "$(port)"))
      url = StringUtils::replaceAll(url, "$(port)", cstring(URL.getPort()));
  }

  return url;
};

///////////////////////////////////////////////////////////
void IdxMultipleDataset::parseDataset(StringTree& cur, Matrix modelview)
{
  String url = cur.getAttribute("url");
  VisusAssert(!url.empty());

  String name = StringUtils::trim(cur.getAttribute("name", cur.getAttribute("id")));

  //override name if exist
  if (name.empty() || this->down_datasets.count(name))
    name = concatenate("child_", StringUtils::formatNumber("%04d", (int)this->down_datasets.size()));

  url = removeAliases(url);

  cur.write("name", name); //in case I changed it
  cur.write("url", url);
  auto child = LoadDatasetEx(cur);

  child->color = Color::fromString(cur.getAttribute("color", Color::random().toString()));;
  auto sdim = child->getPointDim() + 1;

  //override physic_box 
  if (cur.hasAttribute("physic_box"))
  {
    auto physic_box = BoxNd::fromString(cur.getAttribute("physic_box"));
    child->setDatasetBounds(physic_box);
  }
  else if (cur.hasAttribute("quad"))
  {
    //in midx physic coordinates
    VisusReleaseAssert(child->getPointDim() == 2);
    auto W = (int)child->getLogicBox().size()[0];
    auto H = (int)child->getLogicBox().size()[1];
    auto dst = Quad::fromString(cur.getAttribute("quad"));
    auto src = Quad(W, H);
    auto T = Quad::findQuadHomography(dst, src);
    child->setDatasetBounds(Position(T, BoxNd(PointNd(0, 0), PointNd(W, H))));
  }

  // transform physic box
  modelview.setSpaceDim(sdim);

  //refresh dataset bounds
  auto bounds = child->getDatasetBounds();
  child->setDatasetBounds(Position(modelview, bounds));

  //update annotation positions by modelview
  if (child->annotations && child->annotations->enabled)
  {
    for (auto annotation : *child->annotations)
    {
      auto ANNOTATION = annotation->cloneAnnotation();
      ANNOTATION->prependModelview(modelview);

      if (!this->annotations)
        this->annotations = std::make_shared<Annotations>();

      this->annotations->push_back(ANNOTATION);
    }
  }

  addChild(name, child);
}

///////////////////////////////////////////////////////////
void IdxMultipleDataset::parseDatasets(StringTree& ar, Matrix modelview)
{
  if (!cbool(ar.getAttribute("enabled", "1")))
    return;

  //final
  if (ar.name == "svg")
  {
    this->annotations = std::make_shared<Annotations>();
    this->annotations->read(ar);

    for (auto& annotation : *this->annotations)
      annotation->prependModelview(modelview);

    return;
  }

  //final
  if (ar.name == "dataset")
  {
    //this is for mosaic
    if (ar.hasAttribute("offset"))
    {
      auto vt = PointNd::fromString(ar.getAttribute("offset"));
      modelview *= Matrix::translate(vt);
    }

    parseDataset(ar, modelview);
    return;
  }

  if (ar.name == "translate")
  {
    double tx = cdouble(ar.getAttribute("x"));
    double ty = cdouble(ar.getAttribute("y"));
    double tz = cdouble(ar.getAttribute("z"));
    modelview *= Matrix::translate(PointNd(tx, ty, tz));
  }
  else if (ar.name == "scale")
  {
    double sx = cdouble(ar.getAttribute("x"));
    double sy = cdouble(ar.getAttribute("y"));
    double sz = cdouble(ar.getAttribute("z"));
    modelview *= Matrix::nonZeroScale(PointNd(sx, sy, sz));
  }

  else if (ar.name == "rotate")
  {
    double rx = Utils::degreeToRadiant(cdouble(ar.getAttribute("x")));
    double ry = Utils::degreeToRadiant(cdouble(ar.getAttribute("y")));
    double rz = Utils::degreeToRadiant(cdouble(ar.getAttribute("z")));
    modelview *= Matrix::rotate(Quaternion::fromEulerAngles(rx, ry, rz));
  }
  else if (ar.name == "transform" || ar.name == "M")
  {
    modelview *= Matrix::fromString(ar.getAttribute("value"));
  }

  //recursive
  for (auto child : ar.getChilds())
    parseDatasets(*child, modelview);
}

///////////////////////////////////////////////////////////
IdxFile IdxMultipleDataset::generateIdxFile(Archive& ar)
{
  IdxFile ret;

  auto first = getFirstChild();
  int pdim = first->getPointDim();
  int sdim = pdim + 1;

  IdxFile& IDXFILE = this->idxfile;

  //set PHYSIC_BOX (union of physic boxes)
  auto PHYSIC_BOX = BoxNd::invalid();
  if (ar.hasAttribute("physic_box"))
  {
    ar.read("physic_box", PHYSIC_BOX);
  }
  else
  {
    for (auto it : down_datasets)
      PHYSIC_BOX = PHYSIC_BOX.getUnion(it.second->getDatasetBounds().toAxisAlignedBox());
  }
  ret.bounds = Position(PHYSIC_BOX);

  if (ar.hasAttribute("logic_box"))
  {
    ar.read("logic_box", ret.logic_box);
  }
  else if (down_datasets.size() == 1)
  {
    ret.logic_box = down_datasets.begin()->second->getLogicBox();
  }
  else
  {
    // logic_npixels' / logic_npixels = physic_module' / physic_module
    // physic_module' * vs = logic_npixels'
    // vs = logic_npixels / physic_module
    auto pdim = getFirstChild()->getPointDim();
    auto VS = PointNd::zero(pdim);
    for (auto it : down_datasets)
    {
      auto dataset = it.second;
      for (auto edge : BoxNd::getEdges(pdim))
      {
        auto logic_num_pixels = dataset->getLogicBox().size()[edge.axis];
        auto physic_points = dataset->getDatasetBounds().getPoints();
        auto physic_edge = physic_points[edge.index1] - physic_points[edge.index0];
        auto physic_axis = physic_edge.abs().max_element_index();
        auto physic_module = physic_edge.module();
        auto density = logic_num_pixels / physic_module;
        auto vs = density;
        VS[physic_axis] = std::max(VS[physic_axis], vs);
      }
    }
    ret.logic_box = Position(Matrix::scale(VS), Matrix::translate(-PHYSIC_BOX.p1), PHYSIC_BOX).toAxisAlignedBox().castTo<BoxNi>();
  }

  //time_template
  if (down_datasets.size() == 1)
    ret.time_template = getFirstChild()->idxfile.time_template;

  //timesteps
  for (auto it : down_datasets)
    ret.timesteps.addTimesteps(it.second->getTimesteps());

  ret.missing_blocks = false;
  return ret;
}

///////////////////////////////////////////////////////////
void IdxMultipleDataset::readDatasetFromArchive(Archive& ar)
{
  //i need this because parseDatasets will call getUrl to remove aliases
  this->dataset_body = ar;
  this->kdquery_mode = KdQueryMode::fromString(ar.readString("kdquery"));
  
  for (auto& it : ar.getChilds())
    parseDatasets(*it, Matrix());
    
  VisusReleaseAssert(!down_datasets.empty());

  //automatically generate idxfile
  ar.removeChild("idxfile");
  auto idxfile = generateIdxFile(ar);

  //used specified fields (happends with midx)
  if (ar.getChild("field"))
  {
    int generate_name = 0;
    for (auto child : ar.getChilds("field"))
    {
      String name = child->readString("name");

      if (name.empty())
        name = concatenate("field_", generate_name++);

      //I expect to find here CData node or Text node...
      String code;
      child->readText("code", code);
      VisusAssert(!code.empty());

      Field FIELD = getField(code);
      VisusReleaseAssert(FIELD.valid());
      addField(name, FIELD); //FIELD.name now contain the real code
      idxfile.fields.push_back(Field(name, FIELD.dtype, "rowmajor"));
    }
  }
  else
  {
    auto createField = [this](String operation_name)
    {
      std::ostringstream out;

      std::vector<String> args;
      for (auto it : down_datasets)
      {
        String arg = "f" + cstring((int)args.size());
        args.push_back(arg);
        out << arg << "=" << getInputName(it.first, it.second->getField().name) << std::endl;
      }
      out << "output=" << operation_name << "([" << StringUtils::join(args, ",") << "])" << std::endl;

      String fieldname = out.str();
      Field ret = getField(fieldname);
      ret.setDescription(operation_name);
      VisusAssert(ret.valid());
      return ret;
    };

    //this will appear in the combo box
    addField(createField("ArrayUtils.average"));
    addField(createField("ArrayUtils.add"));
    addField(createField("ArrayUtils.sub"));
    addField(createField("ArrayUtils.mul"));
    addField(createField("ArrayUtils.div"));
    addField(createField("ArrayUtils.min"));
    addField(createField("ArrayUtils.max"));
    addField(createField("ArrayUtils.standardDeviation"));
    addField(createField("ArrayUtils.median"));

    //note: this wont' work on old servers
    addField(Field("output=voronoi()"));
    addField(Field("output=noBlend()"));
    addField(Field("output=averageBlend()"));

    for (auto it : down_datasets)
    {
      for (auto field : it.second->getFields())
      {
        auto arg = getInputName(it.first, field.name);
        Field FIELD = getField("output=" + arg + ";");
        VisusAssert(FIELD.valid());
        FIELD.setDescription(it.first + "/" + field.getDescription());
        addField(FIELD);
      }
    }

    //this is to pass the validation, an midx has infinite run-time fields 
    idxfile.fields.push_back(Field("__fake__", DTypes::UINT8));
  }

  //I need to validate before writing (otherwise for example I will be missing bitmask V0101...)
  idxfile.validate(this->getUrl());

  ar.writeObject("idxfile", idxfile);
  IdxDataset::readDatasetFromArchive(ar);

  //set logic_to_LOGIC for child datasets
  for (auto it : down_datasets)
  {
    auto dataset = it.second;
    auto PHYSIC_BOX = getDatasetBounds().toAxisAlignedBox(); //TODO: what if getDatasetBounds has a transformation T?
    auto physic_to_PHYSIC = Matrix::identity(getPointDim() + 1);
    auto PHYSIC_to_LOGIC = Position::computeTransformation(getLogicBox(), PHYSIC_BOX);
    dataset->logic_to_LOGIC = PHYSIC_to_LOGIC * physic_to_PHYSIC * dataset->logicToPhysic();

    //here you should see more or less the same number of pixels
    auto logic_box = dataset->getLogicBox();
    auto LOGIC_PIXELS = Position(dataset->logic_to_LOGIC, logic_box).computeVolume();
    auto logic_pixels = Position(logic_box).computeVolume();
    auto ratio = logic_pixels / LOGIC_PIXELS; //ratio>1 means you are loosing pixels, ratio=1 is perfect, ratio<1 that you have more pixels than needed and you will interpolate
    //PrintInfo("  ", it.first, "volume(logic_pixels)", logic_pixels, "volume(LOGIC_PIXELS)", LOGIC_PIXELS, "ratio==logic_pixels/LOGIC_PIXELS", ratio);
  }


  //PrintInfo(this->dataset_body);
}


} //namespace Visus
