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

#include <Visus/Visus.h>
#include <Visus/ModVisus.h>
#include <Visus/Path.h>
#include <Visus/Idx.h>
#include <Visus/ApplicationInfo.h>
#include <Visus/VisusConfig.h>

#if WIN32

#pragma warning(disable:4091)
#include <winsock2.h>
#include <httpserv.h> 

using namespace Visus;

std::vector<String> request_headers = std::vector<String>({
  "CacheControl",
  "Connection",
  "Date",
  "KeepAlive",
  "Pragma",
  "Trailer",
  "TransferEncoding",
  "Upgrade",
  "Via",
  "Warning",
  "Allow",
  "ContentLength",
  "ContentType",
  "ContentEncoding",
  "ContentLanguage",
  "ContentLocation",
  "ContentMd5",
  "ContentRange",
  "Expires",
  "LastModified",
  "Accept",
  "AcceptCharset",
  "AcceptEncoding",
  "AcceptLanguage",
  "Authorization",
  "Cookie",
  "Expect",
  "From",
  "Host",
  "IfMatch",
  "IfModifiedSince",
  "IfNoneMatch",
  "IfRange",
  "IfUnmodifiedSince",
  "MaxForwards",
  "ProxyAuthorization",
  "Referer",
  "Range",
  "Te",
  "Translate",
  "UserAgent"
  });

std::map<String, _HTTP_HEADER_ID> response_headers = std::map<String, _HTTP_HEADER_ID>({
  { "CacheControl",HttpHeaderCacheControl },
  { "Connection",HttpHeaderConnection },
  { "Date",HttpHeaderDate },
  { "KeepAlive",HttpHeaderKeepAlive },
  { "Pragma",HttpHeaderPragma },
  { "Trailer",HttpHeaderTrailer },
  { "TransferEncoding",HttpHeaderTransferEncoding },
  { "Upgrade",HttpHeaderUpgrade },
  { "Via",HttpHeaderVia },
  { "Warning",HttpHeaderWarning },
  { "Allow",HttpHeaderAllow },
  { "ContentLength",HttpHeaderContentLength },
  { "ContentType",HttpHeaderContentType },
  { "ContentEncoding",HttpHeaderContentEncoding },
  { "ContentLanguage",HttpHeaderContentLanguage },
  { "ContentLocation",HttpHeaderContentLocation },
  { "ContentMd5",HttpHeaderContentMd5 },
  { "ContentRange",HttpHeaderContentRange },
  { "Expires",HttpHeaderExpires },
  { "LastModified",HttpHeaderLastModified },
  { "AcceptRanges",HttpHeaderAcceptRanges },
  { "Age",HttpHeaderAge },
  { "Etag",HttpHeaderEtag },
  { "Location",HttpHeaderLocation },
  { "ProxyAuthenticate",HttpHeaderProxyAuthenticate },
  { "RetryAfter",HttpHeaderRetryAfter },
  { "Server",HttpHeaderServer },
  { "SetCookie",HttpHeaderSetCookie },
  { "Vary",HttpHeaderVary },
  { "WwwAuthenticate",HttpHeaderWwwAuthenticate }
});


// Create a global handle for the Event Viewer.
HANDLE g_hEventLog;
ModVisus* mod_visus=nullptr;

/////////////////////////////////////////////////////////////////////
BOOL WriteEventViewerLog(LPCSTR szBuffer[], WORD wNumStrings)
{
  // Test whether the handle for the Event Viewer is open.
  if (NULL != g_hEventLog)
  {
    // Write any strings to the Event Viewer and return.
    return ReportEvent(g_hEventLog, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, wNumStrings, 0, szBuffer, NULL);
  }

  return FALSE;
}

////////////////////////////////////////////////////
class MyGlobalModule : public CGlobalModule
{
public:

  //constructor
  MyGlobalModule()
  {
    // Open a handle to the Event Viewer.
    g_hEventLog = RegisterEventSource(NULL, "IISADMIN");

    VisusConfig::getSingleton()->filename = "/inetpub/wwwroot/visus/visus.config";

    static int argn = 1;
    static const char* argv[] = { "mod_visus.dll" };
    SetCommandLine(argn, argv);
    IdxModule::attach();

    RedirectLog = [](const String& msg)
    {
      LPCSTR szBuffer[1] = { msg.c_str() };
      WriteEventViewerLog(szBuffer, 1);
    };

    mod_visus = new ModVisus();
    mod_visus->configureDatasets();
  }

