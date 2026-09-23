import sys, pathlib

# Fallback for running pytest by hand: CTest sets PYTHONPATH to the fresh build,
# so this is appended (lowest priority) to avoid shadowing it with a stale module.
_bindings = pathlib.Path(__file__).parent.parent.parent / "build" / "bindings"
sys.path.append(str(_bindings))
