/*
 * Copyright (c) 2017 University of Utah
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <ctime>

#include "Visus/VisusXIdx.h"

using namespace Visus;

int write_temporal_hyperslab_reg_grid(const char* filepath, int n_attributes, int n_timesteps){
  XIdxFile meta;
  
  // Create a group to collect a time series
  SharedPtr<Group> time_group(new Group("TimeSeries", GroupType::TEMPORAL_GROUP_TYPE));
  meta.addGroup(time_group);

  // Create a data source for this group
  // if a variable does not redefine a data source the group source will be used
  SharedPtr<DataSource> file(new DataSource("data", "file_path"));
  time_group->addDataSource(file);
  
  const int n_dims = 3;

  // Create an hyperslab time domain (start, step, count)
  SharedPtr<TemporalHyperSlabDomain> time_dom = std::make_shared<TemporalHyperSlabDomain>(new TemporalHyperSlabDomain("Time"));
  time_dom->setDomain(2.0, float(n_timesteps-1)*0.1, n_timesteps);


  // Set the time group domain to use the time domain we just created
  time_group->setDomain(time_dom);
  
  ////////////////////////////////////////////////////////////
  //
  //  Spatial grid with shared dataset
  //
  ////////////////////////////////////////////////////////////
  
  // Create a new group to collect a set of variables that share the same spatial domain
  SharedPtr<Group> grid(new Group("L0", GroupType::SPATIAL_GROUP_TYPE)); // default static group
  
  // Create a spatial domain
  SharedPtr<SpatialDomain> space_dom(new SpatialDomain("Grid"));
  
  uint32_t dims[3] = {10, 20, 30};// logical dims
  double o[3] = {0, 0, 0};         // origin x y z
  double d[3] = {1.f, 1.f, 1.f};   // dx dy dz

  SharedPtr<Topology> topology(new Topology(TopologyType::CORECT_3D_MESH_TOPOLOGY_TYPE, n_dims,
                    dims));
  // Set topology and geometry of the spatial domain
  space_dom->setTopology(topology);

  SharedPtr<Geometry> geometry(new Geometry(GeometryType::ORIGIN_DXDYDZ_GEOMETRY_TYPE, n_dims, o, d));
  space_dom->setGeometry(geometry);
  
  // Set the domain for the spatial group
  grid->setDomain(space_dom);
  
  // add some variables to the spatial group
  for(int i=0; i < n_attributes; i++){
    char name[32];
    sprintf(name, "var_%d", i);
    grid->addVariable(name, DTypes::FLOAT32);
  }
  
  // add the group of variables to the time series
  time_group->addGroup(grid);
  
  // Set the root group of the metadata
  //meta.addGroup(time_group);
  // Write to disk
  meta.save(filepath);
  
  printf("%zu timeteps written in %s\n", meta.groups.size(), filepath);
  
  return 0;
}


int write_temporal_list_multiaxis(const char* filepath, int n_attributes, int n_timesteps){
  XIdxFile meta;

  // Create a group to collect a time series
  SharedPtr<Group> time_group(new Group("TimeSeries", GroupType::TEMPORAL_GROUP_TYPE));
  meta.addGroup(time_group);

  // Create a data source for this group
  // if a variable does not redefine a data source the group source will be used
  SharedPtr<DataSource> file(new DataSource("data", "file_path"));
  time_group->addDataSource(file);

  // Create the time domain
  SharedPtr<Domain> time_dom;

  // Create series of timestep values
  time_dom = SharedPtr<TemporalListDomain>(new TemporalListDomain("Time"));
  time_dom->addAttribute("units", "days since 1980");
  time_dom->addAttribute("calendar", "gregorian");

  for(int i=0; i < n_timesteps; i++){
    std::dynamic_pointer_cast<TemporalListDomain>(time_dom)->addDomainItem(float(i+10));
  }

  // You can also add tuples of items (e.g., netcdf bounds)
  std::dynamic_pointer_cast<TemporalListDomain>(time_dom)->addDomainItems({float(100),float(200)});

  // Set the time group domain to use the time domain we just created
  time_group->setDomain(time_dom);

  ///////////////////////////////////////////////////////////////
  //
  //  Spatial grid defined by an explicit set of axis (Climate)
  //
  ///////////////////////////////////////////////////////////////

  // Define a new domain, group and file for a different set of variables
  SharedPtr<MultiAxisDomain> geo_dom(new MultiAxisDomain("Geospatial"));
  SharedPtr<Axis> latitude_axis(new Axis("latitude"));
  SharedPtr<Axis> longitude_axis(new Axis("longitude"));

  // Populate the axis with explicit values (will be written in the XML)
  for(int i=0; i < 10; i++){
    latitude_axis->addValue((double)i*0.5);
    longitude_axis->addValue((double)i*2*0.6);

    // You can also add tuple of values (e.g., netcdf bounds)
    // longitude_axis.addValues({(double)i*2*0.6,(double)i*2*1.2});
  }

  latitude_axis->addAttribute("units", "degrees_north");
  latitude_axis->addAttribute("units", "degrees_east");

  // add this axis to the domain
  geo_dom->addAxis(latitude_axis);
  geo_dom->addAxis(longitude_axis);

  // Create group for the variables defined in the geospatial domain
  SharedPtr<Group> geo_vars(new Group("geo_vars", GroupType::SPATIAL_GROUP_TYPE, geo_dom));

  // Create and add a variable to the group
  SharedPtr<Variable> temp = geo_vars->addVariable("geo_temperature", DTypes::FLOAT32);
  if(!temp)
    printf("error\n");

  // add attribute to the variable (key-value) pairs
  temp->addAttribute("unit", "Celsius");
  temp->addAttribute("valid_min", "-100.0");
  temp->addAttribute("valid_max", "200.0");

  time_group->addGroup(geo_vars);

  // Write to disk
  meta.save(filepath);

  printf("%d timesteps written in %s\n", n_timesteps, filepath);

  return 0;
}

int write_temporal_list_binary_axis(const char* filepath, int n_attributes, int n_timesteps){
  XIdxFile meta;

  // Create a group to collect a time series
  SharedPtr<Group> time_group(new Group("TimeSeries", GroupType::TEMPORAL_GROUP_TYPE));
  meta.addGroup(time_group);

  // Create a data source for this group
  // if a variable does not redefine a data source the group source will be used
  SharedPtr<DataSource> file(new DataSource("data", "file_path"));
  time_group->addDataSource(file);

  // Create the time domain
  SharedPtr<Domain> time_dom = SharedPtr<TemporalListDomain>(new TemporalListDomain("Time"));

  for(int i=0; i < n_timesteps; i++){
    std::dynamic_pointer_cast<TemporalListDomain>(time_dom)->addDomainItem(float(i+10));
  }

  // Set the time group domain to use the time domain we just created
  time_group->setDomain(time_dom);

  ///////////////////////////////////////////////////////////////
  //
  //  Rectilinear grid with coordinates saved on file
  //
  ///////////////////////////////////////////////////////////////

  /// Use a binary file to define a rectilinear grid coordinates
  int file_n_dims = 2;
  uint32_t file_dims[2] = {100, 200};

  SharedPtr<SpatialDomain> file_dom(new SpatialDomain("FileBasedDomain"));
  SharedPtr<Topology> topology(new Topology(TopologyType::RECT_2D_MESH_TOPOLOGY_TYPE, file_n_dims,
                    file_dims));
  file_dom->setTopology(topology);

  // Create a DataSource that points to the file
  SharedPtr<DataSource> rect_grid_file(new DataSource("grid_data", "file_path"));

  // Create a DataItem which describes the content of the data
  SharedPtr<DataItem> file_item(new DataItem(FormatType::BINARY_FORMAT, DTypes::FLOAT64, rect_grid_file, file_n_dims,
                                             file_dims));

  // Create a geometry which will point to the file
  SharedPtr<Geometry> file_geom(new Geometry(GeometryType::XY_GEOMETRY_TYPE));
  file_geom->addDataItem(file_item);

  file_dom->setGeometry(file_geom);

  // Create group for the variables defined in the geospatial domain
  SharedPtr<Group> rect_grid_vars(new Group("rect_grid_vars", GroupType::SPATIAL_GROUP_TYPE, file_dom));

  rect_grid_vars->addVariable("rect_var", DTypes::INT32);

  // add the groups of variables to the time series
  time_group->addGroup(rect_grid_vars);

  // Write to disk
  meta.save(filepath);

  printf("%d timeteps written in %s\n", n_timesteps, filepath);

  return 0;
}

int write_time_varying(const char* filepath, int n_attributes, int n_timesteps){
  XIdxFile meta;

  // Create a group to collect a time series
  SharedPtr<Group> time_group(new Group("TimeSeries", GroupType::TEMPORAL_GROUP_TYPE, "time_%04d"));

  meta.addGroup(time_group);

  const int n_dims = 3;

  // Create the time domain
  SharedPtr<Domain> time_dom;

  // Create series of timestep values
  time_dom = SharedPtr<TemporalListDomain>(new TemporalListDomain("Time"));

  for(int i=0; i < n_timesteps; i++){
    std::dynamic_pointer_cast<TemporalListDomain>(time_dom)->addDomainItem(float(i+10));
  }

  // Set the time group domain to use the time domain we just created
  time_group->setDomain(time_dom);

  // Create a grid and group of variables for every timestep
  for(int t=0; t < n_timesteps; t++){
    // Create a data source for this timestep
    SharedPtr<DataSource> file(new DataSource("timestep"+std::to_string(t),
                                                    "timestep"+std::to_string(t)+"/file_path"));
    // Create a new group to collect a set of variables that share the same spatial domain
    SharedPtr<Group> grid(new Group("L0", GroupType::SPATIAL_GROUP_TYPE, VariabilityType::VARIABLE_VARIABILITY_TYPE)); // default static group

    grid->addDataSource(file);

    // Create a spatial domain
    SharedPtr<SpatialDomain> space_dom(new SpatialDomain("Grid"));

    uint32_t dims[6] = {10, 20, 30};                     // X Y Z dimensions of the box
    double box_phy[6] = {0.3, 4.2, 0.0, 9.4, 2.5, 19.0}; // physical box (p1x,p2x,p1y,p2y,p1z,p2z)

    // Set topology and geometry of the spatial domain
    SharedPtr<Topology> topology(new Topology(TopologyType::CORECT_3D_MESH_TOPOLOGY_TYPE, n_dims,
                                              dims));
    space_dom->setTopology(topology);
    SharedPtr<Geometry> geometry(new Geometry(GeometryType::RECT_GEOMETRY_TYPE, n_dims, box_phy));
    space_dom->setGeometry(geometry);

    // Set the domain for the spatial group
    grid->setDomain(space_dom);

    // add some variables to the spatial group
    for(int i=0; i < n_attributes; i++){
      char name[32];
      sprintf(name, "var_%d", i);
      grid->addVariable(name, DTypes::FLOAT32);
    }

    time_group->addGroup(grid);
  }

  meta.save(filepath);

  printf("%d timeteps written in %s\n", n_timesteps, filepath);

  return 0;

}

int main(int argc, char** argv){

  if(argc == 2 && strncmp(argv[1],"help",4)==0){
    fprintf(stderr, "Usage: write [n_attributes] [n_timesteps] [n_levels]\n");

    return 1;
  }

  int n_attributes = 4;
  int n_timesteps = 3;
  int n_levels = 2;

  if(argc > 1)
    n_attributes = atoi(argv[1]);

  if(argc > 2)
    n_timesteps = atoi(argv[2]);

  // TODO n_levels not used, create an AMR example
  if(argc > 3)
    n_levels = atoi(argv[3]);

  clock_t start, finish;
  start = clock();
  
  int ret = write_temporal_hyperslab_reg_grid("temporal_hyperslab_reg_grid.xidx", n_attributes, n_timesteps);
  VisusAssert(ret==0);

  ret = write_temporal_list_multiaxis("temporal_list_multiaxis.xidx", n_attributes, n_timesteps);
  VisusAssert(ret==0);

  ret = write_temporal_list_binary_axis("temporal_list_binary_axis.xidx", n_attributes, n_timesteps);
  VisusAssert(ret==0);

  ret = write_time_varying("time_varying.xidx", n_attributes, n_timesteps);
  VisusAssert(ret==0);

  finish = clock();

  printf("Time taken %fms\n",(double(finish)-double(start))/CLOCKS_PER_SEC);

  return ret;
}
