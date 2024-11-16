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

#ifndef __VISUS_GL_CANVAS_H
#define __VISUS_GL_CANVAS_H

#include <Visus/Gui.h>

#include <Visus/Frustum.h>
 
#include <Visus/GLShader.h>
#include <Visus/GLMaterial.h>
#include <Visus/GLTexture.h>
#include <Visus/GLMesh.h>
#include <Visus/GLObject.h>
#include <Visus/GLMouse.h>

#include <QOpenGLWidget>
#include <QOpenGLContext>

#if !SWIG
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
  #include <QOpenGLExtraFunctions>
#else
  #include <QOpenGLFunctions>
  #define QOpenGLExtraFunctions QOpenGLFunctions
#endif
#endif


#include <functional>

namespace Visus {

class VISUS_GUI_API GLSharedContext
{
public:

  VISUS_DECLARE_SINGLETON_CLASS(GLSharedContext)

  //destructor
  ~GLSharedContext() {
    delete gl;
  }

  //makeCurrent
  void makeCurrent() {
    gl->makeCurrent();
  }

  //doneCurrent
  void doneCurrent() {
    gl->doneCurrent();
  }

private:

  QOpenGLWidget* gl;

  //constructor
  GLSharedContext() {
    gl = new QOpenGLWidget();
    gl->resize(120, 120);
    gl->show();
    gl->setVisible(false); //keep the gl_shared_canvas canvas in memory!
  }

};

//////////////////////////////////////////////////////
class VISUS_GUI_API GLNeedContext
{
public:

  VISUS_NON_COPYABLE_CLASS(GLNeedContext)

  //constructor
  GLNeedContext() {

    if (!QOpenGLContext::currentContext())
    {
      bMadeSharedCurrent=true;
      GLSharedContext::getSingleton()->makeCurrent();
    }
  }

  //destructor
  ~GLNeedContext() {
    if (bMadeSharedCurrent)
      GLSharedContext::getSingleton()->doneCurrent();
  }

  //operator->
  QOpenGLExtraFunctions* operator->() {
    return QOpenGLContext::currentContext()->extraFunctions();
  }

private:

  friend class GLCanvas;

  bool bMadeSharedCurrent=false;


};


///////////////////////////////////////////////////////
class VISUS_GUI_API GLDoWithContext
{
public:

  VISUS_DECLARE_SINGLETON_CLASS(GLDoWithContext)

  //push_back
  void push_back(std::function<void()> fn) 
  {
    if (QOpenGLContext::currentContext())
    {
      fn();
    }
    else
    {
      ScopedLock lock(this->lock);
      v.push_back(fn);
    }
  }

private:

  friend class GLCanvas;
  
  std::mutex lock;
  std::vector< std::function<void()> > v; 

  //constructor
  GLDoWithContext() {
  }

