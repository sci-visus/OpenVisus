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

#ifndef VISUS_GUI_FACTORY_H__
#define VISUS_GUI_FACTORY_H__

#include <Visus/Gui.h>
#include <Visus/GLMaterial.h>
#include <Visus/Point.h>
#include <Visus/Box.h>
#include <Visus/QDoubleSlider.h>

#include <functional>

#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QSlider>
#include <QColorDialog>
#include <QComboBox>
#include <QBoxLayout>
#include <QFormLayout>
#include <QTextEdit>
#include <QFontDatabase>
#include <QToolButton>
#include <QMenu>
#include <QToolBar>

namespace Visus {

////////////////////////////////////////////////////////////////////////
namespace GuiFactory
{

  //CreateAction
  inline QAction* CreateAction(String name, QObject* parent, std::function<void()> action_callback = std::function<void()>()) {
    auto action = new QAction(name.c_str(), parent);
    QObject::connect(action, &QAction::triggered, action_callback);
    return action;
  }

  //CreateAction
  inline QAction* CreateAction(String name, QObject* parent, QIcon icon, std::function<void()> action_callback = std::function<void()>()) {
    auto action = CreateAction(name,parent,action_callback);
    action->setIcon(icon);
    return action;
  }

  //CreateMenu
  inline QMenu* CreateMenu(QWidget* parent_widget,std::vector<QAction*> actions)
  {
    auto ret=new QMenu(parent_widget);
    
    for (auto it : actions)
      ret->addAction(it);

    return ret;
  }

  //CreateButton
  inline QToolButton* CreateButton(QIcon icon,String text,std::function<void(bool)> clicked=std::function<void(bool)>())
  {
    auto ret=new QToolButton();

    if (!icon.isNull())
      ret->setIcon(icon);

    if (!text.empty())
      ret->setText(text.c_str());

    if (clicked)
      QObject::connect(ret,&QToolButton::clicked,clicked);

    return ret;
  }

  //CreateButton
  inline QToolButton* CreateButton(String text,std::function<void(bool)> clicked=std::function<void(bool)>()) {
    return CreateButton(QIcon(),text,clicked);
  }

  //CreateCheckBox
  inline QCheckBox* CreateCheckBox(bool value,String text,std::function<void(int)> callback=std::function<void(int)>())
  {
    auto ret=new QCheckBox();
    ret->setChecked(value);
    if (!text.empty())
      ret->setText(text.c_str());
    if (callback)
      QCheckBox::connect(ret,&QCheckBox::stateChanged,callback);
    return ret;
  }


  //CreateComboBox
  inline QComboBox* CreateComboBox(String value, std::vector<String> options, std::function<void(String)> callback = std::function<void(String)>())
  {
    auto ret = new QComboBox();
    ret->setEditable(false);

    for (auto it : options)
      ret->addItem(it.c_str());

    ret->setCurrentText(value.c_str());

    if (callback)
    {
      QComboBox::connect(ret, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [ret, callback](int index) {
        callback(cstring(ret->currentText()));
      });
    }

    return ret;
  }

  //CreateIntegerComboBoxWidget
  inline QComboBox* CreateIntegerComboBoxWidget(int value, std::map<int, String> options, std::function<void(int)> callback = std::function<void(int)>())
  {
    auto ret = new QComboBox();
    ret->setEditable(false);

    for (auto it : options)
      ret->addItem(it.second.c_str(), QVariant(it.first));

    ret->setCurrentText(options[value].c_str());

    if (callback)
    {
      QComboBox::connect(ret, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [ret, callback](int index) {
        callback(ret->itemData(index).toInt());
      });
    }

    return ret;
  }

  //CreateTextBox
  inline QLineEdit* CreateTextBox(String value,std::function<void(String)> callback=std::function<void(String)>()) 
  {
    auto ret=new QLineEdit(QString(value.c_str()));
    if (callback)
      QLineEdit::connect(ret, &QLineEdit::editingFinished, [callback,ret](){callback(cstring(ret->text()));});
    return ret;  
  }

  //CreateIntegerTextBoxWidget
  inline QLineEdit* CreateIntegerTextBoxWidget(int value,std::function<void(int)> callback=std::function<void(int)>()) 
  {
    auto ret=new QLineEdit(QString(cstring(value).c_str()));
    ret->setValidator(new QIntValidator());
    if (callback)
      QLineEdit::connect(ret, &QLineEdit::editingFinished, [callback,ret](){callback(cint(cstring(ret->text())));});
    return ret;  
  }

