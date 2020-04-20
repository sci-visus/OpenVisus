%module(directors="1") VisusGuiPy

%{ 
#include <Visus/Gui.h>
#include <Visus/Frustum.h>
#include <Visus/GLCanvas.h>
#include <Visus/GLObjects.h>
#include <Visus/StringTree.h>

#include <Visus/GLCamera.h>
#include <Visus/GLOrthoCamera.h>
#include <Visus/GLLookAtCamera.h>

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

//__________________________________________________________
%pythonbegin %{

this_dir=os.path.dirname(os.path.realpath(__file__))

# using embedded Qt5?
if os.path.isdir(os.path.join(this_dir,"bin","Qt")):
	AddSysPath(os.path.join(this_dir,"bin"),bBegin=True)
	os.environ["QT_PLUGIN_PATH"]= os.path.join(this_dir,"bin","Qt","plugins")

# using PyQt5
else:
	import PyQt5
	PYQT5_DIR=os.path.dirname(PyQt5.__file__)
	AddSysPath(os.path.join(PYQT5_DIR,"Qt","bin"),bBegin=True)
	AddSysPath(os.path.join(PYQT5_DIR,"Qt","lib"),bBegin=True)
	os.environ["QT_PLUGIN_PATH"]= os.path.join(PYQT5_DIR,"Qt","plugins")
%}

%import <Visus/VisusKernelPy.i>

#define Q_OBJECT
#define signals public
#define slots   

%include <Visus/Gui.h>
%include <Visus/GLObject.h>

%shared_ptr(Visus::GLMesh)
%include <Visus/GLMesh.h>

%include <Visus/GLObjects.h>
%include <Visus/GLCanvas.h>

%shared_ptr(Visus::GLCamera)
%shared_ptr(Visus::GLOrthoCamera)
%shared_ptr(Visus::GLLookAtCamera)

%include <Visus/GLCamera.h>
%include <Visus/GLOrthoCamera.h>
%include <Visus/GLLookAtCamera.h>

//see https://github.com/bleepbloop/Pivy/blob/master/interfaces/soqt.i
namespace Visus {
	QWidget*  ToCppQtWidget   (PyObject* obj);
	PyObject* FromCppQtWidget (void*     widget);
}

