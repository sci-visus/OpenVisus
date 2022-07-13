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

#include <Visus/GoogleMapsAccess.h>

namespace Visus {

  ////////////////////////////////////////////////////////////////////
GoogleMapsAccess::GoogleMapsAccess(GoogleMapsDataset * dataset_, String tiles_url, SharedPtr<NetService> netservice)
  : dataset(dataset_)
{
  this->name = "GoogleMapsAccess";
  this->can_read = true;
  this->can_write = false;
  this->bitsperblock = dataset->getDefaultBitsPerBlock();
  this->tiles_url = tiles_url;
  this->netservice = netservice;
}

////////////////////////////////////////////////////////////////////
void GoogleMapsAccess::readBlock(SharedPtr<BlockQuery> query)
{
  //I have only even levels
  VisusReleaseAssert((query->H % 2) == 0);

  auto p0 = query->logic_samples.logic_box.p1;
  auto block_logic_size = dataset->block_samples[query->H].logic_box.size();
  auto block_nsamples   = dataset->block_samples[query->H].nsamples;
  auto block_coord = p0.innerDiv(block_logic_size);

  auto X = block_coord[0];
  auto Y = block_coord[1];
  auto Z = (query->H - bitsperblock) >> 1; //I have only even levels

	//mirror along Y
	Y = (int)((Int64(1) << Z) - Y - 1);

	auto url = Url(this->tiles_url);
	url.setParam("x", cstring(X));
	url.setParam("y", cstring(Y));
	url.setParam("z", cstring(Z));

	auto request = NetRequest(url);

	if (!request.valid())
		return readFailed(query, "request not valid");

	request.aborted = query->aborted;

	//note [...,query] keep the query in memory
	NetService::push(netservice, request).when_ready([this, query, block_nsamples](NetResponse response) {

		DType response_dtype = DTypes::UINT8_RGB;

		response.setHeader("visus-compression", query->field.default_compression);
		response.setHeader("visus-nsamples", block_nsamples.toString());
		response.setHeader("visus-dtype", response_dtype.toString());
		response.setHeader("visus-layout", "");

		if (query->aborted())
			return readFailed(query, "aborted");

		if (!response.isSuccessful())
			return readFailed(query, "response not valid");

		auto decoded = response.getCompatibleArrayBody(query->getNumberOfSamples(), response_dtype);
		if (!decoded.valid())
			return readFailed(query, "cannot decode array");

		//need to cast?
		if (query->field.dtype != response_dtype)
			decoded = ArrayUtils::smartCast(decoded, query->field.dtype, query->aborted);

		query->buffer = decoded;

		return readOk(query);
		});
}

} //namespace Visus 

