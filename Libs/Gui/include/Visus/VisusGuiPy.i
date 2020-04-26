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
if os.path.isdir(os.path.join(this_dir,"bin","qt")):
	QT5_DIR=os.path.join(this_dir,"bin","qt")

# using PyQt5
else: 
	import PyQt5
	QT5_DIR=os.path.join(os.path.dirname(PyQt5.__file__),"qt")

AddSysPath(os.path.join(QT5_DIR,"bin"),bBegin=True)
os.environ["QT_PLUGIN_PATH"]= os.path.join(QT5_DIR,"plugins")
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

