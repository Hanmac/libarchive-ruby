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
#include <vector>
#include <map>
#include <exception>
#include <stdexcept> 
#define _self wrap<rarchive*>(self)
#define RB_ENSURE(func1,obj1,func2,obj2)\
 rb_ensure(RUBY_METHOD_FUNC(func1),(VALUE)obj1,RUBY_METHOD_FUNC(func2),(VALUE)obj2)

//TODO: handle string IO as archive Archive.new(stringio)

#ifndef rb_proc_arity
	#define rb_proc_arity(obj) 2
#endif


typedef std::vector<struct archive_entry *> EntryVector;
typedef std::vector<std::string> BuffVector;
typedef std::vector<int> IntVector;

typedef std::vector<std::pair<struct archive_entry *,std::string> > DataVector;

typedef std::map<ID,int> SymToIntType;
SymToIntType SymToFormat;
SymToIntType SymToFilter;

typedef std::map<std::string,std::pair<int,int> > FileExtType;

FileExtType fileExt;

VALUE rb_cArchive,rb_eArchiveError,rb_eArchiveErrorFormat,rb_eArchiveErrorCompression;

VALUE Archive_alloc(VALUE self)
{
	rarchive *result = new rarchive;
	return wrap(result);
}

VALUE Archive_initialize_copy(VALUE self, VALUE other)
{
	VALUE result = rb_call_super(1,&other);
	_self->path = std::string(wrap<rarchive*>(other)->path.c_str());
	_self->type = wrap<rarchive*>(other)->type;
	return result;
}

struct extract_obj {
	struct archive* archive;
	int extract_opt;
};

struct write_obj {
	struct archive* archive;
	DataVector *data;
};

struct add_obj {
	struct archive* archive;
	DataVector *data;
	VALUE obj;
	VALUE name;
};


VALUE wrap_data(const std::string &str)
{
#if HAVE_RUBY_ENCODING_H
	return rb_external_str_new_with_enc(&str[0],str.length(),rb_filesystem_encoding());
#else
	return rb_str_new(&str[0],str.length());
#endif
}



VALUE Archive_read_block_ensure(struct archive * data)
{
	archive_read_finish(data);
	return Qnil;
}

VALUE Archive_write_block_ensure(struct archive * data)
{
	archive_write_finish(data);
	return Qnil;
}

int rubymyopen(struct archive *a, void *client_data)
{
	rb_funcall((VALUE)client_data,rb_intern("rewind"),0);
	return ARCHIVE_OK;
}
ssize_t rubymyread(struct archive *a, void *client_data, const void **buff)
{
	VALUE result =  rb_funcall((VALUE)client_data,rb_intern("read"),0);
	if(result == Qnil){
		*buff = NULL;
		return 0;
	}else{
		if (RSTRING_LEN(result))
			*buff = RSTRING_PTR(result);
		else
			*buff = NULL;
		return RSTRING_LEN(result);
	}
}

ssize_t rubymywrite(struct archive *a,void *client_data,const void *buffer, size_t length){
	VALUE string = rb_str_new((char*)buffer,length);
//	rb_funcall(string,rb_intern("rstrip!"),0);
	return NUM2INT(rb_funcall((VALUE)client_data,rb_intern("write"),1,string));
}

int rubymyclose(struct archive *a, void *client_data)
{
	rb_funcall((VALUE)client_data,rb_intern("rewind"),0);
	return ARCHIVE_OK;
}

namespace RubyArchive {

void read_get_filters(struct archive *a,IntVector &filters)
{
	size_t filtercount = archive_filter_count(a);
	for(size_t i = 0; i < filtercount; ++i)
		filters.push_back(archive_filter_code(a,i));
}

void read_data(struct archive *a,std::string &str)
{
		char buff[8192];
		size_t bytes_read;
	try{
		while ((bytes_read=archive_read_data(a,&buff,sizeof(buff)))>0)
			str.append(buff,bytes_read);
	} catch (...){
		rb_raise(rb_eArchiveError,"error:%d:%s",archive_errno(a),archive_error_string(a));
	}

}

void read_old_data(struct archive *a,DataVector &entries, int &format, IntVector &filters)
{
		struct archive_entry *entry;
		
		while(archive_read_next_header(a, &entry)==ARCHIVE_OK){
			format = archive_format(a);
			if(filters.empty())
				read_get_filters(a,filters);
			std::string buff;
			read_data(a,buff);
			
			entries.push_back(std::make_pair(archive_entry_clone(entry),buff));
			
		}
		archive_read_finish(a);
}

struct archive* create_match_object(VALUE opts)
{
	VALUE temp;
	struct archive* result = archive_match_new();
	if(rb_obj_is_kind_of(opts,rb_cHash))
	{
	if(RTEST(temp=rb_hash_aref(opts,ID2SYM(rb_intern("uid")))))
		archive_match_include_uid(result,NUM2INT(temp));
	if(RTEST(temp=rb_hash_aref(opts,ID2SYM(rb_intern("gid")))))
		archive_match_include_gid(result,NUM2INT(temp));

	}
	return result;
}


bool match_entry(struct archive_entry *entry,VALUE name)
{
	const char *cstr = archive_entry_pathname(entry);
	if(rb_obj_is_kind_of(name,rb_cArchiveEntry)){
		return std::string(cstr).compare(archive_entry_pathname(wrap<archive_entry*>(name)))==0;
	}else if(rb_obj_is_kind_of(name,rb_cRegexp)){
		VALUE str = rb_str_new2(cstr);
		return rb_reg_match(name,str)!=Qnil;
	}else
		name = rb_funcall(name,rb_intern("to_s"),0);
		std::string str1(StringValueCStr(name)),str2(str1);
		str2 += '/'; // dir ends of '/'
		return str1.compare(cstr)==0 || str2.compare(cstr)==0;
					
	return false;
}


int write_set_filters(struct archive *data,const IntVector &filter)
{
	int error = 0;
	
	for (IntVector::const_reverse_iterator it = filter.rbegin(); it != filter.rend();++it)
		if(*it != ARCHIVE_FILTER_NONE)
			archive_write_add_filter(data,*it);

	return error;
}

void read_data_from_fd(struct archive *file,VALUE name,int fd,DataVector &data)
{
	char buff[8192];
	size_t bytes_read;
	std::string strbuff;
	
	struct archive_entry *entry = archive_entry_new();
	
	archive_read_disk_entry_from_file(file, entry, fd, NULL);
	archive_entry_copy_pathname(entry, StringValueCStr(name));
	
	
	while ((bytes_read = read(fd, buff, sizeof(buff))) > 0)
		strbuff.append(buff, bytes_read);
	
	data.push_back(std::make_pair(entry,strbuff));
}

void read_data_from_ruby(struct archive *file,VALUE name,VALUE obj,DataVector &data)
{
	std::string strbuff;
	
	struct archive_entry *entry = archive_entry_new();
	
	archive_entry_copy_pathname(entry, StringValueCStr(name));
	
	//TODO should i read in chunks?
	VALUE result = rb_funcall(obj,rb_intern("read"),0);
	strbuff.append(StringValueCStr(result), RSTRING_LEN(result));
	
	data.push_back(std::make_pair(entry,strbuff));
}

struct read_data_path_obj
{
	struct archive* archive;
	struct archive* match;
	DataVector *data;
	VALUE path;
};


VALUE read_data_from_path_block(struct read_data_path_obj *obj)
{
	struct archive_entry *entry = archive_entry_new();
	
	while(archive_read_next_header2(obj->archive,entry) == ARCHIVE_OK)
	{
		const char *cstr = archive_entry_pathname(entry);
		
		//allways add subdirs
		archive_read_disk_descend(obj->archive);
		
		// add the "." base dir
		if(strcmp(cstr,".") == 0)
			continue;
		
		// fist look at the match object
		if(archive_match_excluded(obj->match,entry))
			continue;
		
		// then if path is Regexp try to match against it
		if(rb_obj_is_kind_of(obj->path,rb_cRegexp))
			if(!RTEST(rb_reg_match(obj->path,rb_str_new2(cstr))))
				continue;
		
		// then use the block if given
		if((!rb_block_given_p()) || RTEST(rb_yield(wrap(entry))))
		{
			//TODO: react to the change of the entry object
			std::string strbuff;
			const void *p;
			size_t size;
			int64_t offset;

			while(archive_read_data_block(obj->archive, &p, &size, &offset) == ARCHIVE_OK)
				strbuff.append((const char*)p,size);

			struct archive_entry *entry2 = archive_entry_clone(entry);
			
			
			//Drop the ./ from a filename when using pattern
			std::string path(archive_entry_pathname(entry2));
			if(path.compare(0,2,"./") == 0) {
				path.erase(0,2);
				archive_entry_copy_pathname(entry2,path.c_str());
			}
			
			obj->data->push_back(std::make_pair(entry2,strbuff));
			
		}
		
	}
	return Qnil;
}

VALUE Archive_read_block_close(struct archive * data)
{
	archive_read_close(data);
	return Qnil;
}

void read_data_from_path(struct archive * file,struct archive * match,VALUE rpath,const char* path,DataVector &data)
{

	archive_read_disk_open(file,path);
	
	read_data_path_obj obj;
	obj.archive = file;
	obj.path = rpath;
	obj.match = match;
	obj.data = &data;

	//struct archive_entry *entry = archive_entry_new();
	
	//archive_read_next_header2(file,entry);
	//archive_read_disk_descend(file);

	RB_ENSURE(read_data_from_path_block,&obj,Archive_read_block_close,file);
}


void read_data_from_path2(VALUE path,VALUE opts,DataVector &data)
{
	struct archive * file = archive_read_disk_new();
	struct archive * match = create_match_object(opts);
	
	archive_read_disk_set_standard_lookup(file);
	const char * str = ".";
	
	if(!rb_obj_is_kind_of(path,rb_cRegexp))
	{
		path = rb_funcall(path,rb_intern("to_s"),0);
		if(!RTEST(rb_funcall(rb_cFile,rb_intern("exist?"),1,path))){
			archive_match_include_pattern(match,StringValueCStr(path));
		}else
			str = StringValueCStr(path);
	}
	read_data_from_path(file,match,path,str,data);
}

}

