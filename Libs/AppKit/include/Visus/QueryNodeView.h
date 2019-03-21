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

#ifndef VISUS_QUERYNODE_VIEW_H__
#define VISUS_QUERYNODE_VIEW_H__

#include <Visus/AppKit.h>
#include <Visus/QueryNode.h>
#include <Visus/Model.h>
#include <Visus/GuiFactory.h>

#include <QFrame>
#include <QComboBox>
#include <QFormLayout>
#include <QButtonGroup>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

#include <sstream>

namespace Visus {

/////////////////////////////////////////////////////////
class VISUS_APPKIT_API QueryNodeView : 
  public QFrame, 
  public View<QueryNode>
{
public:

  VISUS_NON_COPYABLE_CLASS(QueryNodeView)
  
  //________________________________________
  class Widgets
  {
  public:

    QComboBox* accessindex=nullptr;
    QCheckBox* enable_viewdep=nullptr;

    struct
    {
      QCheckBox* guess=nullptr;
      QCheckBox* noprogression=nullptr;
      struct
      {
        QCheckBox* enabled=nullptr;
        QSlider*   set=nullptr;
      }
      user;
    }
    progression;

    QSlider* quality;
    
    // Export
    QSlider* resolution;
    Field selected_field;
    QLineEdit* fileEdit;
    QPushButton* saveButton;
    QComboBox* formatComboBox;
    QComboBox* fieldComboBox;
  };



  Widgets widgets;

  //constructor
  QueryNodeView(QueryNode* model) {
    bindModel(model);
  }

  //destructor
  virtual ~QueryNodeView() {
    bindModel(nullptr);
  }

  //bindModel
  virtual void bindModel(QueryNode* model) override
  {
    if (this->model)
    {
      QUtils::clearQWidget(this);
      widgets=Widgets();
    }

    View<ModelClass>::bindModel(model);

    if (this->model)
    {
      QFormLayout* layout=new QFormLayout();
      auto tabs=new QTabWidget();
      
      //viewdep
      {
        layout->addRow("Enable viewdep",widgets.enable_viewdep=GuiFactory::CreateCheckBox(model->isViewDependentEnabled(),"",[this](int value){
          this->model->setViewDependentEnabled(value?true:false);
        }));
      }

      //progression
      {
        auto sub=new QVBoxLayout();
        sub->addWidget(widgets.progression.guess=GuiFactory::CreateCheckBox(true,"Guess"));
        sub->addWidget(widgets.progression.noprogression=GuiFactory::CreateCheckBox(false,"No Progression"));
        sub->addWidget(widgets.progression.user.enabled=GuiFactory::CreateCheckBox(false,"User value"));
        sub->addLayout(GuiFactory::CreateIntegerSliderAndShowToolTip(widgets.progression.user.set,6,1,32,[this](int value) {
          this->model->setProgression((Query::Progression)widgets.progression.user.set->value());
        }));

        auto group=new QButtonGroup(layout);
        group->addButton(widgets.progression.guess);
        group->addButton(widgets.progression.noprogression);
        group->addButton(widgets.progression.user.enabled);
        group->setExclusive(true);

        connect(group, static_cast<void(QButtonGroup::*)(QAbstractButton *)>(&QButtonGroup::buttonClicked),[this](QAbstractButton *button){ 
          if (button==widgets.progression.guess) 
            this->model->setProgression(Query::GuessProgression);
          else if (button==widgets.progression.noprogression)
            this->model->setProgression(Query::NoProgression);
          else
            this->model->setProgression((Query::Progression)widgets.progression.user.set->value());
        });

        layout->addRow("Progression",sub);
      }

      //quality
      layout->addRow("Quality",GuiFactory::CreateIntegerSliderAndShowToolTip(widgets.quality,model->getQuality(),-12,+12,[this](int value){
        this->model->setQuality((Query::Quality)value);
      }));

      //access
      {
        std::map<int,String> access_options;
        if (auto dataset=model->getDataset())
        {
          std::vector<StringTree*> access_configs=dataset->getAccessConfigs();
          for (int I=0;I<(int)access_configs.size();I++)
          {
            String name=access_configs[I]->readString("name","Access["+cstring(I)+"]");
            access_options[I]=name;
          }
        }

        layout->addRow("Access" ,widgets.accessindex=GuiFactory::CreateIntegerComboBoxWidget(model->getAccessIndex(),access_options,[this](int index) {
          this->model->setAccessIndex(index);
        }));
      }
      
      auto ret=new QFrame();
      ret->setLayout(layout);

      tabs->addTab(ret,"Query settings");
      tabs->addTab(createExportWidget(),"Export");
      
      auto glayout=new QVBoxLayout();
      glayout->addWidget(tabs);
      setLayout(glayout);
      //setLayout(layout);
      refreshGui();
    }
  }

private:

  //createExportWidget
  QWidget* createExportWidget()
  {
    auto layout=new QFormLayout();
    
    auto fileRow=new QHBoxLayout();
    
    widgets.fileEdit=new QLineEdit();
    
    QPushButton* browseButton=new QPushButton("&Select file...");
    connect(browseButton, &QPushButton::clicked, [this]()
            {
              widgets.fileEdit->setText(QString(QFileDialog::getSaveFileName(nullptr,"Choose a file to save...")));
            });
    
    fileRow->addWidget(widgets.fileEdit);
    fileRow->addWidget(browseButton);
    
    auto dataset=this->model->getDataset();
    auto dataset_node=this->model->getDatasetNode();
    auto time_node=dataset_node->findChild<TimeNode*>();
    
    widgets.selected_field = dataset->getDefaultField();
    
    widgets.fieldComboBox=new QComboBox();
    std::vector<Field> fields=dataset->getFields();
    
    for (int I=0;I<(int)fields.size();I++)
    {
      widgets.fieldComboBox->addItem(fields[I].name.c_str());
    }
   
    connect(widgets.fieldComboBox,static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged),[this, fields](const QString& value)
            {
              for(auto& f: fields)
                if(QString(f.name.c_str()).compare(value)==0)
                  widgets.selected_field = f;
            });
    
    fileRow->addWidget(widgets.fieldComboBox);
    layout->addRow("Output file", fileRow);
    
    auto resRow=new QHBoxLayout();
    
    QLabel* resLabel=new QLabel("Est. Size: ");
    resLabel->setFixedWidth(150);
    QLabel* dimsLabel=new QLabel();
    dimsLabel->setFixedWidth(100);
    
    // TODO make a slot to update on query change
    widgets.resolution=GuiFactory::CreateIntegerSliderWidget(std::min(24, dataset->getMaxResolution()),1,dataset->getMaxResolution(), [this,resLabel,dimsLabel,dataset,time_node](int value)
    {
        auto access=dataset->createAccess();
        auto query=std::make_shared<Query>(dataset.get(),'r');
        query->field= dataset->getFieldByName(widgets.selected_field.name);
        query->position=this->model->getQueryPosition();
        query->time=time_node->getCurrentTime();
        query->end_resolutions.push_back(value);

        VisusReleaseAssert(dataset->beginQuery(query));
      
        auto num_samples = query->nsamples.innerProduct();
        std::stringstream ss;
        ss<<"Est. Size:  ";
        ss.precision(3);
        ss<<std::fixed<<widgets.selected_field.dtype.getByteSize(query->nsamples)/1000000.0;
        ss<<"MB";
        resLabel->setText(ss.str().c_str());
      
        std::stringstream ss_dims;
        ss_dims<<"["<<query->nsamples[0];
        for(int I=1; I<dataset->getPointDim(); I++)
          ss_dims<<"x"<<query->nsamples[I];
        ss_dims<<"]";
        dimsLabel->setText(ss_dims.str().c_str());
    
    });
    
    emit(widgets.resolution->valueChanged(std::min(24, dataset->getMaxResolution())));
    
    resRow->addWidget(widgets.resolution);
    resRow->addWidget(dimsLabel);
    resRow->addWidget(resLabel);
    
    layout->addRow("Resolution", resRow);
    
    auto saveRow=new QHBoxLayout();
    
    widgets.saveButton = new QPushButton("Save");
    
    connect(widgets.saveButton, &QPushButton::clicked, [this,dataset,time_node]()
    {
      int res_value = this->widgets.resolution->value();

      auto access = dataset->createAccess();
      auto query = std::make_shared<Query>(dataset.get(), 'r');
      query->field = dataset->getFieldByName(widgets.selected_field.name);
      query->position = this->model->getQueryPosition();
      query->time = time_node->getCurrentTime();
      query->end_resolutions.push_back(res_value);
      VisusReleaseAssert(dataset->beginQuery(query));

      VisusInfo() << "position " << query->position.toString();

      unsigned char* buffer = (unsigned char*)query->buffer.c_ptr();
      //read data
      VisusReleaseAssert(dataset->executeQuery(access, query));

      std::stringstream filename;
      filename << widgets.fileEdit->text().toStdString();
      for (int I = 0; I < dataset->getPointDim(); I++)
        filename << "_" << query->nsamples[I];
      filename << query->field.dtype.toString();
      //filename << "_t_" << query->time;
      //filename << "_f_" << query->field.name;
      filename << ".raw";

      File data_file;
      if (data_file.createAndOpen(filename.str(),"rw"))
      {
        if (!data_file.write(0, query->buffer.c_size(), query->buffer.c_ptr()))
        {
          VisusWarning() << "write error on file " << filename.str();
          return false;
        }

      }
      else
      {
        VisusWarning() << "file.open(" << filename.str() << ",\"rb\") failed";
      }

      QMessageBox::information(nullptr, "Success", "Data saved on disk");

      VisusInfo() << "Wrote data size " << query->buffer.c_size() << " in raw file " << filename.str();

      return true;
    });

    widgets.formatComboBox=new QComboBox();
    widgets.formatComboBox->addItem("RAW");
    widgets.formatComboBox->addItem("IDX");
    
    // TODO enable when/if implemented IDX or other export formats
    widgets.formatComboBox->setEnabled(false);
    
    saveRow->addWidget(widgets.formatComboBox);
    saveRow->addWidget(widgets.saveButton);
    
    layout->addRow("Format", saveRow);
    
    auto ret=new QFrame();
    ret->setLayout(layout);
    return ret;
  }

  //refreshGui
  void refreshGui() 
  {
    widgets.accessindex->setCurrentIndex(model->getAccessIndex());
    widgets.enable_viewdep->setChecked(model->isViewDependentEnabled());

    int progression=model->getProgression();
    if (progression==Query::GuessProgression) {
      widgets.progression.guess->setChecked(true);
      widgets.progression.user.set->setEnabled(false);
    }
    else if (progression==Query::NoProgression) {
      widgets.progression.noprogression->setChecked(true);
      widgets.progression.user.set->setEnabled(false);
    }
    else {
      widgets.progression.user.enabled->setChecked(true);
      widgets.progression.user.set->setEnabled(true);
      widgets.progression.user.set->setValue(progression);
    }

    widgets.quality->setValue(model->getQuality());
  }

  //modelChanged
  virtual void modelChanged() override {
    refreshGui();
  }
  
};

} //namespace Visus

#endif  //VISUS_QUERYNODE_VIEW_H__

