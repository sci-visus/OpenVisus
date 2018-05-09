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

#include <Visus/ModVisus.h>
#include <Visus/Dataset.h>
#include <Visus/Scene.h>
#include <Visus/DatasetFilter.h>
#include <Visus/File.h>
#include <Visus/TransferFunction.h>
#include <Visus/NetService.h>
#include <Visus/VisusConfig.h>
#include <Visus/ApplicationInfo.h>

namespace Visus {

////////////////////////////////////////////////////////////////////////////////
static NetResponse CreateNetResponseError(int status,String errormsg,String file,int line)
{
  errormsg+=" __FILE__("+file+") __LINE__("+cstring(line)+")";
  return NetResponse(status,errormsg);
}

#define NetResponseError(status,errormsg) CreateNetResponseError(status,errormsg,__FILE__,__LINE__)


////////////////////////////////////////////////////////////////////////////////
class PublicDataset
{
public:

  //constructor
  PublicDataset(String name,SharedPtr<Dataset> dataset,String url_template="$(protocol)://$(hostname):$(port)/mod_visus?action=readdataset&dataset=$(name)")
  {
    this->name=name;
    this->url_template=url_template;
    this->url=StringUtils::replaceAll(url_template,"$(name)",name);
    this->dataset=dataset;
    this->dataset_body=getDatasetBody(dataset);
  }

  //getName
  const String& getName() const
  {return name;}

  //getUrl
  const String& getUrl() const
  {return url;}

  //getDataset
  SharedPtr<Dataset> getDataset() const
  {
    ScopedReadLock lock(const_cast<PublicDataset*>(this)->dataset_lock);
    return dataset;
  }

  //getDatasetBody
  String getDatasetBody() const
  {
    ScopedReadLock lock(const_cast<PublicDataset*>(this)->dataset_lock);
    return dataset_body;
  }

  //reloadIfNeeded
  bool reloadIfNeeded()
  {
    //NOTE: i don't have a way to know if the file has changed meanwhile... so this is not very optimized
    auto new_dataset=Dataset::loadDataset(this->getName());
    if (!new_dataset)
      return false;

    String new_dataset_body=getDatasetBody(new_dataset);
    if (new_dataset_body==getDatasetBody())
      return false;

    {
      ScopedWriteLock lock(dataset_lock);
      this->dataset=new_dataset;
      this->dataset_body=new_dataset_body;
    }

    return true;
  }

private:

  VISUS_NON_COPYABLE_CLASS(PublicDataset)

  
  String             name;
  String             url;
  String             url_template;

  RWLock             dataset_lock;
  SharedPtr<Dataset> dataset;
  String             dataset_body;

  //getDatasetBody
  String getDatasetBody(SharedPtr<Dataset> dataset)
  {
    String dataset_body= StringUtils::trim(dataset->getDatasetBody());

    if (dataset_body.empty())
      return dataset->getUrl().toString();

    //special case for IdxMultipleDataset, I need to remap urls
    if (bool bMaybeXml=StringUtils::startsWith(dataset_body,"<"))
    {
      StringTree stree;
      if (stree.loadFromXml(dataset_body)) 
      {
        fixUrls(stree);
        dataset_body=stree.toString();
      }
    }

    return dataset_body;
  }

  //fixUrls
  void fixUrls(StringTree& stree)
  {
    if (stree.name=="dataset" && stree.hasValue("name") && stree.hasValue("url"))
    {
      String name=stree.readString("name");
      String url =stree.readString("url");
      url=StringUtils::replaceAll(this->url_template,"$(name)",this->name+"/"+name);
      stree.writeString("url",url);
    }

    for (int I=0;I<stree.getNumberOfChilds();I++)
      fixUrls(stree.getChild(I));
  }

};


////////////////////////////////////////////////////////////////////////////////
class ModVisus::PublicDatasets
{
public:

  enum BodyFormat
  {
    XmlFormat,
    HtmlFormat,
    JSONFormat
  };

  //constructor
  PublicDatasets() : list("datasets") {
  }

  //getList
  String getList(BodyFormat format, String property="") 
  {
    ScopedReadLock read_lock(this->lock);

    if (!property.empty())
    {
      StringTree filtered_list;
      recursiveFindDatasets(filtered_list,this->list,property);
      return dumpList(format,filtered_list);
    }
    else
    {
      return dumpList(format,this->list);
    }
  }

  //findPublicDataset
  SharedPtr<PublicDataset> findPublicDataset(String name)
  {
    if (!name.empty()) 
    {
      {
        ScopedReadLock read_lock(this->lock);
        auto it=this->map.find(name);
        if (it!=this->map.end())
          return it->second;
      }

      //not found, reload visus.config and try again
      if (bool bReloadVisusConfig=VisusConfig::needReload())
      {
        VisusConfig::reload();
        this->configureDatasets(VisusConfig::storage); //TODO: (optimization for reloads) only add new public datasets by diffing entries in string tree instead of reconfiguring all datasets
        {
          ScopedReadLock read_lock(this->lock);
          auto it=this->map.find(name);
          if (it!=this->map.end())
            return it->second;
        }
      }
    }
    return SharedPtr<PublicDataset>();
  }

  //addPublicDataset
  //NOTE: this works only IIF you don't use alias or templates or any XML processing
  bool addPublicDataset(StringTree& src,bool bPersistent)
  {
    ScopedWriteLock write_lock(this->lock);

    //add src to visus.config in memory (this is necessary for Dataset::loadDataset(name))
    {
      VisusConfig::storage.addChild(src);
    }

    //add src to visus.config on the file system (otherwise when mod_visus restarts it will be lost)
    if (bPersistent)
    {
      String     visus_config_filename=VisusConfig::filename;
      StringTree new_visus_config;
      bool bEnablePostProcessing=false;
      if (!new_visus_config.loadFromXml(Utils::loadTextDocument(visus_config_filename),bEnablePostProcessing))
      {
        VisusWarning()<<"Cannot load visus.config";
        VisusAssert(false);//TODO rollback
        return false;
      }

      new_visus_config.addChild(src);

      if (!Utils::saveTextDocument(visus_config_filename,new_visus_config.toString()))
      {
        VisusWarning()<<"Cannot save new visus.config";
        VisusAssert(false);//TODO rollback
        return false;
      }
    }

    return recursiveAddDatasetsFromStringTree(write_lock,this->list,src)>0;
  }