void Archive_format_from_path(VALUE self,int &format,IntVector &filters)
{
	if(_self->type == archive_path){
		std::string selfpath = _self->path;
		size_t selfpathlength = selfpath.length();
		for(FileExtType::iterator it = fileExt.begin(); it != fileExt.end(); ++it) {
			if(selfpath.rfind(it->first) + it->first.length() == selfpathlength) {
				format = it->second.first;
				filters.push_back(it->second.second);
				return;
			}
		}
	}
	
	format = _self->format;
	filters.push_back(_self->filter);
	
}

int Archive_read_ruby(VALUE self,struct archive *data)
{

	archive_read_support_filter_all(data);
	archive_read_support_format_all(data);
#if HAVE_ARCHIVE_READ_SUPPORT_FORMAT_RAW
	archive_read_support_format_raw(data);
#endif
	int error =0;
	switch(_self->type){
		case archive_path:
			error = archive_read_open_filename(data,_self->path.c_str(),10240);
			break;
		case archive_fd:
			error = archive_read_open_fd(data,_self->fd,10240);
			break;
		case archive_buffer: 
			break;
		case archive_ruby:
			error = archive_read_open(data, (void*)_self->ruby,rubymyopen,rubymyread,rubymyclose);
			break;
		}
	return error;
}



int Archive_write_ruby(VALUE self,struct archive *data,int format, const IntVector &filters)
{
	int error =0;
	
	if((error = archive_write_set_format(data,format)) != ARCHIVE_OK)
		rb_raise(rb_eArchiveErrorFormat,"error (%d): %s ",error,archive_error_string(data));
		
	RubyArchive::write_set_filters(data,filters);
	
	switch(_self->type){
		case archive_path:
			error = archive_write_open_filename(data,_self->path.c_str());
			break;
		case archive_fd:
			error = archive_write_open_fd(data,_self->fd);
			break;
		case archive_buffer: 
			break;
		case archive_ruby:
			error = archive_write_open(data,(void*)_self->ruby,rubymyopen,rubymywrite,rubymyclose);
			break;
		}
	return error;
}

/*
 *call-seq:
 *   new( path [, format [, compression ] ] ) → an_archive
 * 
 * Makes a new Archive object. If format is given, a new archive is created or 
 * an existing archive will be converted into the given format. 
 * ===Parameters
 * [path]   The path to the archive. May or may not exist. 
 * [format] The archive's format as a symbol. If you ommit this, the format will 
 *          be guessed from the file extension. Possible formats are: 
 *          * :ar
 *          * :tar
 *          * :pax
 *          * :xar
 *          * :zip
 * [compression] Symbol inidicating the compression you want to use. If 
 *               ommited, it will be guessed from the file extension. 
 *               Possible formats are: 
 *               * :bzip2
 *               * :compress
 *               * :gzip
 *               * :lzma
 *               * :xz
 * ===Raises
 * [FormatError]      Unknown archive format or writing not supported. 
 * [CompressionError] Unknown compression format. 
 * ===Examples
 * See the README for some examples. 
*/
 
VALUE Archive_initialize(int argc, VALUE *argv,VALUE self)
{
	VALUE path,r_format,r_filter;
	rb_scan_args(argc, argv, "12", &path,&r_format,&r_filter);
	if(rb_obj_is_kind_of(path,rb_cIO)){
		_self->fd = NUM2INT(rb_funcall(path,rb_intern("fileno"),0));
		_self->type = archive_fd;
		rb_scan_args(argc, argv, "21", &path,&r_format,&r_filter);
	}else if(rb_obj_is_kind_of(path,rb_cInteger)){
		_self->fd = NUM2INT(path);
		_self->type = archive_fd;
		rb_scan_args(argc, argv, "21", &path,&r_format,&r_filter);
	}else if(rb_respond_to(path,rb_intern("read"))){
		_self->type = archive_ruby;
		rb_scan_args(argc, argv, "21", &path,&r_format,&r_filter);
	}else{
		path = rb_file_s_expand_path(1,&path);
		_self->path = std::string(StringValueCStr(path));
		_self->type =archive_path;
	}
	_self->ruby = path;
	//to set the format the file must convert
	if(!NIL_P(r_format)){
	
		//struct archive *a = archive_read_new(),*b=archive_write_new();
		//struct archive_entry *entry;
		int format,filter;
		
		if(SYMBOL_P(r_format)){
			SymToIntType::iterator it = SymToFormat.find(SYM2ID(r_format));
			if(it != SymToFormat.end()) {
				format = it->second;
			}else
				rb_raise(rb_eTypeError,"wrong format");
		}else
			rb_raise(rb_eTypeError,"exepted Symbol");
			
		if(NIL_P(r_filter)){
			filter = ARCHIVE_FILTER_NONE;
		}else if(SYMBOL_P(r_filter)){
			SymToIntType::iterator it = SymToFilter.find(SYM2ID(r_filter));
			if(it != SymToFilter.end()) {
				filter = it->second;
			} else
				rb_raise(rb_eTypeError,"unsupported filter");
		}else
			rb_raise(rb_eTypeError,"exepted Symbol");
		
		//autodetect format and compression

		_self->format = format;
		_self->filter = filter;
		
//		if(Archive_read_ruby(self,a)==ARCHIVE_OK){
//			while(archive_read_next_header(a, &entry)==ARCHIVE_OK){
//				if(format==archive_format(a) && filter==archive_filter_code(a,0)){
//					archive_read_finish(a);
//					return self;
//				}
//				entries.push_back(archive_entry_clone(entry));
//				allbuff.push_back(std::string(""));
//				
//				RubyArchive::read_data(a,allbuff.back());
//			}
//			archive_read_finish(a);
//			if(Archive_write_ruby(self,b,format,IntVector(filter))==ARCHIVE_OK){
//				//write old data back
//				for(unsigned int i=0; i<entries.size(); i++){
//					archive_write_header(b,entries[i]);
//					archive_write_data(b,allbuff[i].c_str(),allbuff[i].length());
//					archive_write_finish_entry(b);
//				}
//				archive_write_finish(b);
//			}
//		}
	}
	return self;
}

