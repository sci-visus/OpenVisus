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

#ifndef __VISUS_GL_MESH_H
#define __VISUS_GL_MESH_H

#include <Visus/Gui.h>
#include <Visus/GLArrayBuffer.h>
#include <Visus/Frustum.h>

namespace Visus {

///////////////////////////////////////////////////////////
class VISUS_GUI_API GLBatch
{
public:

  VISUS_CLASS(GLBatch)

  SharedPtr<GLArrayBuffer> vertices;
  SharedPtr<GLArrayBuffer> normals;
  SharedPtr<GLArrayBuffer> colors;
  SharedPtr<GLArrayBuffer> texcoords;

  //getNumberOfVertices
  int getNumberOfVertices() const{
    return vertices? vertices->getNumberOfVertices() : 0;
  }
};


///////////////////////////////////////////////////////////
class VISUS_GUI_API GLMesh 
{
public:

  VISUS_CLASS(GLMesh)

  //constructor
  GLMesh();

  //begin
  void begin(int primitive,int vertices_per_batch=0);

  //end
  void end();

  //vertex
  void vertex(double x, double y, double z=0) {
    push(building.vertices,Point3f((float)x, (float)y, (float)z));
  }

  //normal
  void normal(double x, double y, double z) { 
    push(building.normals, Point3f((float)x, (float)y, (float)z)); 
  }

  //color
  void color(double R, double G, double B, double A = 1.0f) { 
    push(building.colors, Point4f((float)(float)R, (float)G, (float)B, (float)A)); 
  }

  //color
  void color(const Color& c) { 
    color(c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha()); 
  }

  //texcoord2
  void texcoord2(double x, double y) { 
    push(building.texcoords2f, Point2f((float)x, (float)y)); 
  }

  //texcoord3
  void texcoord3(double x, double y, double z) { 
    push(building.texcoords3f, Point3f((float)x, (float)y, (float)z)); 
  }

  void vertex(const Point2i& p) { vertex(p[0], p[1], 0); }
  void vertex(const Point2f& p) { vertex(p[0], p[1], 0); }
  void vertex(const Point2d& p) { vertex(p[0], p[1], 0); }
  void vertex(const Point3i& p) { vertex(p[0], p[1], p[2]); }
  void vertex(const Point3f& p) { vertex(p[0], p[1], p[2]); }
  void vertex(const Point3d& p) { vertex(p[0], p[1], p[2]); }
  void vertex(const PointNd& p) { vertex(p.toPoint3()); }

  void normal(const Point2f& p) { normal(p[0], p[1], 0); }
  void normal(const Point2d& p) { normal(p[0], p[1], 0); }
  void normal(const Point3f& p) { normal(p[0], p[1], p[2]); }
  void normal(const Point3d& p) { normal(p[0], p[1], p[2]); }
  void normal(const PointNd& p) { normal(p.toPoint3()); }

  void texcoord2(const Point2f& p) { texcoord2(p[0], p[1]); }
  void texcoord2(const Point2d& p) { texcoord2(p[0], p[1]); }
  void texcoord2(const PointNd& p) { texcoord2(p.toPoint2()); }

  void texcoord3(const Point3f& p) { texcoord3(p[0], p[1], p[2]); }
  void texcoord3(const Point3d& p) { texcoord3(p[0], p[1], p[2]); }
  void texcoord3(const PointNd& p) { texcoord3(p.toPoint3()); }

  //hasColorAttribute
  bool hasColorAttribute() const {
    return !batches.empty() && batches[0].colors;
  }

public:

  //LineLoop
  template <class Point>
  static GLMesh LineLoop(const std::vector<Point>& points) {
    GLMesh ret;
    ret.begin(GL_LINE_LOOP);
    for (auto v:points)
      ret.vertex(v);
    ret.end();
    return ret;
  }

  static GLMesh LineLoop(const std::vector<Point2d>& points) {
    return LineLoop<Point2d>(points);
  }

  static GLMesh LineLoop(const std::vector<Point3d>& points) {
    return LineLoop<Point3d>(points);
  }

public:

