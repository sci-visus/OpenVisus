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

#include <Visus/Matrix.h>

namespace Visus {

double Matrix::__identity__[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
double Matrix::__zero__    [16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

//////////////////////////////////////////////////////////////////////
Matrix Matrix::identity() 
  {return Matrix();}

//////////////////////////////////////////////////////////////////////
Matrix Matrix::zero() 
  {return Matrix(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);}

//////////////////////////////////////////////////////////////////////
Matrix Matrix::scaleAndTranslate(Point3d vs,Point3d vt)
{
  return Matrix(vs.x,    0,    0, vt.x,
                    0, vs.y,    0, vt.y,
                    0,    0, vs.z, vt.z,
                    0,    0,    0,   1);
}

//////////////////////////////////////////////////////////////////////
  bool Matrix::isOrthogonal() const
{
  return (((mat[ 0]?1:0) + (mat[ 4]?1:0) + (mat[ 8]?1:0))<=1)
     && (((mat[ 1]?1:0) + (mat[ 5]?1:0) + (mat[ 9]?1:0))<=1)
     && (((mat[ 2]?1:0) + (mat[ 6]?1:0) + (mat[10]?1:0))<=1);
}



//////////////////////////////////////////////////////////////////////
Matrix Matrix::transpose() const
{
  return Matrix 
  (
    mat[0],mat[4],mat[ 8],mat[12],
    mat[1],mat[5],mat[ 9],mat[13],
    mat[2],mat[6],mat[10],mat[14],
    mat[3],mat[7],mat[11],mat[15]
  );
}

//////////////////////////////////////////////////////////////////////
double Matrix::determinant() const
{
  return 
    mat[3] * mat[6] * mat[9]  * mat[12]-mat[2] * mat[7] * mat[9]  * mat[12]-mat[3] * mat[5] * mat[10] * mat[12]+mat[1] * mat[7]    * mat[10] * mat[12]+
    mat[2] * mat[5] * mat[11] * mat[12]-mat[1] * mat[6] * mat[11] * mat[12]-mat[3] * mat[6] * mat[8]  * mat[13]+mat[2] * mat[7]    * mat[8]  * mat[13]+
    mat[3] * mat[4] * mat[10] * mat[13]-mat[0] * mat[7] * mat[10] * mat[13]-mat[2] * mat[4] * mat[11] * mat[13]+mat[0] * mat[6]    * mat[11] * mat[13]+
    mat[3] * mat[5] * mat[8]  * mat[14]-mat[1] * mat[7] * mat[8]  * mat[14]-mat[3] * mat[4] * mat[9]  * mat[14]+mat[0] * mat[7]    * mat[9]  * mat[14]+
    mat[1] * mat[4] * mat[11] * mat[14]-mat[0] * mat[5] * mat[11] * mat[14]-mat[2] * mat[5] * mat[8]  * mat[15]+mat[1] * mat[6]    * mat[8]  * mat[15]+
    mat[2] * mat[4] * mat[9]  * mat[15]-mat[0] * mat[6] * mat[9]  * mat[15]-mat[1] * mat[4] * mat[10] * mat[15]+mat[0] * mat[5]    * mat[10] * mat[15];
}


//////////////////////////////////////////////////////////////////////
Matrix Matrix::invert() const
{
  if (isIdentity())
    return *this;

  double inv[16];
  int i;
  Matrix     _m=this->transpose();
  double*  m=_m.mat;

  inv[ 0] =  m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15]+ m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
  inv[ 4] = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15]- m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
  inv[ 8] =  m[4]*m[ 9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15]+ m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[ 9];
  inv[12] = -m[4]*m[ 9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14]- m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[ 9];
  inv[ 1] = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15]- m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
  inv[ 5] =  m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15]+ m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
  inv[ 9] = -m[0]*m[ 9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15]- m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[ 9];
  inv[13] =  m[0]*m[ 9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14]+ m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[ 9];
  inv[ 2] =  m[1]*m[ 6]*m[15] - m[1]*m[ 7]*m[14] - m[5]*m[2]*m[15]+ m[5]*m[3]*m[14] + m[13]*m[2]*m[ 7] - m[13]*m[3]*m[ 6];
  inv[ 6] = -m[0]*m[ 6]*m[15] + m[0]*m[ 7]*m[14] + m[4]*m[2]*m[15]- m[4]*m[3]*m[14] - m[12]*m[2]*m[ 7] + m[12]*m[3]*m[ 6];
  inv[10] =  m[0]*m[ 5]*m[15] - m[0]*m[ 7]*m[13] - m[4]*m[1]*m[15]+ m[4]*m[3]*m[13] + m[12]*m[1]*m[ 7] - m[12]*m[3]*m[ 5];
  inv[14] = -m[0]*m[ 5]*m[14] + m[0]*m[ 6]*m[13] + m[4]*m[1]*m[14]- m[4]*m[2]*m[13] - m[12]*m[1]*m[ 6] + m[12]*m[2]*m[ 5];
  inv[ 3] = -m[1]*m[ 6]*m[11] + m[1]*m[ 7]*m[10] + m[5]*m[2]*m[11]- m[5]*m[3]*m[10] - m[ 9]*m[2]*m[ 7] + m[ 9]*m[3]*m[ 6];
  inv[ 7] =  m[0]*m[ 6]*m[11] - m[0]*m[ 7]*m[10] - m[4]*m[2]*m[11]+ m[4]*m[3]*m[10] + m[ 8]*m[2]*m[ 7] - m[ 8]*m[3]*m[ 6];
  inv[11] = -m[0]*m[ 5]*m[11] + m[0]*m[ 7]*m[ 9] + m[4]*m[1]*m[11]- m[4]*m[3]*m[ 9] - m[ 8]*m[1]*m[ 7] + m[ 8]*m[3]*m[ 5];
  inv[15] =  m[0]*m[ 5]*m[10] - m[0]*m[ 6]*m[ 9] - m[4]*m[1]*m[10]+ m[4]*m[2]*m[ 9] + m[ 8]*m[1]*m[ 6] - m[ 8]*m[2]*m[ 5];

  double det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
    
  if (!det)
    return Matrix::identity();

  det = 1.0/det;

  Matrix ret;
  for (i = 0; i < 16; i++) 
    ret.mat[i] = inv[i] * det;

  return ret.transpose();
}

