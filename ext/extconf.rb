#Encoding: UTF-8
=begin
This file is part of libarchive-ruby. 

libarchive-ruby is a Ruby binding for the C library libarchive. 

Copyright © 2011 Hans Mackowiak

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
=end

require 'mkmf'


unless(pkg_config("libarchive"))
	find_library("archive","main")
	find_header("archive.h")
end

$CFLAGS += " -Wall"

unless have_func("rb_string_value_cstr","ruby.h")
	abort("missing VALUE to char convert!")
end

create_makefile("archive")