/*
 * call-seq:
 *   path() → a_string
 * 
 * Returns the path (filename) of the archive. 
 * ===Return value
 * Returns path of the archive. May or may not exist. 
 * ===Example
 *  a.path #=> /home/freak/myarchive.tar.gz
*/

VALUE Archive_path(VALUE self)
{
	return (_self->type == archive_path) ? rb_str_new2(_self->path.c_str()) : Qnil;
}

VALUE Archive_each_block(struct archive *data)
{
	struct archive_entry *entry;
	while (archive_read_next_header(data, &entry) == ARCHIVE_OK) {
		VALUE temp = wrap(entry);
		if(rb_proc_arity(rb_block_proc())==1){
			rb_yield(temp);
			archive_read_data_skip(data);
		}else{
			std::string str;
			RubyArchive::read_data(data,str);
			rb_yield_values(2,temp,wrap_data(str));
		}
	}
	return Qnil;
}

/*
 * call-seq:
 *   each(){|entry [, data]| ... } → self
 *   each()                        → an_enumerator
 * 
 * Iterates through the archive and yields each entry as an Archive::Entry object. The second parameter 
 * contains the data of that entry, so you don't have to extract it only to read what's in it. 
 * ===Return value
 * If a block is given, returns self, otherwise an enumerator. 
 * ===Example
 *   a.each{|entry| p entry.path}
 *   a.each{|entry, data| puts "'#{entry.path}' contains '#{data}'"}
 * Output: 
 *   "file1.txt"
 *   "file2.txt"
 *   'file1.txt' contains 'I am file1!'
 *   'file2.txt' contains 'I am file2!'
*/

VALUE Archive_each(VALUE self)
{
	RETURN_ENUMERATOR(self,0,NULL);
	struct archive *a = archive_read_new();

	
	int error=Archive_read_ruby(self,a);
	if(error==ARCHIVE_OK){
		RB_ENSURE(Archive_each_block,a,Archive_read_block_ensure,a);
		return self;
	}
	return Qnil;
}


VALUE Archive_each_entry_block(struct archive *data)
{
	VALUE result = rb_ary_new();
	struct archive_entry *entry;
	while (archive_read_next_header(data, &entry) == ARCHIVE_OK) {
		VALUE temp = wrap(entry);
		rb_yield(temp);
		archive_read_data_skip(data);
		rb_ary_push(result,temp);
	}
	return result;
}

/*
 * call-seq:
 *   each_entry() {|entry| ... } → an_array
 *   each_entry()                → an_enumerator
 * 
 * Iterates through the archive and yields each entry as an Archive::Entry object. 
 * This is the same as #each, but doesn't allow for the second block parameter. 
 * ===Return value
 * If a block is given, returns an array of Archive::Entry objects, otherwise an enumerator. 
 * ===Example
 *   a.each_entry{|entry| p entry.path}
 * Output:
 *   "file1.txt"
 *   "file2.txt"
*/

VALUE Archive_each_entry(VALUE self)
{
	RETURN_ENUMERATOR(self,0,NULL);
	struct archive *a = archive_read_new();
	
	if(Archive_read_ruby(self,a)==ARCHIVE_OK)
		return RB_ENSURE(Archive_each_entry_block,a,Archive_read_block_ensure,a);
	return Qnil;
}


VALUE Archive_each_data_block(struct archive *data)
{
	struct archive_entry *entry;
	while (archive_read_next_header(data, &entry) == ARCHIVE_OK) {
		std::string str;
		RubyArchive::read_data(data,str);
		rb_yield(wrap_data(str));
	}
	return Qnil;
}


/*
 * call-seq:
 *   each_data() {|data| }    → self
 *   each_data()              → an_enumerator
 * 
 * Iterates through the archive and yields each entry's data as a string. 
 * This is the same as #each, but doesn't allow for the first block parameter. 
 * ===Return value
 * If a block is given, returns self, otherwise an enumerator. 
 * ===Example
 *   a.each{|data| puts "This is: '#{data}'"}
 * Output: 
 *   This is: 'I am file1!'
 *   This is: 'I am file2!'
*/

VALUE Archive_each_data(VALUE self)
{
	RETURN_ENUMERATOR(self,0,NULL);
	struct archive *a = archive_read_new();
	
	if(Archive_read_ruby(self,a)==ARCHIVE_OK)
		RB_ENSURE(Archive_each_data_block,a,Archive_read_block_ensure,a);
	return self;
}

VALUE Archive_each_filter_block(struct archive *data)
{
	struct archive_entry *entry;
	archive_read_next_header(data, &entry);
	size_t count = archive_filter_count(data);
	for(size_t i = 0; i < count; ++i)
		rb_yield_values(2,INT2NUM(archive_filter_code(data,i)),rb_str_new2(archive_filter_name(data,i)));

	return Qnil;
}


VALUE Archive_each_filter(VALUE self)
{
	RETURN_ENUMERATOR(self,0,NULL);
	struct archive *a = archive_read_new();
	
	if(Archive_read_ruby(self,a)==ARCHIVE_OK)
		RB_ENSURE(Archive_each_filter_block,a,Archive_read_block_ensure,a);
	return self;
}

/*
 * call-seq:
 *   archive.to_hash → Hash
 * 
 * Iterates through the archive and yields each data of an entry as a string object.
 * ===Return value
 * returns Hash of Archive::Entry => String
*/

VALUE Archive_to_hash(VALUE self)
{
	VALUE result = rb_hash_new();
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	
	if(Archive_read_ruby(self,a)==ARCHIVE_OK){
		while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
			std::string str;
			RubyArchive::read_data(a,str);
			rb_hash_aset(result,wrap(entry),wrap_data(str));
		}
		archive_read_finish(a);
	}
	return result;
}

VALUE Archive_map_block(struct write_obj * data){
	for(unsigned int i=0; i< data->data->size(); i++){
		VALUE temp = wrap(data->data->at(i).first);
		VALUE val;
		if(rb_proc_arity(rb_block_proc())<2){
			val = rb_yield(temp);
		}else{
			val = rb_yield_values(2,temp,wrap_data(data->data->at(i).second));
		}
		VALUE entry,rdata= Qnil;
		if(rb_obj_is_kind_of(val,rb_cArray)){
			entry=rb_ary_entry(val,0);
			rdata=rb_ary_entry(val,1);
		}else
			entry = val;
		if(rb_obj_is_kind_of(entry,rb_cArchiveEntry)){
			archive_write_header(data->archive,wrap<struct archive_entry *>(val));
			
			if(rdata == Qnil){
				std::string &buff = data->data->at(i).second;
				archive_write_data(data->archive,&buff[0],buff.length());
			}else{
				//char* buff = StringValueCStr(rdata);
				archive_write_data(data->archive,RSTRING_PTR(rdata),RSTRING_LEN(rdata));
			}
			archive_write_finish_entry(data->archive);
		}else if(entry==Qnil){
		}else
			rb_raise(rb_eTypeError,"exepted %s!",rb_class2name(rb_cArchiveEntry));
	}
	return Qnil;
}

