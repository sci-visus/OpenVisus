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

#ifndef XIDX_DATA_SOURCE_H_
#define XIDX_DATA_SOURCE_H_

#include <Visus/Utils.h>

namespace Visus{

///////////////////////////////////////////////////////////////////////
class VISUS_XIDX_API DataSource : public XIdxElement 
{
public:

  VISUS_XIDX_CLASS(DataSource)

  String url;
  bool   use_cdata = false;

  //constructor
  DataSource(String name_= "", String url_ = "")
    : XIdxElement(name_),url(url_){
  }
  
  //getXPathPrefix
  virtual String getXPathPrefix() override {
    return StringUtils::format() << XIdxElement::getXPathPrefix() << "[@Name=\"" + this->name + "\"]";
  };
  
public:
  
  //writeToObjectStream
  virtual void writeToObjectStream(ObjectStream& ostream) override
  {
    XIdxElement::writeToObjectStream(ostream);
    ostream.writeInline("Url", url);
    // TODO write content only if datasource is "inline"
    //writeUrlContent(ostream);

  }

  //readFromObjectStream
  virtual void readFromObjectStream(ObjectStream& istream) override {
    XIdxElement::readFromObjectStream(istream);
    this->url  = istream.readInline("Url");
  }

private:

  //writeUrlContent
  void writeUrlContent(ObjectStream& ostream)
  {
    auto content = Utils::loadTextDocument(this->url);
    if (content.empty())
      ThrowException(StringUtils::format() << "Unable to read file" << url);

    //write the content in another file
    if (use_cdata)
    {
      ostream.writeText(content, /*cdata*/true);
      return;
    }

    //must be xml data
    StringTree stree;
    if (!stree.fromXmlString(content))
      ThrowException("Invalid xml data");

    ostream.getCurrentContext()->addChild(stree);
  }

};

}; //namespace

#endif //XIDX_DATA_SOURCE_H_


