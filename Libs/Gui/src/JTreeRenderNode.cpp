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
JTreeRenderNode::JTreeRenderNode() 
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
void JTreeRenderNode::execute(Archive& ar)
{
  if (ar.name == "SetColorByComponent") {
    bool value;
    ar.read("value", value);
    setColorByComponent(value);
    return;
  }

  if (ar.name == "SetDrawSaddles") {
    bool value;
    ar.read("value", value);
    setDrawSaddles(value);
    return;
  }

  if (ar.name == "SetDrawExtrema") {
    bool value;
    ar.read("value", value);
    setDrawExtrema(value);
    return;
  }

  if (ar.name == "SetDrawEdges") {
    bool value;
    ar.read("value", value);
    setDrawEdges(value);
    return;
  }

  if (ar.name == "Set2d") {
    bool value;
    ar.read("value", value);
    set2d(value);
    return;
  }

  if (ar.name == "SetRadius") {
    double value;
    ar.read("value", value);
    setRadius(value);
    return;
  }

  if (ar.name == "SetMinMaterial") {
    GLMaterial value;
    value.read(*ar.getFirstChild());
    setMinMaterial(value);
    return;
  }

  if (ar.name == "SetMaxMaterial") {
    GLMaterial value;
    value.read(*ar.getFirstChild());
    setMaxMaterial(value);
    return;
  }

  if (ar.name == "SetSaddleMaterial") {
    GLMaterial value;
    value.read(*ar.getFirstChild());
    setSaddleMaterial(value);
    return;
  }

  return Node::execute(ar);
}

////////////////////////////////////////////////////////////
Position JTreeRenderNode::getBounds()
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

  gl.pushDepthTest(!is_2d);
  gl.pushCullFace(GL_BACK);

  GLPhongShader* shader=GLPhongShader::getSingleton(GLPhongShader::Config().withLightingEnabled(is_2d ?false:true));
  gl.setShader(shader);

  if (bool bEnableLighting= is_2d ?false:true)
  {
    Point3d pos,dir,vup;
    gl.getModelview().getLookAt(pos, dir,vup);
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
        RenderVertex(gl,shader, is_2d,radius,T*v.data,material);

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
      if      (draw_saddles && in_degree>0 && out_degree>0) RenderVertex(gl,shader, is_2d,radius,T*v.data,saddle_material);
      else if (draw_extrema && in_degree==0)                RenderVertex(gl,shader, is_2d,radius,T*v.data,minima_tree?min_material : max_material);
      else if (draw_extrema && out_degree==0)               RenderVertex(gl,shader, is_2d,radius,T*v.data,minima_tree?max_material : min_material);
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
void JTreeRenderNode::write(Archive& ar) const
{
  Node::write(ar);

  ar.write("color_by_component", color_by_component);
  ar.write("draw_saddles", draw_saddles);
  ar.write("draw_extrema", draw_extrema);
  ar.write("draw_edges", draw_edges);
  ar.write("is_2d", is_2d);
  ar.write("radius", radius);

  ar.writeObject("min_material", min_material);
  ar.writeObject("max_material", max_material);
  ar.writeObject("saddle_material", saddle_material);
}

/////////////////////////////////////////////////////////////
void JTreeRenderNode::read(Archive& ar)
{
  Node::read(ar);

  ar.read("color_by_component", color_by_component);
  ar.read("draw_saddles", draw_saddles);
  ar.read("draw_extrema", draw_extrema);
  ar.read("draw_edges", draw_edges);
  ar.read("is_2d", is_2d);
  ar.read("radius", radius);

  ar.readObject("min_material", min_material);
  ar.readObject("max_material", max_material);
  ar.readObject("saddle_material", saddle_material);
}

/////////////////////////////////////////////////////////
class JTreeRenderNodeView :
  public QFrame,
  public View<JTreeRenderNode>
{
public:

  //constructor
  JTreeRenderNodeView(JTreeRenderNode* model = nullptr) {
    if (model)
      bindModel(model);
  }

  //destructor
  virtual ~JTreeRenderNodeView() {
    bindModel(nullptr);
  }

  //bindModel
  virtual void bindModel(JTreeRenderNode* value) override
  {
    if (this->model)
    {
      QUtils::clearQWidget(this);
      widgets = Widgets();
    }

    View<ModelClass>::bindModel(value);

    if (this->model)
    {
      QFormLayout* layout = new QFormLayout();

      layout->addRow("color_by_component", widgets.color_by_component = GuiFactory::CreateCheckBox(model->colorByComponent(), "", [this](int value) {
        model->setColorByComponent((bool)value);
      }));

      layout->addRow("draw_saddles", widgets.draw_saddles = GuiFactory::CreateCheckBox(model->drawSaddles(), "", [this](int value) {
        model->setDrawSaddles((bool)value);
      }));

      layout->addRow("draw_extrema", widgets.draw_extrema = GuiFactory::CreateCheckBox(model->drawExtrema(), "", [this](int value) {
        model->setDrawExtrema((bool)value);
      }));

      layout->addRow("draw_edges", widgets.draw_edges = GuiFactory::CreateCheckBox(model->drawEdges(), "", [this](int value) {
        model->setDrawEdges((bool)value);
      }));

      layout->addRow("is_2d", widgets.is_2d = GuiFactory::CreateCheckBox(model->is2d(), "", [this](int value) {
        model->set2d((bool)value);
      }));

      layout->addRow("radius", widgets.radius = GuiFactory::CreateDoubleSliderWidget(model->getRadius(), Range(0.1, 10, 0), [this](double value) {
        model->setRadius(value);
      }));

      layout->addRow("min_material", widgets.min_material = GuiFactory::CreateGLMaterialView(model->getMinMaterial(), [this](GLMaterial value) {
        model->setMinMaterial(value);
      }));

      layout->addRow("max_material", widgets.max_material = GuiFactory::CreateGLMaterialView(model->getMaxMaterial(), [this](GLMaterial value) {
        model->setMaxMaterial(value);
      }));

      layout->addRow("saddle_material", widgets.saddle_material = GuiFactory::CreateGLMaterialView(model->getSaddleMaterial(), [this](GLMaterial value) {
        model->setSaddleMaterial(value);
      }));

      setLayout(layout);
      refreshGui();
    }
  }

private:

  class Widgets
  {
  public:
    QCheckBox* color_by_component = nullptr;
    QCheckBox* draw_saddles = nullptr;
    QCheckBox* draw_extrema = nullptr;
    QCheckBox* draw_edges = nullptr;
    QCheckBox* is_2d = nullptr;
    QDoubleSlider* radius = nullptr;
    GuiFactory::GLMaterialView* min_material = nullptr;
    GuiFactory::GLMaterialView* max_material = nullptr;
    GuiFactory::GLMaterialView* saddle_material = nullptr;
  };

  Widgets widgets;

  ///refreshGui
  void refreshGui()
  {
    widgets.color_by_component->setChecked(model->colorByComponent());
    widgets.draw_saddles->setChecked(model->drawSaddles());
    widgets.draw_extrema->setChecked(model->drawExtrema());
    widgets.draw_edges->setChecked(model->drawEdges());
    widgets.is_2d->setChecked(model->is2d());
    widgets.radius->setValue(model->getRadius());
    widgets.min_material->setMaterial(model->getMinMaterial());
    widgets.max_material->setMaterial(model->getMaxMaterial());
    widgets.saddle_material->setMaterial(model->getSaddleMaterial());
  }

  //modelChanged
  virtual void modelChanged() override {
    refreshGui();
  }

};

void JTreeRenderNode::createEditor()
{
  auto win = new JTreeRenderNodeView(this);
  win->show();
}




} //namespace Visus

