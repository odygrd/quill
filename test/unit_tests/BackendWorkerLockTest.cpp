#include "doctest/doctest.h"

#include "quill/backend/BackendWorkerLock.h"
#include "quill/core/Filesystem.h"
#include "quill/core/QuillError.h"

#include <string>

TEST_SUITE_BEGIN("BackendWorkerLock");

using namespace quill;
using namespace quill::detail;

/**
 * Verifies that creating two BackendWorkerLock instances with the same pid
 * detects the duplicate and throws.
 */
TEST_CASE("duplicate_lock_throws")
{
#if !defined(QUILL_NO_EXCEPTIONS)
  std::string const pid = "test_" + std::to_string(99999);

  // First lock should succeed
  BackendWorkerLock lock1{pid};

  // Second lock with same pid should throw - duplicate backend detected
  #if defined(__ANDROID__)
  // Android: detection is disabled, no throw expected
  BackendWorkerLock lock2{pid};
  #else
  REQUIRE_THROWS_AS(BackendWorkerLock{pid}, QuillError);
  #endif
#endif
}

/**
 * Verifies that after a lock is destroyed, a new lock with the same pid succeeds.
 */
TEST_CASE("lock_released_after_destruction")
{
  std::string const pid = "test_" + std::to_string(99998);

  {
    BackendWorkerLock lock1{pid};
    // lock1 goes out of scope here
  }

  // Should succeed - previous lock was released
  REQUIRE_NOTHROW(BackendWorkerLock{pid});
}

/**
 * Verifies that the lock file is created while the lock is held
 * and cleaned up after the lock is destroyed.
 */
TEST_CASE("lock_file_cleaned_up_on_destruction")
{
#if !defined(_WIN32) && !defined(__ANDROID__)
  std::string const pid = "test_" + std::to_string(99997);
  std::string const path = "/tmp/QuillBackendLock" + pid;

  {
    BackendWorkerLock lock1{pid};

    // Lock file should exist while the lock is held
    REQUIRE(fs::exists(path));
  }

  // Lock file should be cleaned up after destruction
  REQUIRE_FALSE(fs::exists(path));
#endif
}

TEST_SUITE_END();
