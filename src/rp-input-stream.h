/*
 * Copyright/Licensing information.
 */

/* inclusion guard */
#pragma once

#include <glib.h>
#include <gio/gio.h>

/*
 * Potentially, include other headers on which this header depends.
 */

G_BEGIN_DECLS

/*
 * Type declaration.
 */
#define RP_TYPE_INPUT_STREAM (rp_input_stream_get_type())
G_DECLARE_FINAL_TYPE (RpInputStream, rp_input_stream, RP, INPUT_STREAM, GInputStream)

/*
 * Method definitions.
 */
RpInputStream* rp_input_stream_new(void);
void rp_input_istream_set_data(RpInputStream* self, const char* data, gsize len);

G_END_DECLS
