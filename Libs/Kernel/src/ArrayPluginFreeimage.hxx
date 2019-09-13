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

#ifndef VISUS_ARRAY_PLUGIN_FREEIMAGE_H
#define VISUS_ARRAY_PLUGIN_FREEIMAGE_H

#include <Visus/Array.h>
#include <Visus/NetMessage.h>
#include <Visus/Encoder.h>
#include <Visus/NetService.h>
#include <Visus/Log.h>

#if WIN32
#include <WinSock2.h>

#elif __APPLE__

#else
#include <arpa/inet.h>

#endif

#include <FreeImage.h>

namespace Visus {

class VISUS_KERNEL_API FreeImageArrayPlugin : public ArrayPlugin
{
public:

  VISUS_NON_COPYABLE_CLASS(FreeImageArrayPlugin)

  //constructor
  FreeImageArrayPlugin()
  {
    FreeImage_Initialise(1);
  }

  //destructor
  virtual ~FreeImageArrayPlugin()
  {
    FreeImage_DeInitialise();
  }

  //handleStatImage
  virtual StringTree handleStatImage(String url) override {
    StringTree info;
    handleLoadImageWithInfo(url, &info, std::vector<String>());
    return info;
  }

  //handleLoadImage
  virtual Array handleLoadImage(String url, std::vector<String> args) override {
    return handleLoadImageWithInfo(url, nullptr, args);
  }

  //handleSaveImage
  virtual bool handleSaveImage(String url_, Array src, std::vector<String> args) override
  {
    Url url(url_);

    if (!url.isFile())
      return false;

    String filename = url.getPath();

    //check if the extension is supportedd
    FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(filename.c_str());
    if (fif == FIF_UNKNOWN)
      return false;

    FIBITMAP* bitmap = ArrayToFreeImage(src);
    if (!bitmap)
      return false;

    if (!FreeImage_Save(fif, bitmap, filename.c_str(), 0))
    {
      FreeImage_Unload(bitmap);
      VisusWarning() << "FreeImageArrayPlugin::handleSaveImage failed filename(" << filename << ")";
      return false;
    }
    else
    {
      FreeImage_Unload(bitmap);
      VisusInfo() << "saved(" << filename << ") done (dtype=" << src.dtype.toString() << ")";
      return true;
    }
  }

  //handleLoadImageFromMemory
  virtual Array handleLoadImageFromMemory(SharedPtr<HeapMemory> heap, std::vector<String> args) override
  {
    FIMEMORY* memory = FreeImage_OpenMemory((BYTE*)heap->c_ptr(), (int)heap->c_size());

    DoAtExit do_at_exit([memory]() {FreeImage_CloseMemory(memory); });

    FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(memory, 0);

    if (fif == FIF_UNKNOWN)
      return Array();

    FIBITMAP* bitmap = FreeImage_LoadFromMemory(fif, memory);
    if (!bitmap)
      return Array();

    Array ret = FreeImageToArray(bitmap);
    FreeImage_Unload(bitmap);
    return ret;
  }

private:

