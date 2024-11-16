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

#include <Visus/VoxelScoopNode.h>
#include <Visus/UnionFind.h>

namespace Visus {

///////////////////////////////////////////////////////////////
class RayBurst
{
public:

  //_____________________________________________
  class Fan
  {
  public:

    class Params
    {
    public:
      unsigned char  *image=nullptr;
      int             image_width=0;
      int             image_height=0;
      int             image_length=0;
      double          voxel_width=0;
      double          voxel_height=0;
      double          voxel_length=0;
      double          threshold=0;
      double          origin_x=0;
      double          origin_y=0;
      double          origin_z=0;
      double          fit_percent=0;
      int             max_iterations=0;
    };

    int      vertex_count=0;
    double  *vertices=nullptr;
    double   diameter=0;
    double   area=0;

    //constructor
    Fan(Params params) {
      ThrowException("TODO to implement...");
    }
  };

  Array data;

  //constructor
  RayBurst() {
  }

  //destructor
  ~RayBurst(){
  }

  //setData
  bool setData(Array data)
  {
    // for uint8 scalar fields only
    if (!(data.valid() && data.dtype==(DTypes::UINT8)))
      return false;

    this->data=data;
    return true;
  }

  //getDiam2d
  double getDiam2d(const Point3d &seed,double threshold)
  {
    auto pixel_to_bounds = Position::computeTransformation(this->data.bounds,this->data.dims);

    TRSMatrixDecomposition trs(pixel_to_bounds);

    Fan::Params params;
    params.image          = this->data.c_ptr();
    params.image_width    = (int)this->data.getWidth ();
    params.image_height   = (int)this->data.getHeight();
    params.image_length   = (int)this->data.getDepth ();
    params.voxel_width    = trs.scale[0];
    params.voxel_height   = trs.scale[1];
    params.voxel_length   = trs.scale[2];
    params.threshold      = threshold;
    params.origin_x       = seed[0]*trs.scale[0]-trs.translate[0];  // note: seed must be in scaled (physical) coordinates
    params.origin_y       = seed[1]*trs.scale[1]-trs.translate[1];
    params.origin_z       = seed[2]*trs.scale[2]-trs.translate[2];
    params.fit_percent    = 1.0;
    params.max_iterations = 6;

    UniquePtr<Fan> srb(new  Fan(params));
    double retval=srb->diameter; //get diameter
    return retval;
  }

};

typedef VoxelScoopNode::TrimGraph TrimGraph;

typedef TrimGraph::vT TNode;
typedef TrimGraph::eT TEdge;

////////////////////////////////////////////////////////////
std::ostream& operator<<(std::ostream &out,const TNode &n)
{
  out<<"s="<<n.s;
  return out;
}


/////////////////////////////////////////////
template<class vT,class eT>
void genDot2(const Graph<vT,eT> &g, std::ostream &out, typename Graph<vT,eT>::Vertex *highlight=nullptr)
{
  typedef typename Graph<vT,eT>::CVertIter citer_t;
  typedef typename Graph<vT,eT>::CEdgeIter eiter_t;
  typedef typename Graph<vT,eT>::Vertex    vert_t;
  typedef typename Graph<vT,eT>::Edge      edge_t;

  out << "digraph G {" << std::endl;

  // vertices
  int i=0;
  for (citer_t it=g.verts.begin();it!=g.verts.end();it++,i++)
  {
    const vert_t *v=&(*it);
    if (v->deleted)
      continue;

    out << i << " [label=\"" << i;
    out << "\"";
    int indeg =(*it).in_degree();
    int outdeg=(*it).out_degree();
    if (v==highlight)
      out << ",style=filled,fillcolor=green,color=green,style=bold,peripheries=2]";

    else if (indeg == 0) 
      out << ",style=filled,fillcolor=red]";    //LEAF (local maxima)

    else if (outdeg == 0 && indeg>0)
      out << ",style=filled,fillcolor=blue]";   //ROOT (global minima)

    else if (indeg==1&&outdeg==1)
      out << ",style=filled,fillcolor=grey]";   //INTERNAL (normal)

    else if (indeg+outdeg>2)
      out << ",style=filled,fillcolor=yellow]"; //SADDLE (join)

    else
      out << "]";

    out << std::endl;
  }

  // edges
  i=0;
  for (eiter_t eit=g.edges.begin();eit!=g.edges.end();eit++,i++)
  {
    const edge_t *e=&(*eit);
    if (e->deleted) 
      continue;

    out << eit->src << "->" << eit->dst << "[label=\"" << i << " (" << e->data.length << ")\"]\n";
  }

  out << "}" << std::endl;
}

/////////////////////////////////////////////
// <ctc> this is ridiculous way to have more information in dot file for specific graph type. 
template<class vT,class eT>
void genDot3(const Graph<vT,eT> &g, std::ostream &out, typename Graph<vT,eT>::Vertex *highlight=nullptr)
{
  typedef typename Graph<vT,eT>::CVertIter citer_t;
  typedef typename Graph<vT,eT>::CEdgeIter eiter_t;
  typedef typename Graph<vT,eT>::Vertex    vert_t;
  typedef typename Graph<vT,eT>::Edge      edge_t;

  out << "digraph G {" << std::endl;

  // vertices
  int i=0;
  for (citer_t it=g.verts.begin();it!=g.verts.end();it++,i++)
  {
    const vert_t *v=&(*it);
    if (v->deleted)
      continue;

    out << i << " [label=\"" << i;
    if (v->data.calculated_max_in_length && v->data.calculated_max_out_lengths)
    {
      int outidx=(v->out_degree()>0)?v->out[0]:-1;
      out << " \\nmax: out("<<outidx<<"):" << v->data.max_in_length << ",";
      for (int j=0;j<v->in_degree();j++)
      {
        out<<"in"<<j<<"("<<v->in[j]<<"):"<<v->data.max_out_lengths[j]
           <<(j<v->in_degree()-1?",":"");
      }
    }
    else
    {
      out << "\\nhave not calculated max in/out lengths";
    }
    out << "\"";
    int indeg =(*it).in_degree();
    int outdeg=(*it).out_degree();
    if (v==highlight)
      out << ",style=filled,fillcolor=green,color=green,style=bold,peripheries=2]";

    else if (indeg == 0) 
      out << ",style=filled,fillcolor=red]";    //LEAF (local maxima)

    else if (outdeg == 0 && indeg>0)
      out << ",style=filled,fillcolor=blue]";   //ROOT (global minima)

    else if (indeg==1&&outdeg==1)
      out << ",style=filled,fillcolor=grey]";   //INTERNAL (normal)

    else if (indeg+outdeg>2)
      out << ",style=filled,fillcolor=yellow]"; //SADDLE (join)

    else
      out << "]";

    out << std::endl;
  }

  // edges
  i=0;
  for (eiter_t eit=g.edges.begin();eit!=g.edges.end();eit++,i++)
  {
    const edge_t *e=&(*eit);
    if (e->deleted) 
      continue;

    out << eit->src << "->" << eit->dst << "[label=\"" << i << " (" << e->data.length << ")\"]\n";
  }

  out << "}" << std::endl;
}

/////////////////////////////////////////////
class BuildVoxelScoop
{
public:

