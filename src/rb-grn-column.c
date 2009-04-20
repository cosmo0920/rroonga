/* -*- c-file-style: "ruby" -*- */
/*
  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "rb-grn.h"

#define SELF(object) (RVAL2GRNCOLUMN(object))

VALUE rb_cGrnColumn;
VALUE rb_cGrnFixSizeColumn;
VALUE rb_cGrnVarSizeColumn;
VALUE rb_cGrnIndexColumn;

grn_obj *
rb_grn_column_from_ruby_object (VALUE object)
{
    if (!RVAL2CBOOL(rb_obj_is_kind_of(object, rb_cGrnColumn))) {
	rb_raise(rb_eTypeError, "not a groonga column");
    }

    return RVAL2GRNOBJECT(object, NULL);
}

VALUE
rb_grn_column_to_ruby_object (VALUE klass, grn_ctx *context, grn_obj *column)
{
    return GRNOBJECT2RVAL(klass, context, column);
}

static VALUE
rb_grn_index_column_array_set (VALUE self, VALUE rb_id, VALUE rb_value)
{
    grn_ctx *context;
    grn_rc rc;
    grn_id id;
    unsigned int section;
    grn_obj *old_value, *new_value;
    VALUE rb_section, rb_old_value, rb_new_value;

    context = rb_grn_object_ensure_context(self, Qnil);

    id = NUM2UINT(rb_id);

    if (!RVAL2CBOOL(rb_obj_is_kind_of(rb_value, rb_cHash))) {
	VALUE hash_value;
	hash_value = rb_hash_new();
	rb_hash_aset(hash_value, RB_GRN_INTERN("value"), rb_value);
	rb_value = hash_value;
    }

    rb_grn_scan_options(rb_value,
			"section", &rb_section,
			"old_value", &rb_old_value,
			"value", &rb_new_value,
			NULL);

    if (NIL_P(rb_section))
	section = 1;
    else
	section = NUM2UINT(rb_section);

    old_value = RVAL2GRNBULK(context, rb_old_value);
    new_value = RVAL2GRNBULK(context, rb_new_value);

    rc = grn_column_index_update(context, SELF(self),
				 id, section, old_value, new_value);
    grn_obj_close(context, old_value);
    grn_obj_close(context, new_value);

    rb_grn_rc_check(rc, self);

    return Qnil;
}

static VALUE
rb_grn_index_column_get_sources (VALUE self)
{
    grn_ctx *context;
    grn_obj *column;
    grn_obj sources;
    grn_id *source_ids;
    VALUE rb_sources;
    int i, n;

    context = rb_grn_object_ensure_context(self, Qnil);
    column = SELF(self);

    grn_obj_get_info(context, column, GRN_INFO_SOURCE, &sources);
    rb_grn_context_check(context, self);

    n = GRN_BULK_VSIZE(&sources) / sizeof(grn_id);
    source_ids = (grn_id *)GRN_BULK_HEAD(&sources);
    rb_sources = rb_ary_new2(n);
    for (i = 0; i < n; i++) {
	grn_obj *source;
	source = grn_ctx_get(context, *source_ids);
	rb_ary_push(rb_sources, GRNOBJECT2RVAL(Qnil, context, source));
	source_ids++;
    }

    return rb_sources;
}

static VALUE
rb_grn_index_column_set_sources (VALUE self, VALUE rb_sources)
{
    grn_ctx *context;
    grn_obj *column;
    int i, n;
    VALUE *rb_source_values;
    grn_id *sources;
    grn_rc rc;

    context = rb_grn_object_ensure_context(self, Qnil);
    column = SELF(self);

    n = RARRAY_LEN(rb_sources);
    rb_source_values = RARRAY_PTR(rb_sources);
    sources = ALLOCA_N(grn_id, n);
    for (i = 0; i < n; i++) {
	VALUE rb_source_id;
	grn_obj *source;
	grn_id source_id;

	rb_source_id = rb_source_values[i];
	if (CBOOL2RVAL(rb_obj_is_kind_of(rb_source_id, rb_cInteger))) {
	    source_id = NUM2UINT(rb_source_id);
	} else {
	    source = RVAL2GRNOBJECT(rb_source_id, context);
	    rb_grn_context_check(context, rb_source_id);
	    source_id = grn_obj_id(context, source);
	}
	sources[i] = source_id;
    }

    {
	grn_obj source;
	GRN_OBJ_INIT(&source, GRN_BULK, GRN_OBJ_DO_SHALLOW_COPY);
	GRN_BULK_SET(context, &source, sources, n * sizeof(grn_id));
	rc = grn_obj_set_info(context, column, GRN_INFO_SOURCE, &source);
    }

    rb_grn_context_check(context, self);
    rb_grn_rc_check(rc, self);

    return Qnil;
}

static VALUE
rb_grn_index_column_set_source (VALUE self, VALUE rb_source)
{
    if (!RVAL2CBOOL(rb_obj_is_kind_of(rb_source, rb_cArray)))
	rb_source = rb_ary_new3(1, rb_source);

    return rb_grn_index_column_set_sources(self, rb_source);
}

void
rb_grn_init_column (VALUE mGrn)
{
    rb_cGrnColumn = rb_define_class_under(mGrn, "Column", rb_cGrnObject);
    rb_cGrnFixSizeColumn =
	rb_define_class_under(mGrn, "FixSizeColumn", rb_cGrnColumn);
    rb_cGrnVarSizeColumn =
	rb_define_class_under(mGrn, "VarSizeColumn", rb_cGrnColumn);
    rb_cGrnIndexColumn =
	rb_define_class_under(mGrn, "IndexColumn", rb_cGrnColumn);

    rb_define_method(rb_cGrnIndexColumn, "[]=",
		     rb_grn_index_column_array_set, 2);

    rb_define_method(rb_cGrnIndexColumn, "sources",
		     rb_grn_index_column_get_sources, 0);
    rb_define_method(rb_cGrnIndexColumn, "sources=",
		     rb_grn_index_column_set_sources, 1);
    rb_define_method(rb_cGrnIndexColumn, "source=",
		     rb_grn_index_column_set_source, 1);
}