  //handleLoadImage
  Array handleLoadImageWithInfo(String url_, StringTree *info, std::vector<String> args)
  {
    Url url(url_);

    if (url.isRemote())
    {
      auto response = NetService::getNetResponse(url);
      if (!response.isSuccessful() || !response.body || !response.body->c_size())
        return Array();

      FIMEMORY* memory = FreeImage_OpenMemory((BYTE*)response.body->c_ptr(), (int)response.body->c_size());
      FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(memory, 0);

      if (fif == FIF_UNKNOWN)
        return Array();

      FIBITMAP* bitmap = FreeImage_LoadFromMemory(fif, memory);


      if (!bitmap) {
        FreeImage_CloseMemory(memory);
        return Array();
      }

      Array ret;
      if (info)
      {
        info->writeString("format", "FreeImageArrayPlugin/remote");
        info->writeString("url", url_);
        GetImageInfo(bitmap, *info);
      }
      else
      {
        ret = FreeImageToArray(bitmap);
      }

      FreeImage_Unload(bitmap);
      FreeImage_CloseMemory(memory);
      return ret;
    }

    if (!url.isFile())
      return Array();

    String filename = url.getPath();

    FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(filename.c_str(), 0);

    if (fif == FIF_UNKNOWN)
      fif = FreeImage_GetFIFFromFilename(filename.c_str());

    if (fif == FIF_UNKNOWN)
    {
      VisusWarning() << "does not know the file format of the filename(" << filename << ") (FIF_UNKNOWN)";
      return Array();
    }

    int npage = -1;
    for (int I = 0; I<(int)args.size(); I++)
    {
      if (args[I] == "--page")
      {
        npage = cint(args[++I]);
      }
    }

    //added support for multipages (example TIFF)
    if (npage >= 0)
    {
      FIMULTIBITMAP* multibitmap = FreeImage_OpenMultiBitmap(fif, filename.c_str(),/*create_new*/false,/*read_only*/true);
      if (!multibitmap)
      {
        VisusWarning() << "FreeImage:: FreeImage_OpenMultiBitmap(" << filename << ") does not seems to be a multipage bitmap (you specified --page)";
        return Array();
      }

      DoAtExit do_at_exit([multibitmap]() {FreeImage_CloseMultiBitmap(multibitmap); });

      int totpages = FreeImage_GetPageCount(multibitmap);
      if (info)
      {
        info->writeString("totpages", cstring(totpages));
        info->writeString("page", cstring(npage));
      }

      if (!(npage >= 0 && npage<totpages))
      {
        VisusWarning() << "FreeImage:: file(" << filename << ") --page " << npage << " wrong, use a range in [0," << (totpages - 1) << "]";
        return Array();
      }

      FIBITMAP* bitmap = FreeImage_LockPage(multibitmap, npage);
      if (!bitmap)
        return Array();

      Array ret;
      if (info)
      {
        info->writeString("format", "FreeImageArrayPlugin/file/multipage");
        info->writeString("url", url_);
        GetImageInfo(bitmap, *info);
      }
      else
      {
        ret = FreeImageToArray(bitmap);
      }

      FreeImage_UnlockPage(multibitmap, bitmap,/*changed*/false);
      return ret;
    }
    else
    {
      FIBITMAP* bitmap = FreeImage_Load(fif, filename.c_str(), 0);
      if (!bitmap)
        return Array();

      Array ret;
      if (info)
      {
        info->writeString("format", "FreeImageArrayPlugin/file");
        info->writeString("url", url_);
        GetImageInfo(bitmap, *info);
      }
      else
      {
        ret = FreeImageToArray(bitmap);
      }

      FreeImage_Unload(bitmap);
      return ret;
    }
  }