  //destructor
  ~MyGlobalModule()
  {
    delete mod_visus;

    RedirectLog = nullptr;

    // Test whether the handle for the Event Viewer is open.
    if (NULL != g_hEventLog)
    {
      DeregisterEventSource(g_hEventLog);
      g_hEventLog = NULL;
    }
  }

  //OnGlobalPreBeginRequest
  virtual GLOBAL_NOTIFICATION_STATUS OnGlobalPreBeginRequest(IN IPreBeginRequestProvider * pProvider) override
  {
    UNREFERENCED_PARAMETER(pProvider);

    // Create an array of strings.
    LPCSTR szBuffer[2] = { "MyGlobalModule","OnGlobalPreBeginRequest" };

    // Write the strings to the Event Viewer.
    WriteEventViewerLog(szBuffer, 2);

    // Return processing to the pipeline.
    return GL_NOTIFICATION_CONTINUE;
  }

  //Terminate
  virtual VOID Terminate() override
  {
    // Remove the class from memory.
    delete this;
  }

};

///////////////////////////////////////////////////////////////////////////////
class MyHttpModule  : public CHttpModule
{
public:

  //OnAcquireRequestState
  virtual REQUEST_NOTIFICATION_STATUS OnBeginRequest(IN IHttpContext * pHttpContext,IN IHttpEventProvider * pProvider) override
  {
    //if you want to debug
    #if 1
    VisusAssert(false);
    #endif

    // Retrieve a pointer to the request.
    auto iis_request_ = pHttpContext->GetRequest();
    if (iis_request_ == nullptr)
      return RQ_NOTIFICATION_CONTINUE;

    auto iis_request = iis_request_->GetRawHttpRequest();

    String iis_raw_url(iis_request->pRawUrl);

    //filter addresses
    if (!StringUtils::startsWith(iis_raw_url, "/mod_visus"))
      return RQ_NOTIFICATION_CONTINUE;

    //convert url
    NetRequest visus_request("http://localhost" + iis_raw_url);

    //convert known headers
    for (int i = 0; i < HttpHeaderRequestMaximum; i++)
    {
      auto hdr = &iis_request->Headers.KnownHeaders[i];
      if (hdr->RawValueLength != 0)
      {
        String key = request_headers[i];
        String value = String(hdr->pRawValue, hdr->RawValueLength);
        visus_request.setHeader(key, value);
      }
    }

    //convert unknown headers
    for (int I = 0; I < iis_request->Headers.UnknownHeaderCount; I++)
    {
      String key(iis_request->Headers.pUnknownHeaders[I].pName, iis_request->Headers.pUnknownHeaders[I].NameLength);
      String value(iis_request->Headers.pUnknownHeaders[I].pRawValue, iis_request->Headers.pUnknownHeaders[I].RawValueLength);
      visus_request.setHeader(key, value);
    }

    NetResponse response = mod_visus->handleRequest(visus_request);

    IHttpResponse * iis_response = pHttpContext->GetResponse();
    iis_response->Clear();
    iis_response->SetStatus(response.status, response.getErrorMessage().c_str());

    for (const auto& header : response.headers)
    {
      String key = header.first;
      String val = header.second;

      auto it = response_headers.find(key);
      if (it != response_headers.end())
        iis_response->SetHeader(it->second, val.c_str(), (USHORT)val.size(), TRUE);
      else
        iis_response->SetHeader(key.c_str(), val.c_str(), (USHORT)val.size(), TRUE);
    }

    //write response body
    if (response.body && response.body->c_size())
    {
      HTTP_DATA_CHUNK dataChunk;
      dataChunk.DataChunkType = HttpDataChunkFromMemory;
      dataChunk.FromMemory.pBuffer = response.body->c_ptr();
      dataChunk.FromMemory.BufferLength = (ULONG)response.body->c_size();
      DWORD nbytes_sent;
      iis_response->WriteEntityChunks(&dataChunk,/*nchuncks*/1,/*fAsync*/FALSE,/*fMoreData*/FALSE, &nbytes_sent);
    }

    return RQ_NOTIFICATION_FINISH_REQUEST;
  }
};

/////////////////////////////////////////////////////////////////////////
class MyHttpModuleFactory : public IHttpModuleFactory
{
public:

  //GetHttpModule
  virtual HRESULT GetHttpModule(OUT CHttpModule ** ppModule,IN IModuleAllocator * ) override
  {
    VisusInfo() << "GetHttpModule";

    // Create a new instance.
    MyHttpModule * pModule = new MyHttpModule;

    // Test for an error.
    if (!pModule)
    {
      // Return an error if the factory cannot create the instance.
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }
    else
    {
      // Return a pointer to the module.
      *ppModule = pModule;
      pModule = NULL;
      // Return a success status.
      return S_OK;
    }
  }

  //Terminate
  virtual void Terminate() override
  {
    VisusInfo() << "Terminate";
    delete this;
  }
};



/////////////////////////////////////////////////////////////////////////////////////
extern "C" __declspec(dllexport)  HRESULT __stdcall RegisterModule(DWORD ,IHttpModuleRegistrationInfo * pModuleInfo,IHttpServer * )
{
  // Set the request notifications.
  HRESULT hr = pModuleInfo->SetRequestNotifications(new MyHttpModuleFactory,RQ_BEGIN_REQUEST, 0);

  // Test for an error and exit if necessary.
  if (FAILED(hr))
    return hr;

  // Set the request priority.
  hr = pModuleInfo->SetPriorityForRequestNotification(RQ_BEGIN_REQUEST, PRIORITY_ALIAS_MEDIUM);

  // Test for an error and exit if necessary.
  if (FAILED(hr))
    return hr;

  // Create an instance of the global module class.
  MyGlobalModule * pGlobalModule = new MyGlobalModule;

  // Test for an error.
  if (NULL == pGlobalModule)
    return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

  // Set the global notifications.
  hr = pModuleInfo->SetGlobalNotifications(pGlobalModule, GL_PRE_BEGIN_REQUEST);

  // Test for an error and exit if necessary.
  if (FAILED(hr))
    return hr;

  // Set the global priority.
  hr = pModuleInfo->SetPriorityForGlobalNotification(GL_PRE_BEGIN_REQUEST, PRIORITY_ALIAS_LOW);

  // Test for an error and exit if necessary.
  if (FAILED(hr))
    return hr;

  // Return a success status;`
  return S_OK;
}


#endif

#if !WIN32

#include <sys/types.h>
#include <unistd.h>


#include <httpd.h>
#include <http_config.h>

extern "C" module AP_MODULE_DECLARE_DATA visus_module; 
#ifdef APLOG_USE_MODULE
  APLOG_USE_MODULE(visus);
#endif

#include <http_log.h>
#include <http_protocol.h>
#include <apr_strings.h>



using namespace Visus;

////////////////////////////////////////////////////////////////////////////////
// for a very strange problem of APR library I need to have the mime type in memory, if I use an STL
// string, the buffer gets released before sending the answer
class MimeTypes : public StringMap
{
public:

  //constructor
  MimeTypes()
  {
    setValue("application/postscript","ai");
    setValue("audio/x-aiff","aif");
    setValue("audio/x-aiff","aifc");
    setValue("audio/x-aiff","aiff");
    setValue("text/plain","asc");
    setValue("application/atom+xml","atom");
    setValue("audio/basic","au");
    setValue("video/x-msvideo","avi");
    setValue("application/x-bcpio","bcpio");
    setValue("application/octet-stream","bin");
    setValue("image/bmp","bmp");
    setValue("application/x-netcdf","cdf");
    setValue("image/cgm","cgm");
    setValue("application/octet-stream","class");
    setValue("application/x-cpio","cpio");
    setValue("application/mac-compactpro","cpt");
    setValue("application/x-csh","csh");
    setValue("text/css","css");
    setValue("application/x-director","dcr");
    setValue("video/x-dv","dif");
    setValue("application/x-director","dir");
    setValue("image/vnd.djvu","djv");
    setValue("image/vnd.djvu","djvu");
    setValue("application/octet-stream","dll");
    setValue("application/octet-stream","dmg");
    setValue("application/octet-stream","dms");
    setValue("application/msword","doc");
    setValue("application/xml-dtd","dtd");
    setValue("video/x-dv","dv");
    setValue("application/x-dvi","dvi");
    setValue("application/x-director","dxr");
    setValue("application/postscript","eps");
    setValue("text/x-setext","etx");
    setValue("application/octet-stream","exe");
    setValue("application/andrew-inset","ez");
    setValue("image/gif","gif");
    setValue("application/srgs","gram");
    setValue("application/srgs+xml","grxml");
    setValue("application/x-gtar","gtar");
    setValue("application/x-hdf","hdf");
    setValue("application/mac-binhex40","hqx");
    setValue("text/html","htm");
    setValue("text/html","html");
    setValue("x-conference/x-cooltalk","ice");
    setValue("image/x-icon","ico");
    setValue("text/calendar","ics");
    setValue("image/ief","ief");
    setValue("text/calendar","ifb");
    setValue("model/iges","iges");
    setValue("model/iges","igs");
    setValue("application/x-java-jnlp-file","jnlp");
    setValue("image/jp2","jp2");
    setValue("image/jpeg","jpe");
    setValue("image/jpeg","jpeg");
    setValue("image/jpeg","jpg");
    setValue("application/x-javascript","js");
    setValue("audio/midi","kar");
    setValue("application/x-latex","latex");
    setValue("application/octet-stream","lha");
    setValue("application/octet-stream","lzh");
    setValue("audio/x-mpegurl","m3u");
    setValue("audio/mp4a-latm","m4a");
    setValue("audio/mp4a-latm","m4b");
    setValue("audio/mp4a-latm","m4p");
    setValue("video/vnd.mpegurl","m4u");
    setValue("video/x-m4v","m4v");
    setValue("image/x-macpaint","mac");
    setValue("application/x-troff-man","man");
    setValue("application/mathml+xml","mathml");
    setValue("application/x-troff-me","me");
    setValue("model/mesh","mesh");
    setValue("audio/midi","mid");
    setValue("audio/midi","midi");
    setValue("application/vnd.mif","mif");
    setValue("video/quicktime","mov");
    setValue("video/x-sgi-movie","movie");
    setValue("audio/mpeg","mp2");
    setValue("audio/mpeg","mp3");
    setValue("video/mp4","mp4");
    setValue("video/mpeg","mpe");
    setValue("video/mpeg","mpeg");
    setValue("video/mpeg","mpg");
    setValue("audio/mpeg","mpga");
    setValue("application/x-troff-ms","ms");
    setValue("model/mesh","msh");
    setValue("video/vnd.mpegurl","mxu");
    setValue("application/x-netcdf","nc");
    setValue("application/oda","oda");
    setValue("application/ogg","ogg");
    setValue("image/x-portable-bitmap","pbm");
    setValue("image/pict","pct");
    setValue("chemical/x-pdb","pdb");
    setValue("application/pdf","pdf");
    setValue("image/x-portable-graymap","pgm");
    setValue("application/x-chess-pgn","pgn");
    setValue("image/pict","pic");
    setValue("image/pict","pict");
    setValue("image/png","png");
    setValue("image/x-portable-anymap","pnm");
    setValue("image/x-macpaint","pnt");
    setValue("image/x-macpaint","pntg");
    setValue("image/x-portable-pixmap","ppm");
    setValue("application/vnd.ms-powerpoint","ppt");
    setValue("application/postscript","ps");
    setValue("video/quicktime","qt");
    setValue("image/x-quicktime","qti");
    setValue("image/x-quicktime","qtif");
    setValue("audio/x-pn-realaudio","ra");
    setValue("audio/x-pn-realaudio","ram");
    setValue("image/x-cmu-raster","ras");
    setValue("application/rdf+xml","rdf");
    setValue("image/x-rgb","rgb");
    setValue("application/vnd.rn-realmedia","rm");
    setValue("application/x-troff","roff");
    setValue("text/rtf","rtf");
    setValue("text/richtext","rtx");
    setValue("text/sgml","sgm");
    setValue("text/sgml","sgml");
    setValue("application/x-sh","sh");
    setValue("application/x-shar","shar");
    setValue("model/mesh","silo");
    setValue("application/x-stuffit","sit");
    setValue("application/x-koan","skd");
    setValue("application/x-koan","skm");
    setValue("application/x-koan","skp");
    setValue("application/x-koan","skt");
    setValue("application/smil","smi");
    setValue("application/smil","smil");
    setValue("audio/basic","snd");
    setValue("application/octet-stream","so");
    setValue("application/x-futuresplash","spl");
    setValue("application/x-wais-source","src");
    setValue("application/x-sv4cpio","sv4cpio");
    setValue("application/x-sv4crc","sv4crc");
    setValue("image/svg+xml","svg");
    setValue("application/x-shockwave-flash","swf");
    setValue("application/x-troff","t");
    setValue("application/x-tar","tar");
    setValue("application/x-tcl","tcl");
    setValue("application/x-tex","tex");
    setValue("application/x-texinfo","texi");
    setValue("application/x-texinfo","texinfo");
    setValue("image/tiff","tif");
    setValue("image/tiff","tiff");
    setValue("application/x-troff","tr");
    setValue("text/tab-separated-values","tsv");
    setValue("text/plain","txt");
    setValue("application/x-ustar","ustar");
    setValue("application/x-cdlink","vcd");
    setValue("model/vrml","vrml");
    setValue("application/voicexml+xml","vxml");
    setValue("audio/x-wav","wav");
    setValue("image/vnd.wap.wbmp","wbmp");
    setValue("application/vnd.wap.wbxml","wbmxl");
    setValue("text/vnd.wap.wml","wml");
    setValue("application/vnd.wap.wmlc","wmlc");
    setValue("text/vnd.wap.wmlscript","wmls");
    setValue("application/vnd.wap.wmlscriptc","wmlsc");
    setValue("model/vrml","wrl");
    setValue("image/x-xbitmap","xbm");
    setValue("application/xhtml+xml","xht");
    setValue("application/xhtml+xml","xhtml");
    setValue("application/vnd.ms-excel","xls");
    setValue("application/xml","xml");
    setValue("image/x-xpixmap","xpm");
    setValue("application/xml","xsl");
    setValue("application/xslt+xml","xslt");
    setValue("application/vnd.mozilla.xul+xml","xul");
    setValue("image/x-xwindowdump","xwd");
    setValue("chemical/x-xyz","xyz");
    setValue("application/zip","zip");
    setValue("application/json","json");
    setValue("text/xml","xml");
  }
};