//////////////////////////////////////////////////////////////////////
Matrix Matrix::perspective(double fovy, double aspect, double zNear, double zFar)
{
  double m[4][4] ={{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};
  double sine, cotangent, deltaZ;
  double radians = fovy / 2 * Math::Pi / 180;

  deltaZ = zFar - zNear;
  sine = sin(radians);
  if ((deltaZ == 0) || (sine == 0) || (aspect == 0)) 
    return Matrix();

  cotangent = cos(radians) / sine;
  m[0][0] = cotangent / aspect;
  m[1][1] = cotangent;
  m[2][2] = -(zFar + zNear) / deltaZ;
  m[2][3] = -1;
  m[3][2] = -2 * zNear * zFar / deltaZ;
  m[3][3] = 0;

  Matrix ret(&m[0][0]);
  return ret.transpose();
}

//////////////////////////////////////////////////////////////////////
Matrix Matrix::lookAt(Point3d Eye,Point3d Center,Point3d Up)
{
  Point3d forward, side, up;
  double m[4][4] = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};

  forward[0] = Center.x - Eye.x;
  forward[1] = Center.y - Eye.y;
  forward[2] = Center.z - Eye.z;

  up[0] = Up.x;
  up[1] = Up.y;
  up[2] = Up.z;

  forward=forward.normalized();

  /* Side = forward x up */
  side = forward.cross(up);
  side=side.normalized();

  /* Recompute up as: up = side x forward */
  up=side.cross(forward);

  m[0][0] = side[0];
  m[1][0] = side[1];
  m[2][0] = side[2];

  m[0][1] = up[0];
  m[1][1] = up[1];
  m[2][1] = up[2];

  m[0][2] = -forward[0];
  m[1][2] = -forward[1];
  m[2][2] = -forward[2];

  Matrix ret(&m[0][0]);
  return ret.transpose() * translate(-1*Eye);
}

