/****************************************************************************
This file is part of libarchive-ruby. 

libarchive-ruby is a Ruby binding for the C library libarchive. 

Copyright (C) 2011 Hans Mackowiak

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

#ifndef __RubyAchiveMain_H__
#define __RubyAchiveMain_H__

#include <ruby.h>
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <string>
extern VALUE rb_cArchive,rb_cArchiveEntry;

void Init_archive_entry(VALUE m);

template <typename T>
VALUE wrap(T *arg){ return Qnil;};
template <typename T>
VALUE wrap(T arg){ return Qnil;};
template <typename T>
T wrap(const VALUE &arg){};

struct rarchive{
	std::string path;
};


template <>
inline VALUE wrap< rarchive >(rarchive *file )
{
	return Data_Wrap_Struct(rb_cArchive, NULL, free, file);
}


template <>
inline rarchive* wrap< rarchive* >(const VALUE &vfile)
{
	if ( ! rb_obj_is_kind_of(vfile, rb_cArchive) )
		return NULL;
	rarchive *file;
  Data_Get_Struct( vfile, rarchive, file);
	return file;
}
/*
inline time_t VALUEtime(VALUE t){
	time_t result = time(NULL);
	t = rb_funcall(t,rb_intern("getutc"),0);
	tm *timeinfo = gmtime(&result);
	timeinfo->tm_year = NUM2INT(rb_funcall(t,rb_intern("year"),0)) - 1900;
	timeinfo->tm_mday = NUM2INT(rb_funcall(t,rb_intern("mday"),0));
	timeinfo->tm_yday = NUM2INT(rb_funcall(t,rb_intern("yday"),0)) - 1;
	timeinfo->tm_mon = NUM2INT(rb_funcall(t,rb_intern("mon"),0)) - 1;
	timeinfo->tm_hour = NUM2INT(rb_funcall(t,rb_intern("hour"),0));
	timeinfo->tm_min = NUM2INT(rb_funcall(t,rb_intern("min"),0));
	timeinfo->tm_sec = NUM2INT(rb_funcall(t,rb_intern("sec"),0));
	timeinfo->tm_isdst =rb_funcall(t,rb_intern("isdst"),0) == Qtrue ? 1 : 0;
	return mktime (timeinfo);
}
*/
template <>
inline VALUE wrap< archive_entry >(struct archive_entry *entry )
{
	return Data_Wrap_Struct(rb_cArchiveEntry, NULL, NULL, archive_entry_clone(entry));
}

template <>
inline archive_entry* wrap< archive_entry* >(const VALUE &vfile)
{
	if ( ! rb_obj_is_kind_of(vfile, rb_cArchiveEntry) )
		return NULL;
	archive_entry *file;
  Data_Get_Struct( vfile, archive_entry, file);
	return file;
}

#endif /* __RubyAchiveMain_H__ */

