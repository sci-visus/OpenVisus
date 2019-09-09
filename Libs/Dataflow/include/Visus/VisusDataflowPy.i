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

%shared_ptr(Visus::DataflowValue)
%shared_ptr(Visus::Dataflow)
%shared_ptr(Visus::ReturnReceipt)
%shared_ptr(Visus::NodeJob)

%feature("director") Visus::Node;
%feature("director") Visus::NodeCreator;

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

%extend Visus::DataflowMessage 
{
    void writeInt   (String key,int     value) {$self->writeValue(key,value);}
    void writeDouble(String key,double  value) {$self->writeValue(key,value);}
    void writeString(String key,String  value) {$self->writeValue(key,value);}
    void writeArray (String key,Array   value) {$self->writeValue(key,value);}
}

%extend Visus::DataflowPort 
{
    void writeInt   (int     value) {$self->writeValue(value);}
    void writeDouble(double  value) {$self->writeValue(value);}
    void writeString(String  value) {$self->writeValue(value);}
    void writeArray (Array   value) {$self->writeValue(value);}
}

%extend Visus::Node 
{
	int     readInt   (String key) {auto ret=$self->readValue<int>   (key); return ret? *ret : 0;}
	double  readDouble(String key) {auto ret=$self->readValue<double>(key); return ret? *ret : 0.0;}
	String  readString(String key) {auto ret=$self->readValue<String>(key); return ret? *ret : "";}
	Array   readArray (String key) {auto ret=$self->readValue<Array> (key); return ret? *ret : Array();}
}

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