static MimeTypes mime_types;

/////////////////////////////////////////////////////////////////////////////
class ApacheModVisus : public ModVisus
{
public:
  
  //constructor
  ApacheModVisus() {
  }
  
  //destructor
  ~ApacheModVisus() {
  }

  //initialiseInCurrentProcess (to call only after the process has been forked)
  void initialiseInCurrentProcess()
  {
    VisusInfo()<<"initialiseInCurrentProcess";

    RedirectLog=[](const String& msg) {
      ap_log_perror(APLOG_MARK, APLOG_NOTICE, 0, NULL, "%s", msg.c_str());
    };

    static int narg=1;
    static const char *argv[]={"mod_visus"};
    SetCommandLine(narg,argv);
    IdxModule::attach();
    this->configureDatasets();    
  }
  
  //shutdownInCurrentProcess
  void shutdownInCurrentProcess()
  {
    VisusInfo() << "shutdownInCurrentProcess";
    IdxModule::detach();
    RedirectLog=nullptr;
  }


};

/////////////////////////////////////////////////////////////////////////////
static void          MyRegisterHook(apr_pool_t *p);
static void*         MyCreateModule(apr_pool_t *pool, server_rec *s);

extern "C" { 
  module AP_MODULE_DECLARE_DATA visus_module =
  {
      STANDARD20_MODULE_STUFF,
      NULL,                    /* per-directory config creator */
      NULL,                    /* dir config merger */
      MyCreateModule,          /* server config creator */
      NULL,                    /* server config merger */
      NULL,                    /* command table */
      MyRegisterHook,          /* set up other request processing hooks */
  };
} //extern "C"

#define GET_APACHE_MOD_VISUS(s) \
  (ApacheModVisus**)ap_get_module_config(s->module_config,&visus_module); 


/////////////////////////////////////////////////////////////////////////////
static void *MyCreateModule(apr_pool_t *pool, server_rec *s) 
{
  ApacheModVisus** module=(ApacheModVisus**)apr_palloc(pool,sizeof(ApacheModVisus*));
  (*module)=nullptr;
  ap_set_module_config(s->module_config,&visus_module,module);
  return module;
}