  static void GetImageInfo(FIBITMAP* bitmap, StringTree &imginfo)
  {
    BITMAPINFO* info = FreeImage_GetInfo(bitmap);
    int width = FreeImage_GetWidth(bitmap);
    int height = FreeImage_GetHeight(bitmap);
    int bpp = FreeImage_GetBPP(bitmap);

    //something wrong
    if (!bpp || !width || !height)
    {
      VisusWarning() << "FreeImage:: FreeImage returned wrong dimension (something is wrong)";
      return;
    }

    //dims
    {
      PointNi dims = PointNi::one(2);
      dims[0] = width;
      dims[1] = height;
      imginfo.writeString("dims", dims.toString());
    }

    //guess dtype
    DType dtype;
    FREE_IMAGE_TYPE format = FreeImage_GetImageType(bitmap);
    switch (format)
    {
    case FIT_BITMAP:
    {
      if (bpp == 8)    dtype = DTypes::UINT8;
      else if (bpp == 16)   dtype = DTypes::UINT8_GA;
      else if (bpp == 24)   dtype = DTypes::UINT8_RGB;
      else if (bpp == 32)   dtype = DTypes::UINT8_RGBA;
      break;
    }
    case FIT_UINT16:      dtype = DTypes::UINT16;       break;
    case FIT_INT16:       dtype = DTypes::INT16;        break;
    case FIT_UINT32:      dtype = DTypes::UINT32;       break;
    case FIT_INT32:       dtype = DTypes::INT32;        break;
    case FIT_FLOAT:       dtype = DTypes::FLOAT32;      break;
    case FIT_DOUBLE:      dtype = DTypes::FLOAT64;      break;
    case FIT_COMPLEX:     dtype = DTypes::FLOAT64_GA;   break;
    case FIT_RGB16:       dtype = DTypes::UINT16_RGB;   break;
    case FIT_RGBF:        dtype = DTypes::FLOAT32_RGB;  break;
    case FIT_RGBA16:      dtype = DTypes::UINT16_RGBA;  break;
    case FIT_RGBAF:       dtype = DTypes::FLOAT32_RGBA; break;
    default:              VisusAssert(false);
    }

    StringTree* fields = imginfo.addChild(StringTree("fields"));
    StringTree* field = fields->addChild(StringTree("field"));
    field->writeString("dtype", dtype.toString());
  }