  //LineStrip
  template <class Point>
  static GLMesh LineStrip(const std::vector<Point>& points) {
    GLMesh ret;
    ret.begin(GL_LINE_STRIP);
    for (auto v:points)
      ret.vertex(v);
    ret.end();
    return ret;
  }

  static GLMesh LineStrip(const std::vector<Point2d>& points) {
    return LineStrip<Point2d>(points);
  }

  static GLMesh LineStrip(const std::vector<Point3d>& points) {
    return LineStrip<Point3d>(points);
  }

public:

  template <class Point>
  static GLMesh Quad(const Point& p1,const Point& p2,const Point& p3,const Point& p4,bool bNormal=false,bool bTexCoord=false)
  {
    GLMesh ret;
    ret.begin(GL_QUADS);
    if (bTexCoord) ret.texcoord2(0,0); if (bNormal) ret.normal(0,0,1); ret.vertex(p1);
    if (bTexCoord) ret.texcoord2(1,0); if (bNormal) ret.normal(0,0,1); ret.vertex(p2);
    if (bTexCoord) ret.texcoord2(1,1); if (bNormal) ret.normal(0,0,1); ret.vertex(p3);
    if (bTexCoord) ret.texcoord2(0,1); if (bNormal) ret.normal(0,0,1); ret.vertex(p4);
    ret.end();
    return ret;
  }

  static GLMesh Quad(const Point2d& p1, const Point2d& p2, const Point2d& p3, const Point2d& p4, bool bNormal = false, bool bTexCoord = false) {
    return Quad<Point2d>(p1, p2, p3, p4, bNormal, bTexCoord);
  }

  static GLMesh Quad(const Point2d& p1,const Point2d& p2,bool bNormal=false,bool bTexCoord=false) {
    return Quad<Point2d>(Point2d(p1[0],p1[1]), Point2d(p2[0],p1[1]), Point2d(p2[0],p2[1]), Point2d(p1[0],p2[1]),bNormal,bTexCoord);
  }

  static GLMesh Quad(const std::vector<Point2d>& points, bool bNormal = false, bool bTexCoord = false) {
    return Quad<Point2d>(points[0], points[1], points[2], points[3], bNormal, bTexCoord);
  }

public:

  //Polygon
  static GLMesh Polygon(const std::vector<Point2d>& points, bool bNormal = false)
  {
    if (points.size() == 3)
      return Quad(points[0], points[1], points[2], points[2], bNormal);

    if (points.size() == 4)
      return Quad(points[0], points[1], points[2], points[3], bNormal);

    GLMesh ret;
#ifdef GL_POLYGON
    ret.begin(GL_POLYGON);
    for (auto point : points)
    {
      if (bNormal) ret.normal(0, 0, 1);
      ret.vertex(point);
    }
    ret.end();
#endif

    return ret;
  }

public:

  //Points
  template <class Point>
  static GLMesh Points(const std::vector<Point>& points)
  {
    GLMesh ret;
    ret.begin(GL_POINTS);
    for (int I = 0; I < (int)points.size(); I ++)
      ret.vertex(points[I + 0]);
    ret.end();
    return ret;
  }

public:

  //Lines
  template <class Point>
  static GLMesh Lines(const std::vector<Point>& points)
  {
    GLMesh ret;
    ret.begin(GL_LINES);
    for (int I=0;I<(int)points.size();I+=2) 
    {
      ret.vertex(points[I+0]);
      ret.vertex(points[I+1]);
    }
    ret.end();
    return ret;
  }
  
  static GLMesh Lines(const std::vector<Point2d>& points) {
    return Lines<Point2d>(points);
  }

  static GLMesh Lines(const std::vector<Point3d>& points) {
    return Lines<Point3d>(points);
  }

  static GLMesh Lines(const std::vector<PointNd>& points) {
    return Lines<PointNd>(points);
  }

public:

