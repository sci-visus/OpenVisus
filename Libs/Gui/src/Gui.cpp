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

#include <Visus/Gui.h>
#include <Visus/DoubleSlider.h>
#include <Visus/GLMaterial.h>
#include <Visus/GLInfo.h>
#include <Visus/GLCamera.h>
#include <Visus/GLOrthoCamera.h>
#include <Visus/GLLookAtCamera.h>
#include <Visus/GLCanvas.h>
#include <Visus/GLPhongShader.h>

#include <QFrame>
#include <QTextStream>
#include <QDirIterator>
#include <QDebug>
#include <QApplication>

void GuiInitResources() {
  Q_INIT_RESOURCE(Gui);
}

void GuiCleanUpResources() {
  Q_CLEANUP_RESOURCE(Gui);
}

namespace Visus {

////////////////////////////////////////////////////////
void QUtils::clearQWidget(QWidget* widget)
{
  auto children = widget->children();
  if (!children.empty())
    qDeleteAll(children);

  if (auto layout = widget->layout())
  {
    delete layout;
    widget->setLayout(nullptr);
  }
}


///////////////////////////////////////////////////////////////////////////////////
void QUtils::RenderCheckerBoard(QPainter& painter,int x, int y, int width, int height, int checkWidth, int checkHeight, const Color& color1_, const Color& color2_)
{
  QRect area(x, y, width, height);
  const QRect clipped = painter.viewport().intersected(area);

  if (clipped.isEmpty())
    return;

  QColor color1 = QUtils::convert<QColor>(color1_);
  QColor color2 = QUtils::convert<QColor>(color2_);

  painter.save();
  painter.setClipRect(clipped);

  const int checkNumX = (clipped.x() - area.x()) / checkWidth;
  const int checkNumY = (clipped.y() - area.y()) / checkHeight;
  const int startX = area.x() + checkNumX * checkWidth;
  const int startY = area.y() + checkNumY * checkHeight;
  const int right  = clipped.right();
  const int bottom = clipped.bottom();

  for (int i = 0; i < 2; ++i)
  {
    const QColor& color = (i == ((checkNumX ^ checkNumY) & 1)) ? color1 : color2;
    int cy = i;
    for (int y = startY                          ; y < bottom; y += checkHeight   )
    for (int x = startX + (cy++ & 1) * checkWidth; x < right ; x += checkWidth * 2)
      painter.fillRect(QRect(x, y, checkWidth, checkHeight), color);
  }

  painter.restore();
}


///////////////////////////////////////////////////////////////////////////////////
String QUtils::LoadTextFileFromResources(String name) 
{
  QFile file(name.c_str());
  if(!file.open(QFile::ReadOnly | QFile::Text))
  {
    VisusAssert(false);
    VisusInfo() << " Could not open "<<name;
    return "";
  }

  QTextStream in(&file);
  auto content=in.readAll();
  return cstring(content);
}

bool GuiModule::bAttached = false;

///////////////////////////////////////////////////////////////////////////////////
void GuiModule::attach()
{
  if (bAttached)  
    return;
  
  VisusInfo() << "Attaching GuiModule...";

  bAttached = true;

  GuiInitResources();

  KernelModule::attach();

  GLSharedContext::allocSingleton();
  GLInfo::allocSingleton();
  GLDoWithContext::allocSingleton();
  GLPhongShader::Shaders::allocSingleton();

  VISUS_REGISTER_OBJECT_CLASS(GLLookAtCamera);
  VISUS_REGISTER_OBJECT_CLASS(GLOrthoCamera);

  //show qt resources
#if 0
  QDirIterator it(":", QDirIterator::Subdirectories);
  while (it.hasNext()) {
    qDebug() << it.next();
  }
#endif

  VisusInfo() << "Attached GuiModule";
}


//////////////////////////////////////////////
void GuiModule::detach()
{
  if (!bAttached)  
    return;
  
  VisusInfo() << "Detaching GuiModule...";
  
  bAttached = false;

  GuiCleanUpResources();

  GLInfo::releaseSingleton();
  GLDoWithContext::releaseSingleton();
  GLSharedContext::releaseSingleton();
  GLPhongShader::Shaders::releaseSingleton();

  KernelModule::detach();

  VisusInfo() << "Detached GuiModule";
}

//////////////////////////////////////////////
static QApplication* qapp = nullptr;

void GuiModule::createApplication()
{
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

  VisusAssert(!qapp && Private::CommandLine::argn != 0);
  qapp=new QApplication(Private::CommandLine::argn, (char**)Private::CommandLine::argv);
  qapp->setAttribute(Qt::AA_UseHighDpiPixmaps, true);

  //otherwise volume render looks bad on my old macbook pro
#if __APPLE__
  auto format = QSurfaceFormat::defaultFormat();
  format.setProfile(QSurfaceFormat::CoreProfile);
  QSurfaceFormat::setDefaultFormat(format);
#endif
}

//////////////////////////////////////////////
void GuiModule::execApplication()
{
  QApplication::exec();
}


void GuiModule::destroyApplication()
{
  delete qapp;
  qapp = nullptr;
}

} //namespace Visus

