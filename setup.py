#!/usr/bin/python

import commands

# adapted from git://github.com/felipec/libmtag-python.git/setup.py
def pkgconfig(*packages, **kw):
    flag_map = {'-I': 'include_dirs', '-L': 'library_dirs', '-l': 'libraries', '-D': 'define_macros'}
    for token in commands.getoutput("pkg-config --libs --cflags %s" % ' '.join(packages)).split():
        key = token[:2]
        val = token[2:]
        if key == "-D":
            val = (val,)
        kw.setdefault(flag_map.get(key), []).append(val)
    return kw

from distutils.core import setup, Extension
setup(name="image_merge",
    author='Sylvain Pasche',
    author_email='sylvain.pasche@gmail.com',
    license='BSD',
    ext_modules=[Extension("image_merge", ["python-image-merge.c"],
        **pkgconfig("cairo", "gdk-pixbuf-2.0")
    )],
    version="0.01"
)
