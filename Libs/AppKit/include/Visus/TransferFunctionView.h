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

#include <Visus/AppKit.h>
#include <Visus/Gui.h>
#include <Visus/TransferFunction.h>
#include <Visus/Model.h>
#include <Visus/DoubleSlider.h>
#include <Visus/GuiFactory.h>
#include <Visus/StringUtils.h>
#include <Visus/Canvas.h>
#include <Visus/ArrayStatisticsView.h>

#include <QApplication>
#include <QFontDatabase>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QWheelEvent>

namespace Visus {

////////////////////////////////////////////////////////////////////////
class VISUS_APPKIT_API TransferFunctionColorBarView :
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

    int nfunctions = (int)model->functions.size();
    if (!(nfunctions>=1 && nfunctions<=4))
      return;

    int nsamples = (int)model->size();
    if (!nsamples)
      return;

    double attenuation=model->attenuation;

    std::vector<double>* R=(nfunctions>=1)? &model->functions[0]->values : nullptr;
    std::vector<double>* G=(nfunctions>=2)? &model->functions[1]->values : nullptr;
    std::vector<double>* B=(nfunctions>=3)? &model->functions[2]->values : nullptr;
    std::vector<double>* A=nullptr;
    if (nfunctions==2 && bShowCheckedBoard) A=&model->functions[1]->values; //Gray Alpha
    if (nfunctions==4 && bShowCheckedBoard) A=&model->functions[3]->values; //RGBA

    this->img.reset(new QImage(nsamples,1,QImage::Format_ARGB32));
    for (int x=0; x<nsamples; x++)
    {
      auto r  = R? (Uint8)((*R)[x] * 255.0                      ) : 0;
      auto g  = G? (Uint8)((*G)[x] * 255.0                      ) : r;
      auto b  = B? (Uint8)((*B)[x] * 255.0                      ) : g;
      auto a  = A? (Uint8)((*A)[x] * 255.0 * (1.0 - attenuation)) : 255;
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
class VISUS_APPKIT_API TransferFunctionSelectedFunctionsView :
  public QFrame,
  public View<TransferFunction>
{
  Q_OBJECT

public:

  VISUS_NON_COPYABLE_CLASS(TransferFunctionSelectedFunctionsView)

  std::vector<QCheckBox*> checkboxes;

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
      checkboxes.clear();
    }

    View<ModelClass>::bindModel(model);

    if (this->model)
    {
      auto layout = new QHBoxLayout();

      for (int F = 0,nfunctions = (int)model->functions.size(); F < (int)nfunctions; F++)
      {
        auto checkbox=GuiFactory::CreateCheckBox(F==nfunctions-1?true:false,model->functions[F]->name.c_str(),[this,F](int) {checkBoxClicked(F);});
        layout->addWidget(checkbox);
        checkboxes.push_back(checkbox);
      }

      setLayout(layout);
    }
  }

  //isSelected
  bool isSelected(TransferFunction::Single* fn)
  {
    int F=0; 
    for (auto it : model->functions) 
    {
      if (it.get()==fn)
        return checkboxes[F]->isChecked();
      F++;
    }
    return false;
  }

signals:

  void selectionChanged();

private:

  //checkBoxClicked
  void checkBoxClicked(int index)
  {
    if (!(QApplication::keyboardModifiers() & Qt::ShiftModifier))
    { 
      int I=0;for (auto checkbox : checkboxes)
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
    std::vector<String> fn_names;
    for (auto fn : model->functions)
      fn_names.push_back(fn->name);

    std::vector<String> check_names;
    for (auto checkbox : checkboxes)
      check_names.push_back(cstring(checkbox->text()));

    if (fn_names!=check_names)
      rebindModel();
  }

};

////////////////////////////////////////////////////////////////////////
class VISUS_APPKIT_API TransferFunctionCanvasView :
  public Canvas,
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

