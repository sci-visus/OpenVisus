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

#include <Visus/Gui.h>
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
class VISUS_GUI_API QueryNodeView : 
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

    QSlider* quality=nullptr;
    QDoubleSlider* accuracy = nullptr;
    
    // Export
    QSlider* end_resolution = nullptr;
    Field selected_field;
    QLineEdit* fileEdit = nullptr;
    QPushButton* saveButton = nullptr;
    QComboBox* formatComboBox = nullptr;
    QComboBox* fieldComboBox = nullptr;
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
          this->model->setProgression(widgets.progression.user.set->value());
        }));

        auto group=new QButtonGroup(layout);
        group->addButton(widgets.progression.guess);
        group->addButton(widgets.progression.noprogression);
        group->addButton(widgets.progression.user.enabled);
        group->setExclusive(true);

        connect(group, static_cast<void(QButtonGroup::*)(QAbstractButton *)>(&QButtonGroup::buttonClicked),[this](QAbstractButton *button){ 
          if (button==widgets.progression.guess) 
            this->model->setProgression(QueryGuessProgression);
          else if (button==widgets.progression.noprogression)
            this->model->setProgression(QueryNoProgression);
          else
            this->model->setProgression((QueryProgression)widgets.progression.user.set->value());
        });

        layout->addRow("Progression",sub);
      }

      //quality
      layout->addRow("Quality",GuiFactory::CreateIntegerSliderAndShowToolTip(widgets.quality,model->getQuality(),-12,+12,[this](int value){
        this->model->setQuality((QueryQuality)value);
      }));

      //accuracy
      layout->addRow("Accuracy", widgets.accuracy = GuiFactory::CreateDoubleSliderWidget(model->getAccuracy(), Range(0.0, 1.0, 0.0), [this](double value) {
        this->model->setAccuracy(value);
        }));

      //access
      {
        std::map<int,String> access_options;
        if (auto dataset=model->getDataset())
        {
          auto access_configs=dataset->getAccessConfigs();
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

  //createQuery
  SharedPtr<BoxQuery> createQuery(QueryNode* node,int end_resolution)
  {
    auto dataset = node->getDataset();

    if (!dataset)
      return SharedPtr<BoxQuery>();

    auto query_bounds = node->getQueryBounds();
    //note: point query not supported!
    VisusAssert(!(dataset->getPointDim() == 3 && query_bounds.getBoxNd().toBox3().minsize() == 0));

    auto query = dataset->createBoxQuery(node->getQueryLogicPosition().toDiscreteAxisAlignedBox(), node->getField(), node->getTime(), 'r');
    query->enableFilters();
    query->setResolutionRange(0, end_resolution);
    query->accuracy = dataset->getDefaultAccuracy();
    return query;
  }

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
    
    widgets.selected_field = dataset->getField();
    
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
    widgets.end_resolution =GuiFactory::CreateIntegerSliderWidget(std::min(24, dataset->getMaxResolution()),1,dataset->getMaxResolution(), [this,resLabel,dimsLabel,dataset,time_node](int end_resolution)
    {
        auto query= createQuery(model,end_resolution);
        if (!query) 
          return;

        dataset->beginBoxQuery(query);

        if (!query->isRunning())
          return;
        
        auto nsamples = query->getNumberOfSamples();
        resLabel->setText(cstring("Est. Size:",StringUtils::getStringFromByteSize(widgets.selected_field.dtype.getByteSize(nsamples))).c_str());
        dimsLabel->setText(cstring("[",nsamples,"]").c_str());
    });
    
    emit(widgets.end_resolution->valueChanged(std::min(24, dataset->getMaxResolution())));
    
    resRow->addWidget(widgets.end_resolution);
    resRow->addWidget(dimsLabel);
    resRow->addWidget(resLabel);
    
    layout->addRow("Resolution", resRow);
    
    auto saveRow=new QHBoxLayout();
    
    widgets.saveButton = new QPushButton("Save");
    
    connect(widgets.saveButton, &QPushButton::clicked, [this,dataset]()
    {
      int end_resolution = this->widgets.end_resolution->value();
      auto query = createQuery(model, end_resolution);
      if (!query)
        return false;

      dataset->beginBoxQuery(query);
      if (!dataset->executeBoxQuery(dataset->createAccess(), query))
        return false;

      auto nsamples = query->getNumberOfSamples();
      String filename = concatenate(widgets.fileEdit->text().toStdString(),nsamples.toString("_"),query->field.dtype.toString(),".raw");

      File data_file;
      if (data_file.createAndOpen(filename, "rw"))
      {
        if (!data_file.write(0, query->buffer.c_size(), query->buffer.c_ptr()))
        {
          PrintWarning("write error on file",filename);
          return false;
        }
      }
      else
      {
        PrintWarning("file.open",filename,"rb","failed");
      }

      QMessageBox::information(nullptr, "Success", "Data saved on disk");
      PrintInfo("Wrote data size",query->buffer.c_size(),"in raw file",filename);
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
    if (progression==QueryGuessProgression) {
      widgets.progression.guess->setChecked(true);
      widgets.progression.user.set->setEnabled(false);
    }
    else if (progression==QueryNoProgression) {
      widgets.progression.noprogression->setChecked(true);
      widgets.progression.user.set->setEnabled(false);
    }
    else {
      widgets.progression.user.enabled->setChecked(true);
      widgets.progression.user.set->setEnabled(true);
      widgets.progression.user.set->setValue(progression);
    }

    widgets.quality->setValue(model->getQuality());
    widgets.accuracy->setValue(model->getAccuracy());
  }

  //modelChanged
  virtual void modelChanged() override {
    refreshGui();
  }
  
};

} //namespace Visus

#endif  //VISUS_QUERYNODE_VIEW_H__

