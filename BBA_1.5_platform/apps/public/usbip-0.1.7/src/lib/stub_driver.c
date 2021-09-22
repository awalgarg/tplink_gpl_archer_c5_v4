/*
 * Copyright (C) 2005-2006 Takahiro Hirofuchi
 */

#include "usbip.h"

static const char *usbip_stub_driver_name = "usbip";



static struct sysfs_driver *open_sysfs_stub_driver(void)
{
	int ret;

	char sysfs_mntpath[SYSFS_PATH_MAX];
	char stub_driver_path[SYSFS_PATH_MAX];
	struct sysfs_driver *stub_driver;


	ret = sysfs_get_mnt_path(sysfs_mntpath, SYSFS_PATH_MAX);
	if(ret < 0) {
		err("sysfs must be mounted");
		return NULL;
	}

	snprintf(stub_driver_path, SYSFS_PATH_MAX, "%s/%s/usb/%s/%s",
			sysfs_mntpath, SYSFS_BUS_NAME, SYSFS_DRIVERS_NAME,
			usbip_stub_driver_name);

	stub_driver = sysfs_open_driver_path(stub_driver_path);
	if(!stub_driver) {
		err("stub.ko must be loaded");
		return NULL;
	}

	return stub_driver;
}

#if 0
static void unshift_if_new(struct dlist *dlist, struct sysfs_device *target)
{
	struct sysfs_device *dev;

	dlist_for_each_data(dlist, dev, struct sysfs_device) {
		if(!strncmp(dev->bus_id, target->bus_id, SYSFS_BUS_ID_SIZE))
			return;
	}

	dlist_unshift(dlist, (void *) target);
}
#endif


/* only the first interface value is true! */
static int32_t read_attr_usbip_status(struct usb_device *udev)
{
	char attrpath[SYSFS_PATH_MAX];
	struct sysfs_attribute *attr;
	int value = 0;
	int  ret;

	snprintf(attrpath, SYSFS_PATH_MAX, "%s/%s:%d.%d/usbip_status",
			udev->path, udev->busid,
			udev->bConfigurationValue, 
			0);

	attr = sysfs_open_attribute(attrpath);
	if(!attr) {
		err("open %s", attrpath);
		return -1;
	}

	ret = sysfs_read_attribute(attr);
	if(ret) {
		err("read %s", attrpath);
		sysfs_close_attribute(attr);
		return -1;
	}

	value = atoi(attr->value);

	sysfs_close_attribute(attr);

	return value;
}


static void usbip_exported_device_delete(void *dev)
{
	struct usbip_exported_device *edev = 
		(struct usbip_exported_device *) dev;

	sysfs_close_device(edev->sudev);
	free(dev);
}


static struct usbip_exported_device *usbip_exported_device_new(char *sdevpath)
{
	struct usbip_exported_device *edev = NULL;

	edev = (struct usbip_exported_device *) calloc(1, sizeof(*edev));
	if(!edev) {
		ps_err("alloc device");
		return NULL;
	}

	edev->sudev = sysfs_open_device_path(sdevpath);
	if(!edev->sudev) {
		info("open %s", sdevpath);
		goto err;
	}

	read_usb_device(edev->sudev, &edev->udev);

	edev->status = read_attr_usbip_status(&edev->udev);
	if(edev->status < 0) 
		goto err;

	/* reallocate buffer to include usb interface data */
	size_t size = sizeof(*edev) + edev->udev.bNumInterfaces * sizeof(struct usb_interface);
	edev = (struct usbip_exported_device *) realloc(edev, size);
	if(!edev) {
		ps_err("alloc device");
		goto err;
	}

	for(int i=0; i < edev->udev.bNumInterfaces; i++)
		read_usb_interface(&edev->udev, i, &edev->uinf[i]);

	return edev;

err:
	if(edev && edev->sudev)
		sysfs_close_device(edev->sudev);
	if(edev)
		free(edev);
	return NULL;
}


