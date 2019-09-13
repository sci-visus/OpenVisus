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

#include <Visus/IsoContourRenderNode.h>

namespace Visus {

/////////////////////////////////////////////////////////////////////////////////////////////////////////
class IsoContourShader : public GLShader
{
public:

  VISUS_NON_COPYABLE_CLASS(IsoContourShader)

  VISUS_DECLARE_SHADER_CLASS(VISUS_GUI_NODES_API, IsoContourShader)

  //________________________________________________
  class Config
  {
  public:

    bool palette_enabled;
    bool vertex_color_index_enabled;

    //constructor
    Config() : palette_enabled(false), vertex_color_index_enabled(false)
    {}

    //valid
    bool valid() const
    {
      return true;
    }

    //getId
    int getId() const
    {
      VisusAssert(valid());
      int ret = 0, shift = 0;
      ret |= (palette_enabled ? 1 : 0) << shift++;
      ret |= (vertex_color_index_enabled ? 1 : 0) << shift++;
      return ret;
    }
  };

  Config config;

  //constructor
  IsoContourShader(const Config& config_) :
    GLShader(":/IsoContourShader.glsl"),
    config(config_)
  {
    addDefine("PALETTE_ENABLED", cstring(config.palette_enabled ? 1 : 0));
    addDefine("VERTEX_COLOR_INDEX_ENABLED", cstring(config.vertex_color_index_enabled ? 1 : 0));

    u_sampler = addSampler("u_sampler");
    u_palette_sampler = addSampler("u_palette_sampler");
  }

  //destructor
  virtual ~IsoContourShader()
  {}

  //getSingleton
  static IsoContourShader* getSingleton(const Config& config)
  {
    return Shaders::getSingleton()->get(config.getId(), config);
  }

  //setTexture
  void setTexture(GLCanvas& gl, SharedPtr<GLTexture> value) {
    gl.setTexture(u_sampler, value);
  }

  //setPaletteTexture 
  //(NOTE: the shader will use the 'g'/'[1]' component of field in case texture has ncomponents>=2. if it has only one that it will use 'r')
  void setPaletteTexture(GLCanvas& gl, SharedPtr<GLTexture> value)
  {
    VisusAssert(config.palette_enabled);
    gl.setTextureInSlot(1, u_palette_sampler, value);
  }

private:

  GLSampler u_sampler;
  GLSampler u_palette_sampler;

};

VISUS_IMPLEMENT_SHADER_CLASS(VISUS_GUI_NODES_API, IsoContourShader)

void IsoContourRenderNode::allocShaders()
{
  IsoContourShader::Shaders::allocSingleton();
}

void IsoContourRenderNode::releaseShaders()
{
  IsoContourShader::Shaders::allocSingleton();
}

///////////////////////////////////////////////////////////////////////////
IsoContourRenderNode::IsoContourRenderNode(String name) : Node(name) 
{
  addInputPort("data");
  addInputPort("palette"); //if provided, can color the vertices by (say) height

  this->material=GLMaterial::createRandom();
}

///////////////////////////////////////////////////////////////////////////
IsoContourRenderNode::~IsoContourRenderNode() {
}

///////////////////////////////////////////////////////////////////////////
bool IsoContourRenderNode::processInput()
{
  auto isocontour  = readValue<IsoContour>("data");
  auto palette     = readValue<Palette>("palette");

  bool bEnablePalette=palette && isocontour && isocontour->field.array.dtype.ncomponents()>=2;
  if (!bEnablePalette)
    palette.reset();

  this->palette.reset();
  this->palette_texture.reset();
  this->isocontour.reset();
  
  if (!isocontour)
    return false;

  this->isocontour=isocontour;

  if (palette)
  {
    this->palette=palette;
    this->palette_texture=std::make_shared<GLTexture>(palette->convertToArray());
  }
  return true;
}

/////////////////////////////////////////////////////////////
void IsoContourRenderNode::glRender(GLCanvas& gl)
{
  if (!isocontour || !isocontour->field.texture) 
    return;

  //gpu normals (first component for computing normals, second component for applying palette)
  //I can calculate the normals on GPU, eventually if the incoming 
  //field has 2 components I can use the second component to shop on top of the surface

  gl.pushModelview();
  gl.multModelview(isocontour->bounds.getTransformation());
  Point3d pos,dir,vup;
  gl.getModelview().getLookAt(pos,dir,vup);

  IsoContourShader::Config shader_config;
  shader_config.palette_enabled=this->palette_texture?true:false;

  IsoContourShader* shader=IsoContourShader::getSingleton(shader_config);
  gl.setShader(shader);
  gl.setUniformMaterial(*shader,material);
  gl.setUniformLight(*shader,Point4d(pos,1.0));

  if (palette_texture)
    shader->setPaletteTexture(gl,palette_texture);

  //this is to compute normals on gpu
  shader->setTexture(gl, isocontour->field.texture);

  gl.glRenderMesh(*isocontour);
  gl.popModelview();
}

/////////////////////////////////////////////////////////////
void IsoContourRenderNode::writeToObjectStream(ObjectStream& ostream)
{
  Node::writeToObjectStream(ostream);

  ostream.pushContext("material");
  material.writeToObjectStream(ostream);
  ostream.popContext("material");
}

/////////////////////////////////////////////////////////////
void IsoContourRenderNode::readFromObjectStream(ObjectStream& istream)
{
  Node::readFromObjectStream(istream);

  istream.pushContext("material");
  material.readFromObjectStream(istream);
  istream.popContext("material");
}


} //namespace Visus
