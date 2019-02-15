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

#include <libxml/xmlreader.h>
#include <libxml/xinclude.h>
#include <time.h>

#include "xidx/xidx.h"

using namespace xidx;

int main(int argc, char** argv){

  if(argc < 2){
    fprintf(stderr, "Usage: read file_path [debug]\n");
    return 1;
  }

  clock_t start, finish;
  start = clock();

  MetadataFile metadata(argv[1]);

  int ret = metadata.Load();

  finish = clock();

  printf("Time taken %fms\n",(double(finish)-double(start))/CLOCKS_PER_SEC);

  std::shared_ptr<Group> root_group = metadata.getRootGroup();
  
  std::shared_ptr<Domain> time_domain = root_group->getDomain();
  
  std::shared_ptr<TemporalListDomain> domain = std::static_pointer_cast<TemporalListDomain>(time_domain);
  
  printf("Time Domain[%s]:\n", Domain::toString(domain->getType()));
  for(auto& att: domain->getAttributes())
    printf("\t\tAttribute %s value %s\n", att->name.c_str(), att->value.c_str());
  
  int t_count=0;
  for(auto t : domain->getLinearizedIndexSpace()){
    printf("Timestep %f\n", t);

    auto& grid = root_group->getGroup(t_count++);
    std::shared_ptr<Domain> domain = grid->getDomain();
    
    printf("\tGrid Domain[%s]:\n", Domain::toString(domain->getType()));
    
    for(auto& att: domain->getAttributes())
      printf("\t\tAttribute %s value %s\n", att->name.c_str(), att->value.c_str());
    
    if(domain->getType() == Domain::DomainType::SPATIAL_DOMAIN_TYPE){
      std::shared_ptr<SpatialDomain> sdom = std::dynamic_pointer_cast<SpatialDomain>(domain);
      printf("\tTopology %s volume %lu\n", Topology::toString(sdom->topology.type), sdom->getVolume());
      printf("\tGeometry %s", Geometry::toString(sdom->geometry.type));
    }
    else if(domain->getType() == Domain::DomainType::MULTIAXIS_DOMAIN_TYPE)
    {
      std::shared_ptr<MultiAxisDomain> mdom = std::dynamic_pointer_cast<MultiAxisDomain>(domain);
      for(int a=0; a < mdom->getNumberOfAxis(); a++){
        const Axis& axis = mdom->getAxis(a);
        printf("\tAxis %s volume %lu: [ ", axis.name.c_str(), axis.getVolume());
        
        // print axis values
        for(auto v: axis.getValues())
          printf("%f ", v);
        printf("]\n");
        
        for(auto& att: axis.getAttributes())
          printf("\t\tAttribute %s value %s\n", att->name.c_str(), att->value.c_str());
      }
    }
    
    printf("\n");
    
    for(auto& var: grid->getVariables()){
      auto source = var->getDataItems()[0]->getDataSource();
      printf("\t\tVariable: %s ", var->name.c_str());
      if(source != nullptr)
        printf("data source url: %s\n", source->getUrl().c_str());
      else printf("\n");
      
      for(auto att: var->getAttributes()){
        printf("\t\t\tAttribute %s value %s\n", att->name.c_str(), att->value.c_str());
      }
    }
    
  }
  
  // (Debug) Saving the content in a different file to compare with the original
  metadata.save("verify.xidx");

  return ret;
}