  //WireBox
  static GLMesh WireBox(const Point3d& p1,const Point3d& p2)
  {
    return Lines(std::vector<Point3d>({
      Point3d(p1[0],p1[1],p1[2]), Point3d(p2[0],p1[1],p1[2]),
      Point3d(p1[0],p1[1],p1[2]), Point3d(p1[0],p2[1],p1[2]),
      Point3d(p1[0],p1[1],p1[2]), Point3d(p1[0],p1[1],p2[2]),
      Point3d(p2[0],p1[1],p1[2]), Point3d(p2[0],p2[1],p1[2]),
      Point3d(p2[0],p1[1],p1[2]), Point3d(p2[0],p1[1],p2[2]),
      Point3d(p1[0],p2[1],p1[2]), Point3d(p2[0],p2[1],p1[2]),
      Point3d(p1[0],p2[1],p1[2]), Point3d(p1[0],p2[1],p2[2]),
      Point3d(p1[0],p1[1],p2[2]), Point3d(p2[0],p1[1],p2[2]),
      Point3d(p1[0],p1[1],p2[2]), Point3d(p1[0],p2[1],p2[2]),
      Point3d(p2[0],p2[1],p2[2]), Point3d(p1[0],p2[1],p2[2]),
      Point3d(p2[0],p2[1],p2[2]), Point3d(p2[0],p1[1],p2[2]),
      Point3d(p2[0],p2[1],p2[2]), Point3d(p2[0],p2[1],p1[2])}));
  }

  //WireBox
  static GLMesh WireBox(BoxNd box) {
    box.setPointDim(3);
    return WireBox(box.p1.toPoint3(),box.p2.toPoint3());
  }

public:

  //SolidBox
  static GLMesh SolidBox(const Point3d& p1,const Point3d& p2,bool bNormal=true) {

    float x1=(float)p1[0] , x2=(float)p2[0];
    float y1=(float)p1[1] , y2=(float)p2[1];
    float z1=(float)p1[2] , z2=(float)p2[2];

    GLMesh ret;
    ret.begin(GL_TRIANGLES);
    ret.vertex(x2,y2,z2); ret.vertex(x1,y2,z2); ret.vertex(x1,y1,z2); if (bNormal) {ret.normal( 0, 0, 1); ret.normal( 0, 0, 1); ret.normal( 0, 0, 1);}
    ret.vertex(x1,y1,z2); ret.vertex(x2,y1,z2); ret.vertex(x2,y2,z2); if (bNormal) {ret.normal( 0, 0, 1); ret.normal( 0, 0, 1); ret.normal( 0, 0, 1);}
    ret.vertex(x2,y2,z2); ret.vertex(x2,y1,z2); ret.vertex(x2,y1,z1); if (bNormal) {ret.normal( 1, 0, 0); ret.normal( 1, 0, 0); ret.normal( 1, 0, 0);}
    ret.vertex(x2,y1,z1); ret.vertex(x2,y2,z1); ret.vertex(x2,y2,z2); if (bNormal) {ret.normal( 1, 0, 0); ret.normal( 1, 0, 0); ret.normal( 1, 0, 0);}
    ret.vertex(x2,y2,z2); ret.vertex(x2,y2,z1); ret.vertex(x1,y2,z1); if (bNormal) {ret.normal( 0, 1, 0); ret.normal( 0, 1, 0); ret.normal( 0, 1, 0);}
    ret.vertex(x1,y2,z1); ret.vertex(x1,y2,z2); ret.vertex(x2,y2,z2); if (bNormal) {ret.normal( 0, 1, 0); ret.normal( 0, 1, 0); ret.normal( 0, 1, 0);}
    ret.vertex(x1,y2,z2); ret.vertex(x1,y2,z1); ret.vertex(x1,y1,z1); if (bNormal) {ret.normal(-1, 0, 0); ret.normal(-1, 0, 0); ret.normal(-1, 0, 0);}
    ret.vertex(x1,y1,z1); ret.vertex(x1,y1,z2); ret.vertex(x1,y2,z2); if (bNormal) {ret.normal(-1, 0, 0); ret.normal(-1, 0, 0); ret.normal(-1, 0, 0);}
    ret.vertex(x1,y1,z1); ret.vertex(x2,y1,z1); ret.vertex(x2,y1,z2); if (bNormal) {ret.normal( 0,-1, 0); ret.normal( 0,-1, 0); ret.normal( 0,-1, 0);}
    ret.vertex(x2,y1,z2); ret.vertex(x1,y1,z2); ret.vertex(x1,y1,z1); if (bNormal) {ret.normal( 0,-1, 0); ret.normal( 0,-1, 0); ret.normal( 0,-1, 0);}
    ret.vertex(x2,y1,z1); ret.vertex(x1,y1,z1); ret.vertex(x1,y2,z1); if (bNormal) {ret.normal( 0, 0,-1); ret.normal( 0, 0,-1); ret.normal( 0, 0,-1);}
    ret.vertex(x1,y2,z1); ret.vertex(x2,y2,z1); ret.vertex(x2,y1,z1); if (bNormal) {ret.normal( 0, 0,-1); ret.normal( 0, 0,-1); ret.normal( 0, 0,-1);}
    ret.end();
    return ret;
  }

