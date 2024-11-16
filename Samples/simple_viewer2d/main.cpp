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

#include <Visus/Kernel.h>
#include <Visus/Dataflow.h>
#include <Visus/DatasetNode.h>
#include <Visus/QueryNode.h>
#include <Visus/GLCanvas.h>
#include <Visus/GLOrthoCamera.h>
#include <Visus/KdQueryNode.h>
#include <Visus/KdRenderArrayNode.h>
#include <Visus/RenderArrayNode.h>
#include <Visus/Gui.h>

#include <QMainWindow>
#include <QTimer>
#include <QFrame>
#include <QPushButton>
#include <QApplication>
#include <QBoxLayout>

using namespace Visus;

////////////////////////////////////////////////////////////////////
class DavidWindow : 
  public QMainWindow,
  public Dataflow::Listener
{
public:

  DatasetNode*              dataset_node=nullptr;
  QueryNode*                query_node=nullptr;
  GLObject*                 render_node=nullptr;
  Position                  dataset_bounds;

  SharedPtr<Dataflow>       dataflow;
  GLCanvas*                 glcanvas=nullptr;
  SharedPtr<GLOrthoCamera>  glcamera;
  QTimer                    timer;

  String                    david_url="http://atlantis.sci.utah.edu/mod_visus?dataset=david";

  //constructor
  DavidWindow() 
  {
    setWindowTitle("Gigapixel Viewer");

    createGui();
    openDataset();

    connect(&timer,&QTimer::timeout,this,&DavidWindow::idle);
    this->timer.start(1000/60);

    postRedisplay();
  }

  //destructor
  virtual ~DavidWindow() {
  }

  //createGui
  void createGui()
  {
    auto layout=new QHBoxLayout();

    //glcanvas
    {
      glcanvas=new GLCanvas();
 
      connect(glcanvas,&GLCanvas::glMousePressEvent,[this](QMouseEvent* evt){
        this->glcamera->glMousePressEvent(evt,glcanvas->getViewport());
      });

      connect(glcanvas,&GLCanvas::glMouseMoveEvent,[this](QMouseEvent* evt){
        this->glcamera->glMouseMoveEvent(evt, glcanvas->getViewport());
      });

      connect(glcanvas,&GLCanvas::glMouseReleaseEvent,[this](QMouseEvent* evt) {
        this->glcamera->glMouseMoveEvent(evt, glcanvas->getViewport());
      });

      connect(glcanvas,&GLCanvas::glWheelEvent,[this](QWheelEvent* evt){
        this->glcamera->glWheelEvent(evt, glcanvas->getViewport());
      });

      connect(glcanvas,&GLCanvas::glRenderEvent,[this](GLCanvas& gl)
      {
        gl.glClearColor(0,0,0,1);
        gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl.setFrustum(glcamera->getCurrentFrustum(glcanvas->getViewport()));

        if (render_node)
          render_node->glRender(gl);
      });

      layout->addWidget(glcanvas,8);
    }
    
    //buttons
    {
      auto buttons=new QVBoxLayout();

	  //choose timesteps
      for (int I=0;I<4;I++)
      {
        auto button = new QPushButton();
        button->setText(cstring("Time",I).c_str());
        connect(button,&QPushButton::clicked,[this,I](){
          setTime((double)I);
        });
        buttons->addWidget(button);
      }

      buttons->addStretch(1);

      layout->addLayout(buttons,2);
    }

    //glcamera
    {
      glcamera=std::make_shared<GLOrthoCamera>();
      
      //this is for the "final" frustum
      glcamera->end_update.connect([this]()
      {
        if (query_node) 
        {
          auto viewport = glcanvas->getViewport();
          query_node->setNodeToScreen(Frustum(glcamera->getFinalFrustum(viewport)));
          dataflow->needProcessInput(query_node);
        }
        postRedisplay();
      });
      
      //this is for the current frustum (interpolated between start and final)
      glcamera->redisplay_needed.connect([this](){
      	postRedisplay();
      }); 
      
      glcamera->guessPosition(BoxNd());
    }
    
    auto central=new QWidget();
    central->setLayout(layout);
    setCentralWidget(central);
  }

  //idle
  void idle() 
  {
    if (dataflow) 
      dataflow->dispatchPublishedMessages();
  }

  //openDataset
  virtual bool openDataset()
  {
    //open the new dataset
    SharedPtr<Dataset> dataset(LoadDataset(david_url));
    if (!dataset)
    {
      PrintInfo("LoadDataset", david_url, "failed.");
      return false;
    }

    PrintInfo("LoadDataset", david_url, "done");

    this->dataflow=std::make_shared<Dataflow>();
    this->dataflow->listeners.push_back(this);

    this->dataflow->addNode(this->dataset_node = new DatasetNode());
    this->dataset_node->setName("Dataset node");
    this->dataset_node->setDataset(dataset);
    this->dataset_bounds=dataset_node->getBounds();

    if (dataset->getKdQueryMode()!=0)
    {
      this->dataflow->addNode(this->query_node=new KdQueryNode());
      auto render_node=new KdRenderArrayNode();
      this->render_node = render_node;
      render_node->setName("KdRender");
      this->dataflow->addNode(render_node);
      this->dataflow->connectNodes(dataset_node,query_node);
      this->dataflow->connectNodes(query_node,"kdarray",render_node);
    }
    else
    { 
      this->dataflow->addNode(this->query_node=new QueryNode());
      auto render_node=new RenderArrayNode();
      this->render_node = render_node;
      render_node->setName("Render Node");
      this->dataflow->addNode(render_node);
      this->dataflow->connectNodes(dataset_node,query_node);
      this->dataflow->connectNodes(query_node,"array",render_node);
    }

    this->query_node->setAccessIndex(0);
    this->query_node->setProgression(QueryGuessProgression);
    this->query_node->setViewDependentEnabled(true);
    this->query_node->setQuality(QueryDefaultQuality);
    this->query_node->setBounds(dataset_bounds);
    this->query_node->setQueryBounds(dataset_bounds);
    this->glcamera->guessPosition(dataset_bounds.toAxisAlignedBox());

    setTime(dataset->getTime());
    setFieldName(dataset->getField().name);

    return true;
  }

  //setFieldName
  void setFieldName(String value)
  {
    auto port=this->query_node->getInputPort("fieldname");
    port->writeValue(value);
    dataflow->needProcessInput(query_node);
  }

  //setTime
  void setTime(double value)
  {
    auto port=query_node->getInputPort("time");
    port->writeValue(value);
    dataflow->needProcessInput(query_node);
  }

  //dataflowBeforeProcessInput
  virtual void dataflowBeforeProcessInput(Node* node) override {
  }

  //dataflowAfterProcessInput
  virtual void dataflowAfterProcessInput(Node* node) override {
    if (dynamic_cast<GLObject*>(node))
      postRedisplay();
  }

  //postRedisplay
  void postRedisplay()
  {
    if (glcanvas)
      glcanvas->postRedisplay();
  }

}; //end class


///////////////////////////////////////
int main(int argn,const char* argv[])
{
  SetCommandLine(argn, argv);
  GuiModule::attach();

  {
    auto win = new DavidWindow();
    win->resize(1024, 768);
    win->show();
    QApplication::exec();
    delete win;
  }
  
  GuiModule::detach();

  return 0;
}