/*
 * call-seq:
 *   map!() {| entry [,data] | ... } → Array
 *   archive.map!()                  → Enumerator
 * 
 * Iterates through the archive and changes it's data "on the fly", i.e. 
 * the value your block returns for each iteration is put for the data 
 * in the yielded entry. Your block is expected to return a 2-element 
 * array of form
 * [archive_entry, "data"]
 * where +archive_enty+ is the +entry+ yielded to the block (which you 
 * may modify via the Archive::Entry methods) and <tt>"data"</tt> is a 
 * string containing the data you want to set for this entry. 
 * 
 * The block parameters are the same as for #each. 
 * ===Return value
 * The archive itself.
 * ===Example
 *   #Double the contents in each file of the archive
 *   a.map!{|entry, data| [entry, data * 2]}
 *   #Clear all files in the archive
 *   a.map!{|entry| [entry, ""]}
*/
VALUE Archive_map_self(VALUE self)
{
	RETURN_ENUMERATOR(self,0,NULL);

	struct archive *a = archive_read_new(),*b=archive_write_new();
	int format = ARCHIVE_FORMAT_EMPTY;
	
	DataVector entries;
	IntVector filters;
	
	//autodetect format and filters
	if(Archive_read_ruby(self,a)==ARCHIVE_OK){
		
		RubyArchive::read_old_data(a,entries, format, filters);

		if(Archive_write_ruby(self,b,format,filters)==ARCHIVE_OK){
			write_obj obj;
			obj.archive = b;
			obj.data = &entries;
			RB_ENSURE(Archive_map_block,&obj,Archive_write_block_ensure,b);
		}	
	}	
	return self;
}

/*
 * call-seq:
 *   [](name) → Archive::Entry or nil
 * 
 * Returns an archive entry for the given name.
 * ===Parameters
 * [name] could be a String or a Regex.
 * ===Return value
 * If a matching entry is found, it's returned as an Archive::Entry object. If not, 
 * nil is returned. 
 * ===Example
 *   #Assuming your archive contains file.txt and ruby.txt
 *   
 *   a["file.txt"] #=> Archive::Entry
 *   a[/txt/] #=> Archive::Entry
*/

VALUE Archive_get(VALUE self,VALUE val)
{
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	
	if(Archive_read_ruby(self,a)==ARCHIVE_OK){
		while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
			if(RubyArchive::match_entry(entry,val)){
				VALUE result = wrap(entry);
				archive_read_finish(a);
				return result;
			}
		}
		archive_read_finish(a);
	}
	return Qnil;
}


void extract_extract(struct archive *a,struct archive_entry *entry,int extract_opt,VALUE io, int fd)
{
	if(rb_obj_is_kind_of(io,rb_cIO)){
		archive_read_data_into_fd(a,fd);
	}else if(rb_respond_to(io,rb_intern("write"))){
		char buff[8192];
		size_t bytes_read;
		while ((bytes_read=archive_read_data(a,&buff,sizeof(buff)))>0)
			rb_funcall(io,rb_intern("write"),1,rb_str_new(buff,bytes_read));
	}else
		archive_read_extract(a,entry,extract_opt);
}
/*
 * call-seq:
 *   extract( [name = nil [, io [ ,opt ] ] ] ) → an_array
 * 
 * Extract files to current directory. 
 * ===Parameters
 * [name]     (nil) could be an Archive::Entry, a String or an Regex. If given, 
 *            only this entry is extracted. Otherwise extracts the whole 
 *            archive. 
 * [io]       an instance of IO or something with a +write+ method like 
 *            StringIO. If given, the entry specified via +name+ will be 
 *            extracted into +io+ instead of a file. 
 * [opt]      is an option hash. See below for possible options. 
 * ===Parameters for the option hash
 * [:extract] flag, Integer combined of the Archive::Extract_* constants. 
 *            This tells libarchive-ruby to only extract the file attributes 
 *            you specify here. Exceptions is Archive::EXTRACT_NO_OVERWRITE 
 *            which prevents this method from overwrtiting existing files. 
 * ===Return value
 * The paths of the extracted entries as an array. 
 * ===Example
 *   #Simply extract everything into the current directory. 
 *   a.extract
 *   #Extract only file1.txt
 *   a.extract("file1.txt")
 *   #Same as above, but extract it to a StringIO
 *   s = StringIO.new
 *   a.extract("file1.txt", s)
 *   #Same as the first example, but only extract information about the 
 *   #modification time. 
 *   a.extract(nil, nil, extract: Archive::EXTRACT_TIME)
*/

VALUE Archive_extract(int argc, VALUE *argv, VALUE self)
{

	VALUE result = rb_ary_new(),name,io,opts,temp;
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	//add raw, this is not by all
	int extract_opt = 0,fd=-1;
	rb_scan_args(argc, argv, "03", &name,&io,&opts);
	if(rb_obj_is_kind_of(name,rb_cHash)){
		opts = name;name = Qnil;
	}
	if(rb_obj_is_kind_of(io,rb_cHash)){
		opts = io;io = Qnil;
	}
	if(rb_obj_is_kind_of(io,rb_cIO))
		fd = NUM2INT(rb_funcall(io,rb_intern("fileno"),0));
	
	
	if(rb_obj_is_kind_of(opts,rb_cHash))
		if(RTEST(temp=rb_hash_aref(opts,ID2SYM(rb_intern("extract")))))
			extract_opt = NUM2INT(temp);
			
	if(Archive_read_ruby(self,a)==ARCHIVE_OK){
		try{
			if(NIL_P(name)){
				if(!NIL_P(io)){
					rb_raise(rb_eArgError,"You can't extract more than 1 entry into an IO-like object!");
				}
				while(archive_read_next_header(a, &entry) == ARCHIVE_OK){
					archive_read_extract(a,entry,extract_opt);
					rb_ary_push(result,rb_str_new2(archive_entry_pathname(entry)));
				}
			}else{
				while(archive_read_next_header(a, &entry) == ARCHIVE_OK){
					if(RubyArchive::match_entry(entry,name)){
						extract_extract(a,entry,extract_opt,io,fd);
						rb_ary_push(result,rb_str_new2(archive_entry_pathname(entry)));
					}
				}
			}
		}catch (...){
			rb_raise(rb_eArchiveError,"error:%d:%s",archive_errno(a),archive_error_string(a));
		}
		archive_read_finish(a);
	}
	return result;
}

VALUE Archive_extract_if_block(struct extract_obj * data)
{
	VALUE result = rb_ary_new();
	struct archive_entry *entry;
	while(archive_read_next_header(data->archive, &entry) == ARCHIVE_OK){
			VALUE str = rb_str_new2(archive_entry_pathname(entry));
			if(RTEST(rb_yield(wrap(entry)))){
				archive_read_extract(data->archive,entry,data->extract_opt);
				rb_ary_push(result,str);
			}
		}
	return result;
}
/*
 * call-seq:
 *   extract_if( [ opt ] ) {|entry| } → an_array
 *   extract_if( [ opt ] )            → an_enumerator
 * 
 * Yields each entry in the archive to the block and extracts only those 
 * entries (to the current directory) for which the block evaluates to a 
 * truth value. 
 * ===Parameters
 * [opt] is the same option hash that you can pass to #extract. 
 * ====Parameters for the option hash
 * See the #extract method for explanation. 
 * ===Return value
 * The paths of all extracted entries if a block was given, an Enumerator 
 * otherwise. 
*/

VALUE Archive_extract_if(int argc, VALUE *argv, VALUE self)
{
	RETURN_ENUMERATOR(self,argc,argv);
	VALUE opts,temp;
	struct archive *a = archive_read_new();
	
	int extract_opt=0;
	
	rb_scan_args(argc, argv, "01", &opts);
	if(rb_obj_is_kind_of(opts,rb_cHash))
		if(RTEST(temp=rb_hash_aref(opts,ID2SYM(rb_intern("extract")))))
			extract_opt = NUM2INT(temp);
	
	if(Archive_read_ruby(self,a)==ARCHIVE_OK){
		extract_obj obj;
		obj.archive = a;
		obj.extract_opt = extract_opt;
		return RB_ENSURE(Archive_extract_if_block,&obj,Archive_read_block_ensure,a);
	}
	return Qnil;
}

