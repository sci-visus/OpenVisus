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

#include <Visus/StringTree.h>
#include <Visus/StringUtils.h>
#include <Visus/Log.h>
#include <Visus/Path.h>
#include <Visus/Utils.h>
#include <Visus/ApplicationInfo.h>

#include <tinyxml.h>

#include <algorithm>
#include <iomanip>

namespace Visus {

class XmlPimpl
{
public:

  //encode
  static String encode(const StringTree& stree)
  {
    TiXmlDocument* xmldoc=new TiXmlDocument();
    xmldoc->LinkEndChild(new TiXmlDeclaration( "1.0", "", "" ));
    xmldoc->LinkEndChild(XmlEncodeStringTree(stree));
    TiXmlPrinter printer;
    printer.SetIndent("\t");
    printer.SetLineBreak("\n");
    xmldoc->Accept(&printer);
    String ret(printer.CStr());
    delete xmldoc;
    return StringUtils::trim(StringUtils::replaceAll(ret,"<?xml version=\"1.0\" ?>",""));
  }


  //decode
  static bool decode(StringTree& dst,const String& src)
  {
    if (src.empty())
    {
      VisusWarning()<<"XmlEncoder::decodeStringTree failed because of empty content";
      return false;
    }

    TiXmlDocument xmldoc;
    xmldoc.Parse(src.c_str());
    if (xmldoc.Error())
    {
      VisusWarning()<<"Failed XmlEncoder::decodeStringTree"
        <<" ErrorRow("<<xmldoc.ErrorRow()<<")"
        <<" ErrorCol("<<xmldoc.ErrorCol()<<")"
        <<" ErrorDesc("<<xmldoc.ErrorDesc()<<")";
      return false;
    }

    dst=XmlDecodeStringTree(xmldoc.FirstChildElement());
    return true;
  }

private:

  //XmlEncodeStringTree
  static TiXmlElement* XmlEncodeStringTree(const StringTree& src)
  {
    TiXmlElement* dst=new TiXmlElement(src.name.c_str());

    for (auto it=src.attributes.begin();it!=src.attributes.end();it++)
    {
      String key   =it->first;
      String value =it->second;
      dst->SetAttribute(key.c_str(),value.c_str());
    }

    for (int I=0;I<src.getNumberOfChilds();I++)
    {
      const StringTree& child=src.getChild(I);
      if (child.isCDataSectionNode())
      {
        VisusAssert(child.attributes.size()==1 && child.attributes.hasValue("value") && child.getNumberOfChilds()==0);
        String text=child.readString("value");
        TiXmlText * ti_xml_text = new TiXmlText(text.c_str());
        ti_xml_text->SetCDATA(true); 
        dst->LinkEndChild(ti_xml_text);
      }
      else if (child.isTextNode())
      {
        VisusAssert(child.attributes.size()==1 && child.attributes.hasValue("value") && child.getNumberOfChilds()==0);
        String text=child.readString("value");
        TiXmlText * ti_xml_text = new TiXmlText(text.c_str());
        ti_xml_text->SetCDATA(false); 
        dst->LinkEndChild(ti_xml_text);
      }
      else
      {
        dst->LinkEndChild(XmlEncodeStringTree(src.getChild(I)));
      }
    }

    return dst;
  }


  //XmlDecodeStringTree
  static StringTree XmlDecodeStringTree(TiXmlElement* src)
  {
    StringTree dst(src->Value());

    for (TiXmlAttribute* attr=src->FirstAttribute();attr;attr=attr->Next())
      dst.writeString(attr->Name(),attr->Value());

    //xml_text                                              
    if ( const TiXmlNode* child = src->FirstChild() )
    {
      if (const TiXmlText* child_text = child->ToText())
      {
        if (const char* xml_text=child_text->Value())
        {
          if (child_text->CDATA())
            dst.addCDataSectionNode(xml_text);
          else
            dst.addTextNode(xml_text);
        }
      }
      else if (const TiXmlComment* child_comment = child->ToComment())
      {
        dst.addCommentNode(child_comment->Value());
      }
    }

    for (TiXmlElement* child=src->FirstChildElement();child;child=child->NextSiblingElement()) 
      dst.addChild(XmlDecodeStringTree(child));

    return dst;
  }

};

} //namespace Visus


