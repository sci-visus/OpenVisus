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

  /////////////////////////////////////////////////////////////////////////////
class KdRenderArrayNodeShaderConfig
{
public:
  int  texture_dim = 0;
  int  texture_nchannels = 0; //(1) luminance (2) luminance+alpha (3) rgb (4) rgba
  bool clippingbox_enabled = false;
  bool palette_enabled = false;
  bool discard_if_zero_alpha = false;

  //constructor
  KdRenderArrayNodeShaderConfig() {
  }

  //valid
  bool valid() const {
    return (texture_dim == 2 || texture_dim == 3) && (texture_nchannels >= 1 && texture_nchannels <= 4);
  }

  //operator<
  bool operator<(const KdRenderArrayNodeShaderConfig& other) const {
    return this->key() < other.key();
  }

private:

  //key
  std::tuple<int, int, bool, bool, bool> key() const {
    VisusAssert(valid());
    return std::make_tuple(texture_dim, texture_nchannels, clippingbox_enabled, palette_enabled, discard_if_zero_alpha);
  }

};


/////////////////////////////////////////////////////////////////////////////
class KdRenderArrayNodeShader : public GLShader
{
public:

  VISUS_NON_COPYABLE_CLASS(KdRenderArrayNodeShader)

  typedef KdRenderArrayNodeShaderConfig Config;

  static std::map<Config, KdRenderArrayNodeShader*> shaders;

  Config config;

  //constructor
  KdRenderArrayNodeShader(const Config& config_) : GLShader(":/KdRenderArrayShader.glsl"), config(config_)
  {
    addDefine("CLIPPINGBOX_ENABLED", cstring(config.clippingbox_enabled ? 1 : 0));
    addDefine("TEXTURE_DIM", cstring(config.texture_dim));
    addDefine("TEXTURE_NCHANNELS", cstring(config.texture_nchannels));
    addDefine("PALETTE_ENABLED", cstring(config.palette_enabled ? 1 : 0));
    addDefine("DISCARD_IF_ZERO_ALPHA", cstring(config.discard_if_zero_alpha ? 1 : 0));

    u_sampler = addSampler("u_sampler");
    u_palette_sampler = addSampler("u_palette_sampler");
  }

  //destructor
  virtual ~KdRenderArrayNodeShader() {
  }

  //getSingleton
  static KdRenderArrayNodeShader* getSingleton(Config config) {
    auto it = shaders.find(config);
    if (it != shaders.end()) return it->second;
    auto ret = new KdRenderArrayNodeShader(config);
    shaders[config] = ret;
    return ret;
  }

  //setTexture
  void setTexture(GLCanvas& gl, SharedPtr<GLTexture> value) {
    gl.setTexture(u_sampler, value);
  }

  //setPaletteTexture 
  void setPaletteTexture(GLCanvas& gl, SharedPtr<GLTexture> value) {
    VisusAssert(config.palette_enabled);
    gl.setTextureInSlot(1, u_palette_sampler, value);
  }

private:

  GLSampler u_sampler;
  GLSampler u_palette_sampler;
};


std::map< KdRenderArrayNodeShader::Config, KdRenderArrayNodeShader*> KdRenderArrayNodeShader::shaders;



void KdRenderArrayNode::allocShaders()
{
}

void KdRenderArrayNode::releaseShaders()
{
  for (auto it : KdRenderArrayNodeShader::shaders)
    delete it.second;
  KdRenderArrayNodeShader::shaders.clear();
}

/////////////////////////////////////////////////////////////////////////
static bool isNodeFillingVisibleSpace(const SharedPtr<KdArray>& kdarray,KdArrayNode* node) 
{
  VisusAssert(node);

  if (!kdarray->isNodeVisible(node) || node->displaydata.valid())
    return true;

  return node->left  && isNodeFillingVisibleSpace(kdarray,node->left .get()) && 
          node->right && isNodeFillingVisibleSpace(kdarray,node->right.get());
}

