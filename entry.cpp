#include "main.hpp"

#define _self wrap<archive_entry*>(self)

VALUE rb_cArchiveEntry;






VALUE ArchiveEntry_alloc(VALUE self)
{
	return wrap(archive_entry_new());
}

/* TODO funktioniert nicht
VALUE ArchiveEntry_initialize_copy(VALUE self,VALUE source)
{
	VALUE result = rb_call_super(1,&source);
	archive_entry* vself = _self;
	archive_entry* vsource = archive_entry_clone(wrap<archive_entry*>(source));
	*vself = *vsource;
	
	return result;
}
//*/
VALUE ArchiveEntry_dev(VALUE self)
{
	return INT2NUM(archive_entry_dev(_self));
}
VALUE ArchiveEntry_devmajor(VALUE self)
{
	return INT2NUM(archive_entry_devmajor(_self));
}
VALUE ArchiveEntry_devminor(VALUE self)
{
	return INT2NUM(archive_entry_devminor(_self));
}

VALUE ArchiveEntry_rdev(VALUE self)
{
	return INT2NUM(archive_entry_rdev(_self));
}
VALUE ArchiveEntry_rdevmajor(VALUE self)
{
	return INT2NUM(archive_entry_rdevmajor(_self));
}
VALUE ArchiveEntry_rdevminor(VALUE self)
{
	return INT2NUM(archive_entry_rdevminor(_self));
}

//__LA_DECL __LA_MODE_T	 archive_entry_filetype(struct archive_entry *);




VALUE ArchiveEntry_gname(VALUE self)
{
	return rb_str_new2(archive_entry_gname(_self));
}

VALUE ArchiveEntry_uname(VALUE self)
{
	return rb_str_new2(archive_entry_uname(_self));
}

VALUE ArchiveEntry_gid(VALUE self)
{
	return INT2NUM(archive_entry_gid(_self));
}

VALUE ArchiveEntry_uid(VALUE self)
{
	return INT2NUM(archive_entry_uid(_self));
}

VALUE ArchiveEntry_set_gid(VALUE self,VALUE id){
	archive_entry_set_gid(_self,NUM2INT(id));
	return id;
}

VALUE ArchiveEntry_set_uid(VALUE self,VALUE id){
	archive_entry_set_uid(_self,NUM2INT(id));
	return id;
}

VALUE ArchiveEntry_set_gname(VALUE self,VALUE val)
{
	archive_entry_set_gname(_self,rb_string_value_cstr(&val));
	return val;
}
VALUE ArchiveEntry_set_uname(VALUE self,VALUE val)
{
	archive_entry_set_uname(_self,rb_string_value_cstr(&val));
	return val;
}



VALUE ArchiveEntry_pathname(VALUE self)
{
	const char* str = archive_entry_pathname(_self);
	return str ==NULL? Qnil : rb_str_new2(str);
}

VALUE ArchiveEntry_hardlink(VALUE self)
{
	const char* str = archive_entry_hardlink(_self);
	return str ==NULL? Qnil : rb_str_new2(str);
}

VALUE ArchiveEntry_sourcepath(VALUE self)
{
	const char* str = archive_entry_sourcepath(_self);
	return str ==NULL? Qnil : rb_str_new2(str);
}

VALUE ArchiveEntry_strmode(VALUE self)
{
	const char* str = archive_entry_strmode(_self);
	return str ==NULL? Qnil : rb_str_new2(str);
}

VALUE ArchiveEntry_symlink(VALUE self)
{
	const char* str = archive_entry_symlink(_self);
	return str ==NULL? Qnil : rb_str_new2(str);
}

VALUE ArchiveEntry_set_pathname(VALUE self,VALUE val)
{
	archive_entry_set_pathname(_self,rb_string_value_cstr(&val));
	return val;
}

VALUE ArchiveEntry_set_hardlink(VALUE self,VALUE val)
{
	archive_entry_set_hardlink(_self,rb_string_value_cstr(&val));
	return val;
}

VALUE ArchiveEntry_set_symlink(VALUE self,VALUE val)
{
	archive_entry_set_symlink(_self,rb_string_value_cstr(&val));
	return val;
}


VALUE ArchiveEntry_atime(VALUE self)
{
	if(archive_entry_atime_is_set(_self))
		return rb_time_new(archive_entry_atime(_self),archive_entry_atime_nsec(_self));
	else
		return Qnil;
}

VALUE ArchiveEntry_ctime(VALUE self)
{
	if(archive_entry_ctime_is_set(_self))
		return rb_time_new(archive_entry_ctime(_self),archive_entry_ctime_nsec(_self));
	else
		return Qnil;
}

VALUE ArchiveEntry_mtime(VALUE self)
{
	if(archive_entry_mtime_is_set(_self))
		return rb_time_new(archive_entry_mtime(_self),archive_entry_mtime_nsec(_self));
	else
		return Qnil;
}

VALUE ArchiveEntry_birthtime(VALUE self)
{
	if(archive_entry_birthtime_is_set(_self))
		return rb_time_new(archive_entry_birthtime(_self),archive_entry_birthtime_nsec(_self));
	else
		return Qnil;
}

VALUE ArchiveEntry_set_atime(VALUE self,VALUE value)
{
	if(value ==Qnil)
		archive_entry_unset_atime(_self);
	else
		archive_entry_set_atime(_self,NUM2INT(rb_funcall(value,rb_intern("to_i"),0)),NUM2INT(rb_funcall(value,rb_intern("usec"),0)));
	return value;
}
VALUE ArchiveEntry_set_ctime(VALUE self,VALUE value)
{
	if(value ==Qnil)
		archive_entry_unset_ctime(_self);
	else
		archive_entry_set_ctime(_self,NUM2INT(rb_funcall(value,rb_intern("to_i"),0)),NUM2INT(rb_funcall(value,rb_intern("usec"),0)));
	return value;
}
VALUE ArchiveEntry_set_mtime(VALUE self,VALUE value)
{
	if(value ==Qnil)
		archive_entry_unset_mtime(_self);
	else
		archive_entry_set_mtime(_self,NUM2INT(rb_funcall(value,rb_intern("to_i"),0)),NUM2INT(rb_funcall(value,rb_intern("usec"),0)));
	return value;
}

VALUE ArchiveEntry_set_birthtime(VALUE self,VALUE value)
{
	if(value ==Qnil)
		archive_entry_unset_birthtime(_self);
	else
		archive_entry_set_birthtime(_self,NUM2INT(rb_funcall(value,rb_intern("to_i"),0)),NUM2INT(rb_funcall(value,rb_intern("usec"),0)));
	return value;
}

VALUE ArchiveEntry_set_dev(VALUE self,VALUE dev){
	archive_entry_set_dev(_self,NUM2INT(dev));
	return dev;
}
VALUE ArchiveEntry_set_devmajor(VALUE self,VALUE dev){
	archive_entry_set_devmajor(_self,NUM2INT(dev));
	return dev;
}
VALUE ArchiveEntry_set_devminor(VALUE self,VALUE dev){
	archive_entry_set_devminor(_self,NUM2INT(dev));
	return dev;
}

VALUE ArchiveEntry_set_rdev(VALUE self,VALUE dev){
	archive_entry_set_dev(_self,NUM2INT(dev));
	return dev;
}
VALUE ArchiveEntry_set_rdevmajor(VALUE self,VALUE dev){
	archive_entry_set_rdevmajor(_self,NUM2INT(dev));
	return dev;
}
VALUE ArchiveEntry_set_rdevminor(VALUE self,VALUE dev){
	archive_entry_set_rdevminor(_self,NUM2INT(dev));
	return dev;
}

VALUE ArchiveEntry_compare(VALUE self,VALUE other)
{
	if(rb_obj_is_kind_of(other,rb_cArchiveEntry) != Qtrue)
		return Qnil;
	else {
		return rb_funcall(ArchiveEntry_mtime(self),rb_intern("<=>"),1,ArchiveEntry_mtime(other));
	}
}

//ACL struff wird später mit acl gem gekoppelt

VALUE ArchiveEntry_acl(VALUE self){
	archive_entry_acl_reset(_self,ARCHIVE_ENTRY_ACL_TYPE_ACCESS);
	int type,permset,tag,qual;
	const char* name;
	while(archive_entry_acl_next(_self, ARCHIVE_ENTRY_ACL_TYPE_DEFAULT,&type, &permset, &tag,&qual, &name) == 0){
		rb_warn("%i %i %i %s",permset,tag,qual,name);
	}
	return Qnil;
}

VALUE ArchiveEntry_access_acl_count(VALUE self){
	return INT2NUM(archive_entry_acl_count(_self,ARCHIVE_ENTRY_ACL_TYPE_ACCESS));
}

VALUE ArchiveEntry_acl_add(VALUE self){
	archive_entry_acl_add_entry(_self,ARCHIVE_ENTRY_ACL_TYPE_ACCESS, ARCHIVE_ENTRY_ACL_READ, ARCHIVE_ENTRY_ACL_GROUP,
	    101, "abc");
	return self;
}