  //addPublicDataset
  bool addPublicDataset(String name,String url,bool bPersistent)
  {
    StringTree stree("dataset");
    stree.writeString("name",name);
    stree.writeString("url",url);
    stree.writeString("permissions","public");
    return addPublicDataset(stree,bPersistent);
  }

  //configureDatasets
  void configureDatasets(const StringTree& visus_config)
  {
    ScopedWriteLock write_lock(this->lock);
    this->map.clear();
    this->list=StringTree("datasets");
    recursiveAddDatasetsFromStringTree(write_lock,this->list,visus_config);
  }

private:

  VISUS_NON_COPYABLE_CLASS(PublicDatasets)

  typedef std::map<String, SharedPtr<PublicDataset > > Map;

  RWLock            lock;
  Map               map;
  StringTree        list;

  //recursiveFindDatasets
  static int recursiveFindDatasets(StringTree& result,const StringTree& list,const String property="")
  {
    if (list.name=="dataset") 
    {
      bool is_public=StringUtils::contains(list.readString("permissions"),"public"); 
      if (!is_public)
      {
        //VisusWarning()<<"Dataset name("<<name<<") is not public, skipping it";
        return 0;
      }

      String name = list.readString("name");      
      if (name.empty())
      {
        VisusWarning()<<"Dataset name("<<name<<") is not valid, skipping it";
        VisusAssert(false);
        return 0;
      }

      auto dataset=Dataset::loadDataset(name);
      if (!dataset)
      {
        VisusWarning()<<"Dataset::loadDataset("<<name<<") failed, skipping it";
        return 0;
      }

      //filters
      Url url=dataset->getUrl();
      String TypeName;
      if (!url.valid())
        return 0;
      else if (url.isFile())
      {
        String extension=Path(url.getPath()).getExtension();
        TypeName = DatasetPluginFactory::getSingleton()->getRegisteredDatasetType(extension);
        if (TypeName.empty()) 
          return 0;
      }
      else if (StringUtils::contains(url.toString(),"mod_visus"))
      {
        url.setParam("action","readdataset");
      
        auto response=NetService::getNetResponse(url);
        if (!response.isSuccessful())
          return 0;
      
        TypeName = response.getHeader("visus-typename","IdxDataset");
        if (TypeName.empty())
          return 0;

        // backward compatible 
        if (TypeName == "MultipleDataset")
          TypeName="IdxMultipleDataset";
      }
      //legacy dataset (example google maps)
      else if (StringUtils::endsWith(url.getHostname(),".google.com"))
      {
        TypeName = "LegacyDataset";
      }

      if (property.empty())
        result.addChild(list);
      else if (property=="midx" && TypeName=="IdxMultipleDataset")
        result.addChild(list);
      else if (property=="idx" && TypeName=="IdxDataset")
        result.addChild(list);
      else
        VisusInfo()<<"Skipping "<<name<<"(type="<<TypeName<<") because property="<<property;

      return 1;
    }
    //I want to maintain the group hierarchy!
    else if (list.name=="group")
    {
      StringTree body_group(list.name);
      body_group.attributes=list.attributes;
    
      int ret=0;
      for (int I=0;I<(int)list.getNumberOfChilds();I++)
        ret+=recursiveFindDatasets(result,list.getChild(I),property);

      if (ret)
        result.addChild(body_group);

      return ret;
    }
    //flattening the hierarchy!
    else
    {
      int ret=0;
      for (int I=0;I<(int)list.getNumberOfChilds();I++)
        ret+=recursiveFindDatasets(result,list.getChild(I),property);
      return ret;
    }
  }

  //recursiveAddPublicDataset
  void recursiveAddPublicDataset(ScopedWriteLock& write_lock,StringTree& list,SharedPtr<PublicDataset> public_dataset)
  {
    String public_name=public_dataset->getName();

    if (map.find(public_name)!=map.end())
      VisusWarning()<<"Dataset name("<<public_name<<") already exists, overwriting it";

    public_dataset->getDataset()->bServerMode = true;
    this->map[public_name]=public_dataset;

    StringTree* list_child=list.addChild(StringTree("dataset"));
    list_child->attributes=public_dataset->getDataset()->getConfig().attributes; //for example kdquery=true could be maintained!
    list_child->writeString("name",public_name);
    list_child->writeString("url",public_dataset->getUrl()); 

    //automatically add the childs of a multiple datasets
    for (auto it : public_dataset->getDataset()->getInnerDatasets())
    {
      auto child_public_name=public_name+"/"+it.first;
      auto child_dataset=it.second;
      recursiveAddPublicDataset(write_lock,*list_child,std::make_shared<PublicDataset>(child_public_name,child_dataset));
    }
  }

  //recursiveAddDatasetsFromStringTree
  int recursiveAddDatasetsFromStringTree(ScopedWriteLock& write_lock,StringTree& list,const StringTree& src)
  {
    if (src.name=="dataset") 
    {
      bool is_public=StringUtils::contains(src.readString("permissions"),"public"); 
      if (!is_public)
      {
        //VisusWarning()<<"Dataset name("<<name<<") is not public, skipping it";
        return 0;
      }

      String name = src.readString("name");
      
      if (name.empty())
      {
        VisusWarning()<<"Dataset name("<<name<<") is not valid, skipping it";
        VisusAssert(false);
        return 0;
      }

      auto dataset=Dataset::loadDataset(name);
      if (!dataset)
      {
        VisusWarning()<<"Dataset::loadDataset("<<name<<") failed, skipping it";
        return 0;
      }

      recursiveAddPublicDataset(write_lock,list,std::make_shared<PublicDataset>(name,dataset));
      return 1;
    }
    //I want to maintain the group hierarchy!
    else if (src.name=="group")
    {
      StringTree body_group(src.name);
      body_group.attributes=src.attributes;
    
      int ret=0;
      for (int I=0;I<(int)src.getNumberOfChilds();I++)
        ret+=recursiveAddDatasetsFromStringTree(write_lock,body_group,src.getChild(I));

      if (ret)
        list.addChild(body_group);

      return ret;
    }
    //flattening the hierarchy!
    else
    {
      int ret=0;
      for (int I=0;I<(int)src.getNumberOfChilds();I++)
        ret+=recursiveAddDatasetsFromStringTree(write_lock,list,src.getChild(I));
      return ret;
    }
  }

