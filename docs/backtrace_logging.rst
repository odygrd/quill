.. title:: Backtrace Logging

Backtrace Logging
=================

Backtrace logging enables log messages to be stored in a ring buffer and either

- displayed later on demand or
- when a high severity log message is logged

Backtrace logging needs to be enabled first on the instance of :cpp:class:`quill::LoggerImpl`

.. doxygenfunction:: init_backtrace
.. doxygenfunction:: flush_backtrace

.. note:: Backtrace log messages store the original timestamp of the message. Since they are kept and flushed later the timestamp in the log file will be out of order.

.. note:: Backtrace log messages are still pushed to the SPSC queue from the frontend to the backend.

Store messages in the ring buffer and display them when ``LOG_ERROR`` is logged
-------------------------------------------------------------------------------

.. code:: cpp

    // a LOG_ERROR(...) or higher severity log message occurs via this logger.
    // Enable the backtrace with a max ring buffer size of 2 messages which will get flushed when
    // Backtrace has to be enabled only once in the beginning before calling LOG_BACKTRACE(...) for the first time.
    logger->init_backtrace(2, quill::LogLevel::Error);

    LOG_INFO(logger, "BEFORE backtrace Example {}", 1);

    LOG_BACKTRACE(logger, "Backtrace log {}", 1);
    LOG_BACKTRACE(logger, "Backtrace log {}", 2);
    LOG_BACKTRACE(logger, "Backtrace log {}", 3);
    LOG_BACKTRACE(logger, "Backtrace log {}", 4);

    // Backtrace is not flushed yet as we requested to flush on errors
    LOG_INFO(logger, "AFTER backtrace Example {}", 1);

    // log message with severity error - This will also flush_sink the backtrace which has 2 messages
    LOG_ERROR(logger, "An error has happened, Backtrace is also flushed.");

    // The backtrace is flushed again after LOG_ERROR but in this case it is empty
    LOG_ERROR(logger, "An second error has happened, but backtrace is now empty.");

    // Log more backtrace messages
    LOG_BACKTRACE(logger, "Another Backtrace log {}", 1);
    LOG_BACKTRACE(logger, "Another Backtrace log {}", 2);

    // Nothing is logged at the moment
    LOG_INFO(logger, "Another log info");

    // Still nothing logged - the error message is on a different logger object
    quill::LoggerImpl* logger_2 = quill::get_logger("example_1_1");

    LOG_CRITICAL(logger_2, "A critical error from different logger.");

    // The new backtrace is flushed again due to LOG_CRITICAL
    LOG_CRITICAL(logger, "A critical error from the logger we had a backtrace.");

Store messages in the ring buffer and display them on demand
------------------------------------------------------------

.. code:: cpp

    // Store maximum of two log messages. By default they will never be flushed since no LogLevel severity is specified
    logger->init_backtrace(2);

    LOG_INFO(logger, "BEFORE backtrace Example {}", 2);

    LOG_BACKTRACE(logger, "Backtrace log {}", 100);
    LOG_BACKTRACE(logger, "Backtrace log {}", 200);
    LOG_BACKTRACE(logger, "Backtrace log {}", 300);

    LOG_INFO(logger, "AFTER backtrace Example {}", 2);

    // an error has happened - flush_log_messages the backtrace manually
    logger->flush_backtrace();