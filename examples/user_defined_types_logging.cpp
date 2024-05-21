#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include <cstdint>
#include <sstream>
#include <string>
#include <utility>

/**
 * This example illustrates logging user-defined types.
 *
 * Starting from version 4.0.0, direct passing of user-defined and standard library types to the
 * 'LOG_' macros is no longer supported. Instead, these types must be converted to strings
 * before being passed to the logger.
 *
 * In this example, std::ostringstream is used for simplicity, but it's recommended to use modern
 * formatting libraries such as std::format or fmt::format.
 */

class User
{
public:
  User(std::string name, std::string surname, uint32_t age)
    : name(std::move(name)), surname(std::move(surname)), age(age){};

  friend std::ostream& operator<<(std::ostream& os, User const& obj)
  {
    os << "name: " << obj.name << ", surname: " << obj.surname << ", age: " << obj.age;
    return os;
  }

private:
  std::string name;
  std::string surname;
  uint32_t age;
};

// Helper to convert to string
template <typename T>
std::string to_string(T const& obj)
{
  std::ostringstream oss;
  oss << obj;
  return oss.str();
}

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  User user_1{"Super", "User", 1};

  LOG_INFO(logger, "User is [{}]", to_string(user_1));
}