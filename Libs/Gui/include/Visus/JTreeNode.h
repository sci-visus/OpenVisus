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

#ifndef VISUS_JTREE_NODE_H_
#define VISUS_JTREE_NODE_H_

#include <Visus/Gui.h>
#include <Visus/Nodes.h>
#include <Visus/DataflowNode.h>
#include <Visus/Array.h>
#include <Visus/Graph.h>
#include <Visus/GuiFactory.h>

#include <QFrame>

namespace Visus {

////////////////////////////////////////////////////////////
class VISUS_GUI_API JTreeNode : public Node
{
public:

  VISUS_NON_COPYABLE_CLASS(JTreeNode)

  // construct 
  JTreeNode(); 

  //destructor
  ~JTreeNode();

  //getMinimaTree
  bool getMinimaTree() const {
    return minima_tree;
  }
  
  //setMinimaTree
  void setMinimaTree(bool value) {
    if (minima_tree==value) return;
    setProperty("SetMinimaTree", this->minima_tree, value);
    recompute();
  }

  //getMinPersistence
  double getMinPersistence() const {
    return min_persistence;
  }

  //setMinPersistence
  void setMinPersistence(double value) {
    if (min_persistence==value) return;
    setProperty("SetMinPersistence", this->min_persistence, value);
    recompute(false);
  }

  //getReduceMinMax
  bool getReduceMinMax() const {
    return reduce_minmax;
  }

  //setReduceMinMax
  void setReduceMinMax(bool value) {
    if (reduce_minmax==value) return;
    setProperty("SetReduceMinMax", this->reduce_minmax, value);
    recompute(false);
  }

  //getThresholdMin
  double getThresholdMin() const {
    return threshold_min;
  }

  //setThresholdMin
  void setThresholdMin(double value) {
    if (threshold_min==value) return;
    setProperty("SetThresholdMin", this->threshold_min, value);
    recompute();
  }


  //getThresholdMax
  double getThresholdMax() const {
    return threshold_max;
  }

  //setThresholdMax
  void setThresholdMax(double value) {
    if (threshold_max==value) return;
    setProperty("SetThresholdMax", this->threshold_max, value);
    recompute();
  }

  //getAutoThreshold
  bool getAutoThreshold() const {
    return auto_threshold;
  }

  //setAutoThreshold
  void setAutoThreshold(bool value) {
    if (auto_threshold==value) return;
    setProperty("SetAutoThreshold", this->auto_threshold, value);
    recompute();
  }

  //processInput
  virtual bool processInput() override;

public:

  //execute
  virtual void execute(Archive& ar) override;

  //write
  virtual void write(Archive& ar) const override;

  //read
  virtual void read(Archive& ar) override;

private:

  template <typename Type> 
  class MyJob;

  template <typename Type> 
  friend class MyJob;

  //properties
  bool    minima_tree=false; // if true construct a minima tree
  double  min_persistence=21; // minimum persistence to retain nodes
  bool    reduce_minmax=false; // whether or not to reduce min-max edges if they are shorter than min_persist
  double  threshold_min=0;// (just dynamic_pointer_cast this to dtype)
  double  threshold_max=0;
  bool    auto_threshold=true; // set threshold automatically

  //the data on which I need to do jtree calculation
  Array  data;
  
  double minThresholdRange=-1;
  double maxThresholdRange=+1;

  SharedPtr<BaseGraph> last_full_graph;

  //update automatic threshold
  void updateAutoThreshold();

  //messageHasBeenPublished
  virtual void messageHasBeenPublished(DataflowMessage msg) override;

  //recompute
  bool recompute(bool bFull=true);

};


/////////////////////////////////////////////////////////
class VISUS_GUI_API JTreeNodeView :
  public QFrame,
  public View<JTreeNode>
{
public:

  VISUS_NON_COPYABLE_CLASS(JTreeNodeView)

    //constructor
    JTreeNodeView(JTreeNode* model = nullptr) {
    if (model)
      bindModel(model);
  }

  //destructor
  virtual ~JTreeNodeView() {
    bindModel(nullptr);
  }

  //bindModel
  virtual void bindModel(JTreeNode* value) override
  {
    if (this->model)
    {
      QUtils::clearQWidget(this);
      widgets = Widgets();
    }

    View<ModelClass>::bindModel(value);

    if (this->model)
    {
      QFormLayout* layout = new QFormLayout();

      layout->addRow("minima_tree", widgets.minima_tree = GuiFactory::CreateCheckBox(model->getMinimaTree(), "", [this](int value) {
        model->setMinimaTree(value);
      }));

      layout->addRow("min_persistence", widgets.min_persistence = GuiFactory::CreateDoubleTextBoxWidget(model->getMinPersistence(), [this](double value) {
        model->setMinPersistence(value);
      }));

      layout->addRow("reduce_minmax", widgets.reduce_minmax = GuiFactory::CreateCheckBox(model->getReduceMinMax(), "", [this](double value) {
        model->setReduceMinMax(value);
      }));


      layout->addRow("threshold_min", widgets.threshold_min = GuiFactory::CreateDoubleTextBoxWidget(model->getThresholdMin(), [this](double value) {
        model->setThresholdMin(value);
      }));


      layout->addRow("threshold_max", widgets.threshold_max = GuiFactory::CreateDoubleTextBoxWidget(model->getThresholdMax(), [this](double value) {
        model->setThresholdMax(value);
      }));


      layout->addRow("auto_threshold", widgets.auto_threshold = GuiFactory::CreateCheckBox(model->getAutoThreshold(), "", [this](int value) {
        model->setAutoThreshold(value);
      }));


      setLayout(layout);
      refreshGui();
    }
  }

private:

  class Widgets
  {
  public:
    QCheckBox* minima_tree = nullptr;
    QLineEdit* min_persistence = nullptr;
    QCheckBox* reduce_minmax = nullptr;
    QLineEdit* threshold_min = nullptr;
    QLineEdit* threshold_max = nullptr;
    QCheckBox* auto_threshold = nullptr;
  };

  Widgets widgets;

  ///refreshGui
  void refreshGui()
  {
    widgets.minima_tree->setChecked(model->getMinimaTree());
    widgets.min_persistence->setText(cstring(model->getMinPersistence()).c_str());
    widgets.reduce_minmax->setChecked(model->getReduceMinMax());
    widgets.threshold_min->setText(cstring(model->getThresholdMin()).c_str());
    widgets.threshold_max->setText(cstring(model->getThresholdMax()).c_str());
    widgets.auto_threshold->setChecked(model->getAutoThreshold());
  }

  //modelChanged
  virtual void modelChanged() override {
    refreshGui();
  }

};

} //namespace Visus

#endif // VISUS_JTREE_NODE_H_
