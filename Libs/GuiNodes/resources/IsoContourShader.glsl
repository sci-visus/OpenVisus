#include <Visus/GLCommon.glsl>

uniform mat4      u_modelview_matrix;
uniform mat4      u_projection_matrix;
uniform mat3      u_normal_matrix;

uniform sampler3D u_sampler;
uniform vec3      u_sampler_dims;

#define FIELD(pos) texture3D(u_sampler, pos)

#if PALETTE_ENABLED
uniform sampler2D u_palette_sampler;
#define PALETTE(pos) texture2D(u_palette_sampler,vec2(pos,0.0))
#endif

uniform vec4      u_frontmaterial_ambient;
uniform vec4      u_frontmaterial_diffuse;
uniform vec4      u_frontmaterial_specular;
uniform float     u_frontmaterial_shininess;

uniform vec4      u_backmaterial_ambient;
uniform vec4      u_backmaterial_diffuse;
uniform vec4      u_backmaterial_specular;
uniform float     u_backmaterial_shininess;

uniform vec4      u_light_position;

varying vec3      v_lighting_normal;
varying vec3      v_lighting_dir;
varying vec3      v_lighting_eyevec;
varying vec3      v_texcoord;

#if PALETTE_ENABLED && VERTEX_COLOR_INDEX_ENABLED
varying vec4      v_color;
#endif

///////////////////////////////////////////////////////////////
#ifdef VERTEX_SHADER

attribute vec4    a_position;
attribute vec4    a_color;

void main()
{
  vec4 eye_pos= u_modelview_matrix * a_position;
  gl_Position = u_projection_matrix * eye_pos;

  //normals
  vec3 step=vec3(1.0/u_sampler_dims.x,1.0/u_sampler_dims.y,1.0/u_sampler_dims.z);

  v_texcoord=vec3(a_position.xyz*step);

  vec3 world_normal = -normalize(vec3(
    FIELD(v_texcoord+vec3(step.x,0,0)).r - FIELD(v_texcoord-vec3(step.x,0,0)).r,
    FIELD(v_texcoord+vec3(0,step.y,0)).r - FIELD(v_texcoord-vec3(0,step.y,0)).r,
    FIELD(v_texcoord+vec3(0,0,step.z)).r - FIELD(v_texcoord-vec3(0,0,step.z)).r));

  //see http://www.ozone3d.net/tutorials/glsl_lighting_phong_p2.php
  vec3 vVertex = vec3(u_modelview_matrix * a_position);
  v_lighting_normal = u_normal_matrix * world_normal;
  v_lighting_dir    = normalize(vec3(u_light_position.xyz - vVertex));
  v_lighting_eyevec = normalize(-vVertex);

  #if PALETTE_ENABLED && VERTEX_COLOR_INDEX_ENABLED
    v_color = a_color;
  #endif
}

#endif


///////////////////////////////////////////////////////////////
#ifdef FRAGMENT_SHADER
void main()
{
  vec3 N = normalize(v_lighting_normal);
  vec3 L = normalize(v_lighting_dir);
  vec3 E = normalize(v_lighting_eyevec);
  vec4 color=vec4(1.0,1.0,1.0,1.0);

  #if PALETTE_ENABLED
    #if VERTEX_COLOR_INDEX_ENABLED
      color=PALETTE(v_color.r);
    #else
      //PROBLEM since I load the texture as luminance-alpha the second field is in 'a'
      float field2=FIELD(v_texcoord).a;
      color=PALETTE(field2);
    #endif
  #else
    vec3 texcoord=v_texcoord; //quell compiler error on OSX
  #endif

  //lighting
  vec4 lighting_color;
  float lambertTerm = dot(N,L);
  if(lambertTerm>=0.0)
  {
    lighting_color  = u_frontmaterial_ambient;
    lighting_color += u_frontmaterial_diffuse * lambertTerm;
    vec3 R = reflect(-L, N);
    float specular = pow( max(dot(R, E), 0.0), u_frontmaterial_shininess);
    lighting_color += u_frontmaterial_specular * specular;
  }
  else
  {
    lambertTerm=-1.0*lambertTerm;N=-N;
    lighting_color  = u_backmaterial_ambient;
    lighting_color += u_backmaterial_diffuse * lambertTerm;
    vec3 R = reflect(-L, N);
    float specular = pow( max(dot(R, E), 0.0), u_backmaterial_shininess);
    lighting_color += u_backmaterial_specular * specular;
  }
  color.rgb = color.rgb * lighting_color.rgb;

  gl_FragColor = color;
}

#endif
