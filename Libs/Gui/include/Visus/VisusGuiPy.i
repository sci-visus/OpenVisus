%module(directors="1") VisusGuiPy

%{ 
#include <Visus/Gui.h>

#include <Visus/Frustum.h>
#include <Visus/GLCanvas.h>
#include <Visus/StringTree.h>

#include <Visus/Dataflow.h>
#include <Visus/Nodes.h>

#include <Visus/GLCamera.h>
#include <Visus/GLOrthoCamera.h>
#include <Visus/GLLookAtCamera.h>
#include <Visus/GLObjects.h>

#include <Visus/IdxDataset.h>
#include <Visus/IdxMultipleDataset.h>
#include <Visus/IsoContourRenderNode.h>
#include <Visus/GLCameraNode.h>
#include <Visus/IsoContourNode.h>
#include <Visus/IsoContourRenderNode.h>
#include <Visus/RenderArrayNode.h>
#include <Visus/KdRenderArrayNode.h>
#include <Visus/ScriptingNode.h>

#include <Visus/Viewer.h>

namespace Visus { 
	QWidget* ToCppQtWidget(PyObject* obj) {
		return obj? (QWidget*)PyLong_AsVoidPtr(obj) : nullptr; 
	}
	PyObject* FromCppQtWidget(void* widget) {
		return PyLong_FromVoidPtr(widget);
	}
}

using namespace Visus;
%}

%include <Visus/VisusPy.i>
%import <Visus/VisusKernelPy.i>
%import <Visus/VisusDataflowPy.i>
%import <Visus/VisusDbPy.i>
%import <Visus/VisusNodesPy.i>

//__________________________________________________________
%pythonbegin %{

this_dir=os.path.dirname(os.path.realpath(__file__))

import PyQt5
QT5_DIR=os.path.join(os.path.dirname(PyQt5.__file__),"Qt")

# for windows I need to tell how to find Qt
if WIN32:
	AddSysPath(os.path.join(QT5_DIR,"bin"))

# I need to tell where to found Qt plugins
os.environ["QT_PLUGIN_PATH"]= os.path.join(QT5_DIR,"plugins")
%}


#define Q_OBJECT
#define signals public
#define slots   

//VISUS_DISOWN -> DISOWN | DISOWN_FOR_DIRECTOR
%apply SWIGTYPE *DISOWN_FOR_DIRECTOR { Visus::Node* disown };

%feature("director") Visus::Viewer;
%feature("director") Visus::GLCameraNode;
%feature("director") Visus::IsoContourNode;
%feature("director") Visus::IsoContourRenderNode;
%feature("director") Visus::RenderArrayNode;
%feature("director") Visus::OSPRayRenderNode;
%feature("director") Visus::KdRenderArrayNode;
%feature("director") Visus::JTreeRenderNode;
%feature("director") Visus::ScriptingNode;

%shared_ptr(Visus::IsoContour)
%shared_ptr(Visus::GLMesh)
%shared_ptr(Visus::GLCamera)
%shared_ptr(Visus::GLOrthoCamera)
%shared_ptr(Visus::GLLookAtCamera)

%include <Visus/Gui.h>
%include <Visus/GLObject.h>
%include <Visus/GLMesh.h>
%include <Visus/GLObjects.h>
%include <Visus/GLCanvas.h>
%include <Visus/GLCamera.h>
%include <Visus/GLOrthoCamera.h>
%include <Visus/GLLookAtCamera.h>

%include <Visus/GLCameraNode.h>
%include <Visus/IsoContourNode.h>
%include <Visus/IsoContourRenderNode.h>
%include <Visus/RenderArrayNode.h>
%include <Visus/KdRenderArrayNode.h>
%include <Visus/ScriptingNode.h>

%include <Visus/Viewer.h>

//see https://github.com/bleepbloop/Pivy/blob/master/interfaces/soqt.i
namespace Visus {
	QWidget*  ToCppQtWidget   (PyObject* obj);
	PyObject* FromCppQtWidget (void*     widget);
}
