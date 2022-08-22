import argparse
from pathlib import Path
import sys

ap = argparse.ArgumentParser(prog='nulltracer')
ap.add_argument('--prefix', type=Path, help="only trace files starting with a prefix")

g = ap.add_mutually_exclusive_group(required=True)
g.add_argument('-m', dest='module', nargs=1, help="run given module as __main__")
g.add_argument('script', nargs='?', type=Path, help="the script to run")
ap.add_argument('script_or_module_args', nargs=argparse.REMAINDER)

if '-m' in sys.argv: # work around exclusive group not handled properly
    minus_m = sys.argv.index('-m')
    args = ap.parse_args(sys.argv[1:minus_m+2])
    args.script_or_module_args = sys.argv[minus_m+2:]
else:
    args = ap.parse_args(sys.argv[1:])


def start_trace():
    import atexit
    from nulltracer import nulltracer
    if args.prefix:
        nulltracer.set_prefix(str(args.prefix))

    atexit.register(lambda: print("tracer called: ", nulltracer.get_count()))


if args.script:
    code = compile(Path(args.script).read_text(), args.script, "exec")
    sys.argv = [args.script, *args.script_or_module_args]
    start_trace()
    exec(code)
else:
    import runpy
    sys.argv = [*args.module, *args.script_or_module_args]
    runpy.run_module(*args.module, run_name='__main__', alter_sys=True)
