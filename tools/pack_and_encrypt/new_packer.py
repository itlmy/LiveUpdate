import user_config
import shutil
import sys
import os
import platform
# import win32con, win32api, os
import pack_config
import stat
print('lmy----------',platform.system())
if(platform.system() =="Windows"):
	import win32con, win32api

srcPath = ""
bagPath = ""
dstPath = ""
jscompiler_path = "cocos2d.py"
root_path = ""
exePath = "publish"

fullexepath = ""
jsc_zip_ext = ".jscz"
lua_zip_ext = ".ccz"


def pack_all(pack_type, revert=False, svn_up=True):

	global jscompiler_path
	global root_path
	config = pack_config.pack_config[pack_type]
	jscompiler_path = config["jscompiler_path"]
	root_path = config["root_path"]
	print(root_path)

	dst_path = config["dst_path"]

	
	if os.path.isdir(dst_path):
		print('remove dir:'+dst_path)
		if(platform.system() =="Windows"):
			win32api.SetFileAttributes(dst_path, win32con.FILE_ATTRIBUTE_NORMAL)
		else:
			os.chmod(dst_path, stat.S_IRWXU)
		shutil.rmtree(dst_path)

	resource_path = root_path + os.sep+ config["resource_path"]
	resource_folder_name = os.path.basename(resource_path)
	games_path = root_path + os.sep + config["games_path"]
	games_folder_name = os.path.basename(games_path)
	src_path = root_path + os.sep + config["script_path"]
	src_folder_name = os.path.basename(src_path)

	shutil.copytree(resource_path, dst_path + os.sep + resource_folder_name, ignore=myignore)
	shutil.copytree(src_path, dst_path + os.sep + src_folder_name, ignore=myignore)
	shutil.copytree(games_path, dst_path + os.sep + games_folder_name, ignore=myignore)


	#将release版本的dll和exe拷贝进打包目录
	fullexepath = os.path.join(root_path, exePath)
	print("lmy-------------")
	for _file in os.listdir(fullexepath):
		ext = os.path.splitext(_file)[1]
		if ext == ".dll" or ext == ".exe" or ext == ".ocx":
			print "copying", fullexepath +os.sep+ _file
			shutil.copyfile(fullexepath +os.sep+ _file, dst_path + os.sep+ _file)

	_packer_client(config, 'src', revert, svn_up)

#  #打包script成zip文件, 返回生成的脚本打包结果的路径
def _pack_script(script_path, revert=False, svn_up=True):
	print 'lmy--------packing script',script_path, revert, svn_up
	if svn_up==True:
		try:
			from tools import tools
			tools.svn_up(script_path, None, revert)
		except Exception, e:
			print 'svn up %s failed!!!'%(script_path, )
	# pack_path = script_path + "_pack"
	pack_path = script_path

	print pack_path
	for root, dirs, files in os.walk(pack_path):
		for name in files:

			if name.find('.svn')!=-1 or name.find('.pb')!=-1:
				continue
			inputf = os.path.join(root, name)
			outputf = os.path.splitext(inputf)[0]+lua_zip_ext
			print 'lmy input='+inputf
			print 'lmy outputf='+outputf


	return pack_path



def myignore(path, names):
	return [".svn"]


def _packer_client(config, relative_path, revert=False, svn_up=True):
	only_pack_script = config["only_pack_script"]
	dst_path = config["dst_path"]
	script_relative_path = relative_path
	script_path = dst_path + os.sep + script_relative_path
	script_folder_name = os.path.basename(script_path)
	

	# print('lmy---------'+script_path+' sdsa '+root_path)

	if not only_pack_script:
		if svn_up==True:
			try:
				from tools import tools
				tools.svn_up(os.path.join(resource_path, ".."+os.sep), None, revert)
			except Exception, e:
				pass

	
	script_bag_path = _pack_script(script_path, revert, svn_up)

	
	if only_pack_script:
		dst_script_bag_path = dst_path + os.sep + relative_path
	else:
		dst_script_bag_path = dst_path + os.sep + relative_path
	# print('lmy======================'+script_path)
	# print('lmy======================'+script_bag_path)
	# print('lmy======================'+dst_script_bag_path)
	# shutil.copytree(script_bag_path, dst_script_bag_path)

	# 删除svn文件
	for dirpath, dirs, files in os.walk(dst_script_bag_path):
		for name in files:
			if name.find('.svn') != -1:
				print('remove svn file :' + dirpath + os.sep + name)
				# win32api.SetFileAttributes(dirpath + os.sep + name, win32con.FILE_ATTRIBUTE_NORMAL)
				if(platform.system() =="Windows"):
					win32api.SetFileAttributes(dirpath + os.sep + name, win32con.FILE_ATTRIBUTE_NORMAL)
				else:
					os.chmod(dirpath + os.sep + name, stat.S_IRWXU)
				os.remove(dirpath + os.sep + name)
	
	# 删除svn目录
	for dirpath, dirs, files in os.walk(dst_script_bag_path):
		if dirpath.find('.svn') != -1:
			print('remove svn dir:' + dirpath)
			# win32api.SetFileAttributes(dirpath, win32con.FILE_ATTRIBUTE_NORMAL)
			if(platform.system() =="Windows"):
				win32api.SetFileAttributes(dirpath, win32con.FILE_ATTRIBUTE_NORMAL)
			else:
				os.chmod(dirpath, stat.S_IRWXU)
			shutil.rmtree(dirpath)

	return


if __name__ == '__main__':
	if len(sys.argv) == 2:
		pack_all(sys.argv[1])
	elif len(sys.argv) == 3:
		pack_all(sys.argv[1], sys.argv[2])
	elif len(sys.argv) == 4:
		pack_all(sys.argv[1], int(sys.argv[2]), sys.argv[3])
