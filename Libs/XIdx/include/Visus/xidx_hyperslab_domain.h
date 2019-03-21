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

#ifndef XIDX_HYPERSLAB_DOMAIN_H_
#define XIDX_HYPERSLAB_DOMAIN_H_


namespace Visus{

//////////////////////////////////////////////////////////////////
class VISUS_XIDX_API HyperSlabDomain : public ListDomain
{
public:

  VISUS_XIDX_CLASS(HyperSlabDomain)

  double start = 0;
  double step = 0;
  int    count = 0;

  //constructor
  HyperSlabDomain(String name_="") : ListDomain(name_)
  {
    type = DomainType::HYPER_SLAB_DOMAIN_TYPE;
    ensureDataItem();
    data_items[0]->name = name;
    data_items[0]->dtype = DTypes::FLOAT64;
  }

  //constructor
  HyperSlabDomain(const HyperSlabDomain* d) : ListDomain(d->name){
    type = DomainType::HYPER_SLAB_DOMAIN_TYPE;
    data_items = d->data_items;
  }

  //setDomain
  int setDomain(double start_, double step_, int count_){
    start=start_;
    step=step_;
    count=count_;
    values = std::vector<double>({ start_,step_,(double)count_ });
    return 0;
  }
  
  //getLinearizedIndexSpace
  virtual LinearizedIndexSpace getLinearizedIndexSpace() override {
    LinearizedIndexSpace ret(count);
    for (int i=0; i< count; i++)
      ret[i] = start + i*step;
    return ret;
  }

public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override
  {
    ListDomain::writeToObjectStream(ostream);
  }

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override
  {
    ListDomain::readFromObjectStream(istream);
    // TODO generalize for many slabs
    if(values.size()==3){
      start = values[0];
      step  = values[1];
      count = (int)values[2];
    }
//    std::istringstream parse(istream.readText());
//    parse >> start >> step >> count;
  }

};
  
typedef HyperSlabDomain TemporalHyperSlabDomain;

} //namespace
#endif

