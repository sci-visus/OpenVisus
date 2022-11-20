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
#include <Visus/CloudStorage.h>

#if VISUS_PYTHON
#include <Visus/Python.h>
#endif

using namespace Visus;

/// /////////////////////////////////////////////////////////////////
void TestCloudStorage(String connection_string)
{
	auto body_small = Utils::loadBinaryDocument("datasets/cat/gray.png");
	VisusReleaseAssert(body_small);

	//to check chunked download
	auto body_big = std::make_shared<HeapMemory>();
	VisusReleaseAssert(body_big->resize(256 * 1024,__FILE__,__LINE__));
	for (int I = 0; I < body_big->c_size(); I++)
		body_big->c_ptr()[I] = (Uint8)I;

	srand((unsigned int)time(0));

	StringMap metadata;
	metadata.setValue("example-metadata1", "value1");
	metadata.setValue("example-metadata2", "value2");

	auto net = std::make_shared<NetService>(1);
	auto cloud = CloudStorage::createInstance(connection_string);

	srand((int)time(0));
	auto path = Url(connection_string).getPath();

	// if it does not exist, create it (NOTE: if it exists, I don't care)
	String bucket = StringUtils::split(path, "/")[0];


	//on AWS and Wasabi it returns ok even if it exists, on OSN it will return CONFLICT if it exists
	//so here I am ingnoring the errors
	cloud->addBucket(net, bucket).get(); 

	VisusReleaseAssert(cloud->addBlob(net, CloudStorageItem::createBlob(path + "/small.png"        , body_small, metadata)).get());
	VisusReleaseAssert(cloud->addBlob(net, CloudStorageItem::createBlob(path + "/dir1/dir2/big.bin", body_big  , metadata)).get());

	auto goodMetadata=[](StringMap a, StringMap b) {
		for (auto it : a)
			if (it.second!=b.getValue(it.first))
				return false;
		return true;
	};

	{
		auto blob = cloud->getBlob(net, path + "/small.png").get();
		VisusReleaseAssert(HeapMemory::equals(blob->body, body_small));
		VisusReleaseAssert(goodMetadata(metadata, blob->metadata));
	}

	{
		auto blob = cloud->getBlob(net, path + "/dir1/dir2/big.bin").get();
		VisusReleaseAssert(HeapMemory::equals(blob->body, body_big));
		VisusReleaseAssert(goodMetadata(metadata, blob->metadata));
	}

	SharedPtr<CloudStorageItem> dir;

	dir = cloud->getDir(net, path).get();
	VisusReleaseAssert(dir->childs.size() == 2 && dir->hasChild(path + "/dir1") && dir->hasChild(path + "/small.png"));

	dir = cloud->getDir(net, path + "/dir1").get();
	VisusReleaseAssert(dir->childs.size() == 1 && dir->hasChild(path + "/dir1/dir2"));

	dir = cloud->getDir(net, path + "/dir1/dir2").get();
	VisusReleaseAssert(dir->childs.size()==1 && dir->hasChild(path + "/dir1/dir2/big.bin"));

	VisusReleaseAssert(cloud->deleteBlob(net, path + "/small.png", Aborted()).get());
	VisusReleaseAssert(cloud->deleteBlob(net, path + "/dir1/dir2/big.bin", Aborted()).get());

	dir = cloud->getDir(net, path).get();
	VisusReleaseAssert(!dir);

	//too dangerous (e.g. on OSN it could exists)
	//cloud->deleteBucket(net, bucket).get();
}


//////////////////////////////////////////////////////////////////////////////
int main(int argn, const char* argv[])
{
  auto T1 = Time::now();
	String action = argn>=2? String(argv[1]): String("");

  SetCommandLine(argn, argv);

#if VISUS_IDX2
	// visus idx2 --encode Miranda-Viscosity-[384-384-256]-Float64.raw --tolerance 1e-16 --num_levels 2 --out_dir .
	// visus idx2 --decode Miranda/Viscosity.idx2 --downsampling 1 1 1 --tolerance 0.001
	// python Executable/visus/idx2_convert.py Miranda-Viscosity-[193-193-129]-float64-accuracy-0.001000.raw
	{
		if (action == "idx2")
		{
			std::vector<const char*> v;
			for (int I = 2; I < argn; I++)
				argv[I - 1] = argv[I];
			return Idx2App(argn - 1, argv);
		}
	}
#endif

	if (action == "test-cloud-storage")
	{
		//create a root access key (has full rights on your AWS resources) 
		auto connection_string= argv[2];
		DbModule::attach();
		TestCloudStorage(connection_string);
		DbModule::detach();
		return 0;
	}

	//switch to visus convert
#if VISUS_PYTHON
  EmbeddedPythonInit();
  auto acquire_gil = PyGILState_Ensure();
  PyRun_SimpleString("from OpenVisus import *");
  PyGILState_Release(acquire_gil);
#else
  DbModule::attach();
#endif

  auto args = std::vector<String>(CommandLine::args.begin() + 1, CommandLine::args.end());
  VisusConvert().runFromArgs(args);
  PrintInfo("All done in ",T1.elapsedSec(),",seconds");
  DbModule::detach();

#if VISUS_PYTHON
  EmbeddedPythonShutdown();
#endif

  return 0;
}
