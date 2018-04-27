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

#ifndef VISUS_RAY_H
#define VISUS_RAY_H

#include <Visus/Kernel.h>
#include <Visus/Box.h>
#include <Visus/Matrix.h>
#include <Visus/Sphere.h>
#include <Visus/Line.h>
#include <Visus/Segment.h>
#include <Visus/Circle.h>

namespace Visus {


/////////////////////////////////////////////////////////////////////
template <typename T>
class Ray3
{
public:

  VISUS_CLASS(Ray3)

  //constructor
  Ray3() :origin(T(0),T(0),T(0)),direction(T(0),T(0),T(1)) {
  }

  //constructor
  Ray3(Point3<T> origin_, Point3<T> direction_)
    :origin(origin_),direction(direction_.normalized()) {
  }

  //fromTwoPoints
  static Ray3 fromTwoPoints(const Point3<T>& A, const Point3<T>& B) {
    return Ray3(A,(B-A));
  }

  //valid
  bool valid() const {
    return this->origin.valid() && this->direction.module2();
  }

  //getOrigin
  const Point3<T>& getOrigin() const {
    return this->origin;
  }

  //getDirection
  const Point3<T>& getDirection() const {
    return this->direction;
  }

  //get a point to a certain distance
  Point3<T> getPoint(T alpha) const {
    return origin+ alpha *direction;
  }

  //transformByMatrix 
  Ray3 transformByMatrix(const Matrix&  M) const {
    return Ray3::fromTwoPoints(M * getPoint(T(0)), M * getPoint(T(1)));
  }

  //findIntersectionOnZeroPlane
  Point3<T> findIntersectionOnZeroPlane() const
  {
    VisusReleaseAssert(direction.z!=T(0));
    T alpha = -origin.z / direction.z;
    return getPoint(alpha);
  }

protected:

  Point3<T> origin;
  Point3<T> direction; 

}; //end class Ray3

typedef Ray3<double> Ray3d;

/////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API RayBoxIntersection
{
public:

  VISUS_CLASS(RayBoxIntersection)

  bool   valid;
  double tmin;
  double tmax;

  RayBoxIntersection(const Ray3d& ray,const Box3d& box);
};

/////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API RayPlaneIntersection
{
public:

  VISUS_CLASS(RayPlaneIntersection)

  bool     valid;
  double   t;
  Point3d  point;

  RayPlaneIntersection(const Ray3d& ray,const Plane& plane);
};


/////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API RaySphereIntersection
{
public:

  VISUS_CLASS(RaySphereIntersection)

  bool   valid;
  double tmin;
  double tmax;

  RaySphereIntersection(const Ray3d& ray,const Sphere& sp);
};


/////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API RayPointDistance
{
public:

  VISUS_CLASS(RayPointDistance)

  double   distance;
  Point3d  closest_ray_point ;

  RayPointDistance(const Ray3d& ray,const Point3d& point);
};


/////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API RayLineDistance
{
public:

  VISUS_CLASS(RayLineDistance)

  double   distance;
  Point3d  closest_ray_point ;
  Point3d  closest_line_point ;

  RayLineDistance(const Ray3d& ray,const Line3d& line);
};

/////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API RaySegmentDistance
{
public:

  VISUS_CLASS(RaySegmentDistance)

  double   distance;
  Point3d  closest_ray_point    ;
  Point3d  closest_segment_point;

  RaySegmentDistance(const Ray3d& ray,const Segment& segment);
};

/////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API RayCircleDistance
{
public:

  VISUS_CLASS(RayCircleDistance)

  double   distance;
  Point3d  closest_ray_point;
  Point3d  closest_circle_point;

  RayCircleDistance(const Ray3d& ray,const Circle& circle);

};


} //namespace Visus

#endif //VISUS_RAY_H