/////////////////////////////////////////////////////////////////////////
static bool isNodeVisible(const SharedPtr<KdArray>& kdarray,KdArrayNode* node) 
{
  VisusAssert(node);

  if (!node->displaydata.valid() || !kdarray->isNodeVisible(node))
    return false;

  //don't allow any overlapping (i.e. if of any of my parents is currently displayed, I cannot show the current node)
  if (kdarray->getPointDim()==3)
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
  this->kdarray  = readValue<KdArray>("kdarray");
  auto  palette = readValue<Palette>("palette");

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
      this->palette_texture=std::make_shared<GLTexture>(palette->toArray());

      if (!dtype.isVectorOf(DTypes::UINT8))
      {
        for (int C=0;C<std::min(4,dtype.ncomponents());C++)
        {
          //NOTE if I had to compute the dynamic range I will use only root data
          auto input_range = palette ? 
            palette->computeRange(kdarray->root->displaydata, C) : 
            ArrayUtils::computeRange(kdarray->root->displaydata, C);

          auto vs_t = input_range.getScaleTranslate();
          vs[C] = vs_t.first;
          vt[C] = vs_t.second;
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
        if (!node->texture)
        {
          auto texture=std::make_shared<GLTexture>(node->displaydata);
          texture->vs=vs;
          texture->vt=vt;
          {
            //ScopedWriteLock wlock(rlock); Don't NEED wlock since I'm the only one to use the texture variable
            node->texture = texture;
          }
        }
      }
      else if (node->texture)
      {
        //ScopedWriteLock wlock(rlock); Don't NEED wlock since I'm the only one to use the texture variable
        node->texture.reset();
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
  config.texture_dim                 = kdarray->getPointDim();
  config.texture_nchannels           = kdarray->root->displaydata.dtype.ncomponents();
  config.palette_enabled             = palette_texture?true:false;
  config.clippingbox_enabled         = kdarray->clipping.valid() || gl.hasClippingBox();
  config.discard_if_zero_alpha       = bBlend? true : false;
  auto shader=KdRenderArrayNodeShader::getSingleton(config);
  gl.setShader(shader);

  auto logic_to_physic = Position::computeTransformation(kdarray->bounds, kdarray->logic_box);

  //clipping is in physic coordinates
  if (kdarray->clipping.valid())
    gl.pushClippingBox(kdarray->clipping);

  //instead from now on I'm displaying stuff in logic coordinates
  gl.pushModelview();
  gl.multModelview(logic_to_physic);

  gl.pushBlend(bBlend);
  gl.pushDepthTest(config.texture_dim==2?false:true);
  gl.pushDepthMask(config.texture_dim==3?false:true);
    
  if (config.palette_enabled)
    shader->setPaletteTexture(gl,this->palette_texture);

  //decide how to render (along X Y or Z)
  Point3d viewpos,center,viewup;
  gl.getModelview().getLookAt(viewpos, center,viewup, 1.0);

  //need to go back to front (i.e. the opposite of viewdir)
  auto viewdir = (center - viewpos).normalized();

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

    if (node->bDisplay && node->texture)
    {
      auto   box        = node->logic_box.castTo<BoxNd>();
      Array displaydata = node->displaydata; 

      shader->setTexture(gl,std::dynamic_pointer_cast<GLTexture>(node->texture));

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
      node->texture.reset();
    }

    if (!node->isLeaf())
    {
      bool bSwapOrder=config.texture_dim==3 && viewpos[node->split_bit]<node->getLogicMiddle();
      stack.push(bSwapOrder? node->left  : node->right);
      stack.push(bSwapOrder? node->right : node->left );
    }
  }

#if _DEBUG
  if (config.texture_dim == 2)
  {
    for (auto node : rendered)
      GLLineLoop(node->logic_box.castTo<BoxNd>().getPoints(), Colors::Black, 3).glRender(gl);
  }
#endif

  gl.popDepthMask();
  gl.popDepthTest();
  gl.popBlend();
  gl.popModelview();

  if (kdarray->clipping.valid())
    gl.popClippingBox();
}

void KdRenderArrayNode::createEditor()
{
  //todo
}



} //namespace Visus

