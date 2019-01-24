#include <Visus/VisusXIdx.h>

#include <stdarg.h> 

namespace Visus {

bool XIdxModule::bAttached = false;

//////////////////////////////////////////////////////////////////
void XIdxModule::attach()
{
  if (bAttached)
    return;

  VisusInfo() << "Attaching XIdxModule...";

  bAttached = true;

  KernelModule::attach();

  VisusInfo() << "Attached XIdxModule";
}


//////////////////////////////////////////////
void XIdxModule::detach()
{
  if (!bAttached)
    return;

  VisusInfo() << "Detaching XIdxModule...";

  bAttached = false;

  KernelModule::detach();

  VisusInfo() << "Detached XIdxModule";
}

//////////////////////////////////////////////
String Group::FormatString(const String fmt_str, ...) 
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
SharedPtr<Domain> Domain::createDomain(DomainType type) {

  if (type == DomainType::HYPER_SLAB_DOMAIN_TYPE)
    return std::make_shared<HyperSlabDomain>();

  if (type == DomainType::LIST_DOMAIN_TYPE)
    return std::make_shared<ListDomain>();

  if (type == DomainType::MULTIAXIS_DOMAIN_TYPE)
    return std::make_shared<MultiAxisDomain>();

  if (type == DomainType::SPATIAL_DOMAIN_TYPE)
    return std::make_shared<SpatialDomain>();

  if (type == DomainType::RANGE_DOMAIN_TYPE)
    ThrowException("not implemented");

  return SharedPtr<Domain>();
}

} //namespace
