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

#include <Visus/Viewer.h>
#include <Visus/GLOrthoCamera.h>
#include <Visus/GLLookAtCamera.h>

#include <Visus/QueryNode.h>
#include <Visus/RenderArrayNode.h>
#include <Visus/KdRenderArrayNode.h>
#include <Visus/ModelViewNode.h>
#include <Visus/IsoContourRenderNode.h>

#include <Visus/IdxMultipleDataset.h>

#include <QApplication>

namespace Visus {

/////////////////////////////////////////////////////////////////////////////////////
class GLSortNode
{
public:

  std::pair<double,double> distance;
  Frustum                  frustum;
  GLObject*                globject;

  //constructor
  GLSortNode(int nqueue_,double distance_,Frustum frustum_,GLObject* globject_)
    :distance(std::make_pair((double)nqueue_,distance_)),frustum(frustum_),globject(globject_)
  {}

  //operator<
  inline bool operator<(const GLSortNode &b) const {
    return this->distance<b.distance;
  }
}; 

////////////////////////////////////////////////////////////////////////////////////////////////////
void Viewer::guessGLCameraPosition(int ref)  
{
  if (auto glcamera=getGLCamera())
    glcamera->guessPosition(this->getWorldBox(),ref);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Viewer::mirrorGLCamera(int ref)  
{ 
  if (auto glcamera=getGLCamera())
    glcamera->mirror(ref);
}

//////////////////////////////////////////////////////////////////////
GLCanvas* Viewer::createGLCanvas()
{
  auto ret=new GLCanvas();
  {
    connect(ret,&GLCanvas::glMouseMoveEvent     ,this,&Viewer::glCanvasMouseMoveEvent);
    connect(ret,&GLCanvas::glMousePressEvent    ,this,&Viewer::glCanvasMousePressEvent);
    connect(ret,&GLCanvas::glMouseReleaseEvent  ,this,&Viewer::glCanvasMouseReleaseEvent);
    connect(ret,&GLCanvas::glRenderEvent        ,this,&Viewer::glRender);
    connect(ret,&GLCanvas::glResizeEvent        ,this,&Viewer::glCanvasResizeEvent);
    connect(ret,&GLCanvas::glWheelEvent         ,this,&Viewer::glCanvasWheelEvent);
  }

  return ret;
}

////////////////////////////////////////////////////////////////////////////////
void Viewer::attachGLCamera(SharedPtr<GLCamera> value) 
{
  VisusAssert(widgets.glcanvas);
  VisusAssert(value);

  detachGLCamera();

  this->glcamera =value;

  ViewerAutoRefresh auto_refresh;
  auto_refresh.enabled = std::dynamic_pointer_cast<GLOrthoCamera>(value) ? true : false;
  auto_refresh.msec = 0;
  setAutoRefresh(auto_refresh);

  //for final frustum (which is part of the GLCamera model)
  this->glcamera->end_update.connect(this->glcamera_end_update_slot=[this](){
    glCameraChangeEvent();
  });

  //for interpolated frustum (which is not part of the GLCamera model)
  this->glcamera->redisplay_needed.connect(this->glcamera_redisplay_needed_slot = [this]() {
    postRedisplay();
  });

  postRedisplay();
}

////////////////////////////////////////////////////////////////////////////////
void Viewer::detachGLCamera()
{
  if (!this->glcamera)
    return;
  
  this->glcamera->end_update.disconnect(this->glcamera_end_update_slot);
  this->glcamera->redisplay_needed.disconnect(this->glcamera_redisplay_needed_slot);
  this->glcamera.reset();
}


////////////////////////////////////////////////////////////////////////////////
void Viewer::glCameraChangeEvent() 
{
  if (!getGLCamera())
    return;

  auto auto_refresh = getAutoRefresh();
  if (!auto_refresh.enabled)
    return;

  //the auto refresh timer will automaticall do what it's needed
  if (auto_refresh.msec>0)
    return;

  auto viewport = widgets.glcanvas->getViewport();

  for (auto node : getNodes())
  {
    if (auto query_node = dynamic_cast<QueryNode*>(node))
    {
      //IMPORTANT: refresh the data considering the FINAL frustum!
      auto node_to_screen = computeNodeToScreen(getGLCamera()->getFinalFrustum(viewport), query_node->getDatasetNode());

      if (node_to_screen != query_node->nodeToScreen())
        dataflow->needProcessInput(query_node);
    }
  }
}

////////////////////////////////////////////////////////////
void Viewer::glCanvasResizeEvent(QResizeEvent* evt)           
{
  auto glcamera=getGLCamera();
  if (!glcamera) 
    return;
}

////////////////////////////////////////////////////////////
void Viewer::glCanvasMousePressEvent(QMouseEvent* evt)
{
  auto glcamera=getGLCamera();
  if (!glcamera) 
    return;

  //start dragging
  this->mouse_timer.reset();
  if (!this->mouse.getNumberOfButtonDown())
    setMouseDragging(true);

  this->mouse.glMousePressEvent(evt);

  auto viewport = widgets.glcanvas->getViewport();
  
  if (free_transform)
  {
    free_transform->glMousePressEvent(FrustumMap(glcamera->getCurrentFrustum(viewport)),evt);
    if (evt->isAccepted()) {
      postRedisplay();
      return;
    }
  }

  //the center of rotation is fixed
  if (auto lookat = dynamic_cast<GLLookAtCamera*>(glcamera.get()))
  {
    auto bounds = getBounds(getSelection());
    lookat->setCameraSelection(bounds);
  }

  glcamera->glMousePressEvent(evt, viewport);

  //needed for gesture render
  postRedisplay();
}

////////////////////////////////////////////////////////////
void Viewer::glCanvasMouseMoveEvent(QMouseEvent* evt)
{
  auto glcamera=getGLCamera();
  if (!glcamera) 
    return;

  auto viewport = widgets.glcanvas->getViewport();
  this->mouse.glMouseMoveEvent(evt);

  if (free_transform)
  {
    free_transform->glMouseMoveEvent(FrustumMap(glcamera->getCurrentFrustum(viewport)),evt);
    if (evt->isAccepted()) {
      postRedisplay();
      return;
    }
  }

  glcamera->glMouseMoveEvent(evt, viewport);
}

////////////////////////////////////////////////////////////
void Viewer::glCanvasMouseReleaseEvent(QMouseEvent* evt)
{
  auto glcamera=getGLCamera();
  if (!glcamera) 
    return;

  auto viewport = widgets.glcanvas->getViewport();
  this->mouse_timer.reset();
  this->mouse.glMouseReleaseEvent(evt);

  if (free_transform)
  {
    free_transform->glMouseReleaseEvent(FrustumMap(glcamera->getCurrentFrustum(viewport)),evt);
    if (evt->isAccepted()) 
    {
      setMouseDragging(false);

      //need to get all levels now that mouse is not dragging anymore
      if (free_transform && getWorldDimension()==3)
        refreshNode(getSelection());

      postRedisplay();
      return;
    }
  }

  glcamera->glMouseReleaseEvent(evt, viewport);

  //postpone a little the end-drag event for the camera
  if (!this->mouse.getNumberOfButtonDown() && isMouseDragging())
    scheduleMouseDragging(false, 1000);

  //click == change current selection
  if (evt->button()==Qt::LeftButton && this->mouse.wasSingleClick(evt->button()))
  {
    //disable selection for ortho glcamera
    if (!std::dynamic_pointer_cast<GLOrthoCamera>(getGLCamera()))
    {
      Node* new_selection=nullptr;
      Node* old_selection=getSelection();

      //force current selection
      if ((QApplication::keyboardModifiers() & Qt::ControlModifier) && (QApplication::keyboardModifiers() & Qt::AltModifier))
      {
        new_selection=old_selection;
      }
      //clicked on the current selection
      else if (old_selection && findPick(old_selection,Point2d(evt->x(),evt->y()),/*bUseChilds*/false))
      {
        new_selection=old_selection;
      }
      //choose a selection
      else
      {
        new_selection=findPick(getRoot(),Point2d(evt->x(),evt->y()),/*bUseChilds*/true);
      } 
  
      if (new_selection!=old_selection)
      {
        if (old_selection)
          dropSelection();

        if (new_selection)
          setSelection(new_selection);
      }
    }
  }

  //needed for gesture render
  postRedisplay();  
}

////////////////////////////////////////////////////////////
void Viewer::glCanvasWheelEvent(QWheelEvent* evt)
{
  auto glcamera=getGLCamera();
  if (!glcamera) return;

  auto viewport = widgets.glcanvas->getViewport();
  this->mouse_timer.reset();
  if (!this->mouse.getNumberOfButtonDown())
  {
    setMouseDragging(true);
    glcamera->glWheelEvent(evt, viewport);
    scheduleMouseDragging(false, 1000);
  }
  else
  {
    glcamera->glWheelEvent(evt, viewport);
  }
}

////////////////////////////////////////////////////////////
int Viewer::glGetRenderQueue(Node* node)
{
  const int DoNotDisplay=-1;

  VisusAssert(node);

  //DatasetNode
  if (auto dataset_node=dynamic_cast<DatasetNode*>(node))
    return 0;

  //RenderArrayNode
  if (auto array_render=dynamic_cast<RenderArrayNode*>(node))
    return array_render->getDataDimension()>2? 2: 3;

  //KdRenderArrayNode
  if (auto kdarray_render_node=dynamic_cast<KdRenderArrayNode*>(node))
  {
    if (SharedPtr<KdArray> kdarray=kdarray_render_node->getKdArray())
      return kdarray->getPointDim();  //2 or 3
    return DoNotDisplay;
  }

  //IsoContourRenderNode
  if (auto isocontour_render_node = dynamic_cast<IsoContourRenderNode*>(node))
    return 8; //before RenderNode...

  //GLObject
  if (auto globject=dynamic_cast<GLObject*>(node))
  {
    int ret=globject->glGetRenderQueue();
    return ret>=0? ret:1;
  }

  return DoNotDisplay;
}


/////////////////////////////////////////////////////////////////////////////////////
void Viewer::glRenderNodes(GLCanvas& gl)
{
  auto glcamera=getGLCamera();
  auto viewport = gl.getViewport();

  bool bOrthoCamera=std::dynamic_pointer_cast<GLOrthoCamera>(glcamera)?true:false;
  std::vector<GLSortNode> sorted_nodes;
  std::stack< std::pair<Frustum,Node*> > stack;
  stack.push(std::make_pair(glcamera->getCurrentFrustum(viewport),getRoot()));
  while (!stack.empty())
  {
    Frustum frustum=stack.top().first;
    Node* node=stack.top().second;
    stack.pop();

    if (!node || !node->isVisible())
      continue;

    if (auto modelview_node=dynamic_cast<ModelViewNode*>(node))
      frustum.multModelview(modelview_node->getModelView());

    int nqueue=glGetRenderQueue(node);
    if (nqueue>=0) 
    {
      if (auto globject = dynamic_cast<GLObject*>(node))
      {
        auto node_to_screen= computeNodeToScreen(getGLCamera()->getCurrentFrustum(viewport),node);
        Position bounds= getBounds(node);
        bool bUseFarPoint=(nqueue==2);
        double distance= node_to_screen.computeZDistance(bounds,bUseFarPoint);
        if (bOrthoCamera || distance>=0)
          sorted_nodes.push_back(GLSortNode(nqueue,distance, node_to_screen,globject));
      }
    }

    if (auto dataset_node=dynamic_cast<DatasetNode*>(node))
    {
      if (dataset_node->showBounds())
      {
        auto bounds= getBounds(dataset_node);
        GLBox(bounds,Colors::Transparent,Colors::Black.withAlpha(0.5)).glRender(gl);
        GLAxis(bounds, 3).glRender(gl);
      }

      auto dataset = dataset_node->getDataset();

      //render annotations
      if (dataset->annotations && dataset->annotations->enabled)
      {
        auto frustum = computeNodeToScreen(getGLCamera()->getCurrentFrustum(viewport), dataset_node);
        auto map = FrustumMap(frustum);

        for (auto annotation : *dataset->annotations)
        {
          //poi
          if (auto poi = std::dynamic_pointer_cast<PointOfInterest>(annotation))
          {
            auto screen_pos = map.projectPoint(Point3d(poi->point));
            auto obj = std::make_shared<GLQuad>(
              screen_pos - Point2d(poi->magnet_size / 2, poi->magnet_size / 2),
              screen_pos + Point2d(poi->magnet_size / 2, poi->magnet_size / 2),
              poi->fill, poi->stroke, poi->stroke_width);
            huds.push_back(obj);
          }
          //polygon
          else if (auto polygon = std::dynamic_pointer_cast<PolygonAnnotation>(annotation))
          {
            std::vector<Point2d> screen_points;
            for (auto it : polygon->points)
              screen_points.push_back(map.projectPoint(Point3d(it)));
            auto obj = std::make_shared<GLPolygon>(screen_points, polygon->fill, polygon->stroke, polygon->stroke_width);
            huds.push_back(obj);
          }
          else
          {
            VisusAssert(false);
          }
        }
      }
    }

    for (auto child : node->getChilds())
      stack.push(std::make_pair(frustum,child));
  }

  std::sort(sorted_nodes.begin(),sorted_nodes.end());

  //in reverse order...
  std::reverse(sorted_nodes.begin(),sorted_nodes.end());

  for (auto sorted_node : sorted_nodes)
  {
    gl.pushFrustum();
    gl.setFrustum(sorted_node.frustum);
    gl.pushDepthTest(bOrthoCamera?false:true);

    if (auto obj=sorted_node.globject)
    {
      if (auto render=dynamic_cast<RenderArrayNode*>(obj))
        render->bFastRendering=isMouseDragging();

      obj->glRender(gl);
    }
    gl.popDepthTest();
    gl.popFrustum();
  }  

}

/////////////////////////////////////////////////////////////////////////////////////
void Viewer::glRenderSelection(GLCanvas& gl)
{
  auto viewport = gl.getViewport();
  if (Node* selection=getSelection())
  {
    auto bounds= getBounds(selection);
    if (bounds.valid())
    {
      gl.pushFrustum();
      gl.setFrustum(computeNodeToScreen(getGLCamera()->getCurrentFrustum(viewport),selection));
      GLBox(bounds,Colors::Transparent,Colors::Black.withAlpha(0.5)).glRender(gl);
      gl.popFrustum();
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////
void Viewer::glRenderGestures(GLCanvas& gl)
{
  auto& mouse= this->mouse;
  if (mouse.getButton(Qt::LeftButton).isDown || mouse.getButton(Qt::MidButton).isDown)
  {
    GLMouse::ButtonStatus b1=mouse.getButton(Qt::LeftButton);
    GLMouse::ButtonStatus b2=mouse.getButton(Qt::MidButton);
    float R=30;//radius in pixels

    if (!b1.isDown && !b2.isDown)
      return;

    gl.pushFrustum();
    gl.setHud();
    gl.pushDepthTest(false);
    gl.pushBlend(true);

    Point2d p1= b1.pos.castTo<Point2d>(),P1= b1.down.castTo<Point2d>();
    Point2d p2= b2.pos.castTo<Point2d>(),P2= b2.down.castTo<Point2d>();

    if (b1.isDown && b2.isDown) 
    {
      GLWireCircle(R,P1,Color(255,0,255,128)).glRender(gl);
      GLWireCircle(R,P2,Color(255,0,255,128)).glRender(gl);
      GLWireCircle(0.5*(P1-P2).module(),0.5*(P1+P2),Color(255,0,255,128)).glRender(gl);
    }

    if (b1.isDown) GLWireCircle(R,p1,Color(255,255,0,128)).glRender(gl);
    if (b2.isDown) GLWireCircle(R,p2,Color(255,255,0,128)).glRender(gl);

    if (b1.isDown && b2.isDown) 
      GLWireCircle(0.5*(p1-p2).module(),0.5*(p1+p2),Color(255,255,0,128),3).glRender(gl);

    gl.popBlend();
    gl.popDepthTest();
    gl.popFrustum();
  }
}

/////////////////////////////////////////////////////////////////////////////////////
void Viewer::glRenderLogos(GLCanvas& gl)
{
  if (!preferences.show_logos)
    return;

  int W = gl.getViewport().width;
  int H = gl.getViewport().height;

  gl.pushFrustum();
  gl.setHud();
  gl.pushDepthTest(false);
  gl.pushBlend(true);

  GLPhongShader* shader=GLPhongShader::getSingleton(GLPhongShader::Config().withTextureEnabled(true));
  gl.setShader(shader);

  for (auto& logo : this->logos)
  {
    double ratio = logo->tex->width() / (double)logo->tex->height();
    Point2d p1(
      logo->border[0] + logo->pos[0]*(W - 2 * logo->border[0] - logo->tex->width()),
      logo->border[1] + logo->pos[1]*(H - 2 * logo->border[1] - logo->tex->height()));

    shader->setUniformColor(gl,Colors::White.withAlpha((float)logo->opacity));
    shader->setTexture(gl,logo->tex);
    gl.glRenderMesh(GLMesh::Quad(p1, p1 + Point2d(logo->tex->width(),logo->tex->height()), false,/*bTexCoord*/true));
  }

  gl.popBlend();
  gl.popDepthTest();
  gl.popFrustum();
}

/////////////////////////////////////////////////////////////////////////////////////
void Viewer::glRender(GLCanvas& gl)
{
  auto viewport = widgets.glcanvas->getViewport();

  huds.clear();

  gl.setViewport(viewport);
  gl.glClearColor(background_color);
  gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  auto glcamera=getGLCamera();
  if (!glcamera)
    return;

  gl.setFrustum(glcamera->getCurrentFrustum(viewport));

  glRenderNodes(gl);
  glRenderSelection(gl);

  if (free_transform)
    free_transform->glRender(gl);

  // render huds
  if (!huds.empty())
  {
    gl.pushFrustum();
    gl.setHud();
    gl.pushBlend(true);
    gl.pushDepthTest(false);

    for (auto it : huds)
      it->glRender(gl);

    gl.popBlend();
    gl.popDepthTest();
    gl.popFrustum();
  }

  glRenderGestures(gl);
  glRenderLogos(gl);

  huds.clear();
}


} //namespace Visus

