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
#include <Visus/DatasetFilter.h>
#include <Visus/Dataflow.h>
#include <Visus/VisusConfig.h>

namespace Visus {

///////////////////////////////////////////////////////////////////////////
class QueryNode::MyJob : public NodeJob
{
public:

  QueryNode*               node;
  SharedPtr<Dataset>       dataset;
  SharedPtr<Access>        access;
  SharedPtr<Query>         query;
  SharedPtr<PointQuery>    pointquery;
  SharedPtr<BoxQuery>      boxquery;
  bool                     verbose;
  SharedPtr<Semaphore>     waiting_ready;

  //this will allow more parallelism since I start the next query before the previous one got rendered
  bool                     bWaitReturnReceipt = false;

  //constructor
  MyJob(QueryNode* node_,SharedPtr<Dataset> dataset_,SharedPtr<Access> access_,SharedPtr<Query> query_)
    : node(node_),dataset(dataset_),access(access_),query(query_)
  {
    this->query->aborted=this->aborted;
    this->verbose=node->isVerbose();
    this->waiting_ready=std::make_shared<Semaphore>();

    this->boxquery   = std::dynamic_pointer_cast<BoxQuery>(query);
    this->pointquery = std::dynamic_pointer_cast<PointQuery>(query);

    //need custom doPublish for scripting since it will not always wish to wait for return receipt
    this->query->incrementalPublish=[this](Array output) {
      if (aborted() || !output)
        return;

      if (boxquery)
      {
        if (auto filter = boxquery->filter.dataset_filter)
          output = filter->dropExtraComponentIfExists(output);
      }

      //change refframe (Dataflow works in physic coordinates, Db in logic coordinates)
      output.bounds   = dataset->logicToPhysic(output.bounds);
      output.clipping = dataset->logicToPhysic(output.clipping);
    
      DataflowMessage msg;
      msg.writeValue("data", output);
	    this->node->publish(msg);
    };
  }

  //destructor
  virtual ~MyJob()
  {
  }

  //runJob
  virtual void runJob() override
  {
    for (int N=0;;N++)
    {
      if (aborted())
        return;

      Time t1=Time::now();
      if (aborted())
        return;

      std::ostringstream out;
      out << "Query msec(" << t1.elapsedMsec() << ") ";

      if (pointquery)
      {
        if (!dataset->executeQuery(access, pointquery))
          return;

        out << "level(" << N << "/" << pointquery->end_resolutions.size() << "/" << pointquery->cur_resolution << "/" << dataset->getMaxResolution() << ") ";
      }
      else
      {
        if (!dataset->executeQuery(access, boxquery))
          return;

        out << "level(" << N << "/" << boxquery->end_resolutions.size() << "/" << boxquery->cur_resolution << "/" << dataset->getMaxResolution() << ") ";
      }

      if (aborted())
        return;

      auto buffer=query->buffer;

      out << "dims(" << buffer.dims.toString() << ") "
        << "dtype(" << buffer.dtype.toString() << ") "
        << "access(" << (access ? "yes" : "nullptr") << ") "
        << "url(" << dataset->getUrl().toString() << ") ";
      
      if (verbose)
        VisusInfo()<<out.str();

      //publish the final result
      doPublish(buffer);
    }
  }

  //doPublish
  void doPublish(Array output)
  {
    if (aborted() || !output)
      return;

    if (boxquery)
    {
      if (auto filter = boxquery->filter.dataset_filter)
        output = filter->dropExtraComponentIfExists(output);
    }

    DataflowMessage msg;

    SharedPtr<ReturnReceipt> return_receipt;

    //TODO: enable or disable return receipt?
    if (bWaitReturnReceipt)
    {
      return_receipt = std::make_shared<ReturnReceipt>();
      msg.setReturnReceipt(return_receipt);
    }

    //change refframe (Dataflow works in physic coordinates, Db in logic coordinates)
    output.bounds   = dataset->logicToPhysic(output.bounds);
    output.clipping = dataset->logicToPhysic(output.clipping);

    msg.writeValue("data",output);

    //only if the publish went well, I could wait
    if (node->publish(msg))
    {
      if (return_receipt)
        return_receipt->waitReady(waiting_ready);
    }

    if (pointquery)
    {
      if (!dataset->nextQuery(pointquery))
        return;
    }
    else
    {
      if (!dataset->nextQuery(boxquery))
        return;
    }
  }

