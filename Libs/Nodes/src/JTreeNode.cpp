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

#include <Visus/JTreeNode.h>
#include <Visus/Graph.h>
#include <Visus/UnionFind.h>

namespace Visus {

#ifndef JTREE_VERIFY_TREE
#define JTREE_VERIFY_TREE 0
#endif


/////////////////////////////////////////////////////////////////////
template <typename CppType>
class BuildJTreeNodeUtils
{
public:

  typedef Graph<CppType*,CppType>          MyGraph;
  typedef typename MyGraph::Vertex         Vertex;
  typedef typename MyGraph::Edge           Edge;

  
  /////////////////////////////////////////////////////////////////////
  //weight comparison between two pairs of critical points
  //(also todo: this means edge weight isn't really that important to store...)
  class JTreeWeightComp
  {
  public:

    const MyGraph &graph;
    bool lt;

    JTreeWeightComp(const MyGraph &g,bool _lt=true) :graph(g),lt(_lt)
    {}

    //operator()
    virtual inline bool operator()(int e0,int e1) 
    {
      bool ret=weight_comp(graph.verts[graph.edges[e0].src].data, graph.verts[graph.edges[e0].dst].data,
                            graph.verts[graph.edges[e1].src].data, graph.verts[graph.edges[e1].dst].data);
      return lt?!ret:ret;
    }

    //weight_comp
    static inline bool weight_comp(CppType *p0, CppType *p1,CppType *q0, CppType *q1)
    {
      //compare edge weight first...
      CppType _p=(CppType)fabs((double)(*p1-*p0));
      CppType _q=(CppType)fabs((double)(*q1-*q0));
      if (_p!=_q) return _p<_q;

      //use in-memory distance as tiebreaker...
      #define PTR_ABS_DIFF(a,b) ((b-a)>0?(b-a):(a-b))
      _p=(CppType)fabs((double)PTR_ABS_DIFF(p0,p1));
      _q=(CppType)fabs((double)PTR_ABS_DIFF(q0,q1));
      #undef PTR_ABS_DIFF
      if (_p!=_q) return _p<_q;

      //use lowest pair memory locations as secondary tiebreaker.
      return (p0<p1?p0:p1)<(q0<q1?q0:q1);
    }

    //weight_comp_lt
    static inline bool weight_comp_lt(CppType *p0, CppType *p1,CppType *q0, CppType *q1)
    {return !weight_comp(p0,p1,q0,q1);}
  };

  //withinThreeShold
  static inline bool withinThreeShold(const CppType& v,const CppType& m,const CppType& M)
  {return m<=v && v<=M;}
  
  // pointer comparison, value first, tiebreaking using addresses
  static  inline bool ptr_comp(CppType *first, CppType *second)
  {return *first<*second || (*first==*second && first<second);}

  static  inline bool ptr_comp_lt(CppType *first, CppType *second)
  {return !ptr_comp(first,second);}

  ////////////////////////////////////////////////////////////
  // Return addresses of p's 6 neighbors based on dims (NULL if no neighbor).
  //  note: v must be a pointer to array which can hold six elements
  //
  // 6 neighbors, to the left, to the right, above, below, in front of, behind
  //  order --> x-1,x+1,y-1,y+1,z-1,z+1
  ////////////////////////////////////////////////////////////
  static void getNeighbors(CppType *v[6], const CppType *p, CppType *data, const Point3i &dims)
  {
    VisusAssert(p>=data && p<data+dims[0]*dims[1]*dims[2]);
    const int stridex= (int)(1);
    const int stridey= (int)(dims[0]);
    const int stridez= (int)(dims[0]*dims[1]);

    const CppType *q  =(const CppType*)(p   -  data);
    size_t   qz =(size_t)q   /     stridez;
    size_t   qzr=(size_t)q   -  qz*stridez;
    size_t   qy =qzr         /     stridey;
    size_t   qyr=qzr         %     stridey;
    size_t   qx =qyr;
    VisusAssert(p==(data+qx+qy*dims[0]+qz*(dims[0]*dims[1])));

    v[0]=qx==0       ?0:data+qx-1+ qy   *stridey+ qz   *stridez;
    v[1]=qx==dims[0]-1?0:data+qx+1+ qy   *stridey+ qz   *stridez;
    v[2]=qy==0       ?0:data+qx  +(qy-1)*stridey+ qz   *stridez;
    v[3]=qy==dims[1]-1?0:data+qx  +(qy+1)*stridey+ qz   *stridez;
    v[4]=qz==0       ?0:data+qx  + qy   *stridey+(qz-1)*stridez;
    v[5]=qz==dims[2]-1?0:data+qx  + qy   *stridey+(qz+1)*stridez;
  }

