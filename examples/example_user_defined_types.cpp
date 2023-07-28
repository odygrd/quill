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

// The preprocessor check is only to support backwards compatibility with older fmt versions. You do not need it
#if QUILL_FMT_VERSION >= 90000
template <>
struct fmtquill::formatter<User> : ostream_formatter
{
};
#endif

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

// The preprocessor check is only to support backwards compatibility with older fmt versions. You do not need it
#if QUILL_FMT_VERSION >= 90000
template <>
struct fmtquill::formatter<User2> : ostream_formatter
{
};
#endif

/**
 * An other user defined type that is registered as safe to copy via copy_logable
 */
class User3
{
public:
  User3(std::string name, std::string surname, uint32_t age)
    : name(std::move(name)), surname(std::move(surname)), age(age){};

  friend std::ostream& operator<<(std::ostream& os, User3 const& obj)
  {
    os << "name : " << obj.name << ", surname: " << obj.surname << ", age: " << obj.age;
    return os;
  }

private:
  std::string name;
  std::string surname;
  uint32_t age;
};

// The preprocessor check is only to support backwards compatibility with older fmt versions. You do not need it
#if QUILL_FMT_VERSION >= 90000
template <>
struct fmtquill::formatter<User3> : ostream_formatter
{
};
#endif

class User4
{
public:
  User4(std::string name, std::string surname, uint32_t age)
    : name(std::move(name)), surname(std::move(surname)), age(age){};

  friend struct fmtquill::formatter<User4>;

private:
  std::string name;
  std::string surname;
  uint32_t age;
};

template <>
struct fmtquill::formatter<User4>
{
  template <typename FormatContext>
  auto parse(FormatContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(User4 const& user, FormatContext& ctx)
  {
    return fmtquill::format_to(ctx.out(), "User: {} {}, Age: {}", user.name, user.surname, user.age);
  }
};

template <>
struct quill::copy_loggable<User4> : std::true_type
{
};

/**
 * Specialise copy_loggable to register User3 object as safe to copy.
 */
namespace quill
{
template <>
struct copy_loggable<User3> : std::true_type
{
};
} // namespace quill

int main()
{
  // Assuming QUILL_MODE_UNSAFE was NOT defined
  quill::start();

  User usr{"James", "Bond", 32};

  // The following fails to compile
  // LOG_INFO(quill::get_logger(), "The user is {}", usr);

  // The user has to explicitly format on the hot path
  LOG_INFO(quill::get_logger(), "The user is {}", quill::utility::to_string(usr));

  // The following compiles and logs, because the object is tagged by the user as safe
  User2 tagged_user{"James", "Bond", 32};
  LOG_INFO(quill::get_logger(), "The user is {}", tagged_user);

  // The following compiles and logs, because the object is registered by the user as safe
  User3 registred_user{"James", "Bond", 42};
  LOG_INFO(quill::get_logger(), "The user is {}", registred_user);

  User4 user{"Super", "User", 42};
  LOG_INFO(quill::get_logger(), "The user is {}", user);
}