  //mousePressEvent
  virtual void mousePressEvent(QMouseEvent* evt) override 
  {
    // painting functions
    if (model && evt->button()==Qt::LeftButton && model->functions.size())
    { 
      model->beginUpdate();

      //do not send too much updates
      dragging.reset(new QTimer());
      connect(dragging.get(),&QTimer::timeout,[this](){
        model->endUpdate();
        model->beginUpdate(); 
      });
      dragging->start(500);

      auto pos=QUtils::convert<Point2d>(unproject(evt->pos()));
      for (auto fn : model->functions) {
        if (selection->isSelected(fn.get()))
          fn->setValue(pos.x,pos.y);
      }

      model->setNotDefault();
      last_pos = pos;
    }
    else
    {
      Canvas::mousePressEvent(evt);
    }
    
    update();
  }

  //mouseMoveEvent
  virtual void mouseMoveEvent(QMouseEvent* evt) override 
  {
    if (dragging)
    {
      auto pos=QUtils::convert<Point2d>(unproject(evt->pos()));
      for (auto fn : model->functions) {
        if (selection->isSelected(fn.get()))
          fn->setValue(last_pos.x,last_pos.y,pos.x,pos.y);
      }
      model->setNotDefault();
      last_pos = pos;
    }
    else
    {
      Canvas::mouseMoveEvent(evt);
    }

    //I need to show the explanation
    update();
  }

  //mouseReleaseEvent
  virtual void mouseReleaseEvent(QMouseEvent* evt) override
  {
    if (dragging)
    {
      model->endUpdate();
      dragging.reset();
    }
    else
    {
      Canvas::mouseReleaseEvent(evt);
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

    int nvalues=(int)model->size();

    if (bool bRenderFunctions=true)
    {
      for (auto fn : model->functions)
      {
        std::vector<QPointF> points(nvalues);

        for (int I = 0; I < nvalues; I++)
        {
          double x=I/(double)(nvalues-1);
          double y=fn->values[I];
          points[I] = project(QPointF(x,y));
        }

        painter.setPen(QPen(QUtils::convert<QColor>(fn->color),selection->isSelected(fn.get())?2:1));
        painter.drawPolyline(&points[0],(int)points.size());
      }
    }

    if (bool bRenderExplanations=true)
    {
      auto pos      = getCurrentPos();
      auto screenpos= project(pos);

      painter.setPen(QColor(0,0,0,120));
      painter.drawLine(QPointF(screenpos.x,0), QPointF(screenpos.x,this->height()));
      painter.drawLine(QPointF(0,screenpos.y), QPointF(this->width(),screenpos.y));

      int    x=Utils::clamp((int)round(pos.x*(nvalues-1)),0,nvalues-1);
      double y=pos.y;
      painter.drawText(QPoint(2,this->height()-12),(StringUtils::format()<<"x("<<(int)x<<") y("<<y<<")").str().c_str());
    }

    renderBorders(painter);
  }

};

////////////////////////////////////////////////////////////////////////
class VISUS_APPKIT_API TransferFunctionTextEditView :
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
      this->encoder=std::make_shared<XmlEncoder>();

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

  SharedPtr<XmlEncoder> encoder = std::make_shared<XmlEncoder>();

  //refreshGui
  void refreshGui() {
    widgets.textedit->setText(encoder->encode(model).c_str());
  }

  //doParse
  void doParse()
  {
    if (!this->model)
      return;

    String content = cstring(widgets.textedit->toPlainText());
    SharedPtr<TransferFunction> parsed = std::dynamic_pointer_cast<TransferFunction>(encoder->decode(content));
    if (!parsed)
    {
      VisusAssert(false);
      return;
    }

    TransferFunction::copy(*this->model,*parsed);
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
      String errormsg=StringUtils::format()<<"Failed to open file "<<filename;
      QMessageBox::information(this,"Error",errormsg.c_str());
      return;
    }
  
