#include "quill/Quill.h"
#include "quill/Utility.h"
#include <cstdint>
#include <string>

/**
 * A user defined type
 */
class User
{
public:
  User(std::string name, std::string surname, uint32_t age)
    : name(std::move(name)), surname(std::move(surname)), age(age){};

  friend std::ostream& operator<<(std::ostream& os, User const& obj)
  {
    os << "name : " << obj.name << ", surname: " << obj.surname << ", age: " << obj.age;
    return os;
  }

  /**
   * Deleted copy ctor
   */
  User(User const&) = delete;
  User& operator=(User const&) = delete;

private:
  std::string name;
  std::string surname;
  uint32_t age;
};

int main()
{
  quill::start();

  User usr{"James", "Bond", 32};

  LOG_INFO(quill::get_logger(), "The user is {}", quill::utility::to_string(usr));
}