  //SolidBox
  static GLMesh SolidBox(BoxNd box,bool bNormal=true) {
    box.setPointDim(3);
    return SolidBox(box.p1.toPoint3(),box.p2.toPoint3(),bNormal);
  } 

public:

  //WireCircle
  static GLMesh WireCircle(const int N=32)
  {
    GLMesh ret;
    ret.begin(GL_LINE_LOOP);
    const float angle_delta=(float)Math::Pi/N;
    for (float angle=0;angle<2*Math::Pi;angle+=angle_delta)
      ret.vertex(Point2d(cos(angle),sin(angle)));
    ret.end();
    return ret;
  }

  //SolidCircle
  static GLMesh SolidCircle(const int N=32)
  {
    GLMesh ret;
    ret.begin(GL_TRIANGLES);
    for (int I=0;I<N;I++)
    {
      float a0=(float)((I  )*Math::Pi/N);
      float a1=(float)((I+1)*Math::Pi/N);
      ret.normal(0,0,1); ret.vertex(0,0,0);
      ret.normal(0,0,1); ret.vertex(cos(a0),sin(a0));
      ret.normal(0,0,1); ret.vertex(cos(a1),sin(a1));
    }
    ret.end();
    return ret;
  }

  //SolidSphere
  static GLMesh SolidSphere(const int N=32);

  //ColoredAxis
  static GLMesh ColoredAxis(BoxNd box) {
    box.setPointDim(3);
    auto points = box.getPoints();
    GLMesh ret;
    ret.begin(GL_LINES);
    ret.color(Colors::Red  ); ret.vertex(points[0]);
    ret.color(Colors::Red  ); ret.vertex(points[1]);
    ret.color(Colors::Green); ret.vertex(points[0]);
    ret.color(Colors::Green); ret.vertex(points[3]);
    ret.color(Colors::Blue ); ret.vertex(points[0]);
    ret.color(Colors::Blue ); ret.vertex(points[4]);
    ret.end();
    return ret;
  }

  //ViewDependentUnitVolume
  static GLMesh ViewDependentUnitVolume(const Frustum& frustum,int nslices)
  {
    Point3d viewpos,viewdir,viewup;
    frustum.getModelview().getLookAt(viewpos, viewdir, viewup);

    //render view dependent texture,unproject back screenpoints
    auto unit_box = BoxNd(Point3d(0,0,0), Point3d(1,1,1));

    FrustumMap project(frustum);
    auto screenbox= BoxNd::invalid();
    for (auto p : unit_box.getPoints())
      screenbox.addPoint(project.applyDirectMap(Point4d(p.toPoint3(), 1.0)).dropHomogeneousCoordinate());

    GLMesh ret;
    ret.begin(GL_QUADS);
    for (int I=0;I<nslices;I++) 
    {
      double alpha=Utils::clamp(I/(double)nslices,0.0,1.0);
      alpha=1-alpha;//back to front
      double z=(1-alpha)*screenbox.p1[2]+(alpha)*screenbox.p2[2];
      auto v0=project.applyInverseMap(PointNd(screenbox.p1[0],screenbox.p1[1],z,1.0)).dropHomogeneousCoordinate(); ret.texcoord3(v0); ret.vertex(v0); 
      auto v1=project.applyInverseMap(PointNd(screenbox.p2[0],screenbox.p1[1],z,1.0)).dropHomogeneousCoordinate(); ret.texcoord3(v1); ret.vertex(v1);
      auto v2=project.applyInverseMap(PointNd(screenbox.p2[0],screenbox.p2[1],z,1.0)).dropHomogeneousCoordinate(); ret.texcoord3(v2); ret.vertex(v2);
      auto v3=project.applyInverseMap(PointNd(screenbox.p1[0],screenbox.p2[1],z,1.0)).dropHomogeneousCoordinate(); ret.texcoord3(v3); ret.vertex(v3);
    }
    ret.end();
    return ret;
  }

