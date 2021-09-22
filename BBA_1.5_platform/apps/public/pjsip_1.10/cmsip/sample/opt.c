#include <stdio.h>
#include "cmsip_cmdopt.h"

int main()
{
	int ret;
	ret = cmsip_config_start();
	if (ret < 0)
	{
		printf("error:can not open conf file\n");
		return -1;
	}

	ret = cmsip_config_option(IDX_ID, "sip:1100@192.168.1.254");
	if (ret < 0)
	{
		printf("error: can not write option\n");
		return -1;
	}

	ret = cmsip_config_option(IDX_CONTACT, "\"flyfish\" <sip:1100@192.168.1.254>");
	if (ret < 0)
	{
		printf("error:can not write option\n");
		return -1;
	}
	
	ret = cmsip_config_option(IDX_USE_IMS, NULL);
	if (ret < 0)
	{
		printf("error");
		return -1;
	}

	ret = cmsip_config_end();
	if (ret < 0)
	{
		printf("error:can not close conf file\n");
		return -1;
	}
}
