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
#include <exception>
#include <stdexcept> 
#define _self wrap<rarchive*>(self)
#define RB_ENSURE(func1,obj1,func2,obj2)\
 rb_ensure(RUBY_METHOD_FUNC(func1),(VALUE)obj1,RUBY_METHOD_FUNC(func2),(VALUE)obj2)

//TODO: handle string IO as archive Archive.new(stringio)


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
	std::vector<struct archive_entry *> *entries;
	std::vector<std::string> *allbuff;
};


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
ssize_t
rubymyread(struct archive *a, void *client_data, const void **buff)
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


void Archive_format_from_path(VALUE self,int &format,int &compression)
{
	if(format == ARCHIVE_FORMAT_EMPTY){
		if(_self->type == archive_path){ 
			std::string selfpath = _self->path;
			if(selfpath.substr(selfpath.length()-7).compare(".tar.xz")==0 || selfpath.substr(selfpath.length()-4).compare(".txz")==0 ){
				format=ARCHIVE_FORMAT_TAR_USTAR;
				compression=ARCHIVE_COMPRESSION_XZ;
			}else if(selfpath.substr(selfpath.length()-9).compare(".tar.lzma")==0 ){
				format=ARCHIVE_FORMAT_TAR_USTAR;
				compression=ARCHIVE_COMPRESSION_LZMA;
			}else if(selfpath.substr(selfpath.length()-7).compare(".tar.gz")==0 || selfpath.substr(selfpath.length()-4).compare(".tgz")==0){
				format=ARCHIVE_FORMAT_TAR_USTAR;
				compression=ARCHIVE_COMPRESSION_GZIP;
			}else if(selfpath.substr(selfpath.length()-8).compare(".tar.bz2")==0 || selfpath.substr(selfpath.length()-4).compare(".tbz")==0 || selfpath.substr(selfpath.length()-4).compare(".tb2")==0){
				format=ARCHIVE_FORMAT_TAR_USTAR;
				compression=ARCHIVE_COMPRESSION_BZIP2;
			}else if(selfpath.substr(selfpath.length()-4).compare(".tar")==0)
				format=ARCHIVE_FORMAT_TAR_USTAR;
		}
	}
	if(format==ARCHIVE_FORMAT_TAR_GNUTAR)
		format=ARCHIVE_FORMAT_TAR_USTAR;
}