    //special case for Visit transfer function
    StringTree stree;
    if (stree.loadFromXml(content) && stree.name=="Object")
    {
      VisusInfo()<<"Loading Visit transfer function";
    
      RGBAColorMap rgba_colormap;
      if (StringTree* controlpoints=stree.findChildWithName("Object/Object"))
      {
        for (int I=0;I<controlpoints->getNumberOfChilds();I++)
        {
          std::vector<StringTree*> fields=controlpoints->getChild(I).findAllChildsWithName("Field",false);

          if (fields.size()!=2) continue;

          VisusAssert(fields[0]->readString("name")=="colors");
          VisusAssert(fields[1]->readString("name")=="position");
          String colors  =        fields[0]->collapseTextAndCData();
          double position=cdouble(fields[1]->collapseTextAndCData());
          rgba_colormap.points.push_back(RGBAColorMap::Point(position, Color::parseFromString(colors)));
        }
        rgba_colormap.refreshMinMax();
      }
      VisusAssert(!rgba_colormap.points.empty());

      const int N=256;
      Array src;
      rgba_colormap.convertToArray(src, N);

      double attenuation=1.0;
      bool useColorVarMin=false; double colorVarMin=0;
      bool useColorVarMax=false; double colorVarMax=0;
      bool isFloatRange=false;
    
      for (int I=0;I<stree.getNumberOfChilds();I++)
      {
        StringTree& child=stree.getChild(I);
        String name=child.readString("name");
        String text=child.collapseTextAndCData();

        if (name=="freeformOpacity")
        {
          auto alpha=GetComponentSamples<Uint8>(src,3);
          std::istringstream istream(text);
          for (int I=0;I<N;I++) {
            int val; istream >> val;
            alpha[I] = (Uint8) val;
          }
        }
        else if (name=="opacityAttenuation") attenuation    = cdouble(text);
        else if (name=="colorVarMin")
        {
          isFloatRange = (child.readString("type")=="float");
          colorVarMin  = cdouble(text);
        }
        else if (name=="colorVarMax"       ) colorVarMax    = cdouble(text);
        else if (name=="useColorVarMax"    ) useColorVarMax = cbool(text);
        else if (name=="useColorVarMin"    ) useColorVarMin = cbool(text);
      }

      {
        this->model->beginUpdate();
        this->model->setFromArray(src,"");
        this->model->output_dtype=isFloatRange? DTypes::FLOAT32_RGBA : DTypes::UINT8_RGBA;
        this->model->attenuation=attenuation;
        if (useColorVarMax || useColorVarMin)
          this->model->input_normalization=TransferFunction::InputNormalization(ComputeRange::UseCustom,Range(colorVarMin, colorVarMax,0));
        this->model->endUpdate();
      }
      return;
    }
  
    SharedPtr<TransferFunction> decoded = std::dynamic_pointer_cast<TransferFunction>(encoder->decode(content));
    if (!decoded)
    {
      VisusAssert(false);
      return;
    }

    TransferFunction::copy(*this->model,*decoded);
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
      
