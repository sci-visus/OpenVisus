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

#include <Visus/DatasetArrayPlugin.h>
#include <Visus/Dataset.h>
#include <Visus/DatasetFilter.h>

namespace Visus {


////////////////////////////////////////////////
class DatasetArrayPluginParseArguments
{
public:

  Dataset*      dataset;
  double        time;
  NdBox         box;
  Field         field;
  int           maxh; //default is write at max resolution!
  int           fromh;
  int           toh;
  bool          bDisableFilters;

  //constructor
  DatasetArrayPluginParseArguments(Dataset* dataset_) : dataset(dataset_),bDisableFilters(false)
  {
    this->time           = dataset->getDefaultTime();
    this->field          = dataset->getDefaultField();
    this->maxh           = dataset->getMaxResolution(); //default is max resolution
    this->fromh          = 0;
    this->toh            = maxh;
  }

  //exec
  bool exec(std::vector<String> args)
  {
    for (int I=0;I<(int)args.size();I++)
    {
      if (args[I]=="--time")
      {
        time=cdouble(args[++I]);
      }
      else if (args[I]=="--box")
      {
        int pdim = dataset->getPointDim();
        box=NdBox::parseFromOldFormatString(pdim,args[++I]);
        box= box.getIntersection(dataset->getBox());
        if (!box.isFullDim())
        {
          VisusWarning()<<"invalid --box "<<args[I] << " intersection with "<< dataset->getBox().toOldFormatString();
          return false;
        }
      }
      else if (args[I]=="--field")
      {
        String sfield=args[++I];
        field=dataset->getFieldByName(sfield);
        if (!field.valid())
        {
          VisusWarning()<<"invalid --field "<<sfield;
          return false;
        }
      }
      else if (args[I]=="--maxh")
      {
        maxh=cint(args[++I]);
      }
      else if (args[I]=="--fromh")
      {
        fromh=cint(args[++I]);
      }
      else if (args[I]=="--toh")
      {
        toh=cint(args[++I]);
      }
      else if (args[I]=="--disable-filters")
      {
        bDisableFilters=true;
      }
    }

    if (!(fromh<=toh && toh<=maxh))
    {
      VisusWarning()<<"invalid --maxh "<<maxh<<" --fromh "<<fromh<<" --toh "<<toh;
      return false;
    }

    return true;
  }

};

///////////////////////////////////////////////////////////////////////////////
StringTree DatasetArrayPlugin::handleStatImage(String url)
{
  auto dataset=LoadDataset(url);

  if (!dataset)
  {
    VisusWarning()<<"Dataset::handleStatImage(\""<<url<<"\") failed";
    return StringTree();
  }

  StringTree ret("stat");
  ObjectStream ostream(ret, 'w');
  
  ostream.writeInline("url",url);
  ostream.writeInline("format", dataset->getTypeName());
  ostream.writeInline("size",dataset->getBox().size().toString());
  ostream.writeInline("box", dataset->getBox().toOldFormatString());
  ostream.writeInline("timesteps",cstring(dataset->getTimesteps().getMin())+" " + cstring(dataset->getTimesteps().getMax()));
  ostream.writeInline("bitsperblock",cstring(dataset->getDefaultBitsPerBlock()));
  ostream.writeInline("bitmask",dataset->getBitmask().toString());

  for (auto field : dataset->getFields())
  {
    ostream.pushContext("field");
    field.writeToObjectStream(ostream);
    ostream.popContext("field");
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
Array DatasetArrayPlugin::handleLoadImage(String url,std::vector<String> args_)
{
  auto dataset= LoadDataset(url);

  if (!dataset)
    return Array();

  DatasetArrayPluginParseArguments args(dataset.get());
  if (!args.exec(args_))
    return Array();

  Time t1=Time::now();

  auto query=std::make_shared<Query>(dataset.get(),'r');
  query->time=args.time;
  query->field=args.field;
  query->position=args.box;
  query->start_resolution=args.fromh;
  query->end_resolutions={args.toh};
  query->max_resolution=args.maxh;

  if (args.bDisableFilters)
  {
    VisusInfo()<<"DatasetFilter disabled.Reason: command line has --disable-filters option";
    query->filter.enabled=false;
  }
  else
  {
    query->filter.enabled=true;
  }

  if (!dataset->beginQuery(query))
  {
    VisusWarning()<<"dataset->beginQuery() failed";
    return Array();
  }

  auto access=dataset->createAccess();
  if (!dataset->executeQuery(access,query))
  {
    VisusWarning()<<"!dataset->executeQuery()";
    return Array();
  }
  VisusAssert(query->buffer.dims==query->nsamples);

  auto dst=query->buffer;

  if (auto filter=query->filter.value)
    dst=filter->dropExtraComponentIfExists(dst);

  VisusInfo()
    <<"field("<<args.field.name<<")"
    <<" original-size("<<StringUtils::getStringFromByteSize(dst.c_size())<<")";

  if (access)
    access->printStatistics();

  VisusInfo()<<"DatasetArrayPlugin::handleLoadImage("<<url<<") done in "<<t1.elapsedSec()<<" seconds";  
  return dst;
}


///////////////////////////////////////////////////////////////////////////////
bool DatasetArrayPlugin::handleSaveImage(String url,Array src,std::vector<String> args_)
{
  auto dataset= LoadDataset(url);

  if (!dataset)
    return false;

  DatasetArrayPluginParseArguments args(dataset.get());
  if (!args.exec(args_))
    return false;

  auto query=std::make_shared<Query>(dataset.get(),'w');
  query->time=args.time;
  query->field=args.field;
  query->position=args.box;
  query->start_resolution=args.fromh;
  query->end_resolutions={args.toh};
  query->max_resolution=args.maxh;

  if (!dataset->beginQuery(query))
  {
    VisusWarning()<<"dataset->beginQuery() failed";
    return false;
  }

  //embedding in case I'm missing point-dims
  int pdim = query->nsamples.getPointDim();
  if (pdim>src.dims.getPointDim())
    src.dims.setPointDim(pdim,1);

  if (query->nsamples!=src.dims)
  {
    VisusWarning()<<" query->dims returned ("<<query->nsamples.toString()<<") which is different from src.dims ("<<src.dims.toString()<<")";
    return false;
  }

  if (args.field.dtype!=src.dtype)
  {
    VisusWarning()<<" data not compatible, args field ("<<args.field.name<<" , current has type "<<src.dtype.toString();
    return false;
  }

  Time t1=Time::now();
  query->buffer=src; 

  auto access=dataset->createAccess();
  if (!dataset->executeQuery(access,query))
  {
    VisusWarning()<<"!dataset->executeQuery()";
    return false;
  }

  VisusInfo()
    <<"field("<<args.field.name<<") "
    <<"original-size("<<StringUtils::getStringFromByteSize(src.c_size())<<") ";

  if (access)
    access->printStatistics();

  VisusInfo()<<"DatasetArrayPlugin::handleSaveImage("<<url<<",..) done in "<<t1.elapsedSec()<<" seconds";  
  return true;
}


} //namespace Visus

