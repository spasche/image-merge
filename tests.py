#!/usr/bin/env python
#
# Image merge module unit tests.

__author__ = "Sylvain Pasche (sylvain.pasche@gmail.com)"

import itertools
import os
import random
import StringIO
import unittest

import Image

from image_merge import merge

class TestImageMerge(unittest.TestCase):

    def setUp(self):
        self.data_dir = os.path.normpath(os.path.join(os.path.dirname(__file__), 'test-data'))

    def testInvalidParams(self):
        self.assertRaises(TypeError, lambda: merge(1))
        self.assertRaises(TypeError, lambda: merge("foo", "bar", []))
        self.assertRaises(SystemError, lambda: merge([]))
        self.assertRaises(TypeError, lambda: merge([1]))
        self.assertRaises(TypeError, lambda: merge(["foo", "bar", []]))

    def _getImg(self, file_name):
        """Returns a string of the image file named file_name in the test-data
           directory"""
        return open(os.path.join(self.data_dir, file_name)).read()

    def assertPNGImage(self, data):
        self.assertEquals(data[:4], '\x89PNG', "Result image is not a png")

    def testMergeImage(self):
        img = merge([self._getImg("img1.png")])
        self.assertPNGImage(img)

        img = merge([self._getImg("img1.png"), self._getImg("img2.png")])
        self.assertPNGImage(img)

    def testMergeImagePixels(self):
        def merge_pixels(input_pixels, preserve_colors, expected_output_pixel):
            images = []
            for p in input_pixels:
                pil_img = Image.new("RGBA", (1, 1))
                pil_img.putdata([p])

                buffer = StringIO.StringIO()
                pil_img.save(buffer, "PNG")
                buffer.seek(0)
                images.append(buffer.read())

            result = merge(images, preserve_colors=preserve_colors)
            self.assertPNGImage(result)
            pil_result = Image.open(StringIO.StringIO(result))

            self.assertEquals(pil_result.getdata()[0], expected_output_pixel)

        merge_pixels([
            (0, 0, 0, 255), # black opaque
        ], False,
            (0, 0, 0, 255)
        )
        merge_pixels([
            (255, 255, 255, 255), # white opaque
            (0, 0, 0, 0), # fully transparent
        ], False,
            (255, 255, 255, 255),
        )
        merge_pixels([
            (255, 255, 255, 255), # white opaque
            (0, 0, 0, 0), # fully transparent
            (0, 0, 0, 0), # fully transparent
            (0, 0, 0, 0), # fully transparent
        ], False,
            (255, 255, 255, 255),
        )
        merge_pixels([
            (255, 255, 255, 255), # white opaque
            (0, 0, 0, 255), # black opaque
        ], False,
            (0, 0, 0, 255)
        )
        merge_pixels([
            (255, 255, 255, 255), # white opaque
            (0, 0, 0, 255), # black opaque
            (0, 0, 0, 255), # black opaque
            (0, 0, 0, 255), # black opaque
            (0, 0, 0, 255), # black opaque
            (0, 0, 0, 255), # black opaque
        ], False,
            (0, 0, 0, 255)
        )

        merge_pixels([
            (0, 202, 0, 155),
            (0, 0, 0, 0),
        ], False,
            (0, 202, 0, 155)
        )

        merge_pixels([
            (0, 0, 0, 0),
            (0, 152, 253, 155),
        ], False,
            (0, 151, 253, 155)
        )

        # Tests preserve_colors flag.

        # Pixels are composed if preserve_colors if False
        merge_pixels([
            (156, 156, 156, 156),
            (156, 156, 156, 156),
        ], False,
            (155, 155, 155, 217)
        )
        # ... but not if preserve_colors is true (minus some rounding).
        merge_pixels([
            (156, 156, 156, 156),
            (156, 156, 156, 156),
        ], True,
            (155, 155, 155, 156),
        )

    def testImageMergeLimit(self):
        MAX_IMAGES = 1024

        # this one shouldn't throw. It's a bit slow, disable for faster
        # testing.
        if True:
            images = list(itertools.repeat(
                          self._getImg("img1.png"), MAX_IMAGES))
            merge(images)

        def doTest():
            images = list(itertools.repeat(
                          self._getImg("img1.png"), MAX_IMAGES + 1))
            merge(images)
        self.assertRaises(SystemError, doTest)

    def testUnsupportedFormat(self):
        self.assertRaises(SystemError, lambda: merge(["foo"]))
        def doTest():
            merge([self._getImg("img1.png"), "foo"])
        self.assertRaises(SystemError, doTest)

    def testInvalidImageSize(self):
        def doTest():
            merge([self._getImg("img1.png"), self._getImg("img1-10x10.png")])
        self.assertRaises(SystemError, doTest)

if __name__ == '__main__':
    unittest.main()
