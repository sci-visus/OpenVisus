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

#ifndef __VISUS_GL_SHADER_H
#define __VISUS_GL_SHADER_H

#include <Visus/Gui.h>

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
  #include <QOpenGLExtraFunctions>
#else
  #include <QOpenGLFunctions>
  #define QOpenGLExtraFunctions QOpenGLFunctions
#endif

#include <QOpenGLShaderProgram>
#include <QOpenGLShader>
#include <QOpenGLWidget>

namespace Visus {

  ////////////////////////////////////////////////////////
class VISUS_GUI_API GLUniform
{
public:

  VISUS_CLASS(GLUniform)

  String name;
  int    id;
  
  //constructor
  inline GLUniform() : id(-1)
  {}

  //constructor
  inline GLUniform(String name_,int id_) : name(name_),id(id_)
  {}

  //valid
  bool valid() const {
    return id >= 0;
  }
  
  //operator==
  bool operator==(const GLUniform& other) const
  {return name==other.name;}

};

////////////////////////////////////////////////////////
class VISUS_GUI_API GLAttribute
{
public:

  VISUS_CLASS(GLAttribute)

  enum
  {
    a_position=0,
    a_normal,
    a_color,
    a_texcoord
  };

  String name;
  int    id;

  //constructor
  GLAttribute() : id(-1)
  {}

  //constructor
  GLAttribute(int id_) : id(id_)
  {
    switch (id)
    {
      case a_position: name="a_position"; break;
      case a_normal  : name="a_normal"  ; break;
      case a_color   : name="a_color"   ; break;
      case a_texcoord: name="a_texcoord"; break;
      default:VisusAssert(false);
    }
  }

  //constructor
  GLAttribute(String name_) : name(name_),id(-1)
  {
    if      (name=="a_position" ) this->id=a_position;
    else if (name=="a_normal"   ) this->id=a_normal;
    else if (name=="a_color"    ) this->id=a_color;
    else if (name=="a_texcoord" ) this->id=a_texcoord;
    else {VisusAssert(false);}
  }

  //valid
  bool valid() const {
    return id >= 0;
  }

  //operator==
  bool operator==(const GLAttribute& other) const
  {return name==other.name;}

};

////////////////////////////////////////////////////////
class VISUS_GUI_API GLSampler
{
public:

  VISUS_CLASS(GLSampler)

  GLUniform u_sampler;
  GLUniform u_sampler_dims;
  GLUniform u_sampler_envmode;
  GLUniform u_sampler_ncomponents;
  GLUniform u_sampler_vs;
  GLUniform u_sampler_vt;

  //construtor
  GLSampler() 
  {}


  //valid
  bool valid() const {
    return u_sampler.valid();
  }

};

//predeclaration
class GLShader;

////////////////////////////////////////////////////////
class VISUS_GUI_API GLProgram 
{
public:

  VISUS_NON_COPYABLE_CLASS(GLProgram)

  //constructor
  GLProgram(QOpenGLWidget* gl,const GLShader& shader);

  //destructor
  virtual ~GLProgram();

  //bind
  void bind() {
    program->bind();
  }

  //getAttributeLocation
  int getAttributeLocation(const GLAttribute& attribute) const
  {return attribute_location[attribute.id];}

  //setAttributeLocation
  void setAttributeLocation(const GLAttribute& attribute,int location)
  {attribute_location[attribute.id]=location;}

  //getUniformLocation
  int getUniformLocation(const GLUniform& uniform) const
  {return uniform_location[uniform.id];}

  //setUniformLocation
  void setUniformLocation(const GLUniform& uniform,int location)
  {uniform_location[uniform.id]=location;}

private:

  QOpenGLShaderProgram* program=nullptr;
  QOpenGLShader*        vertexshader=nullptr;
  QOpenGLShader*        fragmentshader=nullptr;


  std::vector<int> attribute_location;
  std::vector<int> uniform_location;
};


////////////////////////////////////////////////////////
class VISUS_GUI_API GLShader 
{
public:

  VISUS_NON_COPYABLE_CLASS(GLShader)

  //internal use only
  int                         __program_id__ =0;

  String                      filename;
  String                      source;

  StringMap                   defines;

  std::vector<GLAttribute>      attributes;

  GLAttribute                   a_position;
  GLAttribute                   a_normal;
  GLAttribute                   a_color;
  GLAttribute                   a_texcoord;

  std::vector<GLUniform>        uniforms;

  GLUniform                     u_modelview_matrix;
  GLUniform                     u_projection_matrix;
  GLUniform                     u_normal_matrix;
  GLUniform                     u_clippingbox_plane[6];

  //in case of lighting
  GLUniform                     u_light_position;

  GLUniform                     u_frontmaterial_ambient;
  GLUniform                     u_frontmaterial_diffuse;
  GLUniform                     u_frontmaterial_specular;
  GLUniform                     u_frontmaterial_emission;
  GLUniform                     u_frontmaterial_shininess;

  GLUniform                     u_backmaterial_ambient;
  GLUniform                     u_backmaterial_diffuse;
  GLUniform                     u_backmaterial_specular;
  GLUniform                     u_backmaterial_emission;
  GLUniform                     u_backmaterial_shininess;

  // constructor
  GLShader(String filename_);

  //destructor
  virtual ~GLShader() {
  }

  //addDefine
  void addDefine(String key,String value);

  //addUniform
  GLUniform addUniform(String key);

  //addAttribute
  GLAttribute addAttribute(String key);

  //addSampler
  GLSampler addSampler(String key);


}; //end class


} //namespace

#endif //__VISUS_GL_SHADER_H
