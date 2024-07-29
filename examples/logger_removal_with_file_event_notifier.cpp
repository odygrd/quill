#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <utility>

/**
 * This example demonstrates the creation and removal of a logger object for each instance of the Session class.
 * Each session instance logs to a new log file using a unique logger.
 * When the session ends, the logger is removed, and the associated file is closed.
 * Additionally, it showcases the usage of the FileEventNotifier, which provides notifications for file changes.
 */

class Session
{
public:
  explicit Session(std::string const& unique_name)
  {
    // Set up a FileEventNotifier so we are notified on file changes
    quill::FileEventNotifier file_notifier;

    file_notifier.before_open = [](quill::fs::path const& filename)
    { std::cout << "file_notifier - preparing to open file " << filename << std::endl; };

    file_notifier.after_open = [](quill::fs::path const& filename, FILE*)
    { std::cout << "file_notifier - opened file " << filename << std::endl; };

    file_notifier.before_close = [](quill::fs::path const& filename, FILE*)
    { std::cout << "file_notifier - preparing to close file " << filename << std::endl; };

    file_notifier.after_close = [](quill::fs::path const& filename)
    { std::cout << "file_notifier - closed file " << filename << std::endl; };

    // Create a new log file for this session
    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
      std::string{"session_"} + unique_name + ".log",
      []()
      {
        quill::FileSinkConfig cfg;
        cfg.set_open_mode('w');
        cfg.set_filename_append_option(quill::FilenameAppendOption::None);
        return cfg;
      }(),
      file_notifier);

    // Create a session specific logger for the current session
    _logger = quill::Frontend::create_or_get_logger(unique_name, std::move(file_sink));

    LOG_INFO(_logger, "Hello from session {}", unique_name);
  }

  ~Session()
  {
    // Remove the logger when the session is done. That will also remove the associated file_handler
    // and close the file as long as no other logger is using that file_handler
    quill::Frontend::remove_logger(_logger);
  }

private:
  quill::Logger* _logger{nullptr};
};

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  {
    Session session_1 = Session{"SessionA"};
    std::this_thread::sleep_for(std::chrono::seconds{3});

    Session session_2 = Session{"SessionB"};
    std::this_thread::sleep_for(std::chrono::seconds{3});
  }
  std::this_thread::sleep_for(std::chrono::seconds{3});

  {
    Session session_3 = Session{"SessionC"};
    std::this_thread::sleep_for(std::chrono::seconds{3});
  }
  std::this_thread::sleep_for(std::chrono::seconds{3});

  Session session_4 = Session{"SessionD"};
  std::this_thread::sleep_for(std::chrono::seconds{3});
}