/*
 * call-seq:
 *   format() → an_integer or nil
 * 
 * Returns the archive format as an integer. You should use #format_name 
 * instead. 
 * ===Return value
 * An integer or nil if the format wasn't detectable. 
*/

VALUE Archive_format(VALUE self)
{
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	VALUE result = Qnil;
	
	if(Archive_read_ruby(self,a)==ARCHIVE_OK){
		archive_read_next_header(a, &entry);
		result = INT2NUM(archive_format(a));
		archive_read_finish(a);
	}
	return result;
}

/*
 * call-seq:
 *   format_name() → a_string or nil
 * 
 * Returns the archive format's name as a string.
 * ===Return value
 * A string or nil if the format wasn't detectable. 
 * ===Example
 *   a.format_name #=> "GNU tar format"
*/
 
VALUE Archive_format_name(VALUE self)
{
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	const char* name = NULL;
	
	if(Archive_read_ruby(self,a)==ARCHIVE_OK){
		if(archive_read_next_header(a, &entry)==ARCHIVE_OK){
			name = archive_format_name(a);
			archive_read_finish(a);
		}
	}
	return name ? rb_str_new2(name) : Qnil;
}










////key is the entry and val is

////TODO: hier nochmal ein ensure rein damit es das file wieder zu macht?
//int Archive_add_hash_block(VALUE key,VALUE val,struct add_obj *obj){
//	char buff[8192];
//	size_t bytes_read;
//	int fd=-1;
//	obj->file = archive_read_disk_new();
//	archive_read_disk_set_standard_lookup(obj->file);
//	struct archive_entry *entry = archive_entry_new();
//	archive_entry_copy_pathname(entry, StringValueCStr(key));
//	if(rb_obj_is_kind_of(val,rb_cFile)){
//		VALUE pathname = rb_funcall(val,rb_intern("path"),0); //source path
//		VALUE obj2 = rb_file_s_expand_path(1,&pathname);
//		archive_entry_copy_sourcepath(entry, StringValueCStr(obj2));
//		fd = NUM2INT(rb_funcall(val,rb_intern("fileno"),0));
//	}else if(rb_obj_is_kind_of(val,rb_cIO)){
//		fd = NUM2INT(rb_funcall(val,rb_intern("fileno"),0));
//	}else if(rb_respond_to(val,rb_intern("read"))){
//		//stringio has neigther path or fileno, so do nothing
//	}else {
//		VALUE obj2 = rb_file_s_expand_path(1,&val);
//		archive_entry_copy_sourcepath(entry, StringValueCStr(obj2));
//		fd = open(StringValueCStr(obj2), O_RDONLY);
//		if (fd < 0) //TODO: add error
//		{
//			return 0;
//		}
//	}
//	if(fd > 0)
//		archive_read_disk_entry_from_file(obj->file, entry, fd, NULL);
//	if(rb_block_given_p()){
//		VALUE temp = wrap(entry);
//		VALUE result = rb_yield(temp);
//		if(rb_obj_is_kind_of(result,rb_cArchiveEntry))
//			entry = wrap<archive_entry*>(result);
//		else
//			entry = wrap<archive_entry*>(temp);
//	} 
//	for(unsigned int i = 0;i < obj->entries->size();)
//	{
//		if(std::string(archive_entry_pathname(obj->entries->at(i))).compare(archive_entry_pathname(entry)) == 0){
//			obj->entries->erase(obj->entries->begin()+i);
//			obj->allbuff->erase(obj->allbuff->begin()+i);
//		}else
//			i++;
//	}
//	std::string strbuff;
//	if(fd < 0 and rb_respond_to(val,rb_intern("read"))){
//		VALUE result = rb_funcall(val,rb_intern("read"),0);
//		strbuff.append(StringValueCStr(result), RSTRING_LEN(result));
//	}else{
//		while ((bytes_read = read(fd, buff, sizeof(buff))) > 0)
//			strbuff.append(buff, bytes_read);
//	}
//	obj->entries->push_back(entry);
//	obj->allbuff->push_back(strbuff);
//	if(fd >= 0 and !rb_obj_is_kind_of(val,rb_cIO))
//		close(fd);
//	archive_read_finish(obj->file);
//	return 0;
//}

//VALUE Archive_add_block(struct add_obj *obj )
//{
//	VALUE robj = obj->obj;
//	char buff[8192];
//	size_t bytes_read;
//	archive_read_disk_set_standard_lookup(obj->file);
//	if(rb_obj_is_kind_of(robj,rb_cArray)){
//		for(int i=0;i< RARRAY_LEN(robj);i++){
//			VALUE temp = RARRAY_PTR(robj)[i];
//			int fd = -1;
//			char *sourcepath,*pathname;
//			if(rb_obj_is_kind_of(temp,rb_cFile)){
//				VALUE rpath = rb_funcall(temp,rb_intern("path"),0); //source path
//				pathname = StringValueCStr(rpath);
//				VALUE obj2 = rb_file_s_expand_path(1,&rpath);
//				sourcepath = StringValueCStr(obj2);
//				fd = NUM2INT(rb_funcall(temp,rb_intern("fileno"),0));
//			}else{
//				VALUE obj2 = rb_file_s_expand_path(1,&temp);
//				sourcepath = pathname = StringValueCStr(obj2);
//				fd = open(pathname, O_RDONLY);
//				if (fd < 0) //TODO: add error
//					return Qnil;
//			}
//			struct archive_entry *entry = archive_entry_new();
//			archive_entry_copy_sourcepath(entry, sourcepath);
//			archive_entry_copy_pathname(entry, pathname);
//			archive_read_disk_entry_from_file(obj->file, entry, fd, NULL);
//			if(rb_block_given_p()){
//				VALUE temp = wrap(entry);
//				VALUE result = rb_yield(temp);
//				if(rb_obj_is_kind_of(result,rb_cArchiveEntry))
//					entry = wrap<archive_entry*>(result);
//				else
//					entry = wrap<archive_entry*>(temp);
//			}
//			std::string strbuff;
//			while ((bytes_read = read(fd, buff, sizeof(buff))) > 0)
//				strbuff.append(buff, bytes_read);
//			if(fd >= 0 and !rb_obj_is_kind_of(robj,rb_cIO))
//				close(fd);
//			obj->entries->push_back(entry);
//			obj->allbuff->push_back(strbuff);
//		}
//	}else if(rb_obj_is_kind_of(robj,rb_cHash)){
//		archive_read_finish(obj->file);
//		rb_hash_foreach(robj,(int (*)(...))Archive_add_hash_block,(VALUE)obj);
//	}else
//	{
//		if(obj->fd > 0)
//			archive_read_disk_entry_from_file(obj->file, obj->entry, obj->fd, NULL);

//		if(rb_block_given_p()){
//			VALUE temp = wrap(obj->entry);
//			VALUE result = rb_yield(temp);
//			if(rb_obj_is_kind_of(result,rb_cArchiveEntry))
//				obj->entry = wrap<archive_entry*>(result);
//			else
//				obj->entry = wrap<archive_entry*>(temp);
//		}
//		for(unsigned int i = 0;i < obj->entries->size();)
//		{
//			if(std::string(archive_entry_pathname(obj->entries->at(i))).compare(archive_entry_pathname(obj->entry)) == 0){
//				obj->entries->erase(obj->entries->begin()+i);
//				obj->allbuff->erase(obj->allbuff->begin()+i);
//			}else
//				i++;
//		}
//		std::string strbuff;
//		if(obj->fd < 0 and rb_respond_to(robj,rb_intern("read"))){
//			VALUE result = rb_funcall(robj,rb_intern("read"),0);
//			strbuff.append(StringValueCStr(result), RSTRING_LEN(result));
//		}else{
//			while ((bytes_read = read(obj->fd, buff, sizeof(buff))) > 0)
//				strbuff.append(buff, bytes_read);
//		}
//		obj->entries->push_back(obj->entry);
//		obj->allbuff->push_back(strbuff);
//	}
//	return Qnil;
//}

