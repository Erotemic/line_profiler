"""
The line_profiler modula for doing line-by-line profiling of functions
"""
__submodules__ = [
    'line_profiler',
    'ipython_extension',
    'explicit_profiler',
]

__autogen__ = """
mkinit ./line_profiler/__init__.py --relative
mkinit ./line_profiler/__init__.py --relative -w
"""


# from .line_profiler import __version__

# NOTE: This needs to be in sync with ../kernprof.py and line_profiler.py
__version__ = '4.0.4'

from .line_profiler import (LineProfiler,
                            load_ipython_extension, load_stats, main,
                            show_func, show_text,)

from .explicit_profiler import (profile,)


__all__ = ['LineProfiler', 'line_profiler',
           'load_ipython_extension', 'load_stats', 'main', 'show_func',
           'show_text', '__version__', 'profile']
