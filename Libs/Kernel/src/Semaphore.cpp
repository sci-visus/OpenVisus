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

#include <Visus/Semaphore.h>
#include <Visus/Exception.h>


#if WIN32
#include <Windows.h>

#elif __APPLE__
#include <dispatch/dispatch.h>

#else
#include <semaphore.h>
#include <errno.h>

#endif

namespace Visus {

#if WIN32
class Semaphore::Pimpl
{
public:
  HANDLE handle;

 //constructor
  Pimpl(int initial_value)
  {handle=CreateSemaphore(nullptr,initial_value,0x7FFFFFFF,nullptr);VisusAssert(handle!=NULL);}

  //destructor
  ~Pimpl() {
    CloseHandle(handle);
  }

  //return only if "number of resources" is not zero, otherwise wait
  void down()
  {
    if (WaitForSingleObject(handle,INFINITE)!=WAIT_OBJECT_0)
      ThrowException("critical error, cannot down() the semaphore");
  }

  //tryDown
  bool tryDown() {
    return WaitForSingleObject(handle, 0) == WAIT_OBJECT_0;
  }

  //release one "resource"
  void up()
  {
    if (!ReleaseSemaphore(handle,1,nullptr))
      ThrowException("critical error, cannot up() the semaphore");
  }

};
#elif __APPLE__
class Semaphore::Pimpl
{
public:

  dispatch_semaphore_t sem;

  //constructor
  Pimpl(int initial_value)
  {
    sem = dispatch_semaphore_create(initial_value);
    if (!sem) ThrowException("critical error, cannot create the dispatch semaphore:");
  }

  //destructor
  ~Pimpl()
  {dispatch_release(sem);}

  //return only if "number of resources" is not zero, otherwise wait
  void down()
  {
    long retcode=dispatch_semaphore_wait(this->sem, DISPATCH_TIME_FOREVER);
    if (retcode!=0)
      ThrowException("critical error, cannot dispatch_semaphore_wait() the semaphore");
  }

  //tryDown
  bool tryDown() {
    return dispatch_semaphore_wait(this->sem, DISPATCH_TIME_NOW) == 0;
  }

  //release one "resource"
  void up(){
    dispatch_semaphore_signal(this->sem);
  }

};
#else

class Semaphore::Pimpl
{
public:

  sem_t sem;

 //constructor
  Pimpl(int initial_value) {
    sem_init(&sem,0,initial_value);
  }

  //destructor
  ~Pimpl() {
    sem_destroy(&sem);
  }

  //down
  void down()
  {
    for (int retcode=sem_wait(&sem);retcode==-1;retcode=sem_wait(&sem))
    {
      if (errno!=EINTR) 
        ThrowException("critical error, cannot down() the semaphore");
    }
  }


  //tryDown
  bool tryDown() {
    return sem_trywait(&sem) == 0;
  }

  //up
  void up()
  {
    if (sem_post(&sem)==-1) 
      ThrowException("critical error, cannot up() the semaphore");
}

};
#endif


}//namespace Visus


namespace Visus {

 ///////////////////////////////////////////////////
Semaphore::Semaphore(int initial_value){
  pimpl = new Pimpl(initial_value);
}

Semaphore::~Semaphore(){
  delete pimpl;
}

void Semaphore::down(){
  pimpl->down();
}

void Semaphore::up(){
  pimpl->up();
}

bool Semaphore::tryDown() {
  return pimpl->tryDown();
}

} //namespace Visus