VALUE Archive_add_block(struct add_obj *obj )
{
	RubyArchive::read_data_from_path2(obj->obj,Qnil,*obj->data);
	return Qnil;
}

VALUE Archive_add_block_ensure(struct add_obj *obj )
{
//	if(!rb_obj_is_kind_of(obj->obj,rb_cHash))
//		archive_read_finish(obj->file);
		
	size_t size = obj->data->size();
	for(unsigned int i=0; i<size; i++){
		archive_write_header(obj->archive,obj->data->at(i).first);
		archive_write_data(obj->archive,&obj->data->at(i).second[0],obj->data->at(i).second.length());
		archive_write_finish_entry(obj->archive);
	}
	archive_write_finish(obj->archive);
	return Qnil;
}

/*
 * call-seq:
 *   add( obj [, path] ) → self
 *   add( obj [, path] ) {|entry| } → self
 *   add( [obj, ... ] ) → self
 *   add( [obj, ... ] ) {|entry| } → self
 *   add( { path => obj} ) → self
 *   add( { path => obj} ) {|entry| } → self
 *
 * Adds a file to an archive.
 * ===Parameters
 * [obj]  String, IO, File or an object which responds to +read+. 
 * [path] Sets the file name inside the archive. 
 * ===Return value
 * self
 * ===Raises
 * [FormatError] Raised if the archive format is not supported for writing. 
*/

VALUE Archive_add(int argc, VALUE *argv, VALUE self)//(VALUE self,VALUE obj,VALUE name)
{	
	VALUE obj,name;
	rb_scan_args(argc, argv, "11", &obj,&name);
	if(NIL_P(name)){
		if(rb_obj_is_kind_of(obj,rb_cFile))
			name = rb_funcall(name,rb_intern("path"),0);
		else if(rb_obj_is_kind_of(obj,rb_cIO))
			rb_scan_args(argc, argv, "20", &obj,&name);
		else if(rb_respond_to(obj,rb_intern("read")))
			rb_scan_args(argc, argv, "20", &obj,&name);
		else if(rb_obj_is_kind_of(obj,rb_cArray) or rb_obj_is_kind_of(obj,rb_cHash))
			rb_scan_args(argc, argv, "10", &obj);
		else
			name = obj;
	}

	struct archive *a = archive_read_new(),*b=archive_write_new();
	
	DataVector entries;
	IntVector filters;
	

	int format= ARCHIVE_FORMAT_EMPTY;

	
	//autodetect format and compression
	if(Archive_read_ruby(self,a)==ARCHIVE_OK){
		RubyArchive::read_old_data(a, entries, format, filters);
	}
	
//	if(rb_obj_is_kind_of(obj,rb_cFile)){
//		VALUE pathname = rb_funcall(obj,rb_intern("path"),0); //source path
//		VALUE obj2 = rb_file_s_expand_path(1,&pathname);
//		path = StringValueCStr(obj2);
//		fd = NUM2INT(rb_funcall(obj,rb_intern("fileno"),0));
//	}else if(rb_obj_is_kind_of(obj,rb_cIO)){
//		fd = NUM2INT(rb_funcall(obj,rb_intern("fileno"),0));
//	}else if(rb_respond_to(obj,rb_intern("read")) or rb_obj_is_kind_of(obj,rb_cArray) or rb_obj_is_kind_of(obj,rb_cHash)){
//		//stringio has neigther path or fileno, so do nothing
//	}else {
//		if(RTEST(rb_funcall(rb_cFile,rb_intern("directory?"),1,obj)))
//			obj = rb_funcall(rb_cDir,rb_intern("glob"),1,rb_str_new2("**/**/*"));
//		else{
//			VALUE obj2 = rb_file_s_expand_path(1,&obj);
//			path = StringValueCStr(obj2);
//			fd = open(path, O_RDONLY);
//			if (fd < 0) //TODO: add error
//				return self;
//		}
//	}
//	Archive_format_from_path(self,format,compression);
	
	if(format == ARCHIVE_FORMAT_EMPTY){
		Archive_format_from_path(self,format,filters);
	}
	
	if(Archive_write_ruby(self,b,format,filters)==ARCHIVE_OK){
		
		add_obj temp;
		temp.archive = b;
		temp.obj = obj;
		//temp.path = path;
		temp.name = name;
		temp.data = &entries;
		RB_ENSURE(Archive_add_block,&temp,Archive_add_block_ensure,&temp);
	}
	
	return self;
}

/*
 * call-seq:
 *   << obj → self
 * 
 * Adds a file to an archive. Basicly the same as #add, but you can't 
 * set the path inside the archive. 
 * ===Parameters
 * [obj] String or File
 * ===Return value
 * self
 * ===Raises
 * [FormatError] The archive format is not supported for writing. 
*/
VALUE Archive_add_shift(VALUE self,VALUE name)
{
	return Archive_add(1,&name,self);
}

/*
 * call-seq:
 *   delete( name ) → an_array
 * 
 * Delete files from an archive.
 * ===Parameters
 * [name] An Archive::Entry, a String or a Regex.
 * ===Return value
 * A list of paths removed from the archive. 
 * ===Raises
 * raise TypeError if the parameter is neigther String or File.
 * raise Error if the format has no write support 
*/

VALUE Archive_delete(VALUE self,VALUE val)
{
	struct archive *a = archive_read_new(),*b=archive_write_new();
	int format = ARCHIVE_FORMAT_EMPTY;
	
	DataVector entries;
	IntVector filters;
	
	VALUE result = rb_ary_new();
	
	//autodetect format and compression
	if(Archive_read_ruby(self,a)==ARCHIVE_OK){
		RubyArchive::read_old_data(a, entries, format, filters);

		for(unsigned int i=0; i<entries.size();){
			if(RubyArchive::match_entry(entries[i].first,val)){
				rb_ary_push(result,rb_str_new2(archive_entry_pathname(entries[i].first)));
				entries.erase(entries.begin() + i);
			}else
				++i;
		}

		if(Archive_write_ruby(self,b,format,filters)==ARCHIVE_OK){
			//write old data back
			for(unsigned int i=0; i<entries.size(); i++){
				archive_write_header(b,entries[i].first);
				archive_write_data(b,&entries[i].second[0],entries[i].second.length());
				archive_write_finish_entry(b);
			}
			archive_write_finish(b);
		}
		
	}
	return result;
}

VALUE Archive_delete_if_block(struct write_obj * data)
{
	VALUE result= rb_ary_new();
	for(unsigned int i=0; i< data->data->size(); i++){
		if(!RTEST(rb_yield(wrap(data->data->at(i).first)))){
			archive_write_header(data->archive,data->data->at(i).first);
			archive_write_data(data->archive,&data->data->at(i).second[0],data->data->at(i).second.length());
			archive_write_finish_entry(data->archive);
		}else
			rb_ary_push(result,wrap(data->data->at(i).first));
	}
	return result;
}

/*
 * call-seq:
 *   archive.delete_if {|entry| } -> self
 *   archive.delete_if -> Enumerator
 * 
 * Yields each entry in the archive to the block and deletes those for which 
 * the block evaluates to a truth value. 
 * ===Parameters
 * [name] An Archive::Entry, a String or a Regex.
 * ===Return value
 * If a block was given, returns self, otherwise an Enumerator. 
 * ===Raises
 * raise Error if the format has no write support 
*/

