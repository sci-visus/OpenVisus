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


%module(directors="1") GuiOpenVisus

%include <VisusSwigCommon.i>
%import  <NonGuiOpenVisus.i>

//__________________________________________________________
%{ 

#include <Visus/PythonEngine.h>

//Gui
#include <Visus/Gui.h>
#include <Visus/Frustum.h>
#include <Visus/GLCanvas.h>
namespace Visus { QWidget* ToCppQtWidget(PyObject* obj) {return obj? (QWidget*)PyLong_AsVoidPtr(obj) : nullptr; }}

//GuiNodes
#include <Visus/PythonNode.h>

//AppKit
#include <Visus/Viewer.h>

using namespace Visus;

%}

%import "NonGuiOpenVisus.i"


/////////////////////////////////////////////////////////////////////
//swig includes

//Gui
#define Q_OBJECT
#define signals public
#define slots   
%include <Visus/Gui.h>
%include <Visus/GLObject.h>
%include <Visus/GLMesh.h>
%include <Visus/GLObjects.h>
%include <Visus/GLCanvas.h>
//see https://github.com/bleepbloop/Pivy/blob/master/interfaces/soqt.i
namespace Visus {QWidget* ToCppQtWidget(PyObject* obj);}

//GuiNodes
%include <Visus/GuiNodes.h>
//allow rendering inside main GLCanvas (problem of multi-inheritance Node and GLObject)
%feature("director") Visus::PythonNode; 
%feature("nodirector") Visus::PythonNode::beginUpdate;
%feature("nodirector") Visus::PythonNode::endUpdate;
%feature("nodirector") Visus::PythonNode::getNodeBounds;
%feature("nodirector") Visus::PythonNode::glSetRenderQueue;
%feature("nodirector") Visus::PythonNode::glGetRenderQueue;
%feature("nodirector") Visus::PythonNode::modelChanged;
%feature("nodirector") Visus::PythonNode::enterInDataflow;
%feature("nodirector") Visus::PythonNode::exitFromDataflow;
%feature("nodirector") Visus::PythonNode::addNodeJob;
%feature("nodirector") Visus::PythonNode::abortProcessing;
%feature("nodirector") Visus::PythonNode::joinProcessing;
%feature("nodirector") Visus::PythonNode::messageHasBeenPublished;
%feature("nodirector") Visus::PythonNode::toBool;
%feature("nodirector") Visus::PythonNode::toInt;
%feature("nodirector") Visus::PythonNode::toInt64;
%feature("nodirector") Visus::PythonNode::toDouble;
%feature("nodirector") Visus::PythonNode::toString;
%feature("nodirector") Visus::PythonNode::writeToObjectStream;
%feature("nodirector") Visus::PythonNode::readFromObjectStream;
%feature("nodirector") Visus::PythonNode::writeToSceneObjectStream;
%feature("nodirector") Visus::PythonNode::readFromSceneObjectStream;
%include <Visus/PythonNode.h>

//AppKit
%include <Visus/AppKit.h>
%include <Visus/Viewer.h>

