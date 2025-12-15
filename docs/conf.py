import subprocess, os


def configureDoxyfile(input_dir, output_dir):
    doxyfile_in_path = os.path.join(os.path.abspath('.'), 'Doxyfile.in')
    with open(doxyfile_in_path, 'r') as file:
        filedata = file.read()

    filedata = filedata.replace('@DOXYGEN_INPUT_DIR@', input_dir)
    filedata = filedata.replace('@DOXYGEN_OUTPUT_DIR@', output_dir)

    with open('Doxyfile', 'w') as file:
        file.write(filedata)


# Set paths for local build
input_dir = '../include/quill'
output_dir = 'build'

configureDoxyfile(input_dir, output_dir)
subprocess.call('doxygen', shell=True)

# Configuration for Sphinx
breathe_projects = {
    'Quill': os.path.join(os.path.abspath('.'), 'build', 'xml')
}

# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Quill'
copyright = '2024, Odysseas Georgoudis'
author = 'Odysseas Georgoudis'
release = 'v11.1.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ["breathe", "sphinx.ext.autosectionlabel"]
breathe_default_project = "Quill"

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'furo'
html_static_path = ['_static']
html_title = f"Quill {release} - C++ Logging Library"
