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

#include <Visus/OnDemandAccess.h>

#include <Visus/NetService.h>
#include <Visus/DatasetBitmask.h>
#include <Visus/Dataset.h>
#include <Visus/Path.h>
#include <Visus/StringTree.h>

namespace Visus {

int OnDemandAccess::Defaults::nconnections=8;

////////////////////////////////////////////////////////////////////////////
class OnDemandAccessExternalPimpl : public OnDemandAccess::Pimpl
{
public:

  SharedPtr<NetService> netservice;

  //constructor
  OnDemandAccessExternalPimpl(OnDemandAccess* owner, Dataset* dataset)
    : OnDemandAccess::Pimpl(owner)
  {
    if (!dataset->isServerMode())
    {
      if (auto nconnections = OnDemandAccess::Defaults::nconnections)
        this->netservice = std::make_shared<NetService>(nconnections);
    }
  }

  //destructor
  virtual ~OnDemandAccessExternalPimpl()
  {
    netservice.reset();
  }

  
  //generateBlock
  virtual void generateBlock(SharedPtr<BlockQuery> query) override
  {
    Dataset* dataset = owner->getDataset();

    // system process or network request (blocking)
    // Be sure to set nthreads > 0 to ensure application doesn't hang
    // We make only one request for the entire area (box).
    //
    // <!-- search ondemand (disk), generate if not found (ondemand), search disk again to get generated data --> 
    // <dataset name="Test OnDemand RO" url="file://$(data)/ondemand/visus.idx" >
    //   <access name="Multiplex" type="multiplex">
    //     <access type='disk'           chmod='r'  url="file://$(data)/ondemand/visus.idx"/>
    //     <access type='ondemandaccess' chmod='r' type="external" path="file:///path/to/dataset.idx
    //     <access type='disk'           chmod='r'  url="file://$(data)/ondemand/visus.idx"/>
    //    </access>
    // </dataset>

    //where the samples will be in X Y Z, not a simply 1d array

    auto timestep = query->time;
    auto field = query->field.name;
    auto dataset_box = dataset->getLogicBox();
    auto block_logicbox = query->getLogicBox();

    Time t1 = Time::now();

    auto converter_url = owner->getPath();
    auto path          = Url(dataset->getUrl()).getPath();

    if (bool bRemote = StringUtils::startsWith(converter_url, "http://"))
    {
      //external conversion service must follow this protocol:
      Url url(converter_url);
      url.setParam("idx", Path(path).getFileName());
      url.setParam("field", field);
      url.setParam("time", cstring(timestep));
      url.setParam("box", block_logicbox.toString(/*bInterleave*/true));
      PrintInfo(url);

      NetRequest request(url);
      request.aborted = query->aborted;

      NetService::push(netservice, request).when_ready([this,query](NetResponse response) {
        // As noted above, this stage always returns query failed. Third layer of multiplex will get data
        owner->readFailed(query,"managed failure");
      });
    }
    else
    {
      //external conversion app must follow this protocol:
      String params(converter_url);
      params += " --idx " + path;
      params += " --field " + field;
      params += " --time " + cstring(timestep);
      params += " --box \"" + block_logicbox.toString(/*bInterleave*/true) + "\"";
      PrintInfo(params);

      //blocking call
      system(params.c_str());
      PrintInfo("path",converter_url,"time",t1.elapsedMsec());

      // as noted above, this stage will return query failed, then depend on third layer of multiplex to get the data
      return owner->readFailed(query,"managed failure");
    }
  }

};

////////////////////////////////////////////////////////////////////////////
class OnDemandAccessGoogleMapsPimpl : public OnDemandAccess::Pimpl
{
public:

  //constructor
  OnDemandAccessGoogleMapsPimpl(OnDemandAccess* owner) : Pimpl(owner) {
  }

  //destructor
  virtual ~OnDemandAccessGoogleMapsPimpl() {
  }

