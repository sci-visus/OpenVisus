%module(directors="1") VisusDataflowPy

%{ 
#include <Visus/Visus.h>
#include <Visus/PythonEngine.h>
#include <Visus/DataflowModule.h>
#include <Visus/DataflowMessage.h>
#include <Visus/DataflowPort.h>
#include <Visus/DataflowNode.h>
#include <Visus/Dataflow.h>
using namespace Visus;
%}

%include <Visus/VisusPy.i>

%import <Visus/VisusKernelPy.i>

ENABLE_SHARED_PTR(Dataflow)
ENABLE_SHARED_PTR(DataflowMessage)
ENABLE_SHARED_PTR(ReturnReceipt)
ENABLE_SHARED_PTR(NodeJob)

%apply SWIGTYPE *DISOWN_FOR_DIRECTOR { Visus::Node* disown };

%template(VectorNode) std::vector<Visus::Node*>;

%feature("director") Visus::Node;

%include <Visus/DataflowModule.h>
%include <Visus/DataflowMessage.h>
%include <Visus/DataflowPort.h>
%include <Visus/DataflowNode.h>
%include <Visus/Dataflow.h>
