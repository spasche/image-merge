= About =

Python Library to merge images together.

This library creates a single PNG image by layering multiple images on top of
each other. It uses Cairo and GDK-PixBuf for reading, merging and writing the
images.

This library was developed to provide a faster alternative to Python Image
Library when using TileCache (http://tilecache.org/) with the Tile merging
feature.

= Installation instructions =

Use the standard setuptools instruction:

python setup.py install

You will need cairo and gdk-pixbuf-2.0 development packages.

This package also provides the required files for building a Debian/Ubuntu
package.

= Usage with TileCache =

You will need a TileCache version that is able to use this module.
Such a version is available in the repository:
https://github.com/camptocamp/tilecache

This TileCache version will automatically use this module if it is available
in the Python path. You don't need to configure anything.

= Usage with Python =

import image_merge
merged_image = image_merge.merge([image1, image2, ...], preserve_colors=False)

Where image1, image2, ..., imageN are strings with the PNG image bytes to be
merged together (image2 will be placed above image1 and so on).

If the preserve_colors keyword argument is True (defaults to False), do not
compose colors if the source and destination colors have the same rgba values.

The function returns a string containing the merged PNG image.

= Development notes =

Unit tests can be run using the tests.py script.

You can run Valgrind on the tests with the following command line:

G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind --log-file=valgrind.log --leak-check=full python tests.py

= Author =

Developed for Camptocamp SA, by Sylvain Pasche <sylvain.pasche@gmail.com>
