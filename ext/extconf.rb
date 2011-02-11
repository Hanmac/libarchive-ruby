require 'mkmf'


unless(pkg_config("libarchive"))
	find_library("archive","main")
	find_header("archive.h")
end

unless have_func("rb_string_value_cstr","ruby.h")
	abort("missing VALUE to char convert!")
end

create_makefile("archive")
