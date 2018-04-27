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

#ifndef VISUS_LINE_H
#define VISUS_LINE_H

#include <Visus/Kernel.h>
#include <Visus/Point.h>

namespace Visus {

////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Line2d
{
public:

  VISUS_CLASS(Line2d)

  double a=0,b=0,c=0;

  //constructor
  Line2d(Point3d p) {
    setCoefficients(p.x,p.y,p.z);
  }

  //constructor
  Line2d(double a,double b,double c){
    setCoefficients(a,b,c);
  }

  //setCoefficients
  void setCoefficients(double a,double b,double c)
  {
    this->a=this->b=this->c=0;
    double norm=sqrt(a*a+b*b);
    if (!norm) return;
    this->a=a/norm;
    this->b=b/norm;
    this->c=c/norm;
  }

  //valid
  bool valid() const {
    return a || b;
  }

  //getNormal
  Point2d getNormal() const {
    return Point2d(a,b);
  }

  //distance
  double distance(Point2d p){
    return a*p.x+b*p.y+c;
  }

};

/////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Line3d
{
public:

  VISUS_CLASS(Line3d)

  //constructor
  inline Line3d() :origin(0,0,0),direction(0,0,1)
  {}

  //constructor
  inline Line3d(Point3d origin_,Point3d direction_) :origin(origin_),direction(direction_.normalized())
    {}

  //getCenter
  inline Point3d getOrigin() const
    {return origin;}

  //getDirection
  inline Point3d getDirection() const
    {return direction;}

  //get a point to a certain distance
  inline Point3d getPoint(double t) const
    {return origin+t*direction;}

protected:

  Point3d origin;
  Point3d direction;

}; //end class Line3d


} //namespace Visus

#endif //VISUS_LINE_H

