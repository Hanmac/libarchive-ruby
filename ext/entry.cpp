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

#include "main.hpp"

#define _self wrap<archive_entry*>(self)

VALUE rb_cArchiveEntry;



void rb_define_attr_method(VALUE klass,const std::string &name, VALUE get(VALUE),VALUE set(VALUE,VALUE))
{
	rb_define_method(klass,name.c_str(),RUBY_METHOD_FUNC(get),0);
	rb_define_method(klass,(name + "=").c_str(),RUBY_METHOD_FUNC(set),1);
}


#define macro_attr(attr,get,set) \
VALUE _get_##attr(VALUE self) { return get(archive_entry_##attr(_self)); } \
VALUE _set_##attr(VALUE self,VALUE val) { archive_entry_set_##attr(_self,set(val));return val; }

#define is_type(type,flag) \
VALUE _is_##type(VALUE self) { return archive_entry_filetype(_self) == flag ? Qtrue : Qfalse; }



namespace ArchiveEntry {

VALUE _alloc(VALUE self)
{
	return wrap(archive_entry_new());
}

VALUE _initialize_copy(VALUE self,VALUE source)
{
	rarchive_entry  *file;
  Data_Get_Struct( self, rarchive_entry, file);
  file->entry = archive_entry_clone(wrap<archive_entry*>(source));
	return self;
}


macro_attr(dev,INT2NUM,NUM2INT)
macro_attr(devminor,INT2NUM,NUM2INT)
macro_attr(devmajor,INT2NUM,NUM2INT)

macro_attr(rdev,INT2NUM,NUM2INT)
macro_attr(rdevminor,INT2NUM,NUM2INT)
macro_attr(rdevmajor,INT2NUM,NUM2INT)

macro_attr(uid,INT2NUM,NUM2INT)
macro_attr(gid,INT2NUM,NUM2INT)

macro_attr(uname,wrap,wrap<const char*>)
macro_attr(gname,wrap,wrap<const char*>)

macro_attr(pathname,wrap,wrap<const char*>)
macro_attr(symlink,wrap,wrap<const char*>)
macro_attr(hardlink,wrap,wrap<const char*>)
//macro_attr(sourcepath,wrap,wrap<const char*>)

is_type(file,AE_IFREG)
is_type(symlink,AE_IFLNK)
is_type(directory,AE_IFDIR)
is_type(chardev,AE_IFCHR)
is_type(blockdev,AE_IFBLK)
is_type(pipe,AE_IFIFO)
is_type(socket,AE_IFSOCK)




VALUE _get_atime(VALUE self)
{
	if(archive_entry_atime_is_set(_self))
		return rb_time_new(archive_entry_atime(_self),archive_entry_atime_nsec(_self));
	else
		return Qnil;
}

VALUE _get_ctime(VALUE self)
{
	if(archive_entry_ctime_is_set(_self))
		return rb_time_new(archive_entry_ctime(_self),archive_entry_ctime_nsec(_self));
	else
		return Qnil;
}

VALUE _get_mtime(VALUE self)
{
	if(archive_entry_mtime_is_set(_self))
		return rb_time_new(archive_entry_mtime(_self),archive_entry_mtime_nsec(_self));
	else
		return Qnil;
}

VALUE _get_birthtime(VALUE self)
{
	if(archive_entry_birthtime_is_set(_self))
		return rb_time_new(archive_entry_birthtime(_self),archive_entry_birthtime_nsec(_self));
	else
		return Qnil;
}

VALUE _set_atime(VALUE self,VALUE value)
{
	if(NIL_P(value))
		archive_entry_unset_atime(_self);
	else
		archive_entry_set_atime(_self,NUM2INT(rb_funcall(value,rb_intern("to_i"),0)),NUM2INT(rb_funcall(value,rb_intern("usec"),0)));
	return value;
}

VALUE _set_ctime(VALUE self,VALUE value)
{
	if(NIL_P(value))
		archive_entry_unset_ctime(_self);
	else
		archive_entry_set_ctime(_self,NUM2INT(rb_funcall(value,rb_intern("to_i"),0)),NUM2INT(rb_funcall(value,rb_intern("usec"),0)));
	return value;
}

VALUE _set_mtime(VALUE self,VALUE value)
{
	if(NIL_P(value))
		archive_entry_unset_mtime(_self);
	else
		archive_entry_set_mtime(_self,NUM2INT(rb_funcall(value,rb_intern("to_i"),0)),NUM2INT(rb_funcall(value,rb_intern("usec"),0)));
	return value;
}

VALUE _set_birthtime(VALUE self,VALUE value)
{
	if(NIL_P(value))
		archive_entry_unset_birthtime(_self);
	else
		archive_entry_set_birthtime(_self,NUM2INT(rb_funcall(value,rb_intern("to_i"),0)),NUM2INT(rb_funcall(value,rb_intern("usec"),0)));
	return value;
}


/*
 * call-seq:
 * entry <=> other -> -1,0,1 or nil
 *
 * compares two entries
 */
VALUE _compare(VALUE self,VALUE other)
{
	if(rb_obj_is_kind_of(other,rb_cArchiveEntry) != Qtrue)
		return Qnil;
	else {
		return rb_funcall(_get_mtime(self),rb_intern("<=>"),1,_get_mtime(other));
	}
}


/*
 * call-seq:
 * entry.inspect -> String
 *
 * returns readable string.
 */
VALUE _inspect(VALUE self){
		VALUE array[3];
		array[0]=rb_str_new2("#<%s:%s>");
		array[1]=rb_class_of(self);
		array[2]=_get_pathname(self);
		return rb_f_sprintf(3,array);
}

}

