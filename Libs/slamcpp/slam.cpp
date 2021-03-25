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

#if WIN32
#pragma warning(disable:4251 4267 4996 4244 4251)
#endif

#include "slam.h"

#include <g2o/core/block_solver.h>
#include <g2o/core/optimization_algorithm_levenberg.h>
#include <g2o/core/robust_kernel_impl.h>
#include <g2o/types/sba/types_six_dof_expmap.h>
#include <g2o/core/base_multi_edge.h>
#include <g2o/solvers/eigen/linear_solver_eigen.h>
#include <g2o/core/sparse_optimizer_terminate_action.h>


#include <map>
#include <fstream>
#include <iterator>
#include <thread>
#include <numeric>

#if WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <Visus/Time.h>
#include <Visus/Ray.h>
#include <Visus/File.h>

namespace Visus {

  ///////////////////////////////////////////////////////////////////////
void SlamEdge::setMatches(const std::vector<Match>& matches, String text)
{
  auto cross = this->other->getEdge(this->origin);
  this->matches  = matches; this->text  = text;
  cross->matches = matches; cross->text = text;
  for (auto& it : cross->matches)
    std::swap(it.queryIdx, it.trainIdx);
}

///////////////////////////////////////////////////////////////////////
std::vector<int> KeyPoint::adaptiveNonMaximalSuppression(const std::vector<float>& responses, const std::vector<float>& xs, const std::vector<float>& ys, int max_keypoints)
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


//////////////////////////////////////////////////////////////////////////////////
class BundleAdjustment
{
public:

  typedef Eigen::Matrix<double, 2, 1> Vector2d;
  typedef Eigen::Matrix<double, 3, 1> Vector3d;
  typedef Eigen::Matrix<double, 4, 1> Vector4d;
  typedef Eigen::Matrix<double, 5, 1> Vector5d;
  typedef Eigen::Matrix<double, 6, 1> Vector6d;

  //___________________________________________________________________
  //see https://github.com/RainerKuemmerle/g2o/blob/master/g2o/core/sparse_optimizer_terminate_action.cpp
  class MyPostIterationAction : public g2o::HyperGraphAction
  {
  public:

    BundleAdjustment* ba;
    bool              terminate_flag = false;
    double            lastChi = 0;

    //constructor
    MyPostIterationAction(BundleAdjustment* ba_) : ba(ba_) {
    }

    //operator()
    virtual HyperGraphAction* operator()(const g2o::HyperGraph* graph, Parameters* parameters = 0) override
    {
      g2o::SparseOptimizer* optimizer = static_cast<g2o::SparseOptimizer*>(const_cast<g2o::HyperGraph*>(graph));
      HyperGraphAction::ParametersIteration* params = static_cast<HyperGraphAction::ParametersIteration*>(parameters);

      bool stop = false;
      auto iteration = params->iteration;
      if (iteration >= 0)
      {
        optimizer->computeActiveErrors();
        double currentChi = optimizer->activeRobustChi2();
        PrintInfo("Bundle adjustment activeRobustChi2(" , currentChi , ")");
        if (iteration > 0)
        {
          double gain = (lastChi - currentChi) / currentChi;
          if (gain >= 0 && gain < ba->gainThreshold)
            stop = true;
        }
        lastChi = currentChi;

        //callbacks
        ba->updateSolution();
        ba->slam->doPostIterationAction();
      }

      if (optimizer->forceStopFlag())
      {
        *(optimizer->forceStopFlag()) = stop;
      }
      else
      {
        terminate_flag = stop;
        optimizer->setForceStopFlag(&terminate_flag);
      }

      return this;
    }
  };

  Slam* slam;
  g2o::SparseOptimizer* optimizer = nullptr;
  int                   VertexId = 0;
  double                gainThreshold;

