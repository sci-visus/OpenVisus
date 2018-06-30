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

#include <Visus/StringUtils.h>
#include <Visus/Array.h>
#include <Visus/Time.h>

#include <iostream>
#include <iomanip>
#include <cctype>

//for sha256

  #undef  override
  #undef  final

#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/engine.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/md5.h>

#include <openssl/opensslv.h>

// OPENSSL_VERSION_NUMBER>= 1.1
#if OPENSSL_VERSION_NUMBER>=0x10100000L
	#define Create_HMAC_Context(_name_)   HMAC_CTX* _name_=HMAC_CTX_new()
	#define Destroy_HMAC_Context(_name_)  HMAC_CTX_free(_name_);
#else
	#define Create_HMAC_Context(_name_)   HMAC_CTX* _name_=new HMAC_CTX();HMAC_CTX_init(_name_);
	#define Destroy_HMAC_Context(_name_)  HMAC_CTX_cleanup(_name_); delete _name_;
#endif


namespace Visus {


///////////////////////////////////////////////////////////////////////////
bool cbool(const String& s) 
{
  if (s.empty())
    return false;

  auto N=s.length();
  
  if (N==4 && std::tolower(s[0])=='t' && std::tolower(s[1])=='r' && std::tolower(s[2])=='u' && std::tolower(s[3])=='e')
    return true;

  if (N==5 && std::tolower(s[0])=='f' && std::tolower(s[1])=='a' && std::tolower(s[2])=='l' && std::tolower(s[3])=='s' && std::tolower(s[4])=='e')
    return false;

  return std::stoi(s)?true:false;
}


///////////////////////////////////////////////////////////////////////////
static char CharToUpper(char c) 
{return std::toupper(c);}

static char CharToLower(char c) 
{return std::tolower(c);}


String StringUtils::toLower(String ret) 
{std::transform(ret.begin(),ret.end(),ret.begin(),CharToLower);return ret;}

String StringUtils::toUpper(String ret) 
{std::transform(ret.begin(),ret.end(),ret.begin(),CharToUpper);return ret;}

///////////////////////////////////////////////////////////////////////////
std::vector<String> StringUtils::split(String source,String separator,bool bPurgeEmptyItems) 
{
  std::vector<String> ret;
  int m=(int)separator.size();
  for (int j;(j=(int)source.find(separator))>=0;source=source.substr(j+m))
  {
    String item=source.substr(0,j);
    if (!bPurgeEmptyItems || !item.empty()) ret.push_back(item);
  }
  if (!bPurgeEmptyItems || !source.empty()) ret.push_back(source);
  return ret;
}

///////////////////////////////////////////////////////////////////////////
String StringUtils::join(std::vector<String> v,String separator,String prefix,String suffix)
{
  int N=(int)v.size();
  std::ostringstream out;
  out<<prefix;
  for (int I=0;I<N;I++)
  {
    if (I) out<<separator;
    out<<v[I];
  }
  out<<suffix;
  return out.str();
}


///////////////////////////////////////////////////////////////////////////
std::vector<String> StringUtils::getLines(const String& s)
{
  std::vector<String> lines;
  String line;

  int N=(int)s.size();
  for (int I=0;I<N;I++)
  {
    char ch=s[I];
    if (ch=='\r')
    {
      lines.push_back(line);line="";
      if (I<(N-1) && s[I+1]=='\n') I++;
    }
    else if (ch=='\n')
    {
      lines.push_back(line);line="";
      if (I<(N-1) && s[I+1]=='\r') I++;
    }
    else
    {
      line.push_back(ch);
    }
  }

  if (!line.empty())
    lines.push_back(line);

  return lines;
}

///////////////////////////////////////////////////////////////////////////
std::vector<String> StringUtils::getNonEmptyLines(const String& s) 
{
  std::vector<String> lines=getLines(s);
  std::vector<String> ret;
  for (int I=0;I<(int)lines.size();I++)
  {
    if (!StringUtils::trim(lines[I]).empty()) 
      ret.push_back(lines[I]);
  }
  return ret;
}



///////////////////////////////////////////////////////////////////////////
std::vector<String> StringUtils::getLinesAndPurgeComments(String source,String commentString) 
{
  std::vector<String> lines=StringUtils::getNonEmptyLines(source),ret;
  ret.reserve(lines.size());
  for (int I=0;I<(int)lines.size();I++)
  {
    String L=StringUtils::trim(lines[I]);
    if (!StringUtils::startsWith(L,commentString)) 
      ret.push_back(L);
  }
  return ret;
}


///////////////////////////////////////////////////////////////////////////
String StringUtils::base64Encode(const String& input)
{
  auto temp=HeapMemory::createUnmanaged((unsigned char*)input.c_str(),input.size());
  return temp->base64Encode();
}

///////////////////////////////////////////////////////////////////////////
String StringUtils::base64Decode(const String& input)
{
  auto tmp=HeapMemory::base64Decode(input);
  if (!tmp) {VisusAssert(false);return "";}
  return String((const char*)tmp->c_ptr(),(size_t)tmp->c_size());
}

///////////////////////////////////////////////////////////////////////////
String StringUtils::sha256(String input,String key)
{
  char ret[EVP_MAX_MD_SIZE];
  unsigned int  size;
  
	Create_HMAC_Context(ctx);
  HMAC_Init_ex(ctx, key.c_str(), (int)key.size(), EVP_sha256(), NULL);
  HMAC_Update(ctx, (const unsigned char*)input.c_str(), input.size());
  HMAC_Final(ctx, (unsigned char*)ret, &size);
  Destroy_HMAC_Context(ctx);

  return String(ret,(size_t)size);
}

///////////////////////////////////////////////////////////////////////////
String StringUtils::sha1(String input,String key)
{
  char ret[EVP_MAX_MD_SIZE];
  unsigned int size;
  
  Create_HMAC_Context(ctx);
  HMAC_Init_ex(ctx, key.c_str(), (int)key.size(), EVP_sha1(), NULL);
  HMAC_Update(ctx, (const unsigned char*)input.c_str(), input.size());
  HMAC_Final(ctx, (unsigned char*)ret, &size);
  Destroy_HMAC_Context(ctx);
  
  return String(ret,(size_t)size);
}


////////////////////////////////////////////////////////////////////////
String StringUtils::md5(const String& input)
{
  Uint8 computed[MD5_DIGEST_LENGTH];
  MD5((const unsigned char*) input.c_str(), input.size(), computed);

  std::ostringstream out;
  for(int i=0; i <MD5_DIGEST_LENGTH; i++)
    out << std::hex << std::setfill('0') << std::setw(2) << static_cast<Int32>(computed[i]); 

  return out.str();
}

///////////////////////////////////////////////////////////////////////////
String StringUtils::encodeForFilename(String value)
{
  String ret;
  for (int I=0;I<(int)value.size();I++)
  {
    if (std::isalnum(value[I]) || value[I]=='_')
      ret.push_back(value[I]);
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////
String StringUtils::getDateTimeForFilename()
{
  return Time::now().getFormattedLocalTime();
}




//////////////////////////////////////////////////////////////
Int64 StringUtils::getByteSizeFromString(String value)
{
  const Int64 GB=1024*1024*1024;
  const Int64 MB=1024*1024;
  const Int64 KB=1024;

  value=StringUtils::toLower(StringUtils::trim(value));

  if (value=="-1") 
    return (Int64)-1;

  Int64 multiply=1;
  int len=(int)value.length();
  for (int I=0;I<len;I++)
  {
    if (value[I]=='.' || std::isdigit(value[I]))
      continue;

    String unit=value.substr(I);
    value=value.substr(0,I);
    if      (unit=="gb") multiply=GB;
    else if (unit=="mb") multiply=MB;
    else if (unit=="kb") multiply=KB;
    break;
  }

  double size=0;
  std::istringstream in(value);
  in>>size;

  return (Int64)(size*multiply);
}

//////////////////////////////////////////////////////////////
String StringUtils::getStringFromByteSize(Int64 size)
{
  const Int64 GB=1024*1024*1024;
  const Int64 MB=1024*1024;
  const Int64 KB=1024;

  std::ostringstream out;
  if      (size==(Int64)-1)  out<<"-1";
  else if (size>=GB)              out<< std::fixed << std::setprecision(1)<<((double)size/(double)GB)<<"gb";
  else if (size>=MB)              out<< std::fixed << std::setprecision(1)<<((double)size/(double)MB)<<"mb";
  else if (size>=KB)              out<< std::fixed << std::setprecision(1)<<((double)size/(double)KB)<<"kb";
  else                            out<< size;
  return out.str();
}


//////////////////////////////////////////////////////////////////////////
static const char ESCAPE_CHARS[256] =
{
  /*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
  /* 0 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* 1 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* 2 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* 3 */ 1,1,1,1, 1,1,1,1, 1,1,0,0, 0,0,0,0,
      
  /* 4 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
  /* 5 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
  /* 6 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
  /* 7 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
      
  /* 8 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* 9 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* A */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* B */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
      
  /* C */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* D */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* E */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
  /* F */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
};

String StringUtils::addEscapeChars(String src)
{
  // Only alphanum is safe.
  const char DEC2HEX[16 + 1] = "0123456789ABCDEF";
  const char * pSrc = (const char *)src.c_str();
  const int SRC_LEN = (int)src.size();

  Array pStart;
  bool bOk = pStart.resize(SRC_LEN * 3, DTypes::UINT8, __FILE__, __LINE__);
  VisusAssert(bOk);

  char * pEnd = (char*)pStart.c_ptr();
  const char * const SRC_END = pSrc + SRC_LEN;

  for (; pSrc < SRC_END; ++pSrc)
  {
    if (ESCAPE_CHARS[(int)*pSrc])
    {
      *pEnd++ = *pSrc;
    }
    else
    {
      // escape this char
      *pEnd++ = '%';
      *pEnd++ = DEC2HEX[*pSrc >> 4];
      *pEnd++ = DEC2HEX[*pSrc & 0x0F];
    }
  }

  String ret((char*)pStart.c_ptr(), pEnd);
  return ret;
}



////////////////////////////////////////////////////////////////////////////////////
static const char HEX2DEC[256] = 
{
  /*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
  /* 0 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* 1 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* 2 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* 3 */  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,
      
  /* 4 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* 5 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* 6 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* 7 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
      
  /* 8 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* 9 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* A */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* B */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
      
  /* C */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* D */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* E */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  /* F */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
};

String StringUtils::removeEscapeChars(String src)
{
  const char * pSrc = src.c_str();
  const int SRC_LEN = (int)src.size();
  const char * const SRC_END = pSrc + SRC_LEN;
  const char * const SRC_LAST_DEC = SRC_END - 2;   // last decodable '%' 

  Array pStart;
  bool bOk=pStart.resize(SRC_LEN,DTypes::UINT8,__FILE__,__LINE__);
  VisusAssert(bOk);

    char * pEnd = (char*)pStart.c_ptr();

    while (pSrc < SRC_LAST_DEC)
  {
    if (*pSrc == '%')
        {
            char dec1, dec2;
            if (-1 != (dec1 = HEX2DEC[(int)*(pSrc + 1)])
                && -1 != (dec2 = HEX2DEC[(int)*(pSrc + 2)]))
            {
                *pEnd++ = (dec1 << 4) + dec2;
                pSrc += 3;
                continue;
            }
        }

        *pEnd++ = *pSrc++;
  }

    // the last 2- chars
    while (pSrc < SRC_END)
        *pEnd++ = *pSrc++;

  String ret((char*)pStart.c_ptr(),pEnd);
  return ret;
}


////////////////////////////////////////////////////
ParseStringParams::ParseStringParams(String with_params,String question_sep,String and_sep,String equal_sep)
{
  this->source=with_params;

  int question_index=StringUtils::find(with_params,question_sep);
  if (question_index>=0)
  {
    std::vector<String> v=StringUtils::split(with_params.substr(question_index+1),and_sep);
    this->without_params=StringUtils::trim(with_params.substr(0,question_index));
    for (int i=0;i<(int)v.size();i++)
    {
      String key,value;
      int equal_index=(int)v[i].find(equal_sep);
      if (equal_index>=0)
      {
        key  =v[i].substr(0,equal_index);
        value=v[i].substr(equal_index+1);
      }
      else
      {
        key  =v[i];
        value="";
      }

      key  = StringUtils::trim(key);
      value= StringUtils::removeEscapeChars(value);
      if (!key.empty()) 
        this->params.setValue(key,value);
    }
  }
  else
  {
    this->without_params=with_params;
  }
}



} //namespace Visus