  //flush
  void flush(GLCanvas*) {
    ScopedLock lock(this->lock);
    for (auto fn:v)
      fn();
    v.clear();
  }

};

///////////////////////////////////////////////////////
class VISUS_GUI_API GLCanvas : 
  public QOpenGLWidget ,
  public QOpenGLExtraFunctions
{
  Q_OBJECT

public:

  VISUS_NON_COPYABLE_CLASS(GLCanvas)

  //constructor
  GLCanvas();

  //destructor
  virtual ~GLCanvas();

  //flushGLErrors
  int flushGLErrors(bool bVerbose=false);

signals:

  void glRenderEvent      (GLCanvas& gl);
  void glResizeEvent      (QResizeEvent* evt);
  void glKeyPressEvent    (QKeyEvent*    evt);
  void glMousePressEvent  (QMouseEvent*  evt);
  void glMouseMoveEvent   (QMouseEvent*  evt);
  void glMouseReleaseEvent(QMouseEvent*  evt);
  void glWheelEvent       (QWheelEvent*  evt);

public:

  //initializeGL
  virtual void initializeGL() override;

  //postRedisplay
  void postRedisplay(const int fps=30);

  //glClearColor
  void glClearColor(float r, float g, float b, float a) {
    QOpenGLFunctions::glClearColor(r,g,b,a);
  }

  //glClearColor
  void glClearColor(const Color& c) {
    glClearColor(c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha());
  }

public:

  //getShader
  GLShader* getShader() const
  {return this->shader;}

  //setShader
  void setShader(GLShader* value,bool bForce=false);

  //setUniform
  void setUniform(const GLUniform& uniform, int v1)                              {int location=program->getUniformLocation(uniform); if (location<0) return; glUniform1i(location,v1);}
  void setUniform(const GLUniform& uniform, int v1,int v2)                       {int location=program->getUniformLocation(uniform); if (location<0) return; glUniform2i(location,v1,v2);}
  void setUniform(const GLUniform& uniform, int v1,int v2,int v3)                {int location=program->getUniformLocation(uniform); if (location<0) return; glUniform3i(location,v1,v2,v3);}
  void setUniform(const GLUniform& uniform, int v1,int v2,int v3,int v4)         {int location=program->getUniformLocation(uniform); if (location<0) return; glUniform4i(location,v1,v2,v3,v4);}

  void setUniform(const GLUniform& uniform, float v1)                            {int location=program->getUniformLocation(uniform); if (location<0) return; glUniform1f(location,v1);}
  void setUniform(const GLUniform& uniform, float v1,float v2)                   {int location=program->getUniformLocation(uniform); if (location<0) return; glUniform2f(location,v1,v2);}
  void setUniform(const GLUniform& uniform, float v1,float v2,float v3)          {int location=program->getUniformLocation(uniform); if (location<0) return; glUniform3f(location,v1,v2,v3);}
  void setUniform(const GLUniform& uniform, float v1,float v2,float v3,float v4) {int location=program->getUniformLocation(uniform); if (location<0) return; glUniform4f(location,v1,v2,v3,v4);}

  void setUniform(const GLUniform& uniform, double v1)                               {setUniform(uniform,(float)v1);}
  void setUniform(const GLUniform& uniform, double v1,double v2)                     {setUniform(uniform,(float)v1,(float)v2);}
  void setUniform(const GLUniform& uniform, double v1,double v2,double v3)           {setUniform(uniform,(float)v1,(float)v2,(float)v3);}
  void setUniform(const GLUniform& uniform, double v1,double v2,double v3,double v4) {setUniform(uniform,(float)v1,(float)v2,(float)v3,(float)v4);}

  void setUniform(const GLUniform& uniform, Point2f v)                           {setUniform(uniform,v[0],v[1]);}
  void setUniform(const GLUniform& uniform, Point3f v)                           {setUniform(uniform,v[0],v[1],v[2]);}
  void setUniform(const GLUniform& uniform, Point4f v)                           {setUniform(uniform,v[0],v[1],v[2],v[3]);}
  void setUniform(const GLUniform& uniform, Point2d v)                           {setUniform(uniform,(float)v[0],(float)v[1]);}
  void setUniform(const GLUniform& uniform, Point3d v)                           {setUniform(uniform,(float)v[0],(float)v[1],(float)v[2]);}
  void setUniform(const GLUniform& uniform, Point4d v)                           {setUniform(uniform,(float)v[0],(float)v[1],(float)v[2],(float)v[3]);}

  void setUniformColor(const GLUniform& uniform, const Color& color)             {setUniform(uniform,color.getRed(),color.getGreen(),color.getBlue(),color.getAlpha());}
  void setUniformPlane(const GLUniform& uniform, const Plane& h)                 {VisusAssert(h.getSpaceDim()==4);setUniform(uniform,(float)h[0],(float)h[1],(float)h[2],(float)h[3]);}

  //setUniformMatrix
  void setUniformMatrix(const GLUniform& uniform,const Matrix& T);

  //pushClippingBox
  void pushClippingBox(BoxNd box)
  {
    box.setPointDim(3);

    auto Ti = Matrix(getModelview().invert());
    std::array<Plane, 6> planes = { {
      Plane(+1.0, 0.0, 0.0, -box.p1[0])*Ti,
      Plane(-1.0, 0.0, 0.0, +box.p2[0])*Ti,
      Plane(0.0, +1.0, 0.0, -box.p1[1])*Ti,
      Plane(0.0, -1.0, 0.0, +box.p2[1])*Ti,
      Plane(0.0, 0.0, +1.0, -box.p1[2])*Ti,
      Plane(0.0, 0.0, -1.0, +box.p2[2])*Ti
    } };
    clipping_box.push(planes);
    setClippingBoxIfNeeded();
  }

  //pushClippingBox
  void pushClippingBox(const Position& position)
  {
    auto save_modelview = getModelview();
    multModelview(position.getTransformation().withSpaceDim(4));
    pushClippingBox(position.getBoxNd());
    loadModelview(save_modelview);
  }

  //popClippingBox
  void popClippingBox()
  {
    clipping_box.pop();
    setClippingBoxIfNeeded();
  }

  //hasClippingBox
  bool hasClippingBox() const {
    return !clipping_box.empty();
  }

  //setUniformMaterial
  void setUniformMaterial(GLShader& shader,const GLMaterial& material) ;

  //setUniformLight
  void setUniformLight(GLShader& shader,const Point4d& light_pos) ;

public:

  //getViewport
  Viewport getViewport() const {
    return this->viewport.empty()? Viewport(0,0,width(),height()):this->viewport.top();
  }

  //setViewport
  void setViewport(const Viewport& value,bool bForce=false);

  //pushViewport
  void pushViewport();

  //popViewport
  void popViewport();

  //getProjection
  const Matrix& getProjection() const {
    return this->projection.top();
  }

  //setProjection
  void setProjection(const Matrix& value,bool bForce=false);

  //popProjection
  void pushProjection();

  //popProjection
  void popProjection();

  //loadProjection
  void loadProjection(const Matrix& value){
    setProjection(value);
  }

  //multProjection
  void multProjection(const Matrix& value){
    loadProjection(getProjection()*value);
  }

  //getModelview
  const Matrix& getModelview() const {
    return this->modelview.top();
  }

  //setModelview
  void setModelview(Matrix value,bool bForce=false);

  //loadModelview
  void loadModelview(const Matrix& value) {
    setModelview(value);
  }

  //multModelview
  void multModelview(const Matrix& value){
    loadModelview(getModelview()*value);
  }

  //pushModelview
  void pushModelview();

  //popModelview
  void popModelview();

  //getFrustum
  Frustum getFrustum() const{
    return Frustum(getViewport(),getProjection(),getModelview());
  }

  //setFrustum
  void setFrustum(const Frustum& value);

  //pushFrustum
  void pushFrustum();

  //pushFrustum
  void popFrustum();

  //setHud
  void setHud();

public:

  //getPointSize
  int getPointSize() const{
    return this->pointsize.top();
  }

  //setPointSize
  void setPointSize(int value,bool bForce=false);

  //pushPointSize
  void pushPointSize(int value);

  //popPointSize
  void popPointSize();

public:

  //getLineWidth
  int getLineWidth() const{
    return this->linewidth.top();
  }

  //setLineWidth
  void setLineWidth(int value,bool bForce=false);

  //pushLineWidth
  void pushLineWidth(int value);

  //popLineWidth
  void popLineWidth();

public:

  //getBlend
  bool getBlend() const {
    return this->blend.top();
  }

  //setBlend
  void setBlend(bool value, bool bForce = false);

  //pushBlend
  void pushBlend(bool value);

  //popBlend
  void popBlend();

public:

  //getDepthTest
  bool getDepthTest() const{
    return depthtest.top();
  }

  //setDepthTest
  void setDepthTest(bool value, bool bForce = false);

  //pushDepthTest
  void pushDepthTest(bool value);

  //popDepthTest
  void popDepthTest();

public:

  //getDepthMask
  bool getDepthMask() const {
    return this->depthmask.top();
  }

  //setDepthMask
  void setDepthMask(bool value, bool bForce = false);

  //pushDepthMask
  void pushDepthMask(bool value);

  //popDepthMask
  void popDepthMask();

public:

  //getDepthFunc
  int getDepthFunc() const {
    return this->depthfunc.top();
  }

  //setDepthFunction
  void setDepthFunc(int value, bool bForce = false);

  //pushDepthFunction
  void pushDepthFunc(int value);

  //popDepthFunction
  void popDepthFunc();

public:

  //getCullFace
  int getCullFace() const {
    return this->cullface.top();
  }

  //setCullFace
  void setCullFace(int value,bool bForce=false);

  //pushCullFace
  void pushCullFace(int value);

  //popCullFace
  void popCullFace();
  
public:

  //setTextureInSlot
  void setTextureInSlot(int slot,GLSampler& sampler,SharedPtr<GLTexture> texture);

  //setTexture
  void setTexture(GLSampler& sampler,SharedPtr<GLTexture> value) {
    setTextureInSlot(0,sampler,value);
  }

  //glRenderMesh
  void glRenderMesh(const GLMesh& mesh);

  //glRenderScreenText
  void glRenderScreenText(double x,double y,String s,Color color) {
    y=height()-y-1;
    glrender_text.push_back([x,y,s,color](QPainter& painter){
        painter.setPen(QUtils::convert<QColor>(color));
        painter.drawText(x,y,s.c_str());
    });
  }

  //paintGL
  virtual void paintGL() override;

private:

  GLShader*               shader=nullptr;
  SharedPtr<GLProgram>    program;

  std::stack<Viewport>    viewport;
  std::stack<Matrix>      projection;
  std::stack<Matrix>      modelview;
  std::stack<int>         linewidth;
  std::stack<int>         pointsize;
  std::stack<bool>        blend; 
  std::stack<bool>        depthtest;
  std::stack<bool>        depthmask;
  std::stack<int>         depthfunc;
  std::stack<int>         cullface;
  std::stack< std::array<Plane,6> > clipping_box;

  std::vector< std::function<void(QPainter&)> > glrender_text;

  bool                    bUpdating=false;

  std::vector< SharedPtr<GLProgram> > glprograms;

  //update
  void update();

  //updateGLScene
  void updateGLScene();
  
  //mirrorY
  QPointF mirrorY(QPointF value) const {
    return QPointF(value.x(),height()-1-value.y());
  }
  
  //keyPressEvent
  virtual void keyPressEvent(QKeyEvent* evt) override
  {
    QOpenGLWidget::keyPressEvent(evt);
    emit glKeyPressEvent(evt);
  }

  //resizeEvent
  virtual void resizeEvent(QResizeEvent* evt) override
  {
    QOpenGLWidget::resizeEvent(evt);
    emit glResizeEvent(evt);
  }

  //mousePressEvent
  virtual void mousePressEvent(QMouseEvent* evt) override
  {
    QOpenGLWidget::mousePressEvent(evt);
    
    QMouseEvent glEvt(evt->type(),mirrorY(evt->localPos()),evt->button(),evt->buttons(),evt->modifiers());
    glEvt.setAccepted(false);
    emit glMousePressEvent(&glEvt);
  }

  //mouseMoveEvent
  virtual void mouseMoveEvent(QMouseEvent* evt) override
  {
    QOpenGLWidget::mouseMoveEvent(evt);
    
    QMouseEvent glEvt(evt->type(),mirrorY(evt->localPos()),evt->button(),evt->buttons(),evt->modifiers());
    glEvt.setAccepted(false);
    emit glMouseMoveEvent(&glEvt);
  }

  //mouseReleaseEvent
  virtual void mouseReleaseEvent(QMouseEvent* evt) override
  {
    QOpenGLWidget::mouseReleaseEvent(evt);
    
    QMouseEvent glEvt(evt->type(),mirrorY(evt->localPos()),evt->button(),evt->buttons(),evt->modifiers());
    glEvt.setAccepted(false);
    emit glMouseReleaseEvent(&glEvt);
  }


  //wheelEvent
  virtual void wheelEvent(QWheelEvent* evt) override;

  //setClippingBoxIfNeeded
  void setClippingBoxIfNeeded();

};


} //namespace

#endif //__VISUS_GL_CANVAS_H