VALUE Archive_delete_if(VALUE self)
{
	RETURN_ENUMERATOR(self,0,NULL);

	struct archive *a = archive_read_new(),*b=archive_write_new();
	int format = ARCHIVE_FORMAT_EMPTY;
	
	DataVector entries;
	IntVector filters;
	
	if(Archive_read_ruby(self,a)==ARCHIVE_OK){
		RubyArchive::read_old_data(a, entries, format, filters);
		
		if(Archive_write_ruby(self,b,format,filters)==ARCHIVE_OK){
			write_obj obj;
			obj.archive = b;
			obj.data = &entries;
			return RB_ENSURE(Archive_delete_if_block,&obj,Archive_write_block_ensure,b);
		}	
	}	
	return Qnil;
}

/*
 * call-seq:
 *   archive.clear -> self
 * 
 * Deletes all files from an archive. 
 * ===Return value
 * returns self.
 * ===Raises
 * raise Error if the format has no write support 
*/

VALUE Archive_clear(VALUE self)
{

	struct archive *a = archive_read_new(),*b=archive_write_new();
	struct archive_entry *entry;
	int format = ARCHIVE_FORMAT_EMPTY;
	
	if(Archive_read_ruby(self,a)==ARCHIVE_OK){
		archive_read_next_header(a, &entry);
		format = archive_format(a);
		IntVector filters;
		RubyArchive::read_get_filters(a,filters);
		archive_read_finish(a);
		
		if(Archive_write_ruby(self,b,format,filters)==ARCHIVE_OK)
			archive_write_finish(b);
	}
	
	return self;
}

/*:nodoc:
 * call-seq:
 *   archive.exist? -> true or false
 * 
 * Same as 
 *  File.exist?(archive.path)
 * . Checks wheather or not the archive file is existant. 
 * ===Return value
 * True or false. 
*/

VALUE Archive_exist(VALUE self)
{
	return rb_funcall(rb_cFile,rb_intern("exist?"),1,Archive_path(self));
}

/*:nodoc:
 * call-seq:
 * archive.unlink -> self
 * 
 * call the File.unlink(path)
*/

VALUE Archive_unlink(VALUE self)
{
	return rb_funcall(rb_cFile,rb_intern("unlink"),1,Archive_path(self));
	return self;
}

/*:nodoc:
 * call-seq:
 * archive.mtime -> Time
 * 
 * call the File.mtime(path)
*/

VALUE Archive_mtime(VALUE self)
{
	return rb_funcall(rb_cFile,rb_intern("mtime"),1,Archive_path(self));
}

/*:nodoc:
 * call-seq:
 * archive.atime -> Time
 * 
 * call the File.atime(path)
*/

VALUE Archive_atime(VALUE self)
{
	return rb_funcall(rb_cFile,rb_intern("atime"),1,Archive_path(self));
}

/*:nodoc:
 * call-seq:
 * archive.ctime -> Time
 * 
 * call the File.ctime(path)
*/


VALUE Archive_ctime(VALUE self)
{
	return rb_funcall(rb_cFile,rb_intern("ctime"),1,Archive_path(self));
}

/*:nodoc:
 * call-seq:
 * archive.stat -> File::Stat
 * 
 * call the File.stat(path)
*/

VALUE Archive_stat(VALUE self)
{
	return rb_funcall(rb_cFile,rb_intern("stat"),1,Archive_path(self));
}

/*
 * call-seq:
 *   archive.inspect -> String
 * 
 * Human-readable description. 
 * ===Return value
 * String
*/
 
VALUE Archive_inspect(VALUE self)
{
	VALUE array[3];
	switch(_self->type){
	case archive_path:
		array[0]=rb_str_new2("#<%s:%s>");
		array[1]=rb_class_of(self);	
		array[2]=Archive_path(self);
		break;
	case archive_fd:
		array[0]=rb_str_new2("#<%s:%d>");
		array[1]=rb_class_of(self);	
		array[2]=INT2NUM(_self->fd);
		break;
	case archive_buffer: 
		break;
	case archive_ruby:
		array[0]=rb_str_new2("#<%s:%s>");
		array[1]=rb_class_of(self);	
		array[2]=_self->ruby;
		break;
	}	
	return rb_f_sprintf(3,array);
}

/*
 * Document-class: Archive::Error
 * This is the superclass of all errors specific to this library. 
*/

/*
 * Document-class: Archive::Error::Compression
 * This exception is thrown if you try to use an unknown compression format. 
*/

/*
 * Document-class: Archive::Error::Format
 * This exception is thrown if you try to use an unknown archive format or libarchive doesn't 
 * have write support for the format you wanted to write. 
*/

/*
 * Document-class: Archive
 * 
 * This class represents an archive file. The file may or may not exist, 
 * depending on wheather you want to create a new archive or read from 
 * an existing one. When instanciating this class, libarchive-ruby will 
 * automatically detect the correct file format for you using libarchive's 
 * own detection mechanism if the archive file is already present, otherwise 
 * by looking at the archive file's file extension. 
*/ 
/*
 * Document-const: EXTRACT_TIME
 * 
 * extract the atime and mtime
*/
/*
 * Document-const: EXTRACT_PERM
 * 
 * extract the permission
*/
/*
 * Document-const: EXTRACT_OWNER
 * 
 * extract the owner
*/