    if (!Utils::saveTextDocument(filename, content))
    {
      String errormsg=StringUtils::format()<<"Failed to save file "<<filename;
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
class VISUS_APPKIT_API TransferFunctionInputView :
  public QFrame,
  public View<TransferFunction>
{
public:

  VISUS_NON_COPYABLE_CLASS(TransferFunctionInputView)

  //___________________________________________________
  class Widgets
  {
  public:
    QLineEdit*  nsamples=nullptr;
    struct 
    {
      struct 
      {
        QComboBox* mode=nullptr;
        struct 
        {
          QLineEdit *from=nullptr;
          QLineEdit *to=nullptr;
        } 
        custom_range;
      }
      normalization;
    }
    input;
  };
  Widgets widgets;

  //constructor
  TransferFunctionInputView(TransferFunction* model=nullptr) {
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
      QFormLayout* layout = new QFormLayout();

      auto normalization = model->input_normalization;
      
      layout->addRow("Num samples", widgets.nsamples = GuiFactory::CreateIntegerTextBoxWidget((int)model->size(),[this](int nsamples){
        model->beginUpdate();
        for (auto fn : model->functions)
          fn->resize(nsamples);
        model->endUpdate();
      }));

      std::vector<String> options={"Use Array Range","Compute Overall Range","Compute Range Per Component","Use Custom Range"};
      layout->addRow("Normalization mode", widgets.input.normalization.mode = GuiFactory::CreateComboBox(options[0],options,[this](String value) {
        auto normalization = model->input_normalization;
        normalization.mode = (ComputeRange::Mode)widgets.input.normalization.mode->currentIndex();
        model->beginUpdate();
        model->input_normalization=normalization;
        model->endUpdate();
      }));
      
      layout->addRow("Custom Range from" , widgets.input.normalization.custom_range.from = GuiFactory::CreateDoubleTextBoxWidget(normalization.custom_range.from,[this](double value){
        auto normalization = model->input_normalization;
        normalization.custom_range.from = cdouble(widgets.input.normalization.custom_range.from->text());
        model->beginUpdate();
        model->input_normalization=normalization;
        model->endUpdate();
      }));
      widgets.input.normalization.custom_range.from->setEnabled(normalization.mode == ComputeRange::UseCustom);

      layout->addRow("Custom Range to" , widgets.input.normalization.custom_range.to = GuiFactory::CreateDoubleTextBoxWidget(normalization.custom_range.to,[this](double value){
        auto normalization = model->input_normalization;
        normalization.custom_range.to = cdouble(widgets.input.normalization.custom_range.to->text());
        model->beginUpdate();
        model->input_normalization = normalization;
        model->endUpdate();
      }));
      widgets.input.normalization.custom_range.to->setEnabled(normalization.mode == ComputeRange::UseCustom);

      setLayout(layout);
      //refreshGui();
    }
  }

private:

  //refreshGui
  void refreshGui()
  {
    auto normalization = model->input_normalization;
    widgets.nsamples->setText(cstring(model->size()).c_str());
    widgets.input.normalization.mode->setCurrentIndex(normalization.mode);
    widgets.input.normalization.custom_range.from->setText(cstring(normalization.custom_range.from).c_str());
    widgets.input.normalization.custom_range.to->setText(cstring(normalization.custom_range.to).c_str());
    widgets.input.normalization.custom_range.from->setEnabled(normalization.mode==ComputeRange::UseCustom);
    widgets.input.normalization.custom_range.to->setEnabled  (normalization.mode==ComputeRange::UseCustom);
  }

  //modelChanged
  virtual void modelChanged() override {
    refreshGui();
  }
};

////////////////////////////////////////////////////////////////////////
class VISUS_APPKIT_API TransferFunctionOutputView :
  public QFrame,
  public View<TransferFunction>
{
public:

  VISUS_NON_COPYABLE_CLASS(TransferFunctionOutputView)

  class Widgets
  {
  public:
    QLineEdit*            nfunctions=nullptr;
    QComboBox*            dtype=nullptr;
    QLineEdit*            range_from=nullptr;
    QLineEdit*            range_to=nullptr;
    DoubleSlider*        attenuation=nullptr;
  };

  Widgets widgets;

  //constructor
  TransferFunctionOutputView(TransferFunction* model) {
    if (model)
      bindModel(model);
  }

  //destructor
  virtual ~TransferFunctionOutputView() {
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
      QFormLayout* layout = new QFormLayout();

      layout->addRow("Number functions", widgets.nfunctions= GuiFactory::CreateIntegerTextBoxWidget((int)model->functions.size(),[this](int value){
          this->model->setNumberOfFunctions(value);
      }));

      layout->addRow("Output dtype", widgets.dtype = GuiFactory::CreateComboBox(model->output_dtype.toString().c_str(),{"Uint8","Float32","Float64"},[this](String s){
          DType value=DType::fromString(s);
          this->model->beginUpdate();
          this->model->output_dtype=value;
          this->model->endUpdate();
          update();
      }));

      layout->addRow("Range from", widgets.range_from = GuiFactory::CreateDoubleTextBoxWidget(model->output_range.from,[this](double value){

        this->model->beginUpdate();
          this->model->output_range=Range(value,model->output_range.to,0);
          this->model->endUpdate();
      }));

      layout->addRow("Range to", widgets.range_to = GuiFactory::CreateDoubleTextBoxWidget(model->output_range.to,[this](double value){
        this->model->beginUpdate();
          this->model->output_range = Range(model->output_range.from,value,0);
          this->model->endUpdate();
        }));

      layout->addRow("Attenuation",widgets.attenuation=GuiFactory::CreateDoubleSliderWidget(model->attenuation,Range(0,1,0),[this](double value){
        this->model->beginUpdate();
        this->model->attenuation=value;
        this->model->endUpdate();
      }));

      setLayout(layout);
      //refreshGui();
    }
  }

private:

  //refreshGui
  void refreshGui()
  {
    widgets.nfunctions->setText(cstring((int)model->functions.size()).c_str());
    widgets.dtype->setCurrentText(model->output_dtype.toString().c_str());
    widgets.range_from->setText(cstring(model->output_range.from).c_str());
    widgets.range_to->setText(cstring(model->output_range.to).c_str());
    widgets.attenuation->setValue(model->attenuation);
  }

