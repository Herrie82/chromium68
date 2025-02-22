// This file is generated by PAL generator, do not modify.
// To make changes, modify the source file:
// test1.json

#ifndef PAL_PUBLIC_INTERFACES_SYSTEM_INFO_INTERFACE_H_
#define PAL_PUBLIC_INTERFACES_SYSTEM_INFO_INTERFACE_H_

#include "base/callback.h"
#include "pal/ipc/pal_export.h"

namespace pal {

class PAL_EXPORT SystemInfoInterface {
 public:
  virtual ~SystemInfoInterface(){};

  // Interface for getting system info.

  virtual std::string GetSSLCertPath() = 0;
};

}  // namespace pal

#endif  // PAL_PUBLIC_INTERFACES_SYSTEM_INFO_INTERFACE_H_