static int check_new(struct dlist *dlist, struct sysfs_device *target)
{
	struct sysfs_device *dev;

	dlist_for_each_data(dlist, dev, struct sysfs_device) {
		if(!strncmp(dev->bus_id, target->bus_id, SYSFS_BUS_ID_SIZE))
			/* found. not new */
			return 0;
	}

	return 1;
}

/* filled in by tf, 110620, rename "delete_nothing" to "sysfs_device_delete" */
static void sysfs_device_delete(void *dev)
{
	/* do not delete anything. but, its container will be deleted. */
#if 0
	struct sysfs_device *edev = (struct sysfs_device *) dev;

	sysfs_close_device(edev);
#endif
}

static void dump_dlist(Dlist *list)
{
    int i = 0;
    if(list ==NULL)
   	goto end;

    dlist_start(list);
    while(dlist_next(list))
		i++;

end:
   dbg("iiiiiiiiiiiiiiii = %d", i);
}

static int refresh_exported_devices(struct usbip_stub_driver *driver)
{
	struct sysfs_device	*suinf;  /* sysfs_device of usb_interface */
	struct dlist		*suinf_list;

	struct sysfs_device	*sudev;  /* sysfs_device of usb_device */
	struct dlist		*sudev_list; 


	sudev_list = dlist_new_with_delete(sizeof(struct sysfs_device), sysfs_device_delete);
		
	suinf_list = sysfs_get_driver_devices(driver->sysfs_driver);
	if(!suinf_list) {
		/*printf("usbip driver is binded to nothing?\n");*/
		dbg("stub device retrieve");
		dlist_destroy(sudev_list);	/* we must delete sudev_list created just now to fix up memory leak!!![bug 15813] */
		return -1;
	}

#if 0
	dump_dlist(suinf_list);
#endif
	/* collect unique USB devices (not interfaces) */
	dlist_for_each_data(suinf_list, suinf, struct sysfs_device) {

		/* get usb device of this usb interface */
		sudev = sysfs_get_device_parent(suinf);
		if(!sudev) {
			err("get parent dev of %s", suinf->name);
			continue;
		} 

		if(check_new(sudev_list, sudev)) {
			dlist_unshift(sudev_list, sudev);
		}
	}

	dlist_for_each_data(sudev_list, sudev, struct sysfs_device) {
		struct usbip_exported_device *edev; 

		edev = usbip_exported_device_new(sudev->path);
		if(!edev) {
			err("usbip_exported_device new");
			continue;
		}

		dlist_unshift(driver->edev_list, (void *) edev);
		driver->ndevs++;
	}

	dlist_destroy(sudev_list);

	return 0;
}

int usbip_stub_refresh_device_list(struct usbip_stub_driver *driver)
{
	int ret;
	if(driver->edev_list)
		dlist_destroy(driver->edev_list);

	driver->ndevs = 0;

	driver->edev_list = dlist_new_with_delete(sizeof(struct usbip_exported_device),
			usbip_exported_device_delete);
	if(!driver->edev_list) {
		err("alloc dlist");
		return -1;
	}

	/* added by tf, 110620, delete sysfs_driver created last time to fix up memory leak. */
	if(driver->sysfs_driver){
		sysfs_close_driver(driver->sysfs_driver);
		driver->sysfs_driver = open_sysfs_stub_driver();
	}
	
	/* make up  "driver->sysfs_driver" if this user-space program couldn't find usbip module before , tf, 20110301 */
	if(!driver->sysfs_driver)
	{
		dbg("driver->sysfs_driver is none!");
		driver->sysfs_driver = open_sysfs_stub_driver();
		if(!driver->sysfs_driver)
			return STUB_OFFLINE;
	}
	
	ret = refresh_exported_devices(driver);
	if(ret < 0) 
		return ret;
	
	return 0;
}

struct usbip_stub_driver *usbip_stub_driver_open(void)
{
	int ret;
	struct usbip_stub_driver *driver;

	
	driver = (struct usbip_stub_driver *) calloc(1, sizeof(*driver));
	if(!driver) {
		ps_err("alloc driver");
		return NULL;
	}

