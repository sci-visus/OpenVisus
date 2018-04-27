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

#include <Visus/UUID.h>

#include <iomanip>

#ifdef WIN32
#include <objbase.h>

#elif __APPLE__
#include <CoreFoundation/CFUUID.h>

#else
#include <uuid/uuid.h>

#endif



namespace Visus {


  
////////////////////////////////////////////////////////////////////
#ifdef WIN32
class UUIDGenerator::Pimpl
{
public:

  //constructor
  Pimpl()
  {}

  //destructor
  ~Pimpl()
  {}

  //create
  String create()
  {
    GUID newId;
    CoCreateGuid(&newId);

    std::ostringstream out;
    out << std::hex << std::setfill('0')
      << std::setw(2) << (int)(Uint8((newId.Data1 >> 24) & 0xFF))
      << std::setw(2) << (int)(Uint8((newId.Data1 >> 16) & 0xFF))
      << std::setw(2) << (int)(Uint8((newId.Data1 >> 8 ) & 0xFF))
      << std::setw(2) << (int)(Uint8((newId.Data1)       & 0xff))
      << "-"
      << std::setw(2) << (int)(Uint8((newId.Data2 >> 8 ) & 0xFF))
      << std::setw(2) << (int)(Uint8((newId.Data2)       & 0xff))
      << "-"
      << std::setw(2) << (int)(Uint8((newId.Data3 >> 8 ) & 0xFF))
      << std::setw(2) << (int)(Uint8((newId.Data3)       & 0xFF))
      << "-"
      << std::setw(2) << (int)(Uint8(newId.Data4[0]))
      << std::setw(2) << (int)(Uint8(newId.Data4[1]))
      << "-"
      << std::setw(2) << (int)(Uint8(newId.Data4[2]))
      << std::setw(2) << (int)(Uint8(newId.Data4[3]))
      << std::setw(2) << (int)(Uint8(newId.Data4[4]))
      << std::setw(2) << (int)(Uint8(newId.Data4[5]))
      << std::setw(2) << (int)(Uint8(newId.Data4[6]))
      << std::setw(2) << (int)(Uint8(newId.Data4[7]));

    return out.str();
  }

};
#endif


////////////////////////////////////////////////////////////////////
#if __GNUC__ && !__APPLE__
class UUIDGenerator::Pimpl
{
public:

  //constructor
  Pimpl()
  {}

  //destructor
  ~Pimpl()
  {}

  //create
  String create()
  {
    uuid_t bytes;
    uuid_generate(bytes);

    std::ostringstream out;
    out << std::hex << std::setfill('0')
      << std::setw(2) << (int)bytes[0]
      << std::setw(2) << (int)bytes[1]
      << std::setw(2) << (int)bytes[2]
      << std::setw(2) << (int)bytes[3]
      << "-"
      << std::setw(2) << (int)bytes[4]
      << std::setw(2) << (int)bytes[5]
      << "-"
      << std::setw(2) << (int)bytes[6]
      << std::setw(2) << (int)bytes[7]
      << "-"
      << std::setw(2) << (int)bytes[8]
      << std::setw(2) << (int)bytes[9]
      << "-"
      << std::setw(2) << (int)bytes[10]
      << std::setw(2) << (int)bytes[11]
      << std::setw(2) << (int)bytes[12]
      << std::setw(2) << (int)bytes[13]
      << std::setw(2) << (int)bytes[14]
      << std::setw(2) << (int)bytes[15];

    return out.str();
  }
};
#endif

////////////////////////////////////////////////////////////////////
#ifdef __APPLE__
class UUIDGenerator::Pimpl
{
public:

  //constructor
  Pimpl()
  {}

  //destructor
  ~Pimpl()
  {}

  //create
  String create()
  {
    CFUUIDRef   newId = CFUUIDCreate(NULL);
    CFUUIDBytes bytes = CFUUIDGetUUIDBytes(newId);
    CFRelease(newId);

    std::ostringstream out;
    out << std::hex << std::setfill('0')
      << std::setw(2) << (int)bytes.byte0
      << std::setw(2) << (int)bytes.byte1
      << std::setw(2) << (int)bytes.byte2
      << std::setw(2) << (int)bytes.byte3
      << "-"
      << std::setw(2) << (int)bytes.byte4
      << std::setw(2) << (int)bytes.byte5
      << "-"
      << std::setw(2) << (int)bytes.byte6
      << std::setw(2) << (int)bytes.byte7
      << "-"
      << std::setw(2) << (int)bytes.byte8
      << std::setw(2) << (int)bytes.byte9
      << "-"
      << std::setw(2) << (int)bytes.byte10
      << std::setw(2) << (int)bytes.byte11
      << std::setw(2) << (int)bytes.byte12
      << std::setw(2) << (int)bytes.byte13
      << std::setw(2) << (int)bytes.byte14
      << std::setw(2) << (int)bytes.byte15;

    return out.str();
  }
};
#endif

//////////////////////////////////////////////////////////////
UUIDGenerator::UUIDGenerator() 
{pimpl.reset(new Pimpl());}

UUIDGenerator::~UUIDGenerator()
{pimpl.reset();}

String UUIDGenerator::create()
{return pimpl->create();}


} //namespace Visus