  //AxisAlignedUnitVolume
  static GLMesh AxisAlignedUnitVolume(int X,int Y,int Z,Point3d viewdir,int nslices)
  {
    GLMesh ret;
    ret.begin(GL_QUADS);
    Point3d p1,p2,p3,p4;
    for (int I=0;I<nslices;I++) 
    {
      double z=Utils::clamp(I/(double)nslices,0.0,1.0);
      if (viewdir[Z]<0) z=1-z; //back to front
      p1[X]=0.0; p1[Y]=0.0; p1[Z]=z; ret.texcoord3(p1); ret.vertex(p1);
      p2[X]=1.0; p2[Y]=0.0; p2[Z]=z; ret.texcoord3(p2); ret.vertex(p2);
      p3[X]=1.0; p3[Y]=1.0; p3[Z]=z; ret.texcoord3(p3); ret.vertex(p3);
      p4[X]=0.0; p4[Y]=1.0; p4[Z]=z; ret.texcoord3(p4); ret.vertex(p4);
    }
    ret.end();
    return ret;
  }

  //AxisAlignedUnitVolume
  static GLMesh AxisAlignedUnitVolume(const Frustum& frustum,int nslices)
  {
    Point3d viewpos,viewdir,viewup;
    frustum.getModelview().getLookAt(viewpos, viewdir,viewup);

    //need to go back to front (i.e. the opposite of viewdir)
    viewdir = -viewdir;

    //decide how to render (along X Y or Z)
    int Z=0;
    if (fabs(viewdir[1])>fabs(viewdir[Z])) Z=1;
    if (fabs(viewdir[2])>fabs(viewdir[Z])) Z=2;
    int X=(Z+1)%3;
    int Y=(Z+2)%3;
    if (X>Y) std::swap(X,Y);

    return AxisAlignedUnitVolume(X,Y,Z,viewdir,nslices);
  }

private:

  friend class GLCanvas;

  int                  primitive=-1;
  std::vector<GLBatch> batches;

  struct
  {
    int                  vertices_per_batch=0;
    std::vector<Point3f> vertices;
    std::vector<Point3f> normals;
    std::vector<Point4f> colors;
    std::vector<Point2f> texcoords2f;
    std::vector<Point3f> texcoords3f;
  }
  building;

  //push
  template <typename T> 
  void push(std::vector<T>& v,const T& value)
  {
    if (building.vertices_per_batch>0 && (int)building.vertices.size()==building.vertices_per_batch) 
      flush();

    if (v.capacity()==v.size()) 
      v.reserve(std::max(32,(int)v.size()<<1));

    v.push_back(value);
  }

  //ConvertQuadsToTriangles
  template <typename PointType>
  static void ConvertQuadsToTriangles(std::vector<PointType>& src)
  {
    if (src.empty()) return;
    std::vector<PointType> tmp;
    int nvertices=(int)src.size();
    int nquads=nvertices/4; VisusAssert(nquads*4==nvertices);
    int ntriangles=2*nquads;
    int n2vertices=ntriangles*3;  //this n2vertices is different from nvertices that the for loop iterates over.
    tmp.reserve(n2vertices);
    for (int I=0;I<nvertices;I+=4)
    {
      const PointType& p0=src[I+0];
      const PointType& p1=src[I+1];
      const PointType& p2=src[I+2];
      const PointType& p3=src[I+3];
      tmp.push_back(p0); tmp.push_back(p1); tmp.push_back(p2);
      tmp.push_back(p0); tmp.push_back(p2); tmp.push_back(p3);
    }
    src=tmp;
  }

  //flush
  void flush();

};


} //namespace

#endif //__VISUS_GL_MESH_H