  // tracks whether the given vertex has been visited
  struct VisitData 
  { 
    bool flag; 
    VisitData() :flag(false) {} 
  };

  struct Cluster
  { 
    std::vector<Uint8*> elements; 
    double              aabbsize; 
    Point3d             nodepos; 
    int                 nodeid; 
    Uint8               maxval;
    Cluster() : aabbsize(0),nodeid(0),maxval(0)  {} 
  };


  Array                         Data;
  RayBurst                      rayburst;
  std::map<Uint8*,VisitData>    visited;
  std::vector<Cluster*>         clusters;
  SharedPtr<CGraph>             centerlines;
  int                           NumSeed;

  //configuration
  float      min_diam;
  Uint8 threshold;

  //simplification
  float min_length;
  float min_ratio;  // minimum ratio of length to diameter

  //constructor
  BuildVoxelScoop(Array Data=Array(),float min_diam=0,Uint8 threshold=0,float min_length=0,float min_ratio=0)
  {
    this->Data=Data;
    this->min_diam=min_diam;
    this->min_length=min_length;
    this->min_ratio=min_ratio;
    this->threshold=threshold;
    this->centerlines.reset(new CGraph());
    this->rayburst.setData(Data);
    this->NumSeed=0;
  }


  //return result
  SharedPtr<CGraph> getResult() {
    return this->centerlines;
  }

