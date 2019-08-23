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

#include <Visus/Annotation.h>


namespace Visus {

/////////////////////////////////////////////////////////////////////////////
class SVGParser
{
public:

  StringTree* tree;
  std::vector< SharedPtr< Annotation> > annotations;

  //constructor
  SVGParser(StringTree* tree_) : tree(tree_){
  }

  //doParse
  void doParse()
  {
    for (auto child : tree->getChilds())
      acceptGeneric(child, StringMap());
  }

  //parsePoint
  static Point2d parsePoint(String s)
  {
    auto v = StringUtils::split(s, ",");
    v.resize(2, "0.0");
    return Point2d(cdouble(v[0]), cdouble(v[1]));
  }

  //parsePoints
  static std::vector<Point2d> parsePoints(String s)
  {
    std::vector<Point2d> ret;
    for (auto it : StringUtils::split(s, " "))
      ret.push_back(parsePoint(it));
    return ret;
  }

  //parseColor
  static Color parseColor(const StringMap& attributes, String name)
  {
    auto color = Color::parseFromString(attributes.getValue(name));
    auto A = cdouble(attributes.getValue(name + "-opacity", "1.0"));
    return color.withAlpha(float(A));
  }

  //acceptGeneric
  void acceptGeneric(StringTree* cur, StringMap attributes)
  {
    if (cur->name == "#comment")
      return;

    //attributes
    for (auto it : cur->attributes)
    {
      auto key = it.first;
      auto value = it.second;
      attributes.setValue(key, value);
    }

    if (cur->name == "g")
      return acceptGroup(cur, attributes);

    if (cur->name == "poi")
      return acceptPoi(cur, attributes);

    if (cur->name == "polygon")
      return acceptPolygon(cur, attributes);

    ThrowException("not supported");
  }

  //acceptGroup
  void acceptGroup(StringTree* cur, const StringMap& attributes)
  {
    for (auto child : cur->getChilds())
      acceptGeneric(child, attributes);
  }

  //acceptPoi
  void acceptPoi(StringTree* cur, const StringMap& attributes)
  {
    auto poi = std::make_shared<PointOfInterest>();
    poi->point = parsePoint(attributes.getValue("point"));
    poi->text = attributes.getValue("text");
    poi->magnet_size = cint(attributes.getValue("magnet-size"));
    poi->stroke = parseColor(attributes, "stroke");
    poi->stroke_width = cint(attributes.getValue("stroke-width"));
    poi->fill = parseColor(attributes, "fill");
    this->annotations.push_back(poi);
  }

  //acceptPolygon
  void acceptPolygon(StringTree* cur, const StringMap& attributes)
  {
    auto polygon = std::make_shared<PolygonAnnotation>();
    polygon->points = parsePoints(attributes.getValue("points"));
    polygon->stroke = parseColor(attributes, "stroke");
    polygon->stroke_width = cint(attributes.getValue("stroke-width"));
    polygon->fill = parseColor(attributes, "fill");
    this->annotations.push_back(polygon);
  }

};


std::vector< SharedPtr< Annotation> > ParseAnnotations(StringTree* cur)
{
  SVGParser parser(cur);
  parser.doParse();
  return parser.annotations;
}

} //namespace Visus