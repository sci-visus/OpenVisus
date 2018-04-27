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

#ifndef VISUS_PLANE_H
#define VISUS_PLANE_H

#include <Visus/Kernel.h>
#include <Visus/Point.h>

namespace Visus {

/////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Plane 
{
public:

  VISUS_CLASS(Plane)

  double x,y,z,w;

  //default constructor
  inline Plane() : x(0),y(0),z(1),w(0)
    {}

  //constructor
  inline explicit Plane(double X,double Y,double Z,double W) 
  {
    double len=sqrt(X*X + Y*Y + Z*Z); if (!len) len=1;
    x=X/len;y=Y/len;z=Z/len;w=W/len; //normalize!
  }

  //constructor from normal and distance
  inline explicit Plane(Point3d n,double d) 
  {
    n=n.normalized();
    x=n.x;y=n.y;z=n.z;w=-d; 
  }

  //constructor from normal and point
  inline explicit Plane(Point3d n,Point3d p)
  {
    n=n.normalized();
    x=n.x;
    y=n.y;
    z=n.z;
    w=-(n*p);
  }

  //from 3 points
  inline explicit Plane(Point3d p0,Point3d p1,Point3d p2)
  {
    Point3d n=(p1-p0).cross(p2-p0).normalized();
    x=n.x;
    y=n.y;
    z=n.z;
    w=-1*(n*p0);
  }

  //constructor from string
  inline explicit Plane(String s)
  {std::istringstream in(s);in>>x>>y>>z>>w;}

  //toString
  String toString() const
  {std::ostringstream out;out<<x<<" "<<y<<" "<<z<<" "<<w;return out.str();}

  //getNormal
  inline Point3d getNormal() const 
  {return Point3d(x,y,z);}


  //getDistance
  template <typename T>
  inline T getDistance(T v[3]) const {
    return T(x) * v[0] + T(y) * v[1] + T(z) * v[2] + T(w);
  }

  //getDistance
  inline double getDistance(const Point3d& v) const 
  {return x*v.x + y*v.y + z*v.z + w; }

  //! projectPoint (see http://www.9math.com/book/projection-Point3d-plane)
  inline Point3d projectPoint(Point3d P) const
  {
    Point3d N=getNormal();
    return P-(N * getDistance(P));
  }

  //! projectVector (see http://www.gamedev.net/community/forums/topic.asp?topic_id=345149&whichpage=1&#2255698)
  inline Point3d projectVector(Point3d V) const
  {
    Point3d N=getNormal();
    return V-(N *(V*N));
  }


};//end class Plane


} //namespace Visus

#endif //VISUS_PLANE_H