  ///////////////////////////////////////////////////////////////////////
  //This stencil handles boundary cases in the production of relative
  //offsets of the 26-neighborhood of the voxel at the given offset into
  //an array of size dims.
  ///////////////////////////////////////////////////////////////////////
  static void createNeighborStencil(const int offset,const Point3i &dims,int stencil[26])
  {
    const int stridex=(int)(1);
    const int stridey=(int)(dims[0]);
    const int stridez=(int)(dims[0]*dims[1]);
    int   qz =offset   /     stridez;
    int   qzr=offset   -  qz*stridez;
    int   qy =qzr         /     stridey;
    int   qyr=qzr         %     stridey;
    int   qx =qyr;
    VisusAssert(offset==qx+qy*dims[0]+qz*dims[0]*dims[1]);
    bool nz=qz>0;
    bool ny=qy>0;
    bool nx=qx>0;
    bool pz=qz<dims[2]-1;
    bool py=qy<dims[1]-1;
    bool px=qx<dims[0]-1;

    stencil[0] =nz*ny*nx*(-stridez-stridey-stridex);
    stencil[1] =nz*ny*   (-stridez-stridey);
    stencil[2] =nz*ny*px*(-stridez-stridey+stridex);
    stencil[3] =nz*nx*   (-stridez-stridex);
    stencil[4] =nz*      (-stridez);
    stencil[5] =nz*px*   (-stridez+stridex);
    stencil[6] =nz*py*nx*(-stridez+stridey-stridex);
    stencil[7] =nz*py*   (-stridez+stridey);
    stencil[8] =nz*py*px*(-stridez+stridey+stridex);
    stencil[9] =ny*nx*   (-stridey-stridex);
    stencil[10]=ny*      (-stridey);
    stencil[11]=ny*px*   (-stridey+stridex);
    stencil[12]=nx*      (-stridex);
    //itself
    stencil[13]=px*      (+stridex);
    stencil[14]=py*nx*   (+stridey-stridex);
    stencil[15]=py*      (+stridey);
    stencil[16]=py*px*   (+stridey+stridex);
    stencil[17]=pz*ny*nx*(+stridez-stridey-stridex);
    stencil[18]=pz*ny*   (+stridez-stridey);
    stencil[19]=pz*ny*px*(+stridez-stridey+stridex);
    stencil[20]=pz*nx*   (+stridez-stridex);
    stencil[21]=pz*      (+stridez);
    stencil[22]=pz*px*   (+stridez+stridex);
    stencil[23]=pz*py*nx*(+stridez+stridey-stridex);
    stencil[24]=pz*py*   (+stridez+stridey);
    stencil[25]=pz*py*px*(+stridez+stridey+stridex);
  }

  //makeClusterFromSeed
  Cluster* makeClusterFromSeed(const Point3i &seed)
  {
    typedef Uint8 Type;

    const Point3d dims = Data.dims.toPoint3().castTo<Point3d>();
    const Point3i idims = dims.castTo<Point3i>();

    auto pixel_to_bounds = Position::computeTransformation(this->Data.bounds,Data.dims);
    
    TRSMatrixDecomposition trs(pixel_to_bounds);

    //get pointer to data at seed location
    Type *data=reinterpret_cast<Type*>(Data.c_ptr());
    Type *seedData=data+seed[2]*(unsigned)(dims[0]*dims[1])+seed[1]*(unsigned)dims[0]+seed[0];
    VisusAssert(seedData<data+(unsigned)(dims[0]*dims[1]*dims[2]));

    //ignore already visited seeds
    if (visited[seedData].flag) 
    { 
      return nullptr; 
    }

    //create initial cluster with seed
    Cluster *firstCluster=new Cluster();
    firstCluster->elements.push_back(seedData);
    clusters.push_back(firstCluster);
    firstCluster->aabbsize=trs.scale.module(); // size of one voxel
    visited[seedData].flag=true;

    //create first centerline node (in physical/scaled space)
    firstCluster->nodepos=GraphUtils::toPoint(seedData,data,idims,trs.scale,trs.translate);

    double diam=std::max((double)min_diam,rayburst.getDiam2d(firstCluster->nodepos,threshold));
    {
      ScopedLock lock(this->centerlines->lock);
      firstCluster->nodeid=this->centerlines->mkVert(Sphere(firstCluster->nodepos,diam/2));

      auto box=this->centerlines->bounds.getBoxNd();
      box.addPoint(firstCluster->nodepos);
      this->centerlines->bounds=Position(this->centerlines->bounds.getTransformation(), box);
    }

    return firstCluster;
  }

