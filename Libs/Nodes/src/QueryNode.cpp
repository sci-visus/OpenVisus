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

#include <Visus/QueryNode.h>
#include <Visus/Dataflow.h>
#include <Visus/StringTree.h>
#include <Visus/GoogleMapsDataset.h>
#include <Visus/IdxFilter.h>

namespace Visus {


///////////////////////////////////////////////////////////////////////////
class QueryNode::MyJob : public NodeJob
{
public:

  QueryNode*               node;
  SharedPtr<Dataset>       dataset;
  SharedPtr<Access>        access;
  int                      pdim;

  Field                    field;
  double                   time;
  Position                 logic_position;
  Frustum                  logic_to_screen;
  int                      quality;
  double                   accuracy;
  int                      progression;

  bool                     verbose;

  SharedPtr<PointQuery>    point_query;
  SharedPtr<BoxQuery>      box_query;

  //constructor
  MyJob(QueryNode* node_,SharedPtr<Dataset> dataset_,SharedPtr<Access> access_)
    : node(node_),dataset(dataset_),access(access_)
  {
    this->verbose = true;

    this->field = node->getField();
    this->time  = node->getTime();
    this->logic_position = node->getQueryLogicPosition();
    this->logic_to_screen = node->logicToScreen();
    this->quality = node->getQuality();
    this->accuracy = node->getAccuracy();
    this->progression = node->getProgression();
    this->verbose = node->isVerbose();
    this->pdim = dataset->getPointDim();

    if (this->progression == QueryGuessProgression)
      this->progression = (pdim == 2) ? (pdim * 3) : (pdim * 4);

    auto minh = dataset->getDefaultBitsPerBlock();
    auto maxh = dataset->getMaxResolution();

    if (bool bPointQuery = (pdim == 3) && (logic_position.getBoxNd().toBox3().minsize() == 0))
    {
      auto endh = dataset->guessPointQueryEndResolution(logic_to_screen, logic_position);
      endh = Utils::clamp(endh + quality, minh, maxh);

      auto query = dataset->createPointQuery(logic_position, field, time, this->aborted);
      query->end_resolutions.push_back(Utils::clamp(endh - progression, minh, maxh));
      while (query->end_resolutions.back() < endh)
      {
        auto H = Utils::clamp(query->end_resolutions.back() + pdim, minh, endh);
        query->end_resolutions.push_back(H);
      }
      query->accuracy = this->accuracy;
      this->point_query = query;
    }
    else
    {
      auto endh = dataset->guessBoxQueryEndResolution(logic_to_screen, logic_position);
      endh = Utils::clamp(endh + quality, minh, maxh);

      //remove transformation! (in doPublish I will add the physic clipping)
      BoxNi logic_box = this->logic_position.toDiscreteAxisAlignedBox();

      //don't want to produce something with too muchResolutionx
      if (QueryNode::willFitOnGpu)
      {
        while (endh > minh)
        {
          auto query = dataset->createBoxQuery(logic_box, field, time, 'r', this->aborted);
          query->end_resolutions = { endh };
          query->accuracy = accuracy;

          dataset->beginBoxQuery(query);
          auto size = query->field.dtype.getByteSize(query->getNumberOfSamples());
          if (QueryNode::willFitOnGpu(size)) break;
          PrintInfo("Lowering resolution since data wont' fit on GPU", "requested_size", StringUtils::getStringFromByteSize(size), "endh", endh);
          --endh;
        }
      }

      auto query = dataset->createBoxQuery(logic_box, field, time, 'r', this->aborted);
      query->accuracy = this->accuracy;
      query->enableFilters();
      query->incrementalPublish = [&](Array output) {
        doPublish(output, query);
      };

      query->end_resolutions.push_back(Utils::clamp(endh - progression, minh, maxh));
      while (query->end_resolutions.back() < endh)
      {
        auto H = Utils::clamp(query->end_resolutions.back() + pdim, minh, endh);
        query->end_resolutions.push_back(H);
      }

      this->box_query = query;
    }
  }

  //destructor
  virtual ~MyJob()
  {
  }

