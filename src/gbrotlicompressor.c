/* gbrotlicompressor.c
 *
 * Copyright 2019 Igalia S.L.
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <brotli/encode.h>
#include <gio/gio.h>

#include "gbrotlicompressor.h"

struct _GBrotliCompressor
{
	GObject parent_instance;
	BrotliEncoderState *state;
	GError *last_error;
};

static void g_brotli_compressor_iface_init (GConverterIface *iface);

G_DEFINE_TYPE_EXTENDED (GBrotliCompressor, g_brotli_compressor, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (G_TYPE_CONVERTER, g_brotli_compressor_iface_init))

GBrotliCompressor *
g_brotli_compressor_new (void)
{
	return g_object_new (G_TYPE_BROTLI_COMPRESSOR, NULL);
}

static GConverterResult
g_brotli_compressor_convert (GConverter      *converter,
				  const void      *inbuf,
				  gsize            inbuf_size,
				  void            *outbuf,
				  gsize            outbuf_size,
				  GConverterFlags  flags,
				  gsize           *bytes_read,
				  gsize           *bytes_written,
				  GError         **error)
{
	GBrotliCompressor *self = G_BROTLI_COMPRESSOR (converter);
	gsize available_in = inbuf_size;
	const guint8 *next_in = inbuf;
	gsize available_out = outbuf_size;
	guchar *next_out = outbuf;

	g_return_val_if_fail (inbuf, G_CONVERTER_ERROR);

	if (self->last_error) {
		if (error)
			*error = g_steal_pointer (&self->last_error);
		g_clear_error (&self->last_error);
		return G_CONVERTER_ERROR;
	}

	/* NOTE: all error domains/codes must match GZlibCompressor */

	if (self->state == NULL) {
		self->state = BrotliEncoderCreateInstance (NULL, NULL, NULL);
		if (self->state == NULL) {
			g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "GBrotliCompressorError: Failed to initialize state");
			return G_CONVERTER_ERROR;
		}
	}

	BrotliEncoderOperation op = (flags == G_CONVERTER_INPUT_AT_END) ? BROTLI_OPERATION_FINISH :
								(flags == G_CONVERTER_FLUSH) ? BROTLI_OPERATION_FLUSH :
								BROTLI_OPERATION_PROCESS;
	if (!BrotliEncoderCompressStream (self->state, op, &available_in, &next_in, &available_out, &next_out, NULL)) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED, "GBrotliCompressorError: Failed to compress data");
		return G_CONVERTER_ERROR;
	}

	/* available_in is now set to *unread* input size */
	*bytes_read = inbuf_size - available_in;
	/* available_out is now set to *unwritten* output size */
	*bytes_written = outbuf_size - available_out;
g_debug("%s - %zu bytes read, %zu bytes written [%d]", __func__, *bytes_read, *bytes_written, __LINE__);

	if (available_out == 0) {
		self->last_error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_PARTIAL_INPUT, "GBrotliCompressorError: More input required");
		return G_CONVERTER_CONVERTED;
	}

	if (BrotliEncoderIsFinished (self->state)) {
		g_debug("%s - finished [%d]", __func__, __LINE__);
		return G_CONVERTER_FINISHED;
	}

	return G_CONVERTER_CONVERTED;
}

static void
g_brotli_compressor_reset (GConverter *converter)
{
	GBrotliCompressor *self = G_BROTLI_COMPRESSOR (converter);

	if (self->state && BrotliEncoderIsFinished (self->state))
		g_clear_pointer (&self->state, BrotliEncoderDestroyInstance);
	g_clear_error (&self->last_error);
}

static void
g_brotli_compressor_finalize (GObject *object)
{
	GBrotliCompressor *self = (GBrotliCompressor *)object;
	g_clear_pointer (&self->state, BrotliEncoderDestroyInstance);
	g_clear_error (&self->last_error);
	G_OBJECT_CLASS (g_brotli_compressor_parent_class)->finalize (object);
}

static void g_brotli_compressor_iface_init (GConverterIface *iface)
{
g_debug("%s(%p) [%d]", __func__, iface, __LINE__);
	iface->convert = g_brotli_compressor_convert;
	iface->reset = g_brotli_compressor_reset;
}

static void
g_brotli_compressor_class_init (GBrotliCompressorClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = g_brotli_compressor_finalize;
}

static void
g_brotli_compressor_init (GBrotliCompressor *self)
{
}
