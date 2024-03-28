#include "quill/Frontend.h"

#include "quill/core/LoggerManager.h"

#include "quill/core/handlers/FileHandler.h"

#include "quill/Backend.h"

#include <chrono>
#include <thread>

/**
 * Trivial logging example
 */

int main()
{
    quill::Config cfg;
    quill::start_backend_thread(cfg, false, {});

  // Start the logging backend thread
    auto file_handler =
            quill::create_or_get_sink<quill::FileHandler>(
                    "my_handler", "example_trivial.log",
                    []() {
                        quill::FileHandlerConfig cfg;
                        cfg.set_open_mode('w');
                        cfg.set_append_to_filename(quill::FilenameAppend::StartDateTime);
                        cfg.set_pattern("%(time) [%(process_id)] [%(thread_id)] %(logger) - %(message)",
                                        "%D %H:%M:%S.%Qms %z");
                        cfg.set_timezone(quill::Timezone::GmtTime);
                        return cfg;
                    }(),
                    quill::FileEventNotifier{});

    quill::Logger *logger = quill::create_or_get_logger(
            "root", std::move(file_handler), quill::ClockSourceType::System, nullptr);

    // Change the LogLevel to print everything
    logger->set_log_level(quill::LogLevel::TraceL3);

    LOG_TRACE_L3(logger, "This is a log trace l3 example {}", 1);
    LOG_TRACE_L2(logger, "This is a log trace l2 example {} {}", 2, 2.3);
    LOG_TRACE_L1(logger, "This is a log trace l1 {} example", "string");
    LOG_DEBUG(logger, "This is a log debug example {}", 4);
    LOG_INFO(logger, "This is a log info example {}", 5);
    LOG_WARNING(logger, "This is a log warning example {}", 6);
    LOG_ERROR(logger, "This is a log error example {}", 7);
    LOG_CRITICAL(logger, "This is a log critical example {}", 8);

    // logger->flush();
}
