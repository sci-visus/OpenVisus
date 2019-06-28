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

#include <Visus/RenderArrayNode.h>
#include <Visus/GLCanvas.h>
#include <Visus/VisusConfig.h>

namespace Visus {

VISUS_IMPLEMENT_SHADER_CLASS(VISUS_GUI_NODES_API, RenderArrayNodeShader)

/////////////////////////////////////////////////////////////////////////////////////////////////////////
RenderArrayNode::RenderArrayNode(String name) : Node(name)
{
  addInputPort("data");
  addInputPort("palette");

  lighting_material.front.ambient=Colors::Black;
  lighting_material.front.diffuse=Colors::White;
  lighting_material.front.specular=Colors::White;
  lighting_material.front.shininess=100;

  lighting_material.back.ambient=Colors::Black;
  lighting_material.back.diffuse=Colors::White;
  lighting_material.back.specular=Colors::White;
  lighting_material.back.shininess=100;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
RenderArrayNode::~RenderArrayNode()
{}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RenderArrayNode::processInput()
{
  Time t1=Time::now();

  //I want to sign the input return receipt only after the rendering
  auto return_receipt = createPassThroughtReceipt();
  auto palette        = readValue<Palette>("palette");
  auto data           = readValue<Array>("data");

  //so far I can apply the transfer function on the GPU only if the data is atomic
  bool bPaletteEnabled = paletteEnabled() || (palette && data && data->dtype.ncomponents()==1);
  if (!bPaletteEnabled) 
    palette.reset();

  //request to flush all
  if (!data || !data->dims.innerProduct() || !data->dtype.valid())
  {
    this->return_receipt.reset();
    this->data=Array();
    this->data_texture.reset();
    this->palette.reset();
    this->palette_texture.reset();
    return false;
  }

  this->return_receipt=return_receipt;

  bool bGotNewData=(data->heap!=this->data.heap);
  if (bGotNewData)
  {
    //compact dimension (example: 1 128 256 ->128 256 1)
    if (data->getDepth()>1 && (data->getWidth()==1 || data->getHeight()==1))
    {
      this->data=Array(NdPoint(std::max(data->getWidth(), data->getHeight()), data->getDepth()),data->dtype,data->heap);
      this->data.shareProperties(*data);
    }
    else
    {
      this->data=*data;
    }
  }

  std::vector< std::pair<double,double> > vs_t(4,std::make_pair(1.0,0.0));

  //TODO: this range stuff can be slow and blocking...
  if (!data->dtype.isVectorOf(DTypes::UINT8))
  {
    int ncomponents=data->dtype.ncomponents();
    if (palette)
    {
      for (int C=0;C<std::min(4,ncomponents);C++)
        vs_t[C]=palette->input_range.doCompute(*data,C).getScaleTranslate();
    }
    else
    {
      //create a default input normalization
      ComputeRange input_range;
      vs_t[0] = ncomponents>=1? input_range.doCompute(*data,0).getScaleTranslate() : std::make_pair(1.0,0.0);
      vs_t[1] = ncomponents>=3? input_range.doCompute(*data,1).getScaleTranslate() : vs_t[0];
      vs_t[2] = ncomponents>=3? input_range.doCompute(*data,2).getScaleTranslate() : vs_t[0];
      vs_t[3] = ncomponents>=4? input_range.doCompute(*data,3).getScaleTranslate() : vs_t[3];
    }
  }

  if (!this->data_texture || bGotNewData)
    this->data_texture=std::make_shared<GLTexture>(this->data);

  this->data_texture->minfilter=this->minifyFilter();
  this->data_texture->magfilter=this->magnifyFilter();

  this->data_texture->vs=Point4d(vs_t[0].first ,vs_t[1].first ,vs_t[2].first ,vs_t[3].first );
  this->data_texture->vt=Point4d(vs_t[0].second,vs_t[1].second,vs_t[2].second,vs_t[3].second);

  this->palette=palette;

  if (palette)
    this->palette_texture=std::make_shared<GLTexture>(palette->convertToArray());

  Int64 tot_samples = (Int64)this->data_texture->dims[0] * (Int64)this->data_texture->dims[1] * (Int64)this->data_texture->dims[2];

  VisusInfo()<<"got array"
    <<" texture_dims("<<this->data_texture->dims.toString()<< ")"
    <<" texture_dims("<<this->data_texture->dtype.toString()<<")"
    <<" texture_dims("<<StringUtils::getStringFromByteSize(this->data_texture->dtype.getByteSize(tot_samples))<<")"
    <<" bGotNewData("<<bGotNewData<<")"
    <<" msec("<<t1.elapsedMsec()<<")";

  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderArrayNode::glRender(GLCanvas& gl) 
{
  if (!data_texture) 
    return;

  SharedPtr<ReturnReceipt> return_receipt=this->return_receipt;
  this->return_receipt.reset();

  //need to upload a new palette?
  bool b3D          = data.getDepth()>1;

  //if you want to see what's going on...
  #if 0
  {
    static int cont=0;
    std::ostringstream out;
    out<<"temp."<<cont++<<".png";
    ArrayUtils::saveImageUINT8(out.str(),Array::createView(*data,compact_dims,data->dtype));
  }
  #endif

  RenderArrayNodeShader::Config config;
  config.texture_dim                    = b3D?3:2;
  config.texture_nchannels              = data.dtype.ncomponents();
  config.palette_enabled                = palette_texture?true:false;
  config.lighting_enabled               = lightingEnabled();
  config.clippingbox_enabled            = gl.hasClippingBox() || (data.clipping.valid() || useViewDirection());
  config.discard_if_zero_alpha          = true;

  RenderArrayNodeShader* shader=RenderArrayNodeShader::getSingleton(config);
  gl.setShader(shader);

  if (shader->config.lighting_enabled)
  {
    gl.setUniformMaterial(*shader,this->lighting_material);

    Point3d pos,dir,vup;
    gl.getModelview().getLookAt(pos,dir,vup);
    gl.setUniformLight(*shader,Point4d(pos,1.0));
  }

  if (data.clipping.valid())
    gl.pushClippingBox(data.clipping);

  //in 3d I render the box (0,0,0)x(1,1,1)
  //in 2d I render the box (0,0)  x(1,1)
  gl.pushModelview();
  {
    auto box=data.bounds.box.toBox3();

    Point3d vt=box.p1.toPoint3d();
    Point3d vs=box.size().toPoint3();
    if (!vs[0]) vs[0]=1.0;
    if (!vs[1]) vs[1]=1.0;
    if (!vs[2]) vs[2]=1.0;
    gl.multModelview(data.bounds.T);
    gl.multModelview(Matrix::translate(vt));
    gl.multModelview(Matrix::scale(vs));
    if (shader->config.texture_dim==2)
    {
      if (box.p1[0]==box.p2[0]) 
        gl.multModelview(Matrix(Point3d(0,1,0),Point3d(0,0,1),Point3d(1,0,0),Point3d(0,0,0)));

      else if (box.p1[1]==box.p2[1]) 
        gl.multModelview(Matrix(Point3d(1,0,0),Point3d(0,0,1),Point3d(0,1,0),Point3d(0,0,0)));
    }
  }

  gl.pushBlend(true);
  gl.pushDepthMask(shader->config.texture_dim==3?false:true);

  //note: the order seeems to be important, first bind GL_TEXTURE1 then GL_TEXTURE0
  if (shader->config.palette_enabled && palette_texture)
    shader->setPaletteTexture(gl, palette_texture);

  shader->setTexture(gl, data_texture);

  if (b3D)
  {
    double opacity = this->opacity;
    
    int nslices = this->maxNumSlices();
    if (nslices <= 0)
    {
      double factor = useViewDirection() ? 2.5 : 1; //show a little more if use_view_direction!
      nslices = (int)(data.dims.maxsize() * factor);
    }

    //see https://github.com/sci-visus/visus/issues/99
    const int max_nslices = 128;
    if (bFastRendering && nslices > max_nslices) {
      opacity *= nslices / (double)max_nslices;
      nslices = max_nslices;
    }

    shader->setOpacity(gl, opacity);

    if (useViewDirection())
    {
      if (!data.clipping.valid())
        gl.pushClippingBox(BoxNd(Point3d(0,0,0),Point3d(1,1,1)));

      gl.glRenderMesh(GLMesh::ViewDependentUnitVolume(gl.getFrustum(), nslices));

      if (!data.clipping.valid())
        gl.popClippingBox();
    }
    else
      gl.glRenderMesh(GLMesh::AxisAlignedUnitVolume(gl.getFrustum(),nslices));
  }
  else
  {
    shader->setOpacity(gl, opacity);

    gl.glRenderMesh(GLMesh::Quad(Point2d(0,0),Point2d(1,1),/*bNormal*/shader->config.lighting_enabled,/*bTexCoord*/true));
  }

  gl.popDepthMask();
  gl.popBlend();
  gl.popModelview();

  if (data.clipping.valid())
    gl.popClippingBox();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderArrayNode::writeToObjectStream(ObjectStream& ostream)
{
  if (ostream.isSceneMode())
  {
    ostream.pushContext("render");
    ostream.writeInline("type", "volume_slicer");
    ostream.write("lighting_enabled", cstring(lighting_enabled));
    ostream.write("use_view_direction", cstring(use_view_direction));
    ostream.write("max_num_slices", cstring(max_num_slices));
    ostream.popContext("render");
    return;
  }

  Node::writeToObjectStream(ostream);

  ostream.write("lighting_enabled",cstring(lighting_enabled));
  ostream.write("palette_enabled",cstring(palette_enabled));
  ostream.write("use_view_direction",cstring(use_view_direction));
  ostream.write("max_num_slices",cstring(max_num_slices));
  ostream.write("magnify_filter",cstring(magnify_filter));
  ostream.write("minify_filter",cstring(minify_filter));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
void RenderArrayNode::readFromObjectStream(ObjectStream& istream)
{
  if (istream.isSceneMode())
  {
    this->lighting_enabled = cbool(istream.read("lighting_enabled"));
    this->use_view_direction = cbool(istream.read("use_view_direction"));
    this->max_num_slices = cint(istream.read("max_num_slices"));
    return;
  }

  Node::readFromObjectStream(istream);

  this->lighting_enabled=cbool(istream.read("lighting_enabled"));
  this->palette_enabled=cbool(istream.read("palette_enabled"));
  this->use_view_direction=cbool(istream.read("use_view_direction"));
  this->max_num_slices=cint(istream.read("max_num_slices"));
  this->magnify_filter=cint(istream.read("magnify_filter"));
  this->minify_filter=cint(istream.read("minify_filter"));
}
 


} //namespace Visus

