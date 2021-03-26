/***************************************************
** ViSUS Visualization Project                    **
** Copyright (c) 2010 University of Utah          **
** Scientific Computing and Imaging Institute     **
** 72 S Central Campus Drive, Room 3750           **
** Salt Lake City, UT 84112                       **
**                                                **
** For information about this project see:        **
** http://www.pascucci.org/visus/                 **
**                                                **
**      or contact: pascucci@sci.utah.edu         **
**                                                **
****************************************************/

#ifndef _SLAM_CPP_H__
#define _SLAM_CPP_H__

#include <array>
#include <vector>
#include <limits>
#include <fstream>

#include <Visus/Time.h>
#include <Visus/Ray.h>
#include <Visus/File.h>

#include <Visus/Quaternion.h>
#include <Visus/Matrix.h>
#include <Visus/Color.h>
#include <Visus/Array.h>
#include <Visus/Polygon.h>

namespace Visus {

//predeclaration
class Pose;
class Calibration;
class Keypoint;
class Match;
class SlamEdge;
class Camera;
class Slam;

////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Pose
{
public:

  Quaternion q;
  Point3d    t;

  //constructor
  Pose() {
  }

  //constructor
  Pose(Quaternion q_, Point3d t_) : q(q_), t(t_) {
  }

  //constructor
  Pose(Matrix R, Point3d t) : Pose(R.toQuaternion(), t) {
  }

  //constructor
  Pose(Matrix T) : Pose(T.submatrix(3,3), T.getCol(3).withoutBack().toPoint3()) {
    VisusAssert(T.getSpaceDim()==4);
  }

  //identity
  static Pose identity() {
    return Pose();
  }

  //lookingDown
  static Pose lookingDown(Point3d t) {
    //# this transform the Z vector looking up to Z vector looking down(and mirror the image so it looks right
    Matrix R(
      1.0, 0.0, 0.0,
      0.0, -1.0, 0.0,
      0.0, 0.0, -1.0);

    return Pose(R, -(R * t));
  }

  //R
  Matrix R() const {
    return Matrix::rotate(q);
  }

  //isIdentity
  bool isIdentity() const {
    return !q.getAngle() && t == Point3d();
  }

  //toMatrix
  Matrix toMatrix() const {
    return Matrix(R(), t);
  }

  //operator*
  Pose operator*(const Pose& p2) const {
    const Pose& p1 = *this;
    const auto& q1 = p1.q; const auto& t1 = p1.t;
    const auto& q2 = p2.q; const auto& t2 = p2.t;
    return  Pose(q1*q2, q1*t2 + t1);
  }

  //inverse
  Pose inverse() const {
    auto qi = q.conjugate();
    return Pose(qi, -(qi*t));
  }

  //worldToCamera
  Point3d worldToCamera(Point3d worldpos) const {
    const auto& q = this->q;
    return q * worldpos + this->t;
  }

  //cameraToWorld
  Point3d cameraToWorld(Point3d eye) const {
    auto qi = this->q.conjugate();
    return qi * (eye - this->t);
  }

  //getWorldCenter
  Point3d getWorldCenter() const {
    return cameraToWorld(Point3d(0, 0, 0));
  }

  //writoToObjectStream
  void write(Archive& ar) const
  {
    ar.write("q", q);
    ar.write("t", t);
  }

  //read
  void read(Archive& ar)
  {
    ar.read("q", q);
    ar.read("t", t);
  }

};

////////////////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Calibration
{
public:

  double f, cx, cy;

  bool bFixed = false;

  //constructor
  Calibration(double f_ = 1, double cx_ = 0, double cy_ = 0)
    : f(f_), cx(cx_), cy(cy_) {
  }

  //toMatrix
  Matrix  toMatrix() const {
    return Matrix(
      f, 0, cx,
      0, f, cy,
      0, 0, 1);
  }

  //screenToCamera
  inline Point3d screenToCamera(const Point2d& screen) const
  {
    return Point3d(
      ((screen.x - double(this->cx)) / +double(this->f)),
      ((screen.y - double(this->cy)) / -double(this->f)),
      double(1));
  }

  //worldToCamera (note -f for left/right handed!)
  inline Point2d cameraToScreen(const Point3d& eye) const
  {
    return Point2d(
      double(+this->f) * (eye[0] / eye[2]) + double(this->cx),
      double(-this->f) * (eye[1] / eye[2]) + double(this->cy));
  }


  //write
  void write(Archive& ar) const
  {
    ar.write("f", f);
    ar.write("cx", cx);
    ar.write("cy", cy);
  }

  //read
  void read(Archive& ar)
  {
    ar.read("f", f);
    ar.read("cx", cx);
    ar.read("cy", cy);
  }

};

////////////////////////////////////////////////////////////
class VISUS_KERNEL_API KeyPoint
{
public:

  float              x, y;
  float              size;
  float              angle;
  float              response;
  int                octave;
  int                class_id;

  //constructor
  KeyPoint(float _x = 0, float _y = 0, float _size = 0, float _angle = -1, float _response = 0, int _octave = 0, int _class_id = -1)
    :x(_x), y(_y), size(_size), angle(_angle), response(_response), octave(_octave), class_id(_class_id) {
  }

  //adaptiveNonMaximalSuppression (NOTE: responses,xs and ys must be sorted in DESC order)
  static std::vector<int> adaptiveNonMaximalSuppression(const std::vector<float>& responses, const std::vector<float>& xs, const std::vector<float>& ys, int max_keypoints)
  {
    int N = (int)responses.size();

    //http://answers.opencv.org/question/93317/orb-keypoints-distribution-over-an-image/#93395
    //anms (NOTE swift can have problem with duplicated keypoints,see above)
    //NOTE anms seems to be more stable than using the 'response'

    std::vector<double> r2(N);
    for (int I = 0; I < N; ++I)
    {
      float response = responses[I] * 1.11;

      //find the radius to a 'stronger' keypoint
      r2[I] = std::numeric_limits<double>::max();
      for (int J = 0; J < I && responses[J] > response; ++J)
      {
        auto dx = xs[I] - xs[J];
        auto dy = ys[I] - ys[J];
        r2[I] = std::min(r2[I], (double)dx * dx + (double)dy * dy);
      }
    }

    auto sorted_r2 = r2;
    std::sort(sorted_r2.begin(), sorted_r2.end(), [&](const double& A, const double& B) {
      return A > B;
      });
    double r2_threshold = sorted_r2[max_keypoints];

    std::vector<int> ret;
    ret.reserve(N);
    for (int I = 0; I < N; ++I)
    {
      if (r2[I] >= r2_threshold)
        ret.push_back(I);
    }

    return ret;
  }

};

////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Match
{
public:

  int queryIdx;
  int trainIdx; 
  int imgIdx;   
  float distance;

  //constructor
  Match(int _queryIdx=-1, int _trainIdx = -1, int _imgIdx = -1, float _distance= std::numeric_limits<float>::max())
    : queryIdx(_queryIdx), trainIdx(_trainIdx), imgIdx(_imgIdx), distance(_distance) {}

  //operator<
  bool operator<(const Match& m) const {
    return distance < m.distance;
  }
};

////////////////////////////////////////////////////////////
class VISUS_KERNEL_API SlamEdge
{
public:

  //origin
  Camera* origin = nullptr;

  //other
  Camera* other = nullptr;

  //text
  String text;

  //matches
  std::vector<Match> matches;
  
  //constructor
  SlamEdge(Camera* origin_, Camera* other_) : origin(origin_), other(other_) {
  }

  //getNumberOfMatches
  int getNumberOfMatches() const {
    return (int)matches.size();
  }

  //isGood
  bool isGood() const {
    return !matches.empty();
  }

