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

#ifndef VISUS_DATASETNODE_VIEW_H__
#define VISUS_DATASETNODE_VIEW_H__

#include <Visus/Gui.h>
#include <Visus/Model.h>
#include <Visus/DatasetNode.h>
#include <Visus/GuiFactory.h>


#include <QTextEdit>
#include <QFontDatabase>
#include <QTextEdit>

namespace Visus {

/////////////////////////////////////////////////////////
class VISUS_GUI_API DatasetNodeView : 
  public QFrame, 
  public View<DatasetNode>
{
public:

  VISUS_NON_COPYABLE_CLASS(DatasetNodeView)

  //constructor
  DatasetNodeView(DatasetNode* model) {
    bindModel(model);
  }

  //destructor
  virtual ~DatasetNodeView() {
    bindModel(nullptr);
  }

  //bindModel
  virtual void bindModel(DatasetNode* model) override 
  {
    if (this->model)
    {
      QUtils::clearQWidget(this);
    }

    View<ModelClass>::bindModel(model);

    if (this->model)
    {
      auto textedit=GuiFactory::CreateTextEdit();

      if (auto dataset=model->getDataset())
        textedit->setText(dataset->getDatasetBody().toString().c_str());

      auto layout=new QVBoxLayout();
      layout->addWidget(GuiFactory::CreateCheckBox(model->showBounds(),"Show bounds",[model](int value){model->setShowBounds(value);}));
      layout->addWidget(textedit);
      setLayout(layout);
    }
  }

};

} //namespace Visus

#endif  //VISUS_DATASETNODE_VIEW_H__

