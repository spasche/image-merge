#!/usr/bin/python

import commands
import re

# If this is True, gcc specific options optimized for the Dual-Core AMD
# Opteron(tm) Processor 2218 HE will be used. That's the CPU found in Amazon EC2.
# See the
# http://developer.amd.com/assets/AMDGCCQuickRef.pdf document for more
# information.
# IMPORTANT: do not this this paramter to True if the CPU running the code is
# not the one above. The code might not run or the performance might not be the
# best.
USE_AMD_OPTERON_OPTIMIZATION = True

# adapted from git://github.com/felipec/libmtag-python.git/setup.py
def pkgconfig(*packages, **kw):
    flag_map = {'-I': 'include_dirs', '-L': 'library_dirs', '-l': 'libraries', '-D': 'define_macros'}
    for token in commands.getoutput("pkg-config --libs --cflags %s" % ' '.join(packages)).split():
        key = token[:2]
        val = token[2:]
        if key == "-D":
            val = (val,)
        if key not in flag_map:
            continue
        kw.setdefault(flag_map.get(key), []).append(val)
    if USE_AMD_OPTERON_OPTIMIZATION:
        kw["extra_compile_args"] = "-mabm -msse4a -march=amdfam10 -O3 -funroll-all-loops -ffast-math " \
            "-mtune=amdfam10 -fprefetch-loop-arrays -ftree-parallelize-loops=2".split(" ")
    return kw


version_line = open("debian/changelog").readlines()[0]
VERSION = re.match(".*\((.*)\).*", version_line).group(1)

from distutils.core import setup, Extension
setup(name="image_merge",
    author='Sylvain Pasche',
    author_email='sylvain.pasche@gmail.com',
    license='BSD',
    ext_modules=[Extension("image_merge", ["python-image-merge.c"],
        **pkgconfig("cairo", "gdk-pixbuf-2.0")
    )],
    version=VERSION
)
