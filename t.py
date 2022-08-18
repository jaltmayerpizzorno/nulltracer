import atexit
import sys
import threading
from pathlib import Path

sys.path += map(str, Path('.').glob('build/lib.*'))
import nulltracer

t = nulltracer.nulltracer()

def print_count():
    print("tracer called:", t.count())

atexit.register(print_count)

def enable_trace():
    nulltracer.settrace(t)
#    sys.settrace(t)
#    threading.settrace(t)
#    sys._getframe().f_trace = t

if sys.argv[1] == '-m':
    import runpy
    sys.argv = sys.argv[2:]
    enable_trace()
    runpy.run_module(sys.argv[0], run_name='__main__', alter_sys=True)
else:
    code = compile(Path(sys.argv[1]).read_text(), sys.argv[1], "exec")
    sys.argv = sys.argv[1:]
    enable_trace()
    exec(code)