  //CreateDoubleTextBoxWidget
  inline QLineEdit* CreateDoubleTextBoxWidget(double value,std::function<void(double)> callback=std::function<void(double)>()) 
  {
    auto ret=new QLineEdit(QString(cstring(value).c_str()));
    ret->setValidator(new QDoubleValidator());
    if (callback)
      QLineEdit::connect(ret, &QLineEdit::editingFinished, [callback,ret](){callback(cdouble(cstring(ret->text())));});
    return ret;  
  }

  //CreateIntegerSliderWidget
  inline QSlider* CreateIntegerSliderWidget(int value,int From,int To,std::function<void(int)> callback=std::function<void(int)>())
  {
    auto ret=new QSlider();
    ret->setStyleSheet("QSlider {height: 20px;}");
    ret->setOrientation(Qt::Horizontal);
    ret->setValue(value);
    if (callback)
      QLineEdit::connect(ret, &QSlider::valueChanged, callback);
    ret->setMinimum(From);
    ret->setMaximum(To);
    return ret;
  }

  //CreateIntegerSliderAndShowToolTip
  inline QLayout* CreateIntegerSliderAndShowToolTip(QSlider*& slider,int value,int From,int To,std::function<void(int)> callback=std::function<void(int)>()) {
    auto ret=new QHBoxLayout();
    slider=GuiFactory::CreateIntegerSliderWidget(value,From,To,callback);
    auto label=new QLabel(std::to_string(value).c_str());
    ret->addWidget(slider);
    ret->addWidget(label);
    QSlider::connect(slider,&QSlider::valueChanged,[label](int value){
      label->setText(std::to_string(value).c_str());
    });
    return ret;
  }

  //CreateDoubleSliderWidget
  inline QDoubleSlider* CreateDoubleSliderWidget(double value,Range range,std::function<void(double)> callback=std::function<void(double)>())
  {
    auto ret=new QDoubleSlider();
    ret->setRange(range);
    ret->setValue(value);
    if (callback)
      QDoubleSlider::connect(ret, &QDoubleSlider::doubleValueChanged, callback);
    return ret;
  }
  
  //CreateTextEdit
  inline QTextEdit* CreateTextEdit(Color text_color=Colors::Black,Color background_color=Colors::LightGray)
  {
    auto ret=new QTextEdit();
    ret->setLineWrapMode(QTextEdit::NoWrap);
    
    //deprecated
    //ret->setTabStopWidth(5);

    auto font=QFontDatabase::systemFont(QFontDatabase::FixedFont);
    //font.setPointSize(fontsize);
    ret->setFont(font);

    QPalette palette=ret->palette();
    palette.setColor(QPalette::Text ,QUtils::convert<QColor>(text_color));
    palette.setColor(QPalette::Base, QUtils::convert<QColor>(background_color));
    ret->setAutoFillBackground(true);
    ret->setPalette(palette);

    return ret;
  }

  ///////////////////////////////////////////////////
  class VISUS_GUI_API CompactColorView : public QLabel
  {
    Q_OBJECT

  public:

    VISUS_NON_COPYABLE_CLASS(CompactColorView)

    //constructor
    CompactColorView(Color color=Color()) {
      setAutoFillBackground(true);
      setColor(color);
    }

    //destructro
    virtual ~CompactColorView() {
    }

    //getColor
    Color getColor() const {
      return QUtils::convert<Color>(palette().color(QPalette::Window));
    }

    //setColor
    void setColor(Color value) {
      if (getColor()==value) return;
      QPalette palette=this->palette();
      palette.setColor(QPalette::Window,QUtils::convert<QColor>(value));
      setPalette(palette);
      emit valueChanged(value);    
    }

  signals:

    void valueChanged(Color);

  private:

    //mousePressEvent
    virtual void mousePressEvent(QMouseEvent* evt) override {
      setColor(QUtils::convert<Color>(QColorDialog::getColor(palette().color(QPalette::Window))));
    }
  };

  //CreateCompactColorView
  inline CompactColorView* CreateCompactColorView(Color value,std::function<void(Color)> callback=std::function<void(Color)>()) 
  {
    auto ret=new CompactColorView(value);
    if (callback)
      CompactColorView::connect(ret,&CompactColorView::valueChanged,callback);
    return ret;
  }