int Archive_read_ruby(VALUE self,struct archive *data)
{
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



int Archive_write_ruby(VALUE self,struct archive *data)
{
	int error =0;
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

int Archive_write_set_compression(struct archive *data,int compression)
{
	int error = 0;
	switch(compression){
		case ARCHIVE_COMPRESSION_NONE:
			error = archive_write_set_compression_none(data);
			break;
		case ARCHIVE_COMPRESSION_GZIP:
			error = archive_write_set_compression_gzip(data);
			break;
		case ARCHIVE_COMPRESSION_BZIP2:
			error = archive_write_set_compression_bzip2(data);
			break;
		case ARCHIVE_COMPRESSION_COMPRESS:
			error = archive_write_set_compression_compress(data);
			break;
		case ARCHIVE_COMPRESSION_LZMA:
			error = archive_write_set_compression_lzma(data);
			break;
		case ARCHIVE_COMPRESSION_XZ:
			error = archive_write_set_compression_xz(data);
			break;
		default:
			rb_raise(rb_eArchiveErrorCompression,"unsupported compresstype");
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
	VALUE path,r_format,r_compression;
	rb_scan_args(argc, argv, "12", &path,&r_format,&r_compression);
	if(rb_obj_is_kind_of(path,rb_cIO)){
		_self->fd = NUM2INT(rb_funcall(path,rb_intern("fileno"),0));
		_self->type = archive_fd;
		rb_scan_args(argc, argv, "21", &path,&r_format,&r_compression);
	}else if(rb_respond_to(path,rb_intern("read"))){
		_self->ruby = path;
		_self->type = archive_ruby;
		rb_scan_args(argc, argv, "21", &path,&r_format,&r_compression);
	}else{
		path = rb_file_s_expand_path(1,&path);
		_self->path = std::string(rb_string_value_cstr(&path));
		_self->type =archive_path;
	}
	
	//to set the format the file must convert
	if(!NIL_P(r_format)){
	
		char buff[8192];
		//std::string selfpath =_self->path;
		size_t bytes_read;
		struct archive *a = archive_read_new(),*b=archive_write_new();
		struct archive_entry *entry;
		int format,compression,error=0;
		std::vector<struct archive_entry *> entries;
		std::vector<std::string> allbuff;
		if(rb_obj_is_kind_of(r_format,rb_cSymbol)!=Qtrue)
			rb_raise(rb_eTypeError,"exepted Symbol");
		if(SYM2ID(r_format) == rb_intern("tar"))
			format = ARCHIVE_FORMAT_TAR_GNUTAR;
		else if(SYM2ID(r_format) == rb_intern("zip"))
			format = ARCHIVE_FORMAT_ZIP;
		else if(SYM2ID(r_format) == rb_intern("ar"))
			format = ARCHIVE_FORMAT_AR_GNU;
		else if(SYM2ID(r_format) == rb_intern("xar"))
			format = ARCHIVE_FORMAT_XAR;
		else
			rb_raise(rb_eTypeError,"wrong format");
		
		if(NIL_P(r_compression)){
			compression = ARCHIVE_COMPRESSION_NONE;
			error = archive_write_set_compression_none(b);
		}else if(SYM2ID(r_compression) == rb_intern("gzip")){
			compression =ARCHIVE_COMPRESSION_GZIP;
			error = archive_write_set_compression_gzip(b);
		}else if(SYM2ID(r_compression) == rb_intern("bzip2")){
			compression =ARCHIVE_COMPRESSION_BZIP2;
			error = archive_write_set_compression_bzip2(b);
		}else if(SYM2ID(r_compression) == rb_intern("compress")){
			compression =ARCHIVE_COMPRESSION_BZIP2;
			error = archive_write_set_compression_compress(b);
		}else if(SYM2ID(r_compression) == rb_intern("lzma")){
			compression =ARCHIVE_COMPRESSION_LZMA;
			error = archive_write_set_compression_lzma(b);
		}else if(SYM2ID(r_compression) == rb_intern("xz")){
			compression =ARCHIVE_COMPRESSION_XZ;
			error = archive_write_set_compression_xz(b);
		}else
			rb_raise(rb_eTypeError,"unsupported compression");
		
		if(error)
			rb_raise(rb_eArchiveError,"error (%d): %s ",archive_errno(b),archive_error_string(b));
			
		archive_read_support_compression_all(a);
		archive_read_support_format_all(a);
		archive_read_support_format_raw(a);
		//autodetect format and compression
		error=Archive_read_ruby(self,a);
		_self->format = format;
		_self->compression = compression;
		if(error==ARCHIVE_OK){
			while(archive_read_next_header(a, &entry)==ARCHIVE_OK){
				if(format==archive_format(a) && compression==archive_compression(a)){
					archive_read_finish(a);
					return self;
				}
				entries.push_back(archive_entry_clone(entry));
				allbuff.push_back(std::string(""));
				while ((bytes_read=archive_read_data(a,&buff,sizeof(buff)))>0)
					allbuff.back().append(buff,bytes_read);
			}
			archive_read_finish(a);
		}
		//format fix
		if(format==ARCHIVE_FORMAT_TAR_GNUTAR)
			format=ARCHIVE_FORMAT_TAR_USTAR;
		
		if((error = archive_write_set_format(b,format)) != ARCHIVE_OK)
			rb_raise(rb_eArchiveErrorFormat,"error (%d): %s ",error,archive_error_string(b));
		error=Archive_write_ruby(self,b);
		if(error==ARCHIVE_OK){
			//write old data back
			for(unsigned int i=0; i<entries.size(); i++){
				archive_write_header(b,entries[i]);
				archive_write_data(b,allbuff[i].c_str(),allbuff[i].length());
				archive_write_finish_entry(b);
			}
			archive_write_finish(b);
		}
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
	VALUE result = rb_ary_new();
	struct archive_entry *entry;
	while (archive_read_next_header(data, &entry) == ARCHIVE_OK) {
		VALUE temp = wrap(entry);
		if(rb_proc_arity(rb_block_proc())==1){
			rb_yield(temp);
			archive_read_data_skip(data);
		}else{
			char buff[8192];
			std::string str;
			size_t bytes_read;
			try {
				while ((bytes_read=archive_read_data(data,&buff,sizeof(buff)))>0)
					str.append(buff,bytes_read);
			}catch(...){
				rb_raise(rb_eArchiveError,"error:%d:%s",archive_errno(data),archive_error_string(data));
			}
			rb_yield_values(2,temp,rb_str_new2(str.c_str()));
		}
		rb_ary_push(result,temp);
	}
	return result;
}

/*
 * call-seq:
 *   each(){|entry [, data]| ... } → an_array
 *   each()                        → an_enumerator
 * 
 * Iterates through the archive and yields each entry as an Archive::Entry object. The second parameter 
 * contains the data of that entry, so you don't have to extract it only to read what's in it. 
 * ===Return value
 * If a block is given, returns an array of Archive::Entry objects, otherwise an enumerator. 
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

	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	int error=Archive_read_ruby(self,a);
	if(error==ARCHIVE_OK)
		return RB_ENSURE(Archive_each_block,a,Archive_read_block_ensure,a);
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

	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	int error=Archive_read_ruby(self,a);
	if(error==ARCHIVE_OK)
		return RB_ENSURE(Archive_each_entry_block,a,Archive_read_block_ensure,a);
	return Qnil;
}


VALUE Archive_each_data_block(struct archive *data)
{
	VALUE result = rb_ary_new();
	struct archive_entry *entry;
	while (archive_read_next_header(data, &entry) == ARCHIVE_OK) {
		char buff[8192];
		std::string str;
		size_t bytes_read;
		try{
			while ((bytes_read=archive_read_data(data,&buff,sizeof(buff)))>0)
				str.append(buff,bytes_read);
		} catch (...){
			rb_raise(rb_eArchiveError,"error:%d:%s",archive_errno(data),archive_error_string(data));
		}
		rb_yield(rb_str_new2(str.c_str()));
		rb_ary_push(result,rb_str_new2(str.c_str()));
	}
	return result;
}


/*
 * call-seq:
 *   each_data() {|data| }    → an_array
 *   each_data()              → an_enumerator
 * 
 * Iterates through the archive and yields each entry's data as a string. 
 * This is the same as #each, but doesn't allow for the first block parameter. 
 * ===Return value
 * If a block is given, returns an array of String objects, otherwise an enumerator. 
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

	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	int error=Archive_read_ruby(self,a);
	if(error==ARCHIVE_OK)
		return RB_ENSURE(Archive_each_data_block,a,Archive_read_block_ensure,a);
	return Qnil;
}


/*
 * call-seq:
 *   archive.to_hash → Hash
 * 
 * Iterates through the archive and yields each data of an entry as a string object.
 * ===Return value
 * returns Hash of Archive::Entry => Sring
*/

VALUE Archive_to_hash(VALUE self)
{
	VALUE result = rb_hash_new();
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	int error=Archive_read_ruby(self,a);
	if(error==ARCHIVE_OK){
		while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
			char buff[8192];
			std::string str;
			size_t bytes_read;
			try{
				while ((bytes_read=archive_read_data(a,&buff,sizeof(buff)))>0)
					str.append(buff,bytes_read);
			} catch (...){
				rb_raise(rb_eArchiveError,"error:%d:%s",archive_errno(a),archive_error_string(a));
			}
			rb_hash_aset(result,wrap(entry),rb_str_new2(str.c_str()));
		}
		archive_read_finish(a);
	}
	return result;
}

VALUE Archive_map_block(struct write_obj * data){
	for(unsigned int i=0; i< data->entries->size(); i++){
		VALUE temp = wrap(data->entries->at(i));
		VALUE val;
		if(rb_proc_arity(rb_block_proc())<2){
			val = rb_yield(temp);
		}else{
			val = rb_yield_values(2,temp,rb_str_new2(data->allbuff->at(i).c_str()));
		}
		VALUE entry,rdata= Qnil;
		if(rb_obj_is_kind_of(val,rb_cArray)==Qtrue){
			entry=rb_ary_entry(val,0);
			rdata=rb_ary_entry(val,1);
		}else
			entry = val;
		if(rb_obj_is_kind_of(entry,rb_cArchiveEntry)==Qtrue){
			archive_write_header(data->archive,wrap<struct archive_entry *>(val));
			
			if(rdata == Qnil)
				archive_write_data(data->archive,data->allbuff->at(i).c_str(),data->allbuff->at(i).length());
			else{
				char* buff = StringValueCStr(rdata);
				archive_write_data(data->archive,buff,strlen(buff));
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
	char buff[8192];
	std::string selfpath =_self->path;
	size_t bytes_read;
	struct archive *a = archive_read_new(),*b=archive_write_new();
	struct archive_entry *entry;
	int format = ARCHIVE_FORMAT_EMPTY,compression = ARCHIVE_COMPRESSION_NONE,error=0;
	std::vector<struct archive_entry *> entries;
	std::vector<std::string> allbuff;
	
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	//autodetect format and compression
	error=Archive_read_ruby(self,a);
	if(error==ARCHIVE_OK){
		while(archive_read_next_header(a, &entry)==ARCHIVE_OK){
			format = archive_format(a);
			compression = archive_compression(a);
			entries.push_back(archive_entry_clone(entry));
			allbuff.push_back(std::string(""));
			try{
				while ((bytes_read=archive_read_data(a,&buff,sizeof(buff)))>0)
					allbuff.back().append(buff,bytes_read);
			} catch (...){
				rb_raise(rb_eArchiveError,"error:%d:%s",archive_errno(a),archive_error_string(a));
			}
		}
		archive_read_finish(a);
		//format fix
		if(format==ARCHIVE_FORMAT_TAR_GNUTAR)
			format=ARCHIVE_FORMAT_TAR_USTAR;
		if((error = archive_write_set_format(b,format)) != ARCHIVE_OK)
			rb_raise(rb_eArchiveErrorFormat,"error (%d): %s ",error,archive_error_string(b));
		Archive_write_set_compression(b,compression);
	
		error=Archive_write_ruby(self,b);
		if(error==ARCHIVE_OK){
			write_obj obj;
			obj.archive = b;
			obj.entries = &entries;
			obj.allbuff = &allbuff;
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
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	int error=Archive_read_ruby(self,a);
	if(error==ARCHIVE_OK){
		while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
			bool find = false;
			if(rb_obj_is_kind_of(val,rb_cRegexp)==Qtrue){
				find = rb_reg_match(val,rb_str_new2(archive_entry_pathname(entry)))!=Qnil;
			}else{
				val = rb_funcall(val,rb_intern("to_s"),0);
				std::string str1(rb_string_value_cstr(&val)),str2(str1);
				str2 += '/'; // dir ends of '/'
				const char *cstr = archive_entry_pathname(entry);
				find = (str1.compare(cstr)==0 || str2.compare(cstr)==0);
			}
			if(find){
				VALUE result = wrap(entry);
				archive_read_finish(a);
				return result;
			}
		}
		archive_read_finish(a);
	}
	return Qnil;
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
	char buff[8192];
	size_t bytes_read;

	VALUE result = rb_ary_new(),name,io,opts,temp;
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);//add raw, this is not by all
	int extract_opt = 0,fd=-1,error=0;
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
			
	error=Archive_read_ruby(self,a);
	if(error==ARCHIVE_OK){
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
				if(rb_obj_is_kind_of(name,rb_cArchiveEntry)==Qtrue){
					if(rb_obj_is_kind_of(io,rb_cIO)==Qtrue){
						while(archive_read_next_header(a, &entry) == ARCHIVE_OK){
							if(std::string(archive_entry_pathname(entry)).compare(archive_entry_pathname(wrap<archive_entry*>(name)))==0)
								archive_read_data_into_fd(a,fd);
						}
					}else if(rb_respond_to(io,rb_intern("write"))){
						while(archive_read_next_header(a, &entry) == ARCHIVE_OK){
							if(std::string(archive_entry_pathname(entry)).compare(archive_entry_pathname(wrap<archive_entry*>(name)))==0)
								while ((bytes_read=archive_read_data(a,&buff,sizeof(buff)))>0)
									rb_funcall(io,rb_intern("write"),1,rb_str_new(buff,bytes_read));
							
						}
					}else
						archive_read_extract(a,wrap<archive_entry*>(name),extract_opt);
					rb_ary_push(result,rb_str_new2(archive_entry_pathname(wrap<archive_entry*>(name))));
				}else if(rb_obj_is_kind_of(name,rb_cRegexp)==Qtrue){
					while(archive_read_next_header(a, &entry) == ARCHIVE_OK){
						VALUE str = rb_str_new2(archive_entry_pathname(entry));
						if(rb_reg_match(name,str)!=Qnil){
							if(rb_obj_is_kind_of(io,rb_cIO)==Qtrue){
								archive_read_data_into_fd(a,fd);
							}else if(rb_respond_to(io,rb_intern("write"))){
								while ((bytes_read=archive_read_data(a,&buff,sizeof(buff)))>0)
										rb_funcall(io,rb_intern("write"),1,rb_str_new(buff,bytes_read));
							}else
								archive_read_extract(a,entry,extract_opt);
							rb_ary_push(result,str);
						}
					}
				}else{
					name = rb_funcall(name,rb_intern("to_s"),0);
					std::string str1(rb_string_value_cstr(&name)),str2(str1);
					str2 += '/'; // dir ends of '/'
					while(archive_read_next_header(a, &entry) == ARCHIVE_OK){
						const char *cstr = archive_entry_pathname(entry);
						if(str1.compare(cstr)==0 || str2.compare(cstr)==0){
							if(rb_obj_is_kind_of(io,rb_cIO)==Qtrue){
								archive_read_data_into_fd(a,fd);
							}else if(rb_respond_to(io,rb_intern("write"))){
								while ((bytes_read=archive_read_data(a,&buff,sizeof(buff)))>0)
										rb_funcall(io,rb_intern("write"),1,rb_str_new(buff,bytes_read));
							}else
								archive_read_extract(a,entry,extract_opt);
							rb_ary_push(result,rb_str_new2(cstr));
						}
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
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	int extract_opt=0,error=0;
	
	rb_scan_args(argc, argv, "01", &opts);
	if(rb_obj_is_kind_of(opts,rb_cHash))
		if(RTEST(temp=rb_hash_aref(opts,ID2SYM(rb_intern("extract")))))
			extract_opt = NUM2INT(temp);
	error=Archive_read_ruby(self,a);
	if(error==ARCHIVE_OK){
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
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	int error=Archive_read_ruby(self,a);
	if(error==ARCHIVE_OK){
		archive_read_next_header(a, &entry);
		result = INT2NUM(archive_format(a));
		archive_read_finish(a);
	}
	return result;
}

/*
 * call-seq:
 *   compression() → an_integer or nil
 * 
 * Returns the archive compression as an integer. You should use 
 * #compression_name instead. 
 * ===Return value
 * An integer or nil if the compression wasn't detectable. 0 means 
 * no compression. 
*/

VALUE Archive_compression(VALUE self)
{
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	VALUE result = Qnil;
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	int error=Archive_read_ruby(self,a);
	if(error==ARCHIVE_OK){
		archive_read_next_header(a, &entry);
		result = INT2NUM(archive_compression(a));
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
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	int error=Archive_read_ruby(self,a);
	if(error==ARCHIVE_OK){
		if(archive_read_next_header(a, &entry)==ARCHIVE_OK){
			name = archive_format_name(a);
			archive_read_finish(a);
		}
	}
	return name ? rb_str_new2(name) : Qnil	;
}

/*
 * call-seq:
 *   compression_name() → a_string or nil
 * 
 * Returns the archive compression's name as a string.
 * ===Return value
 * A string or nil if the compression format wasn't detectable. If there 
 * is no compression, <tt>"none"</tt> is returned. 
 * ===Example
 *   a.compression_name #=> "gzip"
 *   
 *   a.compression_name #=> "none"
*/

VALUE Archive_compression_name(VALUE self)
{
	struct archive *a = archive_read_new();
	struct archive_entry *entry;
	const char* name = NULL;
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	int error=Archive_read_ruby(self,a);
	if(error==ARCHIVE_OK){
		archive_read_next_header(a, &entry);
		name = archive_compression_name(a);
		archive_read_finish(a);
	}
	return name ? rb_str_new2(name) : Qnil	;
}

/*
 * call-seq:
 *   add( obj , path ) → self
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

VALUE Archive_add(VALUE self,VALUE obj,VALUE name)
{	
	char buff[8192];
	char *path = NULL;

	size_t bytes_read;
	struct archive *a = archive_read_new(),*b=archive_write_new(),*c=archive_read_disk_new();
	struct archive_entry *entry;
	std::vector<struct archive_entry *> entries;
	std::vector<std::string> allbuff;
	int format= ARCHIVE_FORMAT_EMPTY,compression=ARCHIVE_COMPRESSION_NONE,fd=-1,error=0;
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	//autodetect format and compression
	error=Archive_read_ruby(self,a);
	if(error==ARCHIVE_OK){
		while(archive_read_next_header(a, &entry)==ARCHIVE_OK){
			entries.push_back(archive_entry_clone(entry));
			allbuff.push_back(std::string(""));
			try{
				while ((bytes_read=archive_read_data(a,&buff,sizeof(buff)))>0)
					allbuff.back().append(buff,bytes_read);
			}catch(...){
				rb_raise(rb_eArchiveError,"error:%d:%s",archive_errno(a),archive_error_string(a));	
			}
		}
		format = archive_format(a);
		compression = archive_compression(a);
		archive_read_finish(a);
	}
	if(rb_obj_is_kind_of(obj,rb_cIO)){
		fd = NUM2INT(rb_funcall(obj,rb_intern("fileno"),0));
	}else if(rb_obj_is_kind_of(obj,rb_cFile)){
		VALUE pathname = rb_funcall(obj,rb_intern("path"),0); //source path
		VALUE obj2 = rb_file_s_expand_path(1,&pathname);
		path = rb_string_value_cstr(&obj2);
		fd = NUM2INT(rb_funcall(obj,rb_intern("fileno"),0));
	}else if(rb_respond_to(obj,rb_intern("read"))){
		//stringio has neigther path or fileno, so do nothing
	}else {
		VALUE obj2 = rb_file_s_expand_path(1,&obj);
		path = rb_string_value_cstr(&obj2);
		fd = open(path, O_RDONLY);
		if (fd < 0) //TODO: add error
			return self;
	}
	Archive_format_from_path(self,format,compression);
	if((error = archive_write_set_format(b,format)) != ARCHIVE_OK)
		rb_raise(rb_eArchiveErrorFormat,"error:%d:%s",archive_errno(b),archive_error_string(b));
	Archive_write_set_compression(b,compression);
	
	error=Archive_write_ruby(self,b);
	if(error==ARCHIVE_OK){	
		for(unsigned int i=0; i<entries.size(); i++){
			if(std::string(rb_string_value_cstr(&name)).compare(archive_entry_pathname(entries[i]))!=0){
				archive_write_header(b,entries[i]);
				archive_write_data(b,allbuff[i].c_str(),allbuff[i].length());
				archive_write_finish_entry(b);
			}
		}
		archive_read_disk_set_standard_lookup(c);
		entry = archive_entry_new();
		if (path)
			archive_entry_copy_sourcepath(entry, path);
		archive_entry_copy_pathname(entry, rb_string_value_cstr(&name));
		if(fd < 0 and rb_respond_to(obj,rb_intern("read"))){
			archive_write_header(b, entry);
			VALUE result;
			result = rb_funcall(obj,rb_intern("read"),0);
			archive_write_data(b, rb_string_value_cstr(&result), RSTRING_LEN(result));
		}else{
			archive_read_disk_entry_from_file(c, entry, fd, NULL);
			archive_write_header(b, entry);
			while ((bytes_read = read(fd, buff, sizeof(buff))) > 0)
				archive_write_data(b, buff, bytes_read);
		}
		archive_write_finish_entry(b);
		archive_read_finish(c);
		archive_write_finish(b);
	}
	
	if(fd >= 0 and !rb_obj_is_kind_of(name,rb_cIO))
		close(fd);
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
	if(rb_obj_is_kind_of(name,rb_cFile))
		return Archive_add(self,name,rb_funcall(name,rb_intern("path"),0));
	else
		return Archive_add(self,name,name);
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
	char buff[8192];
	size_t bytes_read;
	struct archive *a = archive_read_new(),*b=archive_write_new();
	struct archive_entry *entry;
	int format = ARCHIVE_FORMAT_EMPTY,compression = ARCHIVE_COMPRESSION_NONE,error=0;
	std::vector<struct archive_entry *> entries;
	std::vector<std::string> allbuff;
	VALUE result = rb_ary_new();
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	//autodetect format and compression
	error=Archive_read_ruby(self,a);
	if(error==ARCHIVE_OK){
		while(archive_read_next_header(a, &entry)==ARCHIVE_OK){
			format = archive_format(a);
			compression = archive_compression(a);
			bool del = false;
			if(rb_obj_is_kind_of(val,rb_cArchiveEntry)==Qtrue){
				del = std::string(archive_entry_pathname(wrap<archive_entry*>(val))).compare(archive_entry_pathname(entry)) == 0;
			}else if(rb_obj_is_kind_of(val,rb_cRegexp)==Qtrue){
				VALUE str = rb_str_new2(archive_entry_pathname(entry));
				del = rb_reg_match(val,str)!=Qnil;
			}else {
				std::string str1(rb_string_value_cstr(&val)),str2(str1);
				str2 += '/'; // dir ends of '/'
				const char *cstr = archive_entry_pathname(entry);
				del = (str1.compare(cstr)==0 || str2.compare(cstr)==0);
			}
			if(!del){
				entries.push_back(archive_entry_clone(entry));
				allbuff.push_back(std::string(""));
				try{
					while ((bytes_read=archive_read_data(a,&buff,sizeof(buff)))>0)
						allbuff.back().append(buff,bytes_read);
				}catch(...){
					rb_raise(rb_eArchiveError,"error:%d:%s",archive_errno(a),archive_error_string(a));	
				}
			}else
				rb_ary_push(result,wrap(archive_entry_clone(entry)));
		}
		archive_read_finish(a);
	
		//format fix
		if(format==ARCHIVE_FORMAT_TAR_GNUTAR)
			format=ARCHIVE_FORMAT_TAR_USTAR;
	
		//TODO add archive-error
		if((error = archive_write_set_format(b,format)) != ARCHIVE_OK)
			rb_raise(rb_eArchiveErrorFormat,"error (%d): %s ",error,archive_error_string(b));
		Archive_write_set_compression(b,compression);
		error=Archive_write_ruby(self,b);
		if(error==ARCHIVE_OK){
			//write old data back
			for(unsigned int i=0; i<entries.size(); i++){
				archive_write_header(b,entries[i]);
				archive_write_data(b,allbuff[i].c_str(),allbuff[i].length());
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
	for(unsigned int i=0; i< data->entries->size(); i++){
		if(!RTEST(rb_yield(wrap(data->entries->at(i))))){
			archive_write_header(data->archive,data->entries->at(i));
			archive_write_data(data->archive,data->allbuff->at(i).c_str(),data->allbuff->at(i).length());
			archive_write_finish_entry(data->archive);
		}else
			rb_ary_push(result,wrap(data->entries->at(i)));
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
	char buff[8192];
	std::string selfpath =_self->path;
	size_t bytes_read;
	struct archive *a = archive_read_new(),*b=archive_write_new();
	struct archive_entry *entry;
	int format = ARCHIVE_FORMAT_EMPTY,compression = ARCHIVE_COMPRESSION_NONE,error=0;
	std::vector<struct archive_entry *> entries;
	std::vector<std::string> allbuff;
	
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	error=Archive_read_ruby(self,a);
	if(error==ARCHIVE_OK){
		while(archive_read_next_header(a, &entry)==ARCHIVE_OK){
			format = archive_format(a);
			compression = archive_compression(a);
			entries.push_back(archive_entry_clone(entry));
			allbuff.push_back(std::string(""));
			try{
				while ((bytes_read=archive_read_data(a,&buff,sizeof(buff)))>0)
					allbuff.back().append(buff,bytes_read);
			}catch(...){
				rb_raise(rb_eArchiveError,"error:%d:%s",archive_errno(a),archive_error_string(a));	
			}
		}
		archive_read_finish(a);
		//format fix
		if(format==ARCHIVE_FORMAT_TAR_GNUTAR)
			format=ARCHIVE_FORMAT_TAR_USTAR;
		if((error = archive_write_set_format(b,format)) != ARCHIVE_OK)
			rb_raise(rb_eArchiveErrorFormat,"error (%d): %s ",error,archive_error_string(b));
		Archive_write_set_compression(b,compression);
		error=Archive_write_ruby(self,b);
		if(error==ARCHIVE_OK){
			write_obj obj;
			obj.archive = b;
			obj.entries = &entries;
			obj.allbuff = &allbuff;
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

	std::string selfpath =_self->path;

	struct archive *a = archive_read_new(),*b=archive_write_new();
	struct archive_entry *entry;
	int format = ARCHIVE_FORMAT_EMPTY,compression = ARCHIVE_COMPRESSION_NONE,error=0;
	
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	archive_read_support_format_raw(a);
	error=Archive_read_ruby(self,a);
	if(error==ARCHIVE_OK){
		archive_read_next_header(a, &entry);
		format = archive_format(a);
		compression = archive_compression(a);
		archive_read_finish(a);
	
		//format fix
		if(format==ARCHIVE_FORMAT_TAR_GNUTAR)
			format=ARCHIVE_FORMAT_TAR_USTAR;
	
		if((error = archive_write_set_format(b,format)) != ARCHIVE_OK)
			rb_raise(rb_eArchiveErrorFormat,"error (%d): %s ",error,archive_error_string(b));
		Archive_write_set_compression(b,compression);
		error=Archive_write_ruby(self,b);
		if(error==ARCHIVE_OK)
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

	rb_define_method(rb_cArchive,"add",RUBY_METHOD_FUNC(Archive_add),2);
	rb_define_method(rb_cArchive,"<<",RUBY_METHOD_FUNC(Archive_add_shift),1);
	
	rb_define_method(rb_cArchive,"inspect",RUBY_METHOD_FUNC(Archive_inspect),0);
	
	rb_define_method(rb_cArchive,"format",RUBY_METHOD_FUNC(Archive_format),0);
	rb_define_method(rb_cArchive,"compression",RUBY_METHOD_FUNC(Archive_compression),0);
	rb_define_method(rb_cArchive,"format_name",RUBY_METHOD_FUNC(Archive_format_name),0);
	rb_define_method(rb_cArchive,"compression_name",RUBY_METHOD_FUNC(Archive_compression_name),0);

	rb_define_method(rb_cArchive,"unlink",RUBY_METHOD_FUNC(Archive_unlink),0);
	rb_define_method(rb_cArchive,"exist?",RUBY_METHOD_FUNC(Archive_exist),0);
	rb_define_method(rb_cArchive,"mtime",RUBY_METHOD_FUNC(Archive_mtime),0);
	rb_define_method(rb_cArchive,"atime",RUBY_METHOD_FUNC(Archive_atime),0);
	rb_define_method(rb_cArchive,"ctime",RUBY_METHOD_FUNC(Archive_ctime),0);
	rb_define_method(rb_cArchive,"stat",RUBY_METHOD_FUNC(Archive_stat),0);
	
	rb_include_module(rb_cArchive,rb_mEnumerable);
	//rb_define_method(rb_cArchive,"size",RUBY_METHOD_FUNC(Archive_size),0);
	
	Init_archive_entry(rb_cArchive);
	
	rb_define_const(rb_cArchive,"EXTRACT_TIME",INT2NUM(ARCHIVE_EXTRACT_TIME));
	rb_define_const(rb_cArchive,"EXTRACT_OWNER",INT2NUM(ARCHIVE_EXTRACT_OWNER));
	rb_define_const(rb_cArchive,"EXTRACT_PERM",INT2NUM(ARCHIVE_EXTRACT_PERM));
	rb_define_const(rb_cArchive,"EXTRACT_NO_OVERWRITE",INT2NUM(ARCHIVE_EXTRACT_NO_OVERWRITE));
	rb_define_const(rb_cArchive,"EXTRACT_ACL",INT2NUM(ARCHIVE_EXTRACT_ACL));
	rb_define_const(rb_cArchive,"EXTRACT_FFLAGS",INT2NUM(ARCHIVE_EXTRACT_FFLAGS));
	rb_define_const(rb_cArchive,"EXTRACT_XATTR",INT2NUM(ARCHIVE_EXTRACT_XATTR));
	
	rb_eArchiveError = rb_define_class_under(rb_cArchive,"Error",rb_eStandardError);
	rb_eArchiveErrorFormat = rb_define_class_under(rb_eArchiveError,"Format",rb_eArchiveError);
	rb_eArchiveErrorCompression = rb_define_class_under(rb_eArchiveError,"Compression",rb_eArchiveError);
}
