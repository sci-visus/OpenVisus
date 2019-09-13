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

#include <Visus/IsoContourNode.h>
#include <Visus/Dataflow.h>
#include <Visus/IsoContourTables.h>

#if __GNUC__ && !__APPLE__
#pragma GCC diagnostic ignored "-Wnarrowing"
#endif

//see http://www.icare3d.org/blog_techno/gpu/opengl_geometry_shader_marching_cubes.html

namespace Visus {

///////////////////////////////////////////////////////////////////////////////////////
class MarchingCube
{
public:

  Array   data;
  double  isovalue;
  int     vertices_per_batch= 16 * 1024;
  Aborted aborted;

  SharedPtr< IsoContour> isocontour;

  //constructor
  MarchingCube() {
  }

  //setData
  void setData(Array data)
  {
    this->data = data;

    this->isocontour = std::make_shared<IsoContour>();

    this->isocontour->field.array = data; //pass through
    this->isocontour->field.texture = std::make_shared<GLTexture>(data);

    //IsoContourenderNode NEEDS the vertices in this range (for computing normals on GPU)
    auto pdim = data.dims.getPointDim();

    if (!data.bounds.valid())
      isocontour->bounds = BoxNd(PointNd(0, 0, 0), PointNd(1, 1, 1));

    auto pixel_to_bounds = Position::computeTransformation(data.bounds, data.dims);
    isocontour->bounds = Position(pixel_to_bounds, BoxNi(PointNi(pdim), data.dims));
  }

  //setIsoValue
  void setIsoValue(double value) {
    this->isovalue = value;
  }

  //enableVoxelUsed
  void enableVoxelUsed() {
    this->isocontour->voxel_used = Array(this->data.dims, this->data.dtype);
    this->isocontour->voxel_used.fillWithValue(0);
  }

  //disableVoxelUsed
  void disableVoxelUsed() {
    this->isocontour->voxel_used = Array();
  }

  //execute
  template <class CppType>
  bool execute() 
  {
    auto t1 = Time::now();

    //std::ofstream out("test.obj");

    IsoContour& isocontour = *this->isocontour;

    int ntriangles = 0;
    isocontour.data_range = Range::invalid();

    // see http://local.wasp.uwa.edu.au/~pbourke/geometry/polygonise/
    isocontour.begin(GL_TRIANGLES,vertices_per_batch);

    Point3i dims(data.getWidth(),data.getHeight(),data.getDepth());

    const CppType* field=data.c_ptr<CppType*>();

    //no data set
    if (!(dims[0]*dims[1]*dims[2]) || !field)
      return false;

    CppType* voxel_used = isocontour.voxel_used ? isocontour.voxel_used.c_ptr<CppType*>() : NULL;

    const int stridex=data.dtype.getByteSize()/sizeof(CppType);
    const int stridey=stridex*dims[0];
    const int stridez=stridey*dims[1];

    const int cube_strides[8]=
    {
      0,                         //(x  ,y  ,z  )
      stridex,                   //(x+1,y  ,z  )
      stridex+stridey,           //(x+1,y+1,z  )
      stridey,                   //(x  ,y+1,z  )
      stridez,                   //(x  ,y  ,z+1)
      stridex+stridez,           //(x+1,y  ,z+1)
      stridex+stridey+stridez,   //(x_1,y+1,z+1)
      stridey+stridez            //(x  ,y+1,z+1)
    };

    double v[12][3]; //vertices

    for (int z=0;(z<dims[2]-1);z++) {  
    for (int y=0;(y<dims[1]-1);y++) {
    for (int x=0;(x<dims[0]-1);x++) 
    {
      //stop signal!
      if (aborted())
        return false;

      int index=x*stridex+y*stridey+z*stridez;

      isocontour.data_range.from=std::min(isocontour.data_range.from,(double)field[index]);
      isocontour.data_range.to  =std::max(isocontour.data_range.to  ,(double)field[index]);

      //field values
      double values[8]=
      {
        (double)field[index+cube_strides[0]], 
        (double)field[index+cube_strides[1]], 
        (double)field[index+cube_strides[2]], 
        (double)field[index+cube_strides[3]],
        (double)field[index+cube_strides[4]] , 
        (double)field[index+cube_strides[5]] , 
        (double)field[index+cube_strides[6]], 
        (double)field[index+cube_strides[7]]
      };

      int L =
         (values[0]<isovalue?  1:0)
        |(values[1]<isovalue?  2:0)
        |(values[2]<isovalue?  4:0)
        |(values[3]<isovalue?  8:0)
        |(values[4]<isovalue? 16:0)
        |(values[5]<isovalue? 32:0)
        |(values[6]<isovalue? 64:0)
        |(values[7]<isovalue?128:0);

      int et=edge_table[L];

      if (!et) 
        continue;

      if (voxel_used)
        voxel_used[index] = 1;

      double cube_points[8][3]=
      {
        {(double)x  ,(double)y  ,(double)z  }, 
        {(double)x+1,(double)y  ,(double)z  }, 
        {(double)x+1,(double)y+1,(double)z  }, 
        {(double)x  ,(double)y+1,(double)z  }, 
        {(double)x  ,(double)y  ,(double)z+1}, 
        {(double)x+1,(double)y  ,(double)z+1}, 
        {(double)x+1,(double)y+1,(double)z+1}, 
        {(double)x  ,(double)y+1,(double)z+1}
      };

      #define INTERPOLATE(d,a,b) \
      { \
        const double alpha=((values[a]==values[b])?(0.5):(isovalue-values[a])/(values[b]-values[a]));\
        v[d][0]=cube_points[a][0] + alpha*(cube_points[b][0]-cube_points[a][0]);\
        v[d][1]=cube_points[a][1] + alpha*(cube_points[b][1]-cube_points[a][1]);\
        v[d][2]=cube_points[a][2] + alpha*(cube_points[b][2]-cube_points[a][2]);\
      }\
      /* -- */

      if (et &    1) INTERPOLATE( 0,0,1);
      if (et &    2) INTERPOLATE( 1,1,2);
      if (et &    4) INTERPOLATE( 2,2,3);
      if (et &    8) INTERPOLATE( 3,3,0);
      if (et &   16) INTERPOLATE( 4,4,5);
      if (et &   32) INTERPOLATE( 5,5,6);
      if (et &   64) INTERPOLATE( 6,6,7);
      if (et &  128) INTERPOLATE( 7,7,4);
      if (et &  256) INTERPOLATE( 8,0,4);
      if (et &  512) INTERPOLATE( 9,1,5);
      if (et & 1024) INTERPOLATE(10,2,6);
      if (et & 2048) INTERPOLATE(11,3,7);

      for (int i=0;triangle_table[L][i]!=-1;i+=3) 
      {
        const double* p0=v[triangle_table[L][i  ]]; isocontour.vertex(float(p0[0]),float(p0[1]),float(p0[2]));
        const double* p1=v[triangle_table[L][i+1]]; isocontour.vertex(float(p1[0]),float(p1[1]),float(p1[2]));
        const double* p2=v[triangle_table[L][i+2]]; isocontour.vertex(float(p2[0]),float(p2[1]),float(p2[2]));

        //out << "v " << p0[0] << " " << p0[1] << " " << p0[2] << std::endl;
        //out << "v " << p1[0] << " " << p1[1] << " " << p1[2] << std::endl;
        //out << "v " << p2[0] << " " << p2[1] << " " << p2[2] << std::endl;
        //out << "f " << (ntriangles * 3 + 1) << " " << (ntriangles * 3 + 2) << " " << (ntriangles * 3 + 3)<<std::endl;
        ++ntriangles;
      }
    }}}

    isocontour.end();

    //out.close();
     
    VisusInfo() << "Marching cube on data("<<data.dims.toString()<<") ntriangles("<< ntriangles<<") done in " << t1.elapsedMsec() << "msec";
    return true;
  }
};

///////////////////////////////////////////////////////////////////////
class IsoContourNode::MyJob : public NodeJob
{
public:

  Node*          node;
  MarchingCube   mc;

  //constructor
  MyJob(Node* node, double isovalue, Array data)  
  {
    this->node = node;

    mc.setData(data);
    mc.setIsoValue(isovalue);
    mc.aborted = this->aborted;

    if (node->isOutputConnected("cell_array"))
      mc.enableVoxelUsed();

  }

  //destructor
  virtual ~MyJob() {
  }

  //runJob (i will work only on the first component, leaving untouched the others)
  virtual void runJob() override
  {
    if (!ExecuteOnCppSamples(this->mc,this->mc.data.dtype))
      return;

    auto isocontour = mc.isocontour;

    //tell that the output has changed, if the port is not connected, this is a NOP!
    DataflowMessage msg;
    msg.writeValue("data", isocontour);
    node->publish(msg);
  }

};


///////////////////////////////////////////////////////////////////////
IsoContourNode::IsoContourNode(String name) 
  : Node(name),isovalue(0)
{
  addInputPort("data");
  addOutputPort("data");
  addOutputPort("cell_array");
}

///////////////////////////////////////////////////////////////////////
IsoContourNode::~IsoContourNode()
{}

///////////////////////////////////////////////////////////////////////
void IsoContourNode::messageHasBeenPublished(DataflowMessage msg)
{
  auto isocontour= msg.readValue<IsoContour>("data");
  
  //avoid rehentrant code
  if (isocontour && this->data_range!= isocontour->data_range)
  {
    beginUpdate();
    this->data_range= isocontour->data_range;
    endUpdate();
  }
}

///////////////////////////////////////////////////////////////////////
bool IsoContourNode::processInput()
{
  abortProcessing();

  auto data = readValue<Array>("data");
  if (!data || !data->dtype.valid())
    return false;
  
  addNodeJob(std::make_shared<MyJob>(this,isovalue,*data));
  return true;
}


///////////////////////////////////////////////////////////////////////
void IsoContourNode::writeToObjectStream(ObjectStream& ostream)
{
  Node::writeToObjectStream(ostream);

  ostream.write("isovalue",cstring(isovalue));
}

///////////////////////////////////////////////////////////////////////
void IsoContourNode::readFromObjectStream(ObjectStream& istream) 
{
  Node::readFromObjectStream(istream);

  isovalue=cdouble(istream.read("isovalue"));
}



} //namespace Visus







