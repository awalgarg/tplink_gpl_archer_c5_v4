#! /bin/bash
################################### Constants  ############################
PAGES_5G="wlScan_5G wlScan wlGuestDulBandAdv wlGuestDulBandBasic dhcpServer \
domain-redirect ethDiv ethMultiWan ethWan fwUpInfoBlock ipsec offlineError \
qosRule sysMode"

PAGES_5G_HELP=""

PAGES_DSL="ipoa pppoa dsl dslcfg "
PAGES_DSL_HELP=""

PAGES_HNAT=""

PAGES_USB="basic3g usb3g diskSettings usb3gModemList folderSharing usbManage"

PAGES_USB_HELP=""

################################### Pre-Process ############################
while [ -n "$1" ]; do
	case "$1" in
		-w|--webdir)
			WEB_DIR=$2; shift 2;;
		
		--no-usb)
			PAGES_TO_DELETE="$PAGES_TO_DELETE $PAGES_USB";
			PAGES_HELP_TO_DELETE="$PAGES_HELP_TO_DELETE $PAGES_USB_HELP";
			shift 1;;
			
		--no-5g)
			PAGES_TO_DELETE="$PAGES_TO_DELETE $PAGES_5G";
			PAGES_HELP_TO_DELETE="$PAGES_HELP_TO_DELETE $PAGES_5G_HELP";
			shift 1;;
			
		--no-dsl)
			PAGES_TO_DELETE="$PAGES_TO_DELETE $PAGES_DSL"; 
			PAGES_HELP_TO_DELETE="$PAGES_HELP_TO_DELETE $PAGES_DSL_HELP";
			shift 1;;
			
		--no-hnat)
			PAGES_TO_DELETE="$PAGES_TO_DELETE $PAGES_HNAT"; 
			PAGES_HELP_TO_DELETE="$PAGES_HELP_TO_DELETE $PAGES_HNAT_HELP";
			shift 1;;
			
		--compress-web)
			CONFIG_COMPRESS_HTML=1
			CONFIG_COMPRESS_JS=1
			CONFIG_COMPRESS_CSS=1
			shift 1;;

		--compress-html)
			CONFIG_COMPRESS_HTML=1; shift 1;;
			
		--compress-multilang)
			CONFIG_COMPRESS_MULTILANG=1; shift 1;;
			
		--compress-js)
			CONFIG_COMPRESS_JS=1; shift 1;;
			
		--compress-css)
			CONFIG_COMPRESS_CSS=1; shift;;
			
		*)
			echo "wrong paramter!"; exit 1;;
	esac
done

WEB_PAGES_DIR="$WEB_DIR/main/"	# web pages dir
WEB_HELP_DIR="$WEB_DIR/help/"	# help pages dir
UTIL_DIR="$WEB_DIR/../../../../../../BBA_1.5_platform/host_tools/fsCompress"

################################# End Pre-Process ##########################


################################### File System ############################

# remove unused usb pages in webpages dir

for file in $PAGES_TO_DELETE;
do
	echo "remove unused $WEB_PAGES_DIR/$file.htm"
	rm -f $WEB_PAGES_DIR/$file.htm
done

#for file in $PAGES_HELP_TO_DELETE;
#do
#	echo "remove unused $WEB_HELP_DIR/$file.htm"
#	rm -f $WEB_HELP_DIR/$file.htm
#done

rm -f $WEB_DIR/img/mark_copy.gif
#rm -f $WEB_DIR/img/tp-beta-mark.png

################################# End File System ###########################

############################### Web Pages Compression #######################
# compress webpages and webmobile html files
if [ $CONFIG_COMPRESS_HTML ]; then
	for file in $(find $WEB_DIR -name '*.htm*')
	do
		echo "compressing $file"
		#python $UTIL_DIR/compressJSinHtml.py $UTIL_DIR "$file" 1
		$UTIL_DIR/node $UTIL_DIR/compressJsInHtml.js $UTIL_DIR "$file"
	done	
fi

# compress css files in webmobile and webpages dir
if [ $CONFIG_COMPRESS_CSS ];then
	for file in $(find $WEB_DIR -name '*.css')
	do	
		echo "compressing $file"
		java -jar $UTIL_DIR/yuicompressor.jar -o "$file-min.css" "$file"
		mv "$file-min.css" "$file"
	done
fi

# compress js files
#--nomunge option of yuicompressor.jar should be used to avoid some puzzles
if [ $CONFIG_COMPRESS_JS ];then
	for file in $(find $WEB_DIR -name '*.js')
	do
		echo "compressing $file"
		java -jar $UTIL_DIR/yuicompressor.jar --nomunge --charset UTF-8 -o "$file-min.js"	"$file"
		mv "$file-min.js" "$file"
	done
fi
################################## End Web Pages Compression########################
