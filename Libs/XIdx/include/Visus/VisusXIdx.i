%module(directors="1") VisusXIdxPy

%{ 
#include <Visus/PythonEngine.h>
#include <Visus/VisusXIdx.h>

using namespace Visus;
%}

%include <Visus/VisusPy.i>
%import <Visus/VisusKernelPy.i>

%include <Visus/VisusXIdx.h>

//TODO: expose SharedPtr<...> and std::vector < ...>

%include <Visus/xidx_element.h>
%include <Visus/xidx_data_source.h>
%include <Visus/xidx_attribute.h>
%include <Visus/xidx_dataitem.h>
%include <Visus/xidx_domain.h>
%include <Visus/xidx_topology.h>
%include <Visus/xidx_geometry.h>
%include <Visus/xidx_variable.h>
%include <Visus/xidx_group.h>
%include <Visus/xidx_metadatafile.h>

%include <Visus/xidx_list_domain.h>
%include <Visus/xidx_hyperslab_domain.h>
%include <Visus/xidx_spatial_domain.h>
%include <Visus/xidx_multiaxis_domain.h>
