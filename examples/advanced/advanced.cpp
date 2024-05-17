/**
 * This example showcases passing user-defined types as arguments to the logger, with their
 * formatting deferred asynchronously to the backend. It's particularly useful in scenarios where
 * string formatting latency is unacceptable and the code operates on the critical path.
 *
 * For a more straightforward approach, it's generally recommended to pass these types as strings,
 * formatting them in the frontend, as demonstrated in the 'user_defined_types_logging.cpp' example.
 */

// Include our wrapper lib for setup_quill
#include "quill_wrapper/quill_wrapper.h"

// Header required for quill::Frontend::get_logger
#include "quill/Frontend.h"

// We need only these two headers in order to log
#include "quill/LogMacros.h"
#include "quill/Logger.h"

// user defined type header
#include "user.h"

// user defined type codec header
#include "user_quill_codec.h"

// Required only when passing to logger std::vector<User> for offloading the formatting to the backend
#include "quill/std/Vector.h"

int main()
{
  setup_quill("recommended_usage.log");
  quill::Logger* logger = quill::Frontend::get_logger("root");

  User user;
  user.name = "Quill";
  user.surname = "Library";
  user.age = 4;
  user.favorite_colors[0] = "red";
  user.favorite_colors[1] = "green";
  user.favorite_colors[2] = "blue";

  LOG_INFO(logger, "The user is {}", user);

  std::vector<User> const users = {{"Alice", "Doe", 25, {"red", "green"}},
                                   {"Bob", "Smith", 30, {"blue", "yellow"}},
                                   {"Charlie", "Johnson", 35, {"green", "orange"}},
                                   {"David", "Brown", 40, {"red", "blue", "yellow"}}};

  LOG_INFO(logger, "The users are {}", users);
}