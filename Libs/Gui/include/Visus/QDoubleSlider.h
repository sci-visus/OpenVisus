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

#ifndef __VISUS_DOUBLE_SLIDER_H
#define __VISUS_DOUBLE_SLIDER_H

#include <Visus/Gui.h>
#include <Visus/Utils.h>

#include <QSlider>
#include <QMoveEvent>
#include <QBoxLayout>

namespace Visus {

////////////////////////////////////////////////////
class VISUS_GUI_API QDoubleSlider : public QWidget
{
  Q_OBJECT

public:

  VISUS_NON_COPYABLE_CLASS(QDoubleSlider)

  //constructor
    QDoubleSlider(double value=0,Range range=Range(0,1,0))
  {
    qslider = new QSlider();

    auto layout=new QVBoxLayout();
    qslider->setOrientation(Qt::Horizontal);
    layout->addWidget(qslider);
    QWidget::setLayout(layout);
    
    setRange(range);
    setValue(value);

    connect(qslider,&QSlider::valueChanged,[this](int ival){
      internalSetValue(int_to_double(ival),/*bRefreshSlider*/false);
    });
  }

  //destructor
  virtual ~QDoubleSlider() {
  }

  //value
  double value() const {
    return dval;
  }
  
  //setValue
  void setValue(double dval) {
    internalSetValue(dval,true);
  }

  //getRange
  Range getRange() const {
    return range;
  }

  //setRange
  void setRange(Range value)
  {
    this->range = value;

    //is an integer range
    if (int(range.from) == range.from && int(range.to) == range.to && int(range.step) == range.step && range.step > 0)
    {
      qslider->setMinimum(range.from);
      qslider->setMaximum(range.to);
      qslider->setSingleStep(range.step);

      this->double_to_int = [](double val) {
        return (int)val;
      };

      this->int_to_double = [](int val) {
        return (double)val;
      };

    }
    else
    {
      qslider->setMinimum(0);
      qslider->setMaximum(1 << 16); //this is the precision

      this->double_to_int=[this](double dval)
      {
        if (range.to == range.from) return 0;
        auto alpha = (dval - range.from) / (range.to - range.from);
        return (int)(alpha*qslider->maximum());
      };

      this->int_to_double=[this](int ival)
      {
        if (range.to == range.from) return range.from;
        auto alpha = ival / (double)qslider->maximum();
        return range.from + alpha*(range.to - range.from);
      };

    }

    //setValue(value());
  }

signals:

  //doubleValueChanged
  void doubleValueChanged(double);

private:

  QSlider* qslider;
  
  Range  range=Range(0,0,0);
  double dval = 0;

  std::function<int(double)> double_to_int;
  std::function<double(int)> int_to_double;

  //internalSetValue
  void internalSetValue(double dval,bool bRefreshSlider)
  {
    dval=Utils::clamp(dval, range.from, range.to);

    // on initialization of QDoubleSlider widget we need to set the slider's value (thus the dval==this->val check is done after setting the slider)
    if (bRefreshSlider)
    {
      auto ival=double_to_int(dval);
      if (ival!=qslider->value())
      {
        auto signals_were_blocked=qslider->blockSignals(true);
        qslider->setValue(ival);
        qslider->blockSignals(signals_were_blocked);
      }
    }

    if (dval==this->dval)
      return;

    this->dval=dval;

    emit doubleValueChanged(dval);
  }
  

};

} //namespace Visus


#endif //__VISUS_DOUBLE_SLIDER_H