/////////////////////////////////////////////////////////////////////////////
static apr_status_t MyDestroyModule(void *s_) 
{
  server_rec *s=(server_rec *) s_;
  ApacheModVisus** module=GET_APACHE_MOD_VISUS(s); 
  if (*module) delete (*module);
  return APR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////
static int MyHookPostConfig(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s) 
{
  void *sentinelp; 
  apr_pool_userdata_get(&sentinelp,"modvisus_userdata", s->process->pool);
  
  //first time is called by using "root"
  if(!sentinelp) 
  {
    apr_pool_userdata_set((const void *)1, "modvisus_userdata",apr_pool_cleanup_null, s->process->pool);
    return OK;
  } 
  
  ApacheModVisus** module=GET_APACHE_MOD_VISUS(s);
  if (*module) return OK;

  //NOTE: I'm not calling initialiseApplication here because I don't know the final PID
  (*module)=new ApacheModVisus();
   
  apr_pool_cleanup_register(pconf,s,MyDestroyModule,apr_pool_cleanup_null);
  return OK;
}


/////////////////////////////////////////////////////////////////////////////
static apr_status_t MyDestroyChild(void *s_)
{
  server_rec *s=(server_rec *) s_;
  ApacheModVisus** module=GET_APACHE_MOD_VISUS(s); 
  if (*module) (*module)->shutdownInCurrentProcess();
  return APR_SUCCESS;  
}

/////////////////////////////////////////////////////////////////////////////
static void MyHookChildInit(apr_pool_t *pconf,server_rec *s)
{
  ApacheModVisus** module=GET_APACHE_MOD_VISUS(s); 
  VisusAssert(*module);
  (*module)->initialiseInCurrentProcess();
  apr_pool_cleanup_register(pconf,s,MyDestroyChild,apr_pool_cleanup_null);
}

/////////////////////////////////////////////////////////////////////////////
static int MyFillRequestHeader(void *arg, const char *key, const char *value) 
{
  if (key == NULL || value == NULL || value[0] == '\0') return 1;
  StringMap& map=*((StringMap*)arg);
  map.setValue(key,String(value));
  return 1;
}

///////////////////////////////////////////////////////////////////////////////////////
//need this string to stick in memory up to the end ... forced to use static const char!
//see http://www.gossamer-threads.com/lists/apache/users/375605?do=post_view_threaded       
static const char* APPLICATION_OCTET_STREAM="application/octet-stream";

static int MyHookRequest(request_rec *apache_request) 
{
  if (!apache_request->handler || strcmp(apache_request->handler, "visus")) 
    return DECLINED;

  ApacheModVisus** module=GET_APACHE_MOD_VISUS(apache_request->server);
  
  if (!(*module) || !apache_request->parsed_uri.query) 
    return DECLINED;

  #if (AP_SERVER_MAJORVERSION_NUMBER>=2) && (AP_SERVER_MINORVERSION_NUMBER<4)
  String client_ip = apache_request->connection->remote_ip;
  #else
  String client_ip = apache_request->useragent_ip;
  #endif

  //convert apache_request to visus_request
  NetRequest visus_request("http://localhost/mod_visus?" + String(apache_request->parsed_uri.query));

  apr_table_do(MyFillRequestHeader, &(visus_request.headers), apache_request->headers_in, NULL);    
  NetResponse visus_response=(*module)->handleRequest(visus_request);  
  
  const char* content_type=APPLICATION_OCTET_STREAM;
  
  bool bOk=visus_response.status==HttpStatus::STATUS_OK;

  //convert visus_response to apache_response
  for (StringMap::iterator it=visus_response.headers.begin();it!=visus_response.headers.end();it++)
  {
    String key = it->first;
    String val = it->second;

    if (key=="Content-Type")
    {
      String temp=StringUtils::trim(StringUtils::toLower(val));
      StringMap::iterator it=mime_types.find(temp);
      if (it!=mime_types.end()) 
        content_type=it->first.c_str();
      else
        VisusInfo()<<"MIME TYPE NOT FOUND "<<temp << "! please add it as soon as possible";
    }
    else if (key=="Content-Length")
      ap_set_content_length(apache_request,cint(val)); 
    else
      apr_table_set(bOk? apache_request->headers_out : apache_request->err_headers_out, key.c_str(), val.c_str());
  }
  
  ap_set_content_type(apache_request,content_type);
  
  if (visus_response.body && visus_response.body->c_size())
    ap_rwrite(visus_response.body->c_ptr(),visus_response.body->c_size(),apache_request);

  return bOk? OK : visus_response.status;    
}

/////////////////////////////////////////////////////////////////////////////
static void MyRegisterHook(apr_pool_t *p) 
{
  ap_hook_post_config(MyHookPostConfig , NULL, NULL, APR_HOOK_MIDDLE);
  ap_hook_handler    (MyHookRequest    , NULL, NULL, APR_HOOK_MIDDLE);
  ap_hook_child_init (MyHookChildInit  , NULL, NULL, APR_HOOK_MIDDLE);
}

#endif //!WIN32





