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

  //changed
  Signal<void()> changed;

  //destroyed
  Signal<void()> destroyed;

  //views
  std::vector<BaseView*> views;

  //constructor
  Model() {
  }

  //destructor
  virtual ~Model() {
    destroyed.emitSignal();
    VisusAssert(destroyed.empty());
  }

  //getTypeName
  virtual String getTypeName() const = 0;

  //isUpdating
  inline bool isUpdating() const {
    return nbegin>0;
  }

  //beginUpdate
  virtual void beginUpdate() 
  {
    if (!nbegin++)
      begin_update.emitSignal();
  }

  //endUpdate
  virtual void endUpdate() 
  {
    if (!--nbegin) 
    {
      modelChanged();
      this->changed.emitSignal();
    }
  }

  template <typename Value>
  void setProperty(Value& dst, Value src) {
    beginUpdate();
    dst = src;
    endUpdate();
  }

  //addView
  void addView(BaseView* value) {
    this->views.push_back(value);
  }

  //removeView
  void removeView(BaseView* value) {
    Utils::remove(this->views,value);
  }

public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) = 0;

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) = 0;

protected:

  //modelChanged
  virtual void modelChanged() {
  }

private:

  int nbegin=0;

};


////////////////////////////////////////////////////////////////
template <class Target>
class UndoableModel : public Model
{
public:

  //constructor
  UndoableModel() {
  }

  //destructor
  virtual ~UndoableModel() {
  }

  //target
  Target* target() {
    return dynamic_cast<Target*>(this);
  }

  //enableLog
  bool enableLog(String filename)
  {
    if (log.is_open()) return true;
    log.open(filename.c_str(), std::fstream::out);
    log.rdbuf()->pubsetbuf(0, 0);
    log.rdbuf()->pubsetbuf(0, 0);
    return true;
  }

  //clearHistory
  void clearHistory()
  {
    this->history.clear();
    this->log.close();
    this->redos = std::stack<StringTree>();
    this->undos = std::stack<StringTree>();
    this->undo_redo.clear();
    this->n_undo_redo = 0;
    this->bUndoing = false;
    this->bRedoing = false;
  }

  //getHistory
  const std::vector<StringTree>& getHistory() const {
    return history;
  }

public:

  //topRedo
  StringTree& topRedo() {
    return redos.top();
  }

  //topUndo
  StringTree& topUndo() {
    return undos.top();
  }

  //pushAction
  void pushAction(StringTree redo,StringTree undo) 
  {
    auto action = std::make_pair(redo,undo);

    if (redos.empty())
    {
      this->Model::beginUpdate();
    }
    else
    {
      topRedo().childs.insert(topRedo().childs.end  (), std::make_shared<StringTree>(redo)); //at the end 
      topUndo().childs.insert(topUndo().childs.begin(), std::make_shared<StringTree>(undo)); //at the beginning
    }

    redos.push(redo);
    undos.push(undo);
  }

  //popAction
  void popAction() 
  {
    auto redo = topRedo(); redos.pop();
    auto undo = topUndo(); undos.pop();

    if (redos.empty())
    {
      history.push_back(redo);

      if (!bUndoing && !bRedoing)
      {
        undo_redo.resize(n_undo_redo++);
        undo_redo.push_back(std::make_pair(redo,undo));
      }

      if (log.is_open())
        log << redo.toString() << std::endl << std::endl;

      this->Model::endUpdate();
    }
  }

public:

  //canRedo
  bool canRedo() const {
    return !undo_redo.empty() && n_undo_redo < undo_redo.size();
  }

  //canUndo
  bool canUndo() const {
    return !undo_redo.empty() && n_undo_redo>0;
  }

  //redo
  bool redo() {
    VisusAssert(VisusHasMessageLock());
    VisusAssert(!bUndoing && !bRedoing);
    if (!canRedo())  return false;
    auto action = undo_redo[n_undo_redo++].first;
    bRedoing = true;
    target()->executeAction(action);
    bRedoing = false;
    return true;
  }

  //undo
  bool undo() {
    VisusAssert(VisusHasMessageLock());
    VisusAssert(!bUndoing && !bRedoing);
    if (!canUndo()) return false;
    auto action=undo_redo[--n_undo_redo].second;
    bUndoing=true;
    target()->executeAction(action);
    bUndoing=false;
    return true;
  }

private:

  typedef std::pair<StringTree, StringTree> UndoRedo;

  std::vector<StringTree> history;
  std::ofstream           log;
  std::stack<StringTree>  redos;
  std::stack<StringTree>  undos;
  std::vector<UndoRedo>   undo_redo;
  int                     n_undo_redo=0;
  bool                    bUndoing=false;
  bool                    bRedoing=false;

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
      this->model->changed.disconnect(changed_slot);
      this->model->Model::destroyed.disconnect(destroyed_slot);
    }

    this->model=value; 

    if (this->model) 
    {
      this->model->changed.connect(changed_slot=Slot<void()>([this] {
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


