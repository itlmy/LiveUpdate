import io
import sys
import hashlib
import string
import os
import plistlib

rootPath = None

def printUsage():
        print ('''Usage: [python] pymd5sum.py <dir>''')

def CalcSha1(filepath):
        with open(filepath,'rb') as f:
                sha1obj = hashlib.sha1()
                sha1obj.update(f.read())
                hash = sha1obj.hexdigest()
                print(hash)
                return hash


def CalcMD5(filepath):
        with open(filepath,'rb') as f:
                md5obj = hashlib.md5()
                md5obj.update(f.read())
                hash = md5obj.hexdigest()
                print(hash)
                return hash

def generate_one_file(file_name, check = 0):
     if(check == 1):
        patchlist = rootPath + "patch.plist"
        file_path = rootPath + file_name
        if not os.path.exists(patchlist) :
            print "file not exist:" + patchlist 
            return
        try:
            doc = plistlib.readPlist(patchlist)
        except:
            print "can't open file:" + patchlist 
            return;
        checkstr= CalcMD5(file_path);
        # checkstr+= CalcSha1(file_path);
        checkstr += ':%d'%os.path.getsize(file_path)
        channels=doc['channels'];
        doc['quickcheck'][file_name]=checkstr
        print('lmy----------'+patchlist+' '+checkstr)
        plistlib.writePlist(doc, patchlist);

def change_sep_from_win_to_linux(path):
    return path.replace(os.sep, '/')

def generate(rootfolder, exclude_dir=[], exclude_files=[], check = 0):
        generate_folder = rootPath + rootfolder
        prefixlen=len(rootfolder)
        connector = '/'
        filename = generate_folder + os.sep + "CHECKSUM.md5";
        dest = io.FileIO(filename,'w')
        dest.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
        dest.write("<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n")
        dest.write("<plist version=\"1.0\">\n")
        dest.write("<dict>\n");

        # print('lmy--------------'+generate_folder)


        for i in os.walk(generate_folder):
            has_exclude_dir = False
            for exclude in exclude_dir:
                if i[0].find(exclude) != -1:
                    has_exclude_dir = True
                    break

            if has_exclude_dir:
                print("skip:"+ i[0])
                continue

            if i[0].find('.git')!= -1 or i[0].find('.svn') != -1:
                print("skip:"+ i[0])
                continue

            # walk all files
            for file_name in i[2]:
                # if file_name=='CHECKSUM.md5' or file_name=='pytest.py' or file_name=='patch.plist':
                if file_name in exclude_files:
                    #print("skip:"+ i[0] + j)
                    continue
                m = hashlib.md5()
                file = io.FileIO(i[0]+os.sep+ file_name,'r')
                bytes = file.read(1024)
                while(bytes != b''):
                    m.update(bytes)
                    bytes = file.read(1024)
                file.close()
                #md5value = ""
                md5value = m.hexdigest()
                prefix=i[0][len(generate_folder)::];
                # root_len = len(rootfolder[2:])
                # prefix = prefix[root_len:]
                
                
                if len(prefix) > 0:
                    prefix=prefix+ os.sep

                # print('lmy---'+'prefix='+prefix+'  '+file_name)
        
                # print(md5value+"\t"+prefix+ j)
                file_size = os.path.getsize(i[0]+os.sep+ file_name)
                value_file_name = change_sep_from_win_to_linux(prefix+file_name)
                dest.write("\t<key>"+value_file_name+"</key>\n")
                dest.write("\t<string>"+md5value+":%d"%file_size+"</string>\n")
        dest.write( "</dict>\n")
        dest.write( "</plist>\n");
        dest.close();
        if(check == 1):
            patchlist = rootPath + "patch.plist"
            if not os.path.exists(patchlist) :
                patch_file = io.FileIO(patchlist,'w')
                patch_file.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
                patch_file.write("<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n")
                patch_file.write("<plist version=\"1.0\">\n")
                patch_file.write("<dict>\n")
                patch_file.write("<key>channels</key>\n")
                patch_file.write("<dict>\n")
                patch_file.write("<key>dev_desktop</key>\n")
                patch_file.write("<dict>\n")
                patch_file.write("<key>download_ipa_url</key>\n")
                patch_file.write("<string>http://www.hema3d.com</string>\n")
                patch_file.write("<key>engine_check_enable</key>\n")
                patch_file.write("<string>1</string>\n")
                patch_file.write("<key>engine_ver_force</key>\n")
                patch_file.write("<string>1</string>\n")
                patch_file.write("<key>engine_ver_recommand</key>\n")
                patch_file.write("<string>1</string>\n")
                patch_file.write("<key>patch_base_url</key>\n")
                patch_file.write("<string>http://www.hema3d.com/LiveUpdate/patch_EHome/</string>\n")
                #patch_file.write("<string>http://localhost/LiveUpdate/patch/</string>\n")
                patch_file.write("</dict>\n")
                patch_file.write("</dict>\n")
                patch_file.write("<key>quickcheck</key>\n")
                patch_file.write("<dict>\n")
                patch_file.write("</dict>\n")
                patch_file.write("</dict>\n")
                patch_file.write("</plist>\n")
                patch_file.close()
            try:
                doc = plistlib.readPlist(patchlist )
            except:
                print "can't open file:" +patchlist 
                return;
            checkstr= CalcMD5(filename);
            checkstr+= CalcSha1(filename);
            channels=doc['channels'];
            # for channel in channels:
				# doc['channels'][channel]['quickcheck']=checkstr;
            # quickchecks=doc['quickcheck'];
            if rootfolder:
                # print('lmy***************'+rootfolder)
                doc['quickcheck'][rootfolder]=checkstr
            else:
                doc['quickcheck']['launcher']=checkstr
            print('lmy------------'+rootfolder+'   checksum: '+checkstr)
            plistlib.writePlist(doc, patchlist);



if __name__ == "__main__":
    if len(sys.argv) == 2:
        rootPath = sys.argv[1]
        # generate game patch
        generate("hippo/EHome/", [], ['CHECKSUM.md5','pytest.py','patch.plist'], 1);

        # generate Lobby patch 
        generate("", ["hippo"], ['CHECKSUM.md5','pytest.py','patch.plist','LiveUpdate.exe'], 1);

        # generate LiveUpdate
        generate_one_file("LiveUpdate.exe", 1);

    else:
        print 'usage: one param for root path'

