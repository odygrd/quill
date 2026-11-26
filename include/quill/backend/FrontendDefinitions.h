/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/ThreadContextManager.h"

#ifndef QUILL_MODULE
  #include <memory>
#endif

/**
 * Definitions of frontend functions that quill/core declares, and the classes only they use. They
 * run rarely on the logging threads, and are compiled here with the backend instead of in every
 * file that logs. Reached through ThreadUtilities.h, which defines get_thread_name() that
 * they need: include that header, not this one alone
 */

QUILL_BEGIN_NAMESPACE

namespace detail
{
class ScopedThreadContext
{
public:
  /***/
  explicit ScopedThreadContext(FrontendOptionsValues const& frontend_options)
    : _thread_context(_create_thread_context(frontend_options))
  {
    // One thread context per thread: get_scoped_thread_context_impl() owns the only thread_local.
    ThreadContextManager::instance().register_thread_context(_thread_context);
  }

  /***/
  ScopedThreadContext(ScopedThreadContext const&) = delete;
  ScopedThreadContext& operator=(ScopedThreadContext const&) = delete;

  /***/
  ~ScopedThreadContext() noexcept
  {
    // This destructor will get called when the thread that created this wrapper stops
    // we will only invalidate the thread context
    // The backend thread will empty an invalidated ThreadContext and then remove it from
    // the ThreadContextManager
    // There is only exception for the thread who owns the ThreadContextManager the
    // main thread. The thread context of the main thread can get deleted before getting invalidated
    // Count retirement before publishing invalidity: the backend may remove it immediately.
    ThreadContextManager::instance().add_invalid_thread_context();
    _thread_context->mark_invalid();
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_HOT ThreadContext* get_thread_context() const noexcept
  {
    QUILL_ASSERT(_thread_context,
                 "_thread_context cannot be null in ScopedThreadContext::get_thread_context()");
    return _thread_context.get();
  }

private:
  /** @throws QuillError if `frontend_options` differ from the ones the application already uses */
  static std::shared_ptr<ThreadContext> _create_thread_context(FrontendOptionsValues const& frontend_options)
  {
    check_frontend_options(frontend_options);

    return std::make_shared<ThreadContext>(
      frontend_options.queue_type, frontend_options.initial_queue_capacity,
      frontend_options.unbounded_queue_max_capacity, frontend_options.huge_pages_policy);
  }

private:
  /**<
   * This could be unique_ptr but the thread context of main thread that owns
   * ThreadContextManager can be destructed last even after the logger singleton destruction
   * so we use shared_ptr */
  std::shared_ptr<ThreadContext> _thread_context;
};

/** Declared in quill/core/ThreadContextManager.h */
QUILL_NODISCARD QUILL_EXPORT QUILL_ATTRIBUTE_USED
#ifndef QUILL_MODULE
  inline
#endif
  ThreadContext*
  get_scoped_thread_context_impl(FrontendOptionsValues const& frontend_options)
{
  thread_local ScopedThreadContext scoped_thread_context{frontend_options};

  return scoped_thread_context.get_thread_context();
}
} // namespace detail

QUILL_END_NAMESPACE