  //constructor
  BundleAdjustment(Slam* slam_, double gainThreshold_) : slam(slam_), gainThreshold(gainThreshold_)
  {
    PrintInfo("Starting bundle adjustment...");

    //create optimizer
    optimizer = new g2o::SparseOptimizer();

    typedef g2o::BlockSolver< g2o::BlockSolverTraits<Eigen::Dynamic, Eigen::Dynamic> > SlamBlockSolver;
    typedef g2o::LinearSolverEigen<SlamBlockSolver::PoseMatrixType>                    SlamLinearSolver;

    VertexId = 0;

    auto linear_solver = new SlamLinearSolver();
    auto block_solver = new SlamBlockSolver(linear_solver);
    optimizer->setAlgorithm(new g2o::OptimizationAlgorithmLevenberg(block_solver));
  }

  //destructor
  virtual ~BundleAdjustment() {
    delete optimizer;
  }

  //setInitialSolution
  virtual void setInitialSolution()=0;

  //updateSolution
  virtual void updateSolution() = 0;

  //doBundleAdjustment
  void doBundleAdjustment() 
  {
    Time t1 = Time::now();

    setInitialSolution();
    
    optimizer->addPostIterationAction(new MyPostIterationAction(this));
    optimizer->initializeOptimization();
    optimizer->optimize(std::numeric_limits<int>::max());
    
    PrintInfo(" bundleAdjustment done in " , t1.elapsedMsec() , "msec", " #cameras(" , slam->cameras.size() , ")");
  }

};

//////////////////////////////////////////////////////////////////////////////////
class DefaultBundleAdjustment : public BundleAdjustment
{
public:

  //___________________________________________________________________
  class BACameraVertex : public g2o::BaseVertex<6, g2o::SE3Quat> {
  public:

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

      //constructor
      BACameraVertex() {
    }

    //constructor
    BACameraVertex(Camera* camera)
    {
      auto q = camera->pose.q;
      auto t = camera->pose.t;
      setEstimate(g2o::SE3Quat(Eigen::Quaterniond(q.w, q.x, q.y, q.z), Vector3d(t.x, t.y, t.z)));
    }

    virtual bool read(std::istream& is)        override { VisusReleaseAssert(false); return true; }
    virtual bool write(std::ostream& os) const override { VisusReleaseAssert(false); return true; }

    virtual void setToOriginImpl() override {
      _estimate = g2o::SE3Quat();
    }

    //oplusImpl
    virtual void oplusImpl(const double* update_) override {
      Eigen::Map<const Vector6d> update(update_);
      setEstimate(g2o::SE3Quat::exp(update) * estimate());
    }

  };

  //___________________________________________________________________
  class BACalibrationVertex : public g2o::BaseVertex<3, Vector3d>
  {
  public:

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

      //constructor
      BACalibrationVertex(const Calibration& calibration) {
      setEstimate(Vector3d(calibration.f, calibration.cx, calibration.cy));
    }

    virtual bool read(std::istream& is)        override { VisusReleaseAssert(false); return true; }
    virtual bool write(std::ostream& os) const override { VisusReleaseAssert(false); return true; }

    //setToOriginImpl
    virtual void setToOriginImpl() override
    {
      _estimate << 1.0, 0.0, 0.0;
    }

    //oplusImpl
    virtual void oplusImpl(const double* update_) override
    {
      Vector3d update(update_);
      setEstimate(estimate() + update);
    }
  };

  //___________________________________________________________________
  class BAPose
  {
  public:

    const g2o::SE3Quat& w2c;
    g2o::SE3Quat        c2w;
    const Calibration& calibration;

    //constructor
    BAPose(const BACameraVertex* vertex, const Calibration& calibration_)
      : w2c(vertex->estimate()), calibration(calibration_) {
      c2w = w2c.inverse();
    }

    //cameraToWorld
    Point3d cameraToWorld(const Point3d& eye) const {
      auto ret = c2w.map(Vector3d(eye.x, eye.y, eye.z));
      return Point3d(ret.x(), ret.y(), ret.z());
    }

    //worldToCamera
    Point3d worldToCamera(const Point3d& world) const {
      auto ret = w2c.map(Vector3d(world.x, world.y, world.z));
      return Point3d(ret.x(), ret.y(), ret.z());
    }

