---
layout: default
title: ViSUS Configuration File
nav_order: 4
has_children: false
---

# ViSUS Configuration File
{: .no_toc }
A ViSUS configuration file contains a set of application configuration parameters and a list of datasets that also represent your bookmarks in [ViSUS Viewer]({{ site.baseurl }}{% link docs/viewer-features.md %}).

# Table of contents
{: .no_toc .text-delta }

1. TOC
{:toc}

## Configuration File Location

Every ViSUS application, on load, uses the following strategy to find the configuration file:

1.  location specified by --visus-config command line parameter
2.  current working directory
3.  visus user directory (note: itunes-accessible documents directory on ios)
4.  resources directory (e.g. osx/ios bundle, same as cwd on other platforms)
5.  hardcoded path provided at compile time (e.g. for mod\_visus default)
6.  executable directory (very unusual deployment, but it doesn't hurt anything)

## Bookmarks, datasets and scenes

The bookmarks can refer to datasets on disk, remote or also to custom scenes that you saved from [ViSUS Viewer]({{ site.baseurl }}{% link docs/viewer-features.md %}).

Here is an example of a visus.config file of these three kind of datasets.

```xml
<visus>
    <!-- Local dataset --> 
    <dataset name="my\_dataset" url="file:///Users/username/data/my\_dataset.idx" permissions="public"/>

    <!-- Remote dataset -->
    <dataset name="2kbit1" url="http://atlantis.sci.utah.edu/mod\_visus?dataset=2kbit1" permissions="public">

    <!-- Saved scene -->
    <scene name="my\_scene" url="file:///Users/username/data/my\_scene.xml" permissions="public"/>
</visus>
```

Note: the visus.config file for a server should have always the attribute `permissions="public"` in the declaration of all the datasets that you want to expose.

## Network Access and Caching

ViSUS can access the same data from both the network or a local cache on your disk. The cache allows faster (and offline) access to the data that already navigated using the viewer. Please notice that ViSUS only saves the timesteps and fields that you explored, not the entire dataset.

An important feature of the ViSUS framework is the _caching_. Data streamed from the server can be cached on disk for later faster access. To enable this feature for a specific dataset we can define a multiple layer access as following:

```xml
  <dataset name="2kbit1" url="http://atlantis.sci.utah.edu/mod\_visus?dataset=2kbit1" permissions="public">
     <access name="Multiplex" type="multiplex">
       <access name="cache"  type="disk" chmod="rw" url="$(VisusCacheDirectory)/2kbit1/visus.idx" />
       <access name="source" type="network" chmod="r" compression="zip" />
     </access> 
  </dataset> 
```

With these lines of code we created two layers of data access the first from the disk (e.g. the cache) and the other from the network. This will allow the application to search first on the disk for the requested data and in case of _miss_ the request will be forwarded to the network. At the same time any data requested to the network layer will be cached on disk (at the specified URL).

**Note**: when frequent and fast access is strictly required, enabling the _caching_ is highly recommended.

## Faster network access

In order to speed up your network performance it is possible to use some special options in your access configurations:

*   _nconnections_: enable parallel requests
*   _num\_queries\_per\_request_: gather multiple block requests in a single query
*   _compression_: compress the data for transfer

Here is an example:

```xml
<access name="source" type="network" chmod="r" compression="zip" nconnections="4" num\_queries\_per\_request="32"/> 
```

## Memory usage

In the configuration we can set limits to the memory allocated by the viewer, defining the size of the memory we want to allocate, and the maximum usage.

```xml
<Configuration>
      <GLMemory total="2gb" maximum\_memory\_allocation\_factor="0.8"/>
</Configuration> 
```

## Use a default scene for a dataset

The scene that you save from the [ViSUS Viewer]({{ site.baseurl }}{% link docs/viewer-features.md %}) can be used as default scene of a certain dataset. This way every time you open it all the saved selection and palette settings will be applied.

This can be done both locally and remotely setting the field _(scene)_ in the .idx file of your dataset, as following:

```
(scene)
file:///path/to/my/local/scene.scn
```

Or using a remote scene

```
(scene)
[http://yourserver.org/mod\_visus?scene=dataset\_scene\_name](http://yourserver.org/mod_visus?scene=dataset_scene_name)
```


## More Settings

You can configure more settings in the \_Configuration\_ section of your visus.config file, for example the background color, logo or the network requests configuration.

```xml
<Configuration\>
  
  <VisusViewer background\_color\="255 255 255 255"\>
   <Logo\>
      <BottomLeft alpha\='0.8' filename\='resources/cat\_rgb.tga' />
   </Logo\> 
  </VisusViewer\>
     
  <ModVisusAccess nconnections\="8" num\_queries\_per\_request\="4"/>

</Configuration\>
```

Retrieved from "[http://wiki.visus.org/index.php?title=ViSUS\_configuration\_file](http://wiki.visus.org/index.php?title=ViSUS_configuration_file)"