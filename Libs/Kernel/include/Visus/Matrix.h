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

#ifndef VISUS_MATRIX_H
#define VISUS_MATRIX_H

#include <Visus/Kernel.h>
#include <Visus/Quaternion.h>
#include <Visus/Plane.h>
#include <Visus/LinearMap.h>

#include <cstring>
#include <iomanip>

namespace Visus {

//////////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API Matrix3
{
public:

  VISUS_CLASS(Matrix3)

  double mat[9];

  // constructor
  Matrix3()
  {
    mat[0] = (double)1; mat[1] = (double)0; mat[2] = (double)0;
    mat[3] = (double)0; mat[4] = (double)1; mat[5] = (double)0;
    mat[6] = (double)0; mat[7] = (double)0; mat[8] = (double)1;  
  }

  //copy constructor
  Matrix3(const Matrix3& other) {
    this->mat[0] = other.mat[0]; this->mat[1] = other.mat[1]; this->mat[2] = other.mat[2];
    this->mat[3] = other.mat[3]; this->mat[4] = other.mat[4]; this->mat[5] = other.mat[5];
    this->mat[6] = other.mat[6]; this->mat[7] = other.mat[7]; this->mat[8] = other.mat[8];
  }

  //constructor
  Matrix3(double m1, double m2, double m3, 
          double m4, double m5, double m6, 
          double m7, double m8, double m9)
  {
    mat[0] = m1; mat[1] = m2; mat[2] = m3;
    mat[3] = m4; mat[4] = m5; mat[5] = m6;
    mat[6] = m7; mat[7] = m8; mat[8] = m9;
  }

  //constructor
  Matrix3(const double other[9]) {
    this->mat[0] = other[0]; this->mat[1] = other[1]; this->mat[2] = other[2];
    this->mat[3] = other[3]; this->mat[4] = other[4]; this->mat[5] = other[5];
    this->mat[6] = other[6]; this->mat[7] = other[7]; this->mat[8] = other[8];
  }

  //constructor
  Matrix3(Point3d c0,Point3d c1,Point3d c2) {
    mat[0] = c0.x; mat[1] = c1.x; mat[2] = c2.x;
    mat[3] = c0.y; mat[4] = c1.y; mat[5] = c2.y;
    mat[6] = c0.z; mat[7] = c1.z; mat[8] = c2.z;
  }
  
  //construct from string
  inline explicit Matrix3(String value)
  {
    memset(mat,0,sizeof(mat));mat[0]=mat[4]=mat[8]=1;
    if (!value.empty())
    {
      std::istringstream parser(value);
      for (int i=0;i<9;i++) parser>>this->mat[i];
    }
  }

  //destructor
  ~Matrix3() {
  }

  //operator=
  Matrix3& operator=(const Matrix3& other) {
    this->mat[0] = other.mat[0]; this->mat[1] = other.mat[1]; this->mat[2] = other.mat[2];
    this->mat[3] = other.mat[3]; this->mat[4] = other.mat[4]; this->mat[5] = other.mat[5];
    this->mat[6] = other.mat[6]; this->mat[7] = other.mat[7]; this->mat[8] = other.mat[8];
    return *this;
  }

  //toString
  inline String toString() const 
  {
    std::ostringstream out;
    for (int i=0;i<9;i++)  {if (i) out<<" ";out<<mat[i];}
    return out.str();
  }

  //operator[]
  inline const double& operator[](int idx) const
  {return mat[idx];}

  inline double& operator[](int idx) 
  {return mat[idx];}

  //operator==
  const bool operator==(const Matrix3& other) const {
    return memcmp(mat,other.mat,sizeof(mat))==0;
  }

  //operator!=
  const bool operator!=(const Matrix3& other) const {
    return !operator==(other);
  }

  //identity
  static Matrix3 identity() 
  {return Matrix3();}

  //zero
  static Matrix3 zero() 
  {return Matrix3(0,0,0,0,0,0,0,0,0);}

  // operator()
  double operator()(int i, int j) const 
  {VisusAssert(i>=0 && i<3 && j>=0 && j<3);return mat[i * 3 + j];}

  //operator()
#if !SWIG
  double& operator()(int i, int j) 
  {VisusAssert(i>=0 && i<3 && j>=0 && j<3);return mat[i * 3 + j];}
#endif

  // operator-
  Matrix3 operator-() const
  {
    return Matrix3(-mat[0] , -mat[1] , -mat[2],
                   -mat[3] , -mat[4] , -mat[5],
                   -mat[6] , -mat[7] , -mat[8]);
  }

  //operator+
  Matrix3 operator+(const Matrix3& other) const
  {
    return Matrix3(mat[0]+other.mat[0] , mat[1]+other.mat[1] , mat[2]+other.mat[2],
                   mat[3]+other.mat[3] , mat[4]+other.mat[4] , mat[5]+other.mat[5],
                   mat[6]+other.mat[6] , mat[7]+other.mat[7] , mat[8]+other.mat[8]);
  }

  //operator-
  Matrix3 operator-=(const Matrix3& other) const
  {
    return Matrix3(mat[0]-other.mat[0] , mat[1]-other.mat[1] , mat[2]-other.mat[2],
                   mat[3]-other.mat[3] , mat[4]-other.mat[4] , mat[5]-other.mat[5],
                   mat[6]-other.mat[6] , mat[7]-other.mat[7] , mat[8]-other.mat[8]);
  }

  // operator: multiplication
  Matrix3 operator*(double value) const
  {
    return Matrix3(value*mat[0] , value*mat[1] , value*mat[2],
                   value*mat[3] , value*mat[4] , value*mat[5],
                   value*mat[6] , value*mat[7] , value*mat[8]);
  }

  // operator: multiplication
  Point3d operator*(Point3d p) const{
    return p.x*col(0)+p.y*col(1)+p.z*col(2);
  }

  //operator*=
  Matrix3 operator*(const Matrix3& b) const
  {
    return Matrix3(
      mat[0]*b.mat[0]+mat[1]*b.mat[3]+mat[2]*b.mat[6] , mat[0]*b.mat[1]+mat[1]*b.mat[4]+mat[2]*b.mat[7] , mat[0]*b.mat[2]+mat[1]*b.mat[5]+mat[2]*b.mat[8],
      mat[3]*b.mat[0]+mat[4]*b.mat[3]+mat[5]*b.mat[6] , mat[3]*b.mat[1]+mat[4]*b.mat[4]+mat[5]*b.mat[7] , mat[3]*b.mat[2]+mat[4]*b.mat[5]+mat[5]*b.mat[8],
      mat[6]*b.mat[0]+mat[7]*b.mat[3]+mat[8]*b.mat[6] , mat[6]*b.mat[1]+mat[7]*b.mat[4]+mat[8]*b.mat[7] , mat[6]*b.mat[2]+mat[7]*b.mat[5]+mat[8]*b.mat[8]);
  }

  //transpose
  Matrix3 transpose() const
  {
    return Matrix3(mat[0] , mat[3] , mat[6],
                   mat[1] , mat[4] , mat[7],
                   mat[2] , mat[5] , mat[8]);
  }

  //determinant
  double determinant() const
  {
    return mat[0] * (mat[4] * mat[8] - mat[5] * mat[7]) -
           mat[1] * (mat[3] * mat[8] - mat[5] * mat[6]) +
           mat[2] * (mat[3] * mat[7] - mat[4] * mat[6]);
  }

  //invert
  Matrix3 invert() const
  {
    const Matrix3& m=*this;
    Matrix3 ret(m(1,1)*m(2,2)-m(1,2)*m(2,1) , m(0,2)*m(2,1)-m(0,1)*m(2,2) , m(0,1)*m(1,2)-m(0,2)*m(1,1),
                m(1,2)*m(2,0)-m(1,0)*m(2,2) , m(0,0)*m(2,2)-m(0,2)*m(2,0) , m(0,2)*m(1,0)-m(0,0)*m(1,2),
                m(1,0)*m(2,1)-m(1,1)*m(2,0) , m(0,1)*m(2,0)-m(0,0)*m(2,1) , m(0,0)*m(1,1)-m(0,1)*m(1,0));
    double det = m(0,0)*ret(0,0) + m(0,1)*ret(1,0)+ m(0,2)*ret(2,0);
    return det? ret*(1.0f/det) : Matrix3::identity();
  }

  // row
  Point3d row(int i) const
  {VisusAssert(i>=0 && i<3);return Point3d(mat[i*3],mat[i*3+1],mat[i*3+2]);}

  //col
  Point3d col(int j) const
  {VisusAssert(j>=0 && j<3);return Point3d(mat[j],mat[j+3], mat[j+6]);}

  //translate
  static Matrix3 translate(Point2d vt) {
    return Matrix3(
      1,0,vt.x,
      0,1,vt.y,
      0,0,   1);
  }

  //translate
  static Matrix3 scale(Point2d vs) {
    return Matrix3(
      vs.x,   0,0,
         0,vs.y,0,
         0,   0,1);
  }

  //translate
  static Matrix3 scale(double vs) {
    return scale(Point2d(vs, vs));
  }

  //rotate from quaternion
  static Matrix3 rotate(const Quaternion4d& q);

  //toQuaternion
  Quaternion4d toQuaternion() const;

  //scaleAroundCenter
  static Matrix3 scaleAroundCenter(Point2d center,double vs) {
    return  Matrix3::translate(center) * scale(Point2d(vs,vs)) * Matrix3::translate(-center);
  }

};

inline Matrix3 operator*(double s,const Matrix3& T){
  return T*s;
}

inline Point3d operator*(Point3d p,const Matrix3& T){
  return p.x*T.row(0)+p.y*T.row(1)+p.z*T.row(2);
}


//////////////////////////////////////////////////////////
class VISUS_KERNEL_API Matrix4 
{
public:

