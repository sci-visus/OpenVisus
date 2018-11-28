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


%include <VisusPy.i>
%import  <VisusKernelPy.i>

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

