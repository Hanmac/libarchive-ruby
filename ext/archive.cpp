#include "main.hpp"

#define _self wrap<rarchive*>(self)

VALUE rb_cArchive;

VALUE Archive_alloc(VALUE self)
{
	rarchive *result = new rarchive;
	return wrap(result);
}

VALUE Archive_initialize_copy(VALUE self, VALUE other)
{
	VALUE result = rb_call_super(1,&other);
	_self->path = std::string(wrap<rarchive*>(other)->path.c_str());
	return result;
}
VALUE Archive_initialize(VALUE self,VALUE path)
{
	path = rb_file_s_expand_path(1,&path);
	_self->path = std::string(rb_string_value_cstr(&path));
	return self;
}


VALUE Archive_path(VALUE self)
{
	return rb_str_new2(_self->path.c_str());
}


VALUE Archive_each(VALUE self)
{
	RETURN_ENUMERATOR(self,0,NULL);
	VALUE result = rb_ary_new();
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	if(archive_read_open_filename(a,_self->path.c_str(),10240)==ARCHIVE_OK){
		while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
			VALUE temp = wrap(entry);
			rb_yield(temp);
			archive_read_data_skip(a); //TODO: method to read the data
			rb_ary_push(result,temp);
		}
		archive_read_finish(a);
	}
	return result;
}



VALUE Archive_extract(int argc, VALUE *argv, VALUE self)
{
	VALUE result = rb_ary_new(),name,opts,temp;
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);//add raw, this is not by all
	int extract_opt = 0;
	rb_scan_args(argc, argv, "02", &name,&opts);
	if(rb_obj_is_kind_of(name,rb_cHash)){
		opts = name;name = Qnil;
	}
	if(rb_obj_is_kind_of(opts,rb_cHash))
		if(RTEST(temp=rb_hash_aref(opts,ID2SYM("extact"))))
			extract_opt = NUM2INT(temp);
	if(archive_read_open_filename(a,_self->path.c_str(),10240)==ARCHIVE_OK){
		if(NIL_P(name)){
			while(archive_read_next_header(a, &entry) == ARCHIVE_OK){
				archive_read_extract(a,entry,extract_opt);
				rb_ary_push(result,rb_str_new2(archive_entry_pathname(entry)));
			}
		}else{
			//TODO: add extract_to_io 
			if(rb_obj_is_kind_of(name,rb_cArchiveEntry)==Qtrue){
				archive_read_extract(a,wrap<archive_entry*>(name),extract_opt);
				rb_ary_push(result,rb_str_new2(archive_entry_pathname(wrap<archive_entry*>(name))));
			}else if(rb_obj_is_kind_of(name,rb_cString)==Qtrue){
				std::string str1(rb_string_value_cstr(&name)),str2(str1);
				str2 += '/'; // dir ends of '/'
				while(archive_read_next_header(a, &entry) == ARCHIVE_OK){
					const char *cstr = archive_entry_pathname(entry);
					if(str1.compare(cstr)==0 || str2.compare(cstr)==0){
						archive_read_extract(a,entry,extract_opt);
						rb_ary_push(result,rb_str_new2(cstr));
					}
				}
			}else if(rb_obj_is_kind_of(name,rb_cRegexp)==Qtrue){
				while(archive_read_next_header(a, &entry) == ARCHIVE_OK){
					VALUE str = rb_str_new2(archive_entry_pathname(entry));
					if(rb_reg_match(name,str)!=Qnil){
						archive_read_extract(a,entry,extract_opt);
						rb_ary_push(result,str);
					}
				}
			}
			
		}
		archive_read_finish(a);
	}
	return result;
}

