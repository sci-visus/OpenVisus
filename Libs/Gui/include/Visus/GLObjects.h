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

#ifndef __VISUS_GL_OBJECTS_H
#define __VISUS_GL_OBJECTS_H

#include <Visus/Gui.h>
#include <Visus/GLObject.h>
#include <Visus/GLCanvas.h>
#include <Visus/GLPhongShader.h>

namespace Visus {

/////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLModelview : public GLObject
{
public:

  VISUS_NON_COPYABLE_CLASS(GLModelview)

  Matrix T;

  //constructor
  GLModelview(Matrix T_=Matrix::identity(4)) : T(T_) {
    T.setSpaceDim(4);
  }

  //destructor
  virtual ~GLModelview() {
  }

  //glRender
  virtual void glRender(GLCanvas& gl) override {
    gl.multModelview(T);
  }

};

/////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLPhongObject : public GLObject 
{
public:

  VISUS_NON_COPYABLE_CLASS(GLPhongObject)

  int                  line_width = 1;
  int                  point_size = 1;
  Color                color=Colors::White;
  SharedPtr<GLTexture> texture;
  GLMesh               mesh;

  //default constructor
  GLPhongObject() {
  }

  //constructor
  GLPhongObject(const Color& color_,int line_width_=1,SharedPtr<GLTexture> texture_=SharedPtr<GLTexture>()) 
    : color(color_),line_width(line_width_),texture(texture_) {
  }

  //constructor
  GLPhongObject(const GLMesh& mesh_,const Color& color_,int line_width_=1,SharedPtr<GLTexture> texture_=SharedPtr<GLTexture>()) 
    : GLPhongObject(color_,line_width_,texture_) {
    this->mesh=mesh_;
  }

  //destructor
  virtual ~GLPhongObject() {
  }

  //glRender  
  virtual void glRender(GLCanvas& gl) override
  {
    bool bHasColors=mesh.hasColorAttribute();

    if (!color.getAlpha() && !texture && !bHasColors)
      return;

    GLPhongShader* shader=GLPhongShader::getSingleton(GLPhongShader::Config()
      .withTextureEnabled(texture?true:false)
      .withColorAttributeEnabled(bHasColors));

    gl.setShader(shader);

    shader->setUniformColor(gl,color);

    if (texture)
      shader->setTexture(gl,texture);

    if (line_width)
      gl.pushLineWidth(line_width);

    if (point_size)
      gl.pushPointSize(point_size);

    //render the mesh
    gl.glRenderMesh(mesh);
    
    if (point_size)
      gl.popPointSize();

    if (line_width)
      gl.popLineWidth();
  }

};

/////////////////////////////////////////////////////////////////////////
#if !SWIG


/////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLStruct : public GLObject
{
public:

  VISUS_NON_COPYABLE_CLASS(GLStruct)

  std::vector< SharedPtr<GLObject> > v;

  //constructor
  GLStruct(const std::vector< SharedPtr<GLObject> >& v_ = std::vector< SharedPtr<GLObject> >()) : v(v_) {
  }

  //destructor
  virtual ~GLStruct() {
  }

  //push_back
  void push_back(SharedPtr<GLObject> value) {
    v.push_back(value);
  }

  //glRender
  virtual void glRender(GLCanvas& gl) override
  {
    gl.pushModelview();

    for (auto it : v) {
      if (it)
        it->glRender(gl);
    }
    gl.popModelview();
  }

};


class VISUS_GUI_API GLPoint : public GLPhongObject
{
public:

  VISUS_NON_COPYABLE_CLASS(GLPoint)

  //constructor
  GLPoint(const Point2d& p_, const Color& color_, int point_size_ = 1)
  : GLPhongObject(GLMesh::Points(std::vector<Point2d>({ p_ })), color_, 1) {
    this->point_size = point_size_;
  }

  //constructor
  GLPoint(const Point3d& p_, const Color& color_, int point_size_ = 1)
    : GLPhongObject(GLMesh::Points(std::vector<Point3d>({ p_ })), color_, 1) {
    this->point_size = point_size_;
  }