  /////////////////////////////////////////////
  //VERIFY
  /////////////////////////////////////////////
  static void verifyTree(const MyGraph &graph)
  {
    // verify tree by walking all sub-trees and deleting branches
    //MyGraph vtree(graph);

    //<ctc> this copy fails because of Data copy ctor and locks. Ugh. Manually copying for now...
    MyGraph vtree;
    vtree.verts=graph.verts;
    vtree.edges=graph.edges;
    vtree.position=graph.position;

    std::vector<int> queued;
    for (int i=0;i<(int)vtree.verts.size();i++)
    {
      const Vertex &v=vtree.verts[i];
      if (v.deleted) continue;
      if (!v.out_degree()) // it's a root
        queued.push_back(i);
    }

    while (!queued.empty())
    {
      int i=queued.back(); queued.pop_back();
      std::vector<int> component;
      component.push_back(i);
      while (!component.empty())
      {
        int j=component.back(); component.pop_back();
        Vertex &v=vtree.verts[j];
        VisusAssert(!v.deleted);
        v.deleted=true; // mark as visited
        int in_degree=v.in_degree();
        for (int k=0;k<in_degree;k++) 
        {
          Edge &e=vtree.edges[v.in[k]];
          if (e.deleted) continue;
          Vertex &s=vtree.verts[e.src];
          VisusAssert(!s.deleted);
          component.push_back(e.src);
        }
      }
    }
    int sz=(int)vtree.verts.size();
    for (int i=0;i<(int)vtree.verts.size();i++)
    {
      const Vertex &v=vtree.verts[i];
      if (!v.deleted) 
      {
        int k=0; k++;
        PrintInfo("(verifyTree) found an unvisited vertex",i);
        VisusAssert(false);
      }
    }  
  }

