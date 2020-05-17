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

#ifndef VISUS_TIME_NODE_VIEW_H__
#define VISUS_TIME_NODE_VIEW_H__

#include <Visus/Gui.h>
#include <Visus/TimeNode.h>
#include <Visus/Model.h>
#include <Visus/QDoubleSlider.h>

#include <QTimer>
#include <QFrame>
#include <QLabel>

namespace Visus {


/////////////////////////////////////////////////////////
class VISUS_GUI_API TimeNodeView :
  public QFrame,
  public View<TimeNode>
  {
public:

  VISUS_NON_COPYABLE_CLASS(TimeNodeView)
  
  class VISUS_GUI_API Widgets
  {
  public:
    QLineEdit*       value=nullptr;
    QDoubleSlider*   slider=nullptr;
    QToolButton*     prev=nullptr;
    QToolButton*     next=nullptr;
    struct
    {
      QLineEdit*       from=nullptr;
      QLineEdit*       to=nullptr;
      QLineEdit*       step=nullptr;
    }
    user_range;

    QToolButton*     play_stop_button=nullptr;
    QLineEdit*       play_msec=nullptr;
  };

  Widgets widgets;

  //constructor
  TimeNodeView(TimeNode* model) {
    bindModel(model);
  }
  
  //destructor
  virtual ~TimeNodeView() {
    bindModel(nullptr);
  }
  
  //bindModel
  virtual void bindModel(TimeNode* value) override 
  {
    if (this->model)
    {
      QUtils::clearQWidget(this);
      widgets=Widgets();
    }
  
    View<ModelClass>::bindModel(value);
  
    if (this->model)
    { 
      widgets.slider=GuiFactory::CreateDoubleSliderWidget(0,Range(0,1,0),[this](double value){
        this->model->setCurrentTime(value);
      });
      
      widgets.value=GuiFactory::CreateDoubleTextBoxWidget(0,[this](double value) {
        this->model->setCurrentTime(value);
      });

      widgets.prev=GuiFactory::CreateButton("-",[this](bool){
        this->model->setCurrentTime(Utils::clamp(model->getCurrentTime()-model->getUserRange().step,model->getUserRange().from,model->getUserRange().to));
      });

      widgets.next=GuiFactory::CreateButton("+",[this](bool){ 
        this->model->setCurrentTime(Utils::clamp(model->getCurrentTime()+model->getUserRange().step,model->getUserRange().from,model->getUserRange().to));
      });
    
      widgets.user_range.from=GuiFactory::CreateDoubleTextBoxWidget(0,[this](double value){auto r=model->getUserRange(); r.from=cdouble(widgets.user_range.from->text()); this->model->setUserRange(r);});
      widgets.user_range.to  =GuiFactory::CreateDoubleTextBoxWidget(0,[this](double value){auto r=model->getUserRange(); r.to  =cdouble(widgets.user_range.to  ->text()); this->model->setUserRange(r);});
      widgets.user_range.step=GuiFactory::CreateDoubleTextBoxWidget(0,[this](double value){auto r=model->getUserRange(); r.step=cdouble(widgets.user_range.step->text()); this->model->setUserRange(r);});
    
      widgets.play_msec=GuiFactory::CreateIntegerTextBoxWidget(0,[this](int value) {
        this->model->setPlayMsec(value);
      });

      widgets.play_stop_button=GuiFactory::CreateButton("Start",[this](bool){
        isPlaying()? stopPlay():startPlay();
      });
    
      QFormLayout* layout=new QFormLayout();

      {
        auto sub=new QHBoxLayout();
        sub->addWidget(widgets.prev);
        sub->addWidget(widgets.value);
        sub->addWidget(widgets.next);
        layout->addRow("Value",sub);
      }

      layout->addRow("",widgets.slider);

      layout->addRow("From",widgets.user_range.from);
      layout->addRow("To",widgets.user_range.to);
      layout->addRow("Step",widgets.user_range.step);

      {
        auto sub=new QHBoxLayout();
        sub->addWidget(new QLabel("Msec"));
        sub->addWidget(widgets.play_msec);
        sub->addWidget(widgets.play_stop_button);
        layout->addRow("",sub);
      }
    
      setLayout(layout);
      refreshGui();
    }
  }
  
private:
  
  QTimer                   play_timer;
  SharedPtr<ReturnReceipt> play_return_receipt;
  
  //refreshGui
  void refreshGui() 
  {
    double curr=model->getCurrentTime();
    widgets.value->setText(cstring(curr).c_str());
    widgets.slider->setRange(model->getUserRange());
    widgets.slider->setValue(curr);
    widgets.user_range.from->setText(cstring(model->getUserRange().from).c_str());
    widgets.user_range.to->setText(cstring(model->getUserRange().to  ).c_str());
    widgets.user_range.step->setText(cstring(model->getUserRange().step).c_str());
    widgets.play_msec->setText(cstring(model->getPlayMsec()).c_str());
  }
  
  //modelChanged
  virtual void modelChanged() override{
    refreshGui();
  }
  
  //isPlaying
  bool isPlaying() {
    return play_timer.isActive()?true:false;
  }

  //startPlay
  void startPlay() 
  {
    VisusAssert(VisusHasMessageLock());

    if (isPlaying()) 
      return;

    int msec=cint(widgets.play_msec->text());
    if (msec<=0) 
      return ;

    widgets.play_stop_button->setText("Stop");
    connect(&play_timer,&QTimer::timeout,[this]() {
      VisusAssert(isPlaying());
    
      //TODO: this is not perfect... what if the model has non linear timesteps?
      double value   = model->getCurrentTime();
    
      //finished playing?
      if (value>=model->getUserRange().to)
      {
        stopPlay();
        return;
      }
    
      //not ready yet... I will retry later
      if (!play_return_receipt->isReady())
        return;
    
      play_return_receipt=std::make_shared<ReturnReceipt>();
      model->setCurrentTime(value+model->getUserRange().step,/*bDoPublish*/false);
      model->doPublish(play_return_receipt);
    });
  
    //start playing
    play_timer.start(msec);
    play_return_receipt=std::make_shared<ReturnReceipt>();
  }
  
  //stopPlay
  void stopPlay() 
  {
    VisusAssert(VisusHasMessageLock());

    if (!isPlaying()) 
      return;

    widgets.play_stop_button->setText("Start");
    play_return_receipt.reset();
    play_timer.stop();
  }


};


} //namespace Visus

#endif //VISUS_TIME_NODE_VIEW_H__


