/* vi: set sw=4 ts=4 sts=4: */
/*
 *	wireless.c -- Wireless Settings 
 *
 *	Copyright (c) Ralink Technology Corporation All Rights Reserved.
 *
 *	$Id: //WIFI_SOC/DEV/COMPUTEX-2014/RT288x_SDK/source/user/goahead/src/media.c#3 $
 */

#include	<stdlib.h>
#include	<dirent.h>
#include	<signal.h>
#include	<unistd.h> 

//#include	"webs.h"
#include	"nvram.h"
#include	"media.h"
#include	"config/autoconf.h" //user config
#include "utils.h"

struct status media_state;
struct list all_list[1024];

#define DBG_MSG_media(x) do{fprintf(stderr, #x); \
					fprintf(stderr, ": %s\n", x); \
				 }while(0)


static void ChStatus(int sig)
{
	fprintf(stderr, "goahead receive signal\n");
	if (sig == SIGWINCH)
	{
		strcpy(media_state.connected, "ok");
	}
	else if (sig == SIGPWR)
	{
		strcpy(media_state.connected, "fail");
	}
}

/* goform/mediaPlayer */
void mediaPlayer(char *input)
{
	char *opt, *target;
  	web_debug_header();
	//opt = websGetVar(wp, T("hidden_opt"), T(""));
	opt = strdup(web_get("hidden_opt", input, 1));
	if(opt == NULL) opt="";
	DBG_MSG("harry:opt=%s\n", opt);
  	nvram_bufset(RT2860_NVRAM, "media_state_operate", opt);
	do_system("killall -9 btut");
	do_system("killall -9 blueangel");
	do_system("killall -9 airplayd");
	do_system("killall -9 mplayer");
	//doSystem("rmmod btmtk_usb");
	if (strcmp(opt, "play") == 0)
	{
		//char_t *service_mode = websGetVar(wp, T("service_mode"), T(""));
    	char *service_mode;
    	service_mode = strdup(web_get("service_mode", input, 1));
	if(service_mode == NULL) service_mode="";
    
	memset(&media_state.operate, 0, sizeof(media_state.operate));
	memset(&media_state.service_mode, 0, sizeof(media_state.service_mode));
	memset(&media_state.option, 0, sizeof(media_state.option));
	memset(&media_state.target, 0, sizeof(media_state.target));
	memset(&media_state.connected, 0, sizeof(media_state.connected));
	DBG_MSG_media(service_mode);
	if (strcmp(service_mode, "0") == 0)
	{
			/* Internet Radio - LIST
			target = websGetVar(wp, T("inet_radio"), T(""));
			doSystem("mplayer -cache 256 %s &", target);
			*/
			/* AirPlay */
		do_system("airplayd 1>/dev/null &");
	}
	else if (strcmp(service_mode, "1") == 0)
	{
			/* Internet Radio - URL
			target = websGetVar(wp, T("radio_url"), T(""));
			doSystem("mplayer %s &", target);
			*/
			/* BTPlay */
			//doSystem("insmod -q btmtk_usb");
		do_system("blueangel 1>/dev/null &");
		do_system("btut 1>/dev/null &");
	}
	else if (strcmp(service_mode, "2") == 0)
	{
			//char_t *option;
      		char *option;
      		if (0 == strlen(all_list[0].name))
		{
			memset(all_list, 0, sizeof(all_list));
			lookupAllList("/media");
			lookupSelectList();
		}
			//option = websGetVar(wp, T("play_mode"), T(""));
			//target = websGetVar(wp, T("play_list"), T(""));
		opt = strdup(web_get("list_opt", input, 1));
		option = strdup(web_get("play_mode", input, 1));
		if(option==NULL) option="";
			//target = strdup(web_get("play_list", input, 1));
      			//DBG_MSG("harry player: track_path =%s\n", track_path);
		if (strcmp(option, "random") == 0)
		{
				 //DBG_MSG("harry player: random\n");
			do_system("mplayer -ao pcm -shuffle -playlist %s 1>/dev/null &", track_path);
		}
		else{
				//DBG_MSG("harry player: else\n");
			do_system("mplayer -ao pcm -playlist %s 1>/dev/null &", track_path);
		}
			//strcpy(media_state.option, option);
			nvram_bufset(RT2860_NVRAM, "media_state_option", option);
	}
	else{
		DBG_MSG("harry: something wrong!");
	}
		//DBG_MSG("harry player: here0000\n");
		//strcpy(media_state.operate, opt);
		
		//strcpy(media_state.service_mode, service_mode);
	nvram_bufset(RT2860_NVRAM, "media_state_service_mode", service_mode);
		//strcpy(media_state.target, target);
	nvram_bufset(RT2860_NVRAM, "media_state_connected", "load");
		
		//strcpy(media_state.connected, "load");
		//DBG_MSG("harry player: here1\n");
		//signal(SIGWINCH, ChStatus);
		//signal(SIGPWR, ChStatus);
	}
	else if (strcmp(opt, "stop") == 0)
	{
		/*to do nvram*/
		memset(&media_state.operate, 0, sizeof(media_state.operate));
		memset(&media_state.service_mode, 0, sizeof(media_state.service_mode));
		memset(&media_state.option, 0, sizeof(media_state.option));
		memset(&media_state.target, 0, sizeof(media_state.target));
		strcpy(media_state.operate, opt);
	}
	DBG_MSG("harry player: here2\n");
	//web_redirect(getenv("HTTP_REFERER"));
	//websRedirect(wp, "media/player.asp");
	free_all(1, opt);
}