  //runJob
  virtual void runJob() override
  {
    auto T1 = Time::now();
    if (auto query = point_query)
    {
      dataset->beginPointQuery(query);

      int I = 0; 
      while (query->isRunning())
      {
        Time t1 = Time::now();
        query->setPoints(dataset->guessPointQueryNumberOfSamples(logic_to_screen, logic_position, query->end_resolution));

        PrintInfo("PointQuery msec", t1.elapsedMsec(), "level", I, "/", query->end_resolutions.size(), "/", query->end_resolution, "/", dataset->getMaxResolution(), "npoints", query->getNumberOfPoints(), "...");

        if (!dataset->executePointQuery(access, query))
          return;

        auto output = query->buffer.clone();
        output.run_time_attributes.setValue("origin", node->getName());
        PrintInfo("PointQuery finished msec", node->getName(), t1.elapsedMsec(),
          "level", I, "/", query->end_resolutions.size(), "/", query->end_resolution, "/", dataset->getMaxResolution(),
          "dims", output.dims, 
          "dtype", output.dtype, 
          "access", access ? "yes" : "nullptr", 
          "url", dataset->getUrl());

        DataflowMessage msg;
        output.bounds = dataset->logicToPhysic(query->logic_position);
        msg.writeValue("array", output);
        node->publish(msg);
        dataset->nextPointQuery(query);
        I++;
      }
    }
    else if (auto query = box_query)
    {
      dataset->beginBoxQuery(query);

      //could be that end_resolutions gets corrected (see google maps for example)
      int I = 0,N = (int)query->end_resolutions.size();
      for (auto EndH : query->end_resolutions)
      {
        Time t1 = Time::now();

        if (aborted() || !query->isRunning())
          return;

        PrintInfo("BoxQuery executeBoxQuery", I, "/", N, "/", EndH, "/", dataset->getMaxResolution(),"...");

        if (!dataset->executeBoxQuery(access, query))
          return;

        if (aborted())
          return;

        auto output = query->buffer;
        output.run_time_attributes.setValue("origin", node->getName()); //origin
        PrintInfo("BoxQuery executeBoxQuery", node->getName(), I, "/", N, "/", EndH, "/", dataset->getMaxResolution(),"done in", t1.elapsedMsec(),"msec",
          "dims", output.dims,"dtype", output.dtype,
          "mem", StringUtils::getStringFromByteSize(output.c_size()),
          "access", access ? "yes" : "nullptr",
          "url", dataset->getUrl());
        
        doPublish(output, query);
        dataset->nextBoxQuery(query);
        I++;
      }
    }

    PrintInfo("Query finished in",T1.elapsedMsec(),"msec");
  }