  ///////////////////////////////////////////////////////
  class VISUS_GUI_API GLMaterialView : public QFrame
  {
    Q_OBJECT

  public:

    VISUS_NON_COPYABLE_CLASS(GLMaterialView)

    class Widgets
    {
    public:

      struct
      {
        CompactColorView* ambient=nullptr;
        CompactColorView* diffuse=nullptr;
        CompactColorView* specular=nullptr;
        CompactColorView* emission=nullptr;
        QSlider* shininess=nullptr;
      }
      front,back;
    };

    Widgets widgets;

  
    //constructor
    GLMaterialView(GLMaterial value=GLMaterial()) {

      auto tab=new QTabWidget();

      auto front_layout=new QFormLayout();
      front_layout->addRow("Ambient"  ,widgets.front.ambient  =GuiFactory::CreateCompactColorView(value.front.ambient  ,[this](Color){emitChanged();}));
      front_layout->addRow("Diffuse"  ,widgets.front.diffuse  =GuiFactory::CreateCompactColorView(value.front.diffuse  ,[this](Color){emitChanged();}));
      front_layout->addRow("Specular" ,widgets.front.specular =GuiFactory::CreateCompactColorView(value.front.specular ,[this](Color){emitChanged();}));
      front_layout->addRow("Emission" ,widgets.front.emission =GuiFactory::CreateCompactColorView(value.front.emission ,[this](Color){emitChanged();}));
      front_layout->addRow("Shininess",widgets.front.shininess=GuiFactory::CreateIntegerSliderWidget(value.front.shininess,0,128,[this](int){emitChanged();}));

      auto front=new QWidget(); front->setLayout(front_layout); tab->addTab(front,"FRONT");

      auto back_layout=new QFormLayout();
      back_layout->addRow("Ambient"   ,widgets.back.ambient  =GuiFactory::CreateCompactColorView(value.back.ambient  ,[this](Color){emitChanged();}));
      back_layout->addRow("Diffuse"   ,widgets.back.diffuse  =GuiFactory::CreateCompactColorView(value.back.diffuse  ,[this](Color){emitChanged();}));
      back_layout->addRow("Specular"  ,widgets.back.specular =GuiFactory::CreateCompactColorView(value.back.specular ,[this](Color){emitChanged();}));
      back_layout->addRow("Emission"  ,widgets.back.emission =GuiFactory::CreateCompactColorView(value.back.emission ,[this](Color){emitChanged();}));
      back_layout->addRow("Shininess" ,widgets.back.shininess=GuiFactory::CreateIntegerSliderWidget(value.back.shininess,0,128,[this](int){emitChanged();}));
      auto back=new QWidget(); back->setLayout(back_layout); tab->addTab(back,"BACK");

      auto layout=new QVBoxLayout();
      layout->addWidget(tab);
      setLayout(layout);

      setMaterial(value);
    }
  
    //destructor
    virtual ~GLMaterialView() {
    }
  
    //getMaterial
    GLMaterial getMaterial() const {
      GLMaterial ret;
      ret.front.ambient  =widgets.front.ambient  ->getColor(); ret.back.ambient  =widgets.back.ambient  ->getColor();
      ret.front.diffuse  =widgets.front.diffuse  ->getColor(); ret.back.diffuse  =widgets.back.diffuse  ->getColor();
      ret.front.specular =widgets.front.specular ->getColor(); ret.back.specular =widgets.back.specular ->getColor();
      ret.front.emission =widgets.front.emission ->getColor(); ret.back.emission =widgets.back.emission ->getColor();
      ret.front.shininess=widgets.front.shininess->value()   ; ret.back.shininess=widgets.back.shininess->value();
      return ret;
    }

    //setMaterial
    void setMaterial(GLMaterial value) 
    {
      if (getMaterial()==value) return;
      widgets.front.ambient  ->setColor(value.front.ambient  ); widgets.back.ambient  ->setColor(value.back.ambient  );
      widgets.front.diffuse  ->setColor(value.front.diffuse  ); widgets.back.diffuse  ->setColor(value.back.diffuse  );
      widgets.front.specular ->setColor(value.front.specular ); widgets.back.specular ->setColor(value.back.specular );
      widgets.front.emission ->setColor(value.front.emission ); widgets.back.emission ->setColor(value.back.emission );
      widgets. front.shininess->setValue(value.front.shininess); widgets.back.shininess->setValue(value.back.shininess);

      emitChanged();
    }

