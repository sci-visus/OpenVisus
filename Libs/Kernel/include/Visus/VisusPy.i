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

%{
#include <Visus/Visus.h>
#include <Visus/PythonEngine.h>
%}

//common code 
%begin %{
  #if _WIN32
    //this is needed if you dont' have python debug library
    //#define SWIG_PYTHON_INTERPRETER_NO_DEBUG
    #pragma warning(disable: 4101)
    #pragma warning(disable: 4244)
  #endif

%}



//__________________________________________________________
%pythonbegin %{

import os,sys

__this_dir__=os.path.abspath(os.path.dirname(os.path.abspath(__file__)))

if not __this_dir__ in sys.path:
  sys.path.append(__this_dir__)

__bin_dir__=os.path.abspath(__this_dir__+ "/bin")
if not __bin_dir__ in sys.path:
  sys.path.append(__bin_dir__)
%}

// Nested class not currently supported
#pragma SWIG nowarn=325 

// The 'using' keyword in template aliasing is not fully supported yet.
#pragma SWIG nowarn=342 

// Nested struct not currently supported
#pragma SWIG nowarn=312 

// warning : Nothing known about base class 'PositionRValue'
#pragma SWIG nowarn=401 

namespace Visus {}
%apply long long  { Visus::Int64 };

//this is needed to expose import_array
%{
#define SWIG_FILE_WITH_INIT
%}


// _____________________________________________________
// init code 
%init %{
	//...your init code here...
%}


//__________________________________________________________
//STL

%include <stl.i>
%include <std_pair.i>
%include <std_vector.i>
%include <std_deque.i>
%include <std_string.i>
%include <std_map.i>
%include <std_set.i>
%include <std_shared_ptr.i>

%template(PairDoubleDouble)                std::pair<double,double>;
%template(PairIntDouble)                   std::pair<int,double>;
%template(VectorString)                    std::vector< std::string >;
%template(VectorInt)                       std::vector<int>;
%template(VectorDouble)                    std::vector<double>;
%template(VectorFloat)                     std::vector<float>;
%template(MapStringString)                 std::map< std::string , std::string >;

//__________________________________________________________
//RENAME

%rename(From)                              *::from;
%rename(To)                                *::to;
%rename(assign         )                   *::operator=;
%rename(__getitem__    )                   *::operator[](int) const;
%rename(__getitem_ref__)                   *::operator[](int);
%rename(__bool_op__)                       *::operator bool() ;
%rename(__structure_derefence_op__)        *::operator->();
%rename(__const_structure_derefence_op__)  *::operator->() const;
%rename(__indirection_op__)                *::operator*();
%rename(__const_indirection_op__)          *::operator*() const;
%rename(__add__)                           *::operator+;
%rename(__sub__)                           *::operator-; 
%rename(__neg__)                           *::operator-();  // Unary -
%rename(__mul__)                           *::operator*;
%rename(__div__)                           *::operator/;

//__________________________________________________________
// EXCEPTION

//whenever a C++ exception happens, I should 'forward' it to python
%exception 
{
  try { 
    $action
  } 
  catch (Swig::DirectorMethodException& e) {
	VisusInfo()<<"Error happened in swig director code: "<<e.what()<< " where("<< VisusHereInTheCode<<")";
	VisusInfo()<<PythonEngine::getLastErrorMessage();
    SWIG_fail;
  }
  catch (std::exception& e) {
    VisusInfo()<<"Swig Catched std::exception: "<<e.what()<<" where("<< VisusHereInTheCode<<")";
    SWIG_exception_fail(SWIG_SystemError, e.what() );
  }
  catch (...) {
    VisusInfo()<<"Swig Catched ... exception: unknown reason"  << " where("<< VisusHereInTheCode<<")";
    SWIG_exception(SWIG_UnknownError, "Unknown exception");
  }

}

//allow using PyObject* as input/output
%typemap(in) PyObject* { 
  $1 = $input; 
} 

// This is for other functions that want to return a PyObject. 
%typemap(out) PyObject* { 
  $result = $1; 
} 

//__________________________________________________________
// DISOWN
// grep for disown

//trick to simpligy the application of disown, rename the argument to disown
#define VISUS_DISOWN(argname) disown

// note: swig-defined DISOWN typemap crashes using directors 
// %apply SWIGTYPE *DISOWN { Visus::Node* disown }; THIS IS WRONG FOR DIRECTORS!
// see http://swig.10945.n7.nabble.com/Disown-Typemap-and-Directors-td9146.html)
%typemap(in, noblock=1) SWIGTYPE *DISOWN_FOR_DIRECTOR(int res = 0) 
{
  res = SWIG_ConvertPtr($input, %as_voidptrptr(&$1), $descriptor, SWIG_POINTER_DISOWN | %convertptr_flags);
  if (!SWIG_IsOK(res))  {
    %argument_fail(res,"$type", $symname, $argnum);
  }
  if (Swig::Director *director = dynamic_cast<Swig::Director *>($1)) 
    director->swig_disown(); //C++ will own swig counterpart
}


//__________________________________________________________
// NEW_OBJECT

// IMPORTANT avoid returning director objects x
#define VISUS_NEWOBJECT(typename) typename


//__________________________________________________________
//UniquePtr
// (not exposing UniquePtr to swig, please move it to the private section) ***


//__________________________________________________________
// ScopedVector
// (not exposing ScopedVector to swig, please move it to the private section) ***
//  %newobject Visus::ScopedVector::release;
// Visus::ScopedVector::*(...VISUS_DISOWN....)