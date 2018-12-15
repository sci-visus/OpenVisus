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

import PyQt5
QT5_DIR=os.path.join(os.path.dirname(PyQt5.__file__),"Qt")

os.environ["QT_PLUGIN_PATH"]= os.path.join(QT5_DIR,"plugins")

for it in [os.path.join(QT5_DIR,"bin"),os.path.join(QT5_DIR,"lib")]:
	if (os.path.isdir(it)) and (not it in sys.path):
		sys.path.insert(0,it)
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