  //doPublish
  void doPublish(Array output, SharedPtr<BoxQuery> query)
  {
    int pdim = dataset->getPointDim();

    if (auto filter= query->filter.dataset_filter)
      output = filter->dropExtraComponentIfExists(output);

    DataflowMessage msg;
    output.bounds = dataset->logicToPhysic(query->logic_box);

    if (pdim==3)
      output.clipping = dataset->logicToPhysic(this->logic_position);

    //a projection happened?
#if 1
    if (query->logic_samples.nsamples != output.dims)
    {
      //disable clipping
      output.clipping = Position::invalid();

      //fix bounds
      auto T   = output.bounds.getTransformation();
      auto box = output.bounds.getBoxNd();
      for (int D = 0; D < pdim; D++)
      {
        if (query->logic_samples.nsamples[D] > 1 && output.dims[D] == 1)
          box.p2[D] = box.p1[D];
      }
      output.bounds = Position(T, box);
    }
#endif

    msg.writeValue("array", output);
    node->publish(msg);
  }

};

std::function<bool(Int64)> QueryNode::willFitOnGpu;

///////////////////////////////////////////////////////////////////////////
QueryNode::QueryNode() 
{
  addInputPort("dataset");
  addInputPort("fieldname");
  addInputPort("time");

  addOutputPort("array");
}

///////////////////////////////////////////////////////////////////////////
QueryNode::~QueryNode(){
}



///////////////////////////////////////////////////////////////////////////
void QueryNode::execute(Archive& ar)
{
  if (ar.name == "SetVerbose")
  {
    int value;
    ar.read("value", value);
    setVerbose(value);
    return;
  }

  if (ar.name == "SetAccessIndex")
  {
    int value;
    ar.read("value", value);
    setAccessIndex(value);
    return;
  }

  if (ar.name == "SetViewDependentEnabled")
  {
    bool value;
    ar.read("value", value);
    setViewDependentEnabled(value);
    return;
  }

  if (ar.name == "SetProgression")
  {
    int value;
    ar.read("value", value);
    setProgression(value);
    return;
  }

  if (ar.name == "SetQuality")
  {
    int value;
    ar.read("value", value);
    setQuality(value);
    return;
  }

  if (ar.name == "SetAccuracy")
  {
    double value;
    ar.read("value", value);
    setAccuracy(value);
    return;
  }

  if (ar.name == "SetBounds")
  {
    Matrix T; BoxNd box;
    ar.read("T", T);
    ar.read("box", box);
    setBounds(Position(T,box));
    return;
  }

  return Node::execute(ar);

}

///////////////////////////////////////////////////////////////////////////
Field QueryNode::getField()
{
  VisusAssert(VisusHasMessageLock());
  auto dataset = getDataset();
  if (!dataset)
    return Field();

  auto fieldname = readValue<String>("fieldname");
  return fieldname? dataset->getField(*fieldname) : dataset->getField();
}


///////////////////////////////////////////////////////////////////////////
double QueryNode::getTime()
{
  VisusAssert(VisusHasMessageLock());
  auto dataset = getDataset();
  if (!dataset)
    return 0.0;

  auto time = readValue<double>("time");
  return time ? *time : dataset->getTime();
}


//////////////////////////////////////////////////////////////////
DatasetNode* QueryNode::getDatasetNode()
{
  VisusAssert(VisusHasMessageLock());
  if (!isInputConnected("dataset")) return nullptr;
  return dynamic_cast<DatasetNode*>((*getInputPort("dataset")->inputs.begin())->getNode());
}

//////////////////////////////////////////////////////////////////
SharedPtr<Dataset> QueryNode::getDataset() 
{
  VisusAssert(VisusHasMessageLock());
  return readValue<Dataset>("dataset");
}


///////////////////////////////////////////////////////////////////////////
Frustum QueryNode::logicToScreen()  
{
  auto dataset = getDataset();
  if (!dataset)
    return Frustum();

  auto physic_to_screen = nodeToScreen();
  if (!physic_to_screen.valid())
    return Frustum();

  return dataset->logicToScreen(physic_to_screen);
}


///////////////////////////////////////////////////////////////////////////
void QueryNode::setBounds(Position new_value) 
{
  auto& old_value = this->node_bounds;
  if (old_value == new_value) 
    return;

  beginUpdate(
    StringTree("SetBounds").write("T", new_value.getTransformation()).write("box", new_value.getBoxNd()),
    StringTree("SetBounds").write("T", old_value.getTransformation()).write("box", old_value.getBoxNd()));
  {
    old_value = new_value;
  }
  endUpdate();
}

///////////////////////////////////////////////////////////////////////////
Position QueryNode::getQueryLogicPosition() 
{
  auto dataset = getDataset();
  if (!dataset)
    return Position();

  auto query_bounds = getQueryBounds();
  if (!query_bounds.valid())
    return Position();

  auto physic_to_screen = nodeToScreen();
  if (physic_to_screen.valid())
  {
    auto map = FrustumMap(physic_to_screen);
    query_bounds = Position::shrink(physic_to_screen.getScreenBox(), map, query_bounds);
    if (!query_bounds.valid())
      return Position();
  }

  //find intersection with dataset box
  auto logic_position = dataset->physicToLogic(query_bounds);
  auto map = MatrixMap(Matrix::identity(dataset->getPointDim()));
  logic_position = Position::shrink(dataset->getLogicBox().castTo<BoxNd>(), map, logic_position);

  if (!logic_position.valid())
    return Position();

  return logic_position;
}

///////////////////////////////////////////////////////////////////////////
bool QueryNode::processInput()
{
  abortProcessing();

  auto failed = [&]() {
    publishDumbArray();
    return false;
  };

  auto dataset = getDataset();
  if (!dataset)
    return failed();

  //create (and store in my class the access)
  if (!this->access)
  {
    auto access_configs = dataset->getAccessConfigs();

    if (this->accessindex >= 0 && this->accessindex < (int)access_configs.size())
      setAccess(dataset->createAccess(*access_configs[this->accessindex]));
    else
      setAccess(dataset->createAccess());
  }
 
  addNodeJob(std::make_shared<MyJob>(this, dataset, access));
  return true;
}

//////////////////////////////////////////////////////////////////
void QueryNode::publishDumbArray()
{
  auto buffer=std::make_shared<Array>();
  buffer->bounds=Position::invalid();

  DataflowMessage msg;
  msg.writeValue("array", buffer);
  publish(msg);
}


//////////////////////////////////////////////////////////////////
void QueryNode::exitFromDataflow() 
{
  Node::exitFromDataflow();
  this->access.reset();
}

//////////////////////////////////////////////////////////////////
void QueryNode::write(Archive& ar) const
{
  Node::write(ar);

  ar.write("verbose", verbose);
  ar.write("accessindex", accessindex);
  ar.write("view_dependent_enabled", view_dependent_enabled);
  ar.write("progression", progression);
  ar.write("quality", quality);
  ar.write("accuracy", accuracy);

  ar.writeObject("node_bounds", node_bounds);

  //query_bounds is a runtime thingy
}

//////////////////////////////////////////////////////////////////
void QueryNode::read(Archive& ar)
{
  Node::read(ar);

  ar.read("verbose", verbose);
  ar.read("accessindex", accessindex);
  ar.read("view_dependent_enabled", view_dependent_enabled);
  ar.read("progression", progression);
  ar.read("quality", quality);
  ar.read("accuracy", accuracy);

  ar.readObject("node_bounds", node_bounds);

  //query_bounds is a runtime thingy
}



} //namespace Visus

