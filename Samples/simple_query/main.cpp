/*-----------------------------------------------------------------------------
Copyright(c) 2010 - 2018 ViSUS L.L.C.,
Scientific Computing and Imaging Institute of the University of Utah

ViSUS L.L.C., 50 W.Broadway, Ste. 300, 84101 - 2044 Salt Lake City, UT
University of Utah, 72 S Central Campus Dr, Room 3750, 84112 Salt Lake City, UT

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

For additional information about this project contact : pascucci@acm.org
For support : support@visus.net
-----------------------------------------------------------------------------*/

#include <Visus/IdxDataset.h>
#include <Visus/ApplicationInfo.h>

#ifdef WIN32
#pragma warning(disable:4996)
#endif

using namespace Visus;

/**
  This application executes a remote (or local) box query
  and dumps on disk a raw file containing the data.
**/
int main(int argc, const char* argv[])
{

  SetCommandLine(argc,argv);
  IdxModule::attach();

  DoAtExit do_at_exit([]{
    IdxModule::detach();
  });

  auto args=ApplicationInfo::args;

  std::string dataset_url = "http://atlantis.sci.utah.edu/mod_visus?dataset=2kbit1";
  std::string fieldname = "";
  std::string outname = "dump";

  int p1_in[3] = {0,0,0};
  int p2_in[3] = {64,64,64};
  int res = -1;

  int timestate = 0;

  for (int I=1;I<(int)args.size();)
  {
    String cmd=StringUtils::toLower(args[I++]);
    if (StringUtils::startsWith(cmd,"--"))
      cmd=cmd.substr(2);

    if(cmd=="dataset")
      dataset_url = args[I];
    else if(cmd=="p1")
      sscanf(args[I].c_str(), "%ux%ux%u", &p1_in[0], &p1_in[1], &p1_in[2]);
    else if(cmd=="p2")
      sscanf(args[I].c_str(), "%ux%ux%u", &p2_in[0], &p2_in[1], &p2_in[2]);
    else if(cmd=="time")
      timestate = atoi(args[I].c_str());
    else if(cmd=="field")
      fieldname = args[I];
    else if(cmd=="res")
      res = atoi(args[I].c_str());
    else if(cmd=="out")
      outname = args[I];
    else if(cmd=="help"){
      VisusInfo() << " example_query " << std::endl
      << "   [--dataset dataset_url]" << std::endl
      << "   [--field <string>]" << std::endl
      << "   [--res <int>]" << std::endl
      << "   [--p1 XXxYYxZZ]" << std::endl
      << "   [--p2 XXxYYxZZ]" << std::endl
      << "   [--time <int>]" << std::endl
      << "   [--out <string>]" << std::endl;
      return 0;
    }
  }

  auto dataset=Dataset::loadDataset(dataset_url);
  VisusReleaseAssert(dataset);

  NdBox world_box=dataset->getBox();

  auto max_resolution = dataset->getMaxResolution();

  if(res==-1) res=max_resolution;

  VisusInfo() << "Data size: " << world_box.p1.toString() << " " << world_box.p2.toString()
    << " max res: " << max_resolution;

  //any time you need to read/write data from/to a Dataset create an Access
  auto access=dataset->createAccess();

  auto query=std::make_shared<Query>(dataset.get(),'r');

  NdBox my_box;

  my_box.p1 = NdPoint(p1_in[0], p1_in[1], p1_in[2]);
  my_box.p2 = NdPoint::one(p2_in[0], p2_in[1], p2_in[2]);

  VisusInfo() << "Box query " << my_box.p1.toString() << " p2 " << my_box.p2.toString()
              << " variable " << fieldname << " time " << timestate;

  query->position = my_box;

  // Choose the field
  Field field;

  if(fieldname!="")
    field = dataset->getFieldByName(fieldname);
  else
    field = dataset->getDefaultField();

  query->field = field;

  // Set timestep
  query->time = timestate;

  // Set resolution levels
  query->start_resolution = 0;
  query->end_resolutions.push_back(res);

  // You can add multiple resolutions values to end_resolutions
  // To execute queries at different resolutions and work on these progressive results use:
  // dataset->nextQuery(query);

  // In case you use lower resolutions you might want to set a merge_mode
  // query->merge_mode = Query::InterpolateSamples;

  query->max_resolution = max_resolution;

  VisusReleaseAssert(dataset->beginQuery(query));

  // Read the data
  VisusReleaseAssert(dataset->executeQuery(access,query));

  Array data = query->buffer;

  // Here we dump the data on disk in a .raw file
  if( data.c_ptr() != NULL)
    VisusInfo() << "Dumping data bytes " << data.c_size();
  else{
    VisusInfo() << "ERROR: No data";
    VisusReleaseAssert(false);
  }

  std::stringstream ss;
  ss << outname << "_" << query->buffer.getWidth() << "_"
     << query->buffer.getHeight() << "_" << query->buffer.getDepth() << "_" << field.dtype.toString() << ".raw";

  std::ofstream out_file;
  out_file.open(ss.str(), std::ios::out | std::ios::binary);
  out_file.write( (char*)data.c_ptr(), data.c_size());
  out_file.close();
}

