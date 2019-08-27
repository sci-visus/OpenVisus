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

#ifndef VISUS_ENCODERS_H
#define VISUS_ENCODERS_H

#include <Visus/Kernel.h>
#include <Visus/HeapMemory.h>
#include <Visus/Point.h>
#include <Visus/DType.h>

namespace Visus {

  //////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Encoder
{
public:

  VISUS_CLASS(Encoder)
  
  //constructor
  Encoder() {
  }
  
  //destructor
  virtual ~Encoder() {
  }

  //isLossy
  virtual bool isLossy() const=0;

  //encode
  virtual SharedPtr<HeapMemory> encode(PointNi dims,DType dtype, SharedPtr<HeapMemory> decoded, std::vector<String> options)=0;

  //decode
  virtual SharedPtr<HeapMemory> decode(PointNi dims,DType dtype, SharedPtr<HeapMemory> encoded, std::vector<String> options)=0;

};


//////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Encoders 
{
public:

  VISUS_DECLARE_SINGLETON_CLASS(Encoders)

  //addEncoder
  void addEncoder(String key,SharedPtr<Encoder> value)
  {
    VisusAssert(encoders.find(key)==encoders.end());
    VisusAssert(key.find('-') == std::string::npos);
    encoders[key]=value;
  }

  //getEncoder
  Encoder* getEncoder(String compression_specs,std::vector<String>& options) const
  {
    //NOTE: the compression_specs is in this format:
    //  key | key-option1 | key-option1,option2 | key-option1,option2,option3 | ...
    //  so the first separator is a '-', the options separator is ','

    String key;
    auto hyphen = compression_specs.find('-');
    if (hyphen == std::string::npos)
    {
      key = compression_specs;
    }
    else
    { 
      key = compression_specs.substr(0, hyphen);

      for (auto it : StringUtils::split(compression_specs.substr(hyphen + 1), ","))
      {
        auto option = StringUtils::trim(it);
        if (!option.empty())
          options.push_back(option);
      }
    }
    
    key = StringUtils::trim(StringUtils::toLower(key));
    auto it=encoders.find(key);
    return it!=encoders.end()? it->second.get() : nullptr;
  }

private:

  std::map<String, SharedPtr<Encoder> > encoders;


  //constructor
  Encoders();


};


} //namespace Visus

#endif //VISUS_ENCODERS_H