  VISUS_CLASS(Matrix4)

  //row major mode
  double mat[16];

  //default constructor
  inline Matrix4() {
    mat[ 0] = 1.0; mat[ 1] = 0.0; mat[ 2] = 0.0; mat[ 3] = 0.0;
    mat[ 4] = 0.0; mat[ 5] = 1.0; mat[ 6] = 0.0; mat[ 7] = 0.0;
    mat[ 8] = 0.0; mat[ 9] = 0.0; mat[10] = 1.0; mat[11] = 0.0;
    mat[12] = 0.0; mat[13] = 0.0; mat[14] = 0.0; mat[15] = 1.0;
  }
  
  //copy constructor
  inline Matrix4(const Matrix4& src) {
    mat[ 0] = src.mat[ 0]; mat[ 1] = src.mat[ 1]; mat[ 2] = src.mat[ 2]; mat[ 3] = src.mat[ 3];
    mat[ 4] = src.mat[ 4]; mat[ 5] = src.mat[ 5]; mat[ 6] = src.mat[ 6]; mat[ 7] = src.mat[ 7];
    mat[ 8] = src.mat[ 8]; mat[ 9] = src.mat[ 9]; mat[10] = src.mat[10]; mat[11] = src.mat[11];
    mat[12] = src.mat[12]; mat[13] = src.mat[13]; mat[14] = src.mat[14]; mat[15] = src.mat[15];
  }

