/*
 * Copyright (c) 2017 University of Utah
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef xidx_dataITEM_H_
#define xidx_dataITEM_H_

#include <algorithm>
#include <cctype>


namespace Visus{

class Variable;
  
/////////////////////////////////////////////////////////////////////////
class VISUS_XIDX_API Endianess
{
public:

  enum Value
  {
    LITTLE_ENDIANESS = 0,
    BIG_ENDIANESS = 1,
    NATIVE_ENDIANESS = 2,
    END_ENUM
  };

  Value value;

  //constructor
  Endianess(Value value_ = (Value)0) : value(value_) {
  }

  //fromString
  static Endianess fromString(String value) {
    for (int I = 0; I < END_ENUM; I++) {
      Endianess ret((Value)I);
      if (ret.toString() == value)
        return ret;
    }
    ThrowException("invalid enum value");
    return Endianess();
  }

  //toString
  String toString() const {
    switch (value)
    {
    case LITTLE_ENDIANESS:    return "Little";
    case BIG_ENDIANESS:       return "Big";
    case NATIVE_ENDIANESS:    return "Native";
    default:                  return "[Unknown]";
    }
  }

  //operator==
  bool operator==(Value other) const {
    return value == other;
  }
  

};

/////////////////////////////////////////////////////////////////////////
class VISUS_XIDX_API FormatType
{
public:

  enum Value {
    XML_FORMAT = 0,
    HDF_FORMAT = 1,
    BINARY_FORMAT = 2,
    TIFF_FORMAT = 3,
    IDX_FORMAT = 4,
    END_ENUM
  };

  Value value;

  //constructor
  FormatType(Value value_ = (Value)0) : value(value_) {
  }

  //fromString
  static FormatType fromString(String value) {
    for (int I = 0; I < END_ENUM; I++) {
      FormatType ret((Value)I);
      if (ret.toString() == value)
        return ret;
    }
    ThrowException("invalid enum value");
    return FormatType();
  }

  //toString
  String toString() const {
    switch (value)
    {
    case XML_FORMAT:      return "XML";
    case HDF_FORMAT:      return "HDF";
    case BINARY_FORMAT:   return "Binary";
    case TIFF_FORMAT:     return "TIFF";
    case IDX_FORMAT:      return "IDX";
    default:              return "[Unknown]";
    }
  }

  //operator==
  bool operator==(Value other) const {
    return value == other;
  }

};
  
/////////////////////////////////////////////////////////////////////////
class VISUS_XIDX_API DataItem : public XIdxElement
{
public:

  VISUS_CLASS(DataItem)
  
  //node infos
  std::vector<int>                    dimensions;
  String                              reference;
  Endianess                           endian_type;
  FormatType                          format_type;
  DType                               dtype = DTypes::FLOAT32;
  String                              text;

  //scrgiorgio: special case for embedded values, should we create another element for this?
  std::vector<double>                 values;

  //down
  std::vector<Attribute*> attributes;
  DataSource*             data_source=nullptr;
  
  //constructor
  DataItem(String name_="") : XIdxElement(name_){
  }

  //constructor
  DataItem(DType dtype_) : XIdxElement(""){
    dtype=dtype_;
  }

  //constructor
  DataItem(FormatType format, DType dtype_, DataSource* VISUS_DISOWN(data_source)){
    this->format_type=format;
    this->dtype=dtype_;
    setDataSource(data_source);
  }

  //constructor
  DataItem(FormatType format_, DType dtype_, DataSource* VISUS_DISOWN(data_source),  int n_dims, uint32_t *dims) 
    : DataItem(format_, dtype_, data_source)
  {
    for (int i = 0; i < n_dims; i++)
      dimensions.push_back(dims[i]);
  }

  //destructor
  virtual ~DataItem() 
  {
    setDataSource(nullptr);
    for (auto it : attributes)
      delete it;
  }

  //setDataSource
  void setDataSource(DataSource* VISUS_DISOWN(value)) 
  {
    if (this->data_source) {
      removeEdge(this, this->data_source);
      delete this->data_source;
    }

    this->data_source = value;

    if (this->data_source)
      addEdge(this, this->data_source);
  }

  //addAttribute
  void addAttribute(Attribute* VISUS_DISOWN(value))
  {
    addEdge(this, value);
    attributes.push_back(value);
  }
  
  //getVolume
  virtual size_t getVolume() const {
    size_t total = 1;
    for(int i=0; i < dimensions.size(); i++)
      total *= dimensions[i];
    return total;
  }
 
  //setValues
  void setValues(std::vector<double> values,int stride=1) 
  {
    this->dtype = DType::fromString("float64");
    this->values = values;
    this->dimensions.resize(stride);
    this->dimensions[0] = (int)(values.size() / stride);

    if (stride > 1) 
      this->dimensions[1] = stride; 
  }

  //addValue
  void addValue(double val, int stride=1)
  {
    this->values.push_back(val);
    
    this->dimensions.resize(stride);
    this->dimensions[0] = (int)(values.size()/stride);

    if (stride>1)
      this->dimensions[1] = stride;
  }
  
  //getXPathPrefix
  virtual String getXPathPrefix() override 
  {
    if (auto data_source = findDataSource())
      return data_source->getXPathPrefix();
    else
      return XIdxElement::getXPathPrefix();
  }
  
  //findDataSource
  virtual DataSource* findDataSource() 
  {
    // TODO fix getParent(), it returns an XIdxElement 
    for(auto cursor = this->getParent(); cursor; cursor = cursor->getParent())
    {
      if (cursor->getTypeName() == "Group")
      {
        if (auto source = cursor->findChildWithName("DataSource"))
          return dynamic_cast<DataSource*>(source);
      }
    }
    return nullptr;
  }

public:
  
  //write
  virtual void write(Archive& ar) const override
  {
    XIdxElement::write(ar);
    ar.write("Format", format_type.toString());
    ar.write("DType", dtype.toString());
    ar.write("Endian", endian_type.toString());
    ar.write("Dimensions", StringUtils::join(dimensions));

    writeChild<DataSource>(ar, "DataSource", this->data_source);

    for (auto child : this->attributes)
      writeChild<Attribute>(ar, "Attribute", child);

    if (this->values.size())
      ar.writeText(StringUtils::join(this->values));
    if (this->text.size())
      ar.writeText(this->text);
  };

  //read
  virtual void read(Archive& ar) override
  {
    XIdxElement::read(ar);

    this->format_type = FormatType::fromString(ar.readString("Format"));
    this->dtype = DType::fromString(ar.readString("DType"));
    this->endian_type = Endianess::fromString(ar.readString("Endian"));

    for (auto it : StringUtils::split(ar.readString("Dimensions")))
      this->dimensions.push_back(cint(it));

    String s_values;
    ar.readText(s_values);
    for (auto it : StringUtils::split(s_values))
      this->values.push_back(cdouble(it));

    if (auto child = readChild<DataSource>(ar, "DataSource"))
      setDataSource(child);

    for (auto it : readChilds<Attribute>(ar, "Attribute"))
      addAttribute(it);
  }
  
};
  
} //namespace
 
#endif //xidx_dataITEM_H_

