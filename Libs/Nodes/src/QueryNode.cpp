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

bool QueryNode::bDisableFindQUeryIntersectionWithDatasetBox = false;


///////////////////////////////////////////////////////////////////////////
class QueryNode::MyJob : public NodeJob
{
public:

  QueryNode*               node;
  SharedPtr<Dataset>       dataset;
  SharedPtr<Access>        access;
  SharedPtr<Query>         query;
  bool                     verbose;
  SharedPtr<Semaphore>     waiting_ready;
  Int64                    max_query_size;

  //this will allow more parallelism since I start the next query before the previous one got rendered
  bool                     bWaitReturnReceipt = false;

  //constructor
  MyJob(QueryNode* node_,SharedPtr<Dataset> dataset_,SharedPtr<Access> access_,SharedPtr<Query> query_,Int64 max_query_size_=0)
    : node(node_),dataset(dataset_),access(access_),query(query_),max_query_size(max_query_size_)
  {
    this->query->aborted=this->aborted;
    this->verbose=cbool(VisusConfig::readString("Configuration/QueryNode/verbose","true"));
    this->waiting_ready=std::make_shared<Semaphore>();

    //need custom doPublish for scripting since it will not always wish to wait for return receipt
    this->query->incrementalPublish=[this](Array data)
    {
      if (aborted() || !data)
        return;

      if (auto filter=query->filter.value)
        data=filter->dropExtraComponentIfExists(data);
    
      DataflowMessage msg;
      msg.writeValue("data", data);
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

      //if too many samples (they wouldn't fit on gpu anyway), end the job
      if (max_query_size>0 && query->nsamples.innerProduct()*query->field.dtype.getByteSize() > max_query_size)
        return;

      Time t1=Time::now();
      if (aborted())
        return;

      if (!dataset->executeQuery(access,query))
        return;

      if (aborted())
        return;

      auto buffer=query->buffer;
      
      if (verbose)
      {
        std::ostringstream out;
        out<<"Query msec(" <<t1.elapsedMsec()<<") "
        <<"level("<<N<<"/"<<query->end_resolutions.size()<<"/"<<query->cur_resolution<<"/"<<query->max_resolution<<") "
        <<"dims(" <<buffer.dims.toString()<<") "
        <<"dtype("<<buffer.dtype.toString()<<") "
        <<"filter("<<(query->filter.value?query->filter.value->getName():"nullptr")<<") "
        <<"access("<<(access?"yes":"nullptr")<<") "
        <<"url("<<dataset->getUrl().toString()<<") ";
        VisusInfo()<<out.str();
        }

      //publish the final result
      doPublish(buffer);
    }
  }

  //doPublish
  void doPublish(Array output)
  {
    if (aborted() || !output)
      return;

    if (auto filter=query->filter.value)
      output=filter->dropExtraComponentIfExists(output);
    
    DataflowMessage msg;

    SharedPtr<ReturnReceipt> return_receipt;

    //TODO: enable or disable return receipt?
    if (bWaitReturnReceipt)
    {
      return_receipt = std::make_shared<ReturnReceipt>();
      msg.setReturnReceipt(return_receipt);
    }

    msg.writeValue("data",output);

    //only if the publish went well, I could wait
    if (node->publish(msg))
    {
      if (return_receipt)
        return_receipt->waitReady(waiting_ready);
    }

    if (!dataset->nextQuery(query))
      return;
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
bool QueryNode::processInput()
{
  abortProcessing();

  //important to do before readValue
  auto dataset          = readValue<Dataset>("dataset");
  auto time             = readValue<double>("time");
  auto fieldname        = readValue<String>("fieldname");

  //I always need a dataset
  if (!dataset)
    return false;

  auto query=std::make_shared<Query>(dataset.get(),'r');
  query->filter.enabled=true;
  query->merge_mode=Query::InsertSamples;

  //position
  auto position=getQueryPosition();
  if (!position.valid())
    return false;

  //time
  if (time)
    query->time=(cdouble(time));

  //create (and store in my class the access)
  if (!access)
  {
    std::vector<StringTree*> access_configs=dataset->getAccessConfigs();
    if (accessindex>=0 && accessindex<(int)access_configs.size())
      setAccess(dataset->createAccess(*access_configs[accessindex]));
    else
      setAccess(dataset->createAccess());
  }

  //field
  if (fieldname)
    query->field=dataset->getFieldByName(cstring(fieldname));

  //set position
  Int64 max_query_size=0;
  {
    Frustum viewdep;
    if (isViewDependentEnabled() && getViewDep().valid())
    {
      viewdep=getViewDep();
      position=Position::shrink(viewdep.getScreenBox(),FrustumMap(viewdep),position);
      if (!position.valid())
      {
        publishDumbArray();
        return false;
      }
    }

    //find intersection with dataset box
    if (!bDisableFindQUeryIntersectionWithDatasetBox)
      position=Position::shrink(Position(dataset->getBox()).withoutTransformation().getBox(),MatrixMap(Matrix::identity()),position);

    if (!position.valid()) 
    {
      publishDumbArray();
      return false;
    }


    query->position=position;
    query->viewdep=viewdep;
  }

  //end resolutions
  query->end_resolutions=dataset->guessEndResolutions(query->viewdep,query->position,getQuality(),getProgression());
 
  //failed for some reason
  if (!dataset->beginQuery(query)) 
    return false;

  addNodeJob(std::make_shared<MyJob>(this, dataset, access, query, max_query_size));
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
    ostream.pushContext("query");
    ostream.pushContext("bounds");
    bounds.writeToObjectStream(ostream);
    ostream.popContext("bounds");
    ostream.popContext("query");
    return;
  }

  Node::writeToObjectStream(ostream);
  ostream.write("accessindex",cstring(accessindex));
  ostream.write("view_dependent",cstring(bViewDependentEnabled));
  ostream.write("progression",std::to_string(progression));
  ostream.write("quality",std::to_string(quality));

  ostream.pushContext("bounds");
  bounds.writeToObjectStream(ostream);
  ostream.popContext("bounds");

  //position=fn(tree_position)
}

//////////////////////////////////////////////////////////////////
void QueryNode::readFromObjectStream(ObjectStream& istream) 
{
  if (istream.isSceneMode())
  {
    istream.pushContext("bounds");
    bounds.readFromObjectStream(istream);
    istream.popContext("bounds");
    return;
  }

  Node::readFromObjectStream(istream);
  this->accessindex=cint(istream.read("accessindex"));
  this->bViewDependentEnabled=cbool(istream.read("view_dependent"));
  this->progression=(Query::Progression)cint(istream.read("progression"));
  this->quality=(Query::Quality)cint(istream.read("quality"));

  istream.pushContext("bounds");
  bounds.readFromObjectStream(istream);
  istream.popContext("bounds");

  //position=fn(tree_position)
}



} //namespace Visus