void Init_archive_entry(VALUE m){
	rb_cArchiveEntry = rb_define_class_under(m,"Entry",rb_cObject);
	rb_define_alloc_func(rb_cArchiveEntry,ArchiveEntry_alloc);
	//rb_define_private_method(rb_cArchiveEntry,"initialize_copy",RUBY_METHOD_FUNC(ArchiveEntry_initialize_copy),1);
	
	rb_define_method(rb_cArchiveEntry,"gname",RUBY_METHOD_FUNC(ArchiveEntry_gname),0);
	rb_define_method(rb_cArchiveEntry,"uname",RUBY_METHOD_FUNC(ArchiveEntry_uname),0);
	rb_define_method(rb_cArchiveEntry,"gid",RUBY_METHOD_FUNC(ArchiveEntry_gid),0);
	rb_define_method(rb_cArchiveEntry,"uid",RUBY_METHOD_FUNC(ArchiveEntry_uid),0);

	rb_define_method(rb_cArchiveEntry,"gname=",RUBY_METHOD_FUNC(ArchiveEntry_set_gname),1);
	rb_define_method(rb_cArchiveEntry,"uname=",RUBY_METHOD_FUNC(ArchiveEntry_set_uname),1);
	rb_define_method(rb_cArchiveEntry,"gid=",RUBY_METHOD_FUNC(ArchiveEntry_set_gid),1);
	rb_define_method(rb_cArchiveEntry,"uid=",RUBY_METHOD_FUNC(ArchiveEntry_set_uid),1);

	rb_define_method(rb_cArchiveEntry,"pathname",RUBY_METHOD_FUNC(ArchiveEntry_pathname),0);
	rb_define_method(rb_cArchiveEntry,"symlink",RUBY_METHOD_FUNC(ArchiveEntry_symlink),0);
	rb_define_method(rb_cArchiveEntry,"hardlink",RUBY_METHOD_FUNC(ArchiveEntry_hardlink),0);
	rb_define_method(rb_cArchiveEntry,"sourcepath",RUBY_METHOD_FUNC(ArchiveEntry_sourcepath),0);

	rb_define_method(rb_cArchiveEntry,"pathname=",RUBY_METHOD_FUNC(ArchiveEntry_set_pathname),1);
	rb_define_method(rb_cArchiveEntry,"symlink=",RUBY_METHOD_FUNC(ArchiveEntry_set_symlink),1);
	rb_define_method(rb_cArchiveEntry,"hardlink=",RUBY_METHOD_FUNC(ArchiveEntry_set_hardlink),1);

	rb_define_method(rb_cArchiveEntry,"atime",RUBY_METHOD_FUNC(ArchiveEntry_atime),0);
	rb_define_method(rb_cArchiveEntry,"ctime",RUBY_METHOD_FUNC(ArchiveEntry_ctime),0);
	rb_define_method(rb_cArchiveEntry,"mtime",RUBY_METHOD_FUNC(ArchiveEntry_mtime),0);
	rb_define_method(rb_cArchiveEntry,"birthtime",RUBY_METHOD_FUNC(ArchiveEntry_birthtime),0);

	rb_define_method(rb_cArchiveEntry,"atime=",RUBY_METHOD_FUNC(ArchiveEntry_set_atime),1);
	rb_define_method(rb_cArchiveEntry,"ctime=",RUBY_METHOD_FUNC(ArchiveEntry_set_ctime),1);
	rb_define_method(rb_cArchiveEntry,"mtime=",RUBY_METHOD_FUNC(ArchiveEntry_set_mtime),1);
	rb_define_method(rb_cArchiveEntry,"birthtime=",RUBY_METHOD_FUNC(ArchiveEntry_set_birthtime),1);
	
	rb_define_method(rb_cArchiveEntry,"dev",RUBY_METHOD_FUNC(ArchiveEntry_dev),0);
	rb_define_method(rb_cArchiveEntry,"dev_major",RUBY_METHOD_FUNC(ArchiveEntry_devmajor),0);
	rb_define_method(rb_cArchiveEntry,"dev_minor",RUBY_METHOD_FUNC(ArchiveEntry_devminor),0);
	rb_define_method(rb_cArchiveEntry,"rdev",RUBY_METHOD_FUNC(ArchiveEntry_rdev),0);
	rb_define_method(rb_cArchiveEntry,"rdev_major",RUBY_METHOD_FUNC(ArchiveEntry_rdevmajor),0);
	rb_define_method(rb_cArchiveEntry,"rdev_minor",RUBY_METHOD_FUNC(ArchiveEntry_rdevminor),0);

	rb_define_method(rb_cArchiveEntry,"dev=",RUBY_METHOD_FUNC(ArchiveEntry_set_dev),1);
	rb_define_method(rb_cArchiveEntry,"dev_major=",RUBY_METHOD_FUNC(ArchiveEntry_set_devmajor),1);
	rb_define_method(rb_cArchiveEntry,"dev_minor=",RUBY_METHOD_FUNC(ArchiveEntry_set_devminor),1);
	rb_define_method(rb_cArchiveEntry,"rdev=",RUBY_METHOD_FUNC(ArchiveEntry_set_rdev),1);
	rb_define_method(rb_cArchiveEntry,"rdev_major=",RUBY_METHOD_FUNC(ArchiveEntry_set_rdevmajor),1);
	rb_define_method(rb_cArchiveEntry,"rdev_minor=",RUBY_METHOD_FUNC(ArchiveEntry_set_rdevminor),1);
	
	rb_include_module(rb_cArchiveEntry,rb_mComparable);
	rb_define_method(rb_cArchiveEntry,"<=>",RUBY_METHOD_FUNC(ArchiveEntry_compare),1);
/*	
	rb_define_method(rb_cArchiveEntry,"acl",RUBY_METHOD_FUNC(ArchiveEntry_acl),0);
	rb_define_method(rb_cArchiveEntry,"acl_add",RUBY_METHOD_FUNC(ArchiveEntry_acl_add),0);
		
	rb_define_method(rb_cArchiveEntry,"access_acl_count",RUBY_METHOD_FUNC(ArchiveEntry_access_acl_count),0);
//*/
}