    //worldToScreen
    Point2d worldToScreen(const Point3d& world) const {
      return calibration.cameraToScreen(worldToCamera(world));
    }

    //getWorldRay
    Ray getWorldRay(const Point2d& screen) const {
      auto w0 = cameraToWorld(Point3d());
      auto w1 = cameraToWorld(calibration.screenToCamera(screen));
      return Ray::fromTwoPoints(w0, w1);
    }

  };

  //___________________________________________________________________
  class BAEdge : public g2o::BaseMultiEdge<
    /*error_dim*/4,
    /*error_vector*/Vector4d>
  {
  public:

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    Point2d s1;
    Point2d s2;

    //constructor
    BAEdge(Camera* camera1, const Point2d& s1_, Camera* camera2, const Point2d& s2_, BACalibrationVertex* Kvertex)
      : s1(s1_), s2(s2_)
    {
      resize(3); //num vertices

      setVertex(0, static_cast<BACameraVertex*>(camera1->ba.vertex));
      setVertex(1, static_cast<BACameraVertex*>(camera2->ba.vertex));
      setVertex(2, Kvertex);

      setMeasurement(Vector4d(s1.x, s1.y, s2.x, s2.y));
      setInformation(Eigen::Matrix4d::Identity());
      setRobustKernel(new g2o::RobustKernelHuber());
    }

    virtual bool read(std::istream& is)        override { VisusReleaseAssert(false); return true; }
    virtual bool write(std::ostream& os) const override { VisusReleaseAssert(false); return true; }

    //virtual void linearizeOplus() override; NOTE this can be faster If I knew how to compute the Jacobian

    //computeError
    virtual void computeError() override
    {
      auto v0 = (BACameraVertex*)_vertices[0];
      auto v1 = (BACameraVertex*)_vertices[1];
      auto v2 = (BACalibrationVertex*)_vertices[2];

      const Vector3d& K = v2->estimate();

      Calibration calibration(K.x(), K.y(), K.z());

      BAPose camera1(v0, calibration);
      BAPose camera2(v1, calibration);

      auto S1 = camera1.worldToScreen(camera2.getWorldRay(s2).findIntersectionOnZeroPlane().toPoint3());
      auto S2 = camera2.worldToScreen(camera1.getWorldRay(s1).findIntersectionOnZeroPlane().toPoint3());

      _error[0] = S1.x - s1.x;
      _error[1] = S1.y - s1.y; 
      _error[2] = S2.x - s2.x; 
      _error[3] = S2.y - s2.y; 
    }
  };

  BACalibrationVertex*  Kvertex = nullptr;

  //constructor
  DefaultBundleAdjustment(Slam* slam_, double gainThreshold_) 
    : BundleAdjustment(slam_, gainThreshold_)
  {
  }

  //destructor
  virtual ~DefaultBundleAdjustment() {
  }

  //setInitialSolution
  virtual void setInitialSolution() override
  {
    //add calibration vertex
    Kvertex = new BACalibrationVertex(slam->calibration);
    Kvertex->setId(VertexId++);
    Kvertex->setFixed(slam->calibration.bFixed);
    optimizer->addVertex(Kvertex);

    //add camera vertex
    for (auto camera : slam->cameras)
    {
      auto vertex = new BACameraVertex(camera);
      vertex->setId(VertexId++);
      vertex->setFixed(camera->bFixed);
      optimizer->addVertex(vertex);
      camera->ba.vertex = vertex;
    }

    //add matches (edge)
    for (auto camera2 : slam->cameras)
    {
      for (auto camera1 : camera2->getGoodLocalCameras())
      {
        if (camera1->id < camera2->id)
        {
          for (auto match : camera2->getEdge(camera1)->matches)
          {
            auto s1 = Point2d(camera1->keypoints[match.queryIdx].x, camera1->keypoints[match.queryIdx].y);
            auto s2 = Point2d(camera2->keypoints[match.trainIdx].x, camera2->keypoints[match.trainIdx].y);
            optimizer->addEdge(new BAEdge(camera1, s1, camera2, s2, Kvertex));
          }
        }
      }
    }
  }