  //setMatches
  void setMatches(const std::vector<Match>& matches, String text);

};

////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Camera
{
public:

  VISUS_NON_COPYABLE_CLASS(Camera)

  // id
  int id=0;

  //idx filename
  String idx_filename;

  //filenames 
  std::vector<String> filenames;

  //color
  Color color = Color::random();

  //keypoints
  std::vector<KeyPoint> keypoints;

  //descriptors
  Array descriptors;

  //pose
  Pose pose;

  //bFixed
  bool bFixed = false;

  //edges
  std::vector<SlamEdge*> edges;

  //homography
  Matrix homography;

  //quad (in world coordinates)
  Quad quad;

  //residuals
  struct
  {
    void* vertex = 0;
  }
  ba;

  //constructor
  Camera() {
  }

  //!destructor
  ~Camera() {
    for (auto edge : edges)
      delete edge;
  }

  //getNumberOfKeyPoints
  int getNumberOfKeyPoints() const {
    return (int)keypoints.size();
  }

  //worldToCamera
  Point3d worldToCamera(Point3d worldpos) const {
    return pose.worldToCamera(worldpos);
  }

  //cameraToWorld
  Point3d cameraToWorld(Point3d eye) const {
    return pose.cameraToWorld(eye);
  }

  //getWorldCenter
  Point3d getWorldCenter() const {
    return pose.getWorldCenter();
  }

  //getViewDirection
  Point3d getWorldViewDirection() const {

    auto p0 = cameraToWorld(Point3d(0, 0, 0));
    auto p1 = cameraToWorld(Point3d(0, 0, 1));
    return (p1 - p0).normalized();
  }

  //addLocalCamera
  void addLocalCamera(Camera* camera1) {
    if (this == camera1) return;
    if (this->getEdge(camera1)) return;
    auto camera2 = this;
    camera2->edges.push_back(new SlamEdge(camera2,camera1));
    camera1->edges.push_back(new SlamEdge(camera1,camera2));
  }

  //getEdge
  SlamEdge* getEdge(Camera* other) {
    for (auto edge : this->edges)
      if (edge->other == other)
        return edge;
    return nullptr;
  }

  //removeLocalCamera
  void removeLocalCamera(Camera* camera1) {
    if (!this->getEdge(camera1)) return;
    auto camera2 = this;
    auto edge2 = camera2->getEdge(camera1); Utils::remove(camera2->edges,edge2); delete edge2;
    auto edge1 = camera1->getEdge(camera2); Utils::remove(camera1->edges,edge1); delete edge1;
  }

  //getAllLocalCameras
  std::vector<Camera*> getAllLocalCameras() const {
    std::vector<Camera*> ret;
    for (auto edge : edges) 
      ret.push_back(edge->other);
    return ret;
  }

  //getGoodLocalCameras 
  std::vector<Camera*> getGoodLocalCameras() const {
    std::vector<Camera*> ret;
    for (auto edge : edges) {
      if (edge->isGood())
        ret.push_back(edge->other);
    }
    return ret;
  }

};


inline 
void SlamEdge::setMatches(const std::vector<Match>& matches, String text)
{
  auto cross = this->other->getEdge(this->origin);
  this->matches = matches; this->text = text;
  cross->matches = matches; cross->text = text;
  for (auto& it : cross->matches)
    std::swap(it.queryIdx, it.trainIdx);
}


//////////////////////////////////////////////////////////
class VISUS_KERNEL_API Slam
{
public:

  //url
  String url;

  //single image dimensions and type
  int width = 0;
  int height = 0;
  DType dtype;

  //cameras
  std::vector<Camera*> cameras;

  //calibration
  Calibration calibration;

  //constructor
  Slam() {
  }

  //destructor
  virtual ~Slam() {
    for (auto it : cameras)
      delete it;
  }

  //addCamera
  void addCamera(Camera* VISUS_DISOWN(camera)) {
    camera->quad = Quad(this->width, this->height);
    camera->homography = Matrix(3);
    this->cameras.push_back(camera);
  }

  //previousCamera
  Camera* previousCamera(Camera* camera) const
  {
    if (cameras.empty() || cameras.front() == camera)
      return nullptr;

    for (int I = 0; I < (int)cameras.size(); I++)
    {
      if (cameras[I] == camera)
        return cameras[I - 1];
    }
    return nullptr;
  }

  //previousCamera
  Camera* nextCamera(Camera* camera) const
  {
    if (cameras.empty() || cameras.back() == camera)
      return nullptr;

    for (int I = 0; I < (int)cameras.size(); I++)
    {
      if (cameras[I] == camera)
        return cameras[I + 1];
    }
    return nullptr;
  }

  //removeCamera
  void removeCamera(Camera* camera2)
  {
    for (auto camera1 : camera2->getAllLocalCameras())
      camera2->removeLocalCamera(camera1);
    VisusReleaseAssert(camera2->edges.empty());

    //PrintInfo("removing camera " , camera2->id ," from group ", GroupId);
    Utils::remove(this->cameras, camera2);
    delete camera2;
  }

