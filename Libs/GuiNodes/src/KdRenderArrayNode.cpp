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

#include <Visus/KdRenderArrayNode.h>
#include <Visus/GLObjects.h>

namespace Visus {

VISUS_IMPLEMENT_SHADER_CLASS(VISUS_GUI_NODES_API, KdRenderArrayNodeShader)

/////////////////////////////////////////////////////////////////////////
static bool isNodeFillingVisibleSpace(const SharedPtr<KdArray>& kdarray,KdArrayNode* node) 
{
  VisusAssert(node);

  if (!kdarray->isNodeVisible(node) || node->displaydata)
    return true;

  return node->left  && isNodeFillingVisibleSpace(kdarray,node->left .get()) && 
          node->right && isNodeFillingVisibleSpace(kdarray,node->right.get());
}

/////////////////////////////////////////////////////////////////////////
static bool isNodeVisible(const SharedPtr<KdArray>& kdarray,KdArrayNode* node) 
{
  VisusAssert(node);

  if (!node->displaydata || !kdarray->isNodeVisible(node))
    return false;

  //don't allow any overlapping (i.e. if of any of my parents is currently displayed, I cannot show the current node)
  if (kdarray->getDataDim()==3)
  {
    for (KdArrayNode* up=node->up;up;up=up->up)
      if (up->bDisplay) return false;
  }

  if (node->left  && isNodeFillingVisibleSpace(kdarray,node->left .get()) && 
      node->right && isNodeFillingVisibleSpace(kdarray,node->right.get()))
  {
    return false; // I can just display the childs
  }
  else
  {
    return true; //display the current node
  }
}


/////////////////////////////////////////////////////////
bool KdRenderArrayNode::processInput()
{
  //assign the input anyway, even if wrong
  this->kdarray  = readValue<KdArray>("data");
  auto  palette  = readValue<Palette>("palette");

  this->palette.reset();
  this->palette_texture.reset();

  if (!kdarray)
    return false;

  {
    ScopedReadLock rlock(kdarray->lock); 

    auto dtype=kdarray->root->displaydata.dtype;
    Point4d vs(1,1,1,1);
    Point4d vt(0,0,0,0);

    if (palette && dtype.ncomponents()==1)
    {
      this->palette=palette;
      this->palette_texture=std::make_shared<GLTexture>(palette->convertToArray());

      if (!dtype.isVectorOf(DTypes::UINT8))
      {
        auto input_range=palette? palette->input_range : ComputeRange();
        for (int I=0;I<std::min(4,dtype.ncomponents());I++)
        {
          //NOTE if I had to compute the dynamic range I will use only root data
          auto vs_t=input_range.doCompute(kdarray->root->displaydata,I).getScaleTranslate();
          vs[I]=vs_t.first;
          vt[I]=vs_t.second; 
        }
      }
    }

    //create the kd textures
    std::stack< SharedPtr<KdArrayNode> > stack;
    stack.push(kdarray->root);
    while (!stack.empty())
    {
      SharedPtr<KdArrayNode> node=stack.top(); stack.pop();
      node->bDisplay=isNodeVisible(kdarray,node.get());

      if (node->bDisplay)
      {
        //need to write lock here
        if (!node->user_value)
        {
          auto texture=std::make_shared<GLTexture>(node->displaydata);
          texture->vs=vs;
          texture->vt=vt;
          {
            //ScopedWriteLock wlock(rlock); Don't NEED wlock since I'm the only one to use the texture variable
            node->user_value = texture;
          }
        }
      }
      else if (node->user_value)
      {
        //ScopedWriteLock wlock(rlock); Don't NEED wlock since I'm the only one to use the texture variable
        node->user_value.reset();
      }

      if (node->right) stack.push(node->right );
      if (node->left ) stack.push(node->left  );
    }
  }

  return true;
}



/////////////////////////////////////////////////////////
void KdRenderArrayNode::glRender(GLCanvas& gl)
{
  if (!kdarray) 
    return;

  //need the read lock here
  ScopedReadLock rlock(kdarray->lock); 

  auto  dtype     = kdarray->root->displaydata.dtype;
  bool  bHasAlpha = dtype==DTypes::UINT8_RGBA;
  bool  bBlend    = palette_texture || bHasAlpha;

  KdRenderArrayNodeShader::Config config;
  config.texture_dim                 = kdarray->getDataDim();
  config.texture_nchannels           = kdarray->root->displaydata.dtype.ncomponents();
  config.palette_enabled             = palette_texture?true:false;
  config.clippingbox_enabled         = kdarray->logic_clipping.valid() || gl.hasClippingBox();
  config.discard_if_zero_alpha       = bBlend? true : false;
  KdRenderArrayNodeShader* shader=KdRenderArrayNodeShader::getSingleton(config);
  gl.setShader(shader);

  if (kdarray->logic_clipping.valid())
    gl.pushClippingBox(kdarray->logic_clipping);

  gl.pushModelview();

  gl.pushBlend(bBlend);
  gl.pushDepthTest(config.texture_dim==2?false:true);
  gl.pushDepthMask(config.texture_dim==3?false:true);
    
  if (config.palette_enabled)
    shader->setPaletteTexture(gl,this->palette_texture);

  //decide how to render (along X Y or Z)
  Point3d viewpos,viewdir,viewup;
  gl.getModelview().getLookAt(viewpos,viewdir,viewup);
  viewdir=viewdir*(-1); //need to go back to front (i.e. the opposite of viewdir)

  //decide how to render (along X Y or Z)
  int Z=0;
  if (fabs(viewdir[1])>fabs(viewdir[Z])) Z=1;
  if (fabs(viewdir[2])>fabs(viewdir[Z])) Z=2;
  int X=(Z+1)%3;
  int Y=(Z+2)%3;
  if (X>Y) std::swap(X,Y);

  //reset the visit
  std::vector< SharedPtr<KdArrayNode> > rendered;

  std::stack< SharedPtr<KdArrayNode> > stack;
  stack.push(kdarray->root);
  while (!stack.empty())
  {
    SharedPtr<KdArrayNode> node=stack.top(); stack.pop();

    if (node->bDisplay && node->user_value)
    {
      auto   box        = node->logic_box.castTo<BoxNd>();
      Array displaydata = node->displaydata; 

      shader->setTexture(gl,std::dynamic_pointer_cast<GLTexture>(node->user_value));

      //2d
      if (config.texture_dim==2)
      {
        gl.glRenderMesh(GLMesh::Quad(box.p1.toPoint2(),box.p2.toPoint2(),false,true));
      }
      //3d 
      else 
      {
        gl.pushModelview();
        gl.multModelview(Matrix::translate(box.p1) * Matrix::scale(box.size()));
        gl.glRenderMesh(GLMesh::AxisAlignedUnitVolume(X,Y,Z,viewdir,(int)displaydata.dims[Z]));
        gl.popModelview();
      }

      rendered.push_back(node);

    }
    else
    {
      node->user_value.reset();
    }

    if (!node->isLeaf())
    {
      bool bSwapOrder=config.texture_dim==3 && viewpos[node->split_bit]<node->getMiddle();
      stack.push(bSwapOrder? node->left  : node->right);
      stack.push(bSwapOrder? node->right : node->left );
    }
  }

  if (ApplicationInfo::debug && config.texture_dim == 2)
  {
    for (auto node : rendered)
      GLLineLoop(node->logic_box.getPoints(), Colors::Black, 3).glRender(gl);
  }

  gl.popDepthMask();
  gl.popDepthTest();
  gl.popBlend();
  gl.popModelview();

  if (kdarray->logic_clipping.valid())
    gl.popClippingBox();
}

} //namespace Visus