namespace Visus {

//////////////////////////////////////////////////////////////////////////////////////////////////
class StringTreePostProcess
{
private:

  //acceptAlias
  /* Example:
    <alias key='server' value='atlantis.sci.utah.edu'/>
  */
  static StringMap acceptAlias(StringTree& dst,StringTree& src,std::map<String,StringTree*> templates,StringMap aliases)
  {
    String key  =src.readString("key");
    String value=resolveAliases(src.readString("value"),aliases);
    VisusAssert(!key.empty() && !value.empty());
    aliases.setValue(key,value);

    //an alias can have childs too!
    for (int I=0;I<(int)src.getNumberOfChilds();I++)  
    {
      dst.addChild(StringTree());
      accept(dst.getLastChild(),src.getChild(I),templates,aliases);
    }

    return aliases;
  }

  //resolveAliases
  /* Example:
    <dataset url='$(server)' />
  */
  static String resolveAliases(String value,const StringMap& aliases)
  {
    if (!StringUtils::contains(value,"$"))
      return value;

    for (auto it=aliases.begin();it!=aliases.end();it++)
      value=StringUtils::replaceAll(value,"$(" + it->first + ")",it->second);

    //VisusAssert(!StringUtils::contains(value,"$")); 
    return value;
  }

  //acceptDefineTemplate
  /* Example:
  
      <template name='myname'>
        ...
      </template>
  */
  static std::map<String,StringTree*> acceptDefineTemplate(StringTree& dst,StringTree& src,std::map<String,StringTree*> templates,StringMap aliases)
  {
    String template_name=src.readString("name"); 
    VisusAssert(!template_name.empty());
    templates[template_name]=&src;
    return templates;
  }

  //acceptCallTemplate
  /* Example:
    <call template="myname" />
  */
  static void acceptCallTemplate(StringTree& dst,StringTree& src,std::map<String,StringTree*> templates,StringMap aliases)
  {
    VisusAssert(src.getNumberOfChilds()==0); //not supported child of calls
    String template_name=src.readString("template");
    VisusAssert(!template_name.empty());
    VisusAssert(templates.find(template_name)!=templates.end());

    //expand the template corrent into dst
    for (int I=0;I<(int)templates[template_name]->getNumberOfChilds();I++) 
    {
      dst.addChild(StringTree());
      accept(dst.getLastChild(),templates[template_name]->getChild(I),templates,aliases);
    }
  }

  //acceptInclude
  /* Example:
    <include url="..." />
  */
  static void acceptInclude(StringTree& dst,StringTree& src,std::map<String,StringTree*> templates,StringMap aliases)
  {
    VisusAssert(src.getNumberOfChilds()==0); //<include> should not have any childs!
    String url=src.readString("url");
    VisusAssert(!url.empty());

    StringTree inplace;
    if (!inplace.loadFromXml(Utils::loadTextDocument(url)))
    {
      VisusInfo()<<"cannot load document "<<url;
      VisusAssert(false);
      return;
    }

    //'explode' all childs inside the current context
    if (inplace.name == "visus")
    {
      VisusAssert(inplace.attributes.empty());

      for (auto child : inplace.getChilds())
      {
        dst.addChild(StringTree());
        accept(dst.getLastChild(), *child, templates, aliases);
      }
    }
    else
    {
      accept(dst, inplace, templates, aliases);
    }
  }

