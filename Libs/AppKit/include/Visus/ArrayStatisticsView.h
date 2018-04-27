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

#ifndef __VISUS_ARRAY_STATISTICS_VIEW_H
#define __VISUS_ARRAY_STATISTICS_VIEW_H

#include <Visus/AppKit.h>
#include <Visus/HistogramView.h>
#include <Visus/Statistics.h>

#include <QFrame>
#include <QLabel>
#include <QComboBox>
#include <QFormLayout>
#include <QTabWidget>
#include <QMouseEvent>

namespace Visus {


////////////////////////////////////////////////
class VISUS_APPKIT_API ArrayStatisticsView : public QFrame
{
public:

  VISUS_NON_COPYABLE_CLASS(ArrayStatisticsView)

  //_______________________________________________
  class Widgets
  {
  public:

    QTabWidget*    tabs = nullptr;
    QLabel*        dtype = nullptr;
    QLabel*        dims = nullptr;

    //for each component of the array...
    class Component
    {
    public:
      QLabel*        dtype = nullptr;
      QLabel*        array_range = nullptr;
      QLabel*        computed_range = nullptr;
      QLabel*        average = nullptr;
      QLabel*        median = nullptr;
      QLabel*        variance = nullptr;
      QLabel*        standard_deviation = nullptr;
      HistogramView* histogram = nullptr;
    };

    std::vector<Component> components;
  };

  Widgets widgets;

  //constructor
  ArrayStatisticsView()
  {
    auto vlayout = new QVBoxLayout();

    {
      auto form = new QFormLayout();
      form->addRow("DType", widgets.dtype = new QLabel(""));
      form->addRow("Dims", widgets.dims = new QLabel(""));
      vlayout->addLayout(form);
    }

    {
      widgets.tabs = new QTabWidget();
      vlayout->addWidget(widgets.tabs, 1);
    }

    setLayout(vlayout);
  }

  //destructor
  virtual ~ArrayStatisticsView() {
  }

  //setStatistics
  void setStatistics(const Statistics& value) 
  {
    int ncomponents = (int)value.components.size();
    widgets.components.resize(ncomponents);

    while (widgets.tabs->count() > ncomponents)
    {
      int C = widgets.tabs->count() - 1;
      widgets.tabs->removeTab(C);
    }

    while (widgets.tabs->count() < ncomponents)
    {
      int   C   = widgets.tabs->count();
      
      auto&       dst = widgets.components[C];
      const auto& src = value.components[C];

      auto vlayout = new QVBoxLayout();

      {
        auto form = new QFormLayout();
        form->addRow("DType", dst.dtype = new QLabel(""));
        form->addRow("Array Range", dst.array_range = new QLabel(""));
        form->addRow("Computed Range", dst.computed_range = new QLabel(""));
        form->addRow("Average", dst.average = new QLabel(""));
        form->addRow("Median", dst.median = new QLabel(""));
        form->addRow("Variance", dst.variance = new QLabel(""));
        form->addRow("Standard deviation", dst.standard_deviation = new QLabel(""));
        vlayout->addLayout(form);
      }

      vlayout->addWidget(dst.histogram = new HistogramView(), 1);

      auto frame = new QFrame();
      frame->setLayout(vlayout);
      widgets.tabs->addTab(frame, cstring(C).c_str());
    }

    widgets.dtype->setText(value.dtype.toString().c_str());
    widgets.dims->setText(value.dims.toString().c_str());

    for (int C = 0; C < ncomponents; C++)
    {
      auto& dst = this->widgets.components[C];
      const auto& src = value.components[C];
      dst.dtype->setText(src.dtype.toString().c_str());
      dst.array_range->setText(src.array_range.toString().c_str());
      dst.computed_range->setText(src.computed_range.toString().c_str());
      dst.average->setText(std::to_string(src.average).c_str());
      dst.median->setText(std::to_string(src.median).c_str());
      dst.variance->setText(std::to_string(src.variance).c_str());
      dst.standard_deviation->setText(std::to_string(src.standard_deviation).c_str());
      dst.histogram->setHistogram(src.histogram);
    }
  }

};

} //namespace Visus

#endif //__VISUS_ARRAY_STATISTICS_VIEW_H


