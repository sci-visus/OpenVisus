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
  static std::vector<int> adaptiveNonMaximalSuppression(const std::vector<float>& responses, const std::vector<float>& xs, const std::vector<float>& ys, int anms);

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
  Slam();

  //destructor
  virtual ~Slam();

  //addCamera
  void addCamera(Camera* VISUS_DISOWN(camera));

  //previousCamera
  Camera* previousCamera(Camera* camera) const;

  //previousCamera
  Camera* nextCamera(Camera* camera) const;

  //removeCamera
  void removeCamera(Camera* camera2);

  //findGroups
  std::vector< std::vector<Camera*> > findGroups() const;

  //removeDisconnectedCameras
  void removeDisconnectedCameras();

  //removeCamerasWithTooMuchSkew
  void removeCamerasWithTooMuchSkew();

  //computeWorldQuad
  // note: camera->quad is in pixel coordinates
  Quad computeWorldQuad(Camera* camera);

  //getQuadsBox
  BoxNd getQuadsBox() const;

  //refreshQuads
  void refreshQuads();

  //loadKeyPoints
  bool loadKeyPoints(Camera* camera2, String filename);

  //saveKeyPoints
  bool saveKeyPoints(Camera* camera2, String filename);

  //removeOutlierMatches
  void removeOutlierMatches(double max_reproj_error);

  //bundleAdjustment
  void bundleAdjustment(double ba_tolerance, String algorithm="");

  //doPostIterationAction
  virtual void doPostIterationAction();

};

} //namespace Visus

#endif // _SLAM_CPP_H__
