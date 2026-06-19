import os
from pathlib import Path

# Configuration for Sphinx
breathe_projects = {
    'Quill': os.path.join(os.path.abspath('.'), 'build', 'xml')
}
tagfile_path = Path(__file__).resolve().parent / "build" / "quill.tag"

# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Quill'
copyright = '2024, Odysseas Georgoudis'
author = 'Odysseas Georgoudis'
release = 'v12.1.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ["breathe", "sphinx.ext.autosectionlabel"]
extensions.append("sphinx_immaterial")
breathe_default_project = "Quill"
autosectionlabel_prefix_document = True

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']
html_extra_path = ['build/quill.tag'] if tagfile_path.exists() else []

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "sphinx_immaterial"
html_theme_options = {
    "site_url": "https://quill.readthedocs.io/",
    "repo_url": "https://github.com/odygrd/quill",
    "repo_name": "quill",
    "icon": {"repo": "fontawesome/brands/github"},
    "features": [
        "navigation.tabs",
        "navigation.tabs.sticky",
        "navigation.sections",
        "navigation.top",
        "toc.integrate",
        "toc.follow",
        "search.highlight",
        "search.share",
        "content.code.copy",
    ],
    "palette": [
        {
            "media": "(prefers-color-scheme: light)",
            "scheme": "default",
            "primary": "white",
            "accent": "blue",
            "toggle": {
                "icon": "material/weather-night",
                "name": "Switch to dark mode",
            },
        },
        {
            "media": "(prefers-color-scheme: dark)",
            "scheme": "slate",
            "primary": "black",
            "accent": "blue",
            "toggle": {
                "icon": "material/weather-sunny",
                "name": "Switch to light mode",
            },
        }
    ],
}
html_logo = "quill_logo.png"
html_static_path = ['_static']
html_css_files = ["theme_extra.css"]
html_title = f"Quill {release} - C++ Logging Library"