  //acceptIf
  /* Example: 
    
  <if condition=''>
    <then>
      ...
    </then>
    ....
    <else>
      ...
    </else>
  </if>
  */
  static void acceptIf(StringTree& dst, StringTree& src, std::map<String, StringTree*> templates, StringMap aliases)
  {
    String condition_expr = src.readString("condition"); VisusAssert(!condition_expr.empty());
    
    //TODO: mini parser here?
    bool bCondition = false;
    if      (condition_expr ==  "win"  ) bCondition = ApplicationInfo::platform_name == "win";
    else if (condition_expr == "!win"  ) bCondition = ApplicationInfo::platform_name != "win";
    else if (condition_expr ==  "osx"  ) bCondition = ApplicationInfo::platform_name == "osx";
    else if (condition_expr == "!osx"  ) bCondition = ApplicationInfo::platform_name != "osx";
    else if (condition_expr ==  "ios"  ) bCondition = ApplicationInfo::platform_name == "ios";
    else if (condition_expr == "!ios"  ) bCondition = ApplicationInfo::platform_name != "ios";
    else if (condition_expr ==  "linux") bCondition = ApplicationInfo::platform_name == "linux";
    else if (condition_expr == "!linux") bCondition = ApplicationInfo::platform_name != "linux";
    else bCondition = cbool(condition_expr);

    for (auto child : src.getChilds())
    {
      if (child->name == "then")
      {
        if (bCondition)
        {
          for (auto then_child : child->getChilds())
          {
            StringTree tmp;
            accept(tmp, *then_child, templates, aliases);
            dst.addChild(tmp);
          }
        }
      }
      else if (child->name == "else")
      {
        if (!bCondition)
        {
          for (auto else_child : child->getChilds())
          {
            StringTree tmp;
            accept(tmp, *else_child, templates, aliases);
            dst.addChild(tmp);
          }
        }
        continue;
      }
      else
      {
        VisusAssert(false);
      }


    }
  }

  //accept
  static void accept(StringTree& dst,StringTree& src,std::map<String,StringTree*> templates,StringMap aliases)
  {
    if (src.name=="alias")
    {
      aliases=acceptAlias(dst,src,templates,aliases);
      return;
    }

    if (src.name=="template")
    {
      templates=acceptDefineTemplate(dst,src,templates,aliases);
      return;
    }

    if (src.name=="call")
    {
      acceptCallTemplate(dst,src,templates,aliases);
      return;
    }

    if (src.name=="include")
    {
      acceptInclude(dst,src,templates,aliases);
      return;
    }

    if (src.name == "if")
    {
      acceptIf(dst, src, templates, aliases);
      return;
    }

    //recursive
    {
      //name
      dst.name=src.name;

      //attributes
      for (auto it : src.attributes)
      {
        String key   = it.first;
        String value = resolveAliases(it.second,aliases);
        dst.writeString(key,value);
      }

      //childs
      for (int I=0;I<(int)src.getNumberOfChilds();I++) 
      {
        StringTree& child=src.getChild(I);

        if (child.name=="alias")
        {
          aliases=acceptAlias(dst,child,templates,aliases);
          continue;
        }

        if (child.name=="template")
        {
          templates=acceptDefineTemplate(dst,child,templates,aliases);
          continue;
        }

        if (child.name=="call")
        {
          acceptCallTemplate(dst,child,templates,aliases);
          continue;
        }

        if (child.name=="include")
        {
          acceptInclude(dst,child,templates,aliases);
          continue;
        }

        if (child.name == "if")
        {
          acceptIf(dst, child, templates, aliases);
          continue;
        }

        dst.addChild(StringTree());
        accept(dst.getLastChild(),child,templates,aliases);
      }
    }

  }

public:

  //exec
  static StringTree exec(StringTree& src)
  {
    std::map<String,StringTree*> templates;
    StringMap      aliases;
    aliases.setValue("VisusHome"              , KnownPaths::VisusHome                  .toString());
    aliases.setValue("CurrentApplicationFile" , KnownPaths::CurrentApplicationFile .toString());
    aliases.setValue("CurrentWorkingDirectory", KnownPaths::CurrentWorkingDirectory.toString());

    StringTree dst;
    accept(dst,src,templates,aliases);
    return dst;
  }

};

/////////////////////////////////////////////////
StringTree StringTree::postProcess(const StringTree& src)
{
  auto dst=StringTreePostProcess::exec(const_cast<StringTree&>(src));
  //VisusInfo() << dst.toString();
  return dst;
}

/////////////////////////////////////////////////
String StringTree::toString() const
{
  return XmlEncoder().encode(this);
}

// This function has been copied from
// commit 13d2ba14a5a8f3cd1c364c21c078575c40d0ba3d
// see ScriptingEngine::encodeObject
String StringTree::encodeObjectToJSON(const Object* obj_, int nrec) const
{
  auto formatString = [](String str)
  {
    String nStr = str;
    for (size_t i = 0; i<nStr.size(); i++)
    {
      String sreplace = "";
      bool replace = true;
      
      switch (nStr[i])
      {
        case '\\': sreplace = "\\\\";    break;
        case '\n': sreplace = "\\n\\\n"; break;
        case '\r': sreplace = "\\r";     break;
        case '\a': sreplace = "\\a";     break;
        case '"':  sreplace = "\\\"";    break;
        default:
        {
          int nCh = ((int)nStr[i]) & 0xFF;
          if (nCh<32 || nCh>127)
          {
            char buffer[5];
#if VISUS_WIN
            sprintf_s(buffer, 5, "\\x%02X", nCh);
#else
            snprintf(buffer, 5, "\\x%02X", nCh);
#endif
            sreplace = buffer;
          }
          else
            replace = false;
        }
      }
      
      if (replace)
      {
        nStr = nStr.substr(0, i) + sreplace + nStr.substr(i + 1);
        i += sreplace.length() - 1;
      }
    }
    return "\"" + nStr + "\"";
  };
  
  Object* obj = const_cast<Object*>(obj_);
  
  //null obj
  if (!obj)
    return "";
  
  //list
  if (ListObject* list = dynamic_cast<ListObject*>(obj))
  {
    const String tab(nrec * 2, ' ');
    std::ostringstream out;
    out << "[\n";
    int len = (int)list->size();
    for (int i = 0; i<len; i++)
    {
      SharedPtr<Object> item = list->getAt(i);
      if (!item)
        out << tab << " NullObject";
      else
        out << tab << "  " << encodeObjectToJSON(item.get(), nrec + 1);
      if (i<len - 1) out << ",\n";
    }
    out << "\n" << tab << "]";
    return out.str();
  }
  
  //dict
  if (DictObject* dict = dynamic_cast<DictObject*>(obj))
  {
    const String tab(nrec * 2, ' ');
    std::ostringstream out;
    out << "{ \n";
    
    //I want the reorder the keys
    std::deque<String> keys;
    for (auto it = dict->begin(); it != dict->end(); it++)
      keys.push_back(it->first);
    
    auto it = std::find(keys.begin(), keys.end(), "name");
    if (it != keys.end())
    {
      keys.erase(it); keys.push_front("name");
    }
    
    for (int I = 0; I<(int)keys.size(); I++)
    {
      out << tab << "  " << formatString(keys[I]) << " : " << encodeObjectToJSON(dict->getattr(keys[I]).get(), nrec + 1);
      if (I != (keys.size() - 1)) out << ",\n";
    }
    out << "\n" << tab << "}";
    return out.str();
  }
  
  //    if (obj == NullObject.get())
  //      return obj->toString();
  //
  //    if (obj == UndefinedObject.get())
  //      return obj->toString();
  
  if (dynamic_cast<IntObject*>(obj))
    return obj->toString();
  
  if (dynamic_cast<DoubleObject*>(obj))
    return obj->toString();
  
  if (dynamic_cast<StringObject*>(obj))
    return formatString(obj->toString());
  
  //    if (ScriptingFunction* fn = dynamic_cast<ScriptingFunction*>(obj))
  //    {
  //      std::ostringstream out;
  //      out << "function (";
  //      int N = 0; for (auto it = fn->argnames.begin(); it != fn->argnames.end(); it++, N++)
  //        out << (N ? "," : "") << (*it);
  //      out << ") " << fn->body;
  //      return out.str();
  //    }
  
  //Object->StringTree->JSON dictionary
  SharedPtr<StringTree> stree(StringTreeEncoder().encode(obj));
  SharedPtr<Object>     json = Utils::convertStringTreeToDictObject(stree.get());
  return encodeObjectToJSON(json.get());
  
}

/////////////////////////////////////////////////
String StringTree::toJSONString() const
{
  return encodeObjectToJSON(this);
}

/////////////////////////////////////////////////
String StringTree::collapseTextAndCData() const
{
  std::ostringstream out;
  for (int I=0;I<childs.size();I++)
  {
    if      (childs[I]->isTextNode())         out<<childs[I]->readString("value");
    else if (childs[I]->isCDataSectionNode()) out<<childs[I]->readString("value");
  }
  return out.str();
}

/////////////////////////////////////////////////
StringTree& StringTree::operator=(const StringTree& other)
{
  this->name      =other.name;
  this->attributes=other.attributes;
  this->childs.clear();
  for (int I=0;I<other.childs.size();I++)
    this->childs.push_back(std::make_shared<StringTree>(*other.childs[I]));
  return *this;
}


/////////////////////////////////////////////////
bool StringTree::loadFromXml(String content,bool bEnablePostProcessing)
{
  if (content.empty())
    return false;

  StringTree tmp;
  if (!XmlEncoder().internalDecode(tmp, content,bEnablePostProcessing))
    return false;
    
  *this=tmp;
  return true;
}

/////////////////////////////////////////////////
String StringTree::readString(String key,String default_value) const
{
  if (StringUtils::contains(key, "/"))
  {
    std::vector<String> vkey = StringUtils::split(key, "/");
    if (vkey.empty())
    {
      VisusAssert(false);
      return default_value;
    }
    StringTree* cursor = const_cast<StringTree*>(this);
    for (int I = 0; cursor && I < (int)vkey.size() - 1; I++)
      cursor = cursor->findChildWithName(vkey[I]);

    return cursor ? cursor->readString(vkey.back(), default_value) : default_value;
  }
  else
  {
    VisusAssert(!key.empty());
    return attributes.getValue(key,default_value);
  }
}

/////////////////////////////////////////////////
void StringTree::writeString(String key,String value)
{
  if (StringUtils::contains(key, "/"))
  {
    std::vector<String> vkey = StringUtils::split(key, "/");
    if (vkey.empty())
    {
      VisusAssert(false); return;
    }
    StringTree* cursor = const_cast<StringTree*>(this),*next=nullptr;
    for (int I = 0; I < (int)vkey.size() - 1; I++)
    {
      next = cursor->findChildWithName(vkey[I]);
      if (!next)
        next = cursor->addChild(StringTree(vkey[I]));
      cursor = next;
    }
    cursor->writeString(vkey.back(), value);
  }
  else
  {
    VisusAssert(!key.empty());
    this->attributes.setValue(key,value);
  }
}

/////////////////////////////////////////////////
StringTree* StringTree::findChildWithName(String name,StringTree* prev) const
{
  if (StringUtils::contains(name, "/"))
  {
    std::vector<String> vname = StringUtils::split(name, "/");
    if (vname.empty())
    {
      VisusAssert(false);
      return nullptr;
    }
    StringTree* cursor = const_cast<StringTree*>(this);
    for (int I = 0; cursor && I < (int)vname.size() - 1; I++)
      cursor = cursor->findChildWithName(vname[I]);

    return cursor ? cursor->findChildWithName(vname.back(), prev) : nullptr;
  }
  else
  {
    int I=0;

    if (prev)
    {
      for (;I<(int)childs.size();I++)
      {
        if (childs[I].get()==prev) 
        {
          I++;
          break;
        }
      }
    }

    for (;I<(int)childs.size();I++)
    {
      if (childs[I]->name==name) 
        return const_cast<StringTree*>(childs[I].get());
    }

    return NULL; //not found
  }
}

/////////////////////////////////////////////////
std::vector<StringTree*> StringTree::findAllChildsWithName(String name,bool bRecursive) const
{
  std::vector<StringTree*> ret;
  for (int I=0;I<(int)childs.size();I++)
  {
    if (name.empty() || childs[I]->name==name) ret.push_back(const_cast<StringTree*>(childs[I].get()));
    if (bRecursive)
    {
      std::vector<StringTree*> sub=childs[I]->findAllChildsWithName(name,true);
      ret.insert(ret.end(),sub.begin(),sub.end());
    }
  }
  return ret;
}

/////////////////////////////////////////////////
int StringTree::getMaxDepth()
{
  int ret=0;
  for (int I=0;I<getNumberOfChilds();I++)
    ret=std::max(ret,1+getChild(I).getMaxDepth());
  return ret;
}

/////////////////////////////////////////////////
void StringTree::writeToObjectStream(ObjectStream& ostream)
{
  StringTree* dst=ostream.getCurrentContext();
  StringTree* src=this;
  VisusAssert(dst->empty() && (dst->name.empty() || dst->name==src->name));
  *dst=*src;
}

/////////////////////////////////////////////////
void StringTree::readFromObjectStream(ObjectStream& istream)
{
  StringTree* dst=this;
  StringTree* src=istream.getCurrentContext();
  VisusAssert(dst->empty() && (dst->name.empty() || dst->name==src->name));
  *dst=*src;
}


/////////////////////////////////////////////////
StringTree* StringTreeEncoder::encode(Object* obj)
{
  //invalid arg
  if (!obj)
    return nullptr;

  //already a StringTree
  if (StringTree* stree=dynamic_cast<StringTree*>(obj))
  {
    VisusAssert(stree->readString("decoded_typename").empty());
    return new StringTree(*stree);
  }
  //create a StringTree by serializing the object
  else
  {
    String decoded_typename=ObjectFactory::getSingleton()->getPortableTypeName(*obj);
    UniquePtr<StringTree> ret(new StringTree(decoded_typename));
    ret->writeString("decoded_typename",decoded_typename); 
    ObjectStream ostream(*ret,'w');
    obj->writeToObjectStream(ostream);
    return ret.release();
  }
}


////////////////////////////////////////////////////////////
VISUS_NEWOBJECT(Object*) StringTreeEncoder::decode(StringTree* stree) 
{
  //invalid arg
  if (!stree || stree->empty())
    return nullptr;

  //see StringTree::create(Object*)
  String decoded_typename=stree->readString("decoded_typename",stree->name);
  if (decoded_typename.empty())
  {
    return new StringTree(*stree);
  }
  else
  {
    UniquePtr<Object> ret(ObjectFactory::getSingleton()->createInstance(decoded_typename));
    if (!ret) return nullptr;
    ObjectStream istream(*stree,'r');
    ret->readFromObjectStream(istream);
    return ret.release();
  }
}



////////////////////////////////////////////////////////////
String XmlEncoder::encode(const Object* obj)
{
  UniquePtr<StringTree> stree(StringTreeEncoder().encode(const_cast<Object*>(obj)));

  if (!stree) 
    return "";

  return XmlPimpl::encode(*stree);
}


////////////////////////////////////////////////////////////
bool XmlEncoder::internalDecode(StringTree& dst,const String& src,bool bEnablePostProcessing)
{
  if (src.empty())
  {
    VisusWarning()<<"XmlEncoder::decodeStringTree failed because of empty content";
    return false;
  }

  if (!XmlPimpl::decode(dst,src))
    return false;

  if (bEnablePostProcessing)
    dst=StringTree::postProcess(dst);

  return true;
}

////////////////////////////////////////////////////////////
SharedPtr<Object> XmlEncoder::decode(const String& content)
{
  StringTree stree;
  if (!internalDecode(stree,content))
    return SharedPtr<Object>();

  return SharedPtr<Object>(StringTreeEncoder().decode(&stree));
}


} //namespace Visus






