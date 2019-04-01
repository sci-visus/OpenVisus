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

#include <Visus/Scene.h>
#include <Visus/DiskAccess.h>
#include <Visus/NetService.h>
#include <Visus/VisusConfig.h>


namespace Visus {

 ////////////////////////////////////////////////////////////////////////////////////
static StringTree* FindScene(String name, const StringTree& stree)
{
  auto all_scenes=stree.findAllChildsWithName("scene");

  for (auto it : all_scenes)
  {
    if (it->readString("name") == name)
      return it;
  }

  for (auto it : all_scenes)
  {
    if (it->readString("url") == name)
      return it;
  }

  return nullptr;
}

/////////////////////////////////////////////////////////////////////////
SharedPtr<Scene> LoadSceneEx(String name, const StringTree& config)
{
  if (name.empty())
    return SharedPtr<Scene>();

  auto it = FindScene(name,config);

  Url url = (it ? it->readString("url") : name);

  if (!url.valid())
  {
    VisusWarning() << "LoadScene(" << name << ") failed. Not a valid url";
    return SharedPtr<Scene>();
  }

  if (!url.isFile() && StringUtils::contains(url.toString(), "mod_visus"))
    url.setParam("action", "readscene");


  String content = Utils::loadTextDocument(url.toString());

  if (content.empty()) {
    VisusWarning() << "LoadScene(" << url.toString() << ") failed";
    return SharedPtr<Scene>();
  }

  StringTree stree;
  if (!stree.fromXmlString(content))
  {
    VisusWarning() << "LoadScene(" << url.toString() << ") failed";
    VisusAssert(false);
    return SharedPtr<Scene>();
  }

  auto ret = std::make_shared<Scene>();
  ret->url = url;
  ret->scene_body = stree.toString();
  return ret; 
}

} //namespace Visus 
