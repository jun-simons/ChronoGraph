# setup.py
import os
import re
import sys
import platform
import subprocess
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

class CMakeExtension(Extension):
    """
    A setuptools.Extension for a CMake‐based build.  
    `name` must match the module name exposed via PYBIND11_MODULE in c++ code
    `sourcedir` points to the directory containing top-level CMakeLists.txt.
    """
    def __init__(self, name, sourcedir=""):
        super().__init__(name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)