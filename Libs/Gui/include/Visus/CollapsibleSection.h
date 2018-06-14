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

#ifndef __VISUS_COLLAPSIBLE_SECTION_H
#define __VISUS_COLLAPSIBLE_SECTION_H

#include <Visus/Gui.h>

#include <QToolButton>
#include <QFrame>
#include <QFrame>
#include <QScrollArea>
#include <QParallelAnimationGroup>
#include <QGridLayout>
#include <QPropertyAnimation>

namespace Visus {

//////////////////////////////////////////////////////////////////////////////////////////
/* example:

  auto vlayout = new QVBoxLayout();

  vlayout->addWidget(new QTextEdit());

  {
    auto section = new CollapsibleSection("Section", 300);
    auto* sub = new QVBoxLayout();
    sub->addWidget(new QLabel("Some Text in Section"));
    sub->addWidget(new QPushButton("Button in Section"));
    section->setLayout(*sub);
    vlayout->addWidget(section);
  }

*/
class CollapsibleSection : public QWidget
{
public:

  //constructor
  CollapsibleSection(const QString & title = "", const int animationDuration = 100, QWidget* parent = 0)
    : QWidget(parent), animationDuration(animationDuration)
  {
    toggleButton = new QToolButton(this);
    headerLine = new QFrame(this);
    toggleAnimation = new QParallelAnimationGroup(this);
    contentArea = new QScrollArea(this);
    mainLayout = new QGridLayout(this);

    toggleButton->setStyleSheet("QToolButton {border: none;}");
    toggleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggleButton->setArrowType(Qt::ArrowType::RightArrow);
    toggleButton->setText(title);
    toggleButton->setCheckable(true);
    toggleButton->setChecked(false);

    headerLine->setFrameShape(QFrame::HLine);
    headerLine->setFrameShadow(QFrame::Sunken);
    headerLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    contentArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // start out collapsed
    contentArea->setMaximumHeight(0);
    contentArea->setMinimumHeight(0);

    // let the entire widget grow and shrink with its content
    toggleAnimation->addAnimation(new QPropertyAnimation(this, "minimumHeight"));
    toggleAnimation->addAnimation(new QPropertyAnimation(this, "maximumHeight"));
    toggleAnimation->addAnimation(new QPropertyAnimation(contentArea, "maximumHeight"));

    mainLayout->setVerticalSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    int row = 0;
    mainLayout->addWidget(toggleButton, row, 0, 1, 1, Qt::AlignLeft);
    mainLayout->addWidget(headerLine, row++, 2, 1, 1);
    mainLayout->addWidget(contentArea, row, 0, 1, 3);
    setLayout(mainLayout);

    QObject::connect(toggleButton, &QToolButton::clicked, [this](const bool checked)
    {
      toggleButton->setArrowType(checked ? Qt::ArrowType::DownArrow : Qt::ArrowType::RightArrow);
      toggleAnimation->setDirection(checked ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
      toggleAnimation->start();
    });
  }

  //constructor
  CollapsibleSection(const QString & title , QWidget* widget, const int animationDuration = 100, QWidget* parent = 0) 
    : CollapsibleSection(title,animationDuration,parent) 
  {
    auto layout = new QVBoxLayout();
    layout->addWidget(widget);
    setLayout(layout);
  }

  //destructor
  virtual ~CollapsibleSection() {
  }

  //setLayout
  void setLayout(QLayout* value)
  {
    delete contentArea->layout();
    contentArea->setLayout(value);
    const auto collapsedHeight = sizeHint().height() - contentArea->maximumHeight();
    auto contentHeight = value->sizeHint().height();

    for (int i = 0; i < toggleAnimation->animationCount() - 1; ++i)
    {
      QPropertyAnimation* SectionAnimation = static_cast<QPropertyAnimation *>(toggleAnimation->animationAt(i));
      SectionAnimation->setDuration(animationDuration);
      SectionAnimation->setStartValue(collapsedHeight);
      SectionAnimation->setEndValue(collapsedHeight + contentHeight);
    }

    QPropertyAnimation* contentAnimation = static_cast<QPropertyAnimation *>(toggleAnimation->animationAt(toggleAnimation->animationCount() - 1));
    contentAnimation->setDuration(animationDuration);
    contentAnimation->setStartValue(0);
    contentAnimation->setEndValue(contentHeight);
  }

private:

  QGridLayout * mainLayout;
  QToolButton* toggleButton;
  QFrame* headerLine;
  QParallelAnimationGroup* toggleAnimation;
  QScrollArea* contentArea;
  int animationDuration;

};

} //namespace Visus


#endif //__VISUS_COLLAPSIBLE_SECTION_H