  //construct from string
  inline explicit Matrix4(String value)
  {
    memset(mat,0,sizeof(mat));mat[0]=mat[5]=mat[10]=mat[15]=1;
    if (!value.empty())
    {
      std::istringstream parser(value);
      for (int i=0;i<16;i++) parser>>this->mat[i];
    }
  }

  //constructor for 16 doubles (row major!)
  inline explicit  Matrix4(double a00,double a01,double a02,double a03,
                           double a10,double a11,double a12,double a13,
                           double a20,double a21,double a22,double a23,
                           double a30,double a31,double a32,double a33)
  {
    double mat[16]={
      a00,a01,a02,a03,
      a10,a11,a12,a13,
      a20,a21,a22,a23,
      a30,a31,a32,a33
    };
    memcpy(this->mat,mat,sizeof(double)*16);
  }

  //constructor
  inline explicit Matrix4(double mat[16]) {
    memcpy(this->mat,mat,sizeof(double)*16);
  }

  //constructor
  inline explicit Matrix4(float mat[16]) {
    for (int I=0;I<16;I++) 
      this->mat[I]=mat[I];
  }

  //constructor: transform the default axis to the system X,Y,Z in P
  inline explicit Matrix4(Point3d X,Point3d Y,Point3d Z,Point3d P)
  {
    mat[ 0]=X.x ; mat[ 1]=Y.x ; mat[ 2]=Z.x ; mat[ 3]=P.x;
    mat[ 4]=X.y ; mat[ 5]=Y.y ; mat[ 6]=Z.y ; mat[ 7]=P.y;
    mat[ 8]=X.z ; mat[ 9]=Y.z ; mat[10]=Z.z ; mat[11]=P.z;
    mat[12]=  0 ; mat[13]=  0 ; mat[14]=  0 ; mat[15]=  1;
  }

