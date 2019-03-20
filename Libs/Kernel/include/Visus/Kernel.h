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

#ifndef VISUS_KERNEL_H__
#define VISUS_KERNEL_H__

#include <Visus/Visus.h>

#if 0
#  define VISUS_OPENGL_ES 1
#else
#define VISUS_OPENGL_ES 0
#endif

#include <memory>
#include <string>
#include <sstream>
#include <atomic>
#include <exception>

//__________________________________________________________
#if WIN32

    //otherwise min() max() macro are declared
  #define NOMINMAX 

  //...needs to have dll-interface to be used by clients of class...
  #pragma warning(disable:4251) 

  #define VISUS_SHARED_EXPORT __declspec (dllexport)
  #define VISUS_SHARED_IMPORT __declspec (dllimport)

#else //#if WIN32

  #define VISUS_SHARED_EXPORT __attribute__ ((visibility("default")))
  #define VISUS_SHARED_IMPORT 

#endif 


//__________________________________________________________
#if SWIG 
  #undef  override
  #define override
  #undef  final
  #define final
#endif

//__________________________________________________________
#define VISUS_DEPRECATED(className) className

#define VISUS_STRINGIFY_MACRO_HELPER(a) #a

#define VISUS_JOIN_MACRO_HELPER(a, b) a ## b
#define VISUS_JOIN_MACRO(item1, item2)   VISUS_JOIN_MACRO_HELPER (item1, item2)


#ifndef VISUS_DISOWN
  #define VISUS_DISOWN(...) __VA_ARGS__
#endif

#ifndef VISUS_NEWOBJECT
  #define VISUS_NEWOBJECT(...) __VA_ARGS__
#endif

#if SWIG || VISUS_STATIC_KERNEL_LIB
  #define VISUS_KERNEL_API
#else
  #if VISUS_BUILDING_VISUSKERNEL
    #define VISUS_KERNEL_API VISUS_SHARED_EXPORT
  #else
    #define VISUS_KERNEL_API VISUS_SHARED_IMPORT
  #endif
#endif