VALUE Archive_extract_if(int argc, VALUE *argv, VALUE self)
{
	RETURN_ENUMERATOR(self,argc,argv);
	VALUE result = rb_ary_new(),opts,temp;
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	int extract_opt;
	
	rb_scan_args(argc, argv, "01", &opts);
	if(rb_obj_is_kind_of(opts,rb_cHash))
		if(RTEST(temp=rb_hash_aref(opts,ID2SYM("extact"))))
			extract_opt = NUM2INT(temp);
	if(archive_read_open_filename(a,_self->path.c_str(),10240)==ARCHIVE_OK){
		while(archive_read_next_header(a, &entry) == ARCHIVE_OK){
			VALUE str = rb_str_new2(archive_entry_pathname(entry));
			if(RTEST(rb_yield(wrap(entry)))){
				archive_read_extract(a,entry,extract_opt);
				rb_ary_push(result,str);
			}
		}		
		archive_read_finish(a);
	}
	return result;
}
//*
//TODO: archiv to archiv does not work

VALUE Archive_move_to(VALUE self,VALUE other)
{
	VALUE result = rb_ary_new(),to_archive;
	struct archive *a = archive_read_new();
	struct archive *b = archive_write_new();
	struct archive_entry *entry;
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	archive_write_set_format_zip(b);
	archive_write_set_compression_none(b);
	if(archive_read_open_filename(a,_self->path.c_str(),10240)==ARCHIVE_OK){

		if(archive_write_open_filename(b,wrap<rarchive*>(other)->path.c_str())==ARCHIVE_OK){
			while(archive_read_next_header(a, &entry) == ARCHIVE_OK){
				rb_warn("%s",archive_entry_pathname(entry));
				archive_read_extract2(a,entry,b);
				rb_ary_push(result,rb_str_new2(archive_entry_pathname(entry)));
			}
			
			archive_write_finish(b);
		}
		archive_read_finish(a);
	}
}

VALUE Archive_format(VALUE self)
{
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	VALUE result = Qnil;
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	if(archive_read_open_filename(a,_self->path.c_str(),10240)==ARCHIVE_OK){
		archive_read_next_header(a, &entry);
		result = INT2NUM(archive_format(a));
		archive_read_finish(a);
	}
	return result;
}

VALUE Archive_compression(VALUE self)
{
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	VALUE result = Qnil;
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	if(archive_read_open_filename(a,_self->path.c_str(),10240)==ARCHIVE_OK){
		archive_read_next_header(a, &entry);
		result = INT2NUM(archive_compression(a));
		archive_read_finish(a);
	}
	return result;
}

VALUE Archive_format_name(VALUE self)
{
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	const char* name = NULL;
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	if(archive_read_open_filename(a,_self->path.c_str(),10240)==ARCHIVE_OK){
		archive_read_next_header(a, &entry);
		name = archive_format_name(a);
		archive_read_finish(a);
	}
	return name ? rb_str_new2(name) : Qnil	;
}

VALUE Archive_compression_name(VALUE self)
{
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	VALUE result;
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	if(archive_read_open_filename(a,_self->path.c_str(),10240)==ARCHIVE_OK){
		archive_read_next_header(a, &entry);
		result = rb_str_new2(archive_compression_name(a));
		archive_read_finish(a);
	}
	return result;
}

