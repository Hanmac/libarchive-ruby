/****************************************************************************
This file is part of libarchive-ruby. 

libarchive-ruby is a Ruby binding for the C library libarchive. 

Copyright (C) 2011,2012,2013 Hans Mackowiak

libarchive-ruby is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

libarchive-ruby is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with libarchive-ruby; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
****************************************************************************/

#include "main.hpp"

template <>
VALUE wrap< rarchive >(rarchive *file )
{
	return Data_Wrap_Struct(rb_cArchive, NULL, free, file);
}


template <>
rarchive* wrap< rarchive* >(const VALUE &vfile)
{
	if ( ! rb_obj_is_kind_of(vfile, rb_cArchive) )
		return NULL;
	rarchive *file;
  Data_Get_Struct( vfile, rarchive, file);
	return file;
}
template <>
VALUE wrap< archive_entry >(struct archive_entry *entry )
{
	rarchive_entry *temp = new rarchive_entry;
	//archive_entry other = archive_entry_clone(entry);
	temp->entry = archive_entry_clone(entry);
	return Data_Wrap_Struct(rb_cArchiveEntry, NULL, free, temp);
}

template <>
archive_entry* wrap< archive_entry* >(const VALUE &vfile)
{
	if ( ! rb_obj_is_kind_of(vfile, rb_cArchiveEntry) )
		return NULL;
	rarchive_entry  *file;
  Data_Get_Struct( vfile, rarchive_entry, file);
	return file->entry;
}
template <>
VALUE wrap< const char >(const char *str )
{
	return str == NULL? Qnil : rb_str_new2(str);
}
template <>
const char* wrap< const char* >(const VALUE &vfile)
{
	return NIL_P(vfile) ? NULL : StringValueCStr((volatile VALUE&)vfile);
}
