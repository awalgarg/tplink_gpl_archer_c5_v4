

##check if the input config is set to y
define check_config
$(shell config=$$(awk '/^$(2)=y$$/{print "y"}' $(1));\
	[ -n "$$config" ] && echo "y" || echo "n";)
endef


##$(1):config file, $(2):config name, $(3):value
define set_config
	config=$$(awk '/\<$(2)\>/{print}' $(1));\
	[ -z "$$config" ] && echo "$(2)=y" >> $(1) || \
	eval "sed -i \"s|"$$config"|$(2)=$(3)|g\"" $(1);
endef

##$(1):config file, $(2):config name
define clear_config
	sed -i "s|^.*\<$(2)\>.*$$|#\ $(2)\ is\ not\ set|g" $(1);
endef

##$(1):config file, $(2):TAG, $(3):value
define get_config_set
$(shell awk 'BEGIN{FS="=";ORS=" ";}/^$(2)[A-Za-z_\-0-9]+\=$(3)$$/{print substr($$1, length("$(2)")+1) }' $(1))
endef

##$(1):config file, $(2):TAG
define get_config_set_not_n
$(shell awk 'BEGIN{FS="=";ORS=" ";}/^$(2)[A-Za-z_\-0-9]+\=[^n]+$$/{print $$0}' $(1))
endef

##$(1):config file, $(2):TAG
define get_config_not_set
$(shell awk 'BEGIN{FS=" ";ORS=" ";}/^# $(2)[A-Za-z_\-0-9]+ is not set$$/{print substr($$2, length("$(2)")+1) }' $(1))
endef

