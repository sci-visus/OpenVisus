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

#include <Visus/GLShader.h>
#include <Visus/GLCanvas.h>
#include <Visus/GLInfo.h>
#include <Visus/Gui.h>

#include <Visus/Exception.h>

#include <QTextStream>

namespace Visus {

///////////////////////////////////////////////
GLProgram::GLProgram(QOpenGLWidget* gl,const GLShader& shader)
{
  String body=StringUtils::replaceFirst(shader.source,"#include <Visus/GLCommon.glsl>",QUtils::LoadTextFileFromResources(":/GLCommon.glsl"));

  vertexshader=new QOpenGLShader(QOpenGLShader::Vertex, gl);
  {
    std::ostringstream out;
    out<<"#define VERTEX_SHADER 1 \n";
    for (auto it : shader.defines)
      out<<"#define "<<it.first<<" "<<it.second<<"\n";
    out<<body;
    bool bOk=vertexshader->compileSourceCode(out.str().c_str());VisusAssert(bOk);
  }
  
  fragmentshader=new QOpenGLShader(QOpenGLShader::Fragment, gl);
  {
    std::ostringstream out;
    out<<"#define FRAGMENT_SHADER 1 \n";
    for (auto it : shader.defines)
      out<<"#define "<<it.first<<" "<<it.second<<"\n";
    out<<body;
    bool bOk=fragmentshader->compileSourceCode(out.str().c_str());VisusAssert(bOk);
  }

  program = new QOpenGLShaderProgram();
  program->addShader(vertexshader);
  program->addShader(fragmentshader);

  //bug on macosx! (see http://forum.libcinder.org/topic/glvertexattrib-incompatibility-between-ios-and-os-x)
  //this means that your shader musy have always the "a_position" attribute! and this has to be the position!
  program->bindAttributeLocation("a_position",0);  
  bool bOk=program->link(); VisusAssert(bOk);
  
  program->bind();
  {
    //uniforms
    this->uniform_location.resize(1024);
    for (int I=0;I<(int)shader.uniforms.size();I++)
    {
      const GLUniform& uniform=shader.uniforms[I];
      setUniformLocation(uniform,program->uniformLocation(uniform.name.c_str()));
    }

    //attributes
    this->attribute_location.resize(256,-1);
    for (int I=0;I<(int)shader.attributes.size();I++)
    {
      const GLAttribute& attribute=shader.attributes[I];
      setAttributeLocation(attribute,program->attributeLocation(attribute.name.c_str()));
    }
  }

  program->release();
}

///////////////////////////////////////////////
GLProgram::~GLProgram()
{
  if (this->program)
  {
    auto program=this->program;
    GLDoWithContext::getSingleton()->push_back([program](){
      delete program;
    });
  }
}

///////////////////////////////////////////////
GLShader::GLShader(String filename_) : filename(filename_),id(GLSHADER_ID()++)
{
  this->source=QUtils::LoadTextFileFromResources(this->filename);

  //VisusInfo() << this->source;

  VisusAssert(!this->source.empty());

  addDefine("VISUS_OPENGL_ES"       ,cstring(VISUS_OPENGL_ES));
  addDefine("VISUS_OPENGL_TEXTURE3D",cstring((int)GLInfo::getSingleton()->hasTexture3D()));

  u_modelview_matrix=addUniform("u_modelview_matrix");
  u_projection_matrix=addUniform("u_projection_matrix");
  u_normal_matrix=addUniform("u_normal_matrix");

  for (int I=0;I<6;I++)
    u_clippingbox_plane[I]=addUniform("u_clippingbox_plane["+cstring(I)+"]");

  a_position = addAttribute("a_position");
  a_normal   = addAttribute("a_normal");
  a_color    = addAttribute("a_color");
  a_texcoord = addAttribute("a_texcoord");

  u_light_position=addUniform("u_light_position");

  u_frontmaterial_ambient=addUniform("u_frontmaterial_ambient");
  u_frontmaterial_diffuse=addUniform("u_frontmaterial_diffuse");
  u_frontmaterial_specular=addUniform("u_frontmaterial_specular");
  u_frontmaterial_emission=addUniform("u_frontmaterial_emission");
  u_frontmaterial_shininess=addUniform("u_frontmaterial_shininess");

  u_backmaterial_ambient=addUniform("u_backmaterial_ambient");
  u_backmaterial_diffuse=addUniform("u_backmaterial_diffuse");
  u_backmaterial_specular=addUniform("u_backmaterial_specular");
  u_backmaterial_emission=addUniform("u_backmaterial_emission");
  u_backmaterial_shininess=addUniform("u_backmaterial_shininess");
}

///////////////////////////////////////////////
void GLShader::addDefine(String key,String value)
{VisusAssert(!defines.hasValue(key) && !StringUtils::contains(key," "));defines.setValue(key,value);}

///////////////////////////////////////////////
GLUniform GLShader::addUniform(String key)
{
  VisusAssert(!key.empty());
  GLUniform uniform(key,(int)uniforms.size());
  VisusAssert(std::find(uniforms.begin(),uniforms.end(),uniform)==uniforms.end());
  uniforms.push_back(uniform);
  return uniform;
}

///////////////////////////////////////////////
GLAttribute GLShader::addAttribute(String key)
{
  VisusAssert(!key.empty());
  GLAttribute attribute(key);
  VisusAssert(std::find(attributes.begin(),attributes.end(),attribute)==attributes.end());
  attributes.push_back(attribute);
  return attribute;
}

///////////////////////////////////////////////
GLSampler GLShader::addSampler(String key) {
  GLSampler ret;
  ret.u_sampler=addUniform(key);
  ret.u_sampler_dims=addUniform(key+"_dims");  
  ret.u_sampler_vs=addUniform(key+"_vs"); 
  ret.u_sampler_vt=addUniform(key+"_vt"); 
  ret.u_sampler_envmode=addUniform(key+"_envmode");
  ret.u_sampler_ncomponents = addUniform(key + "_ncomponents");
  return ret;
}

} //namespace

