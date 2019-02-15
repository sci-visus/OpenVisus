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

#include <time.h>

#include "Visus/VisusXIdx.h"

using namespace Visus;

////////////////////////////////////////////////////////////////////////////////////////////
int main(int argn, const char** argv) {

  SetCommandLine(argn, argv);
  XIdxModule::attach();

  if (argn < 2) {
    VisusInfo()<<"Usage: read file_path [debug]";
    return -1;
  }

  auto t1 = Time::now();

  auto metadata = XIdxFile::load(std::string(argv[1]));

  auto time_group = metadata->getGroup(GroupType::TEMPORAL_GROUP_TYPE);
  //auto time_domain = std::static_pointer_cast<HyperSlabDomain>(root_group->domain);
  auto time_domain = std::static_pointer_cast<TemporalListDomain>(time_group->domain);

  VisusInfo() << "Time Domain " << time_domain->type.toString();

  for (auto& att : time_domain->attributes)
    VisusInfo() << "\t\tAttribute " << att->name << " value " << att->value;

  int t_count = 0;
  for (auto t : time_domain->getLinearizedIndexSpace()) 
  {
    VisusInfo() << "Timestep " << t;

    auto grid = time_group->getGroup(t_count++);
    auto spatial_domain = grid->domain;

    VisusInfo() << "\tGrid Domain[" << spatial_domain->type.toString() << "]";

    for (auto& att : spatial_domain->attributes)
      VisusInfo() << "\t\tAttribute " << att->name << " value " << att->value;

    if (spatial_domain->type == DomainType::SPATIAL_DOMAIN_TYPE) 
    {
      auto sdom = std::dynamic_pointer_cast<SpatialDomain>(spatial_domain);
      VisusInfo() << "\tTopology " << sdom->topology->type.toString() << " volume " << sdom->getVolume();
      VisusInfo() << "\tGeometry " << sdom->geometry->type.toString();
    }
    else if (spatial_domain->type == DomainType::MULTIAXIS_DOMAIN_TYPE)
    {
      auto mdom = std::dynamic_pointer_cast<MultiAxisDomain>(spatial_domain);
      for (auto& axis : mdom->axis) {
        VisusInfo() << "\tAxis " << axis->name << " volume " << axis->getVolume();

        // print axis values
        for (auto v : axis->getValues())
          VisusInfo() << v;

        for (auto& att : axis->attributes)
          VisusInfo() << "\t\tAttribute " << att->name << " value " << att->value;
      }
    }

    VisusInfo() << "";

    for (auto& var : grid->variables) 
    {
      auto source = var->data_items[0]->findDataSource();
      VisusInfo() << "\t\tVariable: " << var->name <<  "data source url:" << (source ?source->url:"");

        for (auto att : var->attributes) 
          VisusInfo() << "\t\t\tAttribute " << att->name << " value " << att->value;
    }

  }

  // (Debug) Saving the content in a different file to compare with the original
  metadata->save("verify.xidx");

  VisusInfo() << "output saved into verify.xidx";

  XIdxModule::detach();

  return 0;
}
