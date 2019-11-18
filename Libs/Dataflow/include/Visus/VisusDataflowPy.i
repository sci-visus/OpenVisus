%module(directors="1") VisusDataflowPy

%{ 
#include <Visus/StringTree.h>
#include <Visus/DataflowModule.h>
#include <Visus/DataflowMessage.h>
#include <Visus/DataflowPort.h>
#include <Visus/DataflowNode.h>
#include <Visus/Dataflow.h>
using namespace Visus;
%}

%include <Visus/VisusPy.i>

%import <Visus/VisusKernelPy.i>

%shared_ptr(Visus::DataflowValue)
%shared_ptr(Visus::Dataflow)
%shared_ptr(Visus::ReturnReceipt)
%shared_ptr(Visus::NodeJob)

%feature("director") Visus::Node;
%feature("director") Visus::NodeCreator;
%feature("director") Visus::DataflowListener;

//VISUS_DISOWN | DISOWN_FOR_DIRECTOR
%apply SWIGTYPE *DISOWN_FOR_DIRECTOR { Visus::Node* disown };
%apply SWIGTYPE *DISOWN_FOR_DIRECTOR { Visus::NodeCreator* disown};

%template(VectorNode) std::vector<Visus::Node*>;

//VISUS_NEWOBJECT
%newobject Visus::NodeCreator::createInstance;
%newobject Visus::NodeFactory::createInstance;

%include <Visus/DataflowModule.h>
%include <Visus/DataflowMessage.h>
%include <Visus/DataflowPort.h>
%include <Visus/DataflowNode.h>
%include <Visus/Dataflow.h>

// _____________________________________________________
// python code 
%pythoncode %{

def VISUS_REGISTER_NODE_CLASS(TypeName):

   class PyNodeCreator(NodeCreator):
   
      def __init__(self,TypeName):
         NodeCreator.__init__(self)
         self.TypeName=TypeName

      def createInstance(self):
         return eval(self.TypeName+"()")

   NodeFactory.getSingleton().registerClass(TypeName,TypeName,PyNodeCreator(TypeName))

%}