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

#ifndef XIDX_SPATIAL_DOMAIN_H_
#define XIDX_SPATIAL_DOMAIN_H_


namespace Visus{

/////////////////////////////////////////////////////////////////////////
class VISUS_XIDX_API SpatialDomain : public Domain
{
public:

  VISUS_XIDX_CLASS(SpatialDomain)

  //down nodes
  SharedPtr<Topology> topology = std::make_shared<Topology>();
  SharedPtr<Geometry> geometry = std::make_shared<Geometry>();
  
  //constructor
  SpatialDomain(String name_="") : Domain(name_, DomainType::SPATIAL_DOMAIN_TYPE) {
  };

  //setTopology
  void setTopology(SharedPtr<Topology> value) { 
    addEdge(this, value);
    this->topology = value;
  }
  
  //setGeometry
  void setGeometry(SharedPtr<Geometry> value) {
    addEdge(this, value);
    this->geometry = value;
  }

  //getLinearizedIndexSpace
  virtual LinearizedIndexSpace getLinearizedIndexSpace() override{
    ThrowException("getLinearizedIndexSpace() for SpatialDomain not implemented yet, please use getLinearizedIndexSpace(int index)");
    static LinearizedIndexSpace dumb;
    return dumb;
  };
  
  //getVolume
  virtual size_t getVolume() const override{
    size_t total = 1;
    for (auto it : topology->dimensions)
      total *= it;
    return total;
  }

public:

  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override
  {
    Domain::writeToObjectStream(ostream);

    writeChild<Topology>(ostream,"Topology",topology);
    writeChild<Geometry>(ostream, "Geometry", geometry);
  }

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override
  {
    Domain::readFromObjectStream(istream);

    if (auto topology = readChild<Topology>(istream, "Topology"))
      setTopology(topology);

    if (auto geometry = readChild<Geometry>(istream, "Geometry"))
      setGeometry(geometry);
  }

};

} //namespace

#endif
