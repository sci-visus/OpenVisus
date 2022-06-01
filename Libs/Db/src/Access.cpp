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

namespace Visus {
const String Access::DefaultChMod = "rw";


///////////////////////////////////////////////////////////////////////////////////////
String Access::getBlockFilename(String filename_template, Field field, double time, String compression, BigInt blockid, bool reverse_filename) 
{
  String fieldname = StringUtils::removeSpaces(field.name);

  String ret = filename_template;

  ret = StringUtils::replaceFirst(ret, "$(field)", fieldname.length() < 32 ? StringUtils::onlyAlNum(fieldname) : StringUtils::computeChecksum(fieldname));
  ret = StringUtils::replaceFirst(ret, "$(time)", StringUtils::onlyAlNum(int(time) == time ? cstring((int)time) : cstring(time)));
  ret = StringUtils::replaceFirst(ret, "$(compression)", compression);

  //NOTE 16x is enough for 16*4 bits==64 bit for block number
  //     splitting by 4 means 2^16= 64K files inside a directory which seams reasonable with max 64/16=4 levels of directories
  {
    auto s_blockid = StringUtils::formatNumber("%016x", blockid);
    ret = StringUtils::replaceFirst(ret, "$(block:%016x)", s_blockid);
    ret = StringUtils::replaceFirst(ret, "$(block:%016x:%04x)", StringUtils::join(StringUtils::splitInChunks(s_blockid, 4), "/"));
  }

  VisusAssert(!StringUtils::contains(ret, "$"));

  //reverse is an AWS s3 trick to distribute better blocks on several AWS instances
  if (reverse_filename)
    ret = StringUtils::reverse(ret);

  return ret;
}


} //namespace Visus



