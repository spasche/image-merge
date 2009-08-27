#!/usr/bin/env python
#
# Image merge module unit tests.

__author__ = "Sylvain Pasche (sylvain.pasche@gmail.com)"

import random
import unittest
import os
import itertools

from image_merge import merge

class TestImageMerge(unittest.TestCase):

    def setUp(self):
        self.data_dir = os.path.normpath(os.path.join(os.path.dirname(__file__), 'test-data'))

    def testInvalidParams(self):
        self.assertRaises(TypeError, lambda: merge(1))
        self.assertRaises(TypeError, lambda: merge("foo", "bar", []))

    def _getImg(self, file_name):
        """Returns a string of the image file named file_name in the test-data
           directory"""
        return open(os.path.join(self.data_dir, file_name)).read()

    def assertPNGImage(self, data):
        self.assertEquals(data[:4], '\x89PNG', "Result image is not a png")

    def testMergeImage(self):
        img = merge(self._getImg("img1.png"))
        self.assertEquals(img[:4], '\x89PNG', "Result image is not a png")

        img = merge(self._getImg("img1.png"), self._getImg("img2.png"))
        self.assertPNGImage(img)
        # TODO: pixel test that the result is correct

    def testImageMergeLimit(self):
        MAX_IMAGES = 1024

        # this one shouldn't throw. It's a bit slow, disable for faster
        # testing.
        if True:
            images = list(itertools.repeat(
                          self._getImg("img1.png"), MAX_IMAGES))
            self.assertPNGImage(merge(*images))

        def doTest():
            images = list(itertools.repeat(
                          self._getImg("img1.png"), MAX_IMAGES + 1))
            merge(*images)
        self.assertRaises(SystemError, doTest)

    def testUnsupportedFormat(self):
        self.assertRaises(SystemError, lambda: merge("foo"))
        def doTest():
            merge(self._getImg("img1.png"), "foo")
        self.assertRaises(SystemError, doTest)

    def testInvalidImageSize(self):
        def doTest():
            merge(self._getImg("img1.png"), self._getImg("img1-10x10.png"))
        self.assertRaises(SystemError, doTest)

if __name__ == '__main__':
    unittest.main()