  //Matrix4
  inline explicit Matrix4(const Matrix3& R,const Point3d& t) {
    mat[ 0]=R(0,0); mat[ 1]=R(0,1); mat[ 2]=R(0,2); mat[ 3]=t.x;
    mat[ 4]=R(1,0); mat[ 5]=R(1,1); mat[ 6]=R(1,2); mat[ 7]=t.y;
    mat[ 8]=R(2,0); mat[ 9]=R(2,1); mat[10]=R(2,2); mat[11]=t.z;
    mat[12]=     0; mat[13]=     0; mat[14]=     0; mat[15]=  1;
  }

  //constructor
  inline explicit Matrix4(const Matrix3& m3) : Matrix4(
    m3.mat[0] , m3.mat[1] , 0 , m3.mat[2],
    m3.mat[3] , m3.mat[4] , 0 , m3.mat[5],
            0 ,         0 , 1 ,         0,
    m3.mat[6] , m3.mat[7] , 0 , m3.mat[8]) {
  }

  //dropW
  Matrix3 dropW() const 
  {
    return Matrix3(mat[ 0], mat[ 1], mat[ 2],
                   mat[ 4], mat[ 5], mat[ 6],
                   mat[ 8], mat[ 9], mat[10]);
  }

  //dropZ
  Matrix3 dropZ() const
  {
    return Matrix3(
      mat[0], mat[1], mat[3],
      mat[4], mat[5], mat[7],
      mat[12], mat[13], mat[15]);
  }

  //getRow
  inline Point4d getRow(int R) const
  {return Point4d((*this)(R,0),(*this)(R,1),(*this)(R,2),(*this)(R,3));}

  //setRow
  inline void setRow(int R,Point4d value) 
  {(*this)(R,0)=value[0];(*this)(R,1)=value[1];(*this)(R,2)=value[2];(*this)(R,3)=value[3];}

  //getColumn
  inline Point4d getColumn(int C) const
  {return Point4d((*this)(0,C),(*this)(1,C),(*this)(2,C),(*this)(3,C));}

  //setRow
  inline void setColumn(int C,Point4d value) 
  {(*this)(0,C)=value[0];(*this)(1,C)=value[1];(*this)(2,C)=value[2];(*this)(3,C)=value[3];}

  //test equality
  inline bool operator==(const Matrix4& src) const
  {return  memcmp(this->mat,src.mat,sizeof(mat))==0;}

  inline bool operator!=(const Matrix4& src) const
  {return  memcmp(this->mat,src.mat,sizeof(mat))!=0;}

  //test validity
  inline bool valid() const
  {
    for (int I=0;I<16;I++) 
      if (!Utils::isValidNumber(mat[I])) return false;
    
    return determinant()!=0.0? true : false;
  }

  //operator[]
  inline const double& operator[](int idx) const
  {return mat[idx];}

  inline double& operator[](int idx) 
  {return mat[idx];}

