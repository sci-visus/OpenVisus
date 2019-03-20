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

  //________________________________________________________
  class Action
  {
  public:

    Time time=Time::now();

    String TypeName;

    //constructor
    Action(String TypeName_) : TypeName(TypeName_) {
    }

    //destructor
    virtual ~Action() {
    }

    //getTypeName
    String getTypeName() const {
      return TypeName;
    }

    //redo
    virtual void redo(Target* target)=0;

    //undo
    virtual void undo(Target* target)=0;

    //write
    virtual void write(Target* target,ObjectStream& ostream)  {
      ostream.writeInline("UTC",cstring(time.getUTCMilliseconds()));
    }

    //read
    virtual void read(Target* target,ObjectStream& istream) {
      this->time=Time(cint64(istream.readInline("UTC")));
    }

  };

  //_______________________________________________________
  class Transaction : public Action
  {
  public:

    std::vector< SharedPtr<Action> > actions;

    //constructor
    Transaction() : Action("Transaction"){
    }

    //destructor
    virtual ~Transaction()
    {}

    //redo
    virtual void redo(Target* target) override
    {
      target->beginUpdate();
      for (auto action : this->actions)
        action->redo(target);
      target->endUpdate();
    }

    //undo
    virtual void undo(Target* target) override
    {
      auto actions=this->actions;
      std::reverse(actions.begin(),actions.end());
      target->beginUpdate();
      for (auto action : actions)
        action->undo(target); 
      target->endUpdate();
    }

    //empty
    bool empty() const {
      return actions.empty();
    }

    //simplify
    void simplify() 
    {
      auto actions=this->actions;
      this->actions.clear();

      for (auto action : actions) 
      {
        if (auto group=std::dynamic_pointer_cast<Transaction>(action)) 
        {
          group->simplify();
          for (auto inner : group->actions)
            this->actions.push_back(inner);
        }
        else
        {
          this->actions.push_back(action);
        }
      }
    }

    //write
    virtual void write(Target* target,ObjectStream& ostream) override
    {
      Action::write(target,ostream);
      for (auto action : this->actions)
      {
        String TypeName=action->TypeName;
        ostream.pushContext(TypeName);
        action->write(target,ostream);
        ostream.popContext(TypeName);
      }
    }

    //read
    virtual void read(Target* target,ObjectStream& istream) override
    {
      Action::read(target,istream);
      this->actions.clear();
      for (int I=0;I<istream.getCurrentContext()->getNumberOfChilds();I++)
      {
        if (istream.getCurrentContext()->getChild(I).isHashNode())
          continue;

        String TypeName=istream.getCurrentContext()->getChild(I).name;
        auto action=target->createAction(TypeName); 
        VisusAssert(action);
        this->actions.push_back(action);

        istream.pushContext(TypeName);
        action->read(target,istream);
        istream.popContext(TypeName);
      }
    }

  private:

    VISUS_NON_COPYABLE_CLASS(Transaction)

  };

  //constructor
  UndoableModel() {
  }

  //destructor
  virtual ~UndoableModel() {
  }

  //createAction
  virtual SharedPtr<Action> createAction(String TypeName)=0;

  //target
  Target* target() {
    return dynamic_cast<Target*>(this);
  }

  //beginUpdate
  virtual void beginUpdate() override {
    pushAction(std::make_shared<Transaction>());
  }

  //endUpdate
   virtual void endUpdate() override  {
    popAction();
  }

  //pushAction
  void pushAction(SharedPtr<Action> action) 
  {
    if (stack.empty())
    {
      this->Model::beginUpdate();
    }
    else if (auto group = std::dynamic_pointer_cast<Transaction>(stack.top()))
    {
      action->time=Time::now();
      group->actions.push_back(action);
    }

    stack.push(action);
  }

  //popAction
  void popAction() 
  {
    auto action=stack.top();
    stack.pop();

    if (stack.empty())
    {
      addActionToHistory(action);
      this->Model::endUpdate();
    }
  }

  //clearHistory
  void clearHistory()
  {
    VisusAssert(stack.empty());
    this->history.clear();
    this->log.close();
    this->stack = std::stack< SharedPtr<Action> >();
    this->undo_redo.clear();
    this->n_undo_redo = 0;
    this->bUndoing = false;
    this->bRedoing = false;
  }

  //getHistory
  const std::vector< SharedPtr<Action> >& getHistory() const {
    return history;
  }

  //getTopAction
  SharedPtr<Action> getTopAction() const {
    return stack.empty()? SharedPtr<Action>() : stack.top();
  }

  //canUndo
  bool canUndo() const {
    return !undo_redo.empty() && n_undo_redo>0;
  }

  //undo
  bool undo() {
    VisusAssert(VisusHasMessageLock());
    VisusAssert(!bUndoing && !bRedoing);

    if (!canUndo()) 
      return false;

    auto action=undo_redo[--n_undo_redo];

    bUndoing=true;
    action->undo(target());
    bUndoing=false;

    return true;
  }

  //canRedo
  bool canRedo() const {
    return !undo_redo.empty() && n_undo_redo<undo_redo.size();
  }

  //redo
  bool redo() {
    VisusAssert(VisusHasMessageLock());
    VisusAssert(!bUndoing && !bRedoing) ;

    if (!canRedo()) 
      return false;

    auto action=undo_redo[n_undo_redo++];

    bRedoing=true;
    action->redo(target());
    bRedoing=false;

    return true;
  }

  //enableLog
  bool enableLog(String filename) 
  {
    if (log.is_open()) return true;
    log.open(filename.c_str(),std::fstream::out);
    log.rdbuf()->pubsetbuf(0,0);
    log.rdbuf()->pubsetbuf(0,0);
    return true;
  }
  
private:

  std::vector< SharedPtr<Action> > history;
  std::ofstream                    log;

  std::stack< SharedPtr<Action> >  stack;
  std::vector< SharedPtr<Action> > undo_redo;
  int                              n_undo_redo=0;
  bool                             bUndoing=false;
  bool                             bRedoing=false;

  //addActionToHistory
  void addActionToHistory(SharedPtr<Action> action) 
  {
    if (auto transaction=std::dynamic_pointer_cast<Transaction>(action))
    {
      transaction->simplify();
      if (transaction->empty())
        return;
    }

    history.push_back(action);

    if (!bUndoing && !bRedoing)
    {
      undo_redo.resize(n_undo_redo++);
      undo_redo.push_back(action);
    }

    if (log.is_open())
    {
      StringTree stree(action->getTypeName());
      ObjectStream ostream(stree, 'w');
      action->write(target(),ostream);
      ostream.close(); 
      log<<stree.toString()<<std::endl<<std::endl;
    }
  }

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


