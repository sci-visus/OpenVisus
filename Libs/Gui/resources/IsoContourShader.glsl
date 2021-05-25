#include <Visus/GLCommon.glsl>

uniform mat4      u_modelview_matrix;
uniform mat4      u_projection_matrix;
uniform mat3      u_normal_matrix;

uniform sampler3D u_field;
uniform vec3      u_field_dims;

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

#if SECOND_FIELD_NCHANNELS>0
uniform sampler3D u_second_field;
uniform sampler2D u_palette;
#endif


CLIPPINGBOX_GLOBALS()

///////////////////////////////////////////////////////////////
#ifdef VERTEX_SHADER

attribute vec4    a_position;
attribute vec4    a_color;

void main()
{
  vec4 eye_pos= u_modelview_matrix * a_position;
  gl_Position = u_projection_matrix * eye_pos;

  //normals
  vec3 step=vec3(1.0/u_field_dims.x,1.0/u_field_dims.y,1.0/u_field_dims.z);

  v_texcoord=vec3(a_position.xyz*step);

  vec3 world_normal = -normalize(vec3(
    texture3D(u_field, v_texcoord+vec3(step.x,0,0)).r - texture3D(u_field, v_texcoord-vec3(step.x,0,0)).r,
    texture3D(u_field, v_texcoord+vec3(0,step.y,0)).r - texture3D(u_field, v_texcoord-vec3(0,step.y,0)).r,
    texture3D(u_field, v_texcoord+vec3(0,0,step.z)).r - texture3D(u_field, v_texcoord-vec3(0,0,step.z)).r));

  //see http://www.ozone3d.net/tutorials/glsl_lighting_phong_p2.php
  vec3 vVertex = vec3(u_modelview_matrix * a_position);
  v_lighting_normal = u_normal_matrix * world_normal;
  v_lighting_dir    = normalize(vec3(u_light_position.xyz - vVertex));
  v_lighting_eyevec = normalize(-vVertex);

  CLIPPINGBOX_VERTEX_SHADER()
}

#endif


///////////////////////////////////////////////////////////////
#ifdef FRAGMENT_SHADER
void main()
{
  CLIPPINGBOX_FRAGMENT_SHADER()

  #if SECOND_FIELD_NCHANNELS>0

    vec4 color=texture3D(u_second_field, v_texcoord);

	//apply palette
	#if SECOND_FIELD_NCHANNELS==1
		color.rgba=texture2D(u_palette, vec2(color.r, 0)).rgba;

	#elif SECOND_FIELD_NCHANNELS==2
		color.rgb=texture2D(u_palette, vec2(color.r, 0)).rgb;
		color.a  =texture2D(u_palette, vec2(color.a, 0)).a;

	#elif SECOND_FIELD_NCHANNELS==3
		color.r=texture2D(u_palette, vec2(color.r, 0)).r;
		color.g=texture2D(u_palette, vec2(color.g, 0)).g;
		color.b=texture2D(u_palette, vec2(color.b, 0)).b;
		color.a=1.0;

	#elif SECOND_FIELD_NCHANNELS==4
		color.r=texture2D(u_palette, vec2(color.r, 0)).r;
		color.g=texture2D(u_palette, vec2(color.g, 0)).g;
		color.b=texture2D(u_palette, vec2(color.b, 0)).b;
		color.a=texture2D(u_palette, vec2(color.a, 0)).a;

	#endif

  #else

	vec4 color=vec4(1.0,1.0,1.0,1.0);

	 //quell compiler error on OSX
    vec3 texcoord=v_texcoord;
  #endif

  //lighting
  vec3 N = normalize(v_lighting_normal);
  vec3 L = normalize(v_lighting_dir);
  vec3 E = normalize(v_lighting_eyevec);
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