  // operator-
  Matrix4 operator-() const
  {
    return Matrix4(-mat[ 0] , -mat[ 1], -mat[ 2], -mat[ 3],
                   -mat[ 4] , -mat[ 5], -mat[ 6], -mat[ 7],
                   -mat[ 8] , -mat[ 9], -mat[10], -mat[11],
                   -mat[12] , -mat[13], -mat[14], -mat[15]);
  }

  // operator: multiplication
  Matrix4 operator*(double value) const
  {
    return Matrix4(value*mat[ 0] , value*mat[ 1], value*mat[ 2], value*mat[ 3],
                   value*mat[ 4] , value*mat[ 5], value*mat[ 6], value*mat[ 7],
                   value*mat[ 8] , value*mat[ 9], value*mat[10], value*mat[11],
                   value*mat[12] , value*mat[13], value*mat[14], value*mat[15]);
  }

  //operator*
  inline Matrix4 operator*(const Matrix4& b) const
  {
    if (    b.isIdentity()) return *this;
    if (this->isIdentity()) return b;

    const double* amat=this->mat;
    const double* bmat=b    .mat;
    return Matrix4
    (
      amat[ 0]*bmat[ 0]+amat[ 1]*bmat[ 4]+amat[ 2]*bmat[ 8]+amat[ 3]*bmat[12],
      amat[ 0]*bmat[ 1]+amat[ 1]*bmat[ 5]+amat[ 2]*bmat[ 9]+amat[ 3]*bmat[13],
      amat[ 0]*bmat[ 2]+amat[ 1]*bmat[ 6]+amat[ 2]*bmat[10]+amat[ 3]*bmat[14],
      amat[ 0]*bmat[ 3]+amat[ 1]*bmat[ 7]+amat[ 2]*bmat[11]+amat[ 3]*bmat[15], 
      amat[ 4]*bmat[ 0]+amat[ 5]*bmat[ 4]+amat[ 6]*bmat[ 8]+amat[ 7]*bmat[12], 
      amat[ 4]*bmat[ 1]+amat[ 5]*bmat[ 5]+amat[ 6]*bmat[ 9]+amat[ 7]*bmat[13], 
      amat[ 4]*bmat[ 2]+amat[ 5]*bmat[ 6]+amat[ 6]*bmat[10]+amat[ 7]*bmat[14], 
      amat[ 4]*bmat[ 3]+amat[ 5]*bmat[ 7]+amat[ 6]*bmat[11]+amat[ 7]*bmat[15], 
      amat[ 8]*bmat[ 0]+amat[ 9]*bmat[ 4]+amat[10]*bmat[ 8]+amat[11]*bmat[12], 
      amat[ 8]*bmat[ 1]+amat[ 9]*bmat[ 5]+amat[10]*bmat[ 9]+amat[11]*bmat[13], 
      amat[ 8]*bmat[ 2]+amat[ 9]*bmat[ 6]+amat[10]*bmat[10]+amat[11]*bmat[14], 
      amat[ 8]*bmat[ 3]+amat[ 9]*bmat[ 7]+amat[10]*bmat[11]+amat[11]*bmat[15], 
      amat[12]*bmat[ 0]+amat[13]*bmat[ 4]+amat[14]*bmat[ 8]+amat[15]*bmat[12], 
      amat[12]*bmat[ 1]+amat[13]*bmat[ 5]+amat[14]*bmat[ 9]+amat[15]*bmat[13], 
      amat[12]*bmat[ 2]+amat[13]*bmat[ 6]+amat[14]*bmat[10]+amat[15]*bmat[14], 
      amat[12]*bmat[ 3]+amat[13]*bmat[ 7]+amat[14]*bmat[11]+amat[15]*bmat[15]
    );
  }

  //operator*=
  inline Matrix4& operator*=(const Matrix4& b)
  {
    (*this)=(*this)*b;
    return *this;
  }

  //access using two indices (in the range [0,3])
  inline double& operator()(int row,int col)
  {VisusAssert(row>=0 && row<4 && col>=0 && col<4);return this->mat[row*4+col];}

  //access using two indices (in the range [0,3])
  #if !SWIG
  inline const double& operator()(int row,int col) const
    {VisusAssert(row>=0 && row<4 && col>=0 && col<4);return this->mat[row*4+col];}
  #endif