namespace Visus {

typedef signed char        Int8;
typedef unsigned char      Uint8;
typedef signed short       Int16;
typedef unsigned short     Uint16;
typedef signed int         Int32;
typedef unsigned int       Uint32;
typedef float              Float32;
typedef double             Float64;
typedef long long          Int64;
typedef unsigned long long Uint64;

namespace Math {
  const double Pi = 3.14159265358979323846;
}

#define SharedPtr std::shared_ptr
#define UniquePtr std::unique_ptr

typedef std::string String;

VISUS_KERNEL_API inline String     cstring(bool   v) { return v ? "True" : "False"; }
VISUS_KERNEL_API inline String     cstring(int    v) { return std::to_string(v); }
VISUS_KERNEL_API inline String     cstring(Uint32 v) { return std::to_string(v); }
VISUS_KERNEL_API inline String     cstring(float  v) { return std::to_string(v); }
VISUS_KERNEL_API inline String     cstring(double v) { return std::to_string(v); }
VISUS_KERNEL_API inline String     cstring(Int64  v) { return std::to_string(v); }
VISUS_KERNEL_API inline String     cstring(Uint64 v) { return std::to_string(v); }

VISUS_KERNEL_API        bool       cbool  (const String& s);
VISUS_KERNEL_API inline int        cint   (const String& s) { return s.empty() ? 0 : std::stoi(s); }
VISUS_KERNEL_API inline float      cfloat (const String& s) { return s.empty() ? 0 : std::stof(s); }
VISUS_KERNEL_API inline double     cdouble(const String& s) { return s.empty() ? 0 : std::stod(s); }
VISUS_KERNEL_API inline Int64      cint64 (const String& s) { return s.empty() ? 0 : std::stoll(s); }
VISUS_KERNEL_API inline Uint64     cuint64(const String& s) { return s.empty() ? 0 : std::stoull(s); }

VISUS_KERNEL_API inline String     cstring(SharedPtr<String> v) { return v ? *v : ""; }
VISUS_KERNEL_API inline double     cdouble(SharedPtr<double> v) { return v ? *v : 0.0; }

#if !SWIG
namespace Private {
class VISUS_KERNEL_API CommandLine
{
public:
  static int argn;
  static const char** argv;
};
}; //namespace Private
#endif

//(default values for swig)
VISUS_KERNEL_API void SetCommandLine(int argn = 0, const char** argv = nullptr);

//for swig
VISUS_KERNEL_API void SetCommandLine(String value);

//VisusAssertFailed
VISUS_KERNEL_API void VisusAssertFailed(const char* file, int line, const char* expr);

//PrintMessageToTerminal
VISUS_KERNEL_API void PrintMessageToTerminal(const String& value);

//VisusHasMessageLock
VISUS_KERNEL_API bool VisusHasMessageLock();

//ThrowExceptionEx
VISUS_KERNEL_API void ThrowExceptionEx(String file,int line,String expr);


#define __S1__(x) #x
#define __S2__(x) __S1__(x)
#define VisusHereInTheCode __FILE__ " : " __S2__(__LINE__)

#define ThrowException(expr) (ThrowExceptionEx(__FILE__,__LINE__,expr))

//VisusAssert
#if !defined(SWIG)
#define VisusReleaseAssert(_Expression) \
  {if (!(_Expression)) Visus::VisusAssertFailed(__FILE__,__LINE__,#_Expression);} \
  /*--*/
  #if defined(VISUS_DEBUG)
    #define VisusAssert(_Expression) VisusReleaseAssert(_Expression)
  #else
    #define VisusAssert(_Expression) ((void)0) 
  #endif
#endif


///////////////////////////////////////////////////////////////////
#if !SWIG
class VISUS_KERNEL_API FormatString
{
public:

  //constructor
  FormatString() {
  }

  //constructor
  FormatString(const FormatString& other) {
    out << other.str();
  }

  //operator=
  FormatString& operator=(const FormatString& other) {
    out.clear(); out << other.str(); return *this;
  }

  //String()
  operator String() const {
    return out.str();
  }

  //str
  String str() const {
    return out.str();
  }

  //operator<<
  template <typename Type>
  FormatString& operator<<(Type value) {
    out << value; return *this;
  }

private:

  std::ostringstream out;

};

#endif

#if !SWIG


template <class ClassName>
class DetectMemoryLeaks
{
public:

  //_________________________________________________________
  class SharedCounter
  {
  public:

    std::atomic<int> value;

    //constructor
    SharedCounter() : value(0) {
    }

    //destructor
    ~SharedCounter()
    {
      if ((int)this->value != 0)
      {
        VisusAssert((int)this->value > 0);
        std::ostringstream out;
        out << "***** Leaked objects detected: " << (int)this->value << " instance(s) of class [" << typeid(ClassName).name()<< "] *****"<<std::endl;
        PrintMessageToTerminal(out.str());
        //VisusAssert(false);
      }
    }

    //getSingleton
    static SharedCounter& getSingleton() {
      static SharedCounter singleton;
      return singleton;
    }

  };

  //constructor
  DetectMemoryLeaks() {
    ++(SharedCounter::getSingleton().value);
  }

  //copy constructor
  DetectMemoryLeaks(const DetectMemoryLeaks&) {
    ++(SharedCounter::getSingleton().value);
  }

  //destructor
  ~DetectMemoryLeaks() {
    --(SharedCounter::getSingleton().value);
  }

};

#endif

#ifdef VISUS_DEBUG
  #define VISUS_CLASS(className) \
    Visus::DetectMemoryLeaks<className> VISUS_JOIN_MACRO(__leak_detector__,__LINE__);\
    /*--*/
#else
  #define VISUS_CLASS(className) \
    /*--*/
#endif

#define VISUS_NON_COPYABLE_CLASS(className) \
  VISUS_CLASS(className) \
  className (const className&)=delete;\
  className& operator= (const className&)=delete;\
  /*--*/

#define VISUS_PIMPL_CLASS(className) \
  VISUS_NON_COPYABLE_CLASS(className) \
  class        Pimpl;  \
  friend class Pimpl;  \
  Pimpl*       pimpl=nullptr;  \
  /*--*/

class VISUS_KERNEL_API Void {};

class VISUS_KERNEL_API KernelModule
{
public:

  static bool bAttached;

  //attach
  static void attach();

  //detach
  static void detach();
};


} //namespace Visus


#endif //VISUS_KERNEL_H__

