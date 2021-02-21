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

#ifndef VISUS_ISOCONTOUR_BUILD_NODE_H
#define VISUS_ISOCONTOUR_BUILD_NODE_H

#include <Visus/Gui.h>
#include <Visus/Dataflow.h>
#include <Visus/Range.h>
#include <Visus/GLTexture.h>
#include <Visus/GLMesh.h>
#include <Visus/TransferFunction.h>
#include <Visus/Model.h>
#include <Visus/QDoubleSlider.h>
#include <Visus/GuiFactory.h>

#include <QLabel>
#include <QFrame>
#include <QFormLayout>
#include <QLineEdit>

namespace Visus {

////////////////////////////////////////////////////////////////////
class VISUS_GUI_API IsoContour : public GLMesh
{
public:

  VISUS_NON_COPYABLE_CLASS(IsoContour)

  Array                       field;
  Array                       second_field; //this is used to color the surface 
  Range                       range;        //field range
  Array                       voxel_used;   // 1 if a voxel contributes to the isosurface; 0 otherwise

  //cosntructor
  IsoContour() {
  }

};

///////////////////////////////////////////////////////////////////////////////////////
class VISUS_GUI_API MarchingCube
{
public:


  Array    data;
  double   isovalue = 0;
  bool     enable_vortex_used = false;
  int      vertices_per_batch = 16 * 1024;
  Aborted  aborted;

  //constructor
  MarchingCube(Array data_,double isovalue_, Aborted aborted_=Aborted())
    : data(data_),isovalue(isovalue_),aborted(aborted_) {
  }

  //run
  SharedPtr<IsoContour> run();

};

////////////////////////////////////////////////////////////////////
class VISUS_GUI_API IsoContourNode : public Node
{
public:

  VISUS_NON_COPYABLE_CLASS(IsoContourNode)

  //constructor
  IsoContourNode();

  //destructor
  virtual ~IsoContourNode();

  //getTypeName
  virtual String getTypeName() const  override {
    return "IsoContourNode";
  }

  //processInput
  virtual bool processInput() override;

  //getLastFieldRange
  Range getLastFieldRange() const {
    return last_field_range;
  }

  //setField
  void setField(Array value) {
    getInputPort("array")->writeValue(std::make_shared<Array>(value));
    dataflow->needProcessInput(this);
  }

  //getIsoValue
  double getIsoValue() const {
    return isovalue;
  }

  //setIsoValue
  void setIsoValue(double value) {
    setProperty("SetIsoValue", this->isovalue, value);
  }

  //createEditor
  virtual void createEditor();

public:

  //execute
  virtual void execute(Archive& ar) override;

  //write
  virtual void write(Archive& ar) const override;

  //read
  virtual void read(Archive& ar) override;

private:

  class MyJob;

  Range last_field_range;

  double isovalue=0;

  //modelChanged
  virtual void modelChanged() override {
    if (dataflow)
      dataflow->needProcessInput(this);
  }

  //messageHasBeenPublished
  virtual void messageHasBeenPublished(DataflowMessage msg) override;

}; //end class



} //namespace Visus

#endif //VISUS_ISOCONTOUR_BUILD_NODE_H


