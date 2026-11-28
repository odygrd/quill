import os
from pathlib import Path
from urllib.parse import quote

from docutils import nodes

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
release = 'v14.0.0'

# Read the Docs provides the canonical URL for the version being built. Use the
# public documentation URL when building locally.
canonical_url = os.environ.get(
    "READTHEDOCS_CANONICAL_URL", "https://quillcpp.readthedocs.io/en/latest/"
).rstrip("/") + "/"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ["breathe", "sphinx.ext.autosectionlabel"]
extensions.append("sphinx_immaterial")
breathe_default_project = "Quill"
autosectionlabel_prefix_document = True

# Text in single backticks is code, as it is in double backticks; without a default role it would be set in italics.
default_role = "literal"

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']
html_extra_path = ['build/quill.tag'] if tagfile_path.exists() else []

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "sphinx_immaterial"
html_baseurl = canonical_url
html_use_index = False
html_theme_options = {
    "site_url": canonical_url,
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
        "content.tabs.link",
    ],
    "palette": [
        {
            "media": "(prefers-color-scheme: light)",
            "scheme": "default",
            "primary": "white",
            "accent": "teal",
            "toggle": {
                "icon": "material/weather-night",
                "name": "Switch to dark mode",
            },
        },
        {
            "media": "(prefers-color-scheme: dark)",
            "scheme": "slate",
            "primary": "black",
            "accent": "teal",
            "toggle": {
                "icon": "material/weather-sunny",
                "name": "Switch to light mode",
            },
        }
    ],
}
# The sidebar of the API page lists every class and member. Their parameters, return values and exceptions stay
# out of it: they would make the list several times longer.
object_description_options = [
    ("cpp:.*", {"include_fields_in_toc": False}),
    ("cpp:.*Param", {"include_in_toc": False}),
]
html_logo = "quill_logo.png"
html_favicon = "quill_logo.png"
html_static_path = ['_static']
html_css_files = ["theme_extra.css", "benchmark_charts.css"]
html_js_files = ["benchmark_charts.js"]
html_title = f"Quill {release} - C++ Logging Library"


def _add_canonical_url(_app, _pagename, _templatename, context, _doctree):
    """Expose Sphinx's canonical page URL to the Immaterial theme."""
    page = context.get("page")
    page_url = context.get("pageurl")
    if page is not None and page_url:
        page["canonical_url"] = page_url


def _link_home_example(app, docname, source):
    if docname != "index":
        return

    snippet = (Path(app.srcdir) / "snippets/quill_docs_home.cpp").read_text(encoding="utf-8")
    # Compiler Explorer's /e route reads a Rison-encoded layout. Inside a
    # quoted Rison string, ! escapes itself and the closing quote.
    code = snippet.replace("!", "!!").replace("'", "!'")
    editor = (
        "(type:component,componentName:codeEditor,height:65,"
        f"componentState:(id:1,lang:'c++',source:'{code}'))"
    )
    executor = (
        "(type:component,componentName:executor,height:35,"
        "componentState:(source:1,lang:'c++',compiler:g142,options:'-O2 -std=c++20',"
        "libs:!((name:quill,ver:'1310')),wrap:!t))"
    )
    layout = f"version:4,content:!((type:column,content:!({editor},{executor})))"
    url = "https://godbolt.org/#" + quote(layout, safe="!(),:'")
    source[0] = source[0].replace("{{ compiler_explorer_url }}", url)


def _trim_api_toc(_app, doctree):
    """Keep Breathe's group titles, such as "Public Members", out of the sidebar.

    Breathe gives every such title the same id, so their sidebar entries all led to the first one.
    """
    for rubric in doctree.findall(nodes.rubric):
        if "breathe-sectiondef-title" in rubric["classes"]:
            rubric["ids"] = []


def _add_home_assets(app, pagename, _templatename, _context, _doctree):
    if pagename == "index":
        app.add_css_file("home.css", priority=900)
        app.add_js_file("home.js", loading_method="defer")


from pygments.lexers.c_cpp import CppLexer
from pygments.token import Name, Operator, Punctuation


class _QuillCppLexer(CppLexer):
    """
    Pygments' C++ lexer gives every identifier the same Name token, so in a code block only strings,
    keywords and numbers stand out. This one classifies identifiers by what follows them: a namespace
    before ``::``, a function before ``(`` or before a template argument list and ``(``, and a type when
    it starts with a capital letter or is followed by a template argument list and a name.
    """

    def get_tokens_unprocessed(self, text, stack=("root",)):
        tokens = list(super().get_tokens_unprocessed(text, stack))

        def next_significant(i):
            i += 1
            while i < len(tokens) and not tokens[i][2].strip():
                i += 1
            return i

        def closes_template(i):
            # The index after the ``>`` that closes the template argument list opened at ``i``
            depth = 0
            while i < len(tokens):
                value = tokens[i][2]
                if value == "<":
                    depth += 1
                elif value == ">":
                    depth -= 1
                    if depth == 0:
                        return next_significant(i)
                elif value in (";", "{", "}", "=", "(", ")") and depth:
                    return None
                i += 1
            return None

        for index, (position, token, value) in enumerate(tokens):
            if token is Name:
                after = next_significant(index)
                following = tokens[after][2] if after < len(tokens) else ""
                if following == ":" and after + 1 < len(tokens) and tokens[after + 1][2] == ":":
                    # A capitalised name before ``::`` is a class, such as ``Frontend::``
                    token = Name.Class if value[:1].isupper() else Name.Namespace
                elif following == "(":
                    token = Name.Function
                elif following == "<":
                    end = closes_template(after)
                    if end is not None and end < len(tokens) and tokens[end][2] == "(" and not value[:1].isupper():
                        token = Name.Function
                    elif value[:1].isupper() or (end is not None and end < len(tokens) and tokens[end][1] is Name):
                        # A template type, such as ``std::vector<int> v``
                        token = Name.Class
                elif value[:1].isupper() and not value.isupper():
                    token = Name.Class
            yield position, token, value


def setup(app):
    app.add_lexer("cpp", _QuillCppLexer)
    app.add_lexer("c++", _QuillCppLexer)
    app.connect("source-read", _link_home_example)
    # Before the theme collects the table of contents of each page.
    app.connect("doctree-read", _trim_api_toc, priority=400)
    app.connect("html-page-context", _add_home_assets)
    # Run after the theme creates its ``page`` context object.
    app.connect("html-page-context", _add_canonical_url, priority=999)