//////////////////////////////////////////////////////////////////////
Matrix Matrix::scale(Point3d s)
{
  return Matrix(  
    s.x,  0,  0, 0,
    0  ,s.y,  0, 0,
    0  ,  0,s.z, 0,
    0  ,  0,  0, 1);
}

//////////////////////////////////////////////////////////////////////
Matrix Matrix::translate(Point3d t)
{
  return Matrix(
    1,0,0,t.x,
    0,1,0,t.y,
    0,0,1,t.z,
    0,0,0,1);
}



//////////////////////////////////////////////////////////////////////
Matrix Matrix::rotate(Point3d axis,double angle)
{
  axis=axis.normalized();
  double x=axis.x;
  double y=axis.y;
  double z=axis.z;

  double c = cos(angle);
  double s = sin(angle);

  return Matrix(
    x*x*(1 - c) +   c,  x*y*(1 - c) - z*s,  x*z*(1 - c) + y*s,  0,
    y*x*(1 - c) + z*s,  y*y*(1 - c) +   c,  y*z*(1 - c) - x*s,  0,
    x*z*(1 - c) - y*s,  y*z*(1 - c) + x*s,  z*z*(1 - c) +   c,  0,
    0,0,0,1);
}

//////////////////////////////////////////////////////////////////////
Matrix Matrix::rotateAroundAxis(Point3d axis,double angle)
{
  return Matrix::rotate(axis,angle);
}


//////////////////////////////////////////////////////////////////////
Matrix Matrix::frustum(double left, double right,double bottom, double top,double nearZ, double farZ)
{
  double m[16];
  #define M(row,col)  m[col*4+row]
    M(0,0) = (2*nearZ) / (right-left); M(0,1) =                        0;  M(0,2) =   (right+left) / (right-left);  M(0,3) = 0;
    M(1,0) =                        0; M(1,1) = (2*nearZ) / (top-bottom);  M(1,2) =   (top+bottom) / (top-bottom);  M(1,3) = 0;
    M(2,0) =                        0; M(2,1) =                        0;  M(2,2) = -(farZ+nearZ) / ( farZ-nearZ);  M(2,3) =  -(2*farZ*nearZ) / (farZ-nearZ);
    M(3,0) =                        0; M(3,1) =                        0;  M(3,2) =                            -1;  M(3,3) = 0;
  #undef M
  Matrix ret(m);
  return ret.transpose();
}

//////////////////////////////////////////////////////////////////////
Matrix Matrix::ortho(double left, double right,double bottom, double top,double nearZ, double farZ)
{
  double m[16];
  #define M(row,col)  m[col*4+row]

    M(0,0) = 2 / (right-left);M(0,1) =                0;M(0,2) =                 0;M(0,3) = -(right+left) / (right-left);
    M(1,0) =                0;M(1,1) = 2 / (top-bottom);M(1,2) =                 0;M(1,3) = -(top+bottom) / (top-bottom);
    M(2,0) =                0;M(2,1) =                0;M(2,2) = -2 / (farZ-nearZ);M(2,3) = -(farZ+nearZ) / (farZ-nearZ);
    M(3,0) =                0;M(3,1) =                0;M(3,2) =                 0;M(3,3) = 1;
    
  #undef M

  Matrix ret(m);
  return ret.transpose();
}


