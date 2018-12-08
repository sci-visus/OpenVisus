%module(directors="1") VisusGuiPy


%{ 
#include <Visus/Visus.h>
#include <Visus/PythonEngine.h>

#include <Visus/Gui.h>
#include <Visus/Frustum.h>
#include <Visus/GLCanvas.h>
#include <Visus/GLObjects.h>
namespace Visus { 
	QWidget* ToCppQtWidget(PyObject* obj) {
		return obj? (QWidget*)PyLong_AsVoidPtr(obj) : nullptr; 
	}
}
using namespace Visus;
%}

%include <Visus/VisusPy.i>

//__________________________________________________________
%pythonbegin %{
import os,platform,PyQt5
QT5_DIR=os.path.join(os.path.dirname(PyQt5.__file__),"Qt")
if (platform.system()=="Windows" or platform.system()=="win32"):
	if not os.path.join(QT5_DIR,"bin") in sys.path:
		sys.path.append(os.path.join(QT5_DIR,"bin"))
	os.environ["QT_PLUGIN_PATH"] = os.path.join(QT5_DIR,"plugins")
elif (platform.system()=="Darwin"):
	print("TODO")
else:
	print("TODO")
%}

%import  <Visus/VisusKernelPy.i>

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