  //abort
  virtual void abort() override
  {
    NodeJob::abort();
    waiting_ready->up(); //in case I'm in waitReady
  }

};


///////////////////////////////////////////////////////////////////////////
QueryNode::QueryNode(String name) : Node(name)
{
  addInputPort("dataset");
  addInputPort("fieldname");
  addInputPort("time");

  addOutputPort("data");
}

///////////////////////////////////////////////////////////////////////////
QueryNode::~QueryNode(){
}

///////////////////////////////////////////////////////////////////////////
SharedPtr<Query> QueryNode::createQuery(int end_resolution,bool bExecute)
{
  auto dataset   = readValue<Dataset>("dataset");
  auto time      = readValue<double>("time");
  auto fieldname = readValue<String>("fieldname");

  //I always need a dataset
  if (!dataset)
    return SharedPtr<Query>();

  auto query_bounds = getQueryBounds();
  if (!query_bounds.valid())
    return SharedPtr<Query>();

  Frustum logic_to_screen;
  if (isViewDependentEnabled() && nodeToScreen().valid())
  {
    auto physic_to_screen = nodeToScreen();
    query_bounds = Position::shrink(physic_to_screen.getScreenBox(), FrustumMap(physic_to_screen), query_bounds);
    if (!query_bounds.valid())
      return SharedPtr<Query>();

    logic_to_screen = dataset->logicToScreen(physic_to_screen);
  }

  //find intersection with dataset box
  auto logic_position = dataset->physicToLogic(query_bounds);
  logic_position = Position::shrink(dataset->getLogicBox().castTo<BoxNd>(), MatrixMap(Matrix::identity(dataset->getPointDim())), logic_position);

  if (!logic_position.valid())
    return SharedPtr<Query>();

  Field field = fieldname ? dataset->getFieldByName(cstring(fieldname)) : dataset->getDefaultField();
  double timestep = time ? cdouble(time) : dataset->getDefaultTime();

  if (bool bPointQuery = dataset->getPointDim() == 3 && query_bounds.getBoxNd().toBox3().minsize() == 0)
  {
    auto query = std::make_shared<PointQuery>(dataset.get(), field, timestep, 'r');
    query->logic_position = logic_position;
    query->logic_to_screen = logic_to_screen;

    if (end_resolution == -1)
      query->end_resolutions = dataset->guessEndResolutions(logic_to_screen, logic_position, getQuality(), getProgression());
    else
      query->end_resolutions = { end_resolution };

    if (!dataset->beginQuery(query))
      return SharedPtr<Query>();

    if (bExecute && !dataset->executeQuery(dataset->createAccess(), query))
      return SharedPtr<Query>();

    return query;
  }
  else
  {
    auto query = std::make_shared<BoxQuery>(dataset.get(), field, timestep, 'r');
    query->filter.enabled = true;
    query->merge_mode = BoxQuery::InsertSamples;
    query->logic_position = logic_position;

    if (end_resolution == -1)
      query->end_resolutions = dataset->guessEndResolutions(logic_to_screen, logic_position, getQuality(), getProgression());
    else
      query->end_resolutions = { end_resolution };

    if (!dataset->beginQuery(query))
      return SharedPtr<Query>();

    if (bExecute && !dataset->executeQuery(dataset->createAccess(), query))
      return SharedPtr<Query>();

    return query;
  }
}


///////////////////////////////////////////////////////////////////////////
bool QueryNode::processInput()
{
  abortProcessing();


  auto dataset = readValue<Dataset>("dataset");

  if (!dataset) {
    publishDumbArray();
    return false;
  }

  //create (and store in my class the access)
  if (!this->access)
  {
    auto access_configs = dataset->getAccessConfigs();

    if (this->accessindex >= 0 && this->accessindex < (int)access_configs.size())
      setAccess(dataset->createAccess(*access_configs[this->accessindex]));
    else
      setAccess(dataset->createAccess());
  }

  auto query = createQuery(-1,/*bExecute*/false);
  if (!query) {
    publishDumbArray();
    return false;
  }

  addNodeJob(std::make_shared<MyJob>(this, dataset, access, query));
  return true;
 

}

//////////////////////////////////////////////////////////////////
void QueryNode::publishDumbArray()
{
  auto buffer=std::make_shared<Array>();
  buffer->bounds=Position::invalid();

  DataflowMessage msg;
  msg.writeValue("data", buffer);
  publish(msg);
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

//////////////////////////////////////////////////////////////////
Field QueryNode::getField()
{
  Dataset* dataset=getDataset().get(); 
  if (!dataset) return Field();
  String fieldname=cstring(readValue<String>("fieldname"));
  return dataset->getFieldByName(fieldname);
}

//////////////////////////////////////////////////////////////////
void QueryNode::exitFromDataflow() 
{
  Node::exitFromDataflow();
  this->access.reset();
}

//////////////////////////////////////////////////////////////////
void QueryNode::writeToObjectStream(ObjectStream& ostream) 
{
  if (ostream.isSceneMode())
  {
    // use same serialization for query which will include
    // a transformation matrix in case of rotated selection
    ostream.pushContext("bounds");
    node_bounds.writeToObjectStream(ostream);
    ostream.popContext("bounds");
    return;
  }

  Node::writeToObjectStream(ostream);

  ostream.write("verbose", cstring(verbose));
  ostream.write("accessindex",cstring(accessindex));
  ostream.write("view_dependent",cstring(bViewDependentEnabled));
  ostream.write("progression",std::to_string(progression));
  ostream.write("quality",std::to_string(quality));

  ostream.pushContext("bounds");
  node_bounds.writeToObjectStream(ostream);
  ostream.popContext("bounds");

  //position=fn(tree_position)
}

//////////////////////////////////////////////////////////////////
void QueryNode::readFromObjectStream(ObjectStream& istream) 
{
  if (istream.isSceneMode())
  {
    istream.pushContext("bounds");
    node_bounds.readFromObjectStream(istream);
    istream.popContext("bounds");
    return;
  }

  Node::readFromObjectStream(istream);

  this->verbose = cint(istream.read("verbose"));
  this->accessindex=cint(istream.read("accessindex"));
  this->bViewDependentEnabled=cbool(istream.read("view_dependent"));
  this->progression=(QueryProgression)cint(istream.read("progression"));
  this->quality=(QueryQuality)cint(istream.read("quality"));

  istream.pushContext("bounds");
  node_bounds.readFromObjectStream(istream);
  istream.popContext("bounds");

  //position=fn(tree_position)
}



} //namespace Visus