  //toString
  String toString() const 
  {
    std::ostringstream out;
    for (int i=0;i<16;i++)  {if (i) out<<" ";out<<mat[i];}
    return out.str();
  }

  //toStringWithPrecision
  String toStringWithPrecision(int precision=2)
  {
    std::ostringstream o;
    for (int R=0;R<4;R++)
    {
      for (int C=0;C<4;C++) 
        o<< std::setprecision(precision)<<std::fixed<<(*this)(R,C)<<" ";
      o<<"\n"; 
    }
    return o.str();
  }


  //test orthogonal
  bool isOrthogonal() const;

  //transpose
  Matrix4 transpose() const;

  //determinant
  double determinant() const;

  //invert
  Matrix4 invert() const;

  //convert to look-at which is a triple (pos,dir,vup)
  void getLookAt(Point3d& pos,Point3d& dir,Point3d& vup) const;

  //static identity
  static Matrix4 identity();

  //isIdentity
  inline bool isIdentity() const
  {return memcmp(this->mat,__identity__,sizeof(__identity__))==0;}

  //isZero
  inline bool isZero() const
  {return memcmp(this->mat,__zero__,sizeof(__zero__))==0;}

  //static zero
  static Matrix4 zero();

  //static perspective
  static Matrix4 perspective(double fovy, double aspect, double zNear, double zFar);

  //static lookAt
  static Matrix4 lookAt(Point3d eye,Point3d center,Point3d up);

  //static scale
  static Matrix4 scale(Point3d vs);

  //static translate
  static Matrix4 translate(Point3d vt);

  //translate
  inline static Matrix4 translate(int axis,double offset)
  {Point3d vt;vt[axis]=offset;return Matrix4::translate(vt);}

  //scaleAndTranslate (equivalent to =Matrix4::tranlate(vt)*Matrix4::scale(vs) so scaling is applied first)
  static Matrix4 scaleAndTranslate(Point3d vs,Point3d vt);

  //static rotate
  static Matrix4 rotate(Point3d axis,double angle);

  //rotate from quaternion
  static Matrix4 rotate(const Quaternion4d& q);

  //rotateAroundCenter
  static Matrix4 rotateAroundAxis(Point3d axis,double angle);

  //rotateAroundCenter
  static Matrix4 rotateAroundCenter(Point3d center,Point3d axis,double angle) {
    return Matrix4::translate(center) * rotateAroundAxis(axis,angle) * Matrix4::translate(-center);
  }

  //scaleAroundCenter (see http://www.gamedev.net/topic/399598-scaling-about-an-arbitrary-axis/)
  static Matrix4 scaleAroundAxis(Point3d axis,double k)
  {
    return 
      Matrix4(1+(k-1)*axis.x*axis.x,   (k-1)*axis.x*axis.y,   (k-1)*axis.x*axis.z, 0,
                (k-1)*axis.x*axis.y, 1+(k-1)*axis.y*axis.y,   (k-1)*axis.y*axis.z, 0,
                (k-1)*axis.x*axis.z,   (k-1)*axis.y*axis.z, 1+(k-1)*axis.z*axis.z, 0,
                                  0,                     0,                     0, 1);
  }

  //scaleAroundCenter (see http://www.gamedev.net/topic/399598-scaling-about-an-arbitrary-axis/)
  static Matrix4 scaleAroundCenter(Point3d center,Point3d axis,double k) {
    return  Matrix4::translate(center) * scaleAroundAxis(axis,k) * Matrix4::translate(-center);
  }

  //static frustum
  static  Matrix4 frustum(double left, double right,double bottom, double top,double nearZ, double farZ);

  //static ortho
  static Matrix4 ortho(double left, double right,double bottom, double top,double nearZ, double farZ);

  //static viewport 
  static Matrix4 viewport(int x,int y,int width,int height);

  //toQuaternion
  Quaternion4d toQuaternion() const;

  //swapColums
  Matrix4 swapColums(int C1,int C2) const
  {
    Matrix4 ret=*this;
    for (int R=0;R<4;R++)
      std::swap(ret(R,C1),ret(R,C2));
    return ret;
  }