  //findGroups
  std::vector< std::vector<Camera*> > findGroups() const
  {
    std::vector< std::vector<Camera*> > groups;

    std::set<Camera*> toassign(this->cameras.begin(), this->cameras.end());
    while (!toassign.empty())
    {
      std::set<Camera*> group;

      auto first = *toassign.begin();

      std::stack<Camera*> stack;
      stack.push(first);
      while (!stack.empty())
      {
        auto camera = stack.top();
        stack.pop();

        if (group.count(camera))
          continue;

        group.insert(camera);
        toassign.erase(camera);

        for (auto it : camera->getGoodLocalCameras())
          stack.push(it);
      }

      //sort by camera id
      auto v = std::vector<Camera*>(group.begin(), group.end());
      std::sort(v.begin(), v.end(), [](Camera* c1, Camera* c2) {
        return c1->id < c2->id;
        });

      groups.push_back(v);
    }

    //sort in descending order
    std::sort(groups.begin(), groups.end(), [](const std::vector<Camera*>& a, const std::vector<Camera*>& b) {
      return a.size() > b.size();
      });

    //print the groups
    PrintInfo("Found the following groups");
    for (int GroupId = 0; GroupId < (int)groups.size(); GroupId++)
    {
      auto group = groups[GroupId];
      std::ostringstream out;
      out << "Group " << GroupId << " #cameras(" << group.size() << ")";
      for (auto camera : group)
        out << " " << camera->id;
      PrintInfo(out.str());
    }

    return groups;
  }

  //removeDisconnectedCameras
  void removeDisconnectedCameras()
  {
    int old_num_cameras = (int)this->cameras.size();

    auto to_remove = findGroups();
    to_remove.erase(to_remove.begin());

    for (auto group : to_remove) {
      for (auto camera : group)
        removeCamera(camera);
    }

    int num_removed = old_num_cameras - (int)this->cameras.size();
    if (num_removed)
      PrintInfo("Removed num_disconnected(", num_removed, ") cameras");
  }

  //removeCamerasWithTooMuchSkew
  void removeCamerasWithTooMuchSkew()
  {
    for (auto camera : std::vector<Camera*>(this->cameras))
    {
      auto quad = camera->quad;
      if (!quad.isConvex() || quad.wrongAngles() || quad.wrongScale(this->width, this->height))
      {
        PrintInfo("Removing camera ", camera->id, " because non convex or too much skew");
        removeCamera(camera);
      }
    }
  }

  //computeWorldQuad
  // note: camera->quad is in pixel coordinates
  Quad computeWorldQuad(Camera* camera)
  {
    int W = this->width;
    int H = this->height;
    auto wc = camera->pose.getWorldCenter();
    auto p0 = Ray::fromTwoPoints(wc, camera->pose.cameraToWorld(calibration.screenToCamera(Point2d(0, 0)))).findIntersectionOnZeroPlane().toPoint2();
    auto p1 = Ray::fromTwoPoints(wc, camera->pose.cameraToWorld(calibration.screenToCamera(Point2d(W, 0)))).findIntersectionOnZeroPlane().toPoint2();
    auto p2 = Ray::fromTwoPoints(wc, camera->pose.cameraToWorld(calibration.screenToCamera(Point2d(W, H)))).findIntersectionOnZeroPlane().toPoint2();
    auto p3 = Ray::fromTwoPoints(wc, camera->pose.cameraToWorld(calibration.screenToCamera(Point2d(0, H)))).findIntersectionOnZeroPlane().toPoint2();
    return { p0,p1,p2,p3 };
  }

  //getQuadsBox
  BoxNd getQuadsBox() const
  {
    auto box = BoxNd::invalid();
    for (auto camera : cameras)
    {
      for (auto point : camera->quad.points)
        box.addPoint(point);
    }
    return box;
  }

  //refreshQuads
  void refreshQuads()
  {
    auto box = BoxNd::invalid();
    double avg_physical_area = 0.0;
    for (auto camera : cameras)
    {
      auto quad = computeWorldQuad(camera);

      for (auto it : quad.points)
        box.addPoint(Point3d(it));

      avg_physical_area += double(quad.area()) / double(cameras.size());
    }

    //try to keep the same number of pixels
    //i.e this->width*this->height = avg_physical_area*scale*scale
    double avg_pixel_area = double(this->width) * double(this->height);
    double scale = sqrt(avg_pixel_area / avg_physical_area);

    auto T =
      Matrix::scale(Point2d(scale, scale)) *
      Matrix::translate(-box.p1.toPoint2());

    for (auto camera : this->cameras)
    {
      auto dst = Quad(T, computeWorldQuad(camera));
      auto src = Quad(this->width, this->height);

      camera->homography = Quad::findQuadHomography(dst, src);
      camera->quad = Quad(camera->homography, Quad(this->width, this->height));
    }
  }

