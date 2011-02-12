#!/usr/bin/env ruby
#Encoding: UTF-8
gem "test-unit", ">= 2.1" #Ensure we use the gem
require "test/unit"
require_relative File.join("..","ext","archive")

class Test_TarArchive < Test::Unit::TestCase

	class << self
		def startup
			Dir.chdir("test")
			path = "neue/neue Datei"
			archive = Archive.new(File.join("test","neue.tar.bz2"))
			archive.extract(path)
		end

		def shutdown
			Dir.chdir("..")
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