//////////////////////////////////////////////////////////////////////
Matrix Matrix::viewport(int x,int y,int width,int height)
{
  return Matrix(
    width/2.0,            0,  0    ,   x+width /2.0,
            0,   height/2.0,  0    ,   y+height/2.0,
            0,            0,  1/2.0,   0+     1/2.0,
            0,            0,      0,              1
    );
}

  
//////////////////////////////////////////////////////////////////////
void Matrix::getLookAt(Point3d& pos,Point3d& dir,Point3d& vup) const
{
  Matrix T=this->invert();
  const double* vmat=T.mat;
  pos=Point3d(  vmat[3], vmat[7], vmat[11]);
  dir=Point3d( -vmat[2],-vmat[6],-vmat[10]).normalized();
  vup=Point3d(  vmat[1], vmat[5], vmat[ 9]).normalized();
}



//////////////////////////////////////////////////////////
Quaternion Matrix3::toQuaternion() const
{
  const Matrix3& m=*this;

  double kRot[3][3]=
  {
    {m(0,0) , m(0,1) , m(0,2)},
    {m(1,0) , m(1,1) , m(1,2)},
    {m(2,0) , m(2,1) , m(2,2)}
  };

  // Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
  // article "Quaternion Calculus and Fast Animation".
   
  double fTrace = kRot[0][0]+kRot[1][1]+kRot[2][2];
  double fRoot;
   
  double qw, qx, qy, qz;

  if ( fTrace > 0.0 )
  {
    // |w| > 1/2, may as well choose w > 1/2
    fRoot = (double)sqrt(fTrace + 1.0);  // 2w
    qw = 0.5f*fRoot;
    fRoot = 0.5f/fRoot;  // 1/(4w)
    qx = (kRot[2][1]-kRot[1][2])*fRoot;
    qy = (kRot[0][2]-kRot[2][0])*fRoot;
    qz = (kRot[1][0]-kRot[0][1])*fRoot;
  }
  else
  {
    // |w| <= 1/2
    int s_iNext[3] = { 1, 2, 0 };
    int i = 0;
    if ( kRot[1][1] > kRot[0][0] )
      i = 1;
    if ( kRot[2][2] > kRot[i][i] )
      i = 2;
    int j = s_iNext[i];
    int k = s_iNext[j];
   
    fRoot = (double)sqrt(kRot[i][i]-kRot[j][j]-kRot[k][k] + 1.0);
    double* apkQuat[3] = { &qx, &qy, &qz };
    *apkQuat[i] = 0.5f * fRoot;
    fRoot = (double)0.5f/fRoot;
    qw = (kRot[k][j]-kRot[j][k])*fRoot;
    *apkQuat[j] = (kRot[j][i]+kRot[i][j])*fRoot;
    *apkQuat[k] = (kRot[k][i]+kRot[i][k])*fRoot;
  }

  return Quaternion(qw, qx, qy, qz);
}


//////////////////////////////////////////////////////////
Quaternion Matrix::toQuaternion() const
{
  return dropW().toQuaternion();
}


//////////////////////////////////////////////////////////
Matrix3 Matrix3::rotate(const Quaternion& q)
{
  double kRot[3][3];
  double fTx  = 2.0f*q.x;double fTy  = 2.0f*q.y;double fTz  = 2.0f*q.z;
  double fTwx = fTx*q.w ;double fTwy = fTy*q.w ;double fTwz = fTz*q.w ;
  double fTxx = fTx*q.x ;double fTxy = fTy*q.x ;double fTxz = fTz*q.x ;
  double fTyy = fTy*q.y ;double fTyz = fTz*q.y ;double fTzz = fTz*q.z ;

  kRot[0][0] = 1.0f-(fTyy+fTzz);kRot[0][1] =      (fTxy-fTwz);kRot[0][2] =      (fTxz+fTwy);
  kRot[1][0] =      (fTxy+fTwz);kRot[1][1] = 1.0f-(fTxx+fTzz);kRot[1][2] =      (fTyz-fTwx);
  kRot[2][0] =      (fTxz-fTwy);kRot[2][1] =      (fTyz+fTwx);kRot[2][2] = 1.0f-(fTxx+fTyy);

  return Matrix3(
    kRot[0][0],kRot[0][1],kRot[0][2],
    kRot[1][0],kRot[1][1],kRot[1][2],
    kRot[2][0],kRot[2][1],kRot[2][2]);
}



