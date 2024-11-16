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

#ifndef VISUS_GUI_H__
#define VISUS_GUI_H__

#include <Visus/Kernel.h>
#include <Visus/Color.h>
#include <Visus/Model.h>
#include <Visus/Rectangle.h>
#include <Visus/Array.h>

#include <QString>
#include <QColor>
#include <QRect>
#include <QPainter>

namespace Visus {

#if SWIG || VISUS_STATIC_LIB
  #define VISUS_GUI_API
#else
  #if VISUS_BUILDING_VISUSGUI
    #define VISUS_GUI_API VISUS_SHARED_EXPORT
  #else
    #define VISUS_GUI_API VISUS_SHARED_IMPORT
  #endif
#endif

class VISUS_GUI_API GuiModule : public VisusModule
{
public:

  static int attached;

  //attach
  static void attach();

  //detach
  static void detach();
};


//having the same function name between two modules seems not to work (one will overwrite the second)
#if !SWIG

inline String cstring(QString value) {
#if 1
  return value.toUtf8().constData();
#else
  return value.toStdString();
#endif
}

inline double cdouble(QString value) {
  return cdouble(cstring(value));
}

inline double cint(QString value) {
  return cint(cstring(value));
}

#endif

#if !SWIG
namespace QUtils {

template <typename DstType, typename SrcType>
DstType convert(const SrcType& r);

template <> inline Color convert(const QColor& c) {
  return Visus::Color(c.red(), c.green(), c.blue(), c.alpha());
}

template <> inline Color convert(const QRgb& c) {
  return Color((unsigned char)qRed(c), (unsigned char)qGreen(c), (unsigned char)qBlue(c), (unsigned char)qAlpha(c));
}

template <> inline QRgb convert(const Color& c) {
  return qRgba((int)(255.0 * c.getRed()), (int)(255.0 * c.getGreen()), (int)(255.0 * c.getBlue()), (int)(255.0 * c.getAlpha()));
}

template <> inline QColor convert(const Color& c) {
  return QColor::fromRgbF(c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha());
}

template <> inline QPoint convert(const Point2i& p) {
  return QPoint(p[0], p[1]);
}

template <> inline QPointF convert(const Point2d& p) {
  return QPointF(p[0], p[1]);
}

template <> inline Point2d convert(const QPointF& p) {
  return Point2d(p.x(), p.y());
}

template <> inline Point2d convert(const QPoint& p) {
  return Point2d(p.x(), p.y());
}

template <> inline Point2i convert(const QPoint& p) {
  return Point2i(p.x(), p.y());
}

template <> inline QRect convert(const Rectangle2d& r) {
  return QRect((int)r.x, (int)r.y, (int)r.width, (int)r.height);
}

template <> inline QRectF convert(const Rectangle2d& r) {
  return QRect(r.x, r.y, r.width, r.height);
}

template <> inline Rectangle2d convert(const QRect& r) {
  return Rectangle2d(r.x(), r.y(), r.width(), r.height());
}

template <> inline Rectangle2d convert(const QRectF& r) {
  return Rectangle2d(r.x(), r.y(), r.width(), r.height());
}


template <> inline QTransform convert(const Matrix& T) {
  VisusAssert(T.getSpaceDim() == 3);
  return QTransform(
    T(0, 0), T(0, 1), T(0, 2),
    T(1, 0), T(1, 1), T(1, 2),
    T(2, 0), T(2, 1), T(2, 2));
}

//RenderCheckerBoard
VISUS_GUI_API void RenderCheckerBoard(QPainter& g, int x, int y, int width, int height, int checkWidth, int checkHeight, const Color& color1_, const Color& color2_);

//ResourceTextFileContent
VISUS_GUI_API String LoadTextFileFromResources(String name);

//clearQWidget
VISUS_GUI_API void clearQWidget(QWidget* widget);

} //QUtils

#endif

} //namespace Visus


#endif //VISUS_GUI_H__


