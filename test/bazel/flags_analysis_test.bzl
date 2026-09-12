"""
Analysis tests verifying preprocessor configuration flags propagate to Quill's defines.
"""

load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

def _flags_default_test_impl(ctx):
    env = analysistest.begin(ctx)
    target = analysistest.target_under_test(env)
    defines = target[CcInfo].compilation_context.defines.to_list()
    asserts.equals(env, [], defines)
    return analysistest.end(env)

flags_default_test = analysistest.make(
    _flags_default_test_impl,
)

def _flags_configured_test_impl(ctx):
    env = analysistest.begin(ctx)
    target = analysistest.target_under_test(env)
    defines = target[CcInfo].compilation_context.defines.to_list()

    expected_defines = [
        "QUILL_NO_EXCEPTIONS",
        "QUILL_NO_THREAD_NAME_SUPPORT",
        "QUILL_USE_SEQUENTIAL_THREAD_ID",
        "QUILL_ENABLE_ASSERTIONS",
        "QUILL_DISABLE_NON_PREFIXED_MACROS",
        "QUILL_DISABLE_FUNCTION_NAME",
        "QUILL_DETAILED_FUNCTION_NAME",
        "QUILL_DISABLE_FILE_INFO",
        "QUILL_ENABLE_IMMEDIATE_FLUSH=0",
        "QUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING",
    ]

    for d in expected_defines:
        asserts.true(env, d in defines, "Expected %s in defines %s" % (d, defines))

    return analysistest.end(env)

flags_configured_test = analysistest.make(
    _flags_configured_test_impl,
    config_settings = {
        str(Label("//:no_exceptions")): True,
        str(Label("//:no_thread_name_support")): True,
        str(Label("//:use_sequential_thread_id")): True,
        str(Label("//:enable_assertions")): True,
        str(Label("//:disable_non_prefixed_macros")): True,
        str(Label("//:disable_function_name")): True,
        str(Label("//:detailed_function_name")): True,
        str(Label("//:disable_file_info")): True,
        str(Label("//:enable_immediate_flush")): False,
        str(Label("//:active_log_level")): "WARNING",
    },
)

def flags_analysis_test_suite(name):
    flags_default_test(
        name = name + "_default",
        target_under_test = "//:quill",
        size = "small",
    )

    flags_configured_test(
        name = name + "_all_configured",
        target_under_test = "//:quill",
        size = "small",
    )

    native.test_suite(
        name = name,
        tests = [
            name + "_default",
            name + "_all_configured",
        ],
    )
