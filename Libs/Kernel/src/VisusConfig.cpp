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

#include <Visus/VisusConfig.h>
#include <Visus/File.h>
#include <Visus/Utils.h>
#include <Visus/RamResource.h>

namespace Visus {

String                   VisusConfig::filename;
StringTree               VisusConfig::storage = StringTree("visus_config");
Int64                    VisusConfig::timestamp = 0;

//////////////////////////////////////////////////////////////
bool VisusConfig::reload()
{
  auto filename = VisusConfig::filename;

  if (FileUtils::existsFile(filename))
  {
    String body = Utils::loadTextDocument(filename);
    StringTree temp("visus");
    if (temp.loadFromXml(body))
    {
      VisusConfig::storage = temp;
      VisusConfig::timestamp = FileUtils::getTimeLastModified(filename);
      //VisusInfo() << VisusConfig::storage.toString();
    }
    else
    {
      VisusWarning() << "visus config content is wrong or empty";
    }
  }

  //in case the user whant to simulate I have a certain amount of RAM
  if (Int64 total = StringUtils::getByteSizeFromString(VisusConfig::readString("Configuration/RamResource/total", "0")))
    RamResource::getSingleton()->setOsTotalMemory(total);

  if (VisusConfig::storage.empty())
  {
    return false;
    VisusInfo() << "Cannot find visus config file, maybe you can specify it using --visus-config <filename>";
  }
  else
  {
    VisusInfo() << "Found Visus configuration [" << filename << "]";
  }


  VisusConfig::validate();
  return true;
}

//////////////////////////////////////////////////////////////
bool VisusConfig::needReload()
{
  return VisusConfig::timestamp == FileUtils::getTimeLastModified(VisusConfig::filename) ? false : true;
}


//////////////////////////////////////////////////////////////
bool VisusConfig::validate()
{
  bool valid = true;
  std::vector<StringTree*> datasets = findAllChildsWithName("dataset");

  //check for duplicates in datasets
  std::map<String, StringTree*> map;
  for (int i = 0; i<(int)datasets.size(); i++)
  {
    String name = datasets[i]->readString("name", "<unnamed>");
    auto iter = map.find(name);
    if (iter != map.end())
    {
      if (name != "<unnamed>")
        VisusWarning() << "Duplicate entries found in visus.config for: " << name;
      valid = false;
    }
    map[name] = datasets[i];
  }

  return valid;
}


} //namespace Visus


