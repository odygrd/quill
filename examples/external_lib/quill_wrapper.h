#pragma once

#include "quill/Quill.h"

class QuillWrapper
{
public:
  static QuillWrapper& instance();

  void setup_log();

  static quill::Logger* get_logger()
  {
    return quill::get_root_logger();
  }

private:
  QuillWrapper()  = default;
};