  signals:

    void valueChanged(GLMaterial value);

  private:

    //emitChanged
    void emitChanged() {
      emit valueChanged(getMaterial());
    }

  };


  //CreateGLMaterialView
  inline GLMaterialView* CreateGLMaterialView(GLMaterial value,std::function<void(GLMaterial)> callback=std::function<void(GLMaterial)>())
  {
    auto ret=new GLMaterialView(value);
    if (callback)
      GLMaterialView::connect(ret,&GLMaterialView::valueChanged,callback);
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////////
  class VISUS_GUI_API Point3dView : public QFrame
  {
    Q_OBJECT

  public:

    VISUS_NON_COPYABLE_CLASS(Point3dView)

    class Widgets
    {
    public:
      QLineEdit* text_box[3];
	    Widgets() { text_box[0] = text_box[1] = text_box[2] = nullptr; }
    };

    Widgets widgets;

    //constructor
    Point3dView(Point3d value=Point3d()) 
    {
      QHBoxLayout* layout=new QHBoxLayout();
      layout->addWidget(widgets.text_box[0]=new QLineEdit("0.0"));
      layout->addWidget(widgets.text_box[1]=new QLineEdit("0.0"));
      layout->addWidget(widgets.text_box[2]=new QLineEdit("0.0"));
      setLayout(layout);
      setPoint(value,true);
    }

    //destructor
    ~Point3dView() {
    }

    //isEditable
    bool isEditable(int axis) const {
      return widgets.text_box[0]->isReadOnly();
    }

    //isEditable
    bool isEditable() const {
      return isEditable(0) || isEditable(1) || isEditable(2);
    }

    //setEnabled
    void setEnabled(int axis,bool value) {
      widgets.text_box[axis]->setReadOnly(value?false:true);
    }

    //setEnabled
    void setEnabled(bool value) {
      for (int I=0;I<3;I++)
        setEnabled(I,value);
    }

    //setPoint
    void setPoint(const Point3d& value,int precision=-1) {

      Point3d old_value=getPoint();
      widgets.text_box[0]->setText(StringUtils::convertDoubleToString(value[0],precision).c_str());
      widgets.text_box[1]->setText(StringUtils::convertDoubleToString(value[1],precision).c_str());
      widgets.text_box[2]->setText(StringUtils::convertDoubleToString(value[2],precision).c_str());
      auto new_value=getPoint();
      if (new_value!=old_value)
        emit valueChanged(new_value);
    }

    //getPoint
    Point3d getPoint() const {
      Point3d ret;
      ret[0]=cdouble(widgets.text_box[0]->text());
      ret[1]=cdouble(widgets.text_box[1]->text());
      ret[2]=cdouble(widgets.text_box[2]->text());
      return ret;
    }

  signals:

    void valueChanged(const Point3d& value);

  };

 //CreatePoint3dView
  inline Point3dView* CreatePoint3dView(Point3d value,std::function<void(Point3d)> callback=std::function<void(Point3d)>())
  {
    auto ret=new Point3dView(value);
    if (callback)
      Point3dView::connect(ret,&Point3dView::valueChanged,callback);
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////////
  class VISUS_GUI_API Box3dView : public QFrame
  {
    Q_OBJECT

  public:

    VISUS_NON_COPYABLE_CLASS(Box3dView)

    class Widgets
    {
    public:
      Point3dView *p1=nullptr;
      Point3dView *p2=nullptr;
      QToolButton* btSet=nullptr;
    };

    Widgets widgets;

    //constructor
    Box3dView(BoxNd value=BoxNd(3))
    {
      value.setPointDim(3);

      QVBoxLayout* layout=new QVBoxLayout();

      {
        QFormLayout* row=new QFormLayout();
        row->addRow("P1",widgets.p1=new Point3dView());
        row->addRow("P2",widgets.p2=new Point3dView());
        layout->addLayout(row);
      }
  
      {
        auto row=new QHBoxLayout();
        row->addStretch(1);
        row->addWidget(widgets.btSet=GuiFactory::CreateButton("Set",[this](bool){
          setValue(getValue(),/*bForce*/true);
        }));

        layout->addLayout(row);
      }

      setLayout(layout);

      setValue(value,true);
    }

    //destructor
    ~Box3dView() {
    }

    //isEditable
    bool isEditable() const {
      return widgets.p1->isEditable();
    }

    //setEnabled
    void setEnabled(bool value) {
      widgets.p1->setEnabled(value);
      widgets.p2->setEnabled(value);
      widgets.btSet->setEnabled(value);
    }

    //setValue
    void setValue(BoxNd value,bool bForce=false) {

      value.setPointDim(3);
      auto old_value=getValue();
      widgets.p1->setPoint(value.p1.toPoint3());
      widgets.p2->setPoint(value.p2.toPoint3());
      auto new_value=getValue();

      if (bForce || old_value!=new_value)
        emit valueChanged(new_value);
    }

    //getValue
    BoxNd getValue() const{
      return BoxNd(widgets.p1->getPoint(),widgets.p2->getPoint());
    }

  signals:

    //valueChanged
    void valueChanged(const BoxNd& value);

  };


  //CreateBox3dView
  inline Box3dView* CreateBox3dView(BoxNd value,std::function<void(BoxNd)> callback=std::function<void(BoxNd)>())
  {
    value.setPointDim(3);
    auto ret=new Box3dView(value);
    if (callback)
      Box3dView::connect(ret,&Box3dView::valueChanged,callback);
    return ret;
  }

  /////////////////////////////////////////////////////////////////////////////////
  class VISUS_GUI_API MatrixView : public QFrame
  {
    Q_OBJECT

  public:

    VISUS_NON_COPYABLE_CLASS(MatrixView)

    class Widgets
    {
    public:
      QLineEdit*    text_box[4][4];
      QToolButton*  btSet = nullptr;
      QToolButton*  btIdentity = nullptr;
    };

    Widgets widgets;

    //constructor
    MatrixView(Matrix value= Matrix::identity(4))
    {
      VisusAssert(value.getSpaceDim() == 4);
      auto layout = new QVBoxLayout();

      QGridLayout* row = new QGridLayout();
      for (int R = 0; R < 4; R++) {
        for (int C = 0; C < 4; C++)
        {
          row->addWidget(widgets.text_box[R][C] = GuiFactory::CreateDoubleTextBoxWidget(R == C ? 1.0 : 0.0), R, C);
        }
      }
      layout->addLayout(row);

      {
        auto row = new QHBoxLayout();
        row->addStretch(1);
        row->addWidget(widgets.btSet=GuiFactory::CreateButton("Set",[this](bool) {
          setMatrix(getMatrix(),true); 
        }));

        row->addWidget(widgets.btIdentity = GuiFactory::CreateButton("Identity",[this](bool) {
          setMatrix(Matrix::identity(4),true); 
        }));
        layout->addLayout(row);
      }

      setLayout(layout);
      setMatrix(value,true);
    }

    //destructor
    ~MatrixView() {
    }

    //isEditable
    bool isEditable() const {
      return widgets.text_box[0][0]->isReadOnly();
    }

    //setEnabled
    void setEnabled(bool value)
    {
      for (int R = 0; R < 4; R++)
        for (int C = 0; C < 4; C++)
          widgets.text_box[R][C]->setReadOnly(value?false:true);

      widgets.btSet->setEnabled(value);
      widgets.btIdentity->setEnabled(value);
    }

    //setMatrix
    void setMatrix(Matrix value, bool bForce = false)
    {
      value.setSpaceDim(4);
      auto old_value = getMatrix();
      for (int R = 0; R < 4; R++) {
        for (int C = 0; C < 4; C++)
          widgets.text_box[R][C]->setText(cstring(value(R, C)).c_str());
      }
      auto new_value = getMatrix();

      if (bForce || new_value!=old_value)
        emit valueChanged(new_value);
    }

    //getMatrix
    Matrix getMatrix() const {
      auto ret = Matrix::identity(4);
      for (int R = 0; R < 4; R++)
        for (int C = 0; C < 4; C++)
          ret(R, C) = cdouble(widgets.text_box[R][C]->text());
      return ret;
    }

  signals:

    void valueChanged(const Matrix& value);

  };

  //CreateMatrixView
  inline MatrixView* CreateMatrixView(Matrix value,std::function<void(Matrix)> callback=std::function<void(Matrix)>())
  {
    auto ret=new MatrixView(value);
    if (callback)
      MatrixView::connect(ret,&MatrixView::valueChanged,callback);
    return ret;
  }

};



} //namespace Visus

#endif //VISUS_GUI_FACTORY_H__

