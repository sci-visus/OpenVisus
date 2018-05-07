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

#include <Visus/Path.h>


namespace Visus {

Path KnownPaths::VisusHome;
Path KnownPaths::VisusCachesDirectory   ; 
Path KnownPaths::CurrentApplicationFile ;
Path KnownPaths::CurrentWorkingDirectory;


///////////////////////////////////////////////////////////////////////
bool Path::isGoodNormalizedPath(String path)
{
  if (path.empty())
    return true;

  return
    //always start with [/] or [c:] or [d:] or ... 
    (((path.size() >= 1 && path[0] == '/') || (path.size() >= 2 && isalpha(path[0]) && path[1] == ':'))) &&
    //does not end with "/" (unless is [/])
    (path == "/" || !StringUtils::endsWith(path, "/")) &&
    //does not contain window separator
    (!StringUtils::contains(path, "\\")) &&

    //i can use the tilde for temporary path
#if 0
    //does not contain home alias
    (!StringUtils::contains(path, "~")) &&
#endif

    //does not contain any other alias
    (!StringUtils::contains(path, "$"));
}


///////////////////////////////////////////////////////////////////////
const String Path::normalizePath(String path)
{
  if (path.empty())
    return "";

  //replace alias
  if (StringUtils::find(path,"$(")>=0)
  {
    path=StringUtils::replaceAll(path,"$(VisusHome)"              ,KnownPaths::VisusHome.path);
    path=StringUtils::replaceAll(path,"$(CurrentApplicationFile)" ,KnownPaths::CurrentApplicationFile .path);
    path=StringUtils::replaceAll(path,"$(CurrentWorkingDirectory)",KnownPaths::CurrentWorkingDirectory.path);
    path=StringUtils::replaceAll(path,"$(VisusCachesDirectory)"   ,KnownPaths::VisusCachesDirectory   .path);
  }

  //don't want window separators
  path=StringUtils::replaceAll(path,"\\", "/");

  // example [./something ]  -> CurrentWorkingDirectory/./something
  //         [../something]  -> CurrentWorkingDirectory/../something
  if (StringUtils::startsWith(path,"./") || StringUtils::startsWith(path,"../"))
    path=KnownPaths::CurrentWorkingDirectory.path + "/" + path;

  // example [~/something] -> VisusHome/something
  if (StringUtils::startsWith(path,"~"))
    path=KnownPaths::VisusHome.path + path.substr(1);

  //example [c:/something/] -> [c:/something]
  if (StringUtils::endsWith(path,"/") && path!="/") 
    path=path.substr(0,path.length()-1);
    
  //example [//something] -> [/something] 
  while (StringUtils::startsWith(path,"//"))
    path=path.substr(1);

  //example [/C:] -> [C:]
  if (path.size()>=3 && path[0]=='/' && isalpha(path[1]) && path[2]==':')
    path=path.substr(1);

  //NEED TO START WITH A ROOT DIR (example [/] or [c:])
  if (!(((path.size()>=1 && path[0]=='/') || (path.size()>=2 && isalpha(path[0]) && path[1]==':'))))
    path=KnownPaths::CurrentWorkingDirectory.path + "/" + path;

  VisusAssert(isGoodNormalizedPath(path));
  return path;
}

//////////////////////////////////////////////////////////
Path Path::getParent(bool normalize) const
{
  if (path.empty() || isRootDirectory())
    return Path();

  const int idx = (int)path.rfind("/");

  if (idx>0) 
  {
    // example [/mnt/free] -> [/mnt]
    String ret=path.substr(0, idx);
    VisusAssert(isGoodNormalizedPath(ret));
    return Path(ret,normalize);
  }
  else 
  {
    //example [/mnt] -> [/]
    VisusAssert(idx==0); //since is not root directory IT MUST HAVE a parent (see normalizePath function)
    return Path("/");
  }

}


} //namespace Visus