	driver->ndevs = 0;

	driver->edev_list = dlist_new_with_delete(sizeof(struct usbip_exported_device),
			usbip_exported_device_delete);
	if(!driver->edev_list) {
		err("alloc dlist");
		goto err;
	}

	driver->sysfs_driver = open_sysfs_stub_driver();
	/* make this user-space program independent of usbip module  ,tf ,20110301 */
	#if 0
	if(!driver->sysfs_driver)
		goto err;
	#endif

	ret = refresh_exported_devices(driver);
	/* make this user-space program independent of usb devices  ,tf ,20110301 */
	#if 0
	if(ret < 0) 
		goto err;
	#endif

	return driver;


err:
	if(driver->sysfs_driver)
		sysfs_close_driver(driver->sysfs_driver);
	if(driver->edev_list)
		dlist_destroy(driver->edev_list);
	free(driver);

	return NULL;
}


void usbip_stub_driver_close(struct usbip_stub_driver *driver)
{
	if(driver->edev_list)
		dlist_destroy(driver->edev_list);
	if(driver->sysfs_driver)
		sysfs_close_driver(driver->sysfs_driver);
	free(driver);
}

/***************************************************************************
 * FUNCTION       :       add_client_waitinglist()
 * AUTHOR          :        TengFei
 * OCCASION      :        
 * DESCRIPTION  :      add client to waiting list
 *
 * INPUT             :       N/A
 * OUTPUT          :       N/A
 * RETURN          :       0: success; -1:error
 ***************************************************************************/
int add_client_waitinglist(struct usbip_exported_device *edev)
{
	struct dev_list *head, *tmp;
	struct waiting_list *cli, *cli_tmp;

	for (tmp = devlist; tmp != NULL; tmp = devlist->next)
	{
		if (!strcmp(tmp->busid, edev->udev.busid))
			break;
	}

	if (tmp == NULL) 
	{
		tmp = (struct dev_list*) calloc(1, sizeof(struct dev_list));
		if(!tmp) {
			ps_err("alloc devlist");
			return -1;
		}
		tmp->client = NULL;
		tmp->next = NULL;
		strncpy(tmp->busid, edev->udev.busid, 7);
		tmp->busid[7] = 0;
		tmp->status = SDEV_ST_USED;
		/* insert new created dev-node */
		if (devlist == NULL)
			devlist = tmp;
		else {
			for(head = devlist; head->next != NULL; head = head->next)
				;
			head->next = tmp;
		}
	}

	for(cli_tmp =  tmp->client; cli_tmp != NULL; cli_tmp = cli_tmp->next)
	{
		if (!strcmp(cli_tmp->hostname, host))
			break;
	}

	if (cli_tmp == NULL)
	{
		cli_tmp = (struct waiting_list*) calloc(1, sizeof(struct waiting_list));
		if(!cli_tmp) {
			ps_err("alloc waiting_list");
			return -1;
		}
		cli_tmp->next = NULL;
		/* Modify by chz for IPv6 host name, 2013-05-30 */
		//strncpy(cli_tmp->hostname, host, 15);
		//cli_tmp->hostname[15] = 0;
		strncpy(cli_tmp->hostname, host, USBIP_MAXHOST - 1);
		cli_tmp->hostname[USBIP_MAXHOST - 1] = 0;
		/* end modify */
		cli_tmp->status = 0;
		/* insert new created client-node */
		if (tmp->client == NULL)
			tmp->client  = cli_tmp;
		else {
			for(cli = tmp->client; cli->next != NULL; cli = cli->next)
				;
			cli->next = cli_tmp;
		}
		ps_info("add new client %s to devlist.\n", host);
	}
	return 0;
}

/***************************************************************************
 * FUNCTION       :       is_first_client()
 * AUTHOR          :        TengFei
 * OCCASION      :        
 * DESCRIPTION  :     is it a first client while dev is available ?
 *
 * INPUT             :       N/A
 * OUTPUT          :       N/A
 * RETURN          :       1: first client ; 0: not first client
 ***************************************************************************/