static int getIndex(char **select)
{
	char *temp;
	int index;

	temp = strstr(*select, " ");
	if (NULL != temp)
	{
		*temp = '\0';
		index = atoi(*select);
		*select = temp+1;
	}
	else if (0 != strlen(*select))
	{
		index = atoi(*select);
		**select = '\0';
	}
	else
		index = 1024;

	return index;
}
/* goform/mediaPlayerList */
void mediaPlayerList(char *input)
{
	char *opt, *select;
	int i;
	web_debug_header();


	//opt = websGetVar(wp, T("list_opt"), T(""));
	opt = strdup(web_get("list_opt", input, 1));
	if(opt==NULL) opt="";
	//DBG_MSG_media(opt);
//DBG_MSG("harry: HERE!");
//DBG_MSG("harry: opt=%s\n", opt);



	if (0 == strlen(all_list[0].name))
	{
		memset(all_list, 0, sizeof(all_list));
		lookupAllList("/media");
		lookupSelectList();
	}
		FILE *fp_list = fopen(track_path, "w");
		if(fp_list==NULL) fp_list="";
	if (0 == strcmp(opt, "add"))
	{

		//select = websGetVar(wp, T("unselected_list"), T(""));
			select = strdup(web_get("unselected_list", input, 1));
			//DBG_MSG("harry: unselected_list=%s\n", select);
	
		do {
			i = getIndex(&select);
			if (i < 1024)
				all_list[i].selected = 1;
		} while(0 != strlen(select));
	}
	else if (0 == strcmp(opt, "del"))
	{
		//select = websGetVar(wp, T("selected_list"), T(""));
		select = strdup(web_get("selected_list", input, 1));
		//DBG_MSG_media(select);
		do {
			i = getIndex(&select);
			if (i < 1024)
				all_list[i].selected = 0;
		} while(0 != strlen(select));
	}
	i = 0;

	while (0 != strlen(all_list[i].name) && i < 1023)
	{
		
	//DBG_MSG("harry: fp_list[%d] len = %d\n", i,strlen(all_list[i].name));
	//DBG_MSG("harry: fp_list[%d] name = %s\n", i,(all_list[i].name));
	//DBG_MSG("harry: fp_list[%d] path = %s\n", i,(all_list[i].path));
	
		//DBG_MSG("harry selected: fp_list[%d] selected = %d\n",i, (all_list[i].selected));
		if (1 == all_list[i].selected)
		{

			fprintf(fp_list, "%s\n", all_list[i].path);
		}
		i++;
	}
	fclose(fp_list);
	//DBG_MSG("HEREEE1\n");
//DBG_MSG("harry: service_mode=%s\n", media_state.service_mode);
	//websRedirect(wp, "media/player.asp");
	//web_redirect(getenv("HTTP_REFERER"));
end:
	free_all(1, opt);

}









void cleanup_musiclist(void)
{
	memset(all_list, 0, sizeof(all_list));
}


int main(int argc, char *argv[]) 
{
	
	
	char *form, inStr[512], *tmp;
	long inLen;

	nvram_init(RT2860_NVRAM);
	tmp = getenv("CONTENT_LENGTH");
	if(tmp == NULL) tmp = "";
	inLen = strtol(tmp, NULL, 10) + 1;
	if (inLen <= 1) {
		DBG_MSG("get no data!");
		goto leave;
	}
	//inStr = malloc(inLen);
	memset(inStr, 0, sizeof(inStr));
	fgets(inStr, inLen, stdin);
	//DBG_MSG("%s\n", inStr);
	form = web_get("media", inStr, 0);
	
	
	if (!strcmp(form, "mediaPlayer")) {
		mediaPlayer(inStr);
	} else if (!strcmp(form, "mediaPlayerList")) {
		mediaPlayerList(inStr);
	} 
	//free(inStr);
leave:

	nvram_close(RT2860_NVRAM);

	return 0;
}
