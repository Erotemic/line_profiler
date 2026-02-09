# line_profiler/_line_profiler.pyi
from typing import Any, Dict, List, Mapping, Optional, Tuple, Union

# Match the public data shape used by the Python layer's LineStats utilities.
# The Python wrapper treats timings like:
#   { (func_name, start_lineno, filename): [(lineno, nhits, time), ...], ... }
_TimingKey = Tuple[str, int, str]
_TimingEntry = Tuple[int, int, int]  # (lineno, nhits, time_in_internal_units)
_Timings = Dict[_TimingKey, List[_TimingEntry]]


class LineStats:
    timings: _Timings
    unit: float

    def __init__(self, timings: _Timings, unit: float) -> None: ...


class LineProfiler:
    # Common attribute in line_profiler implementations; keep it loose.
    code_map: Any

    def __init__(self, *functions: Any, **kw: Any) -> None: ...

    # Called by Python wrapper's add_callable() :contentReference[oaicite:3]{index=3}
    def add_function(self, func: Any) -> None: ...

    # Called by Python wrapper's wrap_callable() via super() :contentReference[oaicite:4]{index=4}
    def wrap_callable(self, func: object) -> object: ...

    # Called by Python wrapper's get_stats() via super() :contentReference[oaicite:5]{index=5}
    def get_stats(self) -> LineStats: ...

    # Typical profiler controls (used by CLI / mixins in many implementations)
    def enable(self) -> None: ...
    def disable(self) -> None: ...

    # Some builds support nested enable/disable via counters; harmless to expose.
    def enable_by_count(self) -> None: ...
    def disable_by_count(self) -> None: ...

    # Often present; optional but commonly used by callers.
    def clear_stats(self) -> None: ...