  static Array FreeImageToArray(FIBITMAP* bitmap_)
  {
    FIBITMAP* bitmap = (FIBITMAP*)bitmap_;
    if (!bitmap)
    {
      VisusWarning() << "bitmap is nullptr probably FreeImage_Load failed";
      return Array();
    }

    Time t1 = Time::now();
    BITMAPINFO* info = FreeImage_GetInfo(bitmap);
    int width = FreeImage_GetWidth(bitmap);
    int height = FreeImage_GetHeight(bitmap);
    int bpp = FreeImage_GetBPP(bitmap);
    FREE_IMAGE_TYPE format = FreeImage_GetImageType(bitmap);

    //something wrong
    if (!bpp || !width || !height)
    {
      VisusWarning() << "FreeImage:: FreeImage returned wrong dimension ";
      return Array();
    }

    Array dst;

    auto allocateArray = [&](DType dtype)
    {
      if (!dst.resize(width, height, dtype, __FILE__, __LINE__))
      {
        VisusWarning() << "allocateArray failed, out of memory";
        return false;
      }
      return true;
    };

    //FIT_BITMAP/8
    if (format == FIT_BITMAP && bpp == 8)
    {
      //what this code means?
#if 1
      {
        if (FreeImage_IsTransparent(bitmap))
        {
          FIBITMAP* bitmap_rgba = FreeImage_ConvertTo32Bits(bitmap);
          Array dst = FreeImageToArray(bitmap_rgba);
          FreeImage_Unload(bitmap_rgba);
          return dst;
        }

        if (FreeImage_GetColorType(bitmap) != FIC_MINISBLACK)
        {
          FIBITMAP* bitmap_rgb = FreeImage_ConvertTo24Bits(bitmap);
          Array dst = FreeImageToArray(bitmap_rgb);
          FreeImage_Unload(bitmap_rgb);
          return dst;
        }
      }
#endif

      if (!allocateArray(DTypes::UINT8))
        return Array();

      Uint8* dst_p = (Uint8*)dst.c_ptr();
      for (int y = 0; y < height; y++)
      {
        Uint8 *src_p = (Uint8 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++)
        {
          *dst_p++ = *src_p++;
        }
      }
#ifdef VISUS_DEBUG
      VisusInfo() << "load done (dtype=" << dst.dtype.toString() << ") FREE_IMAGE_TYPE(FIT_BITMAP/8) in " << cstring((int)t1.elapsedMsec()) << "msec";
#endif
      return dst;
    }

    //FIT_BITMAP/16
    if (format == FIT_BITMAP && bpp == 16)
    {
      if (!allocateArray(DTypes::UINT8_GA))
        return Array();

      Uint8* dst_p = (Uint8*)dst.c_ptr();
      for (int y = 0; y < height; y++)
      {
        Uint8 *src_p = (Uint8 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++)
        {
          *dst_p++ = *src_p++;
          *dst_p++ = *src_p++;
        }
      }
#ifdef VISUS_DEBUG
      VisusInfo() << "load done (dtype=" << dst.dtype.toString() << ") FREE_IMAGE_TYPE(FIT_BITMAP/16) in " << cstring((int)t1.elapsedMsec()) << "msec";
#endif
      return dst;
    }

    //FIT_BITMAP/24
    if (format == FIT_BITMAP && bpp == 24)
    {
      if (!allocateArray(DTypes::UINT8_RGB))
        return Array();

      Uint8* dst_p = (Uint8*)dst.c_ptr();
      for (int y = 0; y < height; y++)
      {
        Uint8 *src_p = (Uint8 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++)
        {
          *dst_p++ = src_p[FI_RGBA_RED];
          *dst_p++ = src_p[FI_RGBA_GREEN];
          *dst_p++ = src_p[FI_RGBA_BLUE];
          src_p += 3;
        }
      }

#ifdef VISUS_DEBUG
      VisusInfo() << "load done (dtype=" << dst.dtype.toString() << ") FREE_IMAGE_TYPE(FIT_BITMAP/24) in " << cstring((int)t1.elapsedMsec()) << "msec";
#endif
      return dst;
    }

    //FIT_BITMAP/32
    if (format == FIT_BITMAP && bpp == 32)
    {
      if (!allocateArray(DTypes::UINT8_RGBA))
        return Array();

      Uint8* dst_p = (Uint8*)dst.c_ptr();
      for (int y = 0; y < height; y++)
      {
        Uint8 *src_p = (Uint8 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++)
        {
          *dst_p++ = src_p[FI_RGBA_RED];
          *dst_p++ = src_p[FI_RGBA_GREEN];
          *dst_p++ = src_p[FI_RGBA_BLUE];
          *dst_p++ = src_p[FI_RGBA_ALPHA];
          src_p += 4;
        }
      }

#ifdef VISUS_DEBUG
      VisusInfo() << "load done (dtype=" << dst.dtype.toString() << ") FREE_IMAGE_TYPE(FIT_BITMAP/32) in " << cstring((int)t1.elapsedMsec()) << "msec";
#endif
      return dst;
    }

    //FIT_UINT16
    if (format == FIT_UINT16)
    {
      if (!allocateArray(DTypes::UINT16))
        return Array();

      Uint16* dst_p = (Uint16*)dst.c_ptr();
      for (int y = 0; y < height; y++)
      {
        Uint16 *src_p = (Uint16*)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++) {
          *dst_p++ = *src_p++;
        }
      }

#ifdef VISUS_DEBUG
      VisusInfo() << "load done (dtype=" << dst.dtype.toString() << ")  FREE_IMAGE_TYPE(FIT_UINT16) in " << cstring((int)t1.elapsedMsec()) << "msec";
#endif
      return dst;
    }

    //FIT_INT16
    if (format == FIT_INT16)
    {
      if (!allocateArray(DTypes::INT16))
        return Array();

      Int16* dst_p = (Int16*)dst.c_ptr();
      for (int y = 0; y < height; y++)
      {
        Int16 *src_p = (Int16 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++) {
          *dst_p++ = *src_p++;
        }
      }

#ifdef VISUS_DEBUG
      VisusInfo() << "load done (dtype=" << dst.dtype.toString() << ")  FREE_IMAGE_TYPE(FIT_INT16) in " << cstring((int)t1.elapsedMsec()) << "msec";
#endif
      return dst;
    }

