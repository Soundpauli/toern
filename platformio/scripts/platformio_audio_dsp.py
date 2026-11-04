"""Keep the sketch small, but let the ladder's per-sample kernel inline at -O2.

At -Os GCC emits four LPF calls per 4x-oversampled sample, per ladder (six
ladders for a CH11 chord). This is a local build choice, not a DSP/quality change:
no fast-math, reduced oversampling, or interpolation-method changes.
"""
from pathlib import Path

Import('env')


def _optimize_ladder(build_env, node):
    if Path(str(node)).name != 'filter_ladder.cpp':
        return node
    flags = [flag for flag in build_env.get('CCFLAGS', [])
             if str(flag) not in ('-Os', '-O0', '-O1', '-O2', '-O3', '-Og', '-Oz', '-Ofast')]
    # Return the constructed object list so PlatformIO does not recompile this
    # translation unit using the library's default size-optimization environment.
    return build_env.Object(node, CCFLAGS=flags + ['-O2'])


env.AddBuildMiddleware(_optimize_ladder)
