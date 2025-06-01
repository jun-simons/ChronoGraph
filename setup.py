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

        # On Windows, specify generator and architecture if needed
        build_args = []
        if platform.system() == "Windows":
            # can override CMAKE_GENERATOR if desired (ninja, visual studio, etc)
            cmake_args += [f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{cfg.upper()}={extdir}"]
            # Example: if you want 64‐bit MSVC: cmake_args += ['-A', 'x64']
            build_args += ["--config", cfg]
        else:
            # On Unix‐like machines, prefer parallel jobs if possible
            build_args += ["--", f"-j{os.cpu_count()}"]

        # 1) Run CMake configure
        subprocess.check_call(
            ["cmake", ext.sourcedir] + cmake_args,
            cwd=build_temp
        )
        # 2) Run CMake build
        subprocess.check_call(
            ["cmake", "--build", "."] + build_args,
            cwd=build_temp
        )

setup(
    name="chronograph",
    version=version,
    author="Your Name",
    author_email="you@example.com",
    description="ChronoGraph: a temporal & versioned C++ graph library with Git‐style repo support",
    long_description="",
    license="MIT",
    classifiers=[
        "Programming Language :: C++",
        "Programming Language :: Python",
        "License :: OSI Approved :: MIT License"
    ],
    # We do not have any pure‐Python modules; the extension itself is 'chronograph'
    ext_modules=[CMakeExtension("chronograph", sourcedir=".")],
    cmdclass={"build_ext": CMakeBuild},
    zip_safe=False,
    # If you ever add pure‐Python subpackages, list them here, or use packages=find_packages()
    packages=[],
    install_requires=[
        # any pure‐Python runtime dependencies, e.g. "graphviz>=0.20.1"
    ],
    python_requires=">=3.8",
)