//////////////////////////////////////////////////////////
Matrix Matrix::rotate(const Quaternion& q)
{
  return Matrix(Matrix3::rotate(q),Point3d(0,0,0));
}


//////////////////////////////////////////////////////////////////////
QDUMatrixDecomposition::QDUMatrixDecomposition(const Matrix& T) 
{
  double m[3][3]=
  {
    {T(0,0) , T(0,1) , T(0,2)},
    {T(1,0) , T(1,1) , T(1,2)},
    {T(2,0) , T(2,1) , T(2,2)}
  };

  double kQ[3][3];

  double fInvLength = m[0][0]*m[0][0] + m[1][0]*m[1][0] + m[2][0]*m[2][0];
  if (!Utils::doubleAlmostEquals(fInvLength,0)) fInvLength = 1.0/sqrt(fInvLength);

  kQ[0][0] = m[0][0]*fInvLength;
  kQ[1][0] = m[1][0]*fInvLength;
  kQ[2][0] = m[2][0]*fInvLength;

  double fDot = kQ[0][0]*m[0][1] + kQ[1][0]*m[1][1] + kQ[2][0]*m[2][1];
  kQ[0][1] = m[0][1]-fDot*kQ[0][0];
  kQ[1][1] = m[1][1]-fDot*kQ[1][0];
  kQ[2][1] = m[2][1]-fDot*kQ[2][0];
  fInvLength = kQ[0][1]*kQ[0][1] + kQ[1][1]*kQ[1][1] + kQ[2][1]*kQ[2][1];
  if (!Utils::doubleAlmostEquals(fInvLength,0)) fInvLength = 1.0/sqrt(fInvLength);
        
  kQ[0][1] *= fInvLength;
  kQ[1][1] *= fInvLength;
  kQ[2][1] *= fInvLength;

  fDot = kQ[0][0]*m[0][2] + kQ[1][0]*m[1][2] + kQ[2][0]*m[2][2];
  kQ[0][2] = m[0][2]-fDot*kQ[0][0];
  kQ[1][2] = m[1][2]-fDot*kQ[1][0];
  kQ[2][2] = m[2][2]-fDot*kQ[2][0];

  fDot = kQ[0][1]*m[0][2] + kQ[1][1]*m[1][2] + kQ[2][1]*m[2][2];
  kQ[0][2] -= fDot*kQ[0][1];
  kQ[1][2] -= fDot*kQ[1][1];
  kQ[2][2] -= fDot*kQ[2][1];

  fInvLength = kQ[0][2]*kQ[0][2] + kQ[1][2]*kQ[1][2] + kQ[2][2]*kQ[2][2];
  if (!Utils::doubleAlmostEquals(fInvLength,0)) fInvLength = 1.0/sqrt(fInvLength);

  kQ[0][2] *= fInvLength;
  kQ[1][2] *= fInvLength;
  kQ[2][2] *= fInvLength;

  double fDet = 
      kQ[0][0]*kQ[1][1]*kQ[2][2] + kQ[0][1]*kQ[1][2]*kQ[2][0] +
      kQ[0][2]*kQ[1][0]*kQ[2][1] - kQ[0][2]*kQ[1][1]*kQ[2][0] -
      kQ[0][1]*kQ[1][0]*kQ[2][2] - kQ[0][0]*kQ[1][2]*kQ[2][1];

  if ( fDet < 0.0 )
  {
    for (size_t iRow = 0; iRow < 3; iRow++)
    for (size_t iCol = 0; iCol < 3; iCol++)
        kQ[iRow][iCol] = -kQ[iRow][iCol];
  }

  double kR[3][3];
  kR[0][0] = kQ[0][0]*m[0][0] + kQ[1][0]*m[1][0] +kQ[2][0]*m[2][0];
  kR[0][1] = kQ[0][0]*m[0][1] + kQ[1][0]*m[1][1] +kQ[2][0]*m[2][1];
  kR[1][1] = kQ[0][1]*m[0][1] + kQ[1][1]*m[1][1] +kQ[2][1]*m[2][1];
  kR[0][2] = kQ[0][0]*m[0][2] + kQ[1][0]*m[1][2] +kQ[2][0]*m[2][2];
  kR[1][2] = kQ[0][1]*m[0][2] + kQ[1][1]*m[1][2] +kQ[2][1]*m[2][2];
  kR[2][2] = kQ[0][2]*m[0][2] + kQ[1][2]*m[1][2] +kQ[2][2]*m[2][2];

  this->D[0] = kR[0][0];
  this->D[1] = kR[1][1];
  this->D[2] = kR[2][2];

  double fInvD0 = 1.0f/this->D[0];
  this->U[0] = kR[0][1]*fInvD0;
  this->U[1] = kR[0][2]*fInvD0;
  this->U[2] = kR[1][2]/this->D[1];

  this->Q=Matrix();
  this->Q(0,0)=kQ[0][0];this->Q(0,1)=kQ[0][1];this->Q(0,2)=kQ[0][2];
  this->Q(1,0)=kQ[1][0];this->Q(1,1)=kQ[1][1];this->Q(1,2)=kQ[1][2];
  this->Q(2,0)=kQ[2][0];this->Q(2,1)=kQ[2][1];this->Q(2,2)=kQ[2][2];
}