//__LA_DECL __LA_MODE_T	 archive_entry_filetype(struct archive_entry *);



/*
 * call-seq:
 * entry.sourcepath -> String or nil
 *
 * returns the hardlink
 */
VALUE ArchiveEntry_sourcepath(VALUE self)
{
	return wrap(archive_entry_sourcepath(_self));
}

/*
 * call-seq:
 * entry.strmode -> String or nil
 *
 * returns the mode as string
 */
VALUE ArchiveEntry_strmode(VALUE self)
{
	return wrap(archive_entry_strmode(_self));
}

//ACL added later with acl gem

VALUE ArchiveEntry_access_acl(VALUE self){
	if(rb_const_defined(rb_cObject,rb_intern("ACL"))){
		VALUE rb_cAcl = rb_const_get(rb_cObject,rb_intern("ACL"));
		VALUE rb_cAclEntry = rb_const_get(rb_cObject,rb_intern("Entry"));
		VALUE result = rb_class_new_instance(0,NULL,rb_cAcl);
		archive_entry_acl_reset(_self,ARCHIVE_ENTRY_ACL_TYPE_ACCESS);
		int type,permset,tag,qual;
		const char* name;
		while(archive_entry_acl_next(_self, ARCHIVE_ENTRY_ACL_TYPE_ACCESS,&type, &permset, &tag,&qual, &name) == 0){
			VALUE entry;
			VALUE temp[3];
			switch(tag){
			case ARCHIVE_ENTRY_ACL_USER:
			case ARCHIVE_ENTRY_ACL_USER_OBJ:
				temp[0] = ID2SYM(rb_intern("user"));
				break;
			case ARCHIVE_ENTRY_ACL_GROUP:
			case ARCHIVE_ENTRY_ACL_GROUP_OBJ:
				temp[0] = ID2SYM(rb_intern("group"));
				break;
			case ARCHIVE_ENTRY_ACL_MASK:
				temp[0] = ID2SYM(rb_intern("mask"));
				break;
			case ARCHIVE_ENTRY_ACL_OTHER:
				temp[0] = ID2SYM(rb_intern("other"));
				break;
			}
			temp[1] = INT2NUM(permset);
			switch(tag){
			case ARCHIVE_ENTRY_ACL_USER:
			case ARCHIVE_ENTRY_ACL_GROUP:
				temp[2] = INT2NUM(qual);
				entry=rb_class_new_instance(3,temp,rb_cAclEntry);
				break;
			default:
				entry=rb_class_new_instance(2,temp,rb_cAclEntry);
				break;
			}
			rb_funcall(result,rb_intern("<<"),1,entry);
		}
		return result;
	}else
		rb_raise(rb_eNotImpError,"this function require the libacl-ruby gem!");
	return Qnil;
}

VALUE ArchiveEntry_acl_add(VALUE self){
	archive_entry_acl_add_entry(_self,ARCHIVE_ENTRY_ACL_TYPE_ACCESS, ARCHIVE_ENTRY_ACL_READ, ARCHIVE_ENTRY_ACL_GROUP,
	    101, "abc");
	return self;
}



/* Document-attr: atime
 *
 * Encapsulate the writing and reading of the configuration
 * file. ...
 */
/* Document-attr: ctime
 *
 * Encapsulate the writing and reading of the configuration
 * file. ...
 */
/* Document-attr: mtime
 *
 * Encapsulate the writing and reading of the configuration
 * file. ...
 */

