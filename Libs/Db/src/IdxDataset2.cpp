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


#if VISUS_IDX2

#include <Visus/IdxDataset2.h>
#include <Visus/IdxHzOrder.h>

#include <idx2_lib.h>
using namespace idx2;

namespace Visus {

////////////////////////////////////////////////////////////////////
SharedPtr<BoxQuery> IdxDataset2::createBoxQuery(BoxNi logic_box, Field field, double time, int mode, Aborted aborted)
{
  return Dataset::createBoxQuery(logic_box, field, time, mode, aborted); //should be the same
}

////////////////////////////////////////////////////////////////////
bool IdxDataset2::setBoxQueryEndResolution(SharedPtr<BoxQuery> query, int value)
{
  PrintInfo("IdxDataset2::setBoxQueryEndResolution");
  VisusAssert(query->end_resolution < value);
  query->end_resolution = value;
  //query->logic_samples = LogicSamples(logic_box, delta);
  VisusAssert(query->logic_samples.valid());
  return true;
}



////////////////////////////////////////////////////////////////////
void IdxDataset2::beginBoxQuery(SharedPtr<BoxQuery> query)
{
  PrintInfo("IdxDataset2::beginBoxQuery");  
  
  if (!query)
    return;

  if (query->getStatus() != Query::QueryCreated)
    return;

  if (query->aborted())
    return query->setFailed("query aborted");

  if (!query->field.valid())
    return query->setFailed("field not valid");

  // override time from field
  if (query->field.hasParam("time"))
    query->time = cdouble(query->field.getParam("time"));

  if (!getTimesteps().containsTimestep(query->time))
    return query->setFailed("wrong time");

  if (!query->logic_box.valid())
    return query->setFailed("query logic_box not valid");

  if (!query->logic_box.getIntersection(this->getLogicBox()).isFullDim())
    return query->setFailed("user_box not valid");

  if (query->end_resolutions.empty())
    query->end_resolutions = { this->getMaxResolution() };

 
  for (auto it : query->end_resolutions)
  {
    if (!(it>=0 && it<this->getMaxResolution()))
      return query->setFailed("wrong end resolution");
  }

  if (query->start_resolution > 0 && (query->end_resolutions.size() != 1 || query->start_resolution != query->end_resolutions[0]))
    return query->setFailed("wrong query start resolution");

  for (auto it : query->end_resolutions)
  {
    if (setBoxQueryEndResolution(query, it))
      return query->setRunning();
  }

  query->setFailed("cannot find a good end_resolution to start with");
}

////////////////////////////////////////////////////////////////////
bool IdxDataset2::executeBoxQuery(SharedPtr<Access> access, SharedPtr<BoxQuery> query)
{
  PrintInfo("IdxDataset2::executeBoxQuery");

  if (!query)
    return false;

  if (!(query->isRunning() && query->getCurrentResolution() < query->getEndResolution()))
    return false;

  if (query->aborted())
  {
    query->setFailed("query aborted");
    return false;
  }

  if (!query->allocateBufferIfNeeded())
  {
    query->setFailed("cannot allocate buffer");
    return false;
  }

  //execute it!
  //....

  VisusAssert(query->buffer.dims == query->getNumberOfSamples());
  query->setCurrentResolution(query->end_resolution);
  return true;
}


////////////////////////////////////////////////////////////////////
void IdxDataset2::nextBoxQuery(SharedPtr<BoxQuery> query)
{
  PrintInfo("IdxDataset2::nextBoxQuery");

  if (!query)
    return;

  if (!(query->isRunning() && query->getCurrentResolution() == query->getEndResolution()))
    return;

  //reached the end?
  if (query->end_resolution == query->end_resolutions.back())
    return query->setOk();

  auto failed = [&](String reason) {
    return query->setFailed(query->aborted() ? "query aborted" : reason);
  };

  auto Rcurrent_resolution = query->getCurrentResolution();
  auto Rsamples = query->logic_samples;
  auto Rbuffer = query->buffer;
  auto Rfilter_query = query->filter.query;

  if (!setBoxQueryEndResolution(query, query->end_resolutions[Utils::find(query->end_resolutions, query->end_resolution) + 1]))
    VisusReleaseAssert(false);

  //no merging
  query->buffer = Array();
  query->setCurrentResolution(Rcurrent_resolution);

}



////////////////////////////////////////////////////////////////////
void IdxDataset2::readDatasetFromArchive(Archive& ar)
{
  //TODO better (!!!)
  String url = ar.readString("url");

  idx2_file idx2;

  SetDir(&idx2, ".");

  VisusReleaseAssert(ReadMetaFile(&idx2, url.c_str()));
  VisusReleaseAssert(Finalize(&idx2));
  //Decode(idx2, P);
  //Dealloc(&idx2);

  DType dtype;
  if (idx2.DType == dtype::float64)
    dtype = DTypes::FLOAT64;
  else if (idx2.DType == dtype::float32)
    dtype = DTypes::FLOAT32;
  else
    ThrowException("internal error");

  PointNi dims(idx2.Dims3[0], idx2.Dims3[1], idx2.Dims3[2]);

  IdxFile idx1;
  idx1.version = 20;
  idx1.logic_box=BoxNi(PointNi(0,0,0),dims);
  idx1.bounds= idx1.logic_box;
  idx1.fields.push_back(Field(idx2.Field,dtype));
  idx1.validate(url);

  this->dataset_body = StringTree();
  this->idxfile = idx1;
  this->bitmask = idx1.bitmask;
  this->default_bitsperblock = idx1.bitsperblock;
  this->logic_box = idx1.logic_box;
  this->timesteps = idx1.timesteps;

  setDatasetBounds(idxfile.bounds);

  for (auto field : idxfile.fields)
    addField(field);

#endif
}

} //namespace Visus


#endif //#if VISUS_IDX2
