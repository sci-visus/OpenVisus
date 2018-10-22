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

#ifndef __VISUS_ONDEMAND_ACCESS_H
#define __VISUS_ONDEMAND_ACCESS_H

#include <Visus/Db.h>
#include <Visus/Access.h>
#include <Visus/ThreadPool.h>

namespace Visus {

class Dataset;

//////////////////////////////////////////////////////////////////////////////
class VISUS_DB_API OnDemandAccess : public Access
{
public:

  VISUS_NON_COPYABLE_CLASS(OnDemandAccess)

  //__________________________________________________
  class VISUS_DB_API Pimpl
  {
  public:

    OnDemandAccess* owner;

    //constructor
    Pimpl(OnDemandAccess* owner_) : owner(owner_) {
    }

    //destructor
    virtual ~Pimpl() {
    }

    //generateBlock
    virtual void generateBlock(SharedPtr<BlockQuery> block_query) = 0;

  };

  //__________________________________________________
  class VISUS_DB_API Type
  { 
  public:
    enum {
      Checkerboard,
      Mandelbrot,
      GoogleMaps,
      External,
      ApplyFilter
    };

    //fromString
    static int fromString(String s)
    {
      s = StringUtils::trim(StringUtils::toLower(s));
      if (s == "checkerboard") return Checkerboard;
      if (s == "mandelbrot")   return Mandelbrot;
      if (s == "googlemaps")   return GoogleMaps;
      if (s == "external")     return External;
      if (s == "applyfilter")  return ApplyFilter;
      VisusAssert(false);
      return Checkerboard;
    }

    //toString
    static String toString(int t) {
      switch (t)
      {
      case Checkerboard: return "checkerboard";
      case Mandelbrot:   return "mandelbrot";
      case GoogleMaps:   return "googlemaps";
      case External:     return "external";
      case ApplyFilter:  return "applyfilter";
      default:           VisusAssert(false); return "checkerboard";
      }
    }
  };

  //constructor
  OnDemandAccess() {
  }

  //constructor
  OnDemandAccess(Dataset* dataset, StringTree config);

  //destructor 
  virtual ~OnDemandAccess();

  //getType
  int getType() const {
    return type;
  }

  //getPath
  String getPath() const {
    return path;
  }

  //getDataset
  Dataset* getDataset() const {
    return dataset;
  }

  //readBlock
  virtual void readBlock(SharedPtr<BlockQuery> query) override {

    if (query->aborted())
      return readFailed(query);

    ThreadPool::push(thread_pool,[this, query]() {
        pimpl->generateBlock(query);
    });
  }

  //writeBlock
  virtual void writeBlock(SharedPtr<BlockQuery> query) override {
    VisusAssert(false);
    VisusInfo() << "OnDemandAccess::write block not supported";
    return writeFailed(query);
  }

  //printStatistics
  virtual void printStatistics() override {
    VisusInfo() << "OnDemandAccess::printStatistics....";
  }

private:

  int                       type= Type::Checkerboard;
  String                    path;  //for example, to an external app or web service
  Dataset*                  dataset=nullptr;
  Pimpl*                    pimpl = nullptr;
  SharedPtr<ThreadPool>     thread_pool;

}; 

} //namespace Visus


#endif //__VISUS_ONDEMAND_ACCESS_H