  //getListAsHtmlFormat
  static void getListAsHtmlFormat(std::ostringstream& out,const StringTree& cursor,const String& tab,const String& crlf)
  {
    if (cursor.name=="datasets")
    {
      String s="<h1>Datasets</h1><br>";
      out<<tab<<s<<crlf;
    }
    else if (cursor.name=="group")
    {
      String s="<h2>$(name)</h2><br>";
      s=StringUtils::replaceAll(s,"$(name)",cursor.readString("name"));
      out<<tab<<s<<crlf;
    }
    else if (cursor.name=="dataset")
    {
      String s="<a href='$(url)'>$(name)</a><br>";
      s=StringUtils::replaceAll(s,"$(name)",cursor.readString("name"));
      s=StringUtils::replaceAll(s,"$(url)" ,cursor.readString("url"));
      out<<tab<<s<<crlf;
    }

    for (int I=0;I<(int)cursor.getNumberOfChilds();I++)
      getListAsHtmlFormat(out,cursor.getChild(I),tab+"&nbsp;&nbsp;",crlf);
  }

  //dumpList
  static String dumpList(BodyFormat format,const StringTree& list)
  {
    if (format==HtmlFormat)
    {
      std::ostringstream  out;
      String crlf="\r\n";
      out<<"<HTML>"<<crlf;
      out<<"<HEAD><TITLE>Visus datasets</TITLE>"<<crlf;
      getListAsHtmlFormat(out,list,"",crlf);
      out<<"</HEAD>"<<crlf;
      out<<"<BODY>"<<crlf;
      return out.str();
    }

    if (format==JSONFormat)
    {
      return list.toJSONString();
    }

    return list.toString();
  }

};
  
////////////////////////////////////////////////////////////////////////////////
class PublicScene
{
public:
  
  //constructor
  PublicScene(String name,SharedPtr<Scene> scene,String url_template="$(protocol)://$(hostname):$(port)/mod_visus?action=readscene&scene=$(name)")
  {
    this->name=name;
    this->url_template=url_template;
    this->url=StringUtils::replaceAll(url_template,"$(name)",name);
    this->scene_body=scene->getSceneBody();
  }
  
  //getName
  const String& getName() const
  {return name;}
  
  //getUrl
  const String& getUrl() const
  {return url;}
  
  //getDatasetBody
  String getSceneBody() const
  {
    ScopedReadLock lock(const_cast<PublicScene*>(this)->scene_lock);
    return scene_body;
  }
  
  //reloadIfNeeded
  bool reloadIfNeeded()
  {
    //NOTE: i don't have a way to know if the file has changed meanwhile... so this is not very optimized
    auto new_scene=Scene::loadScene(this->getName());
    if (!new_scene)
      return false;
    
    String new_scene_body=getSceneBody(new_scene);
    if (new_scene_body==getSceneBody())
      return false;
    
    {
      ScopedWriteLock lock(scene_lock);
      this->scene_body=new_scene_body;
    }
    
    return true;
  }
  
private:
  
  VISUS_NON_COPYABLE_CLASS(PublicScene)
  
  String             name;
  String             url;
  String             url_template;
  
  RWLock             scene_lock;
  String             scene_body;
  
  //getDatasetBody
  String getSceneBody(SharedPtr<Scene> scene)
  {
    String scene_body=scene->getSceneBody();
    
    if (scene_body.empty())
      return scene->getUrl().toString();
    
    //special case for IdxMultipleDataset, I need to remap urls
    {
      StringTree stree;
      if (stree.loadFromXml(scene_body))
      {
        fixUrls(stree);
        scene_body=stree.toString();
      }
    }
    
    return scene_body;
  }
  
  //fixUrls
  void fixUrls(StringTree& stree)
  {
    if (stree.name=="scene" && stree.hasValue("name") && stree.hasValue("url"))
    {
      String name=stree.readString("name");
      String url =stree.readString("url");
      url=StringUtils::replaceAll(this->url_template,"$(name)",this->name+"/"+name);
      stree.writeString("url",url);
    }
    
    for (int I=0;I<stree.getNumberOfChilds();I++)
      fixUrls(stree.getChild(I));
  }
  
};

///////////////////////////////////////////////////////////////////////////////
class ModVisus::PublicScenes
{
public:
  
  enum BodyFormat
  {
    XmlFormat,
    HtmlFormat,
    JSONFormat
  };
  
  //constructor
  PublicScenes() : list("scenes") {
  }
  
  //getList
  String getList(BodyFormat format, String property="")
  {
    ScopedReadLock read_lock(this->lock);
    
    if (!property.empty())
    {
      StringTree filtered_list;
      recursiveFindScenes(filtered_list,this->list,property);
      return dumpList(format,filtered_list);
    }
    else
    {
      return dumpList(format,this->list);
    }
  }
  
  //findPublicScene
  SharedPtr<PublicScene> findPublicScene(String name)
  {
    if (!name.empty())
    {
      {
        ScopedReadLock read_lock(this->lock);
        auto it=this->map.find(name);
        if (it!=this->map.end())
          return it->second;
      }

      //not found, reload visus.config and try again
      if (bool bReloadVisusConfig=VisusConfig::needReload())
      {
        VisusConfig::reload();
        this->configureScenes(VisusConfig::storage); //TODO: (optimization for reloads) only add new public datasets by diffing entries in string tree instead of reconfiguring all datasets
        {
          ScopedReadLock read_lock(this->lock);
          auto it=this->map.find(name);
          if (it!=this->map.end())
            return it->second;
        }
      }
    }
    return SharedPtr<PublicScene>();
  }
  
