%module(directors="1") VisusDataflowPy

%{ 
#include <Visus/DataflowModule.h>
#include <Visus/DataflowMessage.h>
#include <Visus/DataflowPort.h>
#include <Visus/DataflowNode.h>
#include <Visus/Dataflow.h>
using namespace Visus;
%}

%include <Visus/VisusPy.i>

%import <Visus/VisusKernelPy.i>

%shared_ptr(Visus::Dataflow)
%shared_ptr(Visus::DataflowMessage)
%shared_ptr(Visus::ReturnReceipt)
%shared_ptr(Visus::NodeJob)

%apply SWIGTYPE *DISOWN_FOR_DIRECTOR { Visus::Node* disown };

%template(VectorNode) std::vector<Visus::Node*>;

%feature("director") Visus::Node;

%include <Visus/DataflowModule.h>
%include <Visus/DataflowMessage.h>
%include <Visus/DataflowPort.h>
%include <Visus/DataflowNode.h>
%include <Visus/Dataflow.h>
