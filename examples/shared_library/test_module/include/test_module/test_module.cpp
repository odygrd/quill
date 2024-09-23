#include "test_module.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill_wrapper_shared/quill_wrapper_shared.h"
#include <iostream>

#include "quill/Backend.h"

QUILL_EXPORT extern quill::Logger* global_logger_a;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
    // Code to run when the DLL is loaded
    break;
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
    // Code to run when a thread is created or destroyed
    break;
  case DLL_PROCESS_DETACH:
    // Code to run when the DLL is unloaded
    global_logger_a->flush_log(0);
    break;
  }
  return TRUE; // Successfully processed
}

extern "C"
{
  void run_test_module()
  {
    std::cout << "Start module " << std::endl;
    LOG_INFO(global_logger_a, "TestModule is running");
    std::cout << "End module " << std::endl;
  }
}