void Init_archive_entry(VALUE rb_cArchive){
#if 0
	rb_cArchive = rb_define_class("Archive",rb_cObject);

	rb_define_attr(rb_cArchiveEntry,"path",1,1);
	rb_define_attr(rb_cArchiveEntry,"symlink",1,1);
	rb_define_attr(rb_cArchiveEntry,"hardlink",1,1);

	rb_define_attr(rb_cArchiveEntry,"uid",1,1);
	rb_define_attr(rb_cArchiveEntry,"uname",1,1);
	rb_define_attr(rb_cArchiveEntry,"gid",1,1);
	rb_define_attr(rb_cArchiveEntry,"gname",1,1);

	rb_define_attr(rb_cArchiveEntry,"atime",1,1);
	rb_define_attr(rb_cArchiveEntry,"ctime",1,1);
	rb_define_attr(rb_cArchiveEntry,"mtime",1,1);
	rb_define_attr(rb_cArchiveEntry,"birthtime",1,1);
	
	rb_define_attr(rb_cArchiveEntry,"dev",1,1);
	rb_define_attr(rb_cArchiveEntry,"devmajor",1,1);
	rb_define_attr(rb_cArchiveEntry,"devminor",1,1);

	rb_define_attr(rb_cArchiveEntry,"rdev",1,1);
	rb_define_attr(rb_cArchiveEntry,"rdevmajor",1,1);
	rb_define_attr(rb_cArchiveEntry,"rdevminor",1,1);
	
	
#endif

	using namespace ArchiveEntry;
	rb_cArchiveEntry = rb_define_class_under(rb_cArchive,"Entry",rb_cObject);
	rb_define_alloc_func(rb_cArchiveEntry,_alloc);
	rb_define_private_method(rb_cArchiveEntry,"initialize_copy",RUBY_METHOD_FUNC(_initialize_copy),1);
	rb_define_method(rb_cArchiveEntry,"inspect",RUBY_METHOD_FUNC(_inspect),0);
	
	rb_define_attr_method(rb_cArchiveEntry,"path",_get_pathname,_set_pathname);
	rb_define_attr_method(rb_cArchiveEntry,"symlink",_get_symlink,_set_symlink);
	rb_define_attr_method(rb_cArchiveEntry,"hardlink",_get_hardlink,_set_hardlink);
	
	rb_define_method(rb_cArchiveEntry,"sourcepath",RUBY_METHOD_FUNC(ArchiveEntry_sourcepath),0);

	rb_define_attr_method(rb_cArchiveEntry,"uid",_get_uid,_set_uid);
	rb_define_attr_method(rb_cArchiveEntry,"gid",_get_gid,_set_gid);
	rb_define_attr_method(rb_cArchiveEntry,"uname",_get_uname,_set_uname);
	rb_define_attr_method(rb_cArchiveEntry,"gname",_get_gname,_set_gname);

	rb_define_attr_method(rb_cArchiveEntry,"atime",_get_atime,_set_atime);
	rb_define_attr_method(rb_cArchiveEntry,"ctime",_get_ctime,_set_ctime);
	rb_define_attr_method(rb_cArchiveEntry,"mtime",_get_mtime,_set_mtime);
	rb_define_attr_method(rb_cArchiveEntry,"birthtime",_get_birthtime,_set_birthtime);
	
	rb_define_attr_method(rb_cArchiveEntry,"dev",_get_dev,_set_dev);
	rb_define_attr_method(rb_cArchiveEntry,"dev_major",_get_devmajor,_set_devmajor);
	rb_define_attr_method(rb_cArchiveEntry,"dev_minor",_get_devminor,_set_devminor);
	
	rb_define_attr_method(rb_cArchiveEntry,"rdev",_get_rdev,_set_rdev);
	rb_define_attr_method(rb_cArchiveEntry,"rdev_major",_get_rdevmajor,_set_rdevmajor);
	rb_define_attr_method(rb_cArchiveEntry,"rdev_minor",_get_rdevminor,_set_rdevminor);
	
	rb_define_method(rb_cArchiveEntry,"file?",RUBY_METHOD_FUNC(_is_file),0);
	rb_define_method(rb_cArchiveEntry,"directory?",RUBY_METHOD_FUNC(_is_directory),0);
	rb_define_method(rb_cArchiveEntry,"chardev?",RUBY_METHOD_FUNC(_is_chardev),0);
	rb_define_method(rb_cArchiveEntry,"blockdev?",RUBY_METHOD_FUNC(_is_blockdev),0);
	rb_define_method(rb_cArchiveEntry,"symlink?",RUBY_METHOD_FUNC(_is_symlink),0);
	rb_define_method(rb_cArchiveEntry,"pipe?",RUBY_METHOD_FUNC(_is_pipe),0);
	rb_define_method(rb_cArchiveEntry,"socket?",RUBY_METHOD_FUNC(_is_socket),0);
	
	
	rb_include_module(rb_cArchiveEntry,rb_mComparable);
	rb_define_method(rb_cArchiveEntry,"<=>",RUBY_METHOD_FUNC(_compare),1);
	
	rb_define_alias(rb_cArchiveEntry,"to_s","path");
//*	
	rb_define_method(rb_cArchiveEntry,"access_acl",RUBY_METHOD_FUNC(ArchiveEntry_access_acl),0);
//*/
	

}