  ////////////////////////////////////////////////////////////
  // construct join tree algorithm from
  // Computing Contour Trees in All Dimensions [algorithm 4.1]
  // Hamish Carr | Jack Snoeyink | Ulrike Axen
  ////////////////////////////////////////////////////////////
  static bool calculateJoinTree
  (
    MyGraph& graph,
    CppType* data,
    Point3i dims,
    bool minima_tree,
    CppType threshold_min,
    CppType threshold_max,
    Aborted& aborted
  )
  {
    Int64 numvertices=dims[0]*dims[1]*dims[2];

    //comparison
    bool (*comp_fcn)(CppType*,CppType*)=ptr_comp;
    if (minima_tree) comp_fcn=ptr_comp_lt;

    //STEP 1 sortVertives
    std::vector<CppType*> vertices;
    {
      if (aborted())
        return false;

      CppType *v=data;
      CppType *lastvertex=v+numvertices;

      for (; v<lastvertex; v++)
      {

        if (aborted())
          return false;

        if (withinThreeShold(*v,threshold_min,threshold_max))
        {
          vertices.push_back(v);
          std::push_heap(vertices.begin(), vertices.end(), comp_fcn);
        }
      }
    }

    //STEP 2 calculateJoinTree
    UnionFind<CppType*>      components;
    std::map<CppType*,int>   lastJoinVertex;
    std::map<CppType*,CppType*> componentMin;

    // Temporary to keep track of unique neighboring components and only
    // add vertices to the jtree when there will be a join.
    std::set<CppType*> toBeConnected;

    // Consider vertices from greatest to least, starting where we left off.
    CppType *v[6]; // place to hold p's neighbors
    while (!vertices.empty())
    {
      if (aborted())
        return false;

      CppType *p=vertices.front();
      std::pop_heap(vertices.begin(), vertices.end(), comp_fcn); vertices.pop_back();
      components.make_set(p);
      toBeConnected.clear();

      // for each vertex adjacent to p, try to add it to the tree
      getNeighbors(v,p,data,dims);
      for (int j=0;j<6;j++)
      {
        // note: must re-test threshold here
        if (!v[j] || !withinThreeShold(*v[j],threshold_min,threshold_max) || comp_fcn(v[j],p))
          continue; // skip smaller nodes and already-connected nodes

        toBeConnected.insert(components.find_set(v[j]));
      }

      // No neighbors, insert maximum.
      if (toBeConnected.empty())
      {
        int pv=graph.mkVert(p);
        lastJoinVertex[p]=pv;
        componentMin[p]=nullptr; // there is no minimum for current component.
      }
      else
      {
        typename std::set<CppType*>::iterator it=toBeConnected.begin();
        if (toBeConnected.size()==1)
        {
          // Only one neighbor, connect components but don't insert a vertex.
          CppType *_p=components.link(p,*it); // note: can use link because p and *it are their respective component's representative elements
          // Note  vertex as a potential component minimum.
          componentMin[_p]=p;
          lastJoinVertex[_p]=lastJoinVertex[*it]; // make sure lastJoinVertex is set for representative component
        }
        // else insert vertex and connect it to previously disconnected neighboring components.
        else
        {
          int pv=graph.mkVert(p);
          CppType *_p=p; // current representative of component containins p
          for (;it!=toBeConnected.end();it++)
          {
            VisusAssert(0==graph.verts[lastJoinVertex[*it]].out_degree());
            graph.mkEdge(lastJoinVertex[*it],pv,(CppType)fabs((double)*graph.verts[lastJoinVertex[*it]].data-*p));
            VisusAssert(graph.verts[lastJoinVertex[*it]].out_degree()==1);
            _p=components.link(_p,*it);
          }
          // finally, set lastJoinVertex of the newly connected supercomponent to the join vertex.
          lastJoinVertex[_p]=pv;
          componentMin[_p]=nullptr; // no longer a minimum for current component.
        }
      }
    }

    // Finally, add all the minima
    std::vector<int> danglers;
    for (int i=0;i<(int)graph.verts.size();i++)
    {
      if (aborted())
        return false;

      Vertex &v=graph.verts[i];
      VisusAssert(!v.deleted); // There shouldn't be deleted verts during construction.
      VisusAssert(v.out_degree()<=1);

      // If  vertex is a solo maxima or last join, add its corresponding minimum (if any)
      if (v.out_degree()==0)
      {
        danglers.push_back(i);
      }
    }

    for (int i=0;i<(int)danglers.size();i++)
    {
      if (aborted())
        return false;

      Vertex v=graph.verts[danglers[i]]; // note: don't use reference here b/c mkVert (below) can realloc!
      CppType *_v=v.data;
      CppType *_p=components.find_set(_v);
      CppType *cmin=componentMin[_p];
      if (cmin)
      {
        VisusAssert(v.out_degree()==0);
        int pv=graph.mkVert(cmin);
        graph.mkEdge(danglers[i],pv,(CppType)fabs((double)*_v-*cmin));
        VisusAssert(graph.verts[danglers[i]].out_degree()==1);
        VisusAssert(graph.verts[pv].in_degree()==1);
        VisusAssert(graph.verts[pv].out_degree()==0);
      }
      else if (v.in_degree()==0)
      {
        //PrintDebug("Removing solo vertex...");
        graph.rmVert(danglers[i]);
      }
    }

    return true;
  }  

  //printHeap
  static void printHeap(std::vector<int> heap,const MyGraph &g)
  {
    JTreeWeightComp ecomp(g);
    PrintDebug("heapsize: ",heap.size()," =====/////=====/////=====");
    int cnt=0;
    while (!heap.empty())
    {
      int idx=heap.front(); 
      std::pop_heap(heap.begin(), heap.end(), ecomp); heap.pop_back();
      const Edge &e=g.edges[idx];
      PrintDebug(cnt++,":",(float)e.data,"from",e.src,"d to",e.dst);
    }
  }

  //buildBranchDecomposition
  static std::vector<int> buildBranchDecomposition(MyGraph &tree,double min_persist,bool reduce_minmax,JTreeWeightComp &ecomp,Aborted& aborted)
  {    
    std::vector<int> branches;
    for (int i=0;i<(int)tree.edges.size();i++)
    {
      if (aborted())
        return std::vector<int>();

      Edge &e=tree.edges[i];
      if (e.deleted || e.data>=min_persist) continue;
      Vertex &s=tree.verts[e.src];
      Vertex &d=tree.verts[e.dst];
      VisusAssert(!s.deleted && !d.deleted);

      // if s is a leaf and d is not a root (or reduce_minmax is true)
      if (s.in_degree()==0 && (reduce_minmax || d.out_degree()!=0))
        branches.push_back(i);
    }
    std::make_heap(branches.begin(), branches.end(), ecomp);
    return branches;
  }