  //configureScenes
  void configureScenes(const StringTree& visus_config)
  {
    ScopedWriteLock write_lock(this->lock);
    this->map.clear();
    this->list=StringTree("scenes");
    recursiveAddSceneFromStringTree(write_lock,this->list,visus_config);
  }

private:
  
  VISUS_NON_COPYABLE_CLASS(PublicScenes)
  
  typedef std::map<String, SharedPtr<PublicScene > > Map;
  
  RWLock            lock;
  Map               map;
  StringTree        list;
  
  //recursiveFindDatasets
  static int recursiveFindScenes(StringTree& result,const StringTree& list,const String property="")
  {
    if (list.name=="dataset")
    {
      bool is_public=StringUtils::contains(list.readString("permissions"),"public");
      if (!is_public)
      {
        //VisusWarning()<<"Dataset name("<<name<<") is not public, skipping it";
        return 0;
      }
      
      String name = list.readString("name");
      if (name.empty())
      {
        VisusWarning()<<"Dataset name("<<name<<") is not valid, skipping it";
        VisusAssert(false);
        return 0;
      }
      
      auto dataset=Dataset::loadDataset(name);
      if (!dataset)
      {
        VisusWarning()<<"Dataset::loadDataset("<<name<<") failed, skipping it";
        return 0;
      }
      
      //filters
      Url url=dataset->getUrl();
      String TypeName;
      if (!url.valid())
        return 0;
      else if (url.isFile())
      {
        String extension=Path(url.getPath()).getExtension();
        TypeName = DatasetPluginFactory::getSingleton()->getRegisteredDatasetType(extension);
        if (TypeName.empty())
          return 0;
      }
      else if (StringUtils::contains(url.toString(),"mod_visus"))
      {
        url.setParam("action","readdataset");
        
        auto response=NetService::getNetResponse(url);
        if (!response.isSuccessful())
          return 0;
        
        TypeName = response.getHeader("visus-typename","IdxDataset");
        if (TypeName.empty())
          return 0;
        
        // backward compatible
        if (TypeName == "MultipleDataset")
          TypeName="IdxMultipleDataset";
      }
      //legacy dataset (example google maps)
      else if (StringUtils::endsWith(url.getHostname(),".google.com"))
      {
        TypeName = "LegacyDataset";
      }
      
      if (property.empty())
        result.addChild(list);
      else if (property=="midx" && TypeName=="IdxMultipleDataset")
        result.addChild(list);
      else if (property=="idx" && TypeName=="IdxDataset")
        result.addChild(list);
      else
        VisusInfo()<<"Skipping "<<name<<"(type="<<TypeName<<") because property="<<property;
      
      return 1;
    }
    //I want to maintain the group hierarchy!
    else if (list.name=="group")
    {
      StringTree body_group(list.name);
      body_group.attributes=list.attributes;
      
      int ret=0;
      for (int I=0;I<(int)list.getNumberOfChilds();I++)
        ret+=recursiveFindScenes(result,list.getChild(I),property);
      
      if (ret)
        result.addChild(body_group);
      
      return ret;
    }
    //flattening the hierarchy!
    else
    {
      int ret=0;
      for (int I=0;I<(int)list.getNumberOfChilds();I++)
        ret+=recursiveFindScenes(result,list.getChild(I),property);
      return ret;
    }
  }
  
  //recursiveAddPublicScene
  void recursiveAddPublicScene(ScopedWriteLock& write_lock,StringTree& list,SharedPtr<PublicScene> public_scene)
  {
    String public_name=public_scene->getName();
    
    if (map.find(public_name)!=map.end())
      VisusWarning()<<"Scene name("<<public_name<<") already exists, overwriting it";
    
    this->map[public_scene->getName()]=public_scene;
    
    StringTree* list_child=list.addChild(StringTree("dataset"));
   // list_child->attributes=public_dataset->getDataset()->getConfig().attributes; //for example kdquery=true could be maintained!
    list_child->writeString("name",public_name);
    list_child->writeString("url",public_scene->getUrl());
  }
  
  //recursiveAddSceneFromStringTree
  int recursiveAddSceneFromStringTree(ScopedWriteLock& write_lock,StringTree& list,const StringTree& src)
  {
    if (src.name=="scene")
    {
      bool is_public=StringUtils::contains(src.readString("permissions"),"public");
      if (!is_public)
      {
        //VisusWarning()<<"Dataset name("<<name<<") is not public, skipping it";
        return 0;
      }
      
      String name = src.readString("name");
      
      if (name.empty())
      {
        VisusWarning()<<"Scene name("<<name<<") is not valid, skipping it";
        VisusAssert(false);
        return 0;
      }
      
      auto scene=Scene::loadScene(name);
      if (!scene)
      {
        VisusWarning()<<"Scene::loadScene("<<scene<<") failed, skipping it";
        return 0;
      }
      
      recursiveAddPublicScene(write_lock,list,std::make_shared<PublicScene>(name,scene));
      return 1;
    }
    //I want to maintain the group hierarchy!
    else if (src.name=="group")
    {
      StringTree body_group(src.name);
      body_group.attributes=src.attributes;
      
      int ret=0;
      for (int I=0;I<(int)src.getNumberOfChilds();I++)
        ret+=recursiveAddSceneFromStringTree(write_lock,body_group,src.getChild(I));
      
      if (ret)
        list.addChild(body_group);
      
      return ret;
    }
    //flattening the hierarchy!
    else
    {
      int ret=0;
      for (int I=0;I<(int)src.getNumberOfChilds();I++)
        ret+=recursiveAddSceneFromStringTree(write_lock,list,src.getChild(I));
      return ret;
    }
  }
  