/*
 * Document-const: EXTRACT_ACL
 * 
 * extract the access control list
*/
/*
 * Document-const: EXTRACT_FFLAGS
 * 
 * extract the fflags
*/
/*
 * Document-const: EXTRACT_XATTR
 * 
 * extract the extended information
*/
extern "C" void Init_archive(void){
	rb_cArchive = rb_define_class("Archive",rb_cObject);
	rb_define_alloc_func(rb_cArchive,Archive_alloc);
	rb_define_method(rb_cArchive,"initialize",RUBY_METHOD_FUNC(Archive_initialize),-1);
	rb_define_private_method(rb_cArchive,"initialize_copy",RUBY_METHOD_FUNC(Archive_initialize_copy),1);
	rb_define_method(rb_cArchive,"path",RUBY_METHOD_FUNC(Archive_path),0);

	rb_define_method(rb_cArchive,"each",RUBY_METHOD_FUNC(Archive_each),0);
	rb_define_method(rb_cArchive,"each_entry",RUBY_METHOD_FUNC(Archive_each_entry),0);
	rb_define_method(rb_cArchive,"each_data",RUBY_METHOD_FUNC(Archive_each_data),0);
	
	rb_define_method(rb_cArchive,"each_filter",RUBY_METHOD_FUNC(Archive_each_filter),0);
	
	rb_define_method(rb_cArchive,"map!",RUBY_METHOD_FUNC(Archive_map_self),0);
	rb_define_alias(rb_cArchive,"collect!","map!");
	rb_define_method(rb_cArchive,"[]",RUBY_METHOD_FUNC(Archive_get),1);
	
	rb_define_method(rb_cArchive,"to_hash",RUBY_METHOD_FUNC(Archive_to_hash),0);
	
	rb_define_method(rb_cArchive,"extract",RUBY_METHOD_FUNC(Archive_extract),-1);
	rb_define_method(rb_cArchive,"extract_if",RUBY_METHOD_FUNC(Archive_extract_if),-1);
	
	rb_define_method(rb_cArchive,"delete",RUBY_METHOD_FUNC(Archive_delete),1);
	rb_define_method(rb_cArchive,"delete_if",RUBY_METHOD_FUNC(Archive_delete_if),0);
	rb_define_method(rb_cArchive,"clear",RUBY_METHOD_FUNC(Archive_clear),0);
	
	//rb_define_method(rb_cArchive,"move_to",RUBY_METHOD_FUNC(Archive_move_to),1);

	rb_define_method(rb_cArchive,"add",RUBY_METHOD_FUNC(Archive_add),-1);
	rb_define_method(rb_cArchive,"<<",RUBY_METHOD_FUNC(Archive_add_shift),1);
	
	rb_define_method(rb_cArchive,"inspect",RUBY_METHOD_FUNC(Archive_inspect),0);
	
	rb_define_method(rb_cArchive,"format",RUBY_METHOD_FUNC(Archive_format),0);
	//rb_define_method(rb_cArchive,"compression",RUBY_METHOD_FUNC(Archive_compression),0);
	rb_define_method(rb_cArchive,"format_name",RUBY_METHOD_FUNC(Archive_format_name),0);
	//rb_define_method(rb_cArchive,"compression_name",RUBY_METHOD_FUNC(Archive_compression_name),0);

	rb_define_method(rb_cArchive,"unlink",RUBY_METHOD_FUNC(Archive_unlink),0);
	rb_define_method(rb_cArchive,"exist?",RUBY_METHOD_FUNC(Archive_exist),0);
	rb_define_method(rb_cArchive,"mtime",RUBY_METHOD_FUNC(Archive_mtime),0);
	rb_define_method(rb_cArchive,"atime",RUBY_METHOD_FUNC(Archive_atime),0);
	rb_define_method(rb_cArchive,"ctime",RUBY_METHOD_FUNC(Archive_ctime),0);
	rb_define_method(rb_cArchive,"stat",RUBY_METHOD_FUNC(Archive_stat),0);
	
	rb_include_module(rb_cArchive,rb_mEnumerable);
	//rb_define_method(rb_cArchive,"size",RUBY_METHOD_FUNC(Archive_size),0);
	
	Init_archive_entry(rb_cArchive);
	
	rb_define_const(rb_cArchive,"EXTRACT_OWNER",INT2NUM(ARCHIVE_EXTRACT_OWNER));
	rb_define_const(rb_cArchive,"EXTRACT_PERM",INT2NUM(ARCHIVE_EXTRACT_PERM));
	rb_define_const(rb_cArchive,"EXTRACT_TIME",INT2NUM(ARCHIVE_EXTRACT_TIME));
	rb_define_const(rb_cArchive,"EXTRACT_NO_OVERWRITE",INT2NUM(ARCHIVE_EXTRACT_NO_OVERWRITE));
	rb_define_const(rb_cArchive,"EXTRACT_UNLINK",INT2NUM(ARCHIVE_EXTRACT_UNLINK));
	rb_define_const(rb_cArchive,"EXTRACT_ACL",INT2NUM(ARCHIVE_EXTRACT_ACL));
	rb_define_const(rb_cArchive,"EXTRACT_FFLAGS",INT2NUM(ARCHIVE_EXTRACT_FFLAGS));
	rb_define_const(rb_cArchive,"EXTRACT_XATTR",INT2NUM(ARCHIVE_EXTRACT_XATTR));
	rb_define_const(rb_cArchive,"EXTRACT_SECURE_SYMLINKS",INT2NUM(ARCHIVE_EXTRACT_SECURE_SYMLINKS));
	rb_define_const(rb_cArchive,"EXTRACT_SECURE_NODOTDOT",INT2NUM(ARCHIVE_EXTRACT_SECURE_NODOTDOT));
	rb_define_const(rb_cArchive,"EXTRACT_NO_AUTODIR",INT2NUM(ARCHIVE_EXTRACT_NO_AUTODIR));
	rb_define_const(rb_cArchive,"EXTRACT_NO_OVERWRITE_NEWER",INT2NUM(ARCHIVE_EXTRACT_NO_OVERWRITE_NEWER));
	rb_define_const(rb_cArchive,"EXTRACT_SPARSE",INT2NUM(ARCHIVE_EXTRACT_SPARSE));
	rb_define_const(rb_cArchive,"EXTRACT_MAC_METADATA",INT2NUM(ARCHIVE_EXTRACT_MAC_METADATA));
	
	
	rb_eArchiveError = rb_define_class_under(rb_cArchive,"Error",rb_eStandardError);
	rb_eArchiveErrorFormat = rb_define_class_under(rb_eArchiveError,"Format",rb_eArchiveError);
	rb_eArchiveErrorCompression = rb_define_class_under(rb_eArchiveError,"Compression",rb_eArchiveError);
	
	
	SymToFormat[rb_intern("tar")]=ARCHIVE_FORMAT_TAR_GNUTAR;
	SymToFormat[rb_intern("pax")]=ARCHIVE_FORMAT_TAR_PAX_INTERCHANGE;
	SymToFormat[rb_intern("zip")]=ARCHIVE_FORMAT_ZIP;
	SymToFormat[rb_intern("ar")]=ARCHIVE_FORMAT_AR_GNU;
	SymToFormat[rb_intern("xar")]=ARCHIVE_FORMAT_XAR;
	SymToFormat[rb_intern("lha")]=ARCHIVE_FORMAT_LHA;
	SymToFormat[rb_intern("cab")]=ARCHIVE_FORMAT_CAB;
	SymToFormat[rb_intern("rar")]=ARCHIVE_FORMAT_RAR;
	SymToFormat[rb_intern("7zip")]=ARCHIVE_FORMAT_7ZIP;
	
	
	SymToFilter[rb_intern("gzip")]=ARCHIVE_FILTER_GZIP;
	SymToFilter[rb_intern("bzip2")]=ARCHIVE_FILTER_BZIP2;
	SymToFilter[rb_intern("compress")]=ARCHIVE_FILTER_BZIP2;
	SymToFilter[rb_intern("lzma")]=ARCHIVE_FILTER_LZMA;
	SymToFilter[rb_intern("xz")]=ARCHIVE_FILTER_XZ;
	SymToFilter[rb_intern("uu")]=ARCHIVE_FILTER_UU;
	SymToFilter[rb_intern("rpm")]=ARCHIVE_FILTER_RPM;
	SymToFilter[rb_intern("lzip")]=ARCHIVE_FILTER_LZIP;
	
#ifdef ARCHIVE_FILTER_LZOP
	SymToFilter[rb_intern("lzop")]=ARCHIVE_FILTER_LZOP;
#endif
#ifdef ARCHIVE_FILTER_GRZIP
	SymToFilter[rb_intern("grzip")]=ARCHIVE_FILTER_GRZIP;
#endif

	
	FileExtType::mapped_type pair;
	
	pair = std::make_pair(ARCHIVE_FORMAT_TAR,ARCHIVE_FILTER_GZIP);
	fileExt[".tar.gz"]=pair;
	fileExt[".tgz"]=pair;
	
	pair = std::make_pair(ARCHIVE_FORMAT_TAR,ARCHIVE_FILTER_BZIP2);
	fileExt[".tar.bz2"]=pair;
	fileExt[".tbz"]=pair;
	fileExt[".tb2"]=pair;
	
	pair = std::make_pair(ARCHIVE_FORMAT_TAR,ARCHIVE_FILTER_XZ);
	fileExt[".tar.xz"]=pair;
	fileExt[".txz"]=pair;
	
	fileExt[".tar.lzma"]=std::make_pair(ARCHIVE_FORMAT_TAR,ARCHIVE_FILTER_LZMA);
	
	fileExt[".tar"]=std::make_pair(ARCHIVE_FORMAT_TAR,ARCHIVE_FILTER_NONE);
	
	
//	DataVector data;
//	//RubyArchive::read_data_from_path2(rb_reg_new(".",8,0),Qnil,data);
//	RubyArchive::read_data_from_path2(rb_str_new2("."),Qnil,data);
//	rb_warn("%lu",data.size());
////	
//	for(size_t i = 0; i < data.size(); ++i)
//	{
//		rb_warn("%s",archive_entry_pathname(data[i].first));
//		rb_warn("%lu",data[i].second.size());
//	}
}
