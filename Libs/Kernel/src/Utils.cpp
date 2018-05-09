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

#include <Visus/Utils.h>
#include <Visus/StringUtils.h>
#include <Visus/Log.h>
#include <Visus/Url.h>
#include <Visus/Path.h>
#include <Visus/NetService.h>
#include <Visus/File.h>

#include <cctype>
#include <iomanip>
#include <fstream>
#include <assert.h>

#if __GNUC__ && !__APPLE__
#include <signal.h>
#endif

#if WIN32
#include <Windows.h>
#include <process.h>
#else
#include <unistd.h>
#endif

namespace Visus {
  
///////////////////////////////////////////////////////////////////////////////////////
String Utils::safe_strerror(int err)
{
  const int buffer_size = 512;
  char buf[buffer_size];
#if WIN32
  strerror_s(buf, sizeof(buf), err);
#else
  if (strerror_r(err, buf, sizeof(buf))!=0)
    buf[0]=0;
#endif
  return String(buf);
}

///////////////////////////////////////////////////////////////////////////////////////
int Utils::getPid()
{
#if WIN32
  return _getpid();
#else
  return = getpid();
#endif
}

///////////////////////////////////////////////////////////////////////////////////////
void Utils::breakInDebugger()
{
#ifdef _DEBUG
  {
#if WIN32
    _CrtDbgBreak();
#elif __APPLE__
    asm("int $3");
#else
    ::kill(0, SIGTRAP);
    assert(0);
#endif
  }
#endif
}


//////////////////////////////////////////////////////////////////
String Utils::loadTextDocument(String url_)
{
  Url url(url_);

  if (url.isFile())
  {
    String filename=url.getPath();

    if (filename.empty()) 
      return "";

    Path path(filename);
    if (path.empty()) 
      return "";

    String fullpath=path.toString();
    std::ifstream file(fullpath.c_str(), std::ios::binary);
    if (!file.is_open()) 
      return "";

    std::stringstream sstream;
    sstream << file.rdbuf();
    String ret=sstream.str();

    //I got some weird files! (rtrim does not work!)
    int ch; 
    while (!ret.empty() && ((ch=ret[ret.size()-1])=='\0' || ch==' ' || ch=='\t' || ch=='\r' || ch=='\n'))
      ret.resize(ret.size()-1);

    return ret;
  }
  else
  {
    auto net_response=NetService::getNetResponse(url);
    if (!net_response.isSuccessful()) return "";
    return net_response.getTextBody();
  }
}

//////////////////////////////////////////////////////////////////
bool Utils::saveTextDocument(String filename,String content)
{
  VisusAssert(Url(filename).isFile());

  Path path(filename);
  if (path.empty()) 
    return false;

  String fullpath=path.toString();
  std::ofstream file(fullpath.c_str(), std::ios::binary);

  if (!file.is_open())
  {
    //try to create the directory
    FileUtils::createDirectory(path.getParent());
    file.open(fullpath.c_str(), std::ios::binary);

    if (!file.is_open()) 
      return false;
  }

  file.write(content.c_str(),content.size());
  file.close();
  return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////
SharedPtr<HeapMemory> Utils::loadBinaryDocument(String url_)
{
  Url url(url_);

  if (url.isFile())
  {
    String filename=url.getPath();
    if (filename.empty()) 
      return SharedPtr<HeapMemory>();

    Path path(filename);
    if (path.empty()) 
      return SharedPtr<HeapMemory>();

    String fullpath=path.toString();
    std::ifstream file(fullpath.c_str(), std::ios::binary);
    if (!file.is_open()) 
      return SharedPtr<HeapMemory>();

    DoAtExit do_at_exit([&file](){file.close();});

    file.seekg(0, file.end);
    int length=(int)file.tellg(); 

    if (length<=0) 
      return SharedPtr<HeapMemory>();

    file.seekg(0,file.beg);

    auto dst=std::make_shared<HeapMemory>();

    if (!dst->resize(length,__FILE__,__LINE__)) 
      return SharedPtr<HeapMemory>();

    if (!file.read((char*)dst->c_ptr(),length)) 
      return SharedPtr<HeapMemory>();

    return dst;
  }
  else
  {
    auto net_response=NetService::getNetResponse(url);

    if (!net_response.isSuccessful()) 
      return SharedPtr<HeapMemory>();

    return net_response.body;
  }  
}

///////////////////////////////////////////////////////////////////////////////////////////////
bool Utils::saveBinaryDocument(String filename,SharedPtr<HeapMemory> src)
{
  if (!src)
    return false;

  //TODO!
  VisusAssert(Url(filename).isFile()); 

  Path path(filename);
  if (path.empty()) return false;
  String fullpath=path.toString();
  std::ofstream file(fullpath.c_str(), std::ios::binary);

  //try to create the directory
  if (!file.is_open())
  {
    FileUtils::createDirectory(path.getParent());
    file.open(fullpath.c_str(), std::ios::binary);
    if (!file.is_open()) return false;
  }

  if (!file.write((char*)src->c_ptr(),src->c_size())) {
    file.close();
    return false;
  }

  file.close();
  return true;
}



//////////////////////////////////////////////////
SharedPtr<DictObject> Utils::convertStringTreeToDictObject(const StringTree* src)
{
  auto dst = std::make_shared<DictObject>();

  //name
  dst->setattr("name", std::make_shared<StringObject>(src->name));

  //attributes
  if (!src->attributes.empty())
  {
    auto attributes = std::make_shared<DictObject>();
    for (auto it = src->attributes.begin(); it != src->attributes.end(); it++)
      attributes->setattr(it->first, std::make_shared<StringObject>(it->second));
    dst->setattr("attributes", attributes);
  }

  //childs
  if (int N = src->getNumberOfChilds())
  {
    auto childs = std::make_shared<ListObject>();
    dst->setattr("childs", childs);
    for (int I = 0; I<N; I++)
      childs->push_back(convertStringTreeToDictObject(&src->getChild(I)));
  }

  return dst;
}

//////////////////////////////////////////////////
SharedPtr<StringTree> Utils::convertDictObjectToStringTree(DictObject* src)
{
  VisusAssert(src);

  auto dst = std::make_shared<StringTree>();

  dst->name = cstring(src->getattr("name"));
  VisusAssert(!dst->name.empty());

  if (SharedPtr<DictObject> attributes = std::dynamic_pointer_cast<DictObject>(src->getattr("attributes")))
  {
    for (auto it = attributes->begin(); it != attributes->end(); it++)
      dst->writeString(it->first, cstring(it->second));
  }

  if (SharedPtr<ListObject> childs = std::dynamic_pointer_cast<ListObject>(src->getattr("childs")))
  {
    for (auto it = childs->begin(); it != childs->end(); it++)
      dst->addChild(*convertDictObjectToStringTree(dynamic_cast<DictObject*>(it->get())));
  }

  return dst;
}


//////////////////////////////////////////////////
void Utils::LLtoUTM(const double Lat, const double Long, double &UTMNorthing, double &UTMEasting)
{
  const double WGS84_A = 6378137.0;
  const double UTM_K0 = 0.9996;
  const double WGS84_E = 0.0818191908;
  const double UTM_E2 = (WGS84_E*WGS84_E);

  double a = WGS84_A;
  double eccSquared = UTM_E2;
  double k0 = UTM_K0;

  double LongOrigin;
  double eccPrimeSquared;
  double N, T, C, A, M;

  //Make sure the longitude is between -180.00 .. 179.9
  double LongTemp = (Long + 180) - int((Long + 180) / 360) * 360 - 180;

  double LatRad = Utils::degreeToRadiant(Lat);
  double LongRad = Utils::degreeToRadiant(LongTemp);
  double LongOriginRad;
  int    ZoneNumber;

  ZoneNumber = int((LongTemp + 180) / 6) + 1;

  if (Lat >= 56.0 && Lat < 64.0 && LongTemp >= 3.0 && LongTemp < 12.0)
    ZoneNumber = 32;

  // Special zones for Svalbard
  if (Lat >= 72.0 && Lat < 84.0)
  {
    if (LongTemp >= 0.0  && LongTemp < 9.0) ZoneNumber = 31;
    else if (LongTemp >= 9.0  && LongTemp < 21.0) ZoneNumber = 33;
    else if (LongTemp >= 21.0 && LongTemp < 33.0) ZoneNumber = 35;
    else if (LongTemp >= 33.0 && LongTemp < 42.0) ZoneNumber = 37;
  }
  // +3 puts origin in middle of zone
  LongOrigin = (ZoneNumber - 1) * 6 - 180 + 3;
  LongOriginRad = Utils::degreeToRadiant(LongOrigin);

  eccPrimeSquared = (eccSquared) / (1 - eccSquared);

  N = a / sqrt(1 - eccSquared * sin(LatRad)*sin(LatRad));
  T = tan(LatRad)*tan(LatRad);
  C = eccPrimeSquared * cos(LatRad)*cos(LatRad);
  A = cos(LatRad)*(LongRad - LongOriginRad);

  M = a * ((1 - eccSquared / 4 - 3 * eccSquared*eccSquared / 64
    - 5 * eccSquared*eccSquared*eccSquared / 256) * LatRad
    - (3 * eccSquared / 8 + 3 * eccSquared*eccSquared / 32
      + 45 * eccSquared*eccSquared*eccSquared / 1024)*sin(2 * LatRad)
    + (15 * eccSquared*eccSquared / 256
      + 45 * eccSquared*eccSquared*eccSquared / 1024)*sin(4 * LatRad)
    - (35 * eccSquared*eccSquared*eccSquared / 3072)*sin(6 * LatRad));

  UTMEasting = (double)
    (k0*N*(A + (1 - T + C)*A*A*A / 6
      + (5 - 18 * T + T * T + 72 * C - 58 * eccPrimeSquared)*A*A*A*A*A / 120)
      + 500000.0);

  UTMNorthing = (double)
    (k0*(M + N * tan(LatRad)
      *(A*A / 2 + (5 - T + 9 * C + 4 * C*C)*A*A*A*A / 24
        + (61 - 58 * T + T * T + 600 * C - 330 * eccPrimeSquared)*A*A*A*A*A*A / 720)));

  if (Lat < 0)
  {
    //10000000 meter offset for southern hemisphere
    UTMNorthing += 10000000.0;
  }
}


} //namespace Visus


