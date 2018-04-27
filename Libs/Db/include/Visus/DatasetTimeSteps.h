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

#ifndef __VISUS_DB_DATASET_TIMESTEPS_H
#define __VISUS_DB_DATASET_TIMESTEPS_H

#include <Visus/Db.h>
#include <Visus/Range.h>

#include <set>

namespace Visus {


////////////////////////////////////////////////////////
class VISUS_DB_API DatasetTimesteps
{
public:

  VISUS_CLASS(DatasetTimesteps)

  //___________________________________________________
  class IRange
  {
  public:

    int a,b,step;

    //constructor
    IRange(int a_,int b_,int step_=1):a(a_),b(b_),step(step_) 
    {VisusAssert(step>=1);}

    //operator==
    bool operator==(const IRange& other) const 
    {return a==other.a && b==other.b && step==other.step;}

    //operator!=
    bool operator!=(const IRange& other) const
    {return !(operator==(other));}

    //containsTimestep
    bool containsTimestep(double t) const 
    {return ((int)t==t) && (a<=t && t<=b) && (step==1 || (((int)t-a) % step)==0);}
  };


  //constructor
  DatasetTimesteps() 
  {}

  //constructor
  DatasetTimesteps(double a,double b,double step=1.0) 
  {addTimesteps(a,b,step);}

  //destructor
  ~DatasetTimesteps()
  {}

  //star (*) 
  static DatasetTimesteps star()
  {DatasetTimesteps ret(NumericLimits<int>::lowest(),NumericLimits<int>::highest(),1);return ret;}

  //clear
  void clear()
  {values.clear();}

  //empty
  bool empty() const
  {return values.empty();}

  //size
  inline int size() const
  {return (int)values.size();}

  //getAt
  inline const IRange& getAt(int I) const
  {return values[I];}


  //operator==
  inline bool operator==(const DatasetTimesteps& other) const
  {return values==other.values;}

  //operator!=
  bool operator!=(const DatasetTimesteps& other) const
  {return !(operator==(other));}

    //containsTimestep
  bool containsTimestep(double t) const
  {
    for (int I=0;I<size();I++)
      if (values[I].containsTimestep(t)) return true;
    return false;
  }

  //addTimesteps
  void addTimesteps(const IRange& irange)
  {values.push_back(irange);}

  //addTimesteps
  void addTimesteps(const DatasetTimesteps& other)
  {for (int I=0;I<other.size();I++) addTimesteps(other.getAt(I));}

  //addTimesteps
  void addTimesteps(double from,double to,double step)
  {
    VisusAssert((int)from==from && (int)to==to && (int)step==step);
    addTimesteps(IRange((int)from,(int)to,(int)step));
  }

   //addTimestep
  void addTimestep(double t)
  {VisusAssert((int)t==t);addTimesteps(IRange((int)t,(int)t,1));} 

  //getDefault
  double getDefault() const
  {return empty() || *this==star()? 0.0 : values.begin()->a;}

  //getMin
  double getMin() const
  {
    double ret=empty()? 0.0 : getAt(0).a;
    for (int I=1;I<size();I++) ret=std::min(ret,(double)getAt(I).a);
    return ret;
  }

  //getMax
  double getMax() const
  {
    double ret=empty()? 0.0 : getAt(0).b;
    for (int I=1;I<size();I++) ret=std::max(ret,(double)getAt(I).b);
    return ret;
  }

  //getRange
  Range getRange() const {
    double m=getMin(),M=getMax();
    return Range(m,M,int(m)==m && int(M)==M? 1.0 : 0.0);
  }

  //asVector
  std::vector<double> asVector() const
  {
    std::set<double> ret;
    for (int I=0;I<size();I++)
    {
      for (int T=getAt(I).a;T<=getAt(I).b;T+=getAt(I).step)
        ret.insert((double)T);
    }
    return std::vector<double>(ret.begin(), ret.end()); 
  }

  //toString
  String toString() const
  {
    StringTree stree("DatasetTimesteps");
    ObjectStream out(stree,'w');
    const_cast<DatasetTimesteps*>(this)->writeToObjectStream(out);
    return stree.toString();
  }

public:

  //writeToObjectStream
  void writeToObjectStream(ObjectStream& ostream);

  //readFromObjectStream
  void readFromObjectStream(ObjectStream& istream);

private:

  std::vector<IRange> values;

};


} //namespace Visus

#endif //__VISUS_DB_DATASET_TIMESTEPS_H