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
    
class CMakeBuild(build_ext):
    """
    Custom build_ext command that invokes CMake to configure & build the native extension.
    """
    def run(self):
        # 1) Verify CMake is installed
        try:
            subprocess.check_output(["cmake", "--version"])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))

        # 2) For each CMakeExtension, configure & build
        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        # Where to put the final shared‐object
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        cfg   = "Debug" if self.debug else "Release"

        # Make sure the extension directory exists
        os.makedirs(extdir, exist_ok=True)

        # Build directory for CMake out‐of‐source build
        build_temp = os.path.join(self.build_temp, ext.name)
        os.makedirs(build_temp, exist_ok=True)

        # Common CMake arguments
        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            f"-DCMAKE_BUILD_TYPE={cfg}"
        ]
