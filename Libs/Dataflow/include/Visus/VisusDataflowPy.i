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
//%shared_ptr(Visus::NodeJob) NOTE: you cannot mix shared_ptr with directors

%feature("director") Visus::Node;
%feature("director") Visus::NodeJob;
%feature("director") Visus::NodeCreator;
%feature("director") Visus::DataflowListener;

//VISUS_DISOWN -> DISOWN | DISOWN_FOR_DIRECTOR
%apply SWIGTYPE *DISOWN_FOR_DIRECTOR { Visus::Node* disown };
%apply SWIGTYPE *DISOWN_FOR_DIRECTOR { Visus::NodeCreator* disown};
%apply SWIGTYPE *DISOWN_FOR_DIRECTOR { Visus::NodeJob* disown};

%template(VectorNode) std::vector<Visus::Node*>;

//VISUS_NEWOBJECT
%newobject_director(Visus::Node *,Visus::NodeCreator::createInstance);
%newobject_director(Visus::Node *,Visus::NodeFactory::createInstance);

//NOTE I don't think this will help since I don't know all the classes in advance
//https://github.com/swig/swig/blob/master/Examples/test-suite/dynamic_cast.i

//see https://stackoverflow.com/questions/42349170/passing-java-object-to-c-using-swig-then-back-to-java
//see https://cta-redmine.irap.omp.eu/issues/287
%extend Visus::Node {
PyObject* __asPythonObject() {
    if (auto director = dynamic_cast<Swig::Director*>($self)) 
    {
        auto ret=director->swig_get_self();
        director->swig_incref(); //if python is owner this will increase the counter
        return ret;
    }
    else
    {
        return nullptr;
    }
}

%pythoncode %{
   # asPythonObject (whenever you have a director and need to access the python object)
   def asPythonObject(self):
    py_object=self.__asPythonObject()
    return py_object if py_object else self 
%}
}



%include <Visus/DataflowModule.h>
%include <Visus/DataflowMessage.h>
%include <Visus/DataflowPort.h>
%include <Visus/DataflowNode.h>
%include <Visus/Dataflow.h>

// _____________________________________________________
// python code 
%pythoncode %{

class PyNodeCreator(NodeCreator):
   
    def __init__(self,creator):
        super().__init__()
        self.creator=creator

    def createInstance(self):
        return self.creator()

def VISUS_REGISTER_NODE_CLASS(TypeName, PyTypeName, creator):
    print("Registering python class",TypeName,PyTypeName)
    NodeFactory.getSingleton().registerClass(TypeName, PyTypeName , PyNodeCreator(creator))
%}