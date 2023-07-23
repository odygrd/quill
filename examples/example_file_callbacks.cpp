#include "quill/Quill.h"
#include <iostream>

class Session
{
public:
  explicit Session(std::string unique_name)
  {
    // Set up a FileEventNotifier so we are notified on file changes (Optional)
    quill::FileEventNotifier fen;

    fen.before_open = [](quill::fs::path const& filename)
    { std::cout << "before opening " << filename << std::endl; };

    fen.after_open = [](quill::fs::path const& filename, FILE* f)
    { std::cout << "after opening " << filename << std::endl; };

    fen.before_close = [](quill::fs::path const& filename, FILE* f)
    { std::cout << "before closing " << filename << std::endl; };

    fen.after_close = [](quill::fs::path const& filename)
    { std::cout << "after closing " << filename << std::endl; };

    // Create a new log file for this session
    std::shared_ptr<quill::Handler> file_handler = quill::file_handler(
      unique_name + ".log",
      []()
      {
        quill::FileHandlerConfig cfg;
        cfg.set_open_mode('a');
        return cfg;
      }(),
      std::move(fen));

    // Create a logger for the current session
    _logger = quill::create_logger(unique_name, std::move(file_handler));

    LOG_INFO(_logger, "Hello from session {}", unique_name);
  }

  ~Session()
  {
    // Remove the logger when the session is done. That will also remove the associated file_handler
    // and close the file as long as no other logger is using that file_handler
    quill::remove_logger(_logger);
  }

private:
  quill::Logger* _logger{nullptr};
};

int main()
{
  quill::start();

  {
    Session session_1 = Session{"SessionA"};
    std::this_thread::sleep_for(std::chrono::seconds{1});
    Session session_2 = Session{"SessionB"};
    std::this_thread::sleep_for(std::chrono::seconds{1});
  }

  Session session_3 = Session{"SessionC"};
  std::this_thread::sleep_for(std::chrono::seconds{1});
  // SessionA will write to the previous file because we used "a" above
  Session session_4 = Session{"SessionA"};

  std::this_thread::sleep_for(std::chrono::seconds{1});
  quill::flush();
}