  //reduceJoinTree
  static bool reduceJoinTree
  (
    FGraph& ret,
    MyGraph&  full_graph,
    CppType* data,
    Point3i dims, 
    double min_persist,
    bool reduce_minmax,
    std::vector<int> &branch_decomposition,
    Aborted& aborted
  )
  {
    PrintDebug("reduceJoinTree","min_persist",min_persist,"reduce_minmax",reduce_minmax);

    // make a copy (in case I will need the full_graph later)
    //<ctc> this copy fails because of Data copy ctor and locks. Ugh. Manually copying for now...
    MyGraph tree;
    tree.verts=full_graph.verts;
    tree.edges=full_graph.edges;
    tree.bounds=full_graph.bounds;

    #if JTREE_VERIFY_TREE
    verifyTree(tree);
    #endif

    // STEP 1 
    // Revised simplification based on branch reduction (but really, a conversation with Atilla :)
    // Sort leaf edges into priority_queue from least to greatest.
    // Remove edges and when reductions occur, add the edge back
    // into the queue (if it's > persistence threshold).
    JTreeWeightComp ecomp(tree);
    std::vector<int> branches=buildBranchDecomposition(tree,min_persist,reduce_minmax,ecomp,aborted);

    while (!branches.empty())
    {
      if (aborted())
        return false;

      //printHeap(branches,tree);

      #if JTREE_VERIFY_TREE
      verifyTree(tree);
      #endif

      int idx=branches.front(); 
      std::pop_heap(branches.begin(), branches.end(), ecomp); branches.pop_back();
      Edge &e=tree.edges[idx];

      if (e.deleted) continue;
      int newedge=tree.rmVertSmart(e.src);
      if (newedge)
      {
        // if newedge (from reduction) is also a low-persistence leaf branch, add it to the queue
        Edge &ne=tree.edges[newedge];
        VisusAssert(!ne.deleted);
        if (ne.data>=min_persist) continue;
        Vertex &ns=tree.verts[ne.src];
        Vertex &nd=tree.verts[ne.dst];
        VisusAssert(!ns.deleted && !nd.deleted);

        // if ns is a leaf and nd is not a root (or reduce_minmax is true)
        if (ns.in_degree()==0 && (reduce_minmax || nd.out_degree()!=0))
        {
          branches.push_back(newedge); 
          std::push_heap(branches.begin(), branches.end(), ecomp);
        }
      }
    }

    #if JTREE_VERIFY_TREE
    verifyTree(tree);
    #endif

    branch_decomposition=buildBranchDecomposition(tree,min_persist,reduce_minmax,ecomp,aborted);

    // STEP 2 produce the results graph (FPoints with float edge weights)

    const CppType *q;
    size_t   qz,qzr,qy,qyr,qx;
    ret.verts.resize(tree.verts.size());

    Point3f invdims(1.0f/dims[0],1.0f/dims[1],1.0f/dims[2]);

    for (int i=0;i<(int)tree.verts.size();i++)
    {
      if (aborted())
        return false;

      Vertex &v=tree.verts[i];
      ret.verts[i].deleted=v.deleted;
      if (v.deleted) continue;

      q  =(const CppType*)(v.data-data);
      qz =(size_t)q   /    (dims[0]*dims[1]);
      qzr=(size_t)q   -  qz*dims[0]*dims[1];
      qy =qzr         /     dims[0];
      qyr=qzr         %     dims[0];
      qx =qyr;
      ret.verts[i].data   =Point3f((float)qx*invdims[0],(float)qy*invdims[1],dims[2]==1?0.0f:(float)qz*invdims[2]);

      ret.verts[i].out    =v.out;
      ret.verts[i].in.resize(v.in.size());
      for (int j=0;j<(int)v.in.size();j++)
        ret.verts[i].in[j]=v.in[j];
    }

    ret.edges.resize(tree.edges.size());
    for (int i=0;i<(int)tree.edges.size();i++)
    {
      if (aborted())
        return false;

      Edge &e=tree.edges[i];
      ret.edges[i].src    =e.src;
      ret.edges[i].dst    =e.dst;
      ret.edges[i].deleted=e.deleted;
      ret.edges[i].data   =(float)e.data;
    }

    return true;
  }

};


/////////////////////////////////////////////////////////////////////
//TODO! BuildJTreeNodeJob must not access protected fields of JTreeNode
/////////////////////////////////////////////////////////////////////
template <typename CppType>
class JTreeNode::MyJob : public NodeJob, public BuildJTreeNodeUtils<CppType>
{
public:

