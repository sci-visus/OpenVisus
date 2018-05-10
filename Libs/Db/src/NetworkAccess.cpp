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

#include <Visus/CloudStorageAccess.h>
#include <Visus/Dataset.h>
#include <Visus/Encoders.h>
#include <Visus/VisusConfig.h>
#include <Visus/ApplicationInfo.h>

namespace Visus {

///////////////////////////////////////////////////////////////////////////////////////
NetworkAccess::NetworkAccess(String name,Dataset* dataset,StringTree config_)
{
  this->config = config_;

  this->name = name;
  this->can_read     = StringUtils::find(config.readString("chmod", "rw"), "r") >= 0;
  this->can_write    = StringUtils::find(config.readString("chmod", "rw"), "w") >= 0;
  this->bitsperblock = cint(config.readString("bitsperblock", cstring(dataset->getDefaultBitsPerBlock()))); VisusAssert(this->bitsperblock>0);
  this->url                  = config.readString("url",dataset->getUrl().toString()); VisusAssert(url.valid());
  this->compression          = config.readString("compression",url.getParam("compression","zip"));

  this->config.writeString("url", url.toString());

  bool disable_async = config.readBool("disable_async", dataset->bServerMode);

  if (int nconnections = disable_async ? 0 : config.readInt("nconnections", 8))
    this->async.netservice = std::make_shared<NetService>(nconnections);

  //VisusInfo() << "NetworkAccess created url(" << url.toString() << ") async(" << (async.netservice ? "yes" : "no") << ")";
}

} //namespace Visus 