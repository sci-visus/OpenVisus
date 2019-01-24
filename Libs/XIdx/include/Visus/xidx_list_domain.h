/*
 * Copyright (c) 2017 University of Utah
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef XIDX_LIST_DOMAIN_H_
#define XIDX_LIST_DOMAIN_H_

#include <sstream>

namespace Visus{

///////////////////////////////////////////////////////////////////////
class VISUS_XIDX_API ListDomain : public Domain
{
public:

  VISUS_CLASS(ListDomain)

  int pdim = 1;
  std::vector<double> values;
  
  //constructor
  ListDomain(String name_ ="") : Domain(name_, DomainType::LIST_DOMAIN_TYPE) {
    addDataItem(std::make_shared<DataItem>(name));
  }
  
  //addDataItem
  void addDataItem(SharedPtr<DataItem> value) {
    value->setParent(this);
    data_items.push_back(value);
  }

  //getLinearizedIndexSpace
  virtual LinearizedIndexSpace getLinearizedIndexSpace() override{
    return values;
  };

public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override
  {
    Domain::readFromObjectStream(ostream);

    ostream.writeInline("pdim", cstring(pdim));
    ostream.writeText(StringUtils::join(this->values));
  }
  
  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override 
  {
    Domain::readFromObjectStream(istream);

    this->pdim = cint(istream.readInline("pdim"));
    for (auto it : StringUtils::split(istream.readText()))
      this->values.push_back(cdouble(it));
  }

};

typedef ListDomain TemporalListDomain;

} //namespace

#endif //XIDX_LIST_DOMAIN_H_


