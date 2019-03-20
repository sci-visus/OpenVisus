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

#ifndef VISUS_KD_ARRAY_H
#define VISUS_KD_ARRAY_H

#include <Visus/Kernel.h>
#include <Visus/Array.h>

namespace Visus {

////////////////////////////////////////////////////////////
class VISUS_KERNEL_API KdArrayNode
{
public:

  VISUS_NON_COPYABLE_CLASS(KdArrayNode)

#if !SWIG
  class VISUS_KERNEL_API UserValue
  {
  public:
    UserValue() {}
    virtual ~UserValue() {}
  };
#endif

  // box
  NdBox box;    

  //level
  int level=0;

  // resolution
  int resolution=0;             

  // id (root has id 1)
  BigInt id=0;

  //if it has childs, what is the split bit
  int split_bit=-1;

  //up
  KdArrayNode* up=nullptr;

  //childs
  SharedPtr<KdArrayNode> left,right;

  //buffers
  Array                fullres;
  Array                displaydata;
  Array                blockdata;
  bool                 bDisplay=false;
  SharedPtr<UserValue> user_value;

  //default constructor
  KdArrayNode() {
  }

  //destructor
  ~KdArrayNode() {
  }

  //return the middle
  inline NdPoint::coord_t getMiddle() {
    return (this->box.p1[split_bit]+this->box.p2[split_bit])>>1;
  }

  //if leaf or not
  inline bool isLeaf() const
  {
    //must have both childs or no childs
    VisusAssert((this->left && this->right) || (!this->left && !this->right));
    return !this->left && !this->right;
  }

  //c_size (estimation of buffers occupancy)
  inline Int64 c_size() const
  {
    return 
      blockdata.c_size()
      + ((fullres && fullres.heap!=blockdata.heap) ? fullres.c_size() : 0)
      + ((displaydata && displaydata.heap!=blockdata.heap && displaydata.heap!=fullres.heap) ? displaydata.c_size() : 0);
  }


};


//////////////////////////////////////////////
class VISUS_KERNEL_API KdArray 
{
public:

  VISUS_NON_COPYABLE_CLASS(KdArray)

  //read/write lock
#if !SWIG
  RWLock lock;  
#endif

  // (dataset) box 
  NdBox box;  

  //only for kd queries
  SharedPtr<KdArrayNode> root;

  //clipping
  Position clipping;

  //current queried box
  NdBox query_box;

  //current estimated end resolution
  int end_resolution;

  //constructor
  KdArray(int datadim=0);

  //destructor
  virtual ~KdArray();

  //getDataDim
  inline int getDataDim() const {
    return datadim;
  }

  //clearChilds
  void clearChilds(KdArrayNode* node);

  //split
  void split(KdArrayNode* node,int split_bit);

  //isNodeVisible
  inline bool isNodeVisible(KdArrayNode* node) const {
    return node->box.strictIntersect(this->box) && node->box.strictIntersect(this->query_box);
  }

  //enableCaching
  void enableCaching(int cutoff,Int64 fine_maxmemory);

private:

  //in case you need some caching
  class SingleCache;
  class MultiCache;
  SharedPtr<MultiCache> cache;
  int datadim;

  void onNodeEnter(KdArrayNode*);
  void onNodeExit (KdArrayNode*);


};

} //namespace Visus

#endif //VISUS_KD_ARRAY_H

