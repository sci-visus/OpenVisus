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

#ifndef VISUS_VOXELSCOOP_NODE_VIEW_H__
#define VISUS_VOXELSCOOP_NODE_VIEW_H__

#include <Visus/AppKit.h>
#include <Visus/VoxelScoopNode.h>
#include <Visus/GuiFactory.h>

#include <QFrame>

namespace Visus {

/////////////////////////////////////////////////////////
class VISUS_APPKIT_API VoxelScoopNodeView : 
  public QFrame, 
  public View<VoxelScoopNode>
{
public:

  VISUS_NON_COPYABLE_CLASS(VoxelScoopNodeView)

  //constructor
  VoxelScoopNodeView(VoxelScoopNode* model=nullptr) {
    if (model)
      bindModel(model);
  }

  //destructor
  virtual ~VoxelScoopNodeView() {
    bindModel(nullptr);
  }

  //bindModel
  virtual void bindModel(VoxelScoopNode* value) override
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

      
      layout->addRow("simplify",widgets.simplify=GuiFactory::CreateCheckBox(model->simplify,"",[this](int value){
        model->beginUpdate(); model->simplify=value; model->endUpdate();
      }));

      layout->addRow("min_length",widgets.min_length=GuiFactory::CreateDoubleTextBoxWidget(model->min_length,[this](double value){
        model->beginUpdate(); model->min_length=value; model->endUpdate();
      }));

      layout->addRow("min_ratio",widgets.min_ratio=GuiFactory::CreateDoubleTextBoxWidget(model->min_ratio,[this](double value){
        model->beginUpdate(); model->min_ratio=value; model->endUpdate();
      }));

      layout->addRow("threshold",widgets.threshold=GuiFactory::CreateDoubleTextBoxWidget(model->threshold,[this](double value){
        model->beginUpdate(); model->threshold=value; model->endUpdate();
      }));

      layout->addRow("bUseMinimaAsSeed",widgets.bUseMinimaAsSeed=GuiFactory::CreateCheckBox(model->bUseMinimaAsSeed,"",[this](int value){
        model->beginUpdate(); model->bUseMinimaAsSeed=value; model->endUpdate();
      }));

      layout->addRow("bUseMaximaAsSeed",widgets.bUseMaximaAsSeed=GuiFactory::CreateCheckBox(model->bUseMaximaAsSeed,"",[this](int value){
        model->beginUpdate(); model->bUseMaximaAsSeed=value; model->endUpdate();
      }));

      layout->addRow("min_diam",widgets.min_diam=GuiFactory::CreateDoubleTextBoxWidget(model->min_diam,[this](double value){
        model->beginUpdate(); model->min_diam=value; model->endUpdate();
      }));

      setLayout(layout);
      refreshGui();
    }
  }

private:

  class Widgets
  {
  public:
    QCheckBox* simplify=nullptr;
    QLineEdit* min_length=nullptr;
    QLineEdit* min_ratio=nullptr;
    QLineEdit* threshold=nullptr;
    QCheckBox* bUseMinimaAsSeed=nullptr;
    QCheckBox* bUseMaximaAsSeed=nullptr;
    QLineEdit* min_diam=nullptr;
  };

  Widgets widgets;

  ///refreshGui
  void refreshGui()
  {
    widgets.simplify->setChecked(model->simplify);
    widgets.min_length->setText(cstring(model->min_length).c_str());
    widgets.min_ratio->setText(cstring(model->min_ratio).c_str());
    widgets.threshold->setText(cstring(model->threshold).c_str());
    widgets.bUseMinimaAsSeed->setChecked(model->bUseMinimaAsSeed);
    widgets.bUseMaximaAsSeed->setChecked(model->bUseMaximaAsSeed);
    widgets.min_diam->setText(cstring(model->min_diam).c_str());
  }

  //modelChanged
  virtual void modelChanged() override {
    refreshGui();
  }

};

} //namespace Visus

#endif //VISUS_VOXELSCOOP_NODE_VIEW_H__

