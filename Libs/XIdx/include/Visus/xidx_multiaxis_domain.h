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

#ifndef XIDX_MULTIAXIS_DOMAIN_H_
#define XIDX_MULTIAXIS_DOMAIN_H_


namespace Visus{

typedef Variable Axis;

////////////////////////////////////////////////////////////////
class VISUS_XIDX_API MultiAxisDomain : public Domain
{
public:

  VISUS_XIDX_CLASS(MultiAxisDomain)

  //down nodes
  std::vector<Axis*> axis;
  
  //constructor
  MultiAxisDomain(String name_="") 
    : Domain(name_, DomainType::MULTIAXIS_DOMAIN_TYPE) {
  };

  //destructor
  virtual ~MultiAxisDomain() {
    for (auto it : axis)
      delete it;
  }

  //addAxis
  void addAxis(Axis* VISUS_DISOWN(value)){
    addEdge(this, value);
    this->axis.push_back(value);
  }
  
  //getLinearizedIndexSpace
  virtual LinearizedIndexSpace getLinearizedIndexSpace(int index){
    return axis[index]->data_items[0]->values;
  };
  
  //getLinearizedIndexSpace
  virtual LinearizedIndexSpace getLinearizedIndexSpace() override{
    // TODO NOT IMPLEMENTED
    ThrowException("getLinearizedIndexSpace() for MultiAxisDomain not implemented please use getLinearizedIndexSpace(int index)");
    return getLinearizedIndexSpace(0);
  };

public:

  //write
  virtual void write(Archive& ar) const override
  {
    Domain::write(ar);
    for (auto child : this->axis)
      writeChild<Axis>(ar, "Axis", child);
  }

  //read
  virtual void read(Archive& ar) override
  {
    Domain::read(ar);
    for (auto child : readChilds<Axis>(ar, "Axis"))
      addAxis(child);
  }
};

} //namespace

#endif //XIDX_MULTIAXIS_DOMAIN_H_