  JTreeNode*                   node;
  Point3i                      idims;
  Point3d                      ddims;
  bool                         minima_tree;
  double                       threshold_min;
  double                       threshold_max;
  bool                         reduce_minmax;
  double                       min_persistence;
  Array                        data;

  typedef BuildJTreeNodeUtils<CppType> ParentClass;
  typedef typename ParentClass::MyGraph FullGraph;

  SharedPtr<FullGraph> full_graph;

  //constructor
  MyJob(JTreeNode* node_) : node(node_),data(node_->data) 
  {
    this->idims=data.dims.toPoint3();
    this->ddims=data.dims.toPoint3().template castTo<Point3d>();
    this->minima_tree=node->minima_tree;
    this->threshold_min=node->threshold_min;
    this->threshold_max=node->threshold_max;
    this->reduce_minmax=node->reduce_minmax;
    this->min_persistence=node->min_persistence;

    //trying to recycle the last graph
    //NOTE: doing the dynamic cast so that if they are not compatible (example: the CppType template changed) I'm automatically reset full_graph to zero
    this->full_graph=std::dynamic_pointer_cast<FullGraph>(node->last_full_graph);
  }

  //destructor
  virtual ~MyJob()
  {}

  //runJob
  virtual void runJob() override
  {
    CppType *data=(CppType*)this->data.c_ptr();     

    //recompute the full graph only if needed
    if (!this->full_graph)                                         
    {
      this->full_graph=std::make_shared<FullGraph>();
      this->full_graph->properties.setValue("minima_tree",cstring(this->minima_tree));
      if (!this->calculateJoinTree(*this->full_graph,data,idims,minima_tree,(CppType)threshold_min,(CppType)threshold_max,this->aborted))
        return;
    } 

    //simplify 
    auto reduced_graph=std::make_shared<FGraph>(); 
    reduced_graph->properties.setValue("minima_tree",cstring(this->minima_tree));

    std::vector<Int32> branches;
    if (!this->reduceJoinTree(*reduced_graph,*this->full_graph,data,idims,min_persistence,reduce_minmax,branches,this->aborted))
      return;

    //do publish
    {
      reduced_graph->bounds  = this->data.bounds; 

      //transform all points into dims space (they are currently on [0,1] space)
      for (int I=0;I<(int)reduced_graph->verts.size();I++)
      {
        FGraph::Vertex &v=reduced_graph->verts[I];
        if (v.deleted) continue;
        v.data[0]*=(float)ddims[0];
        v.data[1]*=(float)ddims[1];
        v.data[2]*=(ddims[2]==1?0.0f:(float)ddims[2]);
      }

      PrintInfo("Produced tree with",reduced_graph->verts.size(),"verts and",reduced_graph->edges.size(),"edges");

      DataflowMessage msg;
      msg.writeValue("graph", reduced_graph);
      msg.writeValue("array",this->data);
      msg.writeValue("branches",branches);
      msg.writeValue("full_graph",full_graph); //important to recycle the last graph
      node->publish(msg);
    }
  };

};


////////////////////////////////////////////////////////////
JTreeNode::JTreeNode()  
{
  addInputPort("array");

  addOutputPort("array"); //if you do not need the original data, simply do not connect it!
  addOutputPort("graph");
  addOutputPort("branches"); // branch decomposition
  addOutputPort("full_graph");
}

////////////////////////////////////////////////////////////
JTreeNode::~JTreeNode() {
}


////////////////////////////////////////////////////////////
void JTreeNode::execute(Archive& ar)
{
  if (ar.name == "SetMinimaTree") {
    bool value;
    ar.read("value", value);
    setMinimaTree(value);
    return;
  }

  if (ar.name == "SetMinPersistence") {
    double value;
    ar.read("value", value);
    setMinPersistence(value);
    return;
  }

  if (ar.name == "SetReduceMinMax") {
    bool value;
    ar.read("value", value);
    setReduceMinMax(value);
    return;
  }

  if (ar.name == "SetThresholdMin") {
    double value;
    ar.read("value", value);
    setThresholdMin(value);
    return;
  }

  if (ar.name == "SetThresholdMax") {
    double value;
    ar.read("value", value);
    setThresholdMax(value);
    return;
  }

  if (ar.name == "SetAutoThreshold") {
    bool value;
    ar.read("value", value);
    setAutoThreshold(value);
    return;
  }

  return Node::execute(ar);
}

////////////////////////////////////////////////////////////
bool JTreeNode::recompute(bool bFull)
{
  abortProcessing();

  if (bool bWrongInput=!this->data || this->data.dtype.ncomponents()!=1)
  {
    this->last_full_graph.reset();
    this->data=Array();
    return false;
  }

  if (bFull)
    last_full_graph.reset();

  SharedPtr<NodeJob> job;
  DType dtype=this->data.dtype;
  if      (dtype==(DTypes::INT8   )) job.reset(new MyJob<Int8   >(this));
  else if (dtype==(DTypes::UINT8  )) job.reset(new MyJob<Uint8  >(this));
  else if (dtype==(DTypes::INT16  )) job.reset(new MyJob<Int16  >(this));
  else if (dtype==(DTypes::UINT16 )) job.reset(new MyJob<Uint16 >(this));
  else if (dtype==(DTypes::INT32  )) job.reset(new MyJob<Int32  >(this));
  else if (dtype==(DTypes::UINT32 )) job.reset(new MyJob<Uint32 >(this));
  else if (dtype==(DTypes::INT64  )) job.reset(new MyJob<Int64  >(this));
  else if (dtype==(DTypes::UINT64 )) job.reset(new MyJob<Uint64 >(this));
  else if (dtype==(DTypes::FLOAT32)) job.reset(new MyJob<Float32>(this));
  else if (dtype==(DTypes::FLOAT64)) job.reset(new MyJob<Float64>(this));
  else 
  {
    this->last_full_graph.reset();
    this->data=Array();
    return false;
  }

  if (!last_full_graph)
  {
    PrintInfo("Recomputing join tree full graph...");
    updateAutoThreshold(); //NOTE I have to update here because I may need to update GUI stuff (i.e. views)
  }

  Node::addNodeJob(job);
  return true;
}

////////////////////////////////////////////////////////////
bool JTreeNode::processInput()
{
  abortProcessing();

  bool bFull=getInputPort("array")->hasNewValue();

  auto data = readValue<Array>("array");
  this->data=data? *data : Array();
  return recompute(bFull);
}

////////////////////////////////////////////////////////////
void JTreeNode::messageHasBeenPublished(DataflowMessage msg)
{
  if (auto full_graph=msg.readValue<BaseGraph>("full_graph"))
  {
    auto data=msg.readValue<Array>("array"); VisusAssert(data);
    this->last_full_graph=full_graph; //recycling the last full graph
  }
}

////////////////////////////////////////////////////////////
void JTreeNode::updateAutoThreshold()
{
  if (!this->auto_threshold || !this->data) 
    return;

  //if the data does not come with a valid range, calculate it
  VisusAssert(this->data.dtype.ncomponents()==1);
  Range range=this->data.dtype.getDTypeRange();

  if (!range.delta()) 
    range=ArrayUtils::computeRange(data,0);
  
  double Min=range.from;
  double Max=range.to;
  double Mid=0.5*(Max+Min);

  this->minThresholdRange = Min;
  this->maxThresholdRange = Max;

  this->threshold_min=this->minima_tree? Min:Mid;
  this->threshold_max=this->minima_tree? Mid:Max;
}


////////////////////////////////////////////////////////////
void JTreeNode::write(Archive& ar) const
{
  Node::write(ar);

  ar.write("minima_tree", minima_tree);
  ar.write("min_persistence", min_persistence);
  ar.write("reduce_minmax", reduce_minmax);
  ar.write("threshold_min", threshold_min);
  ar.write("threshold_max", threshold_max);
  ar.write("auto_threshold", auto_threshold);
}

////////////////////////////////////////////////////////////
void JTreeNode::read(Archive& ar)
{
  Node::read(ar);

  ar.read("minima_tree", minima_tree);
  ar.read("min_persistence", min_persistence);
  ar.read("reduce_minmax", reduce_minmax);
  ar.read("threshold_min", threshold_min);
  ar.read("threshold_max", threshold_max);
  ar.read("auto_threshold", auto_threshold);
}

} //namespace Visus