int is_first_client(struct usbip_exported_device *edev, int isattach)
{
	struct dev_list  *tmp;
	struct waiting_list *cli;

	for (tmp = devlist; tmp != NULL; tmp = devlist->next)
	{
		if (!strcmp(tmp->busid, edev->udev.busid))
			break;
	}
	
	if (tmp == NULL)	/* it is the only one client who want to attach device , don't insert it into waitinglist */
	{
		return 1;
	}
	if (tmp->client == NULL)
	{
		return 1;
	}

	cli = tmp->client;
	
	/*ps_info("You are %s; current 1st client: %s\n", host, cli->hostname);*/
	if (!strcmp(cli->hostname, host))
	{
		/*ps_info("%s %s success!\n", host, isattach ? "Attach":"Order");*/
		if (isattach)
		{
			tmp->client = cli->next;
			free(cli);			/* delete the node when it is found as a first client.  */
		}
		return 1;
	}
#if 0
	if (isattach)
		add_client_waitinglist(edev);	/* insert another client into waitinglist  */
#endif
	return 0;
}

/***************************************************************************
 * FUNCTION       :       usbip_stub_reserve_device()
 * AUTHOR          :        TengFei
 * OCCASION      :       
 * DESCRIPTION  :      reserver device
 *
 * INPUT             :       N/A
 * OUTPUT          :       N/A
 * RETURN          :       0: success; -1, -2, -3: error
 ***************************************************************************/
int usbip_stub_reserve_device(struct usbip_exported_device *edev)
{
	int ret; 
	
	ret = add_client_waitinglist(edev);
	
	if(edev->status != SDEV_ST_AVAILABLE) 
	{
		info("device not available, %s", edev->udev.busid);
		if (edev->status == SDEV_ST_USED)
		{
			return -1;
		}
		return -2;
	}
	else	 if (!is_first_client(edev, 0))	/* 0 means that it just inqures device */
		return -3;
	
	return 0;
}

int usbip_stub_export_device(struct usbip_exported_device *edev, int sockfd)
{
	char attrpath[SYSFS_PATH_MAX];
	struct sysfs_attribute *attr;
	char sockfd_buff[30];
	int ret; 


	if(edev->status != SDEV_ST_AVAILABLE) 
	{
		info("device not available, %s", edev->udev.busid);
		if (edev->status == SDEV_ST_USED)
		{
			ret = add_client_waitinglist(edev);
			if (ret)
				return -1;
		}
		return -2;
	}
	else	 
	{	/* added by tf, 110513 */
		if (!is_first_client(edev, 1))	/* 1 means it will attach device */
			return -3;
	}

	/* only the first interface is true */
	snprintf(attrpath, sizeof(attrpath), "%s/%s:%d.%d/%s",
			edev->udev.path,
			edev->udev.busid,
			edev->udev.bConfigurationValue, 0,
			"usbip_sockfd");

	attr = sysfs_open_attribute(attrpath);
	if(!attr) {
		err("open %s", attrpath);
		return -4;
	}

	snprintf(sockfd_buff, sizeof(sockfd_buff), "%d\n", sockfd);

	dbg("write: %s", sockfd_buff);

	ret = sysfs_write_attribute(attr, sockfd_buff, strlen(sockfd_buff));
	if(ret < 0) {
		err("write sockfd %s to %s", sockfd_buff, attrpath);
		goto err_write_sockfd;
	}

	info("connect %s", edev->udev.busid);

err_write_sockfd:
	sysfs_close_attribute(attr);

	return ret;
}

struct usbip_exported_device *usbip_stub_get_device(struct usbip_stub_driver *driver,
		int num)
{
	struct usbip_exported_device *edev; 
	struct dlist		*dlist = driver->edev_list;
	int count = 0;

	dlist_for_each_data(dlist, edev, struct usbip_exported_device) {
		if(num == count)
			return edev;
		else
			count++ ;
	}

	return NULL;
}

