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

#ifndef XIDX_H_
#define XIDX_H_

#if SWIG || VISUS_STATIC_XIDX_LIB
#define VISUS_XIDX_API
#else
#if VISUS_BUILDING_VISUSXIDX
#define VISUS_XIDX_API VISUS_SHARED_EXPORT
#else
#define VISUS_XIDX_API VISUS_SHARED_IMPORT
#endif
#endif

#define VISUS_XIDX_CLASS(Name) \
  VISUS_CLASS(Name)\
  virtual String getTypeName() override {return #Name;} \
  /*--*/

#include <vector>
#include <sstream>

#include <Visus/Kernel.h>
#include <Visus/StringUtils.h>
#include <Visus/DType.h>
#include <Visus/File.h>

namespace Visus {

//////////////////////////////////////////////////////////////////////////////
class VISUS_XIDX_API XIdxModule : public VisusModule
{
public:

  static bool bAttached;

  //attach
  static void attach();

  //detach
  static void detach();
};


} //namespace Visus

#include <Visus/xidx_element.h>

#include <Visus/xidx_datasource.h>
#include <Visus/xidx_attribute.h>
#include <Visus/xidx_dataitem.h>
#include <Visus/xidx_variable.h>
#include <Visus/xidx_domain.h>
#include <Visus/xidx_topology.h>
#include <Visus/xidx_geometry.h>
#include <Visus/xidx_spatial_domain.h>
#include <Visus/xidx_group.h>
#include <Visus/xidx_file.h>

#include <Visus/xidx_list_domain.h>
#include <Visus/xidx_hyperslab_domain.h>
#include <Visus/xidx_multiaxis_domain.h>

#endif //XIDX_H_
