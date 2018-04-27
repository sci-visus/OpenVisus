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

#include <Visus/Polygon.h>

#include <functional>

namespace Visus {


//////////////////////////////////////////////////////////////////
class ClipPolygon2d
{
public:

  //______________________________________________
  template <class Comp>
  class BoundaryHor
  {
  public:
    BoundaryHor(double y_) : m_Y(y_) {}
    bool isInside(const Point2d& pnt) const { return Comp()(pnt.y, m_Y); }	// return true if pnt.y is at the inside of the boundary
    Point2d intersect(const Point2d& p0, const Point2d& p1) const			// return intersection point of line p0...p1 with boundary
    {																	// assumes p0...p1 is not strictly horizontal

      Point2d d = p1 - p0;
      double xslope = d.x / d.y;

      Point2d r;
      r.y = m_Y;
      r.x = p0.x + xslope * (m_Y - p0.y);
      return r;
    }
  private:
    double m_Y;
  };

  //______________________________________________
  template <class Comp>
  class BoundaryVert
  {
  public:
    BoundaryVert(double x_) : m_X(x_) {}
    bool isInside(const Point2d& pnt) const { return Comp()(pnt.x, m_X); }
    Point2d intersect(const Point2d& p0, const Point2d& p1) const		// assumes p0...p1 is not strictly vertical
    {

      Point2d d = p1 - p0;
      double yslope = d.y / d.x;

      Point2d r;
      r.x = m_X;
      r.y = p0.y + yslope * (m_X - p0.x);
      return r;
    }
  private:
    double m_X;
  };

  //______________________________________________
  template <class BoundaryClass, class Stage>
  class ClipStage : private BoundaryClass
  {
  public:
    ClipStage(Stage& nextStage, double position)
      : BoundaryClass(position)
      , m_NextStage(nextStage)
      , m_bFirst(true)
    {}

    //handleVertex
    void handleVertex(const Point2d& pntCurrent)	// Function to handle one vertex
    {
      bool bCurrentInside = BoundaryClass::isInside(pntCurrent);		// See if vertex is inside the boundary.

      if (m_bFirst)	// If this is the first vertex...
      {
        m_pntFirst = pntCurrent;	// ... just remember it,...

        m_bFirst = false;
      }
      else		// Common cases, not the first vertex.
      {
        if (bCurrentInside)	// If this vertex is inside...
        {
          if (!m_bPreviousInside)		// ... and the previous one was outside
            m_NextStage.handleVertex(BoundaryClass::intersect(m_pntPrevious, pntCurrent));
          // ... first output the intersection point.

          m_NextStage.handleVertex(pntCurrent);	// Output the current vertex.
        }
        else if (m_bPreviousInside) // If this vertex is outside, and the previous one was inside...
          m_NextStage.handleVertex(BoundaryClass::intersect(m_pntPrevious, pntCurrent));
        // ... output the intersection point.

        // If neither current vertex nor the previous one are inside, output nothing.
      }
      m_pntPrevious = pntCurrent;		// Be prepared for next vertex.
      m_bPreviousInside = bCurrentInside;
    }

    //finalize
    void finalize()
    {
      handleVertex(m_pntFirst);		// Close the polygon.
      m_NextStage.finalize();			// Delegate to the next stage.
    }


  private:
    Stage& m_NextStage;			// the next stage
    bool m_bFirst;				// true if no vertices have been handled
    Point2d m_pntFirst;			// the first vertex
    Point2d m_pntPrevious;		// the previous vertex
    bool m_bPreviousInside;		// true if the previous vertex was inside the Boundary
  };

  //______________________________________________
  class OutputStage
  {
  public:
    OutputStage() : m_pDest(0) {}
    void setDestination(std::vector<Point2d> * pDest) { m_pDest = pDest; }
    void handleVertex(const Point2d& pnt) { m_pDest->push_back(pnt); }	// Append the vertex to the output container.
    void finalize() {}		// Do nothing.
  private:

    std::vector<Point2d> * m_pDest;
  };

  const std::vector<Point2d>& polygon;

  typedef ClipStage< BoundaryHor<  std::less<double>          >, OutputStage>	ClipBottom;
  typedef ClipStage< BoundaryVert< std::greater_equal<double> >, ClipBottom>	ClipLeft;
  typedef ClipStage< BoundaryHor<  std::greater_equal<double> >, ClipLeft>		ClipTop;
  typedef ClipStage< BoundaryVert< std::less<double>          >, ClipTop>		  ClipRight;

  // Our data members.
  OutputStage m_stageOut;
  ClipBottom  m_stageBottom;
  ClipLeft    m_stageLeft;
  ClipTop     m_stageTop;
  ClipRight   m_stageRight;

  // SutherlandHodgman algorithm
  ClipPolygon2d(const Rectangle2d& rect,const std::vector<Point2d>& polygon_)
    : m_stageBottom(m_stageOut   , rect.p2().y)	
    , m_stageLeft  (m_stageBottom, rect.p1().x)		
    , m_stageTop   (m_stageLeft  , rect.p1().y)			
    , m_stageRight (m_stageTop   , rect.p2().x)
    , polygon(polygon_)
  {
  }

  //clipPolygon
  std::vector<Point2d> doClip()
  {
    std::vector<Point2d> ret;
    m_stageOut.setDestination(&ret);
    for (auto point : polygon) 
      m_stageRight.handleVertex(point);
    m_stageRight.finalize();
    return ret;
  }
};

//////////////////////////////////////////////////////////////////
Polygon2d Polygon2d::clip(const Rectangle2d& r) const
{
  return Polygon2d(ClipPolygon2d(r,this->points).doClip()); 
}


//////////////////////////////////////////////////////////////////
double Polygon2d::area() const {

  if (!valid())
    return 0;

  int N=(int)points.size();

  if (N<3)
    return 0.0;

  //http://paulbourke.net/geometry/polygonmesh/source1.c
  double area = 0;
  for (int i = 0; i < N; i++) 
  {
    int j = (i + 1) % N;
    area += points[i].x * points[j].y;
    area -= points[i].y * points[j].x;
  }

  area /= 2.0;
  return (area < 0 ? -area : area);
}


} //namespace Visus