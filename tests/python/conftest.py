import sys, pathlib

# adjust this if your build dir is elsewhere
_bindings = pathlib.Path(__file__).parent.parent / "build" / "bindings"
sys.path.insert(0, str(_bindings))