VALUE Archive_add(VALUE self,VALUE obj,VALUE name)
{
	
	std::string selfpath =_self->path;
	char buff[8192];
	char *path = NULL;

	size_t bytes_read;
	struct archive *a = archive_read_new(),*b=archive_write_new(),*c=archive_read_disk_new();
	struct archive_entry *entry;

	int format= ARCHIVE_FORMAT_EMPTY,compression=ARCHIVE_COMPRESSION_NONE,fd,error;
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	//autodetect format and compression
	if(archive_read_open_filename(a,selfpath.c_str(),10240)==ARCHIVE_OK){
		archive_read_next_header(a, &entry);
		format = archive_format(a);
		compression = archive_compression(a);
		archive_read_finish(a);
	}
	if(rb_obj_is_kind_of(obj,rb_cString)){
		VALUE obj2 = rb_file_s_expand_path(1,&obj);
		path = rb_string_value_cstr(&obj2);
		fd = open(path, O_RDONLY);
		if (fd < 0) //TODO: add error
			return self;
	}else if(rb_obj_is_kind_of(obj,rb_cIO)){
		fd = NUM2INT(rb_funcall(obj,rb_intern("fileno"),0));
	}else if(rb_obj_is_kind_of(obj,rb_cFile)){
		VALUE pathname = rb_funcall(obj,rb_intern("path"),0); //source path
		VALUE obj2 = rb_file_s_expand_path(1,&pathname);
		path = rb_string_value_cstr(&obj2);
		fd = NUM2INT(rb_funcall(obj,rb_intern("fileno"),0));
	}else
		rb_raise(rb_eTypeError,"unsuported parameter!");

	//detect format and compression from filename
	if(format == ARCHIVE_FORMAT_EMPTY){
		if(selfpath.substr(selfpath.length()-7).compare(".tar.xz")==0){
			format=ARCHIVE_FORMAT_TAR_USTAR;
			compression=ARCHIVE_COMPRESSION_XZ;
		}else if(selfpath.substr(selfpath.length()-9).compare(".tar.lzma")==0){
			format=ARCHIVE_FORMAT_TAR_USTAR;
			compression=ARCHIVE_COMPRESSION_LZMA;
		}else if(selfpath.substr(selfpath.length()-7).compare(".tar.gz")==0){
			format=ARCHIVE_FORMAT_TAR_USTAR;
			compression=ARCHIVE_COMPRESSION_GZIP;
		}else if(selfpath.substr(selfpath.length()-8).compare(".tar.bz2")==0){
			format=ARCHIVE_FORMAT_TAR_USTAR;
			compression=ARCHIVE_COMPRESSION_BZIP2;
		}else if(selfpath.substr(selfpath.length()-4).compare(".tar")==0)
			format=ARCHIVE_FORMAT_TAR_USTAR;
	}

	
	if((error = archive_write_set_format(b,format)) != ARCHIVE_OK)
		rb_raise(rb_eStandardError,"error code: %d",error);
	switch(compression){
	case ARCHIVE_COMPRESSION_NONE:
		error = archive_write_set_compression_none(b);
		break;
	case ARCHIVE_COMPRESSION_GZIP:
		error = archive_write_set_compression_gzip(b);
		break;
	case ARCHIVE_COMPRESSION_BZIP2:
		error = archive_write_set_compression_bzip2(b);
		break;
	case ARCHIVE_COMPRESSION_COMPRESS:
		error = archive_write_set_compression_compress(b);
		break;
	case ARCHIVE_COMPRESSION_LZMA:
		error = archive_write_set_compression_lzma(b);
		break;
	case ARCHIVE_COMPRESSION_XZ:
		error = archive_write_set_compression_xz(b);
		break;
	case ARCHIVE_COMPRESSION_UU: //uu and rpm has no write suport
	case ARCHIVE_COMPRESSION_RPM:
		rb_raise(rb_eStandardError,"unsupported compresstype");
		break;	
	}
	
	
	if(archive_write_open_filename(b,selfpath.c_str())==ARCHIVE_OK){	
		archive_read_disk_set_standard_lookup(c);
		entry = archive_entry_new();
		if (path)
			archive_entry_copy_sourcepath(entry, path);
		archive_entry_copy_pathname(entry, rb_string_value_cstr(&name));
		archive_read_disk_entry_from_file(c, entry, fd, NULL);
		archive_write_header(b, entry);
		while ((bytes_read = read(fd, buff, sizeof(buff))) > 0)
			archive_write_data(b, buff, bytes_read);
		archive_write_finish_entry(b);
		archive_read_finish(c);
		archive_write_finish(b);
	}
	
	if(rb_obj_is_kind_of(name,rb_cString))
		close(fd);
	return self;
}
//*/
VALUE Archive_add_shift(VALUE self,VALUE name)
{
	VALUE pathname;
	std::string selfpath =_self->path;
	char buff[8192];
	char *path;

	size_t bytes_read;
	struct archive *a = archive_read_new(),*b=archive_write_new(),*c=archive_read_disk_new();
	struct archive_entry *entry;

	int format= ARCHIVE_FORMAT_EMPTY,compression=ARCHIVE_COMPRESSION_NONE,fd,error;
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	//autodetect format and compression
	if(archive_read_open_filename(a,selfpath.c_str(),10240)==ARCHIVE_OK){
		archive_read_next_header(a, &entry);
		format = archive_format(a);
		compression = archive_compression(a);
		archive_read_finish(a);
	}
	if(rb_obj_is_kind_of(name,rb_cString)){
		pathname = name;
		VALUE name2 = rb_file_s_expand_path(1,&name);
		path = rb_string_value_cstr(&name2);
		fd = open(path, O_RDONLY);
		if (fd < 0) //TODO: add error
			return self;
	}else if(rb_obj_is_kind_of(name,rb_cFile)){
		pathname = rb_funcall(name,rb_intern("path"),0);
		VALUE name2 = rb_file_s_expand_path(1,&pathname);
		path = rb_string_value_cstr(&name2);
		fd = NUM2INT(rb_funcall(name,rb_intern("fileno"),0));
		
	}else
		rb_raise(rb_eTypeError,"unsuported parameter!");

	//detect format and compression from filename
	if(format == ARCHIVE_FORMAT_EMPTY){
		if(selfpath.substr(selfpath.length()-7).compare(".tar.xz")==0){
			format=ARCHIVE_FORMAT_TAR_USTAR;
			compression=ARCHIVE_COMPRESSION_XZ;
		}else if(selfpath.substr(selfpath.length()-9).compare(".tar.lzma")==0){
			format=ARCHIVE_FORMAT_TAR_USTAR;
			compression=ARCHIVE_COMPRESSION_LZMA;
		}else if(selfpath.substr(selfpath.length()-7).compare(".tar.gz")==0){
			format=ARCHIVE_FORMAT_TAR_USTAR;
			compression=ARCHIVE_COMPRESSION_GZIP;
		}else if(selfpath.substr(selfpath.length()-8).compare(".tar.bz2")==0){
			format=ARCHIVE_FORMAT_TAR_USTAR;
			compression=ARCHIVE_COMPRESSION_BZIP2;
		}else if(selfpath.substr(selfpath.length()-4).compare(".tar")==0)
			format=ARCHIVE_FORMAT_TAR_USTAR;
	}
	//TODO add archive-error
	if((error = archive_write_set_format(b,format)) != ARCHIVE_OK)
		rb_raise(rb_eStandardError,"error (%d): %s ",error,archive_error_string(b));
	switch(compression){
	case ARCHIVE_COMPRESSION_NONE:
		error = archive_write_set_compression_none(b);
		break;
	case ARCHIVE_COMPRESSION_GZIP:
		error = archive_write_set_compression_gzip(b);
		break;
	case ARCHIVE_COMPRESSION_BZIP2:
		error = archive_write_set_compression_bzip2(b);
		break;
	case ARCHIVE_COMPRESSION_COMPRESS:
		error = archive_write_set_compression_compress(b);
		break;
	case ARCHIVE_COMPRESSION_LZMA:
		error = archive_write_set_compression_lzma(b);
		break;
	case ARCHIVE_COMPRESSION_XZ:
		error = archive_write_set_compression_xz(b);
		break;
	case ARCHIVE_COMPRESSION_UU: //uu and rpm has no write suport
	case ARCHIVE_COMPRESSION_RPM:
		rb_raise(rb_eStandardError,"unsupported compresstype");
		break;	
	}
	
	//*
	if(archive_write_open_filename(b,selfpath.c_str())==ARCHIVE_OK){	
		archive_read_disk_set_standard_lookup(c);
		entry = archive_entry_new();
		archive_entry_copy_sourcepath(entry, path);
		archive_entry_copy_pathname(entry, rb_string_value_cstr(&pathname));
		archive_read_disk_entry_from_file(c, entry, fd, NULL);
		archive_write_header(b, entry);
		while ((bytes_read = read(fd, buff, sizeof(buff))) > 0)
			archive_write_data(b, buff, bytes_read);
		archive_write_finish_entry(b);
		archive_read_finish(c);
		archive_write_finish(b);
	}
	//*/
	if(rb_obj_is_kind_of(name,rb_cString))
		close(fd);
	return self;
}
//*/
VALUE Archive_exist(VALUE self)
{
	return rb_funcall(rb_cFile,rb_intern("exist?"),1,Archive_path(self));
}
VALUE Archive_mtime(VALUE self)
{
	return rb_funcall(rb_cFile,rb_intern("mtime"),1,Archive_path(self));
}
VALUE Archive_atime(VALUE self)
{
	return rb_funcall(rb_cFile,rb_intern("atime"),1,Archive_path(self));
}
VALUE Archive_ctime(VALUE self)
{
	return rb_funcall(rb_cFile,rb_intern("ctime"),1,Archive_path(self));
}
VALUE Archive_stat(VALUE self)
{
	return rb_funcall(rb_cFile,rb_intern("stat"),1,Archive_path(self));
}

