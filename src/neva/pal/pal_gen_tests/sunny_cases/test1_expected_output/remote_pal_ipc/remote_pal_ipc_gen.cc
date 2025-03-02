// This file is generated by PAL generator, do not modify.
// To make changes, modify the source file:
// test1.json

#include "remote_pal_ipc/remote_pal_ipc.h"

#include <mutex>

#include "remote_pal_ipc/system_info/system_info_remote_pal_interface_ipc.h"

namespace pal {

std::unique_ptr<SystemInfoInterface> system_info_interface_instance;
std::once_flag system_info_interface_instance_flag;

SystemInfoInterface* RemotePalIPC::GetSystemInfoInterface() {
  std::call_once(system_info_interface_instance_flag, []() {
    system_info_interface_instance.reset(new SystemInfoRemotePalInterfaceIPC());
  });
  return system_info_interface_instance.get();
}

}  // namespace pal
