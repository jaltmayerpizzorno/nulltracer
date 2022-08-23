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
    opts = ap.parse_args(sys.argv[1:minus_m+2])
    opts.script_or_module_args = sys.argv[minus_m+2:]
else:
    opts = ap.parse_args(sys.argv[1:])


def start_trace():
    from nulltracer import nulltracer
    tr = nulltracer.nulltracer()

    if opts.prefix:
        abs_prefix = opts.prefix.resolve()
        args = [str(abs_prefix)]
        if abs_prefix.is_relative_to(Path.cwd()):
            args.append(str(abs_prefix.relative_to(Path.cwd())))
        tr.set_prefix(*args)

    import atexit
    atexit.register(lambda: print("tracer called: ", tr.get_count()))

    import threading
    threading.settrace(tr)
    sys.settrace(tr)


if opts.script:
    code = compile(opts.script.read_text(), str(opts.script.resolve()), "exec")
    sys.argv = [str(opts.script.resolve()), *opts.script_or_module_args]
    start_trace()
    exec(code)
else:
    import runpy
    sys.argv = [*opts.module, *opts.script_or_module_args]
    start_trace()
    runpy.run_module(*opts.module, run_name='__main__', alter_sys=True)