VALUE Archive_inspect(VALUE self){
		VALUE array[3];
		array[0]=rb_str_new2("#<%s:%s>");
		array[1]=rb_class_of(self);	
		array[2]=Archive_path(self);
		return rb_f_sprintf(3,array);
}

extern "C" void Init_archive(void){
	rb_cArchive = rb_define_class("Archive",rb_cObject);
	rb_define_alloc_func(rb_cArchive,Archive_alloc);
	rb_define_method(rb_cArchive,"initialize",RUBY_METHOD_FUNC(Archive_initialize),1);
	rb_define_private_method(rb_cArchive,"initialize_copy",RUBY_METHOD_FUNC(Archive_initialize_copy),1);
	rb_define_method(rb_cArchive,"path",RUBY_METHOD_FUNC(Archive_path),0);
	rb_define_method(rb_cArchive,"path=",RUBY_METHOD_FUNC(Archive_initialize),1);
	rb_define_method(rb_cArchive,"each",RUBY_METHOD_FUNC(Archive_each),0);
	rb_define_method(rb_cArchive,"extract",RUBY_METHOD_FUNC(Archive_extract),-1);
	rb_define_method(rb_cArchive,"extract_if",RUBY_METHOD_FUNC(Archive_extract),-1);
//	rb_define_method(rb_cArchive,"move_to",RUBY_METHOD_FUNC(Archive_move_to),1);	

	rb_define_method(rb_cArchive,"add",RUBY_METHOD_FUNC(Archive_add),2);
	rb_define_method(rb_cArchive,"<<",RUBY_METHOD_FUNC(Archive_add_shift),1);
	
	rb_define_method(rb_cArchive,"inspect",RUBY_METHOD_FUNC(Archive_inspect),0);
	
	rb_define_method(rb_cArchive,"format",RUBY_METHOD_FUNC(Archive_format),0);
	rb_define_method(rb_cArchive,"compression",RUBY_METHOD_FUNC(Archive_compression),0);
	rb_define_method(rb_cArchive,"format_name",RUBY_METHOD_FUNC(Archive_format_name),0);
	rb_define_method(rb_cArchive,"compression_name",RUBY_METHOD_FUNC(Archive_compression_name),0);

	rb_define_method(rb_cArchive,"exist?",RUBY_METHOD_FUNC(Archive_exist),0);
	rb_define_method(rb_cArchive,"mtime",RUBY_METHOD_FUNC(Archive_mtime),0);
	rb_define_method(rb_cArchive,"atime",RUBY_METHOD_FUNC(Archive_atime),0);
	rb_define_method(rb_cArchive,"ctime",RUBY_METHOD_FUNC(Archive_ctime),0);
	rb_define_method(rb_cArchive,"stat",RUBY_METHOD_FUNC(Archive_stat),0);
	
	rb_include_module(rb_cArchive,rb_mEnumerable);
	//rb_define_method(rb_cArchive,"size",RUBY_METHOD_FUNC(Archive_size),0);
	
	Init_archive_entry(rb_cArchive);
	
	rb_const_set(rb_cArchive,rb_intern("EXTRACT_TIME"),ARCHIVE_EXTRACT_TIME);
	rb_const_set(rb_cArchive,rb_intern("EXTRACT_OWNER"),ARCHIVE_EXTRACT_OWNER);
	rb_const_set(rb_cArchive,rb_intern("EXTRACT_PERM"),ARCHIVE_EXTRACT_PERM);
}
