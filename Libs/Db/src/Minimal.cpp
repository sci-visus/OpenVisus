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
#include <Visus/Access.h>
#include <Visus/IdxFile.h>
#include <Visus/IdxDiskAccess.h>

#include <Visus/Minimal.h>

namespace Visus {

  
///////////////////////////////////////////////////
MinimalAccess::~MinimalAccess()
{
  auto access_ptr = static_cast<SharedPtr<Access>*>(this->pimpl);
  delete access_ptr;
}

///////////////////////////////////////////////////
MinimalDataset::~MinimalDataset()
{
  auto dataset_ptr = static_cast<SharedPtr<IdxDataset>*>(this->pimpl);
  delete dataset_ptr;
}

///////////////////////////////////////////////////
void MinimalDataset::Create(std::string idx_filename, std::string dtype, int N_x, int N_y, int N_z)
{
  IdxFile idxfile;
  idxfile.logic_box = BoxNi(PointNi(0, 0, 0), PointNi(N_x, N_y, N_z));
  Field field("myfield", DType::fromString(dtype));
  field.default_compression = "";
  field.default_layout = "";
  idxfile.fields.push_back(field);
  idxfile.save(idx_filename);
}

///////////////////////////////////////////////////
MinimalDataset* MinimalDataset::Load(std::string filename)
{
  return new MinimalDataset(new SharedPtr<IdxDataset>(LoadIdxDataset(filename)));
}



///////////////////////////////////////////////////
MinimalAccess* MinimalDataset::createAccess()
{
  auto dataset = *static_cast<SharedPtr<IdxDataset>*>(this->pimpl);
  auto access = new IdxDiskAccess(dataset.get());
  access->disableAsync();
  access->disableWriteLock();
  return new MinimalAccess(new SharedPtr<Access>(access));
}

///////////////////////////////////////////////////
void MinimalDataset::writeData(MinimalAccess* access_, int x1, int y1, int z1, int x2, int y2, int z2, Uint8* buffer, int buffer_size)
{
  auto dataset = *static_cast<SharedPtr<IdxDataset>*>(this->pimpl);
  auto access= *static_cast<SharedPtr<Access>*>(access_->pimpl);
  auto box = BoxNi(PointNi(x1, y1, z1), PointNi(x2, y2, z2));
  std::shared_ptr<BoxQuery> query = dataset->createBoxQuery(box, 'w');
  query->accuracy = dataset->getDefaultAccuracy();
  dataset->beginBoxQuery(query);
  VisusReleaseAssert(query->isRunning());
  VisusReleaseAssert(buffer_size == query->field.dtype.getByteSize(box.size()));
  query->buffer = Array(query->getNumberOfSamples(), query->field.dtype, HeapMemory::createUnmanaged(buffer, buffer_size));
  VisusReleaseAssert(dataset->executeBoxQuery(access, query));

}

///////////////////////////////////////////////////
void MinimalDataset::readData(MinimalAccess* access_, int x1, int y1, int z1, int x2, int y2, int z2, Uint8* buffer, int buffer_size)
{
  auto dataset = *static_cast<SharedPtr<IdxDataset>*>(this->pimpl);
  auto access = *static_cast<SharedPtr<Access>*>(access_->pimpl);
  auto box = BoxNi(PointNi(x1, y1, z1), PointNi(x2, y2, z2));
  std::shared_ptr<BoxQuery> query = dataset->createBoxQuery(box, 'r');
  query->accuracy = dataset->getDefaultAccuracy();
  dataset->beginBoxQuery(query);
  VisusReleaseAssert(query->isRunning());
  VisusReleaseAssert(buffer_size == query->field.dtype.getByteSize(box.size()));
  VisusReleaseAssert(dataset->executeBoxQuery(access, query));
  memcpy(buffer,query->buffer.c_ptr(),buffer_size);
}

///////////////////////////////////////////////////
void InitMinimalModule()
{
  DbModule::attach();
}

///////////////////////////////////////////////////
void ShutdownMinimalModule()
{
  DbModule::detach();
}

} //namespace Visus