  //embed (XY into a slice perpendicular to axis with certain offset)
  static Matrix4 embed(int axis,double offset)
  {
    VisusAssert(axis>=0 && axis<3);
    Matrix4 ret;
    const Point3d X(1,0,0);
    const Point3d Y(0,1,0);
    const Point3d Z(0,0,1);
    const Point3d W(0,0,0);
    if      (axis==0) return Matrix4::translate(axis,offset) * Matrix4(Y,Z,X,W);
    else if (axis==1) return Matrix4::translate(axis,offset) * Matrix4(X,Z,Y,W);
    else              return Matrix4::translate(axis,offset) * Matrix4(X,Y,Z,W);
  }

  //interpolate
  static inline Matrix4 interpolate(const double alpha,const Matrix4& T1,const double beta,const Matrix4& T2)
  {Matrix4 ret;for (int I=0;I<16;I++) ret.mat[I]=alpha*T1.mat[I]+beta*T2.mat[I];return ret;}

  //writeToObjectStream
  void writeToObjectStream(ObjectStream& ostream) 
  {
    ostream.write("matrix",this->toString());
  }

  //writeToObjectStream
  void readFromObjectStream(ObjectStream& istream) 
  {
    Matrix4 T(istream.read("matrix"));
    (*this)=T;
  }

private:

  static double __identity__[16];
  static double __zero__    [16];

}; //end class Matrix4

typedef Matrix4 Matrix;

//////////////////////////////////////////////////////////////////////
#if !SWIG

inline Matrix4 operator*(double s,const Matrix4& T){
  return T*s;
}

inline Point3d operator*(const Matrix4& T,const Point3i& p)
{
  double X=(T.mat[ 0]*(p.x)+T.mat[ 1]*(p.y)+T.mat[ 2]*(p.z)+ T.mat[ 3]*(1));
  double Y=(T.mat[ 4]*(p.x)+T.mat[ 5]*(p.y)+T.mat[ 6]*(p.z)+ T.mat[ 7]*(1));
  double Z=(T.mat[ 8]*(p.x)+T.mat[ 9]*(p.y)+T.mat[10]*(p.z)+ T.mat[11]*(1));
  double W=(T.mat[12]*(p.x)+T.mat[13]*(p.y)+T.mat[14]*(p.z)+ T.mat[15]*(1));
  if (!W) W=1; //infinite point....
  return Point3d(X/W,Y/W,Z/W);
}


inline Point3d operator*(const Matrix4& T,const Point3d& p)
{
  double X=(T.mat[ 0]*(p.x)+T.mat[ 1]*(p.y)+T.mat[ 2]*(p.z)+ T.mat[ 3]*(1));
  double Y=(T.mat[ 4]*(p.x)+T.mat[ 5]*(p.y)+T.mat[ 6]*(p.z)+ T.mat[ 7]*(1));
  double Z=(T.mat[ 8]*(p.x)+T.mat[ 9]*(p.y)+T.mat[10]*(p.z)+ T.mat[11]*(1));
  double W=(T.mat[12]*(p.x)+T.mat[13]*(p.y)+T.mat[14]*(p.z)+ T.mat[15]*(1));
  if (!W) W=1; //infinite point....
  return Point3d(X/W,Y/W,Z/W);
}

inline Point4d operator*(const Matrix4& T,const Point4d& v) 
{
  double X=(T.mat[ 0]*(v.x)+T.mat[ 1]*(v.y)+T.mat[ 2]*(v.z)+ T.mat[ 3]*(v.w));
  double Y=(T.mat[ 4]*(v.x)+T.mat[ 5]*(v.y)+T.mat[ 6]*(v.z)+ T.mat[ 7]*(v.w));
  double Z=(T.mat[ 8]*(v.x)+T.mat[ 9]*(v.y)+T.mat[10]*(v.z)+ T.mat[11]*(v.w));
  double W=(T.mat[12]*(v.x)+T.mat[13]*(v.y)+T.mat[14]*(v.z)+ T.mat[15]*(v.w));
  return Point4d(X,Y,Z,W);
}

inline Point4d operator*(const Point4d& v,const Matrix& T)
{
  const double* mat=T.mat;
  double X=(v.x)*mat[ 0] + (v.y)*mat[ 4] + (v.z)*mat[ 8] + (v.w) * mat[12];
  double Y=(v.x)*mat[ 1] + (v.y)*mat[ 5] + (v.z)*mat[ 9] + (v.w) * mat[13];
  double Z=(v.x)*mat[ 2] + (v.y)*mat[ 6] + (v.z)*mat[10] + (v.w) * mat[14];
  double W=(v.x)*mat[ 3] + (v.y)*mat [7] + (v.z)*mat[11] + (v.w) * mat[15];
  return Point4d(X,Y,Z,W);
}

inline Point3f operator*(const Matrix& T,const Point3f& vf)
  {Point3d vd=T*Point3d(vf.x,vf.y,vf.z);return Point3f((float)vd.x,(float)vd.y,(float)vd.z);}

//NOte if you have a T matrix point'=T*point, to transform planes you need to do: plane'=plane*T.invert()
inline Plane operator*(const Plane& h,const Matrix& Ti) 
{
  const double* mat=Ti.mat;
  double X=(h.x)*mat[ 0] + (h.y)*mat[ 4] + (h.z)*mat[ 8] + (h.w) * mat[12];
  double Y=(h.x)*mat[ 1] + (h.y)*mat[ 5] + (h.z)*mat[ 9] + (h.w) * mat[13];
  double Z=(h.x)*mat[ 2] + (h.y)*mat[ 6] + (h.z)*mat[10] + (h.w) * mat[14];
  double W=(h.x)*mat[ 3] + (h.y)*mat [7] + (h.z)*mat[11] + (h.w) * mat[15];
  return Plane(X,Y,Z,W);
}


#endif  //SWIG



//////////////////////////////////////////////////////////
class VISUS_KERNEL_API  QDUMatrixDecomposition
{
public:

