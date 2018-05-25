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

    QTabWidget* tabs = nullptr;

    //for each component of the array...
    class Tab
    {
    public:

      QLabel*        dtype = nullptr;
      QLabel*        dims = nullptr;
      QLabel*        array_range = nullptr;
      QLabel*        computed_range = nullptr;
      QLabel*        average = nullptr;
      QLabel*        median = nullptr;
      QLabel*        variance = nullptr;
      QLabel*        standard_deviation = nullptr;
      HistogramView* histogram = nullptr;

      //createLayout
      QWidget* createWidget()
      {
        auto hlayout = new QHBoxLayout();

        {
          auto form = new QFormLayout();
          form->addRow("DType", this->dtype = new QLabel(""));
          form->addRow("Dims", this->dims = new QLabel(""));
          form->addRow("Array Range", this->array_range = new QLabel(""));
          form->addRow("Computed Range", this->computed_range = new QLabel(""));
          hlayout->addLayout(form);
        }

        {
          auto form = new QFormLayout();
          form->addRow("Average", this->average = new QLabel(""));
          form->addRow("Median", this->median = new QLabel(""));
          form->addRow("Variance", this->variance = new QLabel(""));
          form->addRow("Standard deviation", this->standard_deviation = new QLabel(""));
          hlayout->addLayout(form);
        }

        auto vlayout = new QVBoxLayout();
        vlayout->addLayout(hlayout);
        vlayout->addWidget(this->histogram = new HistogramView(), 1);

        auto frame = new QFrame();
        frame->setLayout(vlayout);
        return frame;
      }

      //formatRange
      static String formatRange(Range value) {
        return StringUtils::format() << "[" << value.from << " , " << value.to << "]";
      }

      //refresh
      void refresh(const Statistics::Component& src)
      {
        this->dtype->setText(src.dtype.toString().c_str());
        this->dims->setText(src.dims.toString().c_str());
        this->array_range->setText(formatRange(src.array_range).c_str());
        this->computed_range->setText(formatRange(src.computed_range).c_str());
        this->average->setText(std::to_string(src.average).c_str());
        this->median->setText(std::to_string(src.median).c_str());
        this->variance->setText(std::to_string(src.variance).c_str());
        this->standard_deviation->setText(std::to_string(src.standard_deviation).c_str());
        this->histogram->setHistogram(src.histogram);
      }
    };

    std::vector<Tab> components;
  };

  Widgets widgets;

  //constructor
  ArrayStatisticsView()
  {
    setMinimumSize(QSize(100, 80));

    auto vlayout = new QVBoxLayout();
    widgets.tabs = new QTabWidget();
    vlayout->addWidget(widgets.tabs, 1);
    setLayout(vlayout);
  }

  //destructor
  virtual ~ArrayStatisticsView() {
  }

  //setNumberOfTabs
  void setNumberOfTabs(int N)
  {
    widgets.components.resize(N);

    while (widgets.tabs->count() > N)
    {
      int C = widgets.tabs->count() - 1;
      widgets.tabs->removeTab(C);
    }

    while (widgets.tabs->count() < N)
    {
      int   C = widgets.tabs->count();
      auto widget = widgets.components[C].createWidget();
      widgets.tabs->addTab(widget, cstring(C).c_str());
    }
  }

  //setStatistics
  void setStatistics(const Statistics& value) 
  {
    int N = (int)value.components.size();
    setNumberOfTabs(N);

    for (int C = 0; C < N; C++)
      this->widgets.components[C].refresh(value.components[C]);
  }

};

} //namespace Visus

#endif //__VISUS_ARRAY_STATISTICS_VIEW_H


