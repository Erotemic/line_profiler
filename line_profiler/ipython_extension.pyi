from . import LineProfiler


class Magics:
    ...


class LineProfilerMagics(Magics):
    def lprun(self, parameter_s: str = ...) -> LineProfiler | None:
        ...

    def lprun_all(self,
                  parameter_s: str = "",
                  cell: str = "") -> LineProfiler | None:
        ...
