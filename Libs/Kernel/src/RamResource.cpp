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

#include <Visus/RamResource.h>
#include <Visus/Utils.h>
#include <Visus/StringTree.h>
#include <Visus/StringUtils.h>

#if WIN32
#include <Windows.h>
#include <Psapi.h>

#elif __clang__
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/task.h>
#include <mach/mach_init.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_host.h>

#else
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#endif

namespace Visus {

VISUS_IMPLEMENT_SINGLETON_CLASS(RamResource)

///////////////////////////////////////////////////////////////////////////
RamResource::RamResource() 
{
  //os_total_memory
  {
    #if WIN32
    {
      MEMORYSTATUSEX status;
      status.dwLength = sizeof(status);
      GlobalMemoryStatusEx(&status);
      os_total_memory=status.ullTotalPhys;
    }
    #elif __clang__
    {
      int mib[2]={CTL_HW,HW_MEMSIZE};
      size_t length = sizeof(os_total_memory);
      os_total_memory=0;
      sysctl(mib, 2, &os_total_memory, &length, NULL, 0);
    }
    #else
    {
      struct sysinfo memInfo;
      sysinfo (&memInfo);
      os_total_memory=((Int64)memInfo.totalram)*memInfo.mem_unit;
    }
    #endif
  }
}

///////////////////////////////////////////////////////////////////////////
RamResource::~RamResource()
{}

///////////////////////////////////////////////////////////////////////////
Int64 RamResource::getVisusUsedMemory() const
{
  #if WIN32
  {
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof (pmc));
    return pmc.PagefileUsage;
  }
  #elif __clang__
  {
    struct task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;
    task_info(current_task(), TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count);
    size_t size = t_info.resident_size;
    return (Int64)size;
  }
  #else
  {
    long rss = 0L;
    FILE* fp = NULL;
    if ( (fp = fopen( "/proc/self/statm", "r" ))== NULL) return 0;
    if (fscanf( fp, "%*s%ld", &rss ) != 1 )
      {fclose( fp ); return 0; }
    fclose(fp);
    return (Int64)rss * (Int64)sysconf( _SC_PAGESIZE);
  }
  #endif
}

///////////////////////////////////////////////////////////////////////////
Int64 RamResource::getOsUsedMemory() const
{
  #if WIN32
  {
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys-status.ullAvailPhys;
  }
  #elif __clang__
  {
    vm_statistics_data_t vm_stats;
    mach_port_t mach_port = mach_host_self();
    mach_msg_type_number_t count = sizeof(vm_stats) / sizeof(natural_t);
    vm_size_t page_size;
    host_page_size(mach_port, &page_size);
    host_statistics(mach_port, HOST_VM_INFO,(host_info_t)&vm_stats, &count);
    return ((int64_t)vm_stats.active_count +
            (int64_t)vm_stats.inactive_count +
            (int64_t)vm_stats.wire_count) *  (int64_t)page_size;
  }
  #else
  {
    struct sysinfo memInfo;
    sysinfo (&memInfo);

    Int64 ret = memInfo.totalram - memInfo.freeram;    
    //Read /proc/meminfo to get cached ram (freed upon request)
    FILE *fp;
    int MAXLEN = 1000;
    char buf[MAXLEN];
    fp = fopen("/proc/meminfo", "r");

    if(fp){
      for(int i = 0; i <= 3; i++) {
        if (fgets(buf, MAXLEN, fp)==nullptr)
          buf[0]=0;
      }
      char *p1 = strchr(buf, (int)':');
      unsigned long cacheram = strtoull(p1+1, NULL, 10)*1000;
      ret -= cacheram;
      fclose(fp);
    }
    
    ret *= memInfo.mem_unit;
    return ret;
  }
  #endif
}

//////////////////////////////////////////////////////////////////
void RamResource::setOsTotalMemory(Int64 value)
{
  VisusAssert(value > 0);
  os_total_memory=value;
}

//////////////////////////////////////////////////////////////////
bool RamResource::allocateMemory(Int64 reqsize)
{
  VisusAssert(reqsize>=0);

  //NOTE if os_total_memory==0 means that no limit is imposed by the visus.config
  if (!reqsize || !os_total_memory)
  {
    return true;
  }
  else
  {
    ScopedLock lock(this->lock);
    Int64 os_free_memory=((Int64)(getOsTotalMemory()*0.80))-getVisusUsedMemory();
    if (reqsize>os_free_memory)
    {
#if 0
      PrintWarning("RamResource out of memory ",
                    "reqsize(",StringUtils::getStringFromByteSize(reqsize),
                    "visus_used_memory", StringUtils::getStringFromByteSize(getVisusUsedMemory()),
                    "os_used_memory", StringUtils::getStringFromByteSize(getOsUsedMemory()),
                    "os_total_memory", StringUtils::getStringFromByteSize(getOsTotalMemory()),
                    "visus_free_memory", StringUtils::getStringFromByteSize(os_free_memory));
#endif
      return false;
    }
    return true;
  }
}

//////////////////////////////////////////////////////////////////
bool RamResource::freeMemory(Int64 reqsize)
{
  VisusAssert(reqsize>=0);
  return true;
}


} //namespace Visus