    //FIT_UINT32
    if (format == FIT_UINT32)
    {
      if (!allocateArray(DTypes::UINT32))
        return Array();

      Uint32* dst_p = (Uint32*)dst.c_ptr();
      for (int y = 0; y < height; y++)
      {
        DWORD *src_p = (DWORD*)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++) {
          *dst_p++ = *src_p++;
        }
      }

#ifdef VISUS_DEBUG
      VisusInfo() << "load done (dtype=" << dst.dtype.toString() << ") FREE_IMAGE_TYPE(FIT_UINT32) in " << cstring((int)t1.elapsedMsec()) << "msec";
#endif
      return dst;
    }

    //FIT_INT32
    if (format == FIT_INT32)
    {
      if (!allocateArray(DTypes::INT32))
        return Array();

      Int32* dst_p = (Int32*)dst.c_ptr();
      for (int y = 0; y < height; y++)
      {
        LONG *src_p = (LONG *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++) {
          *dst_p++ = *src_p++;
        }
      }
#ifdef VISUS_DEBUG
      VisusInfo() << "load done (dtype=" << dst.dtype.toString() << ") FREE_IMAGE_TYPE(FIT_INT32) in " << cstring((int)t1.elapsedMsec()) << "msec";
#endif
      return dst;
    }

    //FIT_FLOAT
    if (format == FIT_FLOAT)
    {
      if (!allocateArray(DTypes::FLOAT32))
        return Array();

      Float32* dst_p = (Float32*)dst.c_ptr();
      for (int y = 0; y < height; y++)
      {
        Float32 *src_p = (Float32 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++) {
          *dst_p++ = *src_p++;
        }
      }
#ifdef VISUS_DEBUG
      VisusInfo() << "load done (dtype=" << dst.dtype.toString() << ") FREE_IMAGE_TYPE(FIT_FLOAT) in " << cstring((int)t1.elapsedMsec()) << "msec";
#endif
      return dst;
    }

    //FIT_DOUBLE
    if (format == FIT_DOUBLE)
    {
      if (!allocateArray(DTypes::FLOAT64))
        return Array();

      Float64* dst_p = (Float64*)dst.c_ptr();
      for (int y = 0; y < height; y++)
      {
        Float64 *src_p = (Float64 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++) {
          *dst_p++ = *src_p++;
        }
      }
#ifdef VISUS_DEBUG
      VisusInfo() << "load done (dtype=" << dst.dtype.toString() << ") FREE_IMAGE_TYPE(FIT_DOUBLE) in " << cstring((int)t1.elapsedMsec()) << "msec";
#endif
      return dst;
    }

    //FIT_COMPLEX
    if (format == FIT_COMPLEX)
    {
      if (!allocateArray(DTypes::FLOAT64_GA))
        return Array();

      Float64* dst_p = (Float64*)dst.c_ptr();
      for (int y = 0; y < height; y++)
      {
        FICOMPLEX *src_p = (FICOMPLEX *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++, src_p++)
        {
          *dst_p++ = src_p->r;
          *dst_p++ = src_p->i;
        }
      }
#ifdef VISUS_DEBUG
      VisusInfo() << "load done (dtype=" << dst.dtype.toString() << ") FREE_IMAGE_TYPE(FIT_COMPLEX) in " << cstring((int)t1.elapsedMsec()) << "msec";
#endif
      return dst;
    }

    //FIT_RGB16
    if (format == FIT_RGB16)
    {
      if (!allocateArray(DTypes::UINT16_RGB))
        return Array();

      Uint16* dst_p = (Uint16*)dst.c_ptr();
      for (int y = 0; y < height; y++)
      {
        FIRGB16 *src_p = (FIRGB16 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++, src_p++)
        {
          *dst_p++ = src_p->red;
          *dst_p++ = src_p->green;
          *dst_p++ = src_p->blue;
        }
      }
#ifdef VISUS_DEBUG
      VisusInfo() << "load done (dtype=" << dst.dtype.toString() << ") FREE_IMAGE_TYPE(FIT_RGB16) in " << cstring((int)t1.elapsedMsec()) << "msec";
#endif
      return dst;
    }