  //generateBlock
  virtual void generateBlock(SharedPtr<BlockQuery> query) override
  {
    Dataset* dataset = owner->getDataset();

    //system process (blocking)
    // be sure to set nthreads > 0 to ensure application doesn't hang
    //
    //note: this converter doesn't know anything about Hz, so just pass the field and timestep
    //      if we block here, a third layer of cache will find the data after it's converted, cfg like this:
    // <!-- search ondemand (disk), generate if not found (ondemand), search disk again to get generated data --> 
    // <dataset name="Test OnDemand RO" url="file://$(data)/ondemand/visus.idx" >
    //   <access name="Multiplex" type="multiplex">
    //     <access type='disk'           chmod='r'  url="file://$(data)/ondemand/visus.idx"/>
    //     <access type='ondemandaccess' chmod='r' type="googlemaps" path="http://mt1.google.com/vt/lyrs=y?z=8" />
    //     <access type='disk'           chmod='r'  url="file://$(data)/ondemand/visus.idx"/>
    //    </access>
    // </dataset>

    LogicSamples logic_samples = query->logic_samples;
    if (!logic_samples.valid())
      return owner->readFailed(query,"logic samples wrong");

    Int64 Width  = dataset->getLogicBox().size()[0];
    Int64 Height = dataset->getLogicBox().size()[1];

    Url urlpath(owner->getPath());
    int z = cint(urlpath.getParam("z", "0"));
    VisusAssert(z >= 0 && z <= 22);

    auto& buffer = query->buffer;
    buffer.layout = "";

    //very naive strategy: make a request for every point, rely on external app to avoid
    //duplicate fetches, which (for a 256x256 tile) will be for any of the last 9 hz-levels.
    for (auto loc = ForEachPoint(buffer.dims); !loc.end(); loc.next())
    {
      if (query->aborted())
        break;

      PointNi logic_pos = logic_samples.pixelToLogic(loc.pos);

      //mirror y
      logic_pos[1] = Height - logic_pos[1] - 1;

      //url contains the file in which to cache results
      //helpful: export PATH=/Users/cam/code/nvisus/Samples/visusconvert:`pwd`/Release/visusconvert.app/Contents/MacOS:$PATH
      String params("google_earth_ondemand.sh"); params += " ";
      params += "visusconvert"; params += " ";
      params += Url(dataset->getUrl()).getPath(); params += " ";
      params += cstring(logic_pos[0]); params += " ";
      params += cstring(logic_pos[1]); params += " ";
      params += cstring(z); params += " ";
      if (StringUtils::contains(urlpath.getPath(), "lyrs=p"))
        params += String("1");

      //blocking call
      system(params.c_str());
    }

    // this stage will return query failed, then depend on third layer of multiplex to get the data
    owner->readFailed(query,"managed failure");
  }

};


////////////////////////////////////////////////////////////////////////////
template<class SampleGeneratorClass>
class OnDemandAccessSampleGeneratorPimpl : public OnDemandAccess::Pimpl
{
public:

  SampleGeneratorClass sample_generator;
  String layout;

  //constructor
  OnDemandAccessSampleGeneratorPimpl(OnDemandAccess* owner) : Pimpl(owner) {
  }

  //destructor
  virtual ~OnDemandAccessSampleGeneratorPimpl() {
  }

  //generateBlock
  virtual void generateBlock(SharedPtr<BlockQuery> query) override
  {
    DType dtype = query->field.dtype;
    if      (dtype == DTypes::INT8   ) return templatedGenerateBlock<Int8   >(query);
    else if (dtype == DTypes::UINT8  ) return templatedGenerateBlock<Uint8  >(query);
    else if (dtype == DTypes::INT16  ) return templatedGenerateBlock<Int16  >(query);
    else if (dtype == DTypes::UINT16 ) return templatedGenerateBlock<Uint16 >(query);
    else if (dtype == DTypes::INT32  ) return templatedGenerateBlock<Int32  >(query);
    else if (dtype == DTypes::UINT32 ) return templatedGenerateBlock<Uint32 >(query);
    else if (dtype == DTypes::INT64  ) return templatedGenerateBlock<Int64  >(query);
    else if (dtype == DTypes::UINT64 ) return templatedGenerateBlock<Uint64 >(query);
    else if (dtype == DTypes::FLOAT32) return templatedGenerateBlock<Float32>(query);
    else if (dtype == DTypes::FLOAT64) return templatedGenerateBlock<Float64>(query);
    return owner->readFailed(query, "unsupported dtype");
  }