  //getListAsHtmlFormat
  static void getListAsHtmlFormat(std::ostringstream& out,const StringTree& cursor,const String& tab,const String& crlf)
  {
    if (cursor.name=="datasets")
    {
      String s="<h1>Datasets</h1><br>";
      out<<tab<<s<<crlf;
    }
    else if (cursor.name=="group")
    {
      String s="<h2>$(name)</h2><br>";
      s=StringUtils::replaceAll(s,"$(name)",cursor.readString("name"));
      out<<tab<<s<<crlf;
    }
    else if (cursor.name=="dataset")
    {
      String s="<a href='$(url)'>$(name)</a><br>";
      s=StringUtils::replaceAll(s,"$(name)",cursor.readString("name"));
      s=StringUtils::replaceAll(s,"$(url)" ,cursor.readString("url"));
      out<<tab<<s<<crlf;
    }
    
    for (int I=0;I<(int)cursor.getNumberOfChilds();I++)
      getListAsHtmlFormat(out,cursor.getChild(I),tab+"&nbsp;&nbsp;",crlf);
  }
  
  
  //dumpList
  static String dumpList(BodyFormat format,const StringTree& list)
  {
    if (format==HtmlFormat)
    {
      std::ostringstream  out;
      String crlf="\r\n";
      out<<"<HTML>"<<crlf;
      out<<"<HEAD><TITLE>Visus scenes</TITLE>"<<crlf;
      getListAsHtmlFormat(out,list,"",crlf);
      out<<"</HEAD>"<<crlf;
      out<<"<BODY>"<<crlf;
      return out.str();
    }
    
    if (format==JSONFormat)
    {
      return list.toJSONString();
    }
    
    return list.toString();
  }
  
};


////////////////////////////////////////////////////////////////////////////////
ModVisus::ModVisus()
{
  datasets=std::make_shared<PublicDatasets>();
  scenes=std::make_shared<PublicScenes>();
}

////////////////////////////////////////////////////////////////////////////////
ModVisus::~ModVisus()
{}

////////////////////////////////////////////////////////////////////////////////
bool ModVisus::configureDatasets()
{
  VisusInfo()<<"ModVisus::configureDatasets()...";

  if (VisusConfig::needReload())
    VisusConfig::reload();

  datasets->configureDatasets(VisusConfig::storage);
  scenes->configureScenes(VisusConfig::storage);

  this->verbose = cint(VisusConfig::readString("Configuration/ModVisus/verbose"));
  #ifdef _DEBUG
  this->verbose = std::max(verbose,1);
  #endif

  VisusInfo()<<"/mod_visus?action=list\n"<<datasets->getList(PublicDatasets::XmlFormat);
  VisusInfo()<<"done ModVisus::configureDatasets()";
  VisusInfo()<<"/mod_visus?action=list\n"<<scenes->getList(PublicScenes::XmlFormat);
  VisusInfo()<<"done ModVisus::configureScenes()";
  return true;
}


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleConfigureDatasets(const NetRequest& request)
{
  if (!configureDatasets())
    return NetResponseError(HttpStatus::STATUS_INTERNAL_SERVER_ERROR,"Cannot read the visus.config");
  
  return NetResponse(HttpStatus::STATUS_OK);
}


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleAddDataset(const NetRequest& request)
{
  bool bOk=false;

  bool bPersistent=cbool(request.url.getParam("persistent","true"));

  if (request.url.hasParam("name"))
  {
    String name=request.url.getParam("name");
    String url=request.url.getParam("url");

    if (datasets->findPublicDataset(name))
      return NetResponseError(HttpStatus::STATUS_CONFLICT,"Cannot add dataset(" + name + ") because it already exists");

    bOk=datasets->addPublicDataset(name,url,bPersistent);
  }
  else if (request.url.hasParam("xml"))
  {
    String xml=request.url.getParam("xml");
    StringTree stree;
    if (!stree.loadFromXml(xml))
      return NetResponseError(HttpStatus::STATUS_BAD_REQUEST,"Cannot decode xml");

    String name = stree.readString("name");
    if (datasets->findPublicDataset(name))
      return NetResponseError(HttpStatus::STATUS_CONFLICT,"Cannot add dataset(" + name + ") because it already exists");

    bOk=datasets->addPublicDataset(stree,bPersistent);
  }

  if (!bOk)
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST,"Add dataset failed");

  return NetResponse(HttpStatus::STATUS_OK);
}


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleReadDataset(const NetRequest& request)
{
  String dataset_name=request.url.getParam("dataset");

  SharedPtr<PublicDataset> public_dataset=datasets->findPublicDataset(dataset_name);
  if (!public_dataset)
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND,"Cannot find dataset(" + dataset_name + ")");

  NetResponse response(HttpStatus::STATUS_OK);
  response.setHeader("visus-git-revision", ApplicationInfo::git_revision);
  response.setHeader("visus-typename", ObjectFactory::getSingleton()->getPortableTypeName(*public_dataset->getDataset()));

  auto body=public_dataset->getDatasetBody();
  response.setTextBody(body,/*bHasBinary*/true);
  return response;
}
  
///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleReadScene(const NetRequest& request)
{
  String scene_name=request.url.getParam("scene");
  
  SharedPtr<PublicScene> public_scene=scenes->findPublicScene(scene_name);
  if (!public_scene)
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND,"Cannot find scene(" + scene_name + ")");
  
  NetResponse response(HttpStatus::STATUS_OK);
  response.setHeader("visus-git-revision", ApplicationInfo::git_revision);
