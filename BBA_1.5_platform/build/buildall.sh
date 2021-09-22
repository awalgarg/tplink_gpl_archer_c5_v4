#!/bin/sh
dr_user=`whoami`
if [ "$dr_user" != "root" ] ; then
	echo "Aborting script because user is not the root."
	echo ""
	exit
fi
build_dir=$(pwd)

# build all
cd ../../mtk_ApSoC_5020/source/user/wsc_upnp/libupnp-1.3.1 && touch * && cd -
cd ../../BBA_1.5_platform/apps/public/libusb-1.0.8 && touch * && cd -
touch *
make MODEL=C5V4 SPEC=SP env_build boot_build kernel_build modules_build apps_build fs_build image_build

exit 0