//////////////////////////////////////////////////////////////////////
TRSMatrixDecomposition::TRSMatrixDecomposition(const Matrix& T)  
{
  QDUMatrixDecomposition qdu(T);
  this->translate   = Point3d(T(0,3),T(1,3),T(2,3));
  this->rotate      = qdu.Q.toQuaternion();
  this->scale       = qdu.D;
}


//////////////////////////////////////////////////////////////////////
MatrixNd MatrixNd::invert() const
{
  auto A = MatrixNd(*this);
  auto S = PointNd(dim);
  auto B = MatrixNd(dim);
  auto X = std::vector<int>(dim);
  auto ret = MatrixNd(A);

  for (int I = 0; I < dim; I++)
  {
    X[I] = I;
    double scalemax = 0.0;
    for (int J = 0; J < dim; J++)
      scalemax = std::max(scalemax, fabs(A(I, J)));
    S[I] = scalemax;
  }

  int signDet = 1;
  for (int K = 0; K < dim; K++)
  {
    double ratiomax = 0.0;
    int jPivot = K;
    for (int I = K; I < dim; I++)
    {
      double ratio = fabs(A(X[I], K)) / S[X[I]];
      if (ratio > ratiomax) {
        jPivot = I;
        ratiomax = ratio;
      }
    }
    int indexJ = X[K];
    if (jPivot != K)
    {
      indexJ = X[jPivot];
      X[jPivot] = X[K];
      X[K] = indexJ;
      signDet *= -1;
    }

    for (int I = (K + 1); I < dim; I++)
    {
      double coeff = A(X[I], K) / A(indexJ, K);
      for (int J = (K + 1); J < dim; J++)
      {
        double value = A(X[I], J) - coeff * A(indexJ, J);
        A(X[I], J) = value;
      }

      A(X[I], K) = coeff;
      for (int J = 0; J < dim; J++)
      {
        double value = B(X[I], J) - A(X[I], K) * B(indexJ, J);
        B(X[I], J) = value;
      }
    }
  }

  for (int K = 0; K < dim; K++)
  {
    double value = B(X[dim], K) / A(X[dim], dim);
    ret(dim, K) = value;
    for (int I = A.dim - 1; I >= 0; I--)
    {
      double sum = B(X[I], K);
      for (int J = (I + 1); J < dim; J++)
        sum -= A(X[I], J) * ret(J, K);
      value = sum / A(X[I], I);
      ret(I, K) = value;
    }
  }
  return ret;
}

} //namespace Visus