  //constructor
  GLPoint(const PointNd& p_, const Color& color_, int point_size_ = 1)
    : GLPhongObject(GLMesh::Points(std::vector<PointNd>({ p_})), color_, 1) {
    this->point_size = point_size_;
  }

  //destructor
  virtual ~GLPoint() {
  }

};


class VISUS_GUI_API GLLine : public GLPhongObject
{
public:

  VISUS_NON_COPYABLE_CLASS(GLLine)

  //constructor
  GLLine(const Point2d& p1_,const Point2d& p2_,const Color& color_,int line_width_=1)
    : GLPhongObject(GLMesh::Lines(std::vector<Point2d>({p1_,p2_})),color_,line_width_) {}

  //constructor
  GLLine(const Point3d& p1_, const Point3d& p2_, const Color& color_, int line_width_ = 1)
    : GLPhongObject(GLMesh::Lines(std::vector<Point3d>({ p1_,p2_ })), color_, line_width_) {}

  //constructor
  GLLine(const PointNd& p1_, const PointNd& p2_, const Color& color_, int line_width_ = 1)
    : GLPhongObject(GLMesh::Lines({ p1_,p2_ }), color_, line_width_) {}

  //destructor
  virtual ~GLLine() {
  }

};

/////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLLines : public GLPhongObject
{
public:

  VISUS_NON_COPYABLE_CLASS(GLLines)

  //constructor
  GLLines(const std::vector<Point2d>& points_,const Color& color_,int line_width_=1) 
    : GLPhongObject(GLMesh::Lines(points_),color_,line_width_) {}

  //constructor
  GLLines(const std::vector<Point3d>& points_, const Color& color_, int line_width_ = 1)
    : GLPhongObject(GLMesh::Lines(points_), color_, line_width_) {}

  //constructor
  GLLines(const std::vector<PointNd>& points_, const Color& color_, int line_width_ = 1)
    : GLPhongObject(GLMesh::Lines(points_), color_, line_width_) {}

  //destructor
  virtual ~GLLines() {
  }

};

/////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLLineStrip : public GLPhongObject
{
public:

  VISUS_NON_COPYABLE_CLASS(GLLineStrip)

  //constructor
  GLLineStrip(const std::vector<Point2d>& vertices_,const Color& color_,int line_width_=1) 
    : GLPhongObject(GLMesh::LineStrip(vertices_),color_,line_width_) {}

  //constructor
  GLLineStrip(const std::vector<Point3d>& vertices_, const Color& color_, int line_width_ = 1)
    : GLPhongObject(GLMesh::LineStrip(vertices_), color_, line_width_) {}


  //constructor
  GLLineStrip(const std::vector<PointNd>& vertices_, const Color& color_, int line_width_ = 1)
    : GLPhongObject(GLMesh::LineStrip(vertices_), color_, line_width_) {}

  //constructor
  GLLineStrip(const std::vector<Point2i>& vertices_, const Color& color_, int line_width_ = 1)
    : GLPhongObject(GLMesh::LineStrip(vertices_), color_, line_width_) {}


  //destructor
  virtual ~GLLineStrip() {
  }

};

/////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLCubicBezier : public GLLineStrip
{
public:

  VISUS_NON_COPYABLE_CLASS(GLCubicBezier)

  //constructor
  GLCubicBezier(Point2d A, Point2d B, Point2d C, Point2d D,Color color=Colors::Black,int line_width=1,const int nsegments=32)
    : GLLineStrip(bezier(A,B,C,D,nsegments),color,line_width) {}

  //destructor
  virtual ~GLCubicBezier() {
  }

private:

  static std::vector<Point2d> bezier(Point2d A, Point2d B, Point2d C, Point2d D,int nsegments) {
    std::vector<Point2d> ret;
    ret.reserve(nsegments);
    for (int I=0;I<nsegments;I++)
    {
      double t=I/(double)(nsegments-1);
      double s=1-t;
      ret.push_back(((A*s + B*t)*s + (B*s + C*t)*t)*s + ((B*s + C*t)*s + (C*s + D*t)*t)*t);
    }  
    return ret;
  }

};

/////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLLineLoop : public GLPhongObject
{
public:

  VISUS_NON_COPYABLE_CLASS(GLLineLoop)

  //constructor
  GLLineLoop(const std::vector<Point2d>& vertices,const Color& color,int line_width=1) 
    : GLPhongObject(GLMesh::LineLoop(vertices),color,line_width) {}

  //constructor
  GLLineLoop(const std::vector<Point3d>& vertices, const Color& color, int line_width = 1)
    : GLPhongObject(GLMesh::LineLoop(vertices), color, line_width) {}

  //constructor
  GLLineLoop(const std::vector<PointNd>& vertices, const Color& color, int line_width = 1)
    : GLPhongObject(GLMesh::LineLoop(vertices), color, line_width) {}

  //destructor
  virtual ~GLLineLoop() {
  }

};

///////////////////////////////////////////////////////////
class VISUS_GUI_API GLQuad : public GLStruct
{
public:

  VISUS_NON_COPYABLE_CLASS(GLQuad)

  //constructor
  GLQuad(const Point2d& p1, const Point2d& p2, const Point2d& p3, const Point2d& p4, const Color& fill_color, const Color& line_color, int line_width = 1, SharedPtr<GLTexture> texture = SharedPtr<GLTexture>()) {
    push_back(std::make_shared<GLPhongObject>(GLMesh::Quad(p1, p2, p3, p4, false, texture ? true : false), fill_color, 0, texture));
    push_back(std::make_shared<GLPhongObject>(GLMesh::LineLoop(std::vector<Point2d>({ p1,p2,p3,p4 })), line_color, line_width));
  }

  //constructor
  GLQuad(Point2d p1, Point2d p2, const Color& fill_color, const Color& line_color, int line_width = 1, SharedPtr<GLTexture> texture = SharedPtr<GLTexture>())
    : GLQuad(Point2d(p1[0], p1[1]), Point2d(p2[0], p1[1]), Point2d(p2[0], p2[1]), Point2d(p1[0], p2[1]), fill_color, line_color, line_width, texture) {}

  //constructor
  GLQuad(const std::vector<Point2d>& points, const Color& fill_color, const Color& line_color, int line_width = 1, SharedPtr<GLTexture> texture = SharedPtr<GLTexture>())
    : GLQuad(points[0], points[1], points[2], points[3], fill_color, line_color, line_width, texture) {}

  //constructor
  GLQuad(const std::array<Point2d,4>& points, const Color& fill_color, const Color& line_color, int line_width = 1, SharedPtr<GLTexture> texture = SharedPtr<GLTexture>())
    : GLQuad(points[0], points[1], points[2], points[3], fill_color, line_color, line_width, texture) {}


  //destructor
  virtual ~GLQuad() {
  }
};


///////////////////////////////////////////////////////////
class VISUS_GUI_API GLPolygon : public GLStruct
{
public:

  VISUS_NON_COPYABLE_CLASS(GLPolygon)

  //constructor
  GLPolygon(const std::vector<Point2d>& points, const Color& fill_color, const Color& line_color, int line_width = 1) 
  {
    push_back(std::make_shared<GLPhongObject>(GLMesh::Polygon(points, /*bNormal*/false), fill_color, 0));
    push_back(std::make_shared<GLPhongObject>(GLMesh::LineLoop(points), line_color, line_width));
  }

  //destructor
  virtual ~GLPolygon() {
  }

};

///////////////////////////////////////////////////////////
class VISUS_GUI_API GLBox : public GLStruct
{
public:

  VISUS_NON_COPYABLE_CLASS(GLBox)

  //constructor
  GLBox(const Position& position, const Color& fill_color, const Color& line_color, int line_width = 1) {
    push_back(std::make_shared<GLModelview>(position.getTransformation()));
    push_back(std::make_shared<GLPhongObject>(GLMesh::SolidBox(position.getBoxNd()), fill_color, 0));
    push_back(std::make_shared<GLPhongObject>(GLMesh::WireBox(position.getBoxNd()), line_color, line_width));
  }

