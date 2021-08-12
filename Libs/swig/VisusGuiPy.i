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
#include <Visus/GoogleMapsDataset.h>
#include <Visus/IsoContourRenderNode.h>
#include <Visus/GLCameraNode.h>
#include <Visus/IsoContourNode.h>
#include <Visus/IsoContourRenderNode.h>
#include <Visus/RenderArrayNode.h>
#include <Visus/KdRenderArrayNode.h>
#include <Visus/ScriptingNode.h>
#include <Visus/IdxDiskAccess.h>

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

%include <VisusPy.common>
%import <VisusKernelPy.i>
%import <VisusDataflowPy.i>
%import <VisusDbPy.i>
%import <VisusNodesPy.i>

//__________________________________________________________
%pythonbegin %{

this_dir=os.path.dirname(os.path.realpath(__file__))

# ///////////////////////////////////////////////////////////
# Qt5 dependency
if True:

	import PyQt5

	# see https://stackoverflow.com/questions/47608532/how-to-detect-from-within-python-whether-packages-are-managed-with-conda
	is_conda = os.path.exists(os.path.join(sys.prefix, 'conda-meta', 'history'))

	if WIN32:

		if os.path.isdir(os.path.join(os.path.dirname(PyQt5.__file__),"Qt")):
			AddSysPath(os.path.join(os.path.dirname(PyQt5.__file__),"Qt/bin"))
			os.environ["QT_PLUGIN_PATH"]= os.path.join(os.path.join(os.path.dirname(PyQt5.__file__),"Qt/plugins"))
			print("QT_PLUGIN_PATH",os.environ["QT_PLUGIN_PATH"])
		else:
			print("Cannot find Qt5 directory, OpenVisus GUI is probably going to crash")

	else:

		if os.path.isdir(os.path.join(os.path.dirname(PyQt5.__file__),"Qt/plugins")):
			os.environ["QT_PLUGIN_PATH"]= os.path.join(os.path.dirname(PyQt5.__file__),"Qt/plugins")
		
		elif is_conda and os.path.isdir(os.path.join(os.environ['CONDA_PREFIX'],"Library/plugins")):
			os.environ["QT_PLUGIN_PATH"]= os.path.join(os.environ['CONDA_PREFIX'],"Library/plugins")
		
		elif is_conda and os.path.join(os.environ['CONDA_PREFIX'],"plugins"):
			os.environ["QT_PLUGIN_PATH"]= os.path.join(os.environ['CONDA_PREFIX'],"plugins")
		else:
			print("Cannot find Qt5 plugins directory, OpenVisus GUI is probably going to crash")

		print("QT_PLUGIN_PATH",os.environ["QT_PLUGIN_PATH"])
# ///////////////////////////////////////////////////////////
%}


#define Q_OBJECT
#define signals public
#define slots   

//VISUS_DISOWN -> DISOWN | DISOWN_FOR_DIRECTOR
%apply SWIGTYPE *DISOWN_FOR_DIRECTOR { Visus::Node* disown };

%feature("director") Visus::Viewer;
	//see comment below about ScriptingNode
    %feature("nodirector") Visus::Viewer::execute;
    %feature("nodirector") Visus::Viewer::read;
    %feature("nodirector") Visus::Viewer::write;

//do I need these?
#if 0
%feature("director") Visus::GLCameraNode;
%feature("director") Visus::IsoContourNode;
%feature("director") Visus::IsoContourRenderNode;
%feature("director") Visus::RenderArrayNode;
%feature("director") Visus::KdRenderArrayNode;
%feature("director") Visus::JTreeRenderNode;
#endif

%feature("director") Visus::ScriptingNode;
	//on some Linux/Windows the swig generated code has problems with these methods. It tries to call the python counterparts even
	//if they don't exist.  Need to investigate
	//for now I'm just disabling them. It could cause some problems in the future in case you want to specialize the methods in python
	//NOTE: my guess it's failing to map correctly the Archive class (which is a typedef) with SharedPtr<StringTree>
    %feature("nodirector") Visus::ScriptingNode::execute;
    %feature("nodirector") Visus::ScriptingNode::read;
    %feature("nodirector") Visus::ScriptingNode::write;

%shared_ptr(Visus::IsoContour)
%shared_ptr(Visus::GLMesh)
%shared_ptr(Visus::GLCamera)
%shared_ptr(Visus::GLOrthoCamera)
%shared_ptr(Visus::GLLookAtCamera)

%typemap(out) std::shared_ptr<Visus::GLCamera> 
{
  //GLCamera typemape GLTMP01
  if(auto lookat=std::dynamic_pointer_cast<GLLookAtCamera>($1))
    $result = SWIG_NewPointerObj(SWIG_as_voidptr(result ? new std::shared_ptr<  Visus::GLLookAtCamera >(lookat) : 0), SWIGTYPE_p_std__shared_ptrT_Visus__GLLookAtCamera_t, SWIG_POINTER_OWN);

  else if(auto ortho=std::dynamic_pointer_cast<GLOrthoCamera>($1))
    $result = SWIG_NewPointerObj(SWIG_as_voidptr(result ? new std::shared_ptr<  Visus::GLOrthoCamera >(ortho) : 0), SWIGTYPE_p_std__shared_ptrT_Visus__GLOrthoCamera_t, SWIG_POINTER_OWN);

  else
	$result= SWIG_NewPointerObj(SWIG_as_voidptr(result ? new std::shared_ptr<  Visus::GLCamera >(result) : 0), SWIGTYPE_p_std__shared_ptrT_Visus__GLCamera_t, SWIG_POINTER_OWN);
}

%include <Visus/Gui.h>
%include <Visus/GLObject.h>
%include <Visus/GLMesh.h>
%include <Visus/GLObjects.h>
%include <Visus/GLCanvas.h>
%include <Visus/GLOrthoParams.h>
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

