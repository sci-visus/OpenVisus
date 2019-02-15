%module(directors="1") VisusGuiPy

%{ 
#include <Visus/Gui.h>
#include <Visus/Frustum.h>
#include <Visus/GLCanvas.h>
#include <Visus/GLObjects.h>
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
	for it in [os.path.join(this_dir,"bin")]:
		if (os.path.isdir(it)) and (not it in sys.path):
			sys.path.insert(0,it)
	os.environ["QT_PLUGIN_PATH"]= os.path.join(this_dir,"bin","Qt","plugins")

# using PyQt5
else:
	import PyQt5
	PYQT5_DIR=os.path.dirname(PyQt5.__file__)
	for it in [os.path.join(PYQT5_DIR,"Qt","bin"),os.path.join(PYQT5_DIR,"Qt","lib")]:
		if (os.path.isdir(it)) and (not it in sys.path):
			sys.path.insert(0,it)
	os.environ["QT_PLUGIN_PATH"]= os.path.join(PYQT5_DIR,"Qt","plugins")
%}

%import <Visus/VisusKernelPy.i>

#define Q_OBJECT
#define signals public
#define slots   

%include <Visus/Gui.h>
%include <Visus/GLObject.h>
%include <Visus/GLMesh.h>
%include <Visus/GLObjects.h>
%include <Visus/GLCanvas.h>

//see https://github.com/bleepbloop/Pivy/blob/master/interfaces/soqt.i
namespace Visus {
	QWidget*  ToCppQtWidget   (PyObject* obj);
	PyObject* FromCppQtWidget (void*     widget);
}