  //constructor
  GLBox(const BoxNd& box, const Color& fill_color, const Color& line_color, int line_width = 1)
    : GLBox(Position(box), fill_color, line_color, line_width) {}

  //constructor
  GLBox(const Frustum& value, const Color& fill_color, const Color& line_color, int line_width = 1, double Near = -1, double Far = +1)
    : GLBox(Position((value.getProjection() * value.getModelview()).invert(), BoxNd(Point3d(-1, -1, Near), Point3d(+1, +1, Far))), fill_color, line_color, line_width) {}

  //destructor
  virtual ~GLBox() {
  }
};

/////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLAxis : public GLStruct
{
public:

  VISUS_NON_COPYABLE_CLASS(GLAxis)

  //constructor
  GLAxis(const Position& position, int line_width = 1){
    push_back(std::make_shared<GLModelview>(position.getTransformation()));
    push_back(std::make_shared<GLPhongObject>(GLMesh::ColoredAxis(position.getBoxNd()), Colors::White, line_width));
  }

  //constructor
  GLAxis(const BoxNd& box, int line_width = 1)
  : GLAxis(Position(box), line_width) {}

  //GLAxis
  virtual ~GLAxis() {
  }

};

///////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLWireCircle : public GLStruct
{
public:

  VISUS_NON_COPYABLE_CLASS(GLWireCircle)

  //constructor
  GLWireCircle(double R = 1, Point2d center = Point2d(0, 0), const Color& color = Colors::Black, int line_width = 1) {
    push_back(std::make_shared<GLModelview>(Matrix::translate(Point3d(center))));
    push_back(std::make_shared<GLModelview>(Matrix::scale(Point3d(R, R, R))));
    push_back(std::make_shared<GLPhongObject>(*createMesh(), color, line_width));
  }

  //destructor
  virtual ~GLWireCircle() {
  }

private:

  //cacheMesh
  static SharedPtr<GLMesh> createMesh() {
    static auto ret=std::make_shared<GLMesh>(GLMesh::WireCircle(32)); return ret;
  }

};

///////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLSolidCircle : public GLStruct
{
public:

  VISUS_NON_COPYABLE_CLASS(GLSolidCircle)

  //constructor
  GLSolidCircle(double R = 1, Point2d center = Point2d(0, 0), const Color& color = Colors::Black){
    push_back(std::make_shared<GLModelview>(Matrix::translate(Point3d(center))));
    push_back(std::make_shared<GLModelview>(Matrix::scale(Point3d(R, R, R))));
    push_back(std::make_shared<GLPhongObject>(*createMesh(), color));
  }

  //destructor
  virtual ~GLSolidCircle() {

  }

private:

  static SharedPtr<GLMesh> createMesh() {
    static auto ret=std::make_shared<GLMesh>(GLMesh::SolidCircle());
    return ret;
  }

};

///////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLSolidSphere : public GLStruct
{
public:

  VISUS_NON_COPYABLE_CLASS(GLSolidSphere)


  //constructor
  GLSolidSphere(double R = 1, Point3d center = Point3d(0, 0,0), const Color& color = Colors::Black){
    push_back(std::make_shared<GLModelview>(Matrix::translate(center)));
    push_back(std::make_shared<GLModelview>(Matrix::scale(Point3d(R, R, R))));
    push_back(std::make_shared<GLPhongObject>(*createMesh(), color));
  }

  //constructor
  GLSolidSphere(double R, Point2d center, const Color& color = Colors::Black)
    : GLSolidSphere(R,Point3d(center),color) {
  }

  //destructor
  virtual ~GLSolidSphere(){
  }

private:

  static SharedPtr<GLMesh> createMesh() {
    static auto ret=std::make_shared<GLMesh>(GLMesh::SolidSphere()); return ret;
  }

};


#endif //

} //namespace

#endif //__VISUS_GL_OBJECTS_H
