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
  inline void vertex(float x,float y,float z=0) {push(building.vertices,Point3f(x,y,z));}
  inline void vertex(const Point2i& p) {vertex((float)p.x,(float)p.y,(float)  0);}
  inline void vertex(const Point2f& p) {vertex((float)p.x,(float)p.y,(float)  0);}
  inline void vertex(const Point2d& p) {vertex((float)p.x,(float)p.y,(float)  0);}
  inline void vertex(const Point3i& p) {vertex((float)p.x,(float)p.y,(float)p.z);}
  inline void vertex(const Point3f& p) {vertex((float)p.x,(float)p.y,(float)p.z);}
  inline void vertex(const Point3d& p) {vertex((float)p.x,(float)p.y,(float)p.z);}
  template <typename T>
  inline void vertex(T x,T y,T z=0)    {vertex((float)  x,(float)  y,(float)  z);}

  //normal
  inline void normal(float x,float y,float z) {push(building.normals,Point3f(x,y,z));}
  inline void normal(const Point2i& p) {normal((float)p.x,(float)p.y,(float)  0);}
  inline void normal(const Point2f& p) {normal((float)p.x,(float)p.y,(float)  0);}
  inline void normal(const Point2d& p) {normal((float)p.x,(float)p.y,(float)  0);}
  inline void normal(const Point3i& p) {normal((float)p.x,(float)p.y,(float)p.z);}
  inline void normal(const Point3f& p) {normal((float)p.x,(float)p.y,(float)p.z);}
  inline void normal(const Point3d& p) {normal((float)p.x,(float)p.y,(float)p.z);}
  template <typename T>
  inline void normal(T x,T y,T z=0)    {normal((float)  x,(float)  y,(float)  z);}

  //color
  inline void color(float R,float G,float B,float A=1.0f) {push(building.colors,Point4f(R,G,B,A));}
  inline void color(const Color& c) {color(c.getRed(),c.getGreen(),c.getBlue(),c.getAlpha());}

  //texcoord2
  inline void texcoord2(float x,float y)  {push(building.texcoords2f,Point2f(x,y));}
  inline void texcoord2(const Point2i& p) {texcoord2((float)p.x,(float)p.y);}
  inline void texcoord2(const Point2f& p) {texcoord2((float)p.x,(float)p.y);}
  inline void texcoord2(const Point2d& p) {texcoord2((float)p.x,(float)p.y);}

  //texcoord3
  inline void texcoord3(float x,float y,float z) {push(building.texcoords3f,Point3f(x,y,z));}
  inline void texcoord3(const Point3i& p) {texcoord3((float)p.x,(float)p.y,(float)p.z);}
  inline void texcoord3(const Point3f& p) {texcoord3((float)p.x,(float)p.y,(float)p.z);}
  inline void texcoord3(const Point3d& p) {texcoord3((float)p.x,(float)p.y,(float)p.z);}

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

  //Quad
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

  //Quad
  template <typename Point>
  static GLMesh Quad(const Point& p1,const Point& p2,bool bNormal=false,bool bTexCoord=false)
  {return Quad(Point(p1.x,p1.y),Point(p2.x,p1.y),Point(p2.x,p2.y),Point(p1.x,p2.y),bNormal,bTexCoord);}

  //Quad
  template <class Point>
  static GLMesh Quad(const std::vector<Point>& points,bool bNormal=false,bool bTexCoord=false)
  {VisusAssert(points.size()==4);return Quad<Point>(points[0],points[1],points[2],points[3],bNormal,bTexCoord);}

  //Lines
  template <class Point>
  static GLMesh Lines(std::vector<Point> points) 
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

  //WireBox
  template <typename Point>
  static GLMesh WireBox(const Point& p1,const Point& p2)
  {
    return Lines(std::vector<Point>({
      Point(p1.x,p1.y,p1.z), Point(p2.x,p1.y,p1.z),
      Point(p1.x,p1.y,p1.z), Point(p1.x,p2.y,p1.z),
      Point(p1.x,p1.y,p1.z), Point(p1.x,p1.y,p2.z),
      Point(p2.x,p1.y,p1.z), Point(p2.x,p2.y,p1.z), 
      Point(p2.x,p1.y,p1.z), Point(p2.x,p1.y,p2.z),
      Point(p1.x,p2.y,p1.z), Point(p2.x,p2.y,p1.z),
      Point(p1.x,p2.y,p1.z), Point(p1.x,p2.y,p2.z),
      Point(p1.x,p1.y,p2.z), Point(p2.x,p1.y,p2.z),
      Point(p1.x,p1.y,p2.z), Point(p1.x,p2.y,p2.z),
      Point(p2.x,p2.y,p2.z), Point(p1.x,p2.y,p2.z),
      Point(p2.x,p2.y,p2.z), Point(p2.x,p1.y,p2.z),
      Point(p2.x,p2.y,p2.z), Point(p2.x,p2.y,p1.z)}));
  }

  //WireBox
  static GLMesh WireBox(const Box3d& box) {
    return WireBox(box.p1,box.p2);
  }

  //SolidBox
  template <typename Point>
  static GLMesh SolidBox(const Point& p1,const Point& p2,bool bNormal=true) {

    float x1=(float)p1.x , x2=(float)p2.x;
    float y1=(float)p1.y , y2=(float)p2.y;
    float z1=(float)p1.z , z2=(float)p2.z;

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
  static GLMesh SolidBox(const Box3d& box,bool bNormal=true) {
    return SolidBox(box.p1,box.p2,bNormal);
  } 

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
  static GLMesh ColoredAxis(const Box3d& box) {
    GLMesh ret;
    ret.begin(GL_LINES);
    ret.color(Colors::Red  ); ret.vertex(box.getPoint(0));
    ret.color(Colors::Red  ); ret.vertex(box.getPoint(1));
    ret.color(Colors::Green); ret.vertex(box.getPoint(0));
    ret.color(Colors::Green); ret.vertex(box.getPoint(3));
    ret.color(Colors::Blue ); ret.vertex(box.getPoint(0));
    ret.color(Colors::Blue ); ret.vertex(box.getPoint(4));
    ret.end();
    return ret;
  }

  //ViewDependentUnitVolume
  static GLMesh ViewDependentUnitVolume(const Frustum& frustum,int nslices)
  {
    Point3d viewpos,viewdir,viewup;
    frustum.getModelview().getLookAt(viewpos,viewdir,viewup);
    viewdir=viewdir*(-1); //need to go back to front (i.e. the opposite of viewdir)

    //render view dependent texture,unproject back screenpoints
    const Box3d unit_box(Point3d(), Point3d(1,1,1));

    FrustumMap project(frustum);
    Box3d screenbox= Box3d::invalid();
    for(int I=0;I<8;++I)
    {
      Point3d p=project.applyDirectMap(Point4d(unit_box.getPoint(I),1.0)).dropHomogeneousCoordinate();
      screenbox.addPoint(p);
    }

    GLMesh ret;
    ret.begin(GL_QUADS);
    for (int I=0;I<nslices;I++) 
    {
      double alpha=Utils::clamp(I/(double)nslices,0.0,1.0);
      alpha=1-alpha;//back to front
      double z=(1-alpha)*screenbox.p1.z+(alpha)*screenbox.p2.z;
      Point3d v0=project.applyInverseMap(Point4d(screenbox.p1.x,screenbox.p1.y,z,1.0)).dropHomogeneousCoordinate(); ret.texcoord3(v0); ret.vertex(v0); 
      Point3d v1=project.applyInverseMap(Point4d(screenbox.p2.x,screenbox.p1.y,z,1.0)).dropHomogeneousCoordinate(); ret.texcoord3(v1); ret.vertex(v1); 
      Point3d v2=project.applyInverseMap(Point4d(screenbox.p2.x,screenbox.p2.y,z,1.0)).dropHomogeneousCoordinate(); ret.texcoord3(v2); ret.vertex(v2); 
      Point3d v3=project.applyInverseMap(Point4d(screenbox.p1.x,screenbox.p2.y,z,1.0)).dropHomogeneousCoordinate(); ret.texcoord3(v3); ret.vertex(v3); 
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
    frustum.getModelview().getLookAt(viewpos,viewdir,viewup);
    viewdir=viewdir*(-1); //need to go back to front (i.e. the opposite of viewdir)

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