//  response.setHeader("visus-typename", ObjectFactory::getSingleton()->getPortableTypeName(*public_dataset->getDataset()));
  
  auto body=public_scene->getSceneBody();
  response.setTextBody(body,/*bHasBinary*/true);
  return response;
}


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleGetListOfDatasets(const NetRequest& request)
{
  String format=request.url.getParam("format","xml");
  String hostname=request.url.getParam("hostname"); //trick if you want $(localhost):$(port) to be replaced with what the client has
  String port=request.url.getParam("port");
  String property=request.url.getParam("property");

  NetResponse response(HttpStatus::STATUS_OK);

  //ensure we're using latest visus.config
  if (VisusConfig::needReload())
    VisusConfig::reload();

  if (format=="xml")
  {
    response.setXmlBody(datasets->getList(PublicDatasets::XmlFormat,property));
  }
  else if (format=="html")
  {
    response.setHtmlBody(datasets->getList(PublicDatasets::HtmlFormat,property));
  }
  else if (format=="json")
  {
    response.setJSONBody(datasets->getList(PublicDatasets::JSONFormat,property));
  }
  else
  {
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND,"wrong format(" + format + ")");
  }

  if (!hostname.empty())
    response.setTextBody(StringUtils::replaceAll(response.getTextBody(),"$(hostname)",hostname));

  if (!port.empty())
    response.setTextBody(StringUtils::replaceAll(response.getTextBody(),"$(port)",port));

  return response;
}

///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleGetListOfScenes(const NetRequest& request)
{
  String format=request.url.getParam("format","xml");
  String hostname=request.url.getParam("hostname"); //trick if you want $(localhost):$(port) to be replaced with what the client has
  String port=request.url.getParam("port");
  String property=request.url.getParam("property");
  
  NetResponse response(HttpStatus::STATUS_OK);
  
  //ensure we're using latest visus.config
  if (VisusConfig::needReload())
    VisusConfig::reload();
  
  if (format=="xml")
  {
    response.setXmlBody(scenes->getList(PublicScenes::XmlFormat,property));
  }
  else if (format=="html")
  {
    response.setHtmlBody(scenes->getList(PublicScenes::HtmlFormat,property));
  }
  else if (format=="json")
  {
    response.setJSONBody(scenes->getList(PublicScenes::JSONFormat,property));
  }
  else
  {
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND,"wrong format(" + format + ")");
  }
  
  if (!hostname.empty())
    response.setTextBody(StringUtils::replaceAll(response.getTextBody(),"$(hostname)",hostname));
  
  if (!port.empty())
    response.setTextBody(StringUtils::replaceAll(response.getTextBody(),"$(port)",port));
  
  return response;
}


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleHtmlForPlugin(const NetRequest& request)
{
  String htmlcontent=
      "<HTML>\r\n"
      "<HEAD><TITLE>Visus Plugin</TITLE><STYLE>body{margin:0;padding:0;}</STYLE></HEAD><BODY>\r\n"
      "  <center>\r\n"
      "  <script>\r\n"
      "    document.write('<embed  id=\"plugin\" type=\"application/npvisusplugin\" src=\"\" width=\"100%%\" height=\"100%%\"></embed>');\r\n"
      "    document.getElementById(\"plugin\").open(location.href);\r\n"
      "  </script>\r\n"
      "  <noscript>NPAPI not enabled</noscript>\r\n"
      "  </center>\r\n"
      "</BODY>\r\n"
      "</HTML>\r\n";

  NetResponse response(HttpStatus::STATUS_OK);
  response.setHtmlBody(htmlcontent); 
  return response;
}


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleOpenSeaDragon(const NetRequest& request)
{
  String dataset_name  = request.url.getParam("dataset");
  String compression   = request.url.getParam("compression","png");
  String debugMode     = request.url.getParam("debugMode","false");
  String showNavigator = request.url.getParam("showNavigator","true");

  SharedPtr<PublicDataset> public_dataset=datasets->findPublicDataset(dataset_name);
  if (!public_dataset)
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND,"Cannot find dataset(" + dataset_name + ")");
  
  auto dataset=public_dataset->getDataset();
  if (dataset->getPointDim()!=2)
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST,"dataset(" + dataset_name + ") has dimension !=2");

  int w=(int)dataset->getBox().p2[0];
  int h=(int)dataset->getBox().p2[1];
  int maxh=dataset->getBitmask().getMaxResolution();
  int bitsperblock=dataset->getDefaultBitsPerBlock();

  std::ostringstream out;
  out
    <<"<html>"<<std::endl
    <<"<head>"<<std::endl
    <<"<meta charset='utf-8'>"<<std::endl
    <<"<title>Visus OpenSeaDragon</title>"<<std::endl
    <<"<script src='https://openseadragon.github.io/openseadragon/openseadragon.min.js'></script>"<<std::endl
    <<"<style>.openseadragon {background-color: gray;}</style>"<<std::endl
    <<"</head>"<<std::endl

    <<"<body>"<<std::endl
    <<"<div id='osd1' class='openseadragon' style='width:100%; height:100%;'>"<<std::endl

    <<"<script type='text/javascript'>"<<std::endl
    <<"base_url = window.location.protocol + '//' + window.location.host + '/mod_visus?action=boxquery&dataset="<<dataset_name<<"&compression="<<compression<<"';"<<std::endl
    <<"w = "<<w<<";"<<std::endl
    <<"h = "<<h<<";"<<std::endl
    <<"maxh = "<<maxh<<";"<<std::endl
    <<"bitsperblock = "<<bitsperblock<<";"<<std::endl

    //try to align as much as possible to the block shape: we know that a block
    //has 2^bitsperblock samples. Assuming the bitmask is balanced at the end (i.e. V.....01010101)
    //then we simply subdivide the domain in squares with tileSize^2=2^bitsperblock -> tileSize is about 2^(bitsperblock/2)

    <<"tileSize = Math.pow(2,bitsperblock/2);"<<std::endl
    <<"minLevel=bitsperblock/2;"<<std::endl
    <<"maxLevel=maxh/2;"<<std::endl
    <<"OpenSeadragon({"<<std::endl
    <<"  id: 'osd1',"<<std::endl
    <<"  prefixUrl: 'https://raw.githubusercontent.com/openseadragon/svg-overlay/master/openseadragon/images/',"<<std::endl
    <<"  showNavigator: "<<showNavigator<<","<<std::endl
    <<"  debugMode: "<<debugMode<<","<<std::endl
    <<"  tileSources: {"<<std::endl
    <<"    height: h, width:  w, tileSize: tileSize, minLevel: minLevel, maxLevel: maxLevel,"<<std::endl
    <<"    getTileUrl: function(level,x,y) {"<<std::endl

    // trick to return an image with resolution (tileSize*tileSize) at a certain resolution
    // when level=maxLevel I just use the tileSize
    // when I go up one level I need to use 2*tileSize in order to have the same number of samples

    <<"      lvlTileSize = tileSize*Math.pow(2, maxLevel-level);"<<std::endl
    <<"    	 x1 = Math.min(lvlTileSize*x,w); x2 = Math.min(x1 + lvlTileSize, w);"<<std::endl
    <<"    	 y1 = Math.min(lvlTileSize*y,h); y2 = Math.min(y1 + lvlTileSize, h);"<<std::endl
    <<"    	 return base_url"<<std::endl
    <<"    	   + '&box='+x1+'%20'+(x2-1)+'%20'+(h -y2)+'%20'+(h-y1-1)"<<std::endl
    <<"    		 + '&toh=' + level*2"<<std::endl
    <<"    		 + '&maxh=' + maxh ;"<<std::endl
    <<"}}});"<<std::endl

    <<"</script>"<<std::endl
    <<"</div>"<<std::endl
    <<"</body>"<<std::endl
    <<"</html>;"<<std::endl;

  NetResponse response(HttpStatus::STATUS_OK);
  response.setHtmlBody(out.str()); 
  return response;
}


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleBlockQuery(const NetRequest& request)
{
  String dataset_name = request.url.getParam("dataset");

  SharedPtr<PublicDataset> public_dataset=datasets->findPublicDataset(dataset_name);
  if (!public_dataset)
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND,"Cannot find dataset(" + dataset_name + ")");

  SharedPtr<Dataset> dataset=public_dataset->getDataset();

  String compression         =         request.url.getParam("compression");
  String fieldname           =         request.url.getParam("field",dataset->getDefaultField().name);
  double time                = cdouble(request.url.getParam("time",cstring(dataset->getDefaultTime())));

  std::vector<BigInt> start_address; for (auto it : StringUtils::split(request.url.getParam("from" ,"0"))) start_address.push_back(cbigint(it));
  std::vector<BigInt> end_address  ; for (auto it : StringUtils::split(request.url.getParam("to"   ,"0"))) end_address  .push_back(cbigint(it));

  if (start_address.empty())
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND,"start_address.empty()");

  if (start_address.size()!=end_address.size())
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND,"start_address.size()!=end_address.size()");

  Field field=fieldname.empty()?dataset->getDefaultField():dataset->getFieldByName(fieldname);
  if (!field.valid()) 
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND,"Cannot find field(" + fieldname + ")");

  bool bHasFilter=!field.filter.empty();

  auto access=dataset->createAccessForBlockQuery();

  WaitAsync< Future<bool>, SharedPtr<BlockQuery> > async;
  access->beginRead();
  Aborted aborted;
  for (int I = 0; I < (int)start_address.size(); I++)
  {
    auto query = std::make_shared<BlockQuery>(field, time, start_address[I], end_address[I], aborted);
    async.pushRunning(query->future, query);
    dataset->readBlock(access,query);
  }
  access->endRead();

  std::vector<NetResponse> responses;
  for (int I=0,N=async.size();I<N;I++)
  {
    auto query=async.popReady().second;

    if (query->getStatus()!=QueryOk) 
    {
      responses.push_back(NetResponseError(HttpStatus::STATUS_NOT_FOUND,"query->executeAndWait failed"));
      continue;
    }

    //encode data
    NetResponse response(HttpStatus::STATUS_OK);
    if (!response.setArrayBody(compression,query->buffer))
    {
      if (!dataset->convertBlockQueryToRowMajor(query))
      {
        responses.push_back(NetResponseError(HttpStatus::STATUS_INTERNAL_SERVER_ERROR,"convertBlockQueryToRowMajor() failed"));
        continue;
      }

      if (!response.setArrayBody(compression,query->buffer))
      {
         responses.push_back(NetResponseError(HttpStatus::STATUS_INTERNAL_SERVER_ERROR,"Cannot encode the array"));
         continue;
      }
    }

    responses.push_back(response);
  }

  return NetResponse::compose(responses);
}