  VISUS_CLASS(QDUMatrixDecomposition)

  Matrix  Q; //rotation 
  Point3d D; //scaling
  Point3d U; //shear

  //constructor
  QDUMatrixDecomposition(const Matrix& T);

};


//////////////////////////////////////////////////////////
class VISUS_KERNEL_API TRSMatrixDecomposition
{
public:

  VISUS_CLASS(TRSMatrixDecomposition)

  //equivalent to T(translate) * R(rotate)  * S(scale) 
  Point3d      translate;
  Quaternion4d rotate;
  Point3d      scale;

  //constructor
  TRSMatrixDecomposition() : rotate(Point3d(0,0,1),0),scale(1,1,1)
  {}

  //constructor 
  TRSMatrixDecomposition(const Matrix& T);
 
  //toMatrix
  Matrix toMatrix()
  {
    return 
        Matrix::translate(translate)  
      * Matrix::rotate(rotate) 
      * Matrix::scale(scale) ;
  }


};


//////////////////////////////////////////////////////////////////
class VISUS_KERNEL_API MatrixMap : public LinearMap
{
public:

  VISUS_CLASS(MatrixMap)

  Matrix T,Ti;

  //constructor
  MatrixMap() 
  {}

  //constructor
  MatrixMap(const Matrix& T_) : T(T_)
  {Ti=T.invert();}

  //constructor
  MatrixMap(const Matrix& T_,const Matrix& Ti_) : T(T_),Ti(Ti_)
  {}

  //applyDirectMap
  virtual Point4d applyDirectMap(const Point4d& p) const
  {return T*p;}

  //applyInverseMap
  virtual Point4d applyInverseMap(const Point4d& p) const
  {return Ti*p;}

  //applyDirectMap
  virtual Plane applyDirectMap(const Plane& h) const
  {return h*Ti;}

  //applyDirectMap
  virtual Plane applyInverseMap(const Plane& h) const
  {return h*T;}

};


} //namespace Visus

#endif //VISUS_MATRIX_H

