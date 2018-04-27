%module(directors="1") VisusGuiPy 


%{ 
#define SWIG_FILE_WITH_INIT
#include <Visus/Gui.h>
#include <Visus/PythonEngine.h>
#include <Visus/DoubleSlider.h>
#include <Visus/Canvas.h>
#include <Visus/GLTexture.h>
#include <Visus/GLArrayBuffer.h>
#include <Visus/GLInfo.h>
#include <Visus/GLMaterial.h>
#include <Visus/GLMouse.h>
#include <Visus/GLOrthoParams.h>
#include <Visus/GLObject.h>
#include <Visus/GLMesh.h>
#include <Visus/GLCamera.h>
#include <Visus/GLLookAtCamera.h>
#include <Visus/GLOrthoCamera.h>
#include <Visus/GLShader.h>
#include <Visus/GLPhongShader.h>
#include <Visus/GLCanvas.h>
#include <Visus/GLObjects.h>
#include <Visus/GuiFactory.h>
using namespace Visus;
%}

%include <VisusSwigCommon.i>
%import  <VisusKernelPy.i>


//NOTE ignoring all Gui-related new objects (see for example GuiFactory.h) for now. To understand how to mix it with python Qt

#define Q_OBJECT
#define signals public
#define slots   


//SharedPtr
ENABLE_SHARED_PTR(GLArrayBuffer)
ENABLE_SHARED_PTR(GLCamera)
ENABLE_SHARED_PTR(GLMesh)
ENABLE_SHARED_PTR(GLObject)
ENABLE_SHARED_PTR(GLProgram)
ENABLE_SHARED_PTR(GLTexture)

//disown
/*--*/

//new object
/*--*/


///////////////////////////////////////////////////////
//see https://github.com/bleepbloop/Pivy/blob/master/interfaces/soqt.i
namespace Visus {
QWidget* convertToQWidget(PyObject* obj);
}

%{
namespace Visus {
QWidget* convertToQWidget(PyObject* obj) {
  return obj? (QWidget*)PyLong_AsVoidPtr(obj) : nullptr;       
}
}
%}

%include <Visus/Gui.h>
%include <Visus/DoubleSlider.h>
%include <Visus/Canvas.h>
%include <Visus/GLTexture.h>
%include <Visus/GLArrayBuffer.h>
%include <Visus/GLInfo.h>
%include <Visus/GLMaterial.h>
%include <Visus/GLMouse.h>
%include <Visus/GLOrthoParams.h>
%include <Visus/GLObject.h>
%include <Visus/GLMesh.h>
%include <Visus/GLCamera.h>
%include <Visus/GLLookAtCamera.h>
%include <Visus/GLOrthoCamera.h>
%include <Visus/GLShader.h>
%include <Visus/GLPhongShader.h>
%include <Visus/GLCanvas.h>
%include <Visus/GLObjects.h>
//DO I need it?
//%include <Visus/GuiFactory.h> 


%template(GLQuadPoint3d) Visus::GLQuad::GLQuad<Visus::Point3d>;