  //CALCULATE
  void calculate(Aborted& aborted)
  {
    typedef Uint8 Type;

    auto idims = Data.dims.toPoint3();

    auto pixel_to_bounds = Position::computeTransformation(this->Data.bounds,Data.dims);

    TRSMatrixDecomposition trs(pixel_to_bounds);

    //get pointer to data
    Type *data=reinterpret_cast<Type*>(Data.c_ptr());

    int neighbors[26];
    double min_aabbsize=trs.scale.module(); // size of one voxel

    //while there are still clusters being created
    while (clusters.size()>0)
    {
      if (aborted())
        return;

      Cluster &cluster=*clusters.back(); 
      clusters.pop_back();

      //find cluster elements (direct neighbors of this cluster)
      std::vector<Type*> newelems;
      for (int i=0;i<(int)cluster.elements.size();i++)
      {
        Type *elem=cluster.elements[i];

        createNeighborStencil((int)(elem-data),idims,neighbors);
        for (int j=0;j<26;j++)
        {
          Type *neighbor=elem+neighbors[j];
          if (*neighbor>=threshold && !visited[neighbor].flag)
          {
            newelems.push_back(neighbor);
            visited[neighbor].flag=true;
          }
        }
      }

      //identify connected components using union-find
      UnionFind<Type*> uf;
      for (int i=0;i<(int)newelems.size();i++)
      {
        Type *elem=newelems[i];
        createNeighborStencil((int)(elem-data),idims,neighbors);
        uf.make_set(elem);
        Type *rep=elem;
        for (int j=0;j<26;j++)
        {
          Type *neighbor=uf.find_set_safe(elem+neighbors[j]);
          if (neighbor) rep=uf.link(rep,neighbor);
        }
      }

      //create clusters
      std::map<Type*,Cluster*> newclusters; // map from union-find representative to cluster
      for (int i=0;i<(int)newelems.size();i++)
      {
        Type *elem=newelems[i];
        Type *rep=uf.find_set(elem);
        Cluster* C=nullptr;
        if (newclusters.find(rep)==newclusters.end())
        {
          C=newclusters[rep]=new Cluster;
          C->maxval=0;
        }
        else
          C=newclusters[rep];

        C->elements.push_back(elem);
        C->maxval=std::max(*elem,C->maxval);
      }

      //for each cluster...
      typedef std::map<Type*,Cluster*>::iterator iter_t;
      for (iter_t it=newclusters.begin();it!=newclusters.end();it++)
      {
        Cluster &newcluster=*it->second;
        double inv_max=1.0/(double)newcluster.maxval;

        //compute weighted avg pos and axis-aligned bounding box for cluster
        Point3d MinP( 1E34, 1E34, 1E34);
        Point3d MaxP(-1E34,-1E34,-1E34);
        Point3d wavg;
        double tot_weight=0;
        for (int i=0;i<(int)newcluster.elements.size();i++)
        {
          Type *elem=newcluster.elements[i];
          Point3d p=GraphUtils::toPoint(elem,data,idims,trs.scale,trs.translate);
          MinP[0]=std::min(MinP[0],p[0]);
          MinP[1]=std::min(MinP[1],p[1]);
          MinP[2]=std::min(MinP[2],p[2]);
          MaxP[0]=std::max(MaxP[0],p[0]);
          MaxP[1]=std::max(MaxP[1],p[1]);
          MaxP[2]=std::max(MaxP[2],p[2]);
          //double weight=*elem*inv_max;   //<ctc> ???
          double weight=*elem;
          tot_weight+=weight;
          wavg[0]=wavg[0]+weight*p[0];
          wavg[1]=wavg[1]+weight*p[1];
          wavg[2]=wavg[2]+weight*p[2];
        }
        wavg[0]/=tot_weight;
        wavg[1]/=tot_weight;
        wavg[2]/=tot_weight;
        newcluster.aabbsize=std::max(min_aabbsize,(MaxP-MinP).module());

        //determine pos of node: pt half way between prev and wavg (using eq 1 from paper).
        double exp=cluster.aabbsize/newcluster.aabbsize;
        if (newcluster.aabbsize<=cluster.aabbsize) exp=1.0/exp;
        const Point3d &newpos=newcluster.nodepos=cluster.nodepos+pow(0.5,exp)*(wavg-cluster.nodepos);

        //create next centerline node and connect to prev node (edge weight is euclidian distance)
        double diam=std::max((double)min_diam,rayburst.getDiam2d(newpos,threshold));
        {
          ScopedLock lock(this->centerlines->lock);
          newcluster.nodeid=this->centerlines->mkVert(Sphere(newpos,diam/2));

          auto box=this->centerlines->bounds.getBoxNd();
          box.addPoint(newpos);
          this->centerlines->bounds=Position(this->centerlines->bounds.getTransformation(), box);
          this->centerlines->mkEdge(newcluster.nodeid,cluster.nodeid,(Float32)(newpos-cluster.nodepos).module());
        }

        //"scoop" voxels within a given radius of this node
        double scoopdist=std::max((MinP-newpos).module(),(MaxP-newpos).module());
        std::vector<Type*> moreelems=newcluster.elements;
        do
        {
          newelems=moreelems;
          moreelems.clear();
          for (int i=0;i<(int)newelems.size();i++)
          {
            Type *elem=newelems[i];
            createNeighborStencil((int)(elem-data),idims,neighbors);
            for (int j=0;j<26;j++)
            {
              Type *neighbor=elem+neighbors[j];
              if (*neighbor>=threshold && !visited[neighbor].flag && (newpos-GraphUtils::toPoint(neighbor,data,idims,trs.scale,trs.translate)).module()<=scoopdist)
              {
                moreelems.push_back(neighbor);
                newcluster.elements.push_back(neighbor);
                visited[neighbor].flag=true;
              }
            }
          }
        } while (!moreelems.empty() && !(aborted()));

        //add cluster
        clusters.push_back(&newcluster);
      }

      //done processing this cluster
      delete &cluster;
    }
  }