  //modelChanged
  virtual void modelChanged() override {
    refreshGui();
  }

};

////////////////////////////////////////////////////////////////////////
class VISUS_APPKIT_API TransferFunctionView :
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
    QComboBox*                                default_interpolation=nullptr;
    QComboBox*                                default_palette=nullptr;
    QToolButton*                              btImport=nullptr;
    QToolButton*                              btExport=nullptr;
    DoubleSlider*                             attenuation=nullptr;
    QCheckBox*                                show_checkboard=nullptr;
    TransferFunctionCanvasView*               canvas=nullptr;
    TransferFunctionTextEditView*             text=nullptr;
    TransferFunctionInputView*                input=nullptr;
    TransferFunctionOutputView*               output=nullptr;
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
      QVBoxLayout* layout=new QVBoxLayout();
      
      //color bar
      layout->addWidget(widgets.colorbar=new TransferFunctionColorBarView(this->model));

      //second row (list of palettes, Show alpha, import, export)
      {
        auto row=(new QHBoxLayout());

        row->addWidget(new QLabel("Set default"));
        {
          auto combo=GuiFactory::CreateComboBox(TransferFunction::getDefaults()[0],TransferFunction::getDefaults(),[this](String name){
            this->model->setDefault(name);
          });
          combo->setCurrentText(this->model->default_name.c_str()); 
          row->addWidget(widgets.default_palette=combo);
        }

        row->addWidget(new QLabel("Interpolation"));
        {
          auto values = InterpolationMode::getValues();
          auto combo = GuiFactory::CreateComboBox(values[0], values, [this](String type) {
            this->model->beginUpdate();
            this->model->interpolation.set(type);
            this->model->endUpdate();
          });
          combo->setCurrentText(this->model->interpolation.toString().c_str());
          row->addWidget(widgets.default_interpolation = combo);
        }

        row->addWidget(widgets.show_checkboard=GuiFactory::CreateCheckBox(true,"Show alpha",[this](int value){showCheckboard(value);}));

        row->addWidget(widgets.btImport=GuiFactory::CreateButton("Import",[this](bool){
          importTransferFunction();
        }));

        row->addWidget(widgets.btExport=GuiFactory::CreateButton("Export",[this](bool){
          exportTransferFunction();
        }));

        layout->addLayout(row);
      }

      layout->addWidget(widgets.selection=new TransferFunctionSelectedFunctionsView(this->model));
      layout->addWidget(widgets.canvas=new TransferFunctionCanvasView(this->model,widgets.selection));
      
      //text output
      QTabWidget* tabs=new QTabWidget();
      tabs->addTab(widgets.input=new TransferFunctionInputView(this->model),"INPUT");
      tabs->addTab(widgets.output=new TransferFunctionOutputView(this->model),"OUTPUT");
      tabs->addTab(widgets.statistics=new ArrayStatisticsView(),"STATISTICS");
      tabs->addTab(widgets.text=new TransferFunctionTextEditView(this->model),"TEXT");
      layout->addWidget(tabs);

      setLayout(layout);
    }
  }

  //setStatistics
  void setStatistics(const Statistics& stats)  
  {
    if (widgets.statistics)
    {
      widgets.statistics->setStatistics(stats);

      for (auto it : widgets.statistics->widgets.components)
      {
        if (auto histogram_view=it.histogram)
        {
          auto sync=[](Canvas* dst,Canvas* src) 
          {
            dst->blockSignals(true);
            //dst->setOrthoParams(src->getOrthoParams());
            dst->setCurrentPos(src->getCurrentPos());
            dst->blockSignals(false);
          };

          connect(widgets.canvas,&Canvas::repaintNeeded,[this,histogram_view,sync](){ sync(histogram_view,widgets.canvas);});
          connect(histogram_view,&Canvas::repaintNeeded,[this,histogram_view,sync](){ sync(widgets.canvas,histogram_view);});
        }
      }
    }
  }

  //showCheckboard
  void showCheckboard(int value) {
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

typedef TransferFunctionView PaletteView;

} //namespace Visus

#endif //VISUS_TRANSFER_FUNCTION_VIEW_H
