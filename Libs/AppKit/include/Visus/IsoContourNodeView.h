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

#ifndef VISUS_ISOCONTOUR_NODE_VIEW_H
#define VISUS_ISOCONTOUR_NODE_VIEW_H

#include <Visus/AppKit.h>
#include <Visus/IsoContourNode.h>
#include <Visus/Model.h>
#include <Visus/DoubleSlider.h>
#include <Visus/GuiFactory.h>

#include <QLabel>
#include <QFrame>
#include <QFormLayout>

namespace Visus {

/////////////////////////////////////////////////////////
class VISUS_APPKIT_API IsoContourNodeView : 
  public QFrame, 
  public View<IsoContourNode>
{
public:

  VISUS_NON_COPYABLE_CLASS(IsoContourNodeView)

  //_______________________________________________________
  class Widgets
  {
  public:
    DoubleSlider* slider=nullptr;
    QLabel*       data_min=nullptr;
    QLabel*       data_max=nullptr;
    QLabel*       value = nullptr;
  };

  Widgets widgets;

  //constructor
  IsoContourNodeView(IsoContourNode* model) {
    bindModel(model);
  }

  //destructor
  virtual ~IsoContourNodeView()
  {
    bindModel(nullptr);
  }

  //bindModel
  virtual void bindModel(IsoContourNode* value) override
  {
    if (this->model)
    {
      QUtils::clearQWidget(this);
      widgets=Widgets();
    }

    View<ModelClass>::bindModel(value);

    if (this->model)
    {
      QFormLayout* layout=new QFormLayout();
      
      layout->addRow("Isovalue", widgets.slider = GuiFactory::CreateDoubleSliderWidget(0, Range(0, 1, 0), [this](double value) {
        model->setIsoValue(value);
      }));

      layout->addRow("Value",widgets.value  =new QLabel("0.0"));
      layout->addRow("From",widgets.data_min=new QLabel("0.0"));
      layout->addRow("To"  ,widgets.data_max=new QLabel("0.0"));

      setLayout(layout);
      refreshGui();
    }
  }

private:

  //refreshGui
  void refreshGui()
  {
    Range range=model->getDataRange();
    if (!range.delta())
      range=Range::numeric_limits<double>();

    widgets.slider->setRange(range);
    widgets.slider->setValue(model->isoValue());
    widgets.value->setText(cstring(model->isoValue()).c_str());
    widgets.data_min->setText(cstring(range.from).c_str());
    widgets.data_max->setText(cstring(range.to  ).c_str());
  }

  //modelChanged
  virtual void modelChanged() override {
    refreshGui();
  }

};

} //namespace Visus

#endif //VISUS_ISOCONTOUR_NODE_VIEW_H


