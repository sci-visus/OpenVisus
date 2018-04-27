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


#ifndef VISUS_APPLICATION_STATS_H
#define VISUS_APPLICATION_STATS_H

#include <Visus/Kernel.h>
#include <Visus/CriticalSection.h>

#include <atomic>

namespace Visus {

//////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API ApplicationStats
{
public:

  VISUS_CLASS(ApplicationStats)

  //________________________________________________________________
  class VISUS_KERNEL_API Single
  {
  public:

    class Values
    {
    public:
      Int64 nopen = 0;
      Int64 rbytes = 0;
      Int64 wbytes = 0;
    };

    CriticalSection    lock;
    Values             values;

    //constructor
    Single() {
    }

    //readValues
    Values readValues(bool bReset) {
      ScopedLock lock(this->lock);
      auto ret = this->values;
      if (bReset) this->values = Values();
      return ret;
    }

    //trackOpen
    void trackOpen() {
      ScopedLock lock(this->lock); ++values.nopen;
    }

    //trackReadOperation
    void trackReadOperation(Int64 nbytes) {
      ScopedLock lock(this->lock); values.rbytes += nbytes;
    }

    //trackWriteOperation
    void trackWriteOperation(Int64 nbytes) {
      ScopedLock lock(this->lock); values.wbytes += nbytes;
    }
  };

  static Single           io;
  static Single           net;
  static std::atomic<int> num_cpu_jobs;
  static std::atomic<int> num_net_jobs;
  static std::atomic<int> num_threads;

private:

  //constructor
  ApplicationStats();

};

} //namespace Visus

#endif  //VISUS_APPLICATION_STATS_H



