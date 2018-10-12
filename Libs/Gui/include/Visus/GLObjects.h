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

  VISUS_CLASS(GLModelview)

  Matrix T;

  //constructor
  GLModelview(Matrix T_=Matrix::identity()) : T(T_) {
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

  VISUS_CLASS(GLPhongObject)

  int                  line_width=1;
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

    gl.glRenderMesh(mesh);
    
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

  VISUS_CLASS(GLStruct)

    std::vector< SharedPtr<GLObject> > v;

  //constructor
  GLStruct(const std::vector< SharedPtr<GLObject> >& v_ = std::vector< SharedPtr<GLObject> >()) : v(v_) {
  }

  //destructor
  virtual ~GLStruct() {
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


class VISUS_GUI_API GLLine : public GLPhongObject
{
public:

  VISUS_CLASS(GLLine)

  //constructor
  template <typename Point>
  GLLine(const Point& p1_,const Point& p2_,const Color& color_,int line_width_=1) 
    : GLPhongObject(GLMesh::Lines(std::vector<Point>({p1_,p2_})),color_,line_width_) {}
};

/////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLLines : public GLPhongObject
{
public:

  VISUS_CLASS(GLLines)

  //constructor
  template <typename Point>
  GLLines(const std::vector<Point>& points_,const Color& color_,int line_width_=1) 
    : GLPhongObject(GLMesh::Lines(points_),color_,line_width_) {}
};

/////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLLineStrip : public GLPhongObject
{
public:

  VISUS_CLASS(GLLineStrip)

  //constructor
  template <typename Point>
  GLLineStrip(const std::vector<Point>& vertices_,const Color& color_,int line_width_=1) 
    : GLPhongObject(GLMesh::LineStrip(vertices_),color_,line_width_) {}
};

/////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLCubicBezier : public GLLineStrip
{
public:

  VISUS_CLASS(GLCubicBezier)

  //constructor
  template <typename Point>
  GLCubicBezier(Point A, Point B, Point C, Point D,Color color=Colors::Black,int line_width=1,const int nsegments=32)
    : GLLineStrip(bezier(A,B,C,D,nsegments),color,line_width) {}

private:

  template <typename Point>
  static std::vector<Point> bezier(Point A, Point B, Point C, Point D,int nsegments) {
    std::vector<Point> ret;
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

  VISUS_CLASS(GLLineLoop)

  //constructor
  template <typename Point>
  GLLineLoop(const std::vector<Point>& vertices,const Color& color,int line_width=1) 
    : GLPhongObject(GLMesh::LineLoop(vertices),color,line_width) {}

};

///////////////////////////////////////////////////////////
class VISUS_GUI_API GLQuad : public GLStruct
{
public:

  VISUS_CLASS(GLQuad)

  //constructor
  template <typename Point>
  GLQuad(const Point& p1, const Point& p2, const Point& p3, const Point& p4, const Color& fill_color, const Color& line_color, int line_width = 1, SharedPtr<GLTexture> texture = SharedPtr<GLTexture>())
    : GLStruct({
      std::make_shared<GLPhongObject>(GLMesh::Quad(p1, p2, p3, p4, false, texture ? true : false), fill_color, 0, texture),
      std::make_shared<GLPhongObject>(GLMesh::LineLoop(std::vector<Point>({ p1,p2,p3,p4 })), line_color, line_width)
  }) {}

  //constructor
  template <typename Point>
  GLQuad(Point p1, Point p2, const Color& fill_color, const Color& line_color, int line_width = 1, SharedPtr<GLTexture> texture = SharedPtr<GLTexture>())
    : GLQuad(Point(p1.x, p1.y), Point(p2.x, p1.y), Point(p2.x, p2.y), Point(p1.x, p2.y), fill_color, line_color, line_width, texture) {}

  //constructor
  template <typename Point>
  GLQuad(const std::vector<Point>& points, const Color& fill_color, const Color& line_color, int line_width = 1, SharedPtr<GLTexture> texture = SharedPtr<GLTexture>())
    : GLQuad(points[0], points[1], points[2], points[3], fill_color, line_color, line_width, texture) {}

  //constructor
  template <typename Point>
  GLQuad(const std::array<Point,4>& points, const Color& fill_color, const Color& line_color, int line_width = 1, SharedPtr<GLTexture> texture = SharedPtr<GLTexture>())
    : GLQuad(points[0], points[1], points[2], points[3], fill_color, line_color, line_width, texture) {}


};

///////////////////////////////////////////////////////////
class VISUS_GUI_API GLBox : public GLStruct
{
public:

  VISUS_CLASS(GLBox)

  //constructor
  GLBox(const Position& position, const Color& fill_color, const Color& line_color, int line_width = 1)
    : GLStruct({
      std::make_shared<GLModelview>(position.getTransformation()),
      std::make_shared<GLPhongObject>(GLMesh::SolidBox(position.getBox()), fill_color, 0),
      std::make_shared<GLPhongObject>(GLMesh::WireBox(position.getBox()), line_color, line_width)
    }) {}

  //constructor
  GLBox(const Box3d& box, const Color& fill_color, const Color& line_color, int line_width = 1)
    : GLBox(Position(box), fill_color, line_color, line_width) {}

  //constructor
  GLBox(const Frustum& value, const Color& fill_color, const Color& line_color, int line_width = 1, double Near = -1, double Far = +1)
    : GLBox(Position((value.getProjection() * value.getModelview()).invert(), Box3d(Point3d(-1, -1, Near), Point3d(+1, +1, Far))), fill_color, line_color, line_width) {}

};

/////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLAxis : public GLStruct
{
public:

  VISUS_CLASS(GLAxis)

  //constructor
  GLAxis(const Position& position, int line_width = 1)
    : GLStruct({
      std::make_shared<GLModelview>(position.getTransformation()),
      std::make_shared<GLPhongObject>(GLMesh::ColoredAxis(position.getBox()), Colors::White, line_width)
    }){}

  //constructor
  GLAxis(const Box3d& box, int line_width = 1)
  : GLAxis(Position(box), line_width) {}

};

///////////////////////////////////////////////////////////////////
class VISUS_GUI_API GLWireCircle : public GLStruct
{
public:

  VISUS_CLASS(GLWireCircle)

  //constructor
  GLWireCircle(double R = 1, Point2d center = Point2d(0, 0), const Color& color = Colors::Black, int line_width = 1)
    : GLStruct({
      std::make_shared<GLModelview>(Matrix::translate(Point3d(center))),
      std::make_shared<GLModelview>(Matrix::scale(Point3d(R, R, R))),
      std::make_shared<GLPhongObject>(*createMesh(), color, line_width)
    }) {}

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

  VISUS_CLASS(GLSolidCircle)

  //constructor
  GLSolidCircle(double R = 1, Point2d center = Point2d(0, 0), const Color& color = Colors::Black)
    : GLStruct({
      std::make_shared<GLModelview>(Matrix::translate(Point3d(center))),
      std::make_shared<GLModelview>(Matrix::scale(Point3d(R, R, R))),
      std::make_shared<GLPhongObject>(*createMesh(), color)
  }) {}

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

  VISUS_CLASS(GLSolidSphere)


  //constructor
  GLSolidSphere(double R = 1, Point3d center = Point3d(0, 0,0), const Color& color = Colors::Black)
    : GLStruct({
    std::make_shared<GLModelview>(Matrix::translate(center)),
    std::make_shared<GLModelview>(Matrix::scale(Point3d(R, R, R))),
    std::make_shared<GLPhongObject>(*createMesh(), color)
      }) {}


  //constructor
  GLSolidSphere(double R, Point2d center, const Color& color = Colors::Black)
    : GLSolidSphere(R,Point3d(center),color) {
  }

private:

  static SharedPtr<GLMesh> createMesh() {
    static auto ret=std::make_shared<GLMesh>(GLMesh::SolidSphere()); return ret;
  }

};


#endif //

} //namespace

#endif //__VISUS_GL_OBJECTS_H