  //loadKeyPoints
  bool loadKeyPoints(Camera* camera2, String filename)
  {
    std::fstream in(filename.c_str(), std::ios::in | std::ios::binary);
    if (!in.is_open())
      return false;

    Int64 nkyepoints = 0;
    in.read((char*)&nkyepoints, sizeof(Int64));
    camera2->keypoints.resize(nkyepoints);

    if (nkyepoints)
    {
      //read keypoints
      in.read((char*)&camera2->keypoints[0], nkyepoints * sizeof(KeyPoint));

      //write descriptors
      int width = 0, height = 0, type = 0;
      in.read((char*)&height, sizeof(int)); VisusReleaseAssert(height == nkyepoints);
      in.read((char*)&width, sizeof(int));
      in.read((char*)&type, sizeof(int)); VisusReleaseAssert(type == 0); //means "uint8"
      camera2->descriptors = Array(width, height, DTypes::UINT8);
      in.read((char*)camera2->descriptors.c_ptr(), camera2->descriptors.c_size());
    }

    return true;
  }

  //saveKeyPoints
  bool saveKeyPoints(Camera* camera2, String filename)
  {
    FileUtils::createDirectory(Path(filename).getParent());
    std::fstream out(filename.c_str(), std::ios::out | std::ios::binary);
    if (!out.is_open())
      return false;

    //write keypoints
    Int64 nkeypoints = (Int64)camera2->keypoints.size();
    out.write((char*)&nkeypoints, sizeof(Int64));

    if (nkeypoints)
    {
      out.write((char*)&camera2->keypoints[0], nkeypoints * sizeof(KeyPoint));

      //write descriptors
      int width = (int)camera2->descriptors.getWidth();
      int height = (int)camera2->descriptors.getHeight(); VisusReleaseAssert(height == nkeypoints);
      int type = 0; VisusReleaseAssert(camera2->descriptors.dtype == DTypes::UINT8);
      out.write((char*)&height, sizeof(int));
      out.write((char*)&width, sizeof(int));
      out.write((char*)&type, sizeof(int));
      out.write((char*)camera2->descriptors.c_ptr(), camera2->descriptors.c_size());
    }

    return true;
  }

  //removeOutlierMatches
  void removeOutlierMatches(double max_reproj_error)
  {
    Time t1 = Time::now();

    int num_inliers = 0;
    int num_outliers = 0;

    for (auto camera2 : this->cameras)
    {
      for (auto camera1 : camera2->getGoodLocalCameras())
      {
        if (camera1->id < camera2->id)
        {
          auto& matches1 = camera2->getEdge(camera1)->matches;
          auto& matches2 = camera1->getEdge(camera2)->matches;

          VisusReleaseAssert(!matches1.empty());
          VisusReleaseAssert(matches1.size() == matches2.size());

          auto Hw1 = camera1->homography; auto H1w = Hw1.invert();
          auto Hw2 = camera2->homography; auto H2w = Hw2.invert();

          auto H21 = H2w * Hw1;
          auto H12 = H1w * Hw2;

          int ngood = 0;
          for (int I = 0; I < (int)matches1.size(); I++)
          {
            const auto& k1 = camera1->keypoints[matches1[I].queryIdx]; auto point1 = Point2d(k1.x, k1.y); auto point21 = (H21 * Point3d(point1, 1.0)).dropHomogeneousCoordinate();
            const auto& k2 = camera2->keypoints[matches1[I].trainIdx]; auto point2 = Point2d(k2.x, k2.y); auto point12 = (H12 * Point3d(point2, 1.0)).dropHomogeneousCoordinate();

            auto d1 = Utils::square(point21.x - point2.x) + Utils::square(point21.y - point2.y);
            auto d2 = Utils::square(point12.x - point1.x) + Utils::square(point12.y - point1.y);
            auto error = sqrt(Utils::max(d1, d2));

            if (error <= max_reproj_error)
            {
              matches1[ngood] = matches1[I];
              matches2[ngood] = matches2[I];
              ++ngood;
              ++num_inliers;
            }
            else
            {
              ++num_outliers;
            }
          }

          matches1.resize(ngood);
          matches2.resize(ngood);
        }
      }
    }

    PrintInfo("Removed outliers in ", t1.elapsedMsec(), "msec"
      , " num_inliers(", num_inliers, ")"
      , " num_outliers(", num_outliers, ")"
      , " max_reproj_error(", max_reproj_error, ")");
  }

  //bundleAdjustment
  void bundleAdjustment(double ba_tolerance, String algorithm="");

  //doPostIterationAction
  virtual void doPostIterationAction() {
  }

};

} //namespace Visus

#endif // _SLAM_CPP_H__
