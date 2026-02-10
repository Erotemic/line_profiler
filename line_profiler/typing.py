"""Shared typing helpers for line_profiler."""

from __future__ import annotations

import types
from typing import TYPE_CHECKING, Any, Callable, Mapping, Protocol, TypeVar

from ._line_profiler import label

if TYPE_CHECKING:  # pragma: no cover
    from typing_extensions import ParamSpec

    PS = ParamSpec('PS')
    T_co = TypeVar('T_co', covariant=True)

    class CythonCallable(Protocol[PS, T_co]):
        def __call__(self, *args: PS.args, **kwargs: PS.kwargs) -> T_co: ...

        @property
        def __code__(self) -> types.CodeType: ...

        @property
        def func_code(self) -> types.CodeType: ...

        @property
        def __name__(self) -> str: ...

        @property
        def func_name(self) -> str: ...

        @property
        def __qualname__(self) -> str: ...

        @property
        def __doc__(self) -> str | None: ...

        @__doc__.setter
        def __doc__(self, doc: str | None) -> None: ...

        @property
        def func_doc(self) -> str | None: ...

        @property
        def __globals__(self) -> dict[str, Any]: ...

        @property
        def func_globals(self) -> dict[str, Any]: ...

        @property
        def __dict__(self) -> dict[str, Any]: ...

        @__dict__.setter
        def __dict__(self, dict: dict[str, Any]) -> None: ...

        @property
        def func_dict(self) -> dict[str, Any]: ...

        @property
        def __annotations__(self) -> dict[str, Any]: ...

        @__annotations__.setter
        def __annotations__(self, annotations: dict[str, Any]) -> None: ...

        @property
        def __defaults__(self): ...

        @property
        def func_defaults(self): ...

        @property
        def __kwdefaults__(self): ...

        @property
        def __closure__(self): ...

        @property
        def func_closure(self): ...
else:
    CythonCallable = type(label)


CLevelCallable = TypeVar(
    'CLevelCallable',
    types.BuiltinFunctionType,
    types.BuiltinMethodType,
    types.ClassMethodDescriptorType,
    types.MethodDescriptorType,
    types.MethodWrapperType,
    types.WrapperDescriptorType,
)

TimingsMap = Mapping[tuple[str, int, str], list[tuple[int, int, int]]]


class ProfileProtocol(Protocol):
    """Protocol for profiler objects used across modules."""

    def __call__(self, func: Callable[..., Any]) -> Callable[..., Any]: ...

    def print_stats(self, *args: Any, **kwargs: Any) -> None: ...

    def dump_stats(self, *args: Any, **kwargs: Any) -> None: ...


class IPythonLike(Protocol):
    def register_magics(self, magics: type) -> None: ...


class StatsLike(Protocol):
    timings: TimingsMap
    unit: float


__all__ = (
    'CLevelCallable',
    'CythonCallable',
    'IPythonLike',
    'ProfileProtocol',
    'StatsLike',
    'TimingsMap',
)
