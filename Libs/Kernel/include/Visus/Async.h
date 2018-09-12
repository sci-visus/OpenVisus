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

#ifndef _VISUS_ASYNC_H__
#define _VISUS_ASYNC_H__

#include <Visus/Kernel.h>
#include <Visus/Semaphore.h>

#include <list>

namespace Visus {


//using custom class because C++11 cannot wait for multiple future 
//see http://stackoverflow.com/questions/19225372/waiting-for-multiple-futures

///////////////////////////////////////////////////////////
template <typename __Value__>
class BasePromise
{
public:

  typedef __Value__ Value;

  CriticalSection  lock;
  SharedPtr<Value> value;

  typedef std::function<void(Value)> Callback;

  //constructor
  BasePromise() {
  }

  //set_value
  void set_value(const Value& value)
  {
    std::vector<Callback> callbacks;
    {
      ScopedLock lock(this->lock);
      VisusAssert(!this->value);
      this->value = std::make_shared<Value>(value);
      callbacks =this->callbacks;
      this->callbacks.clear(); //one shot
    }

    for (auto fn : callbacks)
      fn(value);
  }

  //addWhenDoneListener (must have the lock and value must not exists)
  void addWhenDoneListener(Callback fn) {
    VisusAssert(!value);
    callbacks.push_back(fn);
  }

  //isDone
  bool is_ready() const {
    ScopedLock lock(const_cast<BasePromise*>(this)->lock);
    return value? true : false;
  }

  //when_ready
  void when_ready(Callback fn) {
    this->lock.lock();
    if (value) {
      this->lock.unlock();
      fn(*value);
    }
    else
    {
      addWhenDoneListener(fn);
      this->lock.unlock();
    }
  }

private:

  VISUS_NON_COPYABLE_CLASS(BasePromise)

  std::vector< Callback > callbacks;

};

///////////////////////////////////////////////////////////
template <typename __Value__>
class Future 
{
public:

  typedef __Value__ Value;

  //default constructor
  Future() {
  }

  //constructor
  Future(SharedPtr< BasePromise<Value> > promise_) : promise(promise_) {
  }

  //copy constructor
  Future(const Future& other) : Future(other.promise) {
  }

  //constructor
  ~Future() {
  }

  //operator=
  Future& operator=(const Future& other) {
    this->promise=other.promise;
    return *this;
  }

  //get
  Value get() const
  {
    ScopedLock lock(promise->lock);
    
    //need to wait?
    if (!promise->value)
    {
      promise->addWhenDoneListener([this](Value){
        const_cast<Future*>(this)->ready.up();
      });
      promise->lock.unlock();
      const_cast<Future*>(this)->ready.down();
      promise->lock.lock();
      VisusAssert(promise->value);
    }

    return *(promise->value);
  }

  //get_promise
  SharedPtr< BasePromise<Value> > get_promise() const {
    return promise;
  }

  //isDone
  bool is_ready() const {
    if (!promise) { VisusAssert(false); return false;}
    return promise->is_ready();
  }

  //when_ready
  void when_ready(typename BasePromise<Value>::Callback fn) {
    if (!promise) {
      VisusAssert(false);
      return;
    }
     return promise->when_ready(fn);
  }

private:

  SharedPtr< BasePromise<Value> > promise;
  Semaphore                       ready;

};

///////////////////////////////////////////////////////////
template <typename __Value__>
class Promise
{
public:

  typedef __Value__ Value;

  //constructor
  Promise() {
  }

  //constructor
  Promise(const Value& value) {
    set_value(value);
  }

  //set_value
  void set_value(const Value& value) {
    base_promise->set_value(value);
  }

  //get_future
  Future<Value> get_future() {
    return Future<Value>(base_promise);
  }

  //when_ready
  void when_ready(typename BasePromise<Value>::Callback fn) {
    base_promise->when_ready(fn);
  }

private:

  SharedPtr< BasePromise<Value> > base_promise =std::make_shared< BasePromise<Value> >() ;

}; 


///////////////////////////////////////////////////////////
template <typename Future,typename Etc>
class WaitAsync 
{
public:

  typedef typename Future::Value Value;

  //constructor
  WaitAsync()  {
  }

  //destructor
  ~WaitAsync() {
    VisusAssert(ninside==0);
  }

  //disableReadyInfo
  void disableReadyInfo() {
    bDisableReadyInfo=true;
  }

  //size
  int size() const
  {
    ScopedLock lock(const_cast<WaitAsync*>(this)->lock);
    return ninside;
  }

  //empty
  bool empty() const {
    return size() == 0;
  }

  //pushRunning
  void pushRunning(Future future, Etc running_info)
  {
    auto promise = future.get_promise();
    VisusAssert(promise);

    ScopedLock promise_lock(promise->lock);
    {
      ScopedLock this_lock(this->lock);

      ++ninside;

      auto running_item = std::make_pair(future, running_info);
      auto ready_item   = std::make_pair(future, bDisableReadyInfo? Etc() : running_info);

      //no need to wait
      if (promise->value)
      {
        ready.push_front(ready_item);
        nready.up();
      }
      //need to wait, as soon as it becomes available I'm moving it to ready deque
      else
      {
        auto it=running.insert(running.end(), running_item);
        promise->addWhenDoneListener(typename BasePromise<Value>::Callback([this, it, ready_item](Value)
        {
          ScopedLock this_lock(this->lock);
          ready.push_front(ready_item);
          running.erase(it);
          nready.up();
        }));
      }
    }
  }

  //popReady
  std::pair<Future, Etc> popReady()
  {
    nready.down();
    {
      ScopedLock lock(this->lock);
      VisusAssert(!ready.empty());
      auto ret = ready.back();
      ready.pop_back();
      --ninside;
      return ret;
    }
  }

  //get_running
  std::list< std::pair<Future, Etc> > getRunning() {
    ScopedLock lock(this->lock);
    return running;
  }

  //waitAllDone
  void waitAllDone() 
  {
    for (int I=0,N=size();I<N;I++)
      popReady();
  }

private:

  VISUS_NON_COPYABLE_CLASS(WaitAsync)

  typedef std::list< std::pair< Future, Etc > > Running;
  typedef std::deque< std::pair<Future, Etc> >  Ready;

  CriticalSection  lock;
  int              ninside = 0;
  Running          running;
  Semaphore        nready;
  Ready            ready;
  bool             bDisableReadyInfo = false;

};


} //namespace Visus

#endif //_VISUS_ASYNC_H__

