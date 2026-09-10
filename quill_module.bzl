"""Keep the experimental module optional on older Bazel versions."""

load("@bazel_features//:features.bzl", "bazel_features")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

def _unsupported_module_impl(ctx):
    fail("{} requires Bazel 9 or newer for C++20 modules; use //:quill for header-only logging.".format(ctx.label))

_unsupported_module = rule(implementation = _unsupported_module_impl)

def quill_module(name, **kwargs):
    """Declare the module without passing unsupported attributes to older Bazel.

    Args:
        name: Name of the module target.
        **kwargs: Attributes forwarded to cc_library when modules are supported.
    """

    # Bazel 7 rejects module_interfaces; Bazel 8 accepts it but does not compile it.
    # Modules require the Starlark C++ implementation used by rules_cc on Bazel 9+.
    if not bazel_features.cc.cc_common_is_in_rules_cc:
        _unsupported_module(name = name, tags = kwargs.get("tags", []))
        return

    cc_library(name = name, **kwargs)
