#include "quill_wrapper.h"

QuillWrapper& QuillWrapper::instance()
{
  static QuillWrapper instance {};
  return instance;
}

void QuillWrapper::setup_log()
{
  quill::Config cfg;
  cfg.enable_console_colours = true;
  cfg.backend_thread_yield = true;

  quill::configure(cfg);
  quill::start();
}
