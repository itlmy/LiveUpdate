# -*- coding: utf-8 -*-

import shutil
import sys
import os
import stat
import platform
if(platform.system() =="Windows"):
	import win32con, win32api

pack_type = None 

md5_ext = ".CHECKSUM.md5"

def myignore(path, names):
	ignore_names = set(['.svn'])

	return ignore_names

def copyToDst(src_path, dst_path):
	if os.path.isdir(dst_path):
		if(platform.system() =="Windows"):
			win32api.SetFileAttributes(dst_path, win32con.FILE_ATTRIBUTE_NORMAL)
		else:
			os.chmod(dst_path, stat.S_IRWXU)
		shutil.rmtree(dst_path,ignore_errors=True)
		print "removing ", dst_path

	print 'start copy ', src_path, ' to ', dst_path

	shutil.copytree(src_path, dst_path, ignore = myignore)

def copyFilesToDst(src_path, dst_path):

	print 'start copy ', src_path, ' to ', dst_path

	for _file in os.listdir(src_path):
		if _file.find('.svn') == -1:
			shutil.copyfile(src_path +os.sep + _file, dst_path + os.sep + _file)

if __name__ == "__main__":
	if len(sys.argv) == 4:
		if sys.argv[3] == 'true':
			copyToDst(sys.argv[1], sys.argv[2])
		else:
			copyFilesToDst(sys.argv[1], sys.argv[2])

	else:
		print 'usage: copyRes src_path dst_path'
