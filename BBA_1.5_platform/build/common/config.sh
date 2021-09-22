#!/bin/sh

##check if the input config is set to y
##$(1):config file, $(2):config name
check_config()
{
    config=$(eval "awk '/^$2=y\$/{print \"y\"}' $1")
	[ -n "$config" ] && echo "y" || echo "n"
}


##$(1):config file, $(2):config name, $(3):value
set_config()
{
	config=$(eval "awk '/\<$2\>/{print}' $1")
	[ -z "$config" ] && echo "$2=y" >> $1 || \
	eval "sed -i \"s|$config|$2=$3|g\" $1"
}

##$(1):config file, $(2):config name
clear_config()
{
	sed -i "s|^.*\<$2\>.*$|#\ $2\ is\ not\ set|g" $1
}

##$(1):config file, $(2):TAG, $(3):value
get_config_set()
{
    eval "awk 'BEGIN{FS=\"=\";ORS=\" \";}/^$2[A-Za-z_\-0-9]+\=$3\$/{print substr(\$1, length(\"$2\")+1) }' $1"
}

##$(1):config file, $(2):TAG
get_config_set_not_n()
{
    eval "awk 'BEGIN{FS=\"=\";ORS=\" \";}/^$2[A-Za-z_\-0-9]+\=[^n]+\$/{print \$0}' $1"
}

##$(1):config file, $(2):TAG
get_config_not_set()
{
    eval "awk 'BEGIN{FS=\" \";ORS=\" \";}/^# $2[A-Za-z_\-0-9]+ is not set\$/{print substr(\$2, length(\"$2\")+1) }' $1"
}

### $1: dir
generate_Kconfig()
{
### NOTE: DO NOT UES -f, as it may be soft link.
    [ -e "$1/Kconfig" ] || {
    
        local dirlist=$(find "$1" -maxdepth 1 -mindepth 1 -type d);
        for i in $dirlist; do
            echo "source \"$1/$(basename $i)/Kconfig\"" >> "$1/Kconfig"
            
        done
        
###        for i in $dirlist; do
###            $(generate_Kconfig $i)
###        done
        
    }
}

generate_module_Makefile()
{
   ### NOTE: DO NOT UES -f, as it may be soft link.
    [ -e "$1/Makefile" ] || {
    
        local dirlist=$(find "$1" -maxdepth 1 -mindepth 1 -type d);
        for i in $dirlist; do
        ### obj-y += using basename here.
        ### for private module under GPL building, do not add it into kernel build list. 
        if [ ! -e "$i/_private" ];then 
            ### if no makefile exists in sub dir, do not add it into makefile.
            if [ -e "$i/Makefile" ]; then
                echo "obj-y+=$(basename $i)/" >> "$1/Makefile"
            fi
        fi
        done
        
###       for i in $dirlist; do
###            $(generate_module_Makefile $i)
###       done
        
    }
} 
