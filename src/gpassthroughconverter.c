/* gpassthroughconverter.c
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

#include <gio/gio.h>

#include "gpassthroughconverter.h"

struct _GPassThroughConverter
{
	GObject parent_instance;
};

static void g_pass_through_converter_iface_init (GConverterIface *iface);

G_DEFINE_TYPE_EXTENDED (GPassThroughConverter, g_pass_through_converter, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (G_TYPE_CONVERTER, g_pass_through_converter_iface_init))

GPassThroughConverter *
g_pass_through_converter_new (void)
{
	return g_object_new (G_TYPE_PASS_THROUGH_CONVERTER, NULL);
}

static GConverterResult
g_pass_through_converter_convert (GConverter      *converter,
				  const void      *inbuf,
				  gsize            inbuf_size,
				  void            *outbuf,
				  gsize            outbuf_size,
				  GConverterFlags  flags,
				  gsize           *bytes_read,
				  gsize           *bytes_written,
				  GError         **error)
{
	gsize available_in = inbuf_size;
	const guint8 *next_in = inbuf;
	gsize available_out = outbuf_size;
	guchar *next_out = outbuf;

	g_return_val_if_fail (inbuf, G_CONVERTER_ERROR);

	gsize nbytes = available_out > available_in ? available_in : available_out;

	memcpy(next_out, next_in, nbytes);

	*bytes_read = *bytes_written = nbytes;

	return available_in - nbytes == 0 ? G_CONVERTER_FINISHED : G_CONVERTER_CONVERTED;
}

static void
g_pass_through_converter_reset (GConverter *converter G_GNUC_UNUSED)
{
}

static void
g_pass_through_converter_finalize (GObject *object)
{
	G_OBJECT_CLASS (g_pass_through_converter_parent_class)->finalize (object);
}

static void g_pass_through_converter_iface_init (GConverterIface *iface)
{
	iface->convert = g_pass_through_converter_convert;
	iface->reset = g_pass_through_converter_reset;
}

static void
g_pass_through_converter_class_init (GPassThroughConverterClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = g_pass_through_converter_finalize;
}

static void
g_pass_through_converter_init (GPassThroughConverter *self)
{
}
