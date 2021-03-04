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

#ifndef VISUS_FIELD_NODE_VIEW_H__
#define VISUS_FIELD_NODE_VIEW_H__

#include <Visus/Gui.h>
#include <Visus/FieldNode.h>
#include <Visus/Model.h>
#include <Visus/Gui.h>
#include <Visus/IdxMultipleDataset.h>

#include <QFrame>
#include <QLabel>
#include <QTextEdit>
#include <QComboBox>
#include <QFontDatabase>
#include <QBoxLayout>

namespace Visus {

///////////////////////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API FieldNodeView : 
  public QFrame, 
  public View<FieldNode> 
{
public:

  VISUS_NON_COPYABLE_CLASS(FieldNodeView)

  //____________________________________________
  class Widgets
  {
  public:
    QComboBox*          cmbPresets=nullptr;
    QTextEdit*          txtCode=nullptr;
    QTextEdit*          txtOutput=nullptr;
    QToolButton*        btnEval=nullptr;
  };

  Widgets widgets;

  //constructor
  FieldNodeView(FieldNode* model,SharedPtr<Dataset> dataset=SharedPtr<Dataset>()) 
  {
    //find out the dataset
    if (!dataset) {
      if (auto query = model->getOutputPort("fieldname")->findFirstConnectedOutputOfType<QueryNode*>()) {
        if (auto dataset_node = query->getInputPort("dataset")->findFirstConnectedInputOfType<DatasetNode*>()) {
          dataset = dataset_node->getDataset();
        }
      }
    }

    this->dataset = dataset;

    if (model)
      bindModel(model);
  }

  //destructor
  virtual ~FieldNodeView() {
    bindModel(nullptr);
  }

  //bindModel
  virtual void bindModel(FieldNode* model) override
  {
    //if already bound, just refresh
    if (this->model && this->model==model)
    {
      refreshGui();
      return;
    }

    if (this->model)
    {
      QUtils::clearQWidget(this);
      widgets=Widgets();
    }

    View<ModelClass>::bindModel(model);

    if (this->model)
    {
      widgets.cmbPresets = new QComboBox();
      widgets.cmbPresets->setEditable(false);
      connect(widgets.cmbPresets,static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged),[this](const QString& value) 
      {
        std::ostringstream out;
        out<<presets.getValue(cstring(value));
        widgets.txtCode->setText(out.str().c_str());
      });

      QVBoxLayout* layout=new QVBoxLayout();
      widgets.txtCode= GuiFactory::CreateTextEdit();

      widgets.btnEval=GuiFactory::CreateButton("Eval",[this](bool){
        String fieldname=cstring(widgets.txtCode->toPlainText());
        this->model->setFieldName(fieldname);
      });

      widgets.txtOutput= GuiFactory::CreateTextEdit();

      layout->addWidget(new QLabel("Input"));
      layout->addWidget(widgets.cmbPresets);
      layout->addWidget(widgets.txtCode);
      layout->addWidget(widgets.btnEval);

      layout->addWidget(new QLabel("Output"));
      layout->addWidget(widgets.txtOutput);

      setLayout(layout);
      refreshGui();
    }
  }

private:

  StringMap                  presets;
  SharedPtr<Dataset>         dataset;

  //refreshGui
  void refreshGui()
  {
    String fieldname=model->getFieldName();

    widgets.cmbPresets->clear();

    if (dataset)
    {
      std::vector<Field> fields=dataset->getFields();

      for (int I=0;I<(int)fields.size();I++)
      {
        String key=fields[I].getDescription();
        String code=fields[I].name;
        presets.setValue(key,code);
        widgets.cmbPresets->addItem(key.c_str());
      }
    }

    widgets.txtCode->setText(fieldname.c_str());

    String msg;
    try
    {
      VisusAssert(dataset);
      Field field=dataset->getFieldEx(fieldname);
      msg=cstring("DTYPE",field.dtype);
    }
    catch (const std::exception& ex)
    {
      msg=String("Error\n") + ex.what();
    }

    widgets.txtOutput->clear();
    widgets.txtOutput->setTextColor(QUtils::convert<QColor>(Colors::Red));
    widgets.txtOutput->append(cstring("[",Time::now().getFormattedLocalTime(),"]").c_str());
    widgets.txtOutput->setTextColor(QUtils::convert<QColor>(Colors::Red));
    widgets.txtOutput->append(msg.c_str());
  }

  //modelChanged
  virtual void modelChanged() override
  {
    refreshGui();
  }

};


} //namespace Visus

#endif  //VISUS_FIELD_NODE_VIEW_H__