  //templatedGenerateBlock
  template <typename CppType>
  void templatedGenerateBlock(SharedPtr<BlockQuery> query)
  {
    Dataset* dataset = owner->getDataset();

    LogicSamples logic_samples = query->logic_samples;
    if (!logic_samples.valid())
      return owner->readFailed(query,"logic samples not valid");

    DType dtype = query->field.dtype;
    VisusAssert(dtype.getByteSize(1) == sizeof(CppType));

    //convert logic range to [0,1]
    auto P0    = dataset->getLogicBox().p1;
    auto Size  = dataset->getLogicBox().size();

    auto& buffer = query->buffer;
    buffer.layout = this->layout;

    CppType* c_ptr = buffer.c_ptr<CppType*>();

    //very naive strategy: make a request for every point, rely on external app to avoid
    //duplicate fetches, which (for a 256x256 tile) will be for any of the last 9 hz-levels.
    for (auto loc = ForEachPoint(buffer.dims); !loc.end(); loc.next())
    {
      if (query->aborted())
        return owner->readFailed(query,"query aborted");

      PointNi logic_pos = logic_samples.pixelToLogic(loc.pos);

      Point3d p(
        (logic_pos[0] - P0[0]) / (double)(Size[0]),
        (logic_pos[1] - P0[1]) / (double)(Size[1]),
        (logic_pos[2] - P0[2]) / (double)(Size[2]));

      *c_ptr++ = sample_generator.template generateSample<CppType>(p);
    }

    return owner->readOk(query);
  }

};

////////////////////////////////////////////////////////////////////////////
class CheckerboardSampleGenerator
{
public:

  const double step;
  const double invstep;

  //constructor
  CheckerboardSampleGenerator(const double step_ = 0.2) 
    : step(step_), invstep(1.0 / step_) {
  }

  //generateSample
  template <typename CppType>
  inline CppType generateSample(const Point3d& p) const
  {
    int xi = int(p[0]*invstep);
    int yi = int(p[1]*invstep);
    int zi = int(p[2]*invstep);
    return CppType((xi % 2 ^ (yi + 1) % 2 ^ zi % 2) ? 255 : 0);
  }
};


////////////////////////////////////////////////////////////////////////////
class MandelbrotSampleGenerator
{
public:

  //constructor
  MandelbrotSampleGenerator() {
  }

  //generateSample
  template <typename CppType>
  inline CppType generateSample(const Point3d& p) const
  {
    double x = p[0];
    double y = p[1];

    //http://nuclear.mutantstargoat.com/articles/sdr_fract/
    const double scale = 2;
    const double center_x = 0;
    const double center_y = 0;
    const int iter = 48;

    double c_x = 1.3333 * (x - 0.5) * scale - center_x;
    double c_y = (y - 0.5) * scale - center_y;
    double z_x = c_x;
    double z_y = c_y;
    for (int i = 0; i < iter; i++, z_x = x, z_y = y)
    {
      x = (z_x*z_x - z_y*z_y) + c_x;
      y = (z_y*z_x + z_x*z_y) + c_y;
      if ((x*x + y*y) > 4.0) return CppType(double(i) / iter);
    }
    return CppType(0.0);
  }
};


typedef OnDemandAccessSampleGeneratorPimpl< CheckerboardSampleGenerator > OnDemandAccessCheckerboardPimpl;
typedef OnDemandAccessSampleGeneratorPimpl< MandelbrotSampleGenerator   > OnDemandAccessMandelbrotPimpl;

////////////////////////////////////////////////////////////////////////////
OnDemandAccess::OnDemandAccess(Dataset* dataset, StringTree config)
{
  this->dataset = dataset;
  this->type = Type::fromString(config.readString("ondemand", "checkerboard"));
  this->path = config.readString("path");

  this->can_read = true;
  this->can_write = false;
  this->bitsperblock = dataset->getDefaultBitsPerBlock();

  //you can use a thread pool or not (default: no)
  if (int nthreads = cint(config.readString("nthreads", "0")))
    this->thread_pool=std::make_shared<ThreadPool>("OnDemandAccess Worker",nthreads);

  switch (this->type)
  {
  case Type::GoogleMaps:
    this->pimpl = new OnDemandAccessGoogleMapsPimpl(this);
    break;

  case Type::Checkerboard:
    this->pimpl = new OnDemandAccessCheckerboardPimpl(this);
    break;

  case Type::Mandelbrot:
    this->pimpl = new OnDemandAccessMandelbrotPimpl(this);
    break;

  case Type::External:
    this->pimpl = new OnDemandAccessExternalPimpl(this,dataset);
    break;

  case Type::ApplyFilter:
    VisusAssert(false);//TODO
    break;
  }
}

////////////////////////////////////////////////////////////////////////////
OnDemandAccess::~OnDemandAccess()
{
  this->thread_pool.reset();
  if (this->pimpl)
    delete this->pimpl;
}



} //namespace Visus

