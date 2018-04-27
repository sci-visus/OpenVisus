%module(directors="1") VisusDataflowPy

%{ 
#define SWIG_FILE_WITH_INIT
#include <Visus/Visus.h>
#include <Visus/PythonEngine.h>
#include <Visus/Array.h>
#include <Visus/Frustum.h>
#include <Visus/DataflowModule.h>
#include <Visus/DataflowMessage.h>
#include <Visus/DataflowPort.h>
#include <Visus/DataflowNode.h>
#include <Visus/Dataflow.h>
using namespace Visus;
%}

%include <VisusSwigCommon.i>
%import  <VisusKernelPy.i>

//new object
/* empty */

//SharedPtr
ENABLE_SHARED_PTR(Dataflow)
ENABLE_SHARED_PTR(DataflowMessage)
ENABLE_SHARED_PTR(ReturnReceipt)
ENABLE_SHARED_PTR(NodeJob)

//disown
%apply SWIGTYPE *DISOWN_FOR_DIRECTOR { Visus::Node* disown };

%template(VectorNode) std::vector<Visus::Node*>;

%feature("director") Visus::Node;

%include <Visus/DataflowModule.h>
%include <Visus/DataflowMessage.h>
%include <Visus/DataflowPort.h>
%include <Visus/DataflowNode.h>
%include <Visus/Dataflow.h>
