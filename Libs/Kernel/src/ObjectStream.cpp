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

#include <Visus/ObjectStream.h>
#include <Visus/StringTree.h>

namespace Visus {

/////////////////////////////////////////////////////////////
void ObjectStream::writeText(const String& value,bool bCData)
{bCData?getCurrentContext()->addCDataSectionNode(value):getCurrentContext()->addTextNode(value);}

/////////////////////////////////////////////////////////////
String ObjectStream::readText()
{return getCurrentContext()->collapseTextAndCData();}


/////////////////////////////////////////////////////////////
void ObjectStream::open(StringTree& root,int mode)
{
  VisusAssert(mode=='w' || mode=='r');
  VisusReleaseAssert(!root.name.empty());
  this->mode=mode;
  stack=std::stack<StackItem>();
  stack.push(StackItem(&root));
}

/////////////////////////////////////////////////////////////
void ObjectStream::close()
{
  this->mode=0;
  stack=std::stack<StackItem>();
}

/////////////////////////////////////////////////////////////
void ObjectStream::writeInline(String name,String value)
{
  getCurrentContext()->writeString(name,value);
}


/////////////////////////////////////////////////////////////
bool ObjectStream::hasAttribute(String name) {
  return getCurrentContext()->attributes.find(name) != getCurrentContext()->attributes.end();
}

/////////////////////////////////////////////////////////////
String ObjectStream::readInline(String name,String default_value)
{
  if (getCurrentContext()->attributes.find(name)!=getCurrentContext()->attributes.end())
    return getCurrentContext()->readString(name);
  else
    return default_value;
}

/////////////////////////////////////////////////////////////
bool ObjectStream::pushContext(String context_name)
{
  if (mode=='w')
  {
    getCurrentContext()->addChild(StringTree(context_name));
    this->stack.push(StackItem(&getCurrentContext()->getLastChild()));
    return true;
  }
  else
  {
    StackItem stack_item;

    //need to keep track of the current child, first time I'm pushing this context
    if (stack.top().next_child.find(context_name)==stack.top().next_child.end())
    {
      stack_item.context=getCurrentContext()->findChildWithName(context_name,nullptr);
    }
    //...already used, I need to start from the next child
    else
    {
      stack_item.context=stack.top().next_child[context_name];
    }

    //cannot find or finished
    if (!stack_item.context)
      return false;

    //next time I will use the next sibling
    stack.top().next_child[context_name]=getCurrentContext()->findChildWithName(context_name,stack_item.context);
    this->stack.push(stack_item);
    return true;
  }
}

/////////////////////////////////////////////////////////////
bool ObjectStream::popContext(String context_name)
{
  VisusAssert(mode=='w' || mode=='r');
  VisusAssert(getCurrentContext()->name==context_name);
  this->stack.pop();
  return true;
}

/////////////////////////////////////////////////////////////
void ObjectStream::write(String name,String value)
{
  pushContext(name);
  writeInline("value",value);
  popContext(name);
}

/////////////////////////////////////////////////////////////
String ObjectStream::read(String name,String default_value)
{
  if (!pushContext(name))
    return default_value;

  String ret=readInline("value",default_value);
  popContext(name);
  return ret;
}


} //namespace Visus

