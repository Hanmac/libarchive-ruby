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
	path = rb_funcall(rb_cFile,rb_intern("expand_path"),1,path);
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
	VALUE result = rb_ary_new();
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	if(archive_read_open_filename(a,_self->path.c_str(),10240)==ARCHIVE_OK){
		if(argc==0){
			while(archive_read_next_header(a, &entry) == ARCHIVE_OK){
				archive_read_extract(a,entry,0);
				rb_ary_push(result,rb_str_new2(archive_entry_pathname(entry)));
			}
		}else{
			VALUE name,io;
			rb_scan_args(argc, argv, "02", &name, &io);
			//TODO: einbauen von to_io 
			if(rb_obj_is_kind_of(name,rb_cArchiveEntry)==Qtrue){
				archive_read_extract(a,wrap<archive_entry*>(name),0);
				rb_ary_push(result,rb_str_new2(archive_entry_pathname(wrap<archive_entry*>(name))));
			}else if(rb_obj_is_kind_of(name,rb_cString)==Qtrue){
				std::string str1(rb_string_value_cstr(&name)),str2(str1);
				str2 += '/'; // verzeichnisse enden auf /, das weis der nutzer aber vllt nicht
				const char *cstr = archive_entry_pathname(entry);
				while(archive_read_next_header(a, &entry) == ARCHIVE_OK)
					if(str1.compare(cstr)==0 || str2.compare(cstr)==0){
						archive_read_extract(a,entry,0);
						rb_ary_push(result,rb_str_new2(cstr));
					}
			}else if(rb_obj_is_kind_of(name,rb_cRegexp)==Qtrue){
				while(archive_read_next_header(a, &entry) == ARCHIVE_OK){
					VALUE str = rb_str_new2(archive_entry_pathname(entry));
					if(rb_reg_match(name,str)!=Qnil){
						archive_read_extract(a,entry,0);
						rb_ary_push(result,str);
					}
				}
			}
			
		}
		archive_read_finish(a);
	}
	return result;
}

VALUE Archive_extract_if(VALUE self)
{
	VALUE result = rb_ary_new();
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	if(archive_read_open_filename(a,_self->path.c_str(),10240)==ARCHIVE_OK){
		while(archive_read_next_header(a, &entry) == ARCHIVE_OK){
			VALUE str = rb_str_new2(archive_entry_pathname(entry));
			if(RTEST(rb_yield(wrap(entry)))){
				archive_read_extract(a,entry,0);
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

extern "C" void Init_archive(void){
	rb_cArchive = rb_define_class("Archive",rb_cObject);
	rb_define_alloc_func(rb_cArchive,Archive_alloc);
	rb_define_method(rb_cArchive,"initialize",RUBY_METHOD_FUNC(Archive_initialize),1);
	rb_define_private_method(rb_cArchive,"initialize_copy",RUBY_METHOD_FUNC(Archive_initialize_copy),1);
	rb_define_method(rb_cArchive,"path",RUBY_METHOD_FUNC(Archive_path),0);
	rb_define_method(rb_cArchive,"path=",RUBY_METHOD_FUNC(Archive_initialize),1);
	rb_define_method(rb_cArchive,"each",RUBY_METHOD_FUNC(Archive_each),0);
	rb_define_method(rb_cArchive,"extract",RUBY_METHOD_FUNC(Archive_extract),-1);
	rb_define_method(rb_cArchive,"extract_if",RUBY_METHOD_FUNC(Archive_extract),0);
//	rb_define_method(rb_cArchive,"move_to",RUBY_METHOD_FUNC(Archive_move_to),1);	

	rb_define_method(rb_cArchive,"exist?",RUBY_METHOD_FUNC(Archive_exist),0);
	rb_define_method(rb_cArchive,"mtime",RUBY_METHOD_FUNC(Archive_mtime),0);
	rb_define_method(rb_cArchive,"atime",RUBY_METHOD_FUNC(Archive_atime),0);
	rb_define_method(rb_cArchive,"ctime",RUBY_METHOD_FUNC(Archive_ctime),0);
	rb_define_method(rb_cArchive,"stat",RUBY_METHOD_FUNC(Archive_stat),0);
	
	rb_include_module(rb_cArchive,rb_mEnumerable);
	
	Init_archive_entry(rb_cArchive);
}
