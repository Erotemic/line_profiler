from types import CodeType
from typing import Any


class LineStats:
    def __init__(self, timings: object, unit: float) -> None:
        ...
    ...


class LineProfiler:
    def add_function(self, func: object) -> None:
        ...

    def get_stats(self) -> object:
        ...

    def print_stats(
        self,
        stream: object | None = ...,
        output_unit: object | None = ...,
        stripzeros: bool = ...,
        details: bool = ...,
        summarize: bool = ...,
        sort: bool | str | int = ...,
        rich: bool = ...,
        *,
        config: object | None = ...,
    ) -> None:
        ...

    def dump_stats(self, filename: object) -> None:
        ...

    def tokeneater(self, *args: object, **kwargs: object) -> object:
        ...

    def wrap_callable(self, func: Any) -> Any:
        ...
    ...


def label(code: CodeType | str) -> tuple[str, int, str]:
    ...


def find_cython_source_file(cython_func: object) -> str | None:
    ...
