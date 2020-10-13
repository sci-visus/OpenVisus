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

#include <Visus/Db.h>
#include <Visus/VisusConvert.h>
#include <Visus/RamResource.h>

#if VISUS_PYTHON
#include <Visus/Python.h>
#endif

using namespace Visus;

int TestBug()
{
  /*
      <dataset typename="IdxDataset">
    <idxfile>
      <version value="6" />
      <bitmask value="V001012012012012012012012012012012" />
      <box value="0 3788 0 2047 0 1000" />
      <bitsperblock value="16" />
      <blocksperfile value="512" />
      <block_interleaving value="0" />
      <filename_template value="./test/%02x/%04x.bin" />
      <missing_blocks value="False" />
      <field name="myfield" index="0" default_layout="" dtype="uint16" />
      <timestep when="0" />
    </idxfile>
  </dataset>
      */

  auto db = LoadDataset("test.idx");
  VisusReleaseAssert(db);

  //IdxFile idxfile;
  //idxfile.logic_box = BoxNi(PointNi(0, 0, 0), PointNi(19456, 12492, 1000));
  //idxfile.fields = { Field("myfield", DTypes::UINT16) };
  //idxfile.save("tmp/huge.idx");
  //auto db = LoadDataset("tmp/huge.idx");

  auto SetEnv = [](String key, String value) {
#if WIN32
    _putenv_s(key.c_str(), value.c_str());
#else
    setenv(key.c_str(), value.c_str(), 1);
#endif

  };

  SetEnv("VISUS_IDX_SKIP_READING", "1");
  SetEnv("VISUS_IDX_DISABLE_ASYNC", "1");

  auto access = db->createAccess();
  Int64 query_size = 0;

  auto ram = RamResource::getSingleton();

  //I should get a number of samples equals to the number of samples written in tutorial 1
  for (int H = 29; H <= db->getMaxResolution(); H++)
  {
    auto t1 = Time::now();
    query_size = 0;
    PrintInfo("H", H);
    PrintInfo("Executing query Used memory", StringUtils::getStringFromByteSize(ram->getVisusUsedMemory()), "...");
    auto query = db->createBoxQuery(db->getLogicBox(), 'r');
    query->end_resolutions = { H };
    db->beginBoxQuery(query);
    VisusReleaseAssert(query->isRunning());
    db->executeBoxQuery(access, query);
    query_size = query->getByteSize();
    PrintInfo("DONE",
      "query_nsamples", query->getNumberOfSamples(),
      "query_size", StringUtils::getStringFromByteSize(query_size),
      "Used memory", StringUtils::getStringFromByteSize(ram->getVisusUsedMemory()),
      "Peak memory", StringUtils::getStringFromByteSize(ram->getPeakMemory())
    );
    PrintInfo("done in", t1.elapsedMsec(), "msec\n");
  }

  return 0;
}

//////////////////////////////////////////////////////////////////////////////
int main(int argn, const char* argv[])
{
  auto T1 = Time::now();

  SetCommandLine(argn, argv);

#if VISUS_PYTHON
  EmbeddedPythonInit();
  auto acquire_gil = PyGILState_Ensure();
  PyRun_SimpleString("from OpenVisus import *");
  PyGILState_Release(acquire_gil);
#else
  DbModule::attach();
#endif

  return TestBug();

  auto args = std::vector<String>(CommandLine::args.begin() + 1, CommandLine::args.end());
  VisusConvert().runFromArgs(args);
  PrintInfo("All done in ",T1.elapsedSec(),",seconds");
  DbModule::detach();

#if VISUS_PYTHON
  EmbeddedPythonShutdown();
#endif

  return 0;
}
