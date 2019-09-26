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

#include <Visus/FilterAccess.h>
#include <Visus/Dataset.h>

namespace Visus {

//////////////////////////////////////////////////////////////////////////////////////////////////
FilterAccess::FilterAccess(Dataset* dataset,StringTree config) 
{
  String chmod = config.readString("chmod","rw");
  Url    url   = config.readString("url",dataset->getUrl().toString());

  //this is the access that will receive write/read if the filter allows it
  this->can_read       = StringUtils::find(chmod,"r")>=0;
  this->can_write      = StringUtils::find(chmod,"w")>=0;

  auto target_access = *config.findChildWithName("access");

  if (!target_access.hasValue("url"))
    target_access.writeString("url",url.toString());

  this->target=dataset->createAccess(target_access);
  if (!this->target)
    ThrowException("Invalid <access> for FilterAccess");

  this->bitsperblock   = this->target->bitsperblock;
  this->name=config.readString("name");

  //parse conditions
  for (auto sub : config.getChilds())
  {
    if (sub->name!="condition")
      continue;

    FilterAccessCondition condition;
    condition.from   = sub->readInt64("from",0);
    condition.to     = sub->readInt64("to"  ,0);
    condition.step   = sub->readInt64("step",condition.to-condition.from );
    condition.full   = sub->readInt64("full",condition.step              );
    this->addCondition(condition);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void FilterAccess::addCondition(FilterAccessCondition condition)
{
  this->conditions.push_back(condition);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
bool FilterAccess::passThrought(SharedPtr<BlockQuery> query)
{
  BigInt from=query->start_address;
  BigInt to  =query->end_address;

  //all the conditions are in OR (i.e. first satisfied return true!)
  for (int i=0;i<(int)conditions.size();i++)
  {
    //basically I need that the requested range in inside a full range i.e. [condition.hzfrom+K*condition.step,condition.hzfrom+K*condition.step+condition.full) for some K
    const FilterAccessCondition& condition=conditions[i];
    BigInt K=(from-condition.from)/condition.step;
    BigInt aligned_from =condition.from+K*condition.step;
    BigInt aligned_to   =aligned_from+condition.full;
    if (bool bOk=(aligned_from>=condition.from && aligned_to<=condition.to) && (from>=aligned_from && to<=aligned_to)) 
      return true;
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void FilterAccess::readBlock(SharedPtr<BlockQuery> query)
{
  if (passThrought(query))
    target->readBlock(query);
  else
    readFailed(query);    
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void FilterAccess::writeBlock(SharedPtr<BlockQuery> query)
{
  if (passThrought(query))
    target->writeBlock(query);
  else
    writeFailed(query);
}

} //namespace Visus 

