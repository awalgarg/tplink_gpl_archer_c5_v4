# Description: deal with compression of js codes in the htmls.
# Author by Tristan Xiao.

import re
import sys, os

############################## Pre-Process ##############################
# argv[1]: utils_dir
# argv[2]: html_dir
# argv[3]: Compress method
# argv[4]: wether webmobile supportted
if len(sys.argv) < 4:
    exit=True
else:
	exit=False
	utils_dir=sys.argv[1]
	html_fimename=sys.argv[2]
	method=int(sys.argv[3])
	if len(sys.argv) == 4:
		webmobile=False
	else:
		webmobile=True
############################ End Pre-Process ############################


############################### Functions ###############################
js_pattern=re.compile("<script[^>]*type=\"text/javascript\"[^>]*>([^@](.|\n)*?)</script>")
html_note_pattern=re.compile("<\!--((.|\n)*?)-->")

js_suffix=".min.js"
identify_prefix="@"

# get js scripts from .html file and store them in the same name .js.
# example: basic.html -> basic.html.js
def get_js_scripts(filename):
    fp=open(filename, 'rb')
    buf=fp.read()
    fp.close
    js_target=js_pattern.findall(buf)
    if js_target is not None:
        i = 0
        # replace the script context to sepcial identify.
        # example: <script type> asfjlasdfj
        #           asdklfjakdsf                --->        <script type>Tristan_0</script>
        #           aksldfjaskldf
        #           </script>
        for entry in js_target:
            buf=js_pattern.sub("<script type=\"text/javascript\">" + identify_prefix + str(i) + "</script>", buf, 1)
            i=i+1
        fp=open(filename, "wb")
        fp.write(buf)
        fp.close()
        return js_target
    else:
        return None

# replace js in basic.html with basic.html.1.js.min.js
def replace_js(html_buf, identify, comp_js_buf):
    js_sub = re.sub(identify, comp_js_buf, html_buf, 1)
    return js_sub

# method: 1 - YUI; 2 - Google closure
def compress_js(filename, method):
    if method == 1:
        os.system("java -jar " + utils_dir + "/yuicompressor.jar --nomunge -o " + filename + js_suffix +" " + filename)
        #print "java -jar " + utils_dir + "/yuicompressor.jar -o " + filename + js_suffix + " " + filename
    elif method == 2:
        os.system("java -jar " + utils_dir + "/compiler.jar --compilation_level=ADVANCED --js=" + filename + " --js_output_file=" + filename + js_suffix + " --warning_level=QUIET")
        #print("java -jar " + utils_dir + "/compiler.jar --compilation_level=ADVANCED --js=" + filename + " --js_output_file=" + filename + js_suffix + " --warning_level=QUIET")
    # remove temp file
    os.remove(filename)


# example: basic.html -> baisc.html.1.js -> basic.html.1.js.min.js
def compress_and_replace_js(html_filename, js_list, method):
    if js_list is not None:
        i=0;
        # loop for doing js compression
        for entry in js_list:
            js_filename_tmp=html_filename+"."+str(i)+".js"
            i=i+1
            fp=open(js_filename_tmp, "wb")
            fp.write(entry[0])
            fp.close()
            compress_js(js_filename_tmp, method)

        h_fp=open(html_filename, "rb")
        h_buf=h_fp.read()
        h_fp.close()
        i=0
        # loop for doing js replacement
        # example: 
        #           <script type>Tristan_0</script>     --->       <script type>alksdjflasdfjlwasdfjaksdflaskjdfalskdf</script>
        for entry in js_list:
            c_jsfilename=html_filename+"."+str(i)+".js"+js_suffix
            c_fp=open(c_jsfilename, "rb")
            c_buf=c_fp.read()
            c_fp.close()
            # remove temp file
            os.remove(c_jsfilename)
            h_buf=replace_js(h_buf, identify_prefix+str(i), c_buf)
            i=i+1
        h_fp=open(html_filename, "wb")
        h_fp.write(h_buf)
        h_fp.close()

def get_html_file(dir):
    return os.listdir(dir)

def rm_note(filename):
    fp=open(filename, "rb")
    buf=fp.read()
    fp.close()
    buf=html_note_pattern.subn('', buf)
    fp=open(filename, "wb")
    fp.write(buf[0])
    fp.close()

# compress js files in the full directory.
def compress_dir(dir, method=1):
    flist=get_html_file(dir)
    if flist is not None:
        # Compression
        for entry in flist:
            ful_filename=dir+entry
            if os.path.isdir(ful_filename) is False:
                if re.match("\.html", entry) is not False:
                    js_list=get_js_scripts(ful_filename)
                    compress_and_replace_js(ful_filename, js_list, method)
        # remove the html notes
        for entry in flist:
            ful_filename=dir+entry
            if os.path.isdir(ful_filename) is False:
                if re.match("\.html", entry) is not False:
                    rm_note(ful_filename)

def compress_html(ful_filename,method=1):
    if os.path.isdir(ful_filename) is False:
        if re.match("\.htm", ful_filename) is not False:
            js_list=get_js_scripts(ful_filename)
            compress_and_replace_js(ful_filename, js_list, method)
            rm_note(ful_filename)
############################# End Functions #############################


#########################################################################
############################## Compression ##############################

compress_html(html_fimename,method)
	
# webmobile process
if webmobile is True:
	print "webmobile process"
############################ End Compression ############################
#########################################################################


################################# Test #################################
# Test case for DEBUG
filename="internet.html"
def test_get_js_scripts(filename):
    js_target=get_js_scripts(filename)
    if js_target is not None:
        for entry in js_target:
            print entry[0]
############################### End Test ###############################