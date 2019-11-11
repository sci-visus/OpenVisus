#include <Visus/VisusXIdx.h>

#include <stdarg.h> 

namespace Visus {

bool XIdxModule::bAttached = false;

//////////////////////////////////////////////////////////////////
void XIdxModule::attach()
{
  if (bAttached)
    return;

  PrintInfo("Attaching XIdxModule...");

  bAttached = true;

  KernelModule::attach();

  PrintInfo("Attached XIdxModule");
}


//////////////////////////////////////////////
void XIdxModule::detach()
{
  if (!bAttached)
    return;

  PrintInfo("Detaching XIdxModule...");

  bAttached = false;

  KernelModule::detach();

  PrintInfo("Detached XIdxModule");
}

//////////////////////////////////////////////
String XIdxFormatString(const String fmt_str, ...)
{
  int final_n, n = ((int)fmt_str.size()) * 2; /* Reserve two times as much as the length of the fmt_str */
  std::unique_ptr<char[]> formatted;
  va_list ap;
  while (1) {
    formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
    strcpy(&formatted[0], fmt_str.c_str());
    va_start(ap, fmt_str);
    final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
    va_end(ap);
    if (final_n < 0 || final_n >= n)
      n += abs(final_n - n + 1);
    else
      break;
  }
  return String(formatted.get());
}

//////////////////////////////////////////////
Domain* Domain::createDomain(DomainType type) {

  if (type == DomainType::HYPER_SLAB_DOMAIN_TYPE)
    return new HyperSlabDomain;

  if (type == DomainType::LIST_DOMAIN_TYPE)
    return new ListDomain;

  if (type == DomainType::MULTIAXIS_DOMAIN_TYPE)
    return new MultiAxisDomain;

  if (type == DomainType::SPATIAL_DOMAIN_TYPE)
    return new SpatialDomain;

  if (type == DomainType::RANGE_DOMAIN_TYPE)
    ThrowException("not implemented");

  return nullptr;
}

} //namespace