  //updateSolution
  virtual void updateSolution() override
  {
    Vector3d est = Kvertex->estimate();
    slam->calibration.f = est[0];
    slam->calibration.cx = est[1];
    slam->calibration.cy = est[2];

    for (auto camera : slam->cameras)
    {
      auto vertex = (BACameraVertex*)camera->ba.vertex;
      auto w2c = vertex->estimate();
      g2o::SE3Quat e(w2c);
      auto R = Quaternion(e.rotation().w(), e.rotation().x(), e.rotation().y(), e.rotation().z());
      auto t = Point3d(e.translation().x(), e.translation().y(), e.translation().z());
      camera->pose = Pose(R, t);
    }

    slam->refreshQuads();

    auto new_calibration = slam->calibration;
    PrintInfo(" K1" , new_calibration.f, " ", new_calibration.cx, " ", new_calibration.cy, ")");
  }


};


//////////////////////////////////////////////////////////////////////////////////
class OffsetBundleAdjustment : public BundleAdjustment
{
public:

  //___________________________________________________________________
  class BAVertex : public g2o::BaseVertex<2, Vector2d> {
  public:

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    //constructor
    BAVertex() {
    }

    //constructor
    BAVertex(Camera* camera)
    {
      setEstimate(Vector2d(camera->homography(0, 2), camera->homography(1, 2)));
    }

    //not supported
    virtual bool read(std::istream& is)        override { VisusReleaseAssert(false); return true; }
    virtual bool write(std::ostream& os) const override { VisusReleaseAssert(false); return true; }

    //setToOriginImpl
    virtual void setToOriginImpl() override {
      _estimate = Vector2d(0, 0);
    }

    //oplusImpl
    virtual void oplusImpl(const double* update_) override {
      Vector2d update(update_);
      setEstimate(estimate() + update);
    }

  };

  //___________________________________________________________________
  class BAEdge : public g2o::BaseMultiEdge</*error_dim*/4,/*error_vector*/Vector4d>
  {
  public:

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    Point2d s1;
    Point2d s2;

    //constructor
    BAEdge(Camera* camera1, const Point2d& s1_, Camera* camera2, const Point2d& s2_)
      : s1(s1_), s2(s2_)
    {
      resize(2); //num vertices

      setVertex(0, static_cast<BAVertex*>(camera1->ba.vertex));
      setVertex(1, static_cast<BAVertex*>(camera2->ba.vertex));

      setMeasurement(Vector4d(s1.x, s1.y, s2.x, s2.y));
      setInformation(Eigen::Matrix4d::Identity());
      setRobustKernel(new g2o::RobustKernelHuber());
    }

    //not supported
    virtual bool read(std::istream& is)        override { VisusReleaseAssert(false); return true; }
    virtual bool write(std::ostream& os) const override { VisusReleaseAssert(false); return true; }

    //virtual void linearizeOplus() override; NOTE this can be faster If I knew how to compute the Jacobian

    //computeError
    virtual void computeError() override
    {
      Vector2d camera1 = ((BAVertex*)_vertices[0])->estimate();
      Vector2d camera2 = ((BAVertex*)_vertices[1])->estimate();

      auto S1 = s2 + Point2d(camera2[0], camera2[1]) - Point2d(camera1[0], camera1[1]);
      auto S2 = s1 + Point2d(camera1[0], camera1[1]) - Point2d(camera2[0], camera2[1]);

      _error[0] = S1.x - s1.x;
      _error[1] = S1.y - s1.y;
      _error[2] = S2.x - s2.x;
      _error[3] = S2.y - s2.y;
    }
  };

  //constructor
  OffsetBundleAdjustment(Slam* slam_, double gainThreshold_) : BundleAdjustment(slam_, gainThreshold_) {
  }

