.. title:: Backend Options

Backend Options
===============

The backend options allow you to customize the behavior of the backend thread and are applied at runtime. To utilize these options, you need to create an object and pass it when starting the backend thread. Most options come with default values, but they can be tailored to meet the specific needs of your application. Refer to :cpp:struct:`BackendOptions` for detailed information.

For example, to pin the backend worker thread to a specific CPU, you can use the following code:

.. literalinclude:: examples/quill_docs_example_backend_options.cpp
   :language: cpp
   :linenos:
