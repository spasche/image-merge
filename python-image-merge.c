// See LICENCE.txt for the licence.
//
// Author: Sylvain Pasche <sylvain.pasche@gmail.com>

#include <Python.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

// Python 2.4 compatibility
#ifndef Py_ssize_t
#define Py_ssize_t ssize_t
#endif

// This is Z_BEST_SPEED compression level, which gives the best speed/size ratio.
#define PNG_COMPRESSION_LEVEL "1"

#define MAX_IMAGES 1024

// from git://github.com/zsx/totem.git/totem-video-thumbnailer.c
// adapted to support alpha
static GdkPixbuf *
cairo_surface_to_pixbuf(cairo_surface_t *surface)
{
  gint stride, width, height, x, y;
  guchar *data, *output, *output_pixel;

  g_assert(cairo_image_surface_get_format(surface) == CAIRO_FORMAT_ARGB32);

  stride = cairo_image_surface_get_stride(surface);
  width = cairo_image_surface_get_width(surface);
  height = cairo_image_surface_get_height(surface);
  data = cairo_image_surface_get_data(surface);

  output = (guchar*) g_malloc(stride * height);
  output_pixel = output;

  for (y = 0; y < height; y++) {
    guint32 *row = (guint32*)(data + y * stride);

    for (x = 0; x < width; x++) {
      uint32_t pixel;
      uint8_t  alpha;

      memcpy (&pixel, &row[x], sizeof (uint32_t));
      alpha = (pixel & 0xff000000) >> 24;

      if (alpha == 0) {
          output_pixel[0] = output_pixel[1] = output_pixel[2] = output_pixel[3] = 0;
      } else {
          output_pixel[0] = (((pixel & 0xff0000) >> 16) * 255 + alpha / 2) / alpha;
          output_pixel[1] = (((pixel & 0x00ff00) >>  8) * 255 + alpha / 2) / alpha;
          output_pixel[2] = (((pixel & 0x0000ff) >>  0) * 255 + alpha / 2) / alpha;
          output_pixel[3] = alpha;
      }
      output_pixel += 4;
    }
  }

  return gdk_pixbuf_new_from_data(output, GDK_COLORSPACE_RGB, TRUE, 8,
                                  width, height, width * 4,
                                  (GdkPixbufDestroyNotify) g_free, NULL);
}

typedef struct {
  char *data;
  unsigned int len, pos;
} PNGBuffer;

static cairo_status_t
read_func(PNGBuffer *buf, unsigned char *data, unsigned int length)
{
  if (buf->pos + length > buf->len) {
    return CAIRO_STATUS_READ_ERROR;
  }
  memcpy(data, &buf->data[buf->pos], length);
  buf->pos += length;
  g_assert(buf->pos <= buf->len);
  return CAIRO_STATUS_SUCCESS;
}

/**
 * Sets pixels of the given target surface to black full transparent if they
 * have the same value as pixels in the source surface.
 */
static void clear_target_if_same_color(cairo_surface_t *target, cairo_surface_t *source) {
  unsigned char *target_data = cairo_image_surface_get_data(target);
  unsigned char *source_data = cairo_image_surface_get_data(source);
  int width = cairo_image_surface_get_width(target);
  int height = cairo_image_surface_get_height(target);
  int stride = cairo_image_surface_get_stride(target);
  int x, y;
  guint32 *t, *s;

  for (y = 0; y < height; y++) {

    t = (guint32*)(target_data + y * stride);
    s = (guint32*)(source_data + y * stride);

    for (x = 0; x < width; x++) {
      if (t[x] == s[x]) {
        t[x] = 0;
      }
    }
  }
}

/**
 * Create the a single PNG image by layering multiple images on top of each other.
 *
 * Input:
 *  images - Array of gchar* pointer to raw PNG image bytes for each image.
 *  sizes - Size in bytes of the image in the images array at the same index.
 *  image_count - Number of images in the images and sizes arrays.
 *  preserve_colors - If true, do not compose colors if the source and destination
 *                    colors have the same rgba values.
 *
 * Output:
 *  @return pointer to the merged PNG image raw bytes, or NULL in case of error
 *          The caller must call g_free on the buffer after use.
 *  buffer_size - size of the returned image buffer in bytes.
 *  error_msg - If this function returns a NULL pointer in case of an error,
 *              This pointer contains the error message.
 *              The caller must call g_free on the pointer after use.
 */
static gchar*
do_merge(gchar* images[], int sizes[], int image_count, int preserve_colors,
         gsize *buffer_size, gchar **error_msg)
{
  cairo_t *cr = NULL;
  cairo_surface_t *argb32_temp = NULL, *argb32 = NULL;
  cairo_status_t status;
  gchar* buffer = NULL;
  int i;
  GError *error = NULL;
  GdkPixbuf *pixbuf = NULL;
  int image_width = 0, image_height = 0;
  PNGBuffer png_buf;

  for (i = 0; i < image_count; i++) {
    png_buf.pos = 0;
    png_buf.len = sizes[i];
    png_buf.data = (char*)images[i];

    argb32_temp = cairo_image_surface_create_from_png_stream(
                    (cairo_read_func_t)read_func, (void *)&png_buf);

    status = cairo_surface_status(argb32_temp);
    if (status) {
      *error_msg = g_strdup_printf("Failed to load image: %s\n",
                                   cairo_status_to_string (status));
      cairo_surface_destroy(argb32);
      cairo_destroy(cr);
      return NULL;
    }
    if (!argb32) {
      image_width = cairo_image_surface_get_width(argb32_temp);
      image_height = cairo_image_surface_get_height(argb32_temp);
      argb32 = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                          image_width, image_height);

      if (!argb32) {
        *error_msg = g_strdup("Failed to create surface");
        cairo_surface_destroy(argb32_temp);
        cairo_surface_destroy(argb32);
        cairo_destroy(cr);
        return NULL;
      }
      cr = cairo_create (argb32);
    } else {
      if (image_width != cairo_image_surface_get_width(argb32_temp) ||
          image_height != cairo_image_surface_get_height(argb32_temp)) {

        *error_msg = g_strdup("Input images must be of the same size");
        cairo_surface_destroy(argb32_temp);
        cairo_surface_destroy(argb32);
        cairo_destroy(cr);
        return NULL;
      }
    }
    cairo_set_source_surface(cr, argb32_temp, 0, 0);
    if (preserve_colors) {
      clear_target_if_same_color(cairo_get_target(cr), argb32_temp);
    }
    cairo_paint(cr);
    cairo_surface_destroy(argb32_temp);
  }

  cairo_destroy (cr);

  pixbuf = cairo_surface_to_pixbuf(argb32);
  if (!pixbuf) {
    *error_msg = g_strdup("NULL returned from cairo_surface_to_pixbuf");
    cairo_surface_destroy(argb32);
    return NULL;
  }
  cairo_surface_destroy(argb32);

  if (!gdk_pixbuf_save_to_buffer(pixbuf, &buffer, buffer_size, "png", &error,
                                 "compression", PNG_COMPRESSION_LEVEL, NULL)) {
    *error_msg = g_strdup_printf("do_merge: %s",
                                 error->message);
    g_error_free(error);
    g_free(buffer);
    g_object_unref(pixbuf);
    return NULL;
  }

  g_object_unref(pixbuf);
  return buffer;
}

static PyObject *
image_merge_merge(PyObject *self, PyObject *args, PyObject *keywds)
{
  PyObject *images_obj, *image_obj;
  int preserve_colors = 0;
  gchar* images[MAX_IMAGES];
  Py_ssize_t size;
  int sizes[MAX_IMAGES];
  int count = 0, i;
  gchar *result_buffer = NULL;
  gsize buffer_size = 0;
  gchar *error_msg = NULL;
  PyObject *res;

  static char *kwlist[] = {"images", "preserve_colors", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|b", kwlist, &images_obj, &preserve_colors)) {
    return NULL;
  }
  if (images_obj == NULL || !PyList_Check(images_obj)) {
    PyErr_SetString(PyExc_TypeError, "First argument must be a list.");
    return NULL;
  }

  count = PyList_Size(images_obj);
  if (count == 0) {
    PyErr_Format(PyExc_SystemError, "No image to merge");
    return NULL;
  }
  if (count > MAX_IMAGES) {
    PyErr_Format(PyExc_SystemError, "Too many images to merge. Max is %d", MAX_IMAGES);
    return NULL;
  }

  for (i = 0; i < count; i++) {
    image_obj = PyList_GetItem(images_obj, i);

    if (!PyString_Check(image_obj)) {
      PyErr_SetString(PyExc_TypeError, "Image data should be a string");
      Py_DECREF(images_obj);
      return NULL;
    }
    if (PyString_AsStringAndSize(image_obj, &images[i], &size) < 0) {
      PyErr_Format(PyExc_SystemError, "Too many images to merge. Max is %d", MAX_IMAGES);
      Py_DECREF(images_obj);
      return NULL;
    }
    sizes[i] = (unsigned int)size;
  }

  result_buffer = do_merge(images, sizes, count, preserve_colors, &buffer_size, &error_msg);
  if (!result_buffer) {
    if (error_msg) {
      PyErr_Format(PyExc_SystemError, "Image merge failure: %s", error_msg);
    } else {
      PyErr_SetString(PyExc_SystemError, "Image merge failure");
    }
    g_free(error_msg);
    Py_DECREF(images_obj);
    return NULL;
  }
  Py_DECREF(images_obj);

  res = Py_BuildValue("s#", result_buffer, buffer_size);
  g_free(result_buffer);
  return res;
}

static PyMethodDef ImageMergeMethods[] = {
  {"merge", (PyCFunction)image_merge_merge, METH_VARARGS | METH_KEYWORDS,
   "Merge images string together (bottom to top) and returns a string with the"
   " resulting image in PNG format"},
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initimage_merge(void)
{
  // FIXME: what if another python module called this already?
  g_type_init();

  (void) Py_InitModule("image_merge", ImageMergeMethods);
}
