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

#ifndef VISUS_MODEL_H__
#define VISUS_MODEL_H__

#include <Visus/Kernel.h>
#include <Visus/Time.h>
#include <Visus/StringUtils.h>
#include <Visus/Log.h>
#include <Visus/SignalSlot.h>
#include <Visus/ApplicationInfo.h>

#include <stack>
#include <fstream>

namespace Visus {

//////////////////////////////////////////////////////////
class VISUS_KERNEL_API BaseView
{
public:

  VISUS_CLASS(BaseView)

  virtual ~BaseView() {
  }
};

//////////////////////////////////////////////////////////
class VISUS_KERNEL_API Model 
{
public:

  VISUS_NON_COPYABLE_CLASS(Model)

  Signal<void()> begin_update;

  //end_update
  Signal<void()> end_update;

  //destroyed
  Signal<void()> destroyed;

  //views
  std::vector<BaseView*> views;

  //constructor
  Model();

  //destructor
  virtual ~Model();

  //getTypeName
  virtual String getTypeName() const  = 0;

public:

  //enableLog
  bool enableLog(String filename);

  //clearHistory
  void clearHistory();

  //getHistory
  const StringTree& getHistory() const {
    return history;
  }
  
public:

  //isUpdating
  inline bool isUpdating() const {
    return redos.size() > 0;
  }

  //topRedo
  StringTree& topRedo() {
    return redos.top();
  }

  //topUndo
  StringTree& topUndo() {
    return undos.top();
  }

  //beginUpdate
  void beginUpdate(StringTree redo, StringTree undo);

  //endUpdate
  void endUpdate();

  //in case you do not want to use actions
  void beginUpdate()
  {
    //note only the root action is important
    beginUpdate(
      StringTree("DiffAction").addChild(isUpdating()? StringTree() : this->encode()),
      StringTree("DiffAction"));
  }

  //beginTransaction
  void beginTransaction() {
    beginUpdate(
      StringTree("Transaction"),
      StringTree("Transaction"));
  }

  //endTransaction
  void endTransaction() {
    endUpdate();
  }

public:

  //setProperty
  template <typename Value>
  void setProperty(String name, Value& old_value, const Value& new_value)
  {
    if (old_value == new_value)
      return;
    
    beginUpdate(
      StringTree("SetProperty").write("name", name).write("value", new_value),
      StringTree("SetProperty").write("name", name).write("value", old_value));
    {
      old_value = new_value;
    }
    endUpdate();
  }


  //setObjectProperty
  template <typename Value>
  void setObjectProperty(String name, Value& old_value, const Value& new_value)
  {
    if (old_value == new_value)
      return;

    beginUpdate(
      StringTree("SetProperty").write("name", name).addChild(Encode(new_value, "Value")),
      StringTree("SetProperty").write("name", name).addChild(Encode(old_value, "Value")));
    {
      old_value = new_value;
    }
    endUpdate();
  }


public:

    //executeAction
  virtual void executeAction(StringTree action);

public:

  //canRedo
  bool canRedo() const {
    return !undo_redo.empty() && n_undo_redo < undo_redo.size();
  }

  //canUndo
  bool canUndo() const {
    return !undo_redo.empty() && n_undo_redo > 0;
  }

  //redo
  bool redo();

  //undo
  bool undo();

public:

  //addView
  void addView(BaseView* value) {
    this->views.push_back(value);
  }

  //removeView
  void removeView(BaseView* value) {
    Utils::remove(this->views,value);
  }

public:

  //writeTo
  virtual void writeTo(StringTree& out) const  = 0;

  //readFrom
  virtual void readFrom(StringTree& in) = 0;

protected:

  //fullUndo
  StringTree fullUndo() {
    return this->encode("Assign");
  }

  //modelChanged
  virtual void modelChanged() {
  }

public:

  //encode
  StringTree encode(String name = "") const {
    if (name.empty()) name = getTypeName();
    return Encode(*this, name);
  }

private:

  typedef std::pair<StringTree, StringTree> UndoRedo;

  StringTree              history;
  std::ofstream           log;
  std::stack<StringTree>  redos;
  std::stack<StringTree>  undos;
  std::vector<UndoRedo>   undo_redo;
  int                     n_undo_redo = 0;
  bool                    bUndoingRedoing = false;

};


//////////////////////////////////////////////////////////
template <class ModelClassArg>
class View : public virtual BaseView
{
public:

  typedef ModelClassArg ModelClass;

  //constructor
  View() : model(nullptr)
  {}

  //destructor
  virtual ~View()
  {
    //did you forget to call bindModel(nullptr)?
    VisusAssert(model==nullptr);
    this->View::bindModel(nullptr);
  }

  //getModel
  inline ModelClass* getModel() const {
    return model;
  }

  //bindViewModel
  virtual void bindModel(ModelClass* value)
  {
    if (value==this->model) return;

    if (this->model) 
    {
      this->model->removeView(this);
      this->model->end_update.disconnect(changed_slot);
      this->model->Model::destroyed.disconnect(destroyed_slot);
    }

    this->model=value; 

    if (this->model) 
    {
      this->model->end_update.connect(changed_slot=Slot<void()>([this] {
        modelChanged();
      }));

      this->model->Model::destroyed.connect(destroyed_slot=Slot<void()>([this] {
        bindModel(nullptr);
      }));

      this->model->addView(this);
    }
  }

  //rebindModel
  inline void rebindModel()
  {
    ModelClass* model=this->model;
    bindModel(nullptr);
    bindModel(model);
  }

  //modelChanged
  virtual void modelChanged(){
  }

protected:

  ModelClass* model;

private:

  VISUS_NON_COPYABLE_CLASS(View)

  Slot<void()> changed_slot;
  Slot<void()> destroyed_slot;

};

} //namespace Visus

#endif //VISUS_MODEL_H__


