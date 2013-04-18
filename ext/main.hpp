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
#if HAVE_RUBY_ENCODING_H
#include <ruby/encoding.h>
#endif
#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <string>
extern VALUE rb_cArchive,rb_cArchiveEntry;

void Init_archive_entry(VALUE m);

template <typename T>
VALUE wrap(T *arg){ return Qnil;};
template <typename T>
T wrap(const VALUE &arg){};

enum archive_type {archive_path,archive_fd,archive_buffer,archive_ruby};

struct rarchive{
 //union drumrum?
	std::string path;
	VALUE ruby;
	std::string buffer;
	int fd;
	archive_type type;
	int format;
	int filter;
};

struct rarchive_entry{
	archive_entry *entry;
};

template <>
VALUE wrap< rarchive >(rarchive *file );


template <>
rarchive* wrap< rarchive* >(const VALUE &vfile);

template <>
VALUE wrap< archive_entry >(struct archive_entry *entry );

template <>
archive_entry* wrap< archive_entry* >(const VALUE &vfile);

template <>
VALUE wrap< const char >(const char *str );

template <>
const char* wrap< const char* >(const VALUE &vfile);

#endif /* __RubyAchiveMain_H__ */