    //FIT_RGBF
    if (format == FIT_RGBF)
    {
      if (!allocateArray(DTypes::FLOAT32_RGB))
        return Array();

      Float32* dst_p = (Float32*)dst.c_ptr();
      for (int y = 0; y < height; y++)
      {
        FIRGBF *src_p = (FIRGBF *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++, src_p++)
        {
          *dst_p++ = src_p->red;
          *dst_p++ = src_p->green;
          *dst_p++ = src_p->blue;
        }
      }
#ifdef VISUS_DEBUG
      VisusInfo() << "load done (dtype=" << dst.dtype.toString() << ") FREE_IMAGE_TYPE(FIT_RGBF) in " << cstring((int)t1.elapsedMsec()) << "msec";
#endif
      return dst;
    }

    //FIT_RGBA16
    if (format == FIT_RGBA16)
    {
      if (!allocateArray(DTypes::UINT16_RGBA))
        return Array();

      Uint16* dst_p = (Uint16*)dst.c_ptr();
      for (int y = 0; y < height; y++)
      {
        FIRGBA16 *src_p = (FIRGBA16 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++, src_p++)
        {
          *dst_p++ = src_p->red;
          *dst_p++ = src_p->green;
          *dst_p++ = src_p->blue;
          *dst_p++ = src_p->alpha;
        }
      }
#ifdef VISUS_DEBUG
      VisusInfo() << "load done (dtype=" << dst.dtype.toString() << ") FREE_IMAGE_TYPE(FIT_RGBA16) in " << cstring((int)t1.elapsedMsec()) << "msec";
#endif
      return dst;
    }

    //FIT_RGBAF
    if (format == FIT_RGBAF)
    {
      if (!allocateArray(DTypes::FLOAT32_RGBA))
        return Array();

      Float32* dst_p = (Float32*)dst.c_ptr();
      for (int y = 0; y < height; y++)
      {
        FIRGBAF *src_p = (FIRGBAF *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++, src_p++)
        {
          *dst_p++ = src_p->red;
          *dst_p++ = src_p->green;
          *dst_p++ = src_p->blue;
          *dst_p++ = src_p->alpha;
        }
      }
#ifdef VISUS_DEBUG
      VisusInfo() << "load done (dtype=" << dst.dtype.toString() << ") FREE_IMAGE_TYPE(FIT_RGBAF) in " << cstring((int)t1.elapsedMsec()) << "msec";
#endif 
      return dst;
    }

