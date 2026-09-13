.. title:: Installing

.. meta::
   :description: Install Quill for C++ with vcpkg, Conan, Homebrew, Conda, Meson, Bazel, CMake, or directly from source.

Installing
==========

For most users, a package manager is the simplest path. Build from source when you
want the latest branch, need local patches, or prefer to embed Quill directly in
your own project.

Package Managers
----------------

=================  ============================================
Package Manager    Installation Command
=================  ============================================
vcpkg              ``vcpkg install quill``
Conan              ``conan install quill``
Homebrew           ``brew install quill``
Meson WrapDB       ``meson wrap install quill``
Conda              ``conda install -c conda-forge quill``
Bzlmod             ``bazel_dep(name = "quill", version = "x.y.z")``
xmake              ``xrepo install quill``
=================  ============================================

CMake-Integration
-----------------

Building and Installing from Source
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use this path when you want a normal install that can later be found with ``find_package()``.

.. code:: bash

   git clone https://github.com/odygrd/quill.git
   cd quill
   mkdir cmake_build
   cd cmake_build
   cmake ..
   make install

.. note::

   To install into a custom directory, configure CMake with
   ``-DCMAKE_INSTALL_PREFIX=/quill/install-dir/``.

Then use the installed library from a CMake project, you can locate it directly with ``find_package()``

Embedded Project Directory Structure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   my_project/
   ├── CMakeLists.txt
   ├── main.cpp

Embedded Project CMakeLists.txt
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: cmake

   # Set only if needed - quill was installed under a custom non-standard directory
   set(CMAKE_PREFIX_PATH /test_quill/usr/local/)

   find_package(quill REQUIRED)

   # Linking your project against quill
   add_executable(example main.cpp)
   target_link_libraries(example PRIVATE quill::quill)

Embedding in Your Project
~~~~~~~~~~~~~~~~~~~~~~~~~

Use this path when you vendor Quill directly into your source tree and include it with
``add_subdirectory()``.

Directory Structure
^^^^^^^^^^^^^^^^^^^

::

   my_project/
   ├── quill/            (source folder)
   ├── CMakeLists.txt
   ├── main.cpp

CMakeLists.txt
^^^^^^^^^^^^^^

.. code:: cmake

   add_subdirectory(quill)
   add_executable(my_project main.cpp)
   target_link_libraries(my_project PRIVATE quill::quill)

Using the Bundled Formatter
---------------------------

Targets that only need formatting can use the bundled formatter without inheriting
Quill's logging compile options, definitions, or link dependencies.

With CMake, ``quill::fmtquill`` is available after ``find_package(quill REQUIRED)``
or ``add_subdirectory(quill)``:

.. code:: cmake

   find_package(quill REQUIRED)
   target_link_libraries(my_project PRIVATE quill::fmtquill)

With Meson, use the installed ``fmtquill`` pkg-config dependency or fall back to the
Quill subproject:

.. code:: meson

   fmtquill_dep = dependency('fmtquill', fallback : ['quill', 'fmtquill_dep'])
   executable('my_project', 'main.cpp', dependencies : [fmtquill_dep])

With Bazel, add ``@quill//:fmtquill`` to the target's ``deps``.

Include ``quill/bundled/fmt/format.h`` and use ``fmtquill::format`` or specialize
``fmtquill::formatter``. These header-only dependencies use Quill's bundled, patched fmt in the
``fmtquill`` namespace; its version follows the Quill release.

Using the C++20 Module
----------------------

Quill provides an experimental C++20 named module interface (``src/quill.cc``).
It is opt-in; normal header-only use continues to require only C++17.

With CMake 3.28 or newer and a compiler and generator supporting C++20 modules, set
``CMAKE_CXX_STANDARD=20`` and ``QUILL_BUILD_MODULE=ON`` when adding Quill through
``add_subdirectory`` or ``FetchContent``. Link against ``quill::quill_module``:

.. code:: cmake

   target_link_libraries(my_project PRIVATE quill::quill_module)

The module target is currently available from the source build, not from an installed
``find_package(quill)`` package.

With Bazel 9 or newer, the declared ``rules_cc`` 0.2.22 dependency, and a toolchain supporting
C++20 modules, add ``@quill//:quill_module`` to the target's ``deps``. Enable C++20 for the
whole build and pass ``--experimental_cpp_modules`` (for Clang, use
``--cxxopt=-std=c++20 --experimental_cpp_modules``):

.. code:: python

   cc_binary(
       name = "my_project",
       srcs = ["main.cpp"],
       features = ["cpp_modules", "prefer_pic_for_opt_binaries"],
       deps = ["@quill//:quill_module"],
   )

The selected Clang toolchain must also provide a matching ``clang-scan-deps`` beside the
compiler executable. On Ubuntu 24.04, install ``clang-18`` and ``clang-tools-18``, and set
``CC=/usr/lib/llvm-18/bin/clang`` and ``CXX=/usr/lib/llvm-18/bin/clang++``.

The module target is excluded from wildcard builds. Older Bazel versions can still use
``@quill//:quill`` and ``@quill//:fmtquill`` without enabling modules.

``prefer_pic_for_opt_binaries`` keeps module producers and consumers in PIC mode, avoiding
conflicting module metadata outputs in optimized builds with ``rules_cc`` 0.2.22.

Modules do not export macros, so include ``quill/LogMacros.h`` when using logging macros:

.. literalinclude:: snippets/quill_docs_module.cpp
   :language: cpp

Warning on Mixing Module and Headers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The transitive link closure of any Quill consumer should never include both the ``quill`` module
and header-based targets. The ``quill`` module attaches all Quill symbols to its module purview,
which is distinct from the unnamed global module purview that the header-based target attaches
its symbols to. This means that the two sets of symbols are completely distinct, and all runtime
singletons (ie. logger registries, worker threads) will be duplicated. An application should
either use the headers, or completely convert over to the module. Note that this does not apply
to ``quill/LogMacros.h``, which does not export any symbols.

Next Steps
----------

- :doc:`Quick Start <quick_start>` for a minimal working example.
- :doc:`Guides <guides>` for sinks, formatters, and advanced configuration.
- :doc:`Recipes <recipes>` for common tasks and code examples.
