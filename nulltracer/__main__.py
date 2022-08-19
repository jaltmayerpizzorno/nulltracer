from pathlib import Path
import sys

def start_trace():
    import atexit
    from nulltracer import nulltracer

    atexit.register(lambda: print("tracer called: ", nulltracer.get_count()))

if len(sys.argv) > 1:
    if sys.argv[1] == '-m':
        import runpy
        sys.argv = sys.argv[2:]
        start_trace()
        runpy.run_module(sys.argv[0], run_name='__main__', alter_sys=True)
    else:
        code = compile(Path(sys.argv[1]).read_text(), sys.argv[1], "exec")
        sys.argv = sys.argv[1:]
        start_trace()
        exec(code)