    VisusWarning() << "load failed. unsupported FREE_IMAGE_TYPE(" << format << ")";
    return Array();
  }

  static VISUS_NEWOBJECT(FIBITMAP*) ArrayToFreeImage(Array& src)
  {
    auto dims = Utils::select<Int64>(src.dims.toVector(), [](Int64 value) {return value > 1; });

    if (dims.size()!=2)
    {
      VisusWarning() << "data input is not 2d, dims("<< src.dims.toString()<<")";
      return nullptr;
    }

    Int64 width  = dims[0];
    Int64 height = dims[1];
    DType dtype = src.dtype;

    FREE_IMAGE_TYPE type = FIT_UNKNOWN;
    int bpp = 0;

    if (dtype == DTypes::UINT8) { type = FIT_BITMAP; bpp = 8; }
    else if (dtype == DTypes::UINT8_GA) { type = FIT_BITMAP; bpp = 16; }
    else if (dtype == DTypes::UINT8_RGB) { type = FIT_BITMAP; bpp = 24; }
    else if (dtype == DTypes::UINT8_RGBA) { type = FIT_BITMAP; bpp = 32; }
    else if (dtype == DTypes::INT16) { type = FIT_INT16; bpp = 8; }
    else if (dtype == DTypes::UINT16) { type = FIT_UINT16; bpp = 8; }
    else if (dtype == DTypes::UINT16_RGB) { type = FIT_RGB16; bpp = 8; }
    else if (dtype == DTypes::UINT16_RGBA) { type = FIT_RGBA16; bpp = 8; }
    else if (dtype == DTypes::UINT32) { type = FIT_UINT32; bpp = 8; }
    else if (dtype == DTypes::INT32) { type = FIT_INT32; bpp = 8; }
    else if (dtype == DTypes::FLOAT32) { type = FIT_FLOAT; bpp = 8; }
    else if (dtype == DTypes::FLOAT32_RGB) { type = FIT_RGBF; bpp = 8; }
    else if (dtype == DTypes::FLOAT32_RGBA) { type = FIT_RGBAF; bpp = 8; }
    else if (dtype == DTypes::FLOAT64) { type = FIT_DOUBLE; bpp = 8; }
    else if (dtype == DTypes::FLOAT64_GA) { type = FIT_COMPLEX; bpp = 8; }
    else
    {
      VisusWarning() << "FreeImage does not know how to handle dtype(" << dtype.toString() << ")";
      return nullptr;
    }

    FIBITMAP* bitmap = FreeImage_AllocateT(type, (int)width, (int)height, bpp);
    if (!bitmap)
    {
      VisusWarning() << "FreeImage_AllocateT(...) failed, probably out of memory";
      return nullptr;
    }

    //UINT8
    if (dtype == (DTypes::UINT8))
    {
      //don't remember why I need a palette here
      RGBQUAD *palette = FreeImage_GetPalette(bitmap);
      for (int I = 0; I < 256; I++)
      {
        palette[I].rgbRed = I;
        palette[I].rgbGreen = I;
        palette[I].rgbBlue = I;
      }
      Uint8* src_p = (Uint8*)src.c_ptr();
      for (int y = 0; y < height; y++)
      {
        Uint8 *dst_p = (Uint8 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++)
        {
          *dst_p++ = *src_p++;
        }
      }
      return bitmap;
    }

    //UINT8_GA
    if (dtype == (DTypes::UINT8_GA))
    {
      Uint8* src_p = (Uint8*)src.c_ptr();
      for (int y = 0; y < height; y++)
      {
        Uint8 *dst_p = (Uint8 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++)
        {
          *dst_p++ = *src_p++;
          *dst_p++ = *src_p++;
        }
      }
      return bitmap;
    }

    //UINT8_RGB
    if (dtype == DTypes::UINT8_RGB)
    {
      Uint8* src_p = (Uint8*)src.c_ptr();
      for (int y = 0; y < height; y++)
      {
        Uint8 *dst_p = (Uint8 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++)
        {
          dst_p[FI_RGBA_RED] = *src_p++;
          dst_p[FI_RGBA_GREEN] = *src_p++;
          dst_p[FI_RGBA_BLUE] = *src_p++;
          dst_p += 3;
        }
      }
      return bitmap;
    }

    //UINT8_RGBA
    if (dtype == (DTypes::UINT8_RGBA))
    {
      Uint8* src_p = (Uint8*)src.c_ptr();
      for (int y = 0; y < height; y++)
      {
        Uint8 *dst_p = (Uint8 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++)
        {
          dst_p[FI_RGBA_RED] = *src_p++;
          dst_p[FI_RGBA_GREEN] = *src_p++;
          dst_p[FI_RGBA_BLUE] = *src_p++;
          dst_p[FI_RGBA_ALPHA] = *src_p++;
          dst_p += 4;
        }
      }
      return bitmap;
    }

    //UINT16
    if (dtype == (DTypes::UINT16))
    {
      Uint16* src_p = (Uint16*)src.c_ptr();
      for (int y = 0; y < height; y++)
      {
        Uint16 *dst_p = (Uint16*)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++)
        {
          *dst_p++ = *src_p++;
        }
      }
      return bitmap;
    }

    //INT16
    if (dtype == DTypes::INT16)
    {
      Int16* src_p = (Int16*)src.c_ptr();
      for (int y = 0; y < height; y++)
      {
        Int16 *dst_p = (Int16 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++)
        {
          *dst_p++ = *src_p++;
        }
      }
      return bitmap;
    }

    //UINT32
    if (dtype == (DTypes::UINT32))
    {
      Uint32* src_p = (Uint32*)src.c_ptr();
      for (int y = 0; y < height; y++)
      {
        DWORD *dst_p = (DWORD*)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++)
        {
          *dst_p++ = *src_p++;
        }
      }
      return bitmap;
    }

    //INT32
    if (dtype == (DTypes::INT32))
    {
      Int32* src_p = (Int32*)src.c_ptr();
      for (int y = 0; y < height; y++)
      {
        LONG *dst_p = (LONG *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++)
        {
          *dst_p++ = *src_p++;
        }
      }
      return bitmap;
    }

    //FLOAT32
    if (dtype == (DTypes::FLOAT32))
    {
      Float32* src_p = (Float32*)src.c_ptr();
      for (int y = 0; y < height; y++)
      {
        Float32 *dst_p = (Float32 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++)
        {
          *dst_p++ = *src_p++;
        }
      }
      return bitmap;
    }

    //FLOAT64
    if (dtype == (DTypes::FLOAT64))
    {
      Float64* src_p = (Float64*)src.c_ptr();
      for (int y = 0; y < height; y++)
      {
        Float64 *dst_p = (Float64 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++)
        {
          *dst_p++ = *src_p++;
        }
      }
      return bitmap;
    }

    //FLOAT64_GA
    if (dtype == (DTypes::FLOAT64_GA))
    {
      Float64* src_p = (Float64*)src.c_ptr();
      for (int y = 0; y < height; y++)
      {
        FICOMPLEX *dst_p = (FICOMPLEX *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++, dst_p++)
        {
          dst_p->r = *src_p++;
          dst_p->i = *src_p++;
        }
      }
      return bitmap;
    }

    //UINT16_RGB
    if (dtype == (DTypes::UINT16_RGB))
    {
      Uint16* src_p = (Uint16*)src.c_ptr();
      for (int y = 0; y < height; y++)
      {
        FIRGB16 *dst_p = (FIRGB16 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++, dst_p++)
        {
          dst_p->red = *src_p++;
          dst_p->green = *src_p++;
          dst_p->blue = *src_p++;
        }
      }
      return bitmap;
    }

    //FLOAT32_RGB
    if (dtype == (DTypes::FLOAT32_RGB))
    {
      Float32* src_p = (Float32*)src.c_ptr();
      for (int y = 0; y < height; y++)
      {
        FIRGBF *dst_p = (FIRGBF *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++, dst_p++)
        {
          dst_p->red = *src_p++;
          dst_p->green = *src_p++;
          dst_p->blue = *src_p++;
        }
      }
      return bitmap;
    }

    //UINT16_RGBA
    if (dtype == (DTypes::UINT16_RGBA))
    {
      Uint16* src_p = (Uint16*)src.c_ptr();
      for (int y = 0; y < height; y++)
      {
        FIRGBA16 *dst_p = (FIRGBA16 *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++, dst_p++)
        {
          dst_p->red = *src_p++;
          dst_p->green = *src_p++;
          dst_p->blue = *src_p++;
          dst_p->alpha = *src_p++;
        }
      }
      return bitmap;
    }

    //FLOAT32_RGBA
    if (dtype == (DTypes::FLOAT32_RGBA))
    {
      Float32* src_p = (Float32*)src.c_ptr();
      for (int y = 0; y < height; y++)
      {
        FIRGBAF *dst_p = (FIRGBAF *)FreeImage_GetScanLine(bitmap, y);
        for (int x = 0; x < width; x++, dst_p++)
        {
          dst_p->red = *src_p++;
          dst_p->green = *src_p++;
          dst_p->blue = *src_p++;
          dst_p->alpha = *src_p++;
        }
      }
      return bitmap;
    }

    VisusAssert(false);
    return nullptr;
  }

};

} //namespace Visus


#endif //VISUS_ARRAY_PLUGIN_FREEIMAGE_H