  /////////////////////////////////////////////
  SharedPtr<TrimGraph> CreateTrimGraph(const CGraph &cg)
  {
    SharedPtr<TrimGraph> tg(new TrimGraph);
    
    //for each rooted subtree of cg...
    for (int i=0;i<(int)cg.verts.size();i++)
    {
      const CGraph::Vertex &v=cg.verts[i];
      if (v.deleted) continue;

      //create TrimGraph of subtree for trimming calculation
      if (!v.out_degree())
      {
        int id=tg->mkVert(TNode(v.data));
        for (int j=0;j<v.in_degree();j++)
        {
          const CGraph::Edge &e(cg.edges[v.in[j]]);
          VisusAssert(!e.deleted);
          const CGraph::Vertex &o=cg.verts[e.src];
          VisusAssert(!o.deleted);
          AddBranch(*tg,id,cg,o,e.data.length);
        }
      }
    }

    return tg;
  }

  /////////////////////////////////////////////
  void CGAddNode(const TrimGraph &tg,CGraph &cg,const TrimGraph::Vertex &N,int prevID=-1,Point3d prevLoc=Point3d())
  {
    //add node...
    int nID=cg.mkVert(N.data.s);
    if (prevID!=-1)
      cg.mkEdge(nID,prevID,(float)(prevLoc-N.data.s.center).module());
    
    //...and all its branches
    for (int i=0;i<N.in_degree();i++)
    {
      const TrimGraph::Edge &E=tg.edges[N.in[i]];
      prevLoc=N.data.s.center;
      prevID=nID;
      for (int k=0;k<(int)E.data.pts.size();k++)
      {
        int pID=cg.mkVert(E.data.pts[k]);
        cg.mkEdge(pID,prevID,(Float32)(prevLoc-E.data.pts[k].center).module());
        prevLoc=E.data.pts[k].center;
        prevID=pID;
      }
      const TrimGraph::Vertex &next=tg.verts[E.src];
      CGAddNode(tg,cg,next,prevID,prevLoc);
    }
  }

  /////////////////////////////////////////////
  SharedPtr<CGraph> CreateCGraph(const TrimGraph &tg)
  {
    auto cg=std::make_shared<CGraph>();
    
    //for each rooted subtree of tg...
    for (int i=0;i<(int)tg.verts.size();i++)
    {
      const TrimGraph::Vertex &v=tg.verts[i];
      if (v.deleted) 
        continue;
      
      if (!v.out_degree())
        CGAddNode(tg,*cg,v);
    }

    return cg;
  }      

  /////////////////////////////////////////////
  void AddBranch(TrimGraph &tg,int parent_id,const CGraph &cg,const CGraph::Vertex &v,float len)
  {
    std::vector<Sphere> pts;
    const CGraph::Vertex *next=nullptr;
    len += LengthToBranchOrEnd(cg,&v,pts,next);
    VisusAssert(next);
    int id=tg.mkVert(next->data);
    tg.mkEdge(id,parent_id,TEdge(len,pts));
    for (int i=0;i<next->in_degree();i++)
    {
      const CGraph::Edge &e(cg.edges[next->in[i]]);
      AddBranch(tg,id,cg,cg.verts[e.src],e.data.length);
    }
  }

  /////////////////////////////////////////////
  void DeleteBranch(TrimGraph &tg,int node_id,bool in=false)
  {
    std::vector<int> verts_to_delete;
    verts_to_delete.push_back(node_id);
    while (!verts_to_delete.empty())
    {
      int id=verts_to_delete.back();
      verts_to_delete.pop_back();
      TrimGraph::Vertex &v=tg.verts[id];
      if (in)
      {
        if (v.out_degree())
        {
          VisusAssert(v.out_degree()==1);
          verts_to_delete.push_back(tg.edges[v.out[0]].dst);
        }
      }
      else
      {
        for (int i=0;i<v.in_degree();i++)
        {
          verts_to_delete.push_back(tg.edges[v.in[i]].src);
        }
      }
      tg.rmVert(id);
    }
  }

  /////////////////////////////////////////////
  float LengthToBranchOrEnd(const CGraph &cg,const CGraph::Vertex *v,std::vector<Sphere> &pts,const CGraph::Vertex *&end)
  {
    VisusAssert(v);
    //find length from v to branch or end point, insert intermediate points into pts, set br to end point
    float len=0.0f;
    while (true)
    {
      int num_out=v->in_degree();
      if (num_out!=1) // v is an end point or branch point
      {
        end=v;
        return len;
      }
      const CGraph::Edge &e(cg.edges[v->in[0]]);
      len+=e.data.length;
      pts.push_back(v->data);
      v=&cg.verts[e.src];
    }

    return len;
  }

  /////////////////////////////////////////////
  float LongestInPathLength(TrimGraph &tg,TrimGraph::Vertex &v)
  {
    if (!v.data.calculated_max_in_length)
    {
      //compute length of in path
      if (v.out_degree()>0)
      {
        VisusAssert(v.out_degree()==1);
        TrimGraph::Edge &e=tg.edges[v.out[0]];
        TrimGraph::Vertex &o=tg.verts[e.dst];
        v.data.max_in_length=e.data.length+LongestInPathLength(tg,o);
      }

      v.data.calculated_max_in_length=true;
    }

    return v.data.max_in_length;
  }