///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleQuery(const NetRequest& request)
{
  String dataset_name    =         request.url.getParam("dataset");
  int maxh               =    cint(request.url.getParam("maxh"));
  int fromh              =    cint(request.url.getParam("fromh"));
  int endh               =    cint(request.url.getParam("toh"));
  String fieldname       =         request.url.getParam("field");
  double time            = cdouble(request.url.getParam("time"));
  String compression     =         request.url.getParam("compression");
  bool   bDisableFilters = cbool  (request.url.getParam("disable_filters"));
  bool   bKdBoxQuery     =         request.url.getParam("kdquery")=="box";
  String action          =         request.url.getParam("action");

  String palette         =         request.url.getParam("palette");
  double palette_min     = cdouble(request.url.getParam("palette_min"));
  double palette_max     = cdouble(request.url.getParam("palette_max"));
  String palette_interp  =        (request.url.getParam("palette_interp"));

  SharedPtr<PublicDataset> public_dataset=datasets->findPublicDataset(dataset_name);
  if (!public_dataset)
    return NetResponseError(HttpStatus::STATUS_NOT_FOUND,"Cannot find dataset(" + dataset_name + ")");

  SharedPtr<Dataset> dataset=public_dataset->getDataset();
  int pdim = dataset->getPointDim();

  Field field = fieldname.empty()? dataset->getDefaultField() : dataset->getFieldByName(fieldname);
  if (!field.valid())
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST,"Cannot find fieldname(" + fieldname + ")");

  auto query=std::make_shared<Query>(dataset.get(),'r');
  query->time=time;
  query->field=field;
  query->start_resolution=fromh;
  query->end_resolutions={endh};
  query->max_resolution=maxh;
  query->aborted=Aborted(); //TODO: how can I get the aborted from network?

  //I apply the filter on server side only for the first coarse query (more data need to be processed on client side)
  if (fromh==0 && !bDisableFilters)
  {
    query->filter.enabled=true;
    query->filter.domain=(bKdBoxQuery? dataset->getBitmask().getPow2Box() : dataset->getBox());
  }
  else
  {
    query->filter.enabled=false;
  }

  //position
  if (action=="boxquery")
  {
    query->position=Position(NdBox::parseFromOldFormatString(pdim, request.url.getParam("box")));
  }
  else if (action=="pointquery")
  {
    Matrix  map     =  Matrix(request.url.getParam("matrix"));
    Box3d   box     =  Box3d::parseFromString(request.url.getParam("box"));

    if (request.url.hasParam("nsamples"))
    {
      NdPoint nsamples = NdPoint::parseDims(request.url.getParam("nsamples"));
      query->desired_nsamples=nsamples;
    }

    query->position=(Position(map,box));
  }
  else
  {
    //TODO
    VisusAssert(false);
  }

  //query failed
  if (!dataset->beginQuery(query))
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST,"dataset->beginQuery() failed "  + query->getLastErrorMsg());

  auto access=dataset->createAccess();
  if (!dataset->executeQuery(access,query))
    return NetResponseError(HttpStatus::STATUS_BAD_REQUEST,"dataset->executeQuery() failed " + query->getLastErrorMsg());

  auto buffer=query->buffer;

  //useful for kdquery=box (for example with discrete wavelets, don't want the extra channel)
  if (bKdBoxQuery)
  {
    if (auto filter=query->filter.value)
      buffer=filter->dropExtraComponentIfExists(buffer);
  }

  if (!palette.empty() && buffer.dtype.ncomponents()==1)
  {
    TransferFunction tf;
    if (!tf.setDefault(palette))
    {
      VisusAssert(false);
      VisusInfo()<<"invalid palette specified: "<<palette;
      VisusInfo()<<"use one of:";
      std::vector<String> tf_defaults=TransferFunction::getDefaults();
      for (int i=0;i<tf_defaults.size();i++)
        VisusInfo()<<"\t"<<tf_defaults[i];
    }
    else
    {
      if (palette_min != palette_max)
        tf.setInputNormalization(TransferFunction::InputNormalization(ComputeRange::UseCustom,Range(palette_min,palette_max,0)));

      if (!palette_interp.empty())
        tf.setInterpolation(palette_interp);

      buffer=tf.applyToArray(buffer);
      if (!buffer)
        return NetResponseError(HttpStatus::STATUS_INTERNAL_SERVER_ERROR,"palette failed");
    }
  }

  NetResponse response(HttpStatus::STATUS_OK);
  if (!response.setArrayBody(compression,buffer))
    return NetResponseError(HttpStatus::STATUS_INTERNAL_SERVER_ERROR,"NetResponse encodeBuffer failed");

  return response;
}


