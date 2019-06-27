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

#include <Visus/GLCanvas.h>

#include <QTimer>

namespace Visus {


VISUS_IMPLEMENT_SINGLETON_CLASS(GLSharedContext)
VISUS_IMPLEMENT_SINGLETON_CLASS(GLDoWithContext)

/////////////////////////////////////////////////////////////////////////////
GLCanvas::GLCanvas() 
{
  glprograms.resize(4096);
  QOpenGLWidget::setMouseTracking(true); 
}

/////////////////////////////////////////////////////////////////////////////
GLCanvas::~GLCanvas() {
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::initializeGL() {
  QOpenGLExtraFunctions::initializeOpenGLFunctions();
}

/////////////////////////////////////////////////////////////////////////////
static String getGLErrorMessage(const GLenum e)
{
  switch (e)
  {
  case GL_INVALID_ENUM:                   return "GL_INVALID_ENUM";
  case GL_INVALID_VALUE:                  return "GL_INVALID_VALUE";
  case GL_INVALID_OPERATION:              return "GL_INVALID_OPERATION";
  case GL_OUT_OF_MEMORY:                  return "GL_OUT_OF_MEMORY";
#ifdef GL_STACK_OVERFLOW
  case GL_STACK_OVERFLOW:                 return "GL_STACK_OVERFLOW";
#endif
#ifdef GL_STACK_UNDERFLOW
  case GL_STACK_UNDERFLOW:                return "GL_STACK_UNDERFLOW";
#endif
#ifdef GL_INVALID_FRAMEBUFFER_OPERATION
  case GL_INVALID_FRAMEBUFFER_OPERATION:  return "GL_INVALID_FRAMEBUFFER_OPERATION";
#endif
  default: break;
  }

  return "Unknown error";
};

/////////////////////////////////////////////////////////////////////////////
int GLCanvas::flushGLErrors(bool bVerbose)
{
  int nerrors=0;

  for (GLenum glerr=glGetError();glerr!=GL_NO_ERROR;glerr=glGetError())
  {
    ++nerrors;
    
    if (bVerbose)
    {
      String error_str=getGLErrorMessage(glerr);
      VisusInfo()<<"glGetError returned "<<glerr<<" ("<<error_str<<")";
    }
  }

  return nerrors;
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::postRedisplay(const int fps)
{
  if (bUpdating) return;
  bUpdating=true;
  QTimer::singleShot(1000/fps,this,&GLCanvas::updateGLScene);
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::update() {
  QOpenGLWidget::update();
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::updateGLScene()
{
  bUpdating=false;
  update();
}



/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setShader(GLShader* value,bool bForce) {

  if (!bForce && value==getShader()) 
    return;
 
  this->shader=value;
  
  if (value)
  {
    int shader_id=shader->getId(); VisusAssert(shader_id<glprograms.size());
    program=glprograms[shader_id];

    if (!program)
      program=glprograms[shader_id]=std::make_shared<GLProgram>(this,*shader);

    program->bind();
    setProjection(getProjection(),/*bForce*/true);
    setModelview (getModelview (),/*bForce*/true);
    setClippingBoxIfNeeded();
  }
  else
  {
    this->program=nullptr;
    this->glUseProgram(0);
  }
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setUniformMatrix(const GLUniform& uniform,const Matrix& T)
{
  int location=program->getUniformLocation(uniform); 
  if (location<0) return; 

  if (T.getSpaceDim() == 3)
  {
    const float fv[] = { (float)T[0],(float)T[3],(float)T[6],
                      (float)T[1],(float)T[4],(float)T[7],
                      (float)T[2],(float)T[5],(float)T[8] };
    glUniformMatrix3fv(location, 1, false, fv);
  }
  else
  {
    VisusAssert(T.getSpaceDim() == 4)
    {
      const float fv[] = { (float)T[0],(float)T[4],(float)T[8],(float)T[12],
                        (float)T[1],(float)T[5],(float)T[9],(float)T[13],
                        (float)T[2],(float)T[6],(float)T[10],(float)T[14],
                        (float)T[3],(float)T[7],(float)T[11],(float)T[15] };
      glUniformMatrix4fv(location, 1, false, fv);
    }
  }
}





/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setClippingBoxIfNeeded() {

  if (shader && shader->u_clippingbox_plane[0].id > 0 && !clipping_box.empty())
  {
    auto planes = clipping_box.top();
    setUniformPlane(shader->u_clippingbox_plane[0], planes[0]);
    setUniformPlane(shader->u_clippingbox_plane[1], planes[1]);
    setUniformPlane(shader->u_clippingbox_plane[2], planes[2]);
    setUniformPlane(shader->u_clippingbox_plane[3], planes[3]);
    setUniformPlane(shader->u_clippingbox_plane[4], planes[4]);
    setUniformPlane(shader->u_clippingbox_plane[5], planes[5]);
  }
}


/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setUniformMaterial(GLShader& shader,const GLMaterial& material) 
{
  setUniformColor(shader.u_frontmaterial_ambient  ,material.front.ambient);
  setUniformColor(shader.u_frontmaterial_diffuse  ,material.front.diffuse);
  setUniformColor(shader.u_frontmaterial_specular ,material.front.specular);
  setUniformColor(shader.u_frontmaterial_emission ,material.front.emission);
  setUniform(shader.u_frontmaterial_shininess,(double)material.front.shininess); 

  setUniformColor(shader.u_backmaterial_ambient  ,material.back.ambient);
  setUniformColor(shader.u_backmaterial_diffuse  ,material.back.diffuse);
  setUniformColor(shader.u_backmaterial_specular ,material.back.specular);
  setUniformColor(shader.u_backmaterial_emission ,material.back.emission);
  setUniform(shader.u_backmaterial_shininess,(double)material.back.shininess);
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setUniformLight(GLShader& shader,const Point4d& light_pos) 
{
  VisusAssert(light_pos.w==1); 
  Point4d pos=getModelview()*light_pos;
  setUniform(shader.u_light_position,pos);
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setViewport(const Viewport& value,bool bForce) 
{
  if (!bForce && value==getViewport()) return;
  this->viewport.top()=value;
  
  float sx=devicePixelRatio();
  float sy=devicePixelRatio();
  glViewport(sx*value.x,sy*value.y,sx*value.width,sy*value.height);
}

void GLCanvas::pushViewport() {
  viewport.push(getViewport());
}

void GLCanvas::popViewport() {
  auto value=getViewport();
  viewport.pop();
  bool bForce=value!=getViewport();
  setViewport(getViewport(),bForce);
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setProjection(const Matrix& value,bool bForce) {
  if (!bForce && value==getProjection()) return;
  this->projection.top()=value;
  if (auto shader=getShader()) 
    setUniformMatrix(shader->u_projection_matrix,value);
}

void GLCanvas::pushProjection() {
  this->projection.push(getProjection());
}

void GLCanvas::popProjection() {
  auto value=getProjection();
  this->projection.pop();
  bool bForce=value!=getProjection();
  setProjection(getProjection(),bForce);
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setModelview(const Matrix& value,bool bForce) {
  if (!bForce && value==getModelview()) return;
  this->modelview.top()=value;
  if (auto shader=getShader()) 
  {
    setUniformMatrix(shader->u_modelview_matrix,value);
    setUniformMatrix(shader->u_normal_matrix ,value.invert().transpose().withoutBack());
  }
}


void GLCanvas::pushModelview() {
  this->modelview.push(getModelview());
}

void GLCanvas::popModelview() {
  auto value=getModelview();
  this->modelview.pop();
  bool bForce=value!=getModelview();
  setModelview(getModelview(),bForce);
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setFrustum(const Frustum& value)
{
  setViewport(value.getViewport());
  loadProjection(value.getProjection());
  loadModelview(value.getModelview());
}

void GLCanvas::pushFrustum() {
  pushViewport();
  pushProjection();
  pushModelview();
}

void GLCanvas::popFrustum() {
  popViewport();
  popProjection();
  popModelview();
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setHud()
{
  int W=width ();
  int H=height();
  Frustum frustum;
  frustum.setViewport(Viewport(0,0,W,H));
  frustum.loadProjection(Matrix::ortho(0,W,0,H,-1,+1));
  frustum.loadModelview(Matrix::identity(4));
  setFrustum(frustum);
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setLineWidth(int value,bool bForce) {
  if (!bForce && value==getLineWidth()) return;
  this->linewidth.top()=value;
  glLineWidth((float)value);
}

void GLCanvas::pushLineWidth(int value) {
  bool bForce=value!=getLineWidth();
  linewidth.push(value);
  setLineWidth(value,bForce);
}

void GLCanvas::popLineWidth() {
  auto value=getLineWidth();
  linewidth.pop();
  bool bForce=value!=getLineWidth();
  setLineWidth(getLineWidth(),bForce);
}


/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setPointSize(int value,bool bForce) {
  if (!bForce && value==getPointSize()) return;
  this->pointsize.top()=value;
  glPointSize((float)value);
}

void GLCanvas::pushPointSize(int value) {
  bool bForce=value!=getPointSize();
  pointsize.push(value);
  setPointSize(value,bForce);
}

void GLCanvas::popPointSize() {
  auto value=getPointSize();
  pointsize.pop();
  bool bForce=value!=getPointSize();
  setPointSize(getPointSize(),bForce);
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setBlend(bool value,bool bForce) {
  if (!bForce && value==getBlend()) return;
  this->blend.top()=value;
  value? glEnable(GL_BLEND) : glDisable(GL_BLEND);
}

void GLCanvas::pushBlend(bool value) {
  bool bForce=value!=getBlend();
  blend.push(value);
  setBlend(value,bForce);
}

void GLCanvas::popBlend() {
  auto value=getBlend();
  blend.pop();
  bool bForce=value!=getBlend();
  setBlend(getBlend(),bForce);
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setDepthTest(bool value,bool bForce) {
  if (!bForce && value==getDepthTest()) return;
  this->depthtest.top()=value;
  value? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
}


void GLCanvas::pushDepthTest(bool value) {
  bool bForce=value!=getDepthTest();
  depthtest.push(value);
  setDepthTest(value,bForce);
}

void GLCanvas::popDepthTest() {
  auto value=getDepthTest();
  depthtest.pop();
  bool bForce=value!=getDepthTest();
  setDepthTest(getDepthTest(),bForce);
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setDepthMask(bool value,bool bForce) {
  if (!bForce && value==getDepthMask()) return;
  this->depthmask.top()=value;
  glDepthMask(value);
}


void GLCanvas::pushDepthMask(bool value) {
  bool bForce=value!=getDepthMask();
  depthmask.push(value);
  setDepthMask(value,bForce);
}

void GLCanvas::popDepthMask() {
  auto value=getDepthMask();
  depthmask.pop();
  bool bForce=value!=getDepthMask();
  setDepthMask(getDepthMask(),bForce);
}


/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setDepthFunc(int value,bool bForce)  
{
  if (!bForce && value==getDepthFunc()) return;
  this->depthfunc.top()=value;
  glDepthFunc(value);
}

void GLCanvas::pushDepthFunc(int value) 
{
  bool bForce=value!=getDepthFunc();
  depthfunc.push(value);
  setDepthFunc(value,bForce);
}

void GLCanvas::popDepthFunc()
{
  auto value=getDepthFunc();
  depthfunc.pop();
  bool bForce=value!=getDepthFunc();
  setDepthFunc(getDepthFunc(),bForce);
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setCullFace(int value,bool bForce)  
{
  if (!bForce && value==getCullFace()) return;
  this->cullface.top()=value;
  glCullFace(value);
}

void GLCanvas::pushCullFace(int value) 
{
  bool bForce=value!=getCullFace();
  cullface.push(value);
  setCullFace(value,bForce);
}

void GLCanvas::popCullFace()
{
  auto value=getCullFace();
  cullface.pop();
  bool bForce=value!=getCullFace();
  setCullFace(getCullFace(),bForce);
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::setTextureInSlot(int slot,GLSampler& sampler,SharedPtr<GLTexture> texture) 
{
  VisusAssert(slot>=0 && slot<8);

  if (!texture) 
    return;

  int texture_id = texture->textureId(*this);
  if (!texture_id)
    return;

  int target = texture->target();

  glActiveTexture(GL_TEXTURE0+slot);

  glBindTexture  (target , texture_id);
  glTexParameteri(target , GL_TEXTURE_MAG_FILTER ,(QOpenGLTexture::Filter)texture->magfilter);
  glTexParameteri(target  ,GL_TEXTURE_MIN_FILTER ,(QOpenGLTexture::Filter)texture->minfilter);

  glTexParameteri(target , GL_TEXTURE_WRAP_S ,(QOpenGLTexture::WrapMode)texture->wrap);
  glTexParameteri(target , GL_TEXTURE_WRAP_T ,(QOpenGLTexture::WrapMode)texture->wrap);

  if (target==QOpenGLTexture::Target3D)
    glTexParameteri(target , GL_TEXTURE_WRAP_R ,(QOpenGLTexture::WrapMode)texture->wrap);

  glActiveTexture(GL_TEXTURE0);

  setUniform(sampler.u_sampler,slot);
  setUniform(sampler.u_sampler_dims ,Point3d((double)texture->dims.x,(double)texture->dims.y,(double)texture->dims.z));
  setUniform(sampler.u_sampler_vs,texture->vs);
  setUniform(sampler.u_sampler_vt,texture->vt);
  setUniform(sampler.u_sampler_envmode,texture->envmode);
  setUniform(sampler.u_sampler_ncomponents, texture->dtype.ncomponents());
}

/////////////////////////////////////////////////////////////////////////////
void GLCanvas::paintGL() 
{
  if (!isVisible()) 
    return;

  GLDoWithContext::getSingleton()->flush(this);

  #ifdef GL_NORMALIZE
  glEnable(GL_NORMALIZE);
  #endif
  
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  #if defined(GL_PERSPECTIVE_CORRECTION_HINT) && defined(GL_NICEST)
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  #endif
  
  #if defined(GL_SMOOTH)
  glShadeModel(GL_SMOOTH);
  #endif

  #if defined(GL_POINT_SMOOTH)
  glEnable(GL_POINT_SMOOTH);
  #endif

  #if defined GL_AMBIENT_AND_DIFFUSE
  glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
  #endif

  glLineWidth(1);
  glPointSize(1);
  glColorMask(1,1,1,1);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glDepthFunc(GL_LESS);
  glDisable(GL_POLYGON_OFFSET_FILL);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_POLYGON_OFFSET_FILL);
  glDepthFunc(GL_LESS);

  viewport  .push(Viewport(0,0,width(),height()));
  projection.push(Matrix::identity(4));
  modelview .push(Matrix::identity(4));
  pointsize .push(1);
  linewidth .push(1);
  blend     .push(false);
  depthtest .push(true);
  depthmask .push(true);
  depthfunc .push(GL_LESS);

  const bool bForce=true;
  setShader(nullptr,bForce);
  setProjection(getProjection(),bForce);
  setModelview(getModelview(),bForce);
  setViewport(getViewport(),bForce);
  setLineWidth(getLineWidth(),bForce);
  setBlend(getBlend(),bForce);
  setDepthTest(getDepthTest(),bForce);
  setDepthMask(getDepthMask(),bForce);

  emit glRenderEvent(*this);

  setShader(nullptr,true);

  QPainter painter(this);
  painter.setFont(QFont());
  painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
  for (auto fn : glrender_text)
    fn(painter);
  glrender_text.clear();

  VisusAssert(viewport  .size()==1);viewport   =std::stack<Viewport>();
  VisusAssert(projection.size()==1);projection =std::stack<Matrix>();
  VisusAssert(modelview .size()==1);modelview  =std::stack<Matrix>();
  VisusAssert(linewidth .size()==1);linewidth  =std::stack<int>();
  VisusAssert(blend     .size()==1);blend      =std::stack<bool>();
  VisusAssert(depthtest .size()==1);depthtest  =std::stack<bool>();
  VisusAssert(depthmask .size()==1);depthmask  =std::stack<bool>();
}



///////////////////////////////////////////////
void GLCanvas::glRenderMesh(const GLMesh& mesh) 
{
  if (mesh.batches.empty())
    return;

  VisusAssert(shader && program);

  for (auto batch:mesh.batches)
  {
    int P=-1; if (batch.vertices ) batch.vertices ->enableForAttribute(*this,P=program->getAttributeLocation(GLAttribute::a_position));
    int N=-1; if (batch.normals  ) batch.normals  ->enableForAttribute(*this,N=program->getAttributeLocation(GLAttribute::a_normal));
    int C=-1; if (batch.colors   ) batch.colors   ->enableForAttribute(*this,C=program->getAttributeLocation(GLAttribute::a_color));
    int T=-1; if (batch.texcoords) batch.texcoords->enableForAttribute(*this,T=program->getAttributeLocation(GLAttribute::a_texcoord));

    glDrawArrays(mesh.primitive,0,batch.getNumberOfVertices());

    if (batch.vertices ) batch.vertices ->disableForAttribute(*this,P);
    if (batch.normals  ) batch.normals  ->disableForAttribute(*this,N);
    if (batch.colors   ) batch.colors   ->disableForAttribute(*this,C);
    if (batch.texcoords) batch.texcoords->disableForAttribute(*this,T);
  }
}


} //namespace Visus