  /////////////////////////////////////////////
  float LongestOutPathLength(TrimGraph &tg,TrimGraph::Vertex &v)
  {
    float max_len=0;
    if (!v.data.calculated_max_out_lengths)
    {
      //compute lengths of all out paths
      for (int k=0;k<v.in_degree();k++)
      {
        TrimGraph::Edge &e=tg.edges[v.in[k]];
        TrimGraph::Vertex &o=tg.verts[e.src];
        float len=e.data.length+LongestOutPathLength(tg,o);
        v.data.max_out_lengths.push_back(len);
        max_len=std::max(max_len,len);
      }

      v.data.calculated_max_out_lengths=true;
    }

    return max_len;
  }

  /////////////////////////////////////////////
  void ComputeLongestPaths(TrimGraph &tg)
  {
    //for each root and leaf of tg...
    for (int i=0;i<(int)tg.verts.size();i++)
    {
      TrimGraph::Vertex &v=tg.verts[i];
      VisusAssert(!v.deleted);

      if (!v.data.calculated_max_in_length && !v.in_degree())     // lonely soldier or leaf
        LongestInPathLength(tg,v);   // side effect calculates longest in path.

      if (!v.data.calculated_max_out_lengths && !v.out_degree())  // root
        LongestOutPathLength(tg,v);  // side effect calculates longest out paths
    }    
  }

  /////////////////////////////////////////////
  void simplify()
  {
    //augmented (TrimGraph) for simplification
    SharedPtr<TrimGraph> tg=CreateTrimGraph(*this->centerlines);
    VisusAssert(tg);

    //calculate max paths for all nodes in tg
    ComputeLongestPaths(*tg);

    std::ofstream cg_dump;
    cg_dump.open("tmp/debug_voxel_scoop/cg.dot");
    genDot2(*this->centerlines,cg_dump);
    cg_dump.close();
    std::ofstream tg_dump;
    tg_dump.open("tmp/debug_voxel_scoop/tg.dot");
    genDot3(*tg,tg_dump);
    tg_dump.close();

    //StringUtils::trim: for all branches of tg, remove out nodes shorter than
    //min_len or w/ len/diam ratio below min_ratio, but keep at least
    //the two longest.
    for (int i=0;i<(int)tg->verts.size();i++)
    {
      const TrimGraph::Vertex &v=tg->verts[i];
      if (v.deleted)
        continue;

      if (!v.out_degree() && !v.in_degree())        // if this is a lonely root...
      {
        //...delete it
        DeleteBranch(*tg,i);
      }        
      else if (!v.out_degree() && v.in_degree()==1) // if this is a root with only one branch...
      {
        //...delete entire subtree if not long enough
        if (v.data.max_out_lengths[0]<this->min_length)
          DeleteBranch(*tg,i);
      }
      else if (v.in_degree()+v.out_degree()<=2)     // continue if not a branching node
      {
        continue;
      }
      else
      {      
        //now we have a branch for which we know the max out path and
        //the length of all in and out paths.
        VisusAssert(v.data.calculated_max_in_length && v.data.calculated_max_out_lengths);

        //find the largest pair of in/out paths (note: if there is no out branch, max_in_len=0 so no worries):
        int max0=v.data.max_in_length>v.data.max_out_lengths[0]?-1:0; // biggest of in_path and first out_path
        int max1=max0==-1?0:-1;                                       // next biggest of those two
        float lmax0=max0==-1?v.data.max_in_length:v.data.max_out_lengths[0];
        float lmax1=max0!=-1?v.data.max_in_length:v.data.max_out_lengths[0];
        for (int k=1;k<v.in_degree();k++)
        {
          if (v.data.max_out_lengths[k]>lmax0)
          {
            max1=max0;
            lmax1=lmax0;
            max0=k;
            lmax0=v.data.max_out_lengths[k];
          }
          else if (v.data.max_out_lengths[k]>lmax1)
          {
            max1=k;
            lmax1=v.data.max_out_lengths[k];
          }
        }

        //remove too short branches or branches with low len/diam ratio
        PrintInfo("Trimming branches of vertex",i,"...");
        for (int k=(v.out_degree()==0)?0:-1;k<v.in_degree();k++) // note: skips -1 if there is no out branch
        {
          float len =k==-1?v.data.max_in_length:v.data.max_out_lengths[k];
          float diam=(float)v.data.s.radius*2;
          if (max0==k||max1==k||
              (len>=this->min_length && len/diam>this->min_ratio))
            continue;
          else if (k==-1)
          {
            VisusAssert(v.out_degree()==1);
            DeleteBranch(*tg,tg->edges[v.out[0]].dst,true/*in*/);
          }
          else
            DeleteBranch(*tg,tg->edges[v.in[k]].src);
        }
      }
    }
    
    //convert from TrimGraph back to CGraph
    auto cg=CreateCGraph(*tg);
    VisusAssert(cg);

    cg_dump.open("tmp/debug_voxel_scoop/cg2.dot");
    genDot2(*cg,cg_dump);
    tg_dump.open("tmp/debug_voxel_scoop/tg2.dot");
    genDot3(*tg,tg_dump);

    this->centerlines=cg;
  }
};

/////////////////////////////////////////////
class VoxelScoopNode::MyJob : public NodeJob
{
public:

  VoxelScoopNode* node;
  Array           data;

  double min_diam;
  double min_length;
  double threshold;
  double min_ratio;
  std::vector<Point3i> seeds;
  bool simplify;

  //constructor
  MyJob(VoxelScoopNode* node_) : node(node_)
  {
    this->data=node->data;
    this->min_diam=node->min_diam;
    this->min_length=node->min_length;
    this->threshold=node->threshold;
    this->min_ratio=node->min_ratio;
    this->seeds=node->seeds;
    this->simplify=node->simplify;
  }

  //runJob
  virtual void runJob() override
  {
    //create builder
    UniquePtr<BuildVoxelScoop> builder(new BuildVoxelScoop(this->data,(float)this->min_diam,(Uint8)this->threshold,(float)this->min_length,(float)this->min_ratio));
    while (builder->NumSeed<(int)this->seeds.size())
    {
      if (this->aborted())
        return;

      while (!builder->makeClusterFromSeed(this->seeds[builder->NumSeed++]) && builder->NumSeed<(int)this->seeds.size())
        return;

      if (this->aborted())
        return;

      builder->calculate(this->aborted);
    }

    //Now trim the centerlines.
     PrintInfo("done calculating.");
      
    //<ctc> TODO: remove this when done.
    auto centerlines_untrimmed=std::make_shared<CGraph>(*builder->getResult());

    if (this->aborted())
      return;

    if (this->simplify) 
    {
      PrintInfo("\tsimplifying...");
       builder->simplify();
    }

    auto centerlines=builder->getResult();

    if (this->aborted() || !centerlines)
      return;

    //publish the results
    //tell that the output has changed (note, if the port is not connected, this is a NOP)
    PrintInfo("publishing centerlines...");

    DataflowMessage msg;
    msg.writeValue("graph", centerlines);
    msg.writeValue("graph_dbg",centerlines_untrimmed);
    msg.writeValue("array",node->data);
    node->publish(msg);
  }
};


////////////////////////////////////////////////////////////
VoxelScoopNode::VoxelScoopNode() 
{
  addInputPort("graph");//use this to get seed points, always Graph<Point3f,float>
  addInputPort("array");
  
  addOutputPort("graph");//centerlines, a Graph<Sphere,float>
  addOutputPort("graph_dbg");//centerlines, a Graph<Sphere,float> (before trimming)
  addOutputPort("array"); //if you do not need the original data, simply do not connect it!
}

/////////////////////////////////////////////////////////////
void VoxelScoopNode::execute(Archive& ar)
{
  if (ar.name == "SetSimplify") {
    bool value;
    ar.read("value", value);
    setSimplify(value);
    return;
  }

  if (ar.name == "SetMinLength") {
    double value;
    ar.read("value", value);
    setMinLength(value);
    return;
  }

  if (ar.name == "SetMinRatio") {
    double value;
    ar.read("value", value);
    setMinRatio(value);
    return;
  }

  if (ar.name == "SetThreshold") {
    double value;
    ar.read("value", value);
    setThreshold(value);
    return;
  }

  if (ar.name == "SetUseMinimaAsSeed") {
    bool value;
    ar.read("value", value);
    setUseMinimaAsSeed(value);
    return;
  }

  if (ar.name == "SetUseMaximaAsSeed") {
    bool value;
    ar.read("value", value);
    setUseMaximaAsSeed(value);
    return;
  }

  if (ar.name == "SetMinDiam") {
    double value;
    ar.read("value", value);
    setMinDiam(value);
    return;
  }

  return Node::execute(ar);


}

////////////////////////////////////////////////////////////
bool VoxelScoopNode::processInput()
{
  abortProcessing();

  auto data=readValue<Array>("array");

  //wrong input
  if (!data || data->dtype!=DTypes::UINT8)
    return false;

  // for uint8 scalar fields only
  this->data=*data;

  SharedPtr<MyJob> myjob(new MyJob(this));

  //getting seeds from Graph<Point3f,float> (they will be in brick space / IJK of data)
  std::vector<Point3i> seeds;
  {
    auto graph=readValue<FGraph>("graph");
    ScopedLock glock(graph->lock);

    for (int I=0;I<(int)graph->verts.size();I++)
    {
      const FGraph::Vertex &v=graph->verts[I];

      if (v.deleted) 
        continue;

      const Point3i p((int)v.data[0],(int)v.data[1],(int)v.data[2]); 
      if (use_minima_as_seed) {if (v.in_degree ()==0)seeds.push_back(p);}
      if (use_maxima_as_seed) {if (v.out_degree()==0)seeds.push_back(p);}
    }
  }

  //set seeds
  this->seeds=seeds;

  addNodeJob(myjob);
  return true;
}


/////////////////////////////////////////////////////////////
void VoxelScoopNode::write(Archive& ar) const
{
  Node::write(ar);

  ar.write("simplify", simplify);
  ar.write("min_length", min_length);
  ar.write("min_ratio", min_ratio);
  ar.write("threshold", threshold);
  ar.write("use_minima_as_seed", use_minima_as_seed);
  ar.write("use_maxima_as_seed", use_maxima_as_seed);
  ar.write("min_diam", min_diam);
}

/////////////////////////////////////////////////////////////
void VoxelScoopNode::read(Archive& ar)
{
  Node::read(ar);

  ar.read("simplify", simplify);
  ar.read("min_length", min_length);
  ar.read("min_ratio", min_ratio);
  ar.read("threshold", threshold);
  ar.read("use_minima_as_seed", use_minima_as_seed);
  ar.read("use_maxima_as_seed", use_maxima_as_seed);
  ar.read("min_diam", min_diam);
}

/////////////////////////////////////////////////////////
class VoxelScoopNodeView :
  public QFrame,
  public View<VoxelScoopNode>
{
public:

  //constructor
  VoxelScoopNodeView(VoxelScoopNode* model = nullptr) {
    if (model)
      bindModel(model);
  }

  //destructor
  virtual ~VoxelScoopNodeView() {
    bindModel(nullptr);
  }

  //bindModel
  virtual void bindModel(VoxelScoopNode* value) override
  {
    if (this->model)
    {
      QUtils::clearQWidget(this);
      widgets = Widgets();
    }

    View<ModelClass>::bindModel(value);

    if (this->model)
    {
      QFormLayout* layout = new QFormLayout();

      layout->addRow("simplify", widgets.simplify = GuiFactory::CreateCheckBox(model->doSimplify(), "", [this](int value) {
        model->setSimplify((bool)value);
      }));

      layout->addRow("min_length", widgets.min_length = GuiFactory::CreateDoubleTextBoxWidget(model->getMinLength(), [this](double value) {
        model->setMinLength(value);
      }));

      layout->addRow("min_ratio", widgets.min_ratio = GuiFactory::CreateDoubleTextBoxWidget(model->getMinRatio(), [this](double value) {
        model->setMinRatio(value);
      }));

      layout->addRow("threshold", widgets.threshold = GuiFactory::CreateDoubleTextBoxWidget(model->getThreshold(), [this](double value) {
        model->setThreshold(value);
      }));

      layout->addRow("use_minima_as_seed", widgets.use_minima_as_seed = GuiFactory::CreateCheckBox(model->useMinimaAsSeed(), "", [this](int value) {
        model->setUseMinimaAsSeed((bool)value);
      }));

      layout->addRow("use_maxima_as_seed", widgets.use_maxima_as_seed = GuiFactory::CreateCheckBox(model->useMaximaAsSeed(), "", [this](int value) {
        model->setUseMaximaAsSeed((bool)value);
      }));

      layout->addRow("min_diam", widgets.min_diam = GuiFactory::CreateDoubleTextBoxWidget(model->getMinDiam(), [this](double value) {
        model->setMinDiam(value);
      }));

      setLayout(layout);
      refreshGui();
    }
  }

private:

  class Widgets
  {
  public:
    QCheckBox* simplify = nullptr;
    QLineEdit* min_length = nullptr;
    QLineEdit* min_ratio = nullptr;
    QLineEdit* threshold = nullptr;
    QCheckBox* use_minima_as_seed = nullptr;
    QCheckBox* use_maxima_as_seed = nullptr;
    QLineEdit* min_diam = nullptr;
  };

  Widgets widgets;

  ///refreshGui
  void refreshGui()
  {
    widgets.simplify->setChecked(model->doSimplify());
    widgets.min_length->setText(cstring(model->getMinLength()).c_str());
    widgets.min_ratio->setText(cstring(model->getMinRatio()).c_str());
    widgets.threshold->setText(cstring(model->getThreshold()).c_str());
    widgets.use_minima_as_seed->setChecked(model->useMinimaAsSeed());
    widgets.use_maxima_as_seed->setChecked(model->useMaximaAsSeed());
    widgets.min_diam->setText(cstring(model->getMinDiam()).c_str());
  }

  //modelChanged
  virtual void modelChanged() override {
    refreshGui();
  }

};

void VoxelScoopNode::createEditor()
{
  auto win = new VoxelScoopNodeView(this);
  win->show();
}


} //namespace Visus