  //destructor
  virtual ~OffsetBundleAdjustment() {
  }

  //setInitialSolution
  virtual void setInitialSolution() override
  {
    for (auto camera : slam->cameras)
    {
      auto vertex = new BAVertex(camera);
      vertex->setId(VertexId++);
      vertex->setFixed(false);
      optimizer->addVertex(vertex);
      camera->ba.vertex = vertex;
    }

    //add matches (edge)
    for (auto camera2 : slam->cameras)
    {
      for (auto camera1 : camera2->getGoodLocalCameras())
      {
        if (camera1->id < camera2->id)
        {
          for (auto match : camera2->getEdge(camera1)->matches)
          {
            auto s1 = Point2d(camera1->keypoints[match.queryIdx].x, camera1->keypoints[match.queryIdx].y);
            auto s2 = Point2d(camera2->keypoints[match.trainIdx].x, camera2->keypoints[match.trainIdx].y);
            optimizer->addEdge(new BAEdge(camera1, s1, camera2, s2));
          }
        }
      }
    }
  }

  //updateSolution 
  virtual void updateSolution() override
  {
    for (auto camera : slam->cameras)
    {
      auto offset = ((BAVertex*)camera->ba.vertex)->estimate();
      camera->homography = Matrix::translate(Point2d(offset[0], offset[1]));
      camera->quad = Quad(camera->homography, Quad(slam->width, slam->height));
    }
  }
  
};



/////////////////////////////////////////////////////
Slam::Slam() {
}

/////////////////////////////////////////////////////
Slam::~Slam() {
  for (auto it : cameras)
    delete it;
}

/////////////////////////////////////////////////////
void Slam::addCamera(Camera* camera) {
  camera->quad = Quad(this->width, this->height);
  camera->homography = Matrix(3);
  this->cameras.push_back(camera);
}

/////////////////////////////////////////////////////
Camera* Slam::previousCamera(Camera* camera) const
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

/////////////////////////////////////////////////////
Camera* Slam::nextCamera(Camera* camera) const
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

/////////////////////////////////////////////////////
void Slam::removeCamera(Camera* camera2)
{
  for (auto camera1 : camera2->getAllLocalCameras())
    camera2->removeLocalCamera(camera1);
  VisusReleaseAssert(camera2->edges.empty());

  //PrintInfo("removing camera " , camera2->id ," from group ", GroupId);
  Utils::remove(this->cameras, camera2);
  delete camera2;
}

/////////////////////////////////////////////////////
std::vector<std::vector<Camera*>> Slam::findGroups() const
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

/////////////////////////////////////////////////////
void Slam::removeDisconnectedCameras()
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

/////////////////////////////////////////////////////
void Slam::removeCamerasWithTooMuchSkew()
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

/////////////////////////////////////////////////////
Quad Slam::computeWorldQuad(Camera* camera)
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

/////////////////////////////////////////////////////
BoxNd Slam::getQuadsBox() const
{
  auto box = BoxNd::invalid();
  for (auto camera : cameras)
  {
    for (auto point : camera->quad.points)
      box.addPoint(point);
  }
  return box;
}

/////////////////////////////////////////////////////
void Slam::refreshQuads()
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

/////////////////////////////////////////////////////
bool Slam::loadKeyPoints(Camera* camera2, String filename)
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

/////////////////////////////////////////////////////
bool Slam::saveKeyPoints(Camera* camera2, String filename)
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

/////////////////////////////////////////////////////
void Slam::removeOutlierMatches(double max_reproj_error)
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

///////////////////////////////////////////////////////////////////////////////
void Slam::bundleAdjustment(double ba_tolerance, String algorithm)
{
  if (algorithm == "offset")
  {
    OffsetBundleAdjustment(this, ba_tolerance).doBundleAdjustment();
    return;
  }

  DefaultBundleAdjustment(this, ba_tolerance).doBundleAdjustment();
}

/////////////////////////////////////////////////////
void Slam::doPostIterationAction() {
}


 
} //namespace Visus




