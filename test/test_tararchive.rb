#!/usr/bin/env ruby
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

gem "test-unit", ">= 2.1" #Ensure we use the gem
require "test/unit"
require_relative File.join("..","ext","archive")

class Test_TarArchive < Test::Unit::TestCase

	class << self
		def startup
			#Dir.chdir("test")
			path = "neue/neue Datei"
			archive = Archive.new(File.join("test","neue.tar.bz2"))
			archive.extract(path,extract: Archive::EXTRACT_TIME | Archive::EXTRACT_OWNER)
		end

		def shutdown
			#Dir.chdir("..")
		end
	end
	def test_format
		a = Archive.new(File.join("test","neue.tar.bz2"))
		assert_equal(a.format_name,"GNU tar format")
	end
	def test_compression
		a = Archive.new(File.join("test","neue.tar.bz2"))
		assert_equal(a.compression_name,"bzip2")
	end
	
	def test_mtime
		path = "neue/neue Datei"
		archive = Archive.new(File.join("test","neue.tar.bz2"))
		assert_equal(archive.find {|e| e.path == path}.mtime,File.mtime(path))
	end

	def test_atime
		path = "neue/neue Datei"
		archive = Archive.new(File.join("test","neue.tar.bz2"))
		assert_equal(archive.find {|e| e.path == path}.atime,File.atime(path))
	end

	def test_ctime
		path = "neue/neue Datei"
		archive = Archive.new(File.join("test","neue.tar.bz2"))
		assert_not_equal(archive.find {|e| e.path == path}.ctime,File.ctime(path))
	end

	def test_uid
		path = "neue/neue Datei"
		archive = Archive.new(File.join("test","neue.tar.bz2"))
		assert_equal(archive.find {|e| e.path == path}.uid,File.stat(path).uid)
	end

	def test_gid
		path = "neue/neue Datei"
		archive = Archive.new(File.join("test","neue.tar.bz2"))
		assert_equal(archive.find {|e| e.path == path}.gid,File.stat(path).gid)
	end
	
end