///////////////////////////////////////////////////////////////////////////
NetResponse ModVisus::handleRequest(NetRequest request)
{
  Time t1=Time::now();

  //default action
  if (request.url.getParam("action").empty())
  {
    String user_agent=StringUtils::toLower(request.getHeader("User-Agent"));

    bool bSpecifyDataset=request.url.hasParam("dataset");
    bool bSpecifyScene=request.url.hasParam("scene");
    bool bCommercialBrower=!user_agent.empty() && !StringUtils::contains(user_agent,"visus");

    if (bCommercialBrower)
    {
      if (bSpecifyDataset)
      {
        request.url.setParam("action","plugin");
      }
      else if(bSpecifyScene)
      {
        request.url.setParam("action","readscene");
      }
      else
      {
        request.url.setParam("action","list");
        request.url.setParam("format","html");
      }
    }
    else
    {
      if (bSpecifyDataset)
      {
        request.url.setParam("action","readdataset");
      }
      else if(bSpecifyScene)
      {
        request.url.setParam("action","readscene");
      }
      else
      {
        request.url.setParam("action","list");
        request.url.setParam("format","xml");
      }
    }
  }
  
  VisusInfo() << "request is " << request.getTextBody();

  String action=request.url.getParam("action");

  NetResponse response;

  if (action=="rangequery" || action=="blockquery") 
    response=handleBlockQuery(request);

  else if (action=="query" || action=="boxquery" || action=="pointquery")
    response=handleQuery(request);

  else if (action=="readdataset" || action=="read_dataset")
    response=handleReadDataset(request);
  
  else if (action=="readscene" || action=="read_scene")
    response=handleReadScene(request);

  else if (action=="plugin") 
    response=handleHtmlForPlugin(request);

  else if (action=="list")
    response=handleGetListOfDatasets(request);
  
  else if (action=="listscenes" || action=="list_scenes")
    response=handleGetListOfScenes(request);

  else if (action=="configure_datasets")
    response=handleConfigureDatasets(request);

  else if (action=="AddDataset" || action=="add_dataset")
    response=handleAddDataset(request);

  else if (action=="openseadragon")
    response=handleOpenSeaDragon(request);

  else if (action=="ping")
    response=NetResponse(HttpStatus::STATUS_OK);

  else
    response=NetResponseError(HttpStatus::STATUS_NOT_FOUND,"unknown action(" + action + ")");

  VisusInfo()
    <<" request("<<request.url.toString()<<") "
    <<" status("<<response.getStatusDescription()<<") body("<<StringUtils::getStringFromByteSize(response.body? response.body->c_size():0)<<") msec("<<t1.elapsedMsec()<<")";

  //add some standard header
  response.setHeader("git_revision",ApplicationInfo::git_revision);
  response.setHeader("version",cstring(ApplicationInfo::version));

  //expose visus headers (for javascript access)
  //see https://stackoverflow.com/questions/35240520/fetch-answer-empty-due-to-the-preflight
  {
    std::vector<String> exposed_headers;
    exposed_headers.reserve(response.headers.size());
    for (auto header : response.headers) {
      if (StringUtils::startsWith(header.first, "visus"))
        exposed_headers.push_back(header.first);
    }
    response.setHeader("Access-Control-Expose-Headers", StringUtils::join(exposed_headers, ","));
  }
 
  return response;
}

} //namespace Visus
