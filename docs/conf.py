import os
import sys
import shutil
import subprocess
import time

# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'flow3r'
copyright = '2023'
author = 'ccc'

# The full version, including alpha/beta/rc tags
release = (
    subprocess.check_output(["git", "describe", "--tags", "--long", "--always"]).decode().strip()
)
release += "\n"
release += time.strftime("%F %R")
version = release

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx_rtd_theme',
    'sphinx.ext.autodoc',
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

pygments_style = 'sphinx'


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
html_logo = 'badge-logo-pink.png'
html_css_files = [
    'css/custom.css',
]
html_theme_options = {
    'logo_only': True,
    'style_nav_header_background': "#000",
    'style_external_links': True,
}

def setup(app):
    tmpdir = "_build/mypystubs"
    shutil.rmtree(tmpdir, ignore_errors=True)
    shutil.copytree("../python_payload/mypystubs", tmpdir)
    for filename in os.listdir(tmpdir):
        full_path = os.path.join(tmpdir, filename)
        os.rename(full_path, full_path.replace(".pyi", ".py"))

    sys.path.insert(0, os.path.abspath(tmpdir))
    sys.path.insert(1, os.path.abspath("../python_payload"))
