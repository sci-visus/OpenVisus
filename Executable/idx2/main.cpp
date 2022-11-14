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
#include <Visus/CloudStorageAccess.h>
#include <Visus/MultiplexAccess.h>


using namespace Visus;



//////////////////////////////////////////////////////////////////////////////
int main(int argn, const char* argv[])
{
  SetCommandLine(argn, argv);
  DbModule::attach();

  //fake value for bitsperblock
  //async disabled
  //compression 'raw' (i.e. you will do the encoding/decoding yourself)
  //do not change the bucket name `test-idx2` that must exists
  String config = concatenate(
    "<access type='CloudStorageAccess' disable_async='1' url='https://s3.us-west-1.wasabisys.com/test-idx2/visus.idx?profile=wasabi' bitsperblock='16'  compression='raw' layout='rowmajor'/>");

  auto cloud = std::make_shared<CloudStorageAccess>(/*dataset*/nullptr, StringTree::fromString(config));

  //id for the block, please try to change it
  BigInt blockid = 0;

  //buffer for writing staff (NOTE: since I am dtype agnostic here, I always use uint8)
  auto write_buffer = Array(256, DTypes::UINT8);

  auto write = std::make_shared<BlockQuery>();
  write->blockid = blockid; 
  write->H = -1;//current resolution (invalid)
  write->logic_samples = LogicSamples::invalid();
  write->field = Field("my-field", DTypes::UINT8);
  write->time = 0;
  write->buffer = write_buffer;
  cloud->writeBlock(write);
  PrintInfo("Write Query", write->ok(), write->errormsg);

  auto read = std::make_shared<BlockQuery>();
  read->blockid = blockid; //id for the block
  read->H = -1;//current resolution (invalid)
  read->logic_samples = LogicSamples::invalid();
  read->field = Field("my-field", DTypes::UINT8);
  read->time = 0;
  read->buffer = Array();
  cloud->readBlock(read);
  PrintInfo("Read", read->ok(), read->errormsg);
  auto read_buffer = read->buffer;

  //check
  VisusReleaseAssert(HeapMemory::equals(write_buffer.heap, read_buffer.heap));

  DbModule::detach();
  return 0;
}
