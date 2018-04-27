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

#ifndef VISUS_FREE_TRANSFORM_VIEW_H__
#define VISUS_FREE_TRANSFORM_VIEW_H__

#include <Visus/AppKit.h>
#include <Visus/FreeTransform.h>

#include <QApplication>
#include <QGroupBox>

namespace Visus {

////////////////////////////////////////////////////////////////////////
class VISUS_APPKIT_API FreeTransformView : 
  public View<FreeTransform>,
  public QFrame
{
public:

  VISUS_NON_COPYABLE_CLASS(FreeTransformView)

  //____________________________________________________________________________
  class Widgets
  {
  public:


    GuiFactory::Point3dView*  vt                =nullptr;
    QToolButton*              vt_forward        =nullptr;
    QToolButton*              vt_backward       =nullptr;

    GuiFactory::Point3dView*  vr                =nullptr;
    QToolButton*              vr_forward        =nullptr;
    QToolButton*              vr_backward       =nullptr;

    GuiFactory::Point3dView*  vs                =nullptr;
    GuiFactory::Point3dView*  vs_center         =nullptr;
    QToolButton*              vs_forward        =nullptr;
    QToolButton*              vs_backward       =nullptr;

    GuiFactory::MatrixView*   T                 =nullptr;
    GuiFactory::Box3dView*    box               =nullptr;

    GLCanvas*                 preview           =nullptr;
    Frustum                   preview_frustum;
  };

  Widgets widgets;

  //costructor
  FreeTransformView(FreeTransform* value) {
    bindModel(value);
  }

  //destructor
  virtual ~FreeTransformView() {
    bindModel(nullptr);
  }

  //bindModel
  virtual void bindModel(FreeTransform* value) override
  {
    if (this->model)
    {
      QUtils::clearQWidget(this);
      widgets=Widgets();
    }

    View<FreeTransform>::bindModel(value);

    if (this->model)
    {
      auto tabs=new QTabWidget();
      tabs->addTab(createTranslateRotateScaleWidget(),"Transform");
      tabs->addTab(createPositionWidget(),"Position");
      tabs->addTab(createPreviewWidget(),"Preview");
      auto layout=new QVBoxLayout();
      layout->addWidget(tabs);
      setLayout(layout);

    }
  }


  //refreshGui
  void refreshGui()
  {
    auto dragging=model->getDragging();

    //translate
    {
      for (int I=0;I<3;I++)
      {
        bool bAllowTranslate=model->canTranslate(I);
        widgets.vt->setEnabled(I,bAllowTranslate); 
        if (!bAllowTranslate) 
          widgets.vt->setPoint(widgets.vt->getPoint().set(I,0.0));
      }

      if (dragging.type==FreeTransform::Translating)
        widgets.vt->setPoint(dragging.vt);   
    }

    //rotate
    {
      for (int I=0;I<3;I++)
      {
        bool bAllowRotate=model->canRotate(I);
        widgets.vr->setEnabled(I,bAllowRotate);
        if (!bAllowRotate) 
          widgets.vr->setPoint(widgets.vr->getPoint().set(I,0.0));
      }

      if (dragging.type==FreeTransform::Rotating)
      {
        Point3d degrees;
        for (int I=0;I<3;I++) degrees[I]=Utils::radiantToDegree(dragging.vr[I]);
        widgets.vr->setPoint(degrees,/*precision*/0);
      }
    }

    //scale
    {
      for (int I=0;I<3;I++)
      {
        bool bAllowScale=model->canScale(I);
        widgets.vs->setEnabled(I,bAllowScale);
        widgets.vs_center->setEnabled(I,bAllowScale);
        if (!bAllowScale) 
        {
          widgets.vs->setPoint(widgets.vs->getPoint().set(I,100.0));
          widgets.vs_center->setPoint(widgets.vs_center->getPoint().set(I,0.0));
        } 
      }

      if (dragging.type==FreeTransform::Scaling)
      {
        widgets.vs->setPoint(100.0*dragging.vs,/*precision*/0);
        widgets.vs_center->setPoint(dragging.vs_center);
      }
    }

    widgets.T->setMatrix(model->getObject().getTransformation()); 
    widgets.box->setValue(model->getObject().getBox());

  }

private:

  //modelChanged
  virtual void modelChanged() override{
    refreshGui();
  }

  //createTranslateRotateScaleWidget
  QFrame* createTranslateRotateScaleWidget()
  {  
    auto layout=new QVBoxLayout();

    //translate
    {
      auto row=new QHBoxLayout();
      row->addWidget(widgets.vt_backward=GuiFactory::CreateButton(QIcon(":/backward.png"),"",[this](bool) {
        auto vt=-1*widgets.vt->getPoint();
        model->doTranslate(vt);
      }));

      row->addWidget(widgets.vt=GuiFactory::CreatePoint3dView(Point3d()));

      row->addWidget(widgets.vt_forward=GuiFactory::CreateButton(QIcon(":/forward.png"),"",[this](bool) {
        auto vt=+1*widgets.vt->getPoint();
        model->doTranslate(vt);
      }));

      layout->addWidget(new QLabel("Translate"));
      layout->addLayout(row);
    }

    //rotate
    {
      auto row=new QHBoxLayout();

      row->addWidget(widgets.vr_backward=GuiFactory::CreateButton(QIcon(":/backward.png"),"",[this](bool) {
        auto vr=-widgets.vr->getPoint(); 
        for (auto I=0;I<3;I++) 
          vr[I]=Utils::degreeToRadiant(vr[I]);
        model->doRotate(vr);
      }));

      row->addWidget(widgets.vr=GuiFactory::CreatePoint3dView(Point3d()));

      row->addWidget(widgets.vr_forward =GuiFactory::CreateButton(QIcon(":/forward.png"),"",[this](bool){
        auto vr=+widgets.vr->getPoint(); 
        for (auto I=0;I<3;I++) 
          vr[I]=Utils::degreeToRadiant(vr[I]);
        model->doRotate(vr);
      }));

      layout->addWidget(new QLabel("Rotate (Degrees)"));
      layout->addLayout(row);
    }
    
    //scale
    {
      auto grid=new QGridLayout();

      int R=0;

      grid->addWidget(new QLabel("Scale (%)"),R,/*col*/0,/*rowspan*/1,/*colspan*/3);
      R++;

      grid->addWidget(widgets.vs_backward=GuiFactory::CreateButton(QIcon(":/backward.png"),"",[this](bool){
        auto vs=0.01*widgets.vs->getPoint();
        auto vs_center=widgets.vs_center->getPoint();
        vs=vs.inv();
        model->doScale(vs,vs_center);
      }),R,0);

      grid->addWidget(widgets.vs=GuiFactory::CreatePoint3dView(Point3d(100,100,100)),R,1);

      grid->addWidget(widgets.vs_forward =GuiFactory::CreateButton(QIcon(":/forward.png"),"",[this](bool){
        auto vs=0.01*widgets.vs->getPoint();
        auto vs_center=widgets.vs_center->getPoint();
        model->doScale(vs,vs_center);
      }),R,2);

      ++R;
        
      grid->addWidget(new QLabel("Scale center"),R,/*col*/0,/*rowspan*/1,/*colspan*/3);
      R++;

      grid->addWidget(widgets.vs_center=GuiFactory::CreatePoint3dView(Point3d(0,0)),R,1);
      R++;
 
      layout->addLayout(grid);
    }


    auto ret=new QFrame();
    ret->setLayout(layout);
    return ret;
  }

  //createPositionWidget
  QWidget* createPositionWidget()
  {
    auto layout=new QFormLayout();

    //Matrix
    layout->addRow("Matrix",widgets.T=GuiFactory::CreateMatrixView(Matrix::identity(),[this](const Matrix& T){
      model->setObject(Position(T,model->getObject().getBox()),true);
    }));

    //Box
    layout->addRow("Box",widgets.box=GuiFactory::CreateBox3dView(Box3d(),[this](const Box3d& box){
      model->setObject(Position(model->getObject().getTransformation(),box),true);
    }));

    auto ret=new QFrame();
    ret->setLayout(layout);
    return ret;
  }

  //createPreviewWidget
  QWidget* createPreviewWidget()
  {
    auto ret=new QVBoxLayout();
    widgets.preview=new GLCanvas();
    
    connect(widgets.preview,&GLCanvas::glRenderEvent,[this](GLCanvas& gl) {
      glCanvasRender(gl);
    });
    
    connect(widgets.preview,&GLCanvas::glResizeEvent,[this](QResizeEvent* evt) {
      model->setObject(model->getObject());
      widgets.preview->postRedisplay();
    });
    
    connect(widgets.preview,&GLCanvas::glMousePressEvent,[this](QMouseEvent* evt) {
      model->glMousePressEvent(widgets.preview_frustum,evt);widgets.preview->postRedisplay();
    });
    
    connect(widgets.preview,&GLCanvas::glMouseMoveEvent,[this](QMouseEvent* evt) {
      model->glMouseMoveEvent(widgets.preview_frustum,evt);
      widgets.preview->postRedisplay();
    });
    
    connect(widgets.preview,&GLCanvas::glMouseReleaseEvent,[this](QMouseEvent* evt) {
      model->glMouseReleaseEvent(widgets.preview_frustum,evt);
      widgets.preview->postRedisplay();
    });
    
    ret->addWidget(widgets.preview);
    return widgets.preview;
  }

  //glCanvasRender 
  void glCanvasRender(GLCanvas& gl)
  {
    auto dragging=this->model->getDragging();

    if (!dragging.type)
    {
      double width =this->widgets.preview->width ();
      double height=this->widgets.preview->height();
      double x=((width>=height)?width/height:1.0)*1.2;
      double y=((width>=height)?1.0:height/width)*1.2;
      LocalCoordinateSystem lcs=LocalCoordinateSystem(this->model->getObject()).toUniformSize();
      widgets.preview_frustum.setViewport(Viewport(0,0,(int)width,(int)height));
      widgets.preview_frustum.loadProjection(Matrix::ortho(-x,+x,-y,+y,-10,+10));
      widgets.preview_frustum.loadModelview(Matrix::identity());
      widgets.preview_frustum.multModelview(Matrix::lookAt(Point3d(1,1,1),Point3d(0,0,0),Point3d(0,0,1)));
      widgets.preview_frustum.multModelview(Matrix(lcs.getXAxis(),lcs.getYAxis(),lcs.getZAxis(),lcs.getCenter()).invert());
    }    

    gl.setViewport(Viewport(0,0,widgets.preview->width(),widgets.preview->height()));
    gl.glClearColor(Colors::DarkBlue);
    gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl.setFrustum(widgets.preview_frustum);
    GLBox(model->getObject(),Colors::Transparent,Colors::Black).glRender(gl);
    model->glRender(gl);
  }

};


} //namespace Visus

#endif //VISUS_FREE_TRANSFORM_VIEW_H__

