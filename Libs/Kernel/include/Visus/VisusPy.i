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

%begin %{
  #if _WIN32
    //this is needed if you dont' have python debug library
    #pragma warning(disable: 4101)
    #pragma warning(disable: 4244)
  #endif
%}

%{
#include <Visus/Kernel.h>
#include <Visus/Python.h>

#include <sstream>
#include <string>
#include <iostream>
%}

//__________________________________________________________
%pythonbegin %{

import os,sys,platform,math

__this_dir__= os.path.dirname(os.path.abspath(__file__))

WIN32=platform.system()=="Windows" or platform.system()=="win32"
if WIN32:

	# this is needed to find swig generated *.py file and DLLs
	def AddSysPath(value):
		os.environ['PATH'] = value + os.pathsep + os.environ['PATH']
		sys.path.insert(0,value)
		if hasattr(os,'add_dll_directory'): 
			os.add_dll_directory(value) # this is needed for python 38  

	AddSysPath(__this_dir__)
	AddSysPath(os.path.join(__this_dir__,"bin"))

else:

	# this is needed to find swig generated *.py file
	sys.path.append(__this_dir__)

%}

// Nested class not currently supported
#pragma SWIG nowarn=325 

// The 'using' keyword in template aliasing is not fully supported yet.
#pragma SWIG nowarn=342 

// Nested struct not currently supported
#pragma SWIG nowarn=312 

// warning : Nothing known about base class 'PositionRValue'
#pragma SWIG nowarn=401 

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


//__________________________________________________________
//argn argv

%include <argcargv.i>

%apply (int ARGC, char **ARGV) { (int argn, const char **argv) }



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

  catch (std::exception& e) {
	auto msg=cstring("Error in swig director code","what", e.what(),"where",__FILE__,":",__LINE__, "\n", GetPythonErrorMessage());
	PrintInfo(msg);
    SWIG_exception_fail(SWIG_SystemError, msg.c_str());
  }
  catch (...) {
	auto msg=cstring("Error in swig director code","where",__FILE__,":",__LINE__, "\n", GetPythonErrorMessage());
	PrintInfo(msg);
    SWIG_exception_fail(SWIG_SystemError, msg.c_str());
  }
}

%feature("director:except") {
   if ($error != NULL) {
     auto msg=cstring("Error calling $symname","where",__FILE__,":",__LINE__, "\n", GetPythonErrorMessage());
     PrintInfo(msg);
     Swig::DirectorMethodException::raise(msg.c_str());
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
  //scrgiorgio : TYPEMAP_0071534
  res = SWIG_ConvertPtr($input, %as_voidptrptr(&$1), $descriptor, SWIG_POINTER_DISOWN | %convertptr_flags);
  if (!SWIG_IsOK(res))  {
    %argument_fail(res,"$type", $symname, $argnum);
  }
  if (auto director = dynamic_cast<Swig::Director *>($1)) 
    director->swig_disown(); //C++ will own swig counterpart
}


//__________________________________________________________
// NEW_OBJECT
#define VISUS_NEWOBJECT(typename) typename


//use this when using directors 
// IMPORTANT NOTE: directorout is a typemape which is used when C++ code call Python code
// in this case (for example see c++ NodeFactory calling one Python createInstance function)
// C++ , after the call to python, owns the object
%define %newobject_director(ReturnType,Function)

    %newobject Function;

    %typemap(directorout,noblock=1) ReturnType Function(void *swig_argp, int swig_res, swig_owntype own) {
      swig_res = SWIG_ConvertPtrAndOwn($input, &swig_argp, $descriptor, %convertptr_flags | SWIG_POINTER_DISOWN, &own);
      if (!SWIG_IsOK(swig_res)) {
        %dirout_fail(swig_res,"$type");
      }
      $result = %reinterpret_cast(swig_argp, $ltype);
  
      //TYPEMAP_4562958  
      if (auto director = dynamic_cast<Swig::Director*>($result))
        director->swig_disown(); //C++ will own swig counterpart
    }

%enddef


//__________________________________________________________
//UniquePtr
// (not exposing UniquePtr to swig, please move it to the private section) ***


//__________________________________________________________
// ScopedVector
// (not exposing ScopedVector to swig, please move it to the private section) ***
