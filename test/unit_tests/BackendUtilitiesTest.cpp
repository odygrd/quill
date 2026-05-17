#include "doctest/doctest.h"

#include "quill/backend/BackendUtilities.h"
#include "quill/backend/BackendWorker.h"
#include "quill/backend/BackendWorkerLock.h"
#include "quill/core/Filesystem.h"
#include "quill/core/SinkManager.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>

TEST_SUITE_BEGIN("BackendUtilities");

using namespace quill;
using namespace quill::detail;

TEST_CASE("normalize_file_sink_path_preserves_special_paths")
{
  REQUIRE_EQ(normalize_file_sink_path(fs::path{"stdout"}).string(), "stdout");
  REQUIRE_EQ(normalize_file_sink_path(fs::path{"stderr"}).string(), "stderr");
  REQUIRE_EQ(normalize_file_sink_path(fs::path{"/dev/null"}).string(), "/dev/null");
}

TEST_CASE("normalize_file_sink_path_missing_directory_without_creation_throws")
{
  fs::path const missing_dir = "backend_utilities_missing_dir";
  std::error_code ec;
  fs::remove_all(missing_dir, ec);

#if !defined(QUILL_NO_EXCEPTIONS)
  auto const normalize_missing_path = [&]()
  { return normalize_file_sink_path(missing_dir / "missing.log", false); };
  REQUIRE_THROWS_AS(normalize_missing_path(), QuillError);
#endif
}

TEST_CASE("sink_manager_get_sink_missing_path_throws")
{
  fs::path const missing_dir = "sink_manager_missing_dir";
  std::error_code ec;
  fs::remove_all(missing_dir, ec);

#if !defined(QUILL_NO_EXCEPTIONS)
  auto const get_missing_sink = [&]()
  { return SinkManager::instance().get_sink((missing_dir / "missing.log").string()); };
  REQUIRE_THROWS_AS(get_missing_sink(), QuillError);
#endif
}

TEST_CASE("backend_worker_lock_rejects_duplicate_while_original_is_alive")
{
  std::string const pid = std::to_string(get_process_id()) + std::string{"1"};
  BackendWorkerLock first_lock{pid};

#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS_AS(BackendWorkerLock{pid}, QuillError);
#endif
}

TEST_CASE("backend_worker_lock_no_throw")
{
  std::string const pid = std::to_string(get_process_id()) + std::string{"2"};
  {
    BackendWorkerLock first_lock{pid};
  }
  REQUIRE_NOTHROW(BackendWorkerLock{pid});
}

TEST_CASE("lock_file_cleaned_up_destruction")
{
#if !defined(_WIN32) && !defined(__ANDROID__)
  std::string const pid = "test_99997";
  std::string const path = "/tmp/QuillBackendLock" + pid;

  {
    BackendWorkerLock first_lock{pid};
    REQUIRE(fs::exists(path));
  }

  REQUIRE_FALSE(fs::exists(path));
#else
  return;
#endif
}

TEST_CASE("backend_worker_run_rejects_duplicate_while_original_is_alive")
{
  BackendOptions options;
  options.error_notifier = [](std::string const&) {};

  detail::BackendWorker first_backend_worker;
  first_backend_worker.run(options);

  detail::BackendWorker second_backend_worker;

#if !defined(QUILL_NO_EXCEPTIONS)
  auto const start_second_backend_worker = [&]() { second_backend_worker.run(options); };
  REQUIRE_THROWS_AS(start_second_backend_worker(), QuillError);
#endif

  first_backend_worker.stop();
  second_backend_worker.stop();
}

TEST_CASE("backend_worker_lock_can_be_reacquired_after_release")
{
  std::string const pid = std::to_string(get_process_id()) + std::string{"3"};

  {
    BackendWorkerLock first_lock{pid};
  }

  REQUIRE_NOTHROW(BackendWorkerLock{pid});
}

TEST_CASE("get_process_id_matches_current_process")
{
#if defined(_WIN32)
  REQUIRE_EQ(get_process_id(), static_cast<uint32_t>(::GetCurrentProcessId()));
#elif !defined(__CYGWIN__)
  REQUIRE_EQ(get_process_id(), static_cast<uint32_t>(::getpid()));
#else
  REQUIRE_EQ(get_process_id(), 0u);
#endif
}

TEST_CASE("set_cpu_affinity_accepts_current_cpu_and_rejects_invalid_cpu")
{
#if defined(__linux__) && !defined(__ANDROID__)
  REQUIRE_NOTHROW(set_cpu_affinity({}));

  cpu_set_t original_cpuset;
  REQUIRE_EQ(::sched_getaffinity(0, sizeof(original_cpuset), &original_cpuset), 0);

  int selected_cpu = -1;
  for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu)
  {
    if (CPU_ISSET(cpu, &original_cpuset))
    {
      selected_cpu = cpu;
      break;
    }
  }

  REQUIRE_NE(selected_cpu, -1);

  REQUIRE_NOTHROW(set_cpu_affinity({static_cast<uint16_t>(selected_cpu)}));
  REQUIRE_EQ(::sched_setaffinity(0, sizeof(original_cpuset), &original_cpuset), 0);

  #if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS_AS(set_cpu_affinity({static_cast<uint16_t>(CPU_SETSIZE)}), QuillError);
  #endif
#else
  return;
#endif
}

TEST_CASE("set_thread_name_validates_length")
{
#if defined(__linux__) && !defined(__ANDROID__)
  set_thread_name("quill-test");

  char current_name[16]{};
  REQUIRE_EQ(::pthread_getname_np(::pthread_self(), current_name, sizeof(current_name)), 0);
  REQUIRE_EQ(std::string{current_name}, "quill-test");

  set_thread_name("quill-thread-name-too-long");
  REQUIRE_EQ(::pthread_getname_np(::pthread_self(), current_name, sizeof(current_name)), 0);
  REQUIRE_EQ(std::string{current_name}, "quill-thread-na");
#else
  return;
#endif
}

TEST_SUITE_END();
