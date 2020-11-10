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

#ifndef __VISUS_HISTOGRAM_VIEW_H
#define __VISUS_HISTOGRAM_VIEW_H

#include <Visus/Gui.h>
#include <Visus/QCanvas2d.h>
#include <Visus/Histogram.h>

#include <QFrame>
#include <QMouseEvent>

namespace Visus {

////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API HistogramView : public QCanvas2d
{
  Q_OBJECT

public:

  VISUS_NON_COPYABLE_CLASS(HistogramView)

  //default constructor
  HistogramView()  {
  }

  //destructor
  virtual ~HistogramView() {
  }

  //setHistogram
  void setHistogram(const Histogram& value) 
  {
    this->histogram=value;
    auto x1 = value.range.from;
    auto x2 = value.range.to;
    auto y1 = 0.0;
    auto y2 = value.bins.empty()? 0.0 : 1.25 * (*std::max_element(value.bins.begin(), value.bins.end()));;
    this->setWorldBox(x1, y1, x2 - x1, y2 - y1);
    this->postRedisplay();
  }

    //resizeEvent
  virtual void resizeEvent(QResizeEvent* evt) override
  {
    QCanvas2d::resizeEvent(evt);
    setHistogram(histogram);
    postRedisplay();
  }

  //getSelectedRegion
  Range getSelectedRegion() const {
    return selected_region;
  }

  //setSelectedRegion
  void setSelectedRegion(Range value) 
  {
    this->selected_region = value;
    postRedisplay();
    emit selectedRegionChanged(value);
  }

  //mousePressEvent
  virtual void mousePressEvent(QMouseEvent* evt) override
  {
    QCanvas2d::mousePressEvent(evt);

    if (evt->button() == Qt::LeftButton)
    {
      bSelectingRegion = true;
      auto x1 = QUtils::convert<Point2d>(unproject(evt->pos())).x;
      this->selected_region = Range(x1, x1, 0);
      postRedisplay();
    }
  }

  //mouseMoveEvent
  virtual void mouseMoveEvent(QMouseEvent* evt) override
  {
    QCanvas2d::mouseMoveEvent(evt);

    if (bSelectingRegion)
    {
      auto x2 = QUtils::convert<Point2d>(unproject(evt->pos())).x;
      this->selected_region = Range(selected_region.from, x2, 0);
      postRedisplay();
    }
  }

  //mouseReleaseEvent
  virtual void mouseReleaseEvent(QMouseEvent* evt) override
  {
    QCanvas2d::mouseReleaseEvent(evt);

    if (bSelectingRegion)
    {
      bSelectingRegion = false;
      auto x2 = QUtils::convert<Point2d>(unproject(evt->pos())).x;
      setSelectedRegion(Range(selected_region.from, x2, 0));
    }
  }

  //paint
  virtual void paintEvent(QPaintEvent* evt) override
  {
    if (!histogram.getNumBins())
      return;

    auto W = getWorldBox().width;
    auto H = getWorldBox().height;

    QPainter painter(this);
    renderBackground(painter);

    //render bars
    for (int bLog : {1, 0})
    {
      painter.save();

      if (bLog)
      {
        painter.setPen(QColor(0, 0, 0, 5));
        painter.setBrush(QColor(173, 216, 230));
      }
      else
      {
        painter.setPen(QColor(0, 0, 0, 5));
        painter.setBrush(QColor(100, 100, 100));
      }

      for (int B = 0; B < histogram.getNumBins() - 1; B++)
      {
        auto bin_range = histogram.getBinRange(B);
        auto bin_value = histogram.readBin(B);

        auto mylog = [&](double x) { return (x == 0) ? (0) : (1 + log(x)); };
        double x1 = bin_range.from;
        double x2 = bin_range.to;
        double y1 = 0;
        double y2 = bLog ? (H * mylog(bin_value) / mylog(H)) : bin_value;
        painter.drawRect(project(QRectF(x1, y1,  x2 - x1, y2-y1)));
      }

      painter.restore();
    }

    //draw selection
    if (selected_region.delta()>0)
    {
      painter.save();
      painter.setPen(QColor(0, 0, 0, 128));
      painter.setBrush(QColor(100, 100, 0, 100));
      auto x1 = selected_region.from; auto y1 = getWorldBox().p1().y;
      auto x2 = selected_region.to;   auto y2 = getWorldBox().p2().y;
      painter.drawRect(project(QRectF(x1, y1, x2 - x1, y2 - y1)));
      painter.restore();
    }

    auto value = getCurrentPos();

    //draw explanation
    {
      int bin = histogram.findBin(value.x);

      String description = cstring(
        cnamed("value", value),
        cnamed("#bin", cstring(bin, "/", histogram.getNumBins())),
        cnamed("bin_count", histogram.readBin(bin)),
        cnamed("bin_range", cstring(histogram.getBinRange(bin).from, ",", histogram.getBinRange(bin).to)));

      painter.save();
      painter.setPen(QColor(0, 0, 0));
      painter.drawText(2, 12, description.c_str());
      painter.restore();
    }

  //render cross
    {
      painter.setPen(QColor(0, 0, 0, 100));
      painter.save();
      painter.drawLine(project(QPointF(0, value[1])), project(QPointF(W, value[1])));
      painter.drawLine(project(QPointF(value[0], 0)), project(QPointF(value[0], H)));
      painter.restore();
    }

    renderBorders(painter);
  }

signals:

  //selectedRegionChanged
  void selectedRegionChanged(Range range);

private:

  Histogram histogram;
  bool      bSelectingRegion = false;
  Range     selected_region = Range::invalid();

};


} //namespace Visus

#endif //__VISUS_HISTOGRAM_VIEW_H


