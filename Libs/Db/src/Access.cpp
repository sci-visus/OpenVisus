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

#include <Visus/Access.h>
#include <Visus/IdxHzOrder.h>
#include <Visus/Dataset.h>

namespace Visus {
const String Access::DefaultChMod = "rw";


///////////////////////////////////////////////////////////////////////////////////////
String Access::getBlockFilename(Dataset* dataset, int bitsperblock, String filename_template, Field field, double time, String compression, BigInt blockid, bool reverse_filename) 
{
  String fieldname = StringUtils::removeSpaces(field.name);

  String ret = filename_template;

  ret = StringUtils::replaceFirst(ret, "$(field)", fieldname.length() < 32 ? StringUtils::onlyAlNum(fieldname) : StringUtils::computeChecksum(fieldname));
  ret = StringUtils::replaceFirst(ret, "$(time)", StringUtils::onlyAlNum(int(time) == time ? cstring((int)time) : cstring(time)));
  ret = StringUtils::replaceFirst(ret, "$(compression)", compression);

  //NOTE 16x is enough for 16*4 bits==64 bit for block number
  //     splitting by 4 means 2^16= 64K files inside a directory which seams reasonable with max 64/16=4 levels of directories
  {
    //auto s_blockid = StringUtils::formatNumber("%016x", blockid); WRONG for int64 (!)
    std::ostringstream out;
    out << std::setw(16) << std::hex << std::setfill('0') << blockid;
    auto s_blockid = out.str();

    ret = StringUtils::replaceFirst(ret, "$(block:%016x)", s_blockid);
    ret = StringUtils::replaceFirst(ret, "$(block:%016x:%04x)", StringUtils::join(StringUtils::splitInChunks(s_blockid, 4), "/"));
  }

  //zarr (e.g. visus/$(time)/$(field)/$(level)/$(block-offset)  (block-offset == Z.Y.X resolution==[0,MaxH])
  if (StringUtils::contains(ret, "$(block-offset)") || StringUtils::contains(ret, "$(level)"))
  {
    //TODO: could this part be too slow?
    auto bitmask = dataset->getBitmask();
    HzOrder hzorder(bitmask);

    auto hzaddress = blockid << bitsperblock;
    PointNi block_p1 = hzorder.getPoint(hzaddress);
    auto pdim = bitmask.getPointDim();
    int H;
    auto block_samples=dataset->getBlockQuerySamples(blockid, H);
    auto level_samples = dataset->getLevelSamples(H);
    auto level_p1 = level_samples.logic_box.p1;

    std::vector< Int64> v(pdim);
    for (int I = 0; I < pdim; I++)
    {
      auto block_delta = block_samples.nsamples[I] * block_samples.delta[I];
      VisusAssert((block_p1[I] - level_p1[I]) % block_delta == 0);
      v[I] = (block_p1[I] - level_p1[I]) / block_delta;
    }



    //zarr use ZXY
    std::reverse(v.begin(), v.end());

    auto nchannels = field.dtype.ncomponents();
    if (nchannels > 1)
      v.push_back(0); // e.g. in uint8[3] the RGB will alwasy be together

    const String separator = "."; //TODO other separator
    String key=StringUtils::join(v, separator);
    ret = StringUtils::replaceFirst(ret, "$(block-offset)", key);
    ret = StringUtils::replaceFirst(ret, "$(level)", cstring(H));

    PrintInfo("level",level_samples.logic_box.toString(),"block",block_samples.logic_box.toString());
  }
   
  VisusAssert(!StringUtils::contains(ret, "$"));

  //reverse is an AWS s3 trick to distribute better blocks on several AWS instances
  if (reverse_filename)
    ret = StringUtils::reverse(ret);

  return ret;
}


} //namespace Visus



