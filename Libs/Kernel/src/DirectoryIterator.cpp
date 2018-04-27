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

#include <Visus/DirectoryIterator.h>
#include <Visus/StringUtils.h>

#if !__APPLE__ || VISUS_WRAPPED_IN_MM

#if WIN32
#include <Windows.h>

#elif __APPLE__
#import <Foundation/NSAutoreleasePool.h>
#import <Foundation/Foundation.h>
#include <unistd.h>
#include <fnmatch.h>
#include <sys/socket.h>
#include <sys/stat.h>

#else
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#endif

#include <stack>

namespace Visus {

////////////////////////////////////////////////
#if WIN32
class DirectoryIterator::Pimpl
{
  const String directory;
  HANDLE       handle;

public:
  
  //addTrailingSeparator
  static String addTrailingSeparator(const String& path)
  {return StringUtils::endsWith(path,"/")? path : path+"/";}

  //constructor
  Pimpl(String directory_)
    : directory(addTrailingSeparator(directory_)),
    handle(INVALID_HANDLE_VALUE)
  {}

  //destructor
  ~Pimpl()
  {
    if (handle != INVALID_HANDLE_VALUE)
      FindClose(handle);
  }

  //next
  Found next()
  {
    while (true)
    {
      WIN32_FIND_DATA find_data;
      if (handle==INVALID_HANDLE_VALUE)
      {
        String directory=this->directory+"*";
        handle = FindFirstFile(directory.c_str(), &find_data);
        if (handle == INVALID_HANDLE_VALUE)
          return Found();
      }
      else
      {
        if (FindNextFile(handle, &find_data)==0)
          return Found();
      }
    
      String filename=find_data.cFileName;
      if (filename=="." || filename=="..")
        continue;

      Found ret;
      ret.filename        = filename;
      ret.is_dir          = ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
      ret.is_hidden       = ((find_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0);
      ret.is_read_only    = ((find_data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0);
      ret.filesize        = find_data.nFileSizeLow + (((Int64)find_data.nFileSizeHigh) << 32);
      return ret;
    }
  }
};
#endif


////////////////////////////////////////////////
#if __APPLE__
class DirectoryIterator::Pimpl
{
public:
  
  //nsstring
  static inline NSString* nsstring(const String& s)
  {return [NSString stringWithUTF8String: s.c_str()];}
  
  //cstring
  static inline String cstring (NSString* s)
  {return String ([s UTF8String]);}
  
  //addTrailingSeparator
  static String addTrailingSeparator(const String& path)
  {return StringUtils::endsWith(path,"/")? path : path+"/";}

  //ScopedAutoReleasePool
  class ScopedAutoReleasePool
  {
  public:
    void* auto_release_pool;
    inline  ScopedAutoReleasePool() {auto_release_pool = [[NSAutoreleasePool alloc] init];}
    inline ~ScopedAutoReleasePool() {[((NSAutoreleasePool*) auto_release_pool) release];}
  };

  String                 directory;
  NSDirectoryEnumerator* enumerator;

  //constructor
  Pimpl (String directory_)
    : directory (addTrailingSeparator(directory_)),enumerator(nil)
  {
    ScopedAutoReleasePool arp;
    enumerator = [[[NSFileManager defaultManager] enumeratorAtPath: nsstring(directory)] retain];
  }

  //destructor
  ~Pimpl()
  {
    [enumerator release];
  }

  //next
  Found next()
  {
    ScopedAutoReleasePool arp;
    while (true)
    {
      NSString* file;

      if (enumerator==nil || (file=[enumerator nextObject])==nil)
        return Found();

      [enumerator skipDescendents];

      String filename=cstring(file);
      if (filename=="." || filename=="..")
        continue;
      
      Found ret;
      ret.filename = filename;
      struct stat info;
      String fullpath=directory + ret.filename;
      if (stat(fullpath.c_str(),&info)==0)
      {
        ret.is_dir        = (info.st_mode & S_IFDIR)?true:false;
        ret.filesize      = (Int64)info.st_size;
        ret.is_read_only  = access(fullpath.c_str(), W_OK)!=0;
        ret.is_hidden     = StringUtils::startsWith(ret.filename,".");
      }
      return ret;
    }

    return Found();
  }
}; 

#endif


////////////////////////////////////////////////
#if __GNUC__ && !__APPLE__
class DirectoryIterator::Pimpl
{
  String directory;
  DIR* dir;
  
public:
  
  //addTrailingSeparator
  static String addTrailingSeparator(const String& path)
  {return StringUtils::endsWith(path,"/")? path : path+"/";}

  //constructor
  Pimpl (String directory_)
    : directory(addTrailingSeparator (directory_))
  {
    dir=opendir(directory.c_str());
  }

  //destructor
  ~Pimpl()
  {
    if (dir!=nullptr)
     closedir (dir);
  }

  //next
  Found next()
  {
    if (!dir)
      return Found();

    while (true)
    {
      struct dirent* const de=readdir(dir);

      if (!de) 
        return Found();

      String filename=de->d_name;
      if (filename=="." || filename=="..")
        continue;

      Found ret;
      ret.filename  = filename;
      struct stat info;
      String fullpath=directory + ret.filename;
      if (stat(fullpath.c_str(),&info)==0)
      {
        ret.is_dir        = (info.st_mode & S_IFDIR)?true:false;
        ret.filesize      = (Int64)info.st_size;
        ret.is_read_only  = access(fullpath.c_str(), W_OK)!=0;
        ret.is_hidden     = StringUtils::startsWith(ret.filename,".");
      }
      return ret;
    }

    return Found();
  }
};

#endif

/////////////////////////////////////////////////////////////////////////
DirectoryIterator::DirectoryIterator(String directory)
{pimpl.reset(new Pimpl(directory));}


DirectoryIterator::~DirectoryIterator()
{pimpl.reset();}

DirectoryIterator::Found DirectoryIterator::next()
{return pimpl->next();}

/////////////////////////////////////////////////////////////////////////
void DirectoryIterator::findAllFilesEndingWith(std::vector<String>& dst,String start_directory,String ending)
{
  std::vector<String> extensions;
  {
    std::vector<String> v=StringUtils::split(ending,";");
    for (int I=0;I<(int)v.size();I++) 
    {
      String extension=StringUtils::trim(StringUtils::toLower(v[I]));
      if (!extension.empty())
        extensions.push_back(extension);
    }
  }

  DirectoryIterator::Found found;
  std::stack<String> stack;
  stack.push(start_directory);
  while (!stack.empty())
  {
    String directory=stack.top();
    stack.pop();

    DirectoryIterator it(directory);
    while ((found=it.next()))
    {
      String fullpath=directory+"/" + found.filename;

      if (found.is_dir)
      {
        stack.push(fullpath);
      }
      else
      {
        String lower_case=StringUtils::toLower(found.filename);
        for (int N=0;N<(int)extensions.size();N++)
        {
          if (StringUtils::endsWith(lower_case,extensions[N]))
          {
            dst.push_back(fullpath);
            break;
          }
        }
      }
    }
  }
}

} //namespace Visus

#endif //#if !__APPLE__ || VISUS_WRAPPED_IN_MM
