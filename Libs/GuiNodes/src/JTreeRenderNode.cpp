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

#include <Visus/JTreeRenderNode.h>
#include <Visus/GLObjects.h>

namespace Visus {

////////////////////////////////////////////////////////////
JTreeRenderNode::JTreeRenderNode(String name) 
  : Node(name) 
{
  // how many vertices/edges have been rendered so far  
  min_material.front.diffuse=Color(0.0f, 0.2f, 1.0f, 1.0f);
  min_material.front.ambient=Color(0.0f, 0.1f, 0.5f, 1.0f);
  min_material.front.specular=Color(0.6f, 0.6f, 0.6f, 1.0f);
  min_material.front.emission=Colors::Black;
  min_material.front.shininess=10;

  max_material.front.diffuse=Color(1.0f, 0.2f, 0.0f, 1.0f);
  max_material.front.ambient=Color(0.5f, 0.1f, 0.0f, 1.0f);
  max_material.front.specular=Color(0.6f, 0.6f, 0.6f, 1.0f);
  max_material.front.emission=Colors::Black;
  max_material.front.shininess=10;

  saddle_material.front.diffuse=Color(204.0f/255.0f, 1.0f, 51.0f/255.0f, 1.0f);
  saddle_material.front.ambient=Color(0.5f*204.0f/255.0f, 0.5f, 0.5f*51.0f/255.0f, 1.0f);
  saddle_material.front.specular=Color(0.6f, 0.6f, 0.6f, 1.0f);
  saddle_material.front.emission=Colors::Black;
  saddle_material.front.shininess=10;

  addInputPort("graph");
}

////////////////////////////////////////////////////////////
JTreeRenderNode::~JTreeRenderNode()
{
}

////////////////////////////////////////////////////////////
Position JTreeRenderNode::getNodeBounds() 
{
  if (!graph) 
    return Position::invalid();

  {
    ScopedLock lock(graph->lock);
    return graph->bounds;
  }
}

////////////////////////////////////////////////////////////
bool JTreeRenderNode::processInput()
{
  this->graph=readValue<FGraph>("graph");
  return this->graph? true:false;
}



/////////////////////////////////////////////////////////////
template <class PointClass>
static void RenderVertex(GLCanvas& gl,GLPhongShader* shader,bool b2D,double radius,const PointClass& p,const GLMaterial& material)
{
  gl.pushModelview();

  gl.multModelview(Matrix::translate(Point3d(p[0],p[1],p[2]))* Matrix::scale(Point3d(radius,radius,radius)));
  if (b2D)
    shader->setUniformColor(gl,material.front.diffuse);
  else
    gl.setUniformMaterial(*shader,material);

  GLSolidSphere().glRender(gl);
  gl.popModelview();
}


/////////////////////////////////////////////////////////////
void JTreeRenderNode::glRender(GLCanvas& gl)
{
  if (!this->graph)
      return;

  ScopedLock lock(this->graph->lock);

  bool minima_tree  = cbool(this->graph->properties.getValue("minima_tree"));
  auto T            = this->graph->bounds.getTransformation();

  std::vector<int>    roots;
  std::map<int,Color> roots_color;
  for (int i=0;i<(int)graph->verts.size();i++)
  {
    const FGraph::Vertex &v=graph->verts[i];
    if (!v.deleted && !v.out_degree()) 
    {
      roots.push_back(i);
      roots_color[i]=Color::random();
    }
  }

  gl.pushDepthTest(!b2D);
  gl.pushCullFace(GL_BACK);

  GLPhongShader* shader=GLPhongShader::getSingleton(GLPhongShader::Config().withLightingEnabled(b2D?false:true));
  gl.setShader(shader);

  if (bool bEnableLighting=b2D?false:true)
  {
    Point3d pos,dir,vup;
    gl.getModelview().getLookAt(pos,dir,vup);
    gl.setUniformLight(*shader,Point4d(pos,1.0));
  }

  if (color_by_component)
  {
    GLMaterial material=this->max_material;

    while (!roots.empty())
    {
      int i=roots.back(); roots.pop_back();
      material.front.diffuse=roots_color[i];
    
      material.front.ambient=(0.5*material.front.diffuse).withAlpha(material.front.diffuse.getAlpha());

      std::vector<int> component;
      component.push_back(i);
      while (!component.empty())
      {
        int j=component.back(); component.pop_back();
        const FGraph::Vertex &v=graph->verts[j];  
        RenderVertex(gl,shader,b2D,radius,T*v.data,material);

        int in_degree =v.in_degree();
        for (int k=0;k<in_degree;k++) 
          component.push_back(graph->edges[v.in[k]].src);
      }
    }
  }
  else
  {
    for (int i=0;i<(int)graph->verts.size();i++)
    {
      const FGraph::Vertex &v=graph->verts[i];
      if (v.deleted) continue;
      int in_degree  =v.in_degree();
      int out_degree =v.out_degree();
      if      (draw_saddles && in_degree>0 && out_degree>0) RenderVertex(gl,shader,b2D,radius,T*v.data,saddle_material);
      else if (draw_extrema && in_degree==0)                RenderVertex(gl,shader,b2D,radius,T*v.data,minima_tree?min_material : max_material);
      else if (draw_extrema && out_degree==0)               RenderVertex(gl,shader,b2D,radius,T*v.data,minima_tree?max_material : min_material);
    }
  }

  // Render edges
  if (draw_edges)
  {
    GLMesh mesh;
    mesh.begin(GL_LINES);
    for (int i=0;i<(int)graph->edges.size();i++)
    {
      const FGraph::Edge &e=graph->edges[i];
      if (e.deleted) 
        continue;
      
      const FGraph::Vertex &s=graph->verts[e.src];
      const FGraph::Vertex &d=graph->verts[e.dst];

      mesh.vertex(T * s.data);
      mesh.vertex(T * d.data);
    }
    mesh.end();

    gl.setUniformMaterial(*shader,saddle_material);
    gl.glRenderMesh(mesh);
  }

  gl.popCullFace();
  gl.popDepthTest();
}


/////////////////////////////////////////////////////////////
void JTreeRenderNode::writeToObjectStream(ObjectStream& ostream)
{
  Node::writeToObjectStream(ostream);

  ostream.writeValue("color_by_component",cstring(color_by_component));
  ostream.writeValue("draw_saddles",cstring(draw_saddles));
  ostream.writeValue("draw_extrema",cstring(draw_extrema));
  ostream.writeValue("draw_edges",cstring(draw_edges));
  ostream.writeValue("b2D",cstring(b2D));
  ostream.writeValue("radius",cstring(radius));

  ostream.writeObject("min_material", min_material);
  ostream.writeObject("max_material", max_material);
  ostream.writeObject("saddle_material", saddle_material);
}

/////////////////////////////////////////////////////////////
void JTreeRenderNode::readFromObjectStream(ObjectStream& istream)
{
  Node::readFromObjectStream(istream);

  color_by_component=cbool(istream.readValue("color_by_component"));
  draw_saddles=cbool(istream.readValue("draw_saddles"));
  draw_extrema=cbool(istream.readValue("draw_extrema"));
  draw_edges=cbool(istream.readValue("draw_edges"));
  b2D=cbool(istream.readValue("b2D"));
  radius=cdouble(istream.readValue("radius"));

  istream.readObject("min_material", min_material);
  istream.readObject("max_material", max_material);
  istream.readObject("saddle_material", saddle_material);
}


} //namespace Visus

