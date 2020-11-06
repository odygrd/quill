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

private:
  std::string name;
  std::string surname;
  uint32_t age;
};

/**
 * An other user defined type that is marked as safe to copy
 */
class User2
{
public:
  User2(std::string name, std::string surname, uint32_t age)
    : name(std::move(name)), surname(std::move(surname)), age(age){};

  friend std::ostream& operator<<(std::ostream& os, User2 const& obj)
  {
    os << "name : " << obj.name << ", surname: " << obj.surname << ", age: " << obj.age;
    return os;
  }

  /**
   * This class is tagged as safe to copy and it does not have to be formatted on the hot path
   * anymore
   */
  QUILL_COPY_LOGGABLE;

private:
  std::string name;
  std::string surname;
  uint32_t age;
};

int main()
{
  // Assuming QUILL_MODE_UNSAFE was NOT defined
  quill::start();

  User usr{"James", "Bond", 32};

  // The following fails to compile
  // LOG_INFO(quill::get_logger(), "The user is {}", usr);

  // The user has to explicitly format on the hot path, or instead tag the object (see User2)
  LOG_INFO(quill::get_logger(), "The user is {}", quill::utility::to_string(usr));

  // The following compiles and logs, because the object is tagged by the user as safe
  User2 tagged_user{"James", "Bond", 32};
  LOG_INFO(quill::get_logger(), "The user is {}", tagged_user);
}