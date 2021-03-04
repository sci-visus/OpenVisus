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

#ifndef VISUS_TRANSFER_FUNCTION_VIEW_H
#define VISUS_TRANSFER_FUNCTION_VIEW_H

#include <Visus/Gui.h>
#include <Visus/Gui.h>
#include <Visus/TransferFunction.h>
#include <Visus/Model.h>
#include <Visus/QDoubleSlider.h>
#include <Visus/GuiFactory.h>
#include <Visus/StringUtils.h>
#include <Visus/QCanvas2d.h>
#include <Visus/ArrayStatisticsView.h>
#include <Visus/RGBAColorMap.h>

#include <QApplication>
#include <QFontDatabase>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QWheelEvent>

namespace Visus {

////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API TransferFunctionColorBarView :
  public QFrame,
  public View<TransferFunction>
{
public:

  VISUS_NON_COPYABLE_CLASS(TransferFunctionColorBarView)

  //constructor
  TransferFunctionColorBarView(TransferFunction* model=nullptr) {
    setMinimumHeight(24);
    if (model)
      bindModel(model);
  }

  //destructor
  virtual ~TransferFunctionColorBarView() {
    bindModel(nullptr);
  }

  //bindModel
  virtual void bindModel(TransferFunction* model) override 
  {
    if (this->model)
    this->img.reset();

    View<ModelClass>::bindModel(model);

    if (this->model)
      refreshGui();
  }
  

  //isShowingCheckboard
  bool isShowingCheckboard() const {
    return bShowCheckedBoard;
  }

  //showCheckboard
  void showCheckboard(bool value) {
    bShowCheckedBoard = value;
    rebindModel();
  }

private:

  bool               bShowCheckedBoard=false;
  SharedPtr<QImage>  img;

  //refreshGui
  virtual void refreshGui() {
    createImage();
    update();
  }

  //modelChanged
  virtual void modelChanged() override {
    refreshGui();
  }

  //createImage
  void createImage()
  {
    VisusAssert(model);
    this->img.reset();

    int nsamples = model->getNumberOfSamples();
    if (!nsamples)
      return;

    double attenuation=model->getAttenuation();
    const auto& R=model->R->values;
    const auto& G=model->G->values;
    const auto& B=model->B->values;
    const auto& A=model->A->values;

    this->img.reset(new QImage(nsamples,1,QImage::Format_ARGB32));
    for (int x=0; x<nsamples; x++)
    {
      auto r  =                     (Uint8)(R[x] * 255.0);
      auto g  =                     (Uint8)(G[x] * 255.0);
      auto b  =                     (Uint8)(B[x] * 255.0);
      auto a  = bShowCheckedBoard ? (Uint8)(A[x] * 255.0 * (1.0 - attenuation)) : 255;
      this->img->setPixel(x,0,qRgba(r,g,b,a));
    }
  }

  //paintEvent
  virtual void paintEvent(QPaintEvent* evt) override 
  {
    if (!img)
      return;

    QPainter painter(this);
    int W = (int)this->width();
    int H = (int)this->height();

    if (bShowCheckedBoard)
      QUtils::RenderCheckerBoard(painter,0, 0, W, H, 8, 8, Color(180, 180, 180, 255), Color(0, 0, 0, 0));

    painter.setTransform(QTransform::fromScale(W / (double)img->width(), H / (double)img->height()),true);
    painter.setOpacity(1.0);
    painter.setPen(QUtils::convert<QColor>(Colors::White));
    painter.drawImage(QPoint(0,0),*img);
  }

};

////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API TransferFunctionSelectedFunctionsView :
  public QFrame,
  public View<TransferFunction>
{
  Q_OBJECT

public:

  VISUS_NON_COPYABLE_CLASS(TransferFunctionSelectedFunctionsView)

  struct
  {
    std::vector<QCheckBox*> checkboxes;
    QDoubleSlider* attenuation = nullptr;
  }
  widgets;

  //constructor
  TransferFunctionSelectedFunctionsView(TransferFunction* model=nullptr) {
    if (model)
      bindModel(model);
  }

  //destructor
  virtual ~TransferFunctionSelectedFunctionsView() {
    bindModel(nullptr);
  }

  //bindModel
  virtual void bindModel(TransferFunction* model) override 
  {
    if (this->model)
    {
      QUtils::clearQWidget(this);
      widgets.checkboxes.clear();
    }

    View<ModelClass>::bindModel(model);

    if (this->model)
    {
      QHBoxLayout* layout = new QHBoxLayout(),*sub;
      layout->addLayout(sub = new QHBoxLayout());

      widgets.checkboxes.resize(4);
      sub->addWidget(widgets.checkboxes[0] = GuiFactory::CreateCheckBox(false, "R", [this](int) {checkBoxClicked(0); }));
      sub->addWidget(widgets.checkboxes[1] = GuiFactory::CreateCheckBox(false, "G", [this](int) {checkBoxClicked(1); }));
      sub->addWidget(widgets.checkboxes[2] = GuiFactory::CreateCheckBox(false, "B", [this](int) {checkBoxClicked(2); }));
      sub->addWidget(widgets.checkboxes[3] = GuiFactory::CreateCheckBox(true , "A", [this](int) {checkBoxClicked(3); }));

      layout->addLayout(sub = new QHBoxLayout());
      sub->addWidget(new QLabel("Attenuation"));
      sub->addWidget(widgets.attenuation = GuiFactory::CreateDoubleSliderWidget(model->getAttenuation(), Range(0, 1, 0), [this](double value) {
        this->model->setAttenutation(value);
      }));

      setLayout(layout);
    }
  }

  //isSelected
  bool isSelected(int F) const 
  {
    return widgets.checkboxes[F]->isChecked();
  }

signals:

  void selectionChanged();

private:

  //checkBoxClicked
  void checkBoxClicked(int index)
  {
    if (!(QApplication::keyboardModifiers() & Qt::ShiftModifier))
    { 
      int I=0;for (auto checkbox : widgets.checkboxes)
      {
        if (I!=index && checkbox->isChecked())
        {
          auto old =checkbox->blockSignals(true);
          checkbox->setChecked(false);
          checkbox->blockSignals(old);
        }
        I++;
      }
    }

    emit selectionChanged();
  }

  //modelChanged
  virtual void modelChanged() override 
  {
    widgets.attenuation->setValue(model->getAttenuation());
    //rebindModel();
  }

};

////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API TransferFunctionCanvasView :
  public QCanvas2d,
  public View<TransferFunction>
{
  Q_OBJECT

  Point2d           last_pos;
  UniquePtr<QTimer> dragging;
  Statistics        stats;

public:

  VISUS_NON_COPYABLE_CLASS(TransferFunctionCanvasView)

  TransferFunctionSelectedFunctionsView*  selection=nullptr;

  //constructor
  TransferFunctionCanvasView(TransferFunction* model=nullptr,TransferFunctionSelectedFunctionsView* selected_functions_=nullptr) 
    : selection(selected_functions_)
  {
    if (model)
      bindModel(model);

    if (selection)
      connect(selection,&TransferFunctionSelectedFunctionsView::selectionChanged,this,&TransferFunctionCanvasView::selectedFunctionChanged);
  }

  //destructor
  virtual ~TransferFunctionCanvasView() {
    bindModel(nullptr);
  }

public slots:

  //selectedFunctionChanged
  void selectedFunctionChanged() {
    update();
  }

public:

  //bindModel
  virtual void bindModel(TransferFunction* model) override 
  {
    View<ModelClass>::bindModel(model);

    if (this->model) 
      update();
  }

  //modelChanged
  virtual void modelChanged() override {
    update();
  }

  //drawLine
  void drawLine(Point2d p1,Point2d p2)
  {
    int N = model->getNumberOfSamples();
    p1.x = Utils::clamp(p1.x, 0.0, 1.0); p1.y = Utils::clamp(p1.y, 0.0, 1.0);
    p2.x = Utils::clamp(p2.x, 0.0, 1.0); p2.y = Utils::clamp(p2.y, 0.0, 1.0);

    int x1 = Utils::clamp((int)(round(p1.x * (N - 1))), 0, N - 1);
    int x2 = Utils::clamp((int)(round(p2.x * (N - 1))), 0, N - 1);

    if (selection->isSelected(0)) model->drawLine(0, x1, p1.y, x2, p2.y);
    if (selection->isSelected(1)) model->drawLine(1, x1, p1.y, x2, p2.y);
    if (selection->isSelected(2)) model->drawLine(2, x1, p1.y, x2, p2.y);
    if (selection->isSelected(3)) model->drawLine(3, x1, p1.y, x2, p2.y);
  }

  //mousePressEvent
  virtual void mousePressEvent(QMouseEvent* evt) override 
  {
    // painting functions
    if (model && evt->button()==Qt::LeftButton)
    { 
      model->beginTransaction();
      dragging.reset(new QTimer());

#if 0
      //do not send too much updates
      connect(dragging.get(),&QTimer::timeout,[this](){
        model->endTransaction();
        model->beginTransaction();
      });
      dragging->start(500);
#endif

      auto pos = QUtils::convert<Point2d>(unproject(evt->pos()));
      drawLine(pos,pos);
      last_pos = pos;
    }
    else
    {
      QCanvas2d::mousePressEvent(evt);
    }
    
    update();
  }

  //mouseMoveEvent
  virtual void mouseMoveEvent(QMouseEvent* evt) override 
  {
    if (dragging)
    {
      auto pos=QUtils::convert<Point2d>(unproject(evt->pos()));
      drawLine(last_pos, pos);
      last_pos = pos;
    }
    else
    {
      QCanvas2d::mouseMoveEvent(evt);
    }

    //I need to show the explanation
    update();
  }

  //mouseReleaseEvent
  virtual void mouseReleaseEvent(QMouseEvent* evt) override
  {
    if (dragging)
    {
      model->endTransaction();
      dragging.reset();
    }
    else
    {
      QCanvas2d::mouseReleaseEvent(evt);
    }

    update();
  }

  //paintEvent
  virtual void paintEvent(QPaintEvent* evt) override
  {
    if (!model) 
      return;

    QPainter painter(this);
    renderBackground(painter);
    renderGrid(painter);

    int nsamples=model->getNumberOfSamples();

    if (bool bRenderFunctions=true)
    {
      std::vector<Color> colors = {Colors::Red,Colors::Green,Colors::Blue,Colors::Gray};
      std::vector< SharedPtr<SingleTransferFunction> > functions = { model->R,model->G,model->B,model->A };

      for (int F=0;F<4;F++)
      {
        std::vector<QPointF> points(nsamples);

        for (int I = 0; I < nsamples; I++)
        {
          double x=I/(double)(nsamples -1);
          double y=functions[F]->values[I];
          points[I] = project(QPointF(x,y));
        }

        painter.setPen(QPen(QUtils::convert<QColor>(colors[F]),selection->isSelected(F)?2:1));
        painter.drawPolyline(&points[0],(int)points.size());
      }
    }

    if (bool bRenderExplanations=true)
    {
      auto pos      = getCurrentPos();
      auto screenpos= project(pos);

      painter.setPen(QColor(0,0,0,120));
      painter.drawLine(QPointF(screenpos[0],0), QPointF(screenpos[0],this->height()));
      painter.drawLine(QPointF(0,screenpos[1]), QPointF(this->width(),screenpos[1]));

      int    x=Utils::clamp((int)round(pos[0]*(nsamples -1)),0, nsamples -1);
      double y=pos[1];
      painter.drawText(QPoint(2,this->height()-12),cstring("x",(int)x,"y",y).c_str());
    }

    renderBorders(painter);
  }

};

////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API TransferFunctionTextEditView :
  public QFrame,
  public View<TransferFunction>
{
public:

  VISUS_NON_COPYABLE_CLASS(TransferFunctionTextEditView)

  class Widgets
  {
  public:
    QTextEdit*   textedit=nullptr;
    QToolButton* btParse=nullptr;
    QToolButton* btOpen=nullptr;
    QToolButton* btSave=nullptr;
  };
  Widgets widgets;

  //constructor
  TransferFunctionTextEditView(TransferFunction* model=nullptr) {
    if (model)
      bindModel(model);
  }

  //destructor
  virtual ~TransferFunctionTextEditView() {
    bindModel(nullptr);
  }

  //bindModel
  virtual void bindModel(TransferFunction* model) override
  {
    if (this->model)
    {
      QUtils::clearQWidget(this);
      widgets = Widgets();
    }

    View<ModelClass>::bindModel(model);

    if (this->model)
    {
      QVBoxLayout* layout = new QVBoxLayout();

      layout->addWidget(widgets.textedit= GuiFactory::CreateTextEdit());

      auto buttons=(new QHBoxLayout());

      buttons->addWidget(widgets.btParse=GuiFactory::CreateButton("Parse"  ,[this](bool){doParse();}));
      buttons->addWidget(widgets.btSave =GuiFactory::CreateButton("Save...",[this](bool){doSave ();}));
      buttons->addWidget(widgets.btOpen =GuiFactory::CreateButton("Open...",[this](bool){doOpen ();}));
      layout->addLayout(buttons);

      setLayout(layout);

      refreshGui();
    }
  }

private:

  //refreshGui
  void refreshGui() {

    StringTree out("TransferFunction");
    model->write(out);
    auto content= out.toXmlString();
    widgets.textedit->setText(content.c_str());
  }

  //doParse
  void doParse()
  {
    if (!this->model)
      return;

    String content = cstring(widgets.textedit->toPlainText());
    TransferFunction::copy(*model,*TransferFunction::fromString(content));
  }

  //doOpen
  void doOpen() 
  {
    if (!this->model)
      return;

    static String last_filename = "";
    String filename = cstring(QFileDialog::getOpenFileName(nullptr,"Choose a file to open...", last_filename.c_str(),"*.*"));
    if (filename.empty()) return;
    last_filename = filename;
    filename = StringUtils::replaceAll(filename, "\\", "/");

    String content = Utils::loadTextDocument(filename);
    if (content.empty())
    {
      String errormsg=cstring("Failed to open file",filename);
      QMessageBox::information(this,"Error",errormsg.c_str());
      return;
    }
  
    //special case for Visit transfer function
    StringTree in=StringTree::fromString(content);
    if (in.valid() && in.name=="Object")
    {
      PrintInfo("Loading Visit transfer function");
    
      RGBAColorMap rgba_colormap;
      if (auto controlpoints= in.getChild("Object/Object"))
      {
        for (auto controlpoint : controlpoints->getChilds())
        {
          auto fields= controlpoint->getChilds("Field");

          if (fields.size()!=2) 
            continue;

          VisusAssert(fields[0]->readString("name")=="colors");
          VisusAssert(fields[1]->readString("name")=="position");
          String s_color, s_position;
          fields[0]->readText(s_color);
          fields[1]->readText(s_position);
          auto color  = Color::fromString(s_color);
          double position=cdouble(s_position);
          rgba_colormap.setColorAt(position, color);
        }
      }

      const int nsamples=256;
      Array src=rgba_colormap.toArray(nsamples);

      double attenuation=1.0;
      bool useColorVarMin=false; double colorVarMin=0;
      bool useColorVarMax=false; double colorVarMax=0;
      bool isFloatRange=false;
    
      for (auto child : in.getChilds())
      {
        String name,text; 
        child->read("name", name);
        child->readText(text);

        if (name=="freeformOpacity")
        {
          auto alpha=GetComponentSamples<Uint8>(src,3);
          std::istringstream istream(text);
          for (int I=0;I< nsamples;I++) {
            int val; 
            istream >> val;
            alpha[I] = (Uint8) val;
          }
        }
        else if (name=="opacityAttenuation") 
          attenuation    = cdouble(text);

        else if (name=="colorVarMin")
        {
          isFloatRange = child->readString("type")=="float";
          colorVarMin  = cdouble(text);
        }
        else if (name=="colorVarMax"       ) colorVarMax    = cdouble(text);
        else if (name=="useColorVarMax"    ) useColorVarMax = cbool(text);
        else if (name=="useColorVarMin"    ) useColorVarMin = cbool(text);
      }

      auto other=TransferFunction::fromArray(src);
      if (useColorVarMax || useColorVarMin)
      {
        other->setNormalizationMode(TransferFunction::UserRange);
        other->setUserRange(Range(colorVarMin, colorVarMax, 0));
      }
      other->setAttenutation(attenuation);

      TransferFunction::copy(*this->model,*other);
    }
    else
    {
      auto other = TransferFunction::fromString(content);
      if (!other)
      {
        VisusAssert(false);
        return;
      }
      TransferFunction::copy(*this->model,*other);
    }
  }

  //doSave
  void doSave()
  {
    if (!this->model)
      return;

    String content = cstring(widgets.textedit->toPlainText());
    static String last_filename = "";
    String filename = cstring(QFileDialog::getOpenFileName(nullptr,"Choose a file to save...", last_filename.c_str(),"*.*"));
    if (filename.empty()) return;
    last_filename = filename;
    filename = StringUtils::replaceAll(filename, "\\", "/");
      
    try
    {
      Utils::saveTextDocument(filename, content);
    }
    catch(...){
      String errormsg=cstring("Failed to save file",filename);
      QMessageBox::information(this,"Error",errormsg.c_str());
      return;
    }
  }

  //modelChanged
  virtual void modelChanged() override
  {
    refreshGui();
  }

};

////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API TransferFunctionInputView :
  public QFrame,
  public View<TransferFunction>
{
public:

  VISUS_NON_COPYABLE_CLASS(TransferFunctionInputView)

    //___________________________________________________
    class Widgets
  {
  public:
    QComboBox* normalization_mode = nullptr;
    QLineEdit* user_range_from = nullptr;
    QLineEdit* user_range_to = nullptr;
  };

  Widgets widgets;

  //constructor
  TransferFunctionInputView(TransferFunction* model = nullptr) {
    if (model)
      bindModel(model);
  }

  //destructor
  virtual ~TransferFunctionInputView() {
    bindModel(nullptr);
  }

  //bindModel
  virtual void bindModel(TransferFunction* value) override
  {
    if (this->model)
    {
      QUtils::clearQWidget(this);
      widgets = Widgets();
    }

    View<ModelClass>::bindModel(value);

    if (this->model)
    {
      auto layout = new QHBoxLayout();

      std::vector<String> options = { "Use field range (if exists)", "Compute dynamic range per component", "Compute all components dynamic range", "Use custom range" };
      layout->addWidget(new QLabel("Palette range"));
      layout->addWidget(widgets.normalization_mode = GuiFactory::CreateComboBox(options[0], options, [this](String value) {
        updateInputNormalizationMode();
        }));

      layout->addWidget(widgets.user_range_from = GuiFactory::CreateDoubleTextBoxWidget(model->getUserRange().from, [this](double value) {
        auto From = cdouble(widgets.user_range_from->text());
        auto To = cdouble(widgets.user_range_to->text());
        model->setUserRange(Range(From, To, 0.0));
        }));

      layout->addWidget(widgets.user_range_to = GuiFactory::CreateDoubleTextBoxWidget(model->getUserRange().to, [this](double value) {
        auto From = cdouble(widgets.user_range_from->text());
        auto To = cdouble(widgets.user_range_to->text());
        model->setUserRange(Range(From, To, 0.0));
        }));

      setLayout(layout);
      refreshGui();
    }
  }

private:

  //updateInputNormalizationMode
  void updateInputNormalizationMode()
  {
    int value = widgets.normalization_mode->currentIndex();
    model->setNormalizationMode(value);
    widgets.user_range_from->setEnabled(value == TransferFunction::UserRange);
    widgets.user_range_to->setEnabled(value == TransferFunction::UserRange);
  }

  //refreshGui
  void refreshGui()
  {
    widgets.normalization_mode->setCurrentIndex(model->getNormalizationMode());
    widgets.user_range_from->setText(cstring(model->getUserRange().from).c_str());
    widgets.user_range_to  ->setText(cstring(model->getUserRange().to).c_str());
    widgets.user_range_from->setEnabled(model->getNormalizationMode() == TransferFunction::UserRange);
    widgets.user_range_to->setEnabled(model->getNormalizationMode() == TransferFunction::UserRange);
  }

  //modelChanged
  virtual void modelChanged() override {
    refreshGui();
  }
};


////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API TransferFunctionView :
  public QFrame,
  public View<TransferFunction>
{

public:

  VISUS_NON_COPYABLE_CLASS(TransferFunctionView)


  //____________________________________________________
  class Widgets
  {
  public:
    TransferFunctionColorBarView*             colorbar=nullptr;
    TransferFunctionSelectedFunctionsView*    selection=nullptr;
    QComboBox*                                default_palette=nullptr;
    QToolButton*                              btImport=nullptr;
    QToolButton*                              btExport=nullptr;
    QDoubleSlider*                            attenuation=nullptr;
    QCheckBox*                                show_checkboard=nullptr;
    TransferFunctionCanvasView*               canvas=nullptr;
    TransferFunctionInputView*                input=nullptr;
    ArrayStatisticsView*                      statistics=nullptr;
  };

  Widgets widgets;

  //default constructor
  TransferFunctionView(TransferFunction* model=nullptr) {
    if (model)
      bindModel(model);
  }

  //destructor
  virtual ~TransferFunctionView() {
    bindModel(nullptr);
  }

  //bindModel
  virtual void bindModel(TransferFunction* model) override
  {
    if (this->model)
    {
      QUtils::clearQWidget(this);
      widgets=Widgets();
    }

    View<ModelClass>::bindModel(model);

    if (this->model)
    {
      auto layout=new QVBoxLayout();

      layout->addWidget(widgets.colorbar=new TransferFunctionColorBarView(this->model));

      //second row (list of palettes, Show alpha, import, export)
      {
        auto row=new QHBoxLayout();

        row->addWidget(new QLabel("Set default"));
        {
          auto defaults = TransferFunction::getDefaults();
          auto combo=GuiFactory::CreateComboBox(defaults[0], defaults,[this](String name){
            this->model->setDefault(name); 
          });
          combo->setCurrentText(this->model->getDefaultName().c_str()); 
          row->addWidget(widgets.default_palette=combo);
        }

        row->addWidget(new QLabel("Set opacity"));
        {
          auto defaults = TransferFunction::getDefaultOpacities();
          auto combo = GuiFactory::CreateComboBox(defaults[0], defaults, [this](String name) {
            this->model->setOpacity(name); 
            });
          combo->setCurrentText(this->model->getDefaultName().c_str());
          row->addWidget(widgets.default_palette = combo);
        }

        row->addWidget(widgets.show_checkboard=GuiFactory::CreateCheckBox(true,"Show alpha",[this](int value){
          setShowCheckboard(value);
        }));

        row->addWidget(widgets.btImport=GuiFactory::CreateButton("Import",[this](bool){
          importTransferFunction();
        }));

        row->addWidget(widgets.btExport=GuiFactory::CreateButton("Export",[this](bool){
          exportTransferFunction();
        }));

        layout->addLayout(row);
      }

      layout->addWidget(widgets.selection=new TransferFunctionSelectedFunctionsView(this->model));
      layout->addWidget(widgets.canvas = new TransferFunctionCanvasView(this->model, widgets.selection));
      layout->addWidget(widgets.input = new TransferFunctionInputView(this->model));
      layout->addWidget(widgets.statistics = new ArrayStatisticsView());

      setLayout(layout);
    }
  }

  //setStatistics
  void setStatistics(const Statistics& value)  
  {
    widgets.statistics->setStatistics(value);

    for (auto it : widgets.statistics->widgets.components)
    {
      if (auto histogram_view=it.histogram)
      {
        connect(histogram_view, &HistogramView::selectedRegionChanged, [this](Range range) {
          model->setUserRange(range);
          model->setNormalizationMode(TransferFunction::UserRange);
        });

#if 0
        auto sync=[](QCanvas2d* dst, QCanvas2d* src)
        {
          dst->blockSignals(true);
          dst->setCurrentPos(src->getCurrentPos());
          dst->blockSignals(false);
        };
        connect(widgets.canvas,&QCanvas2d::postRedisplay,[this,histogram_view,sync](){ sync(histogram_view,widgets.canvas);});
        connect(histogram_view,&QCanvas2d::postRedisplay,[this,histogram_view,sync](){ sync(widgets.canvas,histogram_view);});
#endif
      }
    }
  }

  //setShowCheckboard
  void setShowCheckboard(int value) {
    widgets.colorbar->showCheckboard(value?true:false);
    update();
  }

  //importTransferFunction
  void importTransferFunction()
  {
    String url=cstring(QFileDialog::getOpenFileName(nullptr,"Choose a transfer function to import...","","*.transfer_function"));
    if (!url.empty())
      getModel()->importTransferFunction(url);
  }

  //exportTransferFunctioN
  void exportTransferFunction()
  {
    String filename=cstring(QFileDialog::getSaveFileName(nullptr,"Choose file in which to export...","","*.transfer_function"));
    if (!filename.empty())
      getModel()->exportTransferFunction(filename);
  }

};

} //namespace Visus

#endif //VISUS_TRANSFER_FUNCTION_VIEW_H
