/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright, Ralink Technology, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 ***************************************************************************
 *
 */
#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/netdevice.h>
#include <linux/fs.h>
#include <linux/sched.h>
#ifdef CONFIG_RALINK_GPIO_LED
#include <linux/timer.h>
#endif
#include <asm/uaccess.h>
#include "ralink_gpio.h"

#include <asm/rt2880/surfboardint.h>

#ifdef  CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
static  devfs_handle_t devfs_handle;
#endif
#include <linux/proc_fs.h>

#include <linux/rtnetlink.h>
#include <net/rtnetlink.h>

#define NAME			"ralink_gpio"
#define RALINK_GPIO_DEVNAME	"gpio"
int ralink_gpio_major = 252;
int ralink_gpio_irqnum = 0;
u32 ralink_gpio_intp = 0;
u32 ralink_gpio_edge = 0;
#if defined (RALINK_GPIO_HAS_2722)
u32 ralink_gpio2722_intp = 0;
u32 ralink_gpio2722_edge = 0;
#elif defined (RALINK_GPIO_HAS_4524)
u32 ralink_gpio3924_intp = 0;
u32 ralink_gpio3924_edge = 0;
u32 ralink_gpio4540_intp = 0;
u32 ralink_gpio4540_edge = 0;
#elif defined (RALINK_GPIO_HAS_5124)
u32 ralink_gpio3924_intp = 0;
u32 ralink_gpio3924_edge = 0;
u32 ralink_gpio5140_intp = 0;
u32 ralink_gpio5140_edge = 0;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
u32 ralink_gpio3924_intp = 0;
u32 ralink_gpio3924_edge = 0;
u32 ralink_gpio7140_intp = 0;
u32 ralink_gpio7140_edge = 0;
#if defined (RALINK_GPIO_HAS_7224)
u32 ralink_gpio72_intp = 0;
u32 ralink_gpio72_edge = 0;
#else
u32 ralink_gpio9572_intp = 0;
u32 ralink_gpio9572_edge = 0;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
u32 ralink_gpio6332_intp = 0;
u32 ralink_gpio6332_edge = 0;
u32 ralink_gpio9564_intp = 0;
u32 ralink_gpio9564_edge = 0;
#endif
ralink_gpio_reg_info ralink_gpio_info[RALINK_GPIO_NUMBER];
extern unsigned long volatile jiffies;

#ifdef CONFIG_RALINK_GPIO_LED
#define RALINK_LED_DEBUG 0
#define RALINK_GPIO_LED_FREQ (HZ/5)

#if defined(CONFIG_TP_MODEL_WR841HPV5)
#define RESET_TRIGGER (5*5)
#define WLAN_LOCK_TIMES (8*5)
#else
#define RESET_TRIGGER (5*5)  //Modified by chz from (5*3) to (5*5), we use 5 seconds to reset. 2017-6-7
#ifdef CONFIG_TP_CONFIG_RESET /* Press reset button restore to ISP Config. Added by Wang Yang. 2016-12-15 */
#ifndef CONFIG_ISP_RESET_TRIGGER

#define CONFIG_ISP_RESET_TRIGGER (5*30)
#endif
#endif /* end add. */
#define WLAN_LOCK_TIMES (5*5)
#endif

#define WLAN_TRIGGER (5*2)

struct timer_list ralink_gpio_led_timer;
ralink_gpio_led_info ralink_gpio_led_data[RALINK_GPIO_NUMBER];

u32 ra_gpio_led_set = 0;
u32 ra_gpio_led_clr = 0;
#if defined (RALINK_GPIO_HAS_2722)
u32 ra_gpio2722_led_set = 0;
u32 ra_gpio2722_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_4524)
u32 ra_gpio3924_led_set = 0;
u32 ra_gpio3924_led_clr = 0;
u32 ra_gpio4540_led_set = 0;
u32 ra_gpio4540_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_5124)
u32 ra_gpio3924_led_set = 0;
u32 ra_gpio3924_led_clr = 0;
u32 ra_gpio5140_led_set = 0;
u32 ra_gpio5140_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
u32 ra_gpio3924_led_set = 0;
u32 ra_gpio3924_led_clr = 0;
u32 ra_gpio7140_led_set = 0;
u32 ra_gpio7140_led_clr = 0;
#if defined (RALINK_GPIO_HAS_7224)
u32 ra_gpio72_led_set = 0;
u32 ra_gpio72_led_clr = 0;
#else
u32 ra_gpio9572_led_set = 0;
u32 ra_gpio9572_led_clr = 0;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
u32 ra_gpio6332_led_set = 0;
u32 ra_gpio6332_led_clr = 0;
u32 ra_gpio9564_led_set = 0;
u32 ra_gpio9564_led_clr = 0;
#endif
struct ralink_gpio_led_status_t {
	int ticks;
	unsigned int ons;
	unsigned int offs;
	unsigned int resting;
	unsigned int times;
} ralink_gpio_led_stat[RALINK_GPIO_NUMBER];
#endif
void ralink_gpio_notify_user(int usr);
static struct work_struct gpio_event_hold;
static struct work_struct gpio_event_click;

static struct proc_dir_entry *simple_config_entry = NULL;

void send_gpiobtn_event_to_user(char *buf, int len);

MODULE_DESCRIPTION("Ralink SoC GPIO Driver");
MODULE_AUTHOR("Winfred Lu <winfred_lu@ralinktech.com.tw>");
MODULE_LICENSE("GPL");
ralink_gpio_reg_info info;

//void send_wlanSwitch_to_user(char *buf, int len);

typedef enum _ENUM_MULTIMODE
{
    MULTIMODE_NONE_MODE = -1,
    MULTIMODE_ROUTER_MODE = 0,
    MULTIMODE_AP_MODE,
    MULTIMODE_CLIENT_MODE,
    MULTIMODE_REPEATER_MODE,
    MULTIMODE_MSSID_MODE,
    MULTIMODE_WISP_MODE,
    MULTIMODE_NUM,
}ENUM_MULTIMODE;

static int wlan_24G_status = 0;
static int wlan_5G_status = 0;
static int wan_ipv4_status = 0;
static int wan_ipv6_status = 0;
static int wan_status = 0;
static int sys_status = 0;
static int led_status = 1;
static int wl_mode = MULTIMODE_ROUTER_MODE; 
static int onekey_re_pressed = 0;
static int onekey_re_event_sent = 0;
/* Added by KangYi on 1st Nov 2017, for button testing when manufacuring */
static int manufacture_flag = 0;
static int button_wlan_pressed = 0;
static int button_reset_pressed = 0;
static int button_wps_pressed = 0;
static int tp_wps_success_flag = 0;
/* Added end */

void gpio_click_notify(struct work_struct *work)
{
    //printk("<hua-dbg> %s, 1\n", __FUNCTION__);
    ralink_gpio_notify_user(1);
}


void gpio_hold_notify(struct work_struct *work)
{
    //printk("<hua-dbg> %s, 2\n", __FUNCTION__);
    ralink_gpio_notify_user(2);
}


int ralink_gpio_led_set(ralink_gpio_led_info led)
{
#ifdef CONFIG_RALINK_GPIO_LED
	unsigned long tmp;
	if (0 <= led.gpio && led.gpio < RALINK_GPIO_NUMBER) {
		if (led.on > RALINK_GPIO_LED_INFINITY)
			led.on = RALINK_GPIO_LED_INFINITY;
		if (led.off > RALINK_GPIO_LED_INFINITY)
			led.off = RALINK_GPIO_LED_INFINITY;
		if (led.blinks > RALINK_GPIO_LED_INFINITY)
			led.blinks = RALINK_GPIO_LED_INFINITY;
		if (led.rests > RALINK_GPIO_LED_INFINITY)
			led.rests = RALINK_GPIO_LED_INFINITY;
		if (led.times > RALINK_GPIO_LED_INFINITY)
			led.times = RALINK_GPIO_LED_INFINITY;
		if (led.on == 0 && led.off == 0 && led.blinks == 0 &&
				led.rests == 0) {
			ralink_gpio_led_data[led.gpio].gpio = -1; //stop it
			return 0;
		}
		//register led data
		ralink_gpio_led_data[led.gpio].gpio = led.gpio;
		ralink_gpio_led_data[led.gpio].on = (led.on == 0)? 1 : led.on;
		ralink_gpio_led_data[led.gpio].off = (led.off == 0)? 1 : led.off;
		ralink_gpio_led_data[led.gpio].blinks = (led.blinks == 0)? 1 : led.blinks;
		ralink_gpio_led_data[led.gpio].rests = (led.rests == 0)? 1 : led.rests;
		ralink_gpio_led_data[led.gpio].times = (led.times == 0)? 1 : led.times;

		//clear previous led status
		ralink_gpio_led_stat[led.gpio].ticks = -1;
		ralink_gpio_led_stat[led.gpio].ons = 0;
		ralink_gpio_led_stat[led.gpio].offs = 0;
		ralink_gpio_led_stat[led.gpio].resting = 0;
		ralink_gpio_led_stat[led.gpio].times = 0;

		printk("led=%d, on=%d, off=%d, blinks,=%d, reset=%d, time=%d\n",
				ralink_gpio_led_data[led.gpio].gpio,
				ralink_gpio_led_data[led.gpio].on,
				ralink_gpio_led_data[led.gpio].off,
				ralink_gpio_led_data[led.gpio].blinks,
				ralink_gpio_led_data[led.gpio].rests,
				ralink_gpio_led_data[led.gpio].times);
		//set gpio direction to 'out'
#if defined (RALINK_GPIO_HAS_2722)
		if (led.gpio <= 21) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
			tmp |= RALINK_GPIO(led.gpio);
			*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		}
		else {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722DIR));
			tmp |= RALINK_GPIO((led.gpio-22));
			*(volatile u32 *)(RALINK_REG_PIO2722DIR) = tmp;
		}
#elif defined (RALINK_GPIO_HAS_9532)
		if (led.gpio <= 31) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
			tmp |= RALINK_GPIO(led.gpio);
			*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		} else if (led.gpio <= 63) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
			tmp |= RALINK_GPIO((led.gpio-32));
			*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;
		} else {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564DIR));
			tmp |= RALINK_GPIO((led.gpio-64));
			*(volatile u32 *)(RALINK_REG_PIO9564DIR) = tmp;
		}
#else
		if (led.gpio <= 23) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
			tmp |= RALINK_GPIO(led.gpio);
			*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		}
#if defined (RALINK_GPIO_HAS_4524)
		else if (led.gpio <= 39) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
			tmp |= RALINK_GPIO((led.gpio-24));
			*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		}
		else {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540DIR));
			tmp |= RALINK_GPIO((led.gpio-40));
			*(volatile u32 *)(RALINK_REG_PIO4540DIR) = tmp;
		}
#elif defined (RALINK_GPIO_HAS_5124)
		else if (led.gpio <= 39) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
			tmp |= RALINK_GPIO((led.gpio-24));
			*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		}
		else {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140DIR));
			tmp |= RALINK_GPIO((led.gpio-40));
			*(volatile u32 *)(RALINK_REG_PIO5140DIR) = tmp;
		}
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
		else if (led.gpio <= 39) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
			tmp |= RALINK_GPIO((led.gpio-24));
			*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		}
		else if (led.gpio <= 71) {
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140DIR));
			tmp |= RALINK_GPIO((led.gpio-40));
			*(volatile u32 *)(RALINK_REG_PIO7140DIR) = tmp;
		}
		else {
#if defined (RALINK_GPIO_HAS_7224)
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72DIR));
			tmp |= RALINK_GPIO((led.gpio-72));
			*(volatile u32 *)(RALINK_REG_PIO72DIR) = tmp;
#else
			tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572DIR));
			tmp |= RALINK_GPIO((led.gpio-72));
			*(volatile u32 *)(RALINK_REG_PIO9572DIR) = tmp;
#endif
		}
#endif
#endif
#if RALINK_LED_DEBUG
		printk("dir_%x gpio_%d - %d %d %d %d %d\n", tmp,
				led.gpio, led.on, led.off, led.blinks,
				led.rests, led.times);
#endif
	}
	else {
		printk(KERN_ERR NAME ": gpio(%d) out of range\n", led.gpio);
		return -1;
	}
	return 0;
#else
	printk(KERN_ERR NAME ": gpio led support not built\n");
	return -1;
#endif
}
EXPORT_SYMBOL(ralink_gpio_led_set);

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
long ralink_gpio_ioctl(struct file *file, unsigned int req,
		unsigned long arg)
#else
int ralink_gpio_ioctl(struct inode *inode, struct file *file, unsigned int req,
		unsigned long arg)
#endif
{
	unsigned long tmp;
	ralink_gpio_reg_info info;
#ifdef CONFIG_RALINK_GPIO_LED
	ralink_gpio_led_info led;
#endif

	req &= RALINK_GPIO_DATA_MASK;

	switch(req) {
	case RALINK_GPIO_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIODIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		break;
	case RALINK_GPIO_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
		break;
	case RALINK_GPIO_READ: //RALINK_GPIO_READ_INT
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO_WRITE: //RALINK_GPIO_WRITE_INT
		*(volatile u32 *)(RALINK_REG_PIODATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO_SET: //RALINK_GPIO_SET_INT
		*(volatile u32 *)(RALINK_REG_PIOSET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO_CLEAR: //RALINK_GPIO_CLEAR_INT
		*(volatile u32 *)(RALINK_REG_PIORESET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO_ENABLE_INTP:
		*(volatile u32 *)(RALINK_REG_INTENA) = cpu_to_le32(RALINK_INTCTL_PIO);
		break;
	case RALINK_GPIO_DISABLE_INTP:
#if defined (CONFIG_RALINK_MT7621) || defined (CONFIG_RALINK_MT7628)
		*(volatile u32 *)(RALINK_REG_PIORENA) = 0;
		*(volatile u32 *)(RALINK_REG_PIOFENA) = 0;
		*(volatile u32 *)(RALINK_REG_PIO6332RENA) = 0;
		*(volatile u32 *)(RALINK_REG_PIO6332FENA) = 0;
		*(volatile u32 *)(RALINK_REG_PIO9564RENA) = 0;
		*(volatile u32 *)(RALINK_REG_PIO9564FENA) = 0;
#else
		*(volatile u32 *)(RALINK_REG_INTDIS) = cpu_to_le32(RALINK_INTCTL_PIO);
#endif
		break;
	case RALINK_GPIO_REG_IRQ:
		copy_from_user(&info, (ralink_gpio_reg_info *)arg, sizeof(info));
		if (0 <= info.irq && info.irq < RALINK_GPIO_NUMBER) {
			ralink_gpio_info[info.irq].pid = info.pid;
#if defined (RALINK_GPIO_HAS_2722)
			if (info.irq <= 21) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIORENA));
				tmp |= (0x1 << info.irq);
				*(volatile u32 *)(RALINK_REG_PIORENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOFENA));
				tmp |= (0x1 << info.irq);
				*(volatile u32 *)(RALINK_REG_PIOFENA) = cpu_to_le32(tmp);
			}
			else {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722RENA));
				tmp |= (0x1 << (info.irq-22));
				*(volatile u32 *)(RALINK_REG_PIO2722RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722FENA));
				tmp |= (0x1 << (info.irq-22));
				*(volatile u32 *)(RALINK_REG_PIO2722FENA) = cpu_to_le32(tmp);
			}
#elif defined (RALINK_GPIO_HAS_9532)
			if (info.irq <= 31) {
#if defined (CONFIG_FB_MEDIATEK_ILITEK) || defined (CONFIG_FB_MEDIATEK_TRULY)&& defined (CONFIG_RALINK_MT7621)
				if(info.irq !=10){
					tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIORENA));
					tmp |= (0x1 << info.irq);
					*(volatile u32 *)(RALINK_REG_PIORENA) = cpu_to_le32(tmp);
					tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOFENA));
					tmp |= (0x1 << info.irq);
					*(volatile u32 *)(RALINK_REG_PIOFENA) = cpu_to_le32(tmp);
				}else if (info.irq ==10){
					tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOFENA));
					tmp |= (0x1 << info.irq);
					*(volatile u32 *)(RALINK_REG_PIOFENA) = cpu_to_le32(tmp);
				}
#else
					tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIORENA));
					tmp |= (0x1 << info.irq);
					*(volatile u32 *)(RALINK_REG_PIORENA) = cpu_to_le32(tmp);
					tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOFENA));
					tmp |= (0x1 << info.irq);
					*(volatile u32 *)(RALINK_REG_PIOFENA) = cpu_to_le32(tmp);
#endif
			} else if (info.irq <= 63) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332RENA));
				tmp |= (0x1 << (info.irq-32));
				*(volatile u32 *)(RALINK_REG_PIO6332RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332FENA));
				tmp |= (0x1 << (info.irq-32));
				*(volatile u32 *)(RALINK_REG_PIO6332FENA) = cpu_to_le32(tmp);
			} else {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564RENA));
				tmp |= (0x1 << (info.irq-64));
				*(volatile u32 *)(RALINK_REG_PIO9564RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564FENA));
				tmp |= (0x1 << (info.irq-64));
				*(volatile u32 *)(RALINK_REG_PIO9564FENA) = cpu_to_le32(tmp);
			}
#else
			if (info.irq <= 23) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIORENA));
				tmp |= (0x1 << info.irq);
				*(volatile u32 *)(RALINK_REG_PIORENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOFENA));
				tmp |= (0x1 << info.irq);
				*(volatile u32 *)(RALINK_REG_PIOFENA) = cpu_to_le32(tmp);
			}
#if defined (RALINK_GPIO_HAS_4524)
			else if (info.irq <= 39) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924RENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924FENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924FENA) = cpu_to_le32(tmp);
			}
			else {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540RENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO4540RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540FENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO4540FENA) = cpu_to_le32(tmp);
			}
#elif defined (RALINK_GPIO_HAS_5124)
			else if (info.irq <= 39) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924RENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924FENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924FENA) = cpu_to_le32(tmp);
			}
			else {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140RENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO5140RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140FENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO5140FENA) = cpu_to_le32(tmp);
			}
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
			else if (info.irq <= 39) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924RENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924FENA));
				tmp |= (0x1 << (info.irq-24));
				*(volatile u32 *)(RALINK_REG_PIO3924FENA) = cpu_to_le32(tmp);
			}
			else if (info.irq <= 71) {
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140RENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO7140RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140FENA));
				tmp |= (0x1 << (info.irq-40));
				*(volatile u32 *)(RALINK_REG_PIO7140FENA) = cpu_to_le32(tmp);
			}
			else {
#if defined (RALINK_GPIO_HAS_7224)
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72RENA));
				tmp |= (0x1 << (info.irq-72));
				*(volatile u32 *)(RALINK_REG_PIO72RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72FENA));
				tmp |= (0x1 << (info.irq-72));
				*(volatile u32 *)(RALINK_REG_PIO72FENA) = cpu_to_le32(tmp);
#else
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572RENA));
				tmp |= (0x1 << (info.irq-72));
				*(volatile u32 *)(RALINK_REG_PIO9572RENA) = cpu_to_le32(tmp);
				tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572FENA));
				tmp |= (0x1 << (info.irq-72));
				*(volatile u32 *)(RALINK_REG_PIO9572FENA) = cpu_to_le32(tmp);
#endif
			}
#endif
#endif
		}
		else
			printk(KERN_ERR NAME ": irq number(%d) out of range\n",
					info.irq);
		break;

#if defined (RALINK_GPIO_HAS_2722)
	case RALINK_GPIO2722_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO2722DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO2722_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO2722DIR) = tmp;
		break;
	case RALINK_GPIO2722_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO2722DIR) = tmp;
		break;
	case RALINK_GPIO2722_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO2722_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO2722DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO2722_SET:
		*(volatile u32 *)(RALINK_REG_PIO2722SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO2722_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO2722SET) = cpu_to_le32(arg);
		break;
#elif defined (RALINK_GPIO_HAS_9532)
	case RALINK_GPIO6332_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO6332DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO6332_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;
		break;
	case RALINK_GPIO6332_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;
		break;
	case RALINK_GPIO6332_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO6332_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO6332DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO6332_SET:
		*(volatile u32 *)(RALINK_REG_PIO6332SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO6332_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO6332SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9564_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO9564DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9564_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO9564DIR) = tmp;
		break;
	case RALINK_GPIO9564_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO9564DIR) = tmp;
		break;
	case RALINK_GPIO9564_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO9564_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO9564DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9564_SET:
		*(volatile u32 *)(RALINK_REG_PIO9564SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9564_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO9564SET) = cpu_to_le32(arg);
		break;
#elif defined (RALINK_GPIO_HAS_4524)
	case RALINK_GPIO3924_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO3924_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO3924DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;

	case RALINK_GPIO4540_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO4540DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO4540_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO4540DIR) = tmp;
		break;
	case RALINK_GPIO4540_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO4540DIR) = tmp;
		break;
	case RALINK_GPIO4540_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO4540_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO4540DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO4540_SET:
		*(volatile u32 *)(RALINK_REG_PIO4540SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO4540_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO4540SET) = cpu_to_le32(arg);
		break;
#elif defined (RALINK_GPIO_HAS_5124)
	case RALINK_GPIO3924_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO3924_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO3924DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;

	case RALINK_GPIO5140_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO5140DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO5140_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO5140DIR) = tmp;
		break;
	case RALINK_GPIO5140_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO5140DIR) = tmp;
		break;
	case RALINK_GPIO5140_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO5140_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO5140DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO5140_SET:
		*(volatile u32 *)(RALINK_REG_PIO5140SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO5140_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO5140SET) = cpu_to_le32(arg);
		break;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	case RALINK_GPIO3924_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO3924DIR) = tmp;
		break;
	case RALINK_GPIO3924_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO3924_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO3924DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_SET:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO3924_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO3924SET) = cpu_to_le32(arg);
		break;

	case RALINK_GPIO7140_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO7140DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO7140_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO7140DIR) = tmp;
		break;
	case RALINK_GPIO7140_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO7140DIR) = tmp;
		break;
	case RALINK_GPIO7140_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO7140_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO7140DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO7140_SET:
		*(volatile u32 *)(RALINK_REG_PIO7140SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO7140_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO7140SET) = cpu_to_le32(arg);
		break;
#if defined (RALINK_GPIO_HAS_7224)
	case RALINK_GPIO72_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO72DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO72_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO72DIR) = tmp;
		break;
	case RALINK_GPIO72_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO72DIR) = tmp;
		break;
	case RALINK_GPIO72_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO72_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO72DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO72_SET:
		*(volatile u32 *)(RALINK_REG_PIO72SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO72_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO72SET) = cpu_to_le32(arg);
		break;
#else
	case RALINK_GPIO9572_SET_DIR:
		*(volatile u32 *)(RALINK_REG_PIO9572DIR) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9572_SET_DIR_IN:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572DIR));
		tmp &= ~cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO9572DIR) = tmp;
		break;
	case RALINK_GPIO9572_SET_DIR_OUT:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572DIR));
		tmp |= cpu_to_le32(arg);
		*(volatile u32 *)(RALINK_REG_PIO9572DIR) = tmp;
		break;
	case RALINK_GPIO9572_READ:
		tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572DATA));
		put_user(tmp, (int __user *)arg);
		break;
	case RALINK_GPIO9572_WRITE:
		*(volatile u32 *)(RALINK_REG_PIO9572DATA) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9572_SET:
		*(volatile u32 *)(RALINK_REG_PIO9572SET) = cpu_to_le32(arg);
		break;
	case RALINK_GPIO9572_CLEAR:
		*(volatile u32 *)(RALINK_REG_PIO9572SET) = cpu_to_le32(arg);
		break;
#endif
#endif

	case RALINK_GPIO_LED_SET:
#ifdef CONFIG_RALINK_GPIO_LED
		copy_from_user(&led, (ralink_gpio_led_info *)arg, sizeof(led));
		ralink_gpio_led_set(led);
#else
		printk(KERN_ERR NAME ": gpio led support not built\n");
#endif
		break;
	default:
		return -ENOIOCTLCMD;
	}
	return 0;
}

int ralink_gpio_open(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	MOD_INC_USE_COUNT;
#else
	try_module_get(THIS_MODULE);
#endif
    INIT_WORK(&gpio_event_hold, gpio_hold_notify);
    INIT_WORK(&gpio_event_click, gpio_click_notify);
	return 0;
}

int ralink_gpio_release(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	MOD_DEC_USE_COUNT;
#else
	module_put(THIS_MODULE);
#endif
	return 0;
}

struct file_operations ralink_gpio_fops =
{
	owner:		THIS_MODULE,
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	unlocked_ioctl:	ralink_gpio_ioctl,
#else
	ioctl:		ralink_gpio_ioctl,
#endif
	open:		ralink_gpio_open,
	release:	ralink_gpio_release,
};

#ifdef CONFIG_RALINK_GPIO_LED

#if RALINK_GPIO_LED_LOW_ACT

#define __LED_ON(gpio)      ra_gpio_led_clr |= RALINK_GPIO(gpio);
#define __LED_OFF(gpio)     ra_gpio_led_set |= RALINK_GPIO(gpio);
#define __LED2722_ON(gpio)  ra_gpio2722_led_clr |= RALINK_GPIO((gpio-22));
#define __LED2722_OFF(gpio) ra_gpio2722_led_set |= RALINK_GPIO((gpio-22));
#define __LED3924_ON(gpio)  ra_gpio3924_led_clr |= RALINK_GPIO((gpio-24));
#define __LED3924_OFF(gpio) ra_gpio3924_led_set |= RALINK_GPIO((gpio-24));
#define __LED4540_ON(gpio)  ra_gpio4540_led_clr |= RALINK_GPIO((gpio-40));
#define __LED4540_OFF(gpio) ra_gpio4540_led_set |= RALINK_GPIO((gpio-40));
#define __LED5140_ON(gpio)  ra_gpio5140_led_clr |= RALINK_GPIO((gpio-40));
#define __LED5140_OFF(gpio) ra_gpio5140_led_set |= RALINK_GPIO((gpio-40));
#define __LED7140_ON(gpio)  ra_gpio7140_led_clr |= RALINK_GPIO((gpio-40));
#define __LED7140_OFF(gpio) ra_gpio7140_led_set |= RALINK_GPIO((gpio-40));

#if defined (RALINK_GPIO_HAS_7224)
#define __LED72_ON(gpio)  ra_gpio72_led_clr |= RALINK_GPIO((gpio-72));
#define __LED72_OFF(gpio) ra_gpio72_led_set |= RALINK_GPIO((gpio-72));
#else
#define __LED9572_ON(gpio)  ra_gpio9572_led_clr |= RALINK_GPIO((gpio-72));
#define __LED9572_OFF(gpio) ra_gpio9572_led_set |= RALINK_GPIO((gpio-72));
#endif

#if defined (RALINK_GPIO_HAS_9532)
#define __LED6332_ON(gpio)  ra_gpio6332_led_clr |= RALINK_GPIO((gpio-32));
#define __LED6332_OFF(gpio) ra_gpio6332_led_set |= RALINK_GPIO((gpio-32));
#define __LED9564_ON(gpio)  ra_gpio9564_led_clr |= RALINK_GPIO((gpio-64));
#define __LED9564_OFF(gpio) ra_gpio9564_led_set |= RALINK_GPIO((gpio-64));
#endif

#else

#define __LED_ON(gpio)      ra_gpio_led_set |= RALINK_GPIO(gpio);
#define __LED_OFF(gpio)     ra_gpio_led_clr |= RALINK_GPIO(gpio);
#define __LED2722_ON(gpio)  ra_gpio2722_led_set |= RALINK_GPIO((gpio-22));
#define __LED2722_OFF(gpio) ra_gpio2722_led_clr |= RALINK_GPIO((gpio-22));
#define __LED3924_ON(gpio)  ra_gpio3924_led_set |= RALINK_GPIO((gpio-24));
#define __LED3924_OFF(gpio) ra_gpio3924_led_clr |= RALINK_GPIO((gpio-24));
#define __LED4540_ON(gpio)  ra_gpio4540_led_set |= RALINK_GPIO((gpio-40));
#define __LED4540_OFF(gpio) ra_gpio4540_led_clr |= RALINK_GPIO((gpio-40));
#define __LED5140_ON(gpio)  ra_gpio5140_led_set |= RALINK_GPIO((gpio-40));
#define __LED5140_OFF(gpio) ra_gpio5140_led_clr |= RALINK_GPIO((gpio-40));
#define __LED7140_ON(gpio)  ra_gpio7140_led_set |= RALINK_GPIO((gpio-40));
#define __LED7140_OFF(gpio) ra_gpio7140_led_clr |= RALINK_GPIO((gpio-40));
#if defined (RALINK_GPIO_HAS_7224)
#define __LED72_ON(gpio)  ra_gpio72_led_set |= RALINK_GPIO((gpio-72));
#define __LED72_OFF(gpio) ra_gpio72_led_clr |= RALINK_GPIO((gpio-72));
#else
#define __LED9572_ON(gpio)  ra_gpio9572_led_set |= RALINK_GPIO((gpio-72));
#define __LED9572_OFF(gpio) ra_gpio9572_led_clr |= RALINK_GPIO((gpio-72));
#endif

#if defined (RALINK_GPIO_HAS_9532)
#define __LED6332_ON(gpio)  ra_gpio6332_led_set |= RALINK_GPIO((gpio-32));
#define __LED6332_OFF(gpio) ra_gpio6332_led_clr |= RALINK_GPIO((gpio-32));
#define __LED9564_ON(gpio)  ra_gpio9564_led_set |= RALINK_GPIO((gpio-64));
#define __LED9564_OFF(gpio) ra_gpio9564_led_clr |= RALINK_GPIO((gpio-64));
#endif


#endif
/* add for c2 & c20i, yuanshang, 2013-11-14 */
static void pollingGpio(void);
/* add end */

static void ralink_gpio_led_do_timer(unsigned long unused)
#if 1
		{
			pollingGpio();
			
			init_timer(&ralink_gpio_led_timer);
			ralink_gpio_led_timer.expires = jiffies + RALINK_GPIO_LED_FREQ;
			add_timer(&ralink_gpio_led_timer);
		}
#else
{
	int i;
	unsigned int x;

#if defined (RALINK_GPIO_HAS_2722)
	for (i = 0; i < 22; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 22; i <  RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED2722_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED2722_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED2722_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED2722_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED2722_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED2722_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#else
#if defined (RALINK_GPIO_HAS_9532)
	for (i = 0; i < 32; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1){ //-1 means unused	
			continue;
		}
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED_ON(i);	
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED_OFF(i);	
			continue;
		}	
		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}	
	
	
	#else
	for (i = 0; i < 24; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#endif
#if defined (RALINK_GPIO_HAS_4524)
	for (i = 24; i < 40; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED3924_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED3924_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED3924_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED3924_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED3924_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED3924_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 40; i <  RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED4540_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED4540_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED4540_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED4540_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED4540_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED4540_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#elif defined (RALINK_GPIO_HAS_5124)
	for (i = 24; i < 40; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED3924_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED3924_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED3924_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED3924_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED3924_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED3924_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 40; i < RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED5140_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED5140_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED5140_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED5140_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED5140_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED5140_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#elif defined (RALINK_GPIO_HAS_9532)
	for (i = 32; i < 64; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED6332_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED6332_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED6332_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED6332_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED6332_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED6332_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 64; i < RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED9564_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED9564_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED9564_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED9564_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED9564_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED9564_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	for (i = 24; i < 40; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED3924_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED3924_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED3924_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED3924_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED3924_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED3924_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 40; i < 72; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
			__LED7140_ON(i);
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
			__LED7140_OFF(i);
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
			__LED7140_ON(i);
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
			__LED7140_OFF(i);
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
			__LED7140_OFF(i);
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
				__LED7140_OFF(i);
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}

	for (i = 72; i < RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_led_stat[i].ticks++;
		if (ralink_gpio_led_data[i].gpio == -1) //-1 means unused
			continue;
		if (ralink_gpio_led_data[i].on == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].off == 0) { //always on
#if defined (RALINK_GPIO_HAS_7224)
			__LED72_ON(i);
#else
			__LED9572_ON(i);
#endif
			continue;
		}
		if (ralink_gpio_led_data[i].off == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].on == 0 ||
				ralink_gpio_led_data[i].blinks == 0 ||
				ralink_gpio_led_data[i].times == 0) { //always off
#if defined (RALINK_GPIO_HAS_7224)
			__LED72_OFF(i);
#else
			__LED9572_OFF(i);
#endif
			continue;
		}

		//led turn on or off
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			x = ralink_gpio_led_stat[i].ticks % (ralink_gpio_led_data[i].on
					+ ralink_gpio_led_data[i].off);
		}
		else {
			unsigned int a, b, c, d, o, t;
			a = ralink_gpio_led_data[i].blinks / 2;
			b = ralink_gpio_led_data[i].rests / 2;
			c = ralink_gpio_led_data[i].blinks % 2;
			d = ralink_gpio_led_data[i].rests % 2;
			o = ralink_gpio_led_data[i].on + ralink_gpio_led_data[i].off;
			//t = blinking ticks
			t = a * o + ralink_gpio_led_data[i].on * c;
			//x = ticks % (blinking ticks + resting ticks)
			x = ralink_gpio_led_stat[i].ticks %
				(t + b * o + ralink_gpio_led_data[i].on * d);
			//starts from 0 at resting cycles
			if (x >= t)
				x -= t;
			x %= o;
		}
		if (x < ralink_gpio_led_data[i].on) {
#if defined (RALINK_GPIO_HAS_7224)
			__LED72_ON(i);
#else
			__LED9572_ON(i);
#endif
			if (ralink_gpio_led_stat[i].ticks && x == 0)
				ralink_gpio_led_stat[i].offs++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d on,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}
		else {
#if defined (RALINK_GPIO_HAS_7224)
			__LED72_OFF(i);
#else
			__LED9572_OFF(i);
#endif
			if (x == ralink_gpio_led_data[i].on)
				ralink_gpio_led_stat[i].ons++;
#if RALINK_LED_DEBUG
			printk("t%d gpio%d off,", ralink_gpio_led_stat[i].ticks, i);
#endif
		}

		//blinking or resting
		if (ralink_gpio_led_data[i].blinks == RALINK_GPIO_LED_INFINITY ||
				ralink_gpio_led_data[i].rests == 0) { //always blinking
			continue;
		}
		else {
			x = ralink_gpio_led_stat[i].ons + ralink_gpio_led_stat[i].offs;
			if (!ralink_gpio_led_stat[i].resting) {
				if (x == ralink_gpio_led_data[i].blinks) {
					ralink_gpio_led_stat[i].resting = 1;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
					ralink_gpio_led_stat[i].times++;
				}
			}
			else {
				if (x == ralink_gpio_led_data[i].rests) {
					ralink_gpio_led_stat[i].resting = 0;
					ralink_gpio_led_stat[i].ons = 0;
					ralink_gpio_led_stat[i].offs = 0;
				}
			}
		}
		if (ralink_gpio_led_stat[i].resting) {
#if defined (RALINK_GPIO_HAS_7224)
			__LED72_OFF(i);
#else
			__LED9572_OFF(i);
#endif
#if RALINK_LED_DEBUG
			printk("resting,");
		} else {
			printk("blinking,");
#endif
		}

		//number of times
		if (ralink_gpio_led_data[i].times != RALINK_GPIO_LED_INFINITY)
		{
			if (ralink_gpio_led_stat[i].times ==
					ralink_gpio_led_data[i].times) {
#if defined (RALINK_GPIO_HAS_7224)
				__LED72_OFF(i);
#else
				__LED9572_OFF(i);
#endif
				ralink_gpio_led_data[i].gpio = -1; //stop
			}
#if RALINK_LED_DEBUG
			printk("T%d\n", ralink_gpio_led_stat[i].times);
		} else {
			printk("T@\n");
#endif
		}
	}
#endif
#endif

	//always turn the power LED on
#ifdef CONFIG_RALINK_RT2880
	__LED_ON(12);
#elif defined (CONFIG_RALINK_RT3052) || defined (CONFIG_RALINK_RT2883)
	__LED_ON(9);
#endif

	*(volatile u32 *)(RALINK_REG_PIORESET) = ra_gpio_led_clr;
	*(volatile u32 *)(RALINK_REG_PIOSET) = ra_gpio_led_set;
#if defined (RALINK_GPIO_HAS_2722)
	*(volatile u32 *)(RALINK_REG_PIO2722RESET) = ra_gpio2722_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO2722SET) = ra_gpio2722_led_set;
#elif defined (RALINK_GPIO_HAS_4524)
	*(volatile u32 *)(RALINK_REG_PIO3924RESET) = ra_gpio3924_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO3924SET) = ra_gpio3924_led_set;
	*(volatile u32 *)(RALINK_REG_PIO4540RESET) = ra_gpio4540_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO4540SET) = ra_gpio4540_led_set;
#elif defined (RALINK_GPIO_HAS_5124)
	*(volatile u32 *)(RALINK_REG_PIO3924RESET) = ra_gpio3924_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO3924SET) = ra_gpio3924_led_set;
	*(volatile u32 *)(RALINK_REG_PIO5140RESET) = ra_gpio5140_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO5140SET) = ra_gpio5140_led_set;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	*(volatile u32 *)(RALINK_REG_PIO3924RESET) = ra_gpio3924_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO3924SET) = ra_gpio3924_led_set;
	*(volatile u32 *)(RALINK_REG_PIO7140RESET) = ra_gpio7140_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO7140SET) = ra_gpio7140_led_set;
#if defined (RALINK_GPIO_HAS_7224)
	*(volatile u32 *)(RALINK_REG_PIO72RESET) = ra_gpio72_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO72SET) = ra_gpio72_led_set;
#else
	*(volatile u32 *)(RALINK_REG_PIO9572RESET) = ra_gpio9572_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO9572SET) = ra_gpio9572_led_set;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	*(volatile u32 *)(RALINK_REG_PIO6332RESET) = ra_gpio6332_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO6332SET) = ra_gpio6332_led_set;
	*(volatile u32 *)(RALINK_REG_PIO9564RESET) = ra_gpio9564_led_clr;
	*(volatile u32 *)(RALINK_REG_PIO9564SET) = ra_gpio9564_led_set;
#endif

#if RALINK_LED_DEBUG
	printk("led_set= %x, led_clr= %x\n", ra_gpio_led_set, ra_gpio_led_clr);
#if defined (RALINK_GPIO_HAS_2722)
	printk("led2722_set= %x, led2722_clr= %x\n", ra_gpio2722_led_set, ra_gpio2722_led_clr);
#elif defined (RALINK_GPIO_HAS_4524)
	printk("led3924_set= %x, led3924_clr= %x\n", ra_gpio3924_led_set, ra_gpio3924_led_clr);
	printk("led4540_set= %x, led4540_clr= %x\n", ra_gpio4540_led_set, ra_gpio4540_led_set);
#elif defined (RALINK_GPIO_HAS_5124)
	printk("led3924_set= %x, led3924_clr= %x\n", ra_gpio3924_led_set, ra_gpio3924_led_clr);
	printk("led5140_set= %x, led5140_clr= %x\n", ra_gpio5140_led_set, ra_gpio5140_led_set);
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	printk("led3924_set= %x, led3924_clr= %x\n", ra_gpio3924_led_set, ra_gpio3924_led_clr);
	printk("led7140_set= %x, led7140_clr= %x\n", ra_gpio7140_led_set, ra_gpio7140_led_set);
#if defined (RALINK_GPIO_HAS_7224)
	printk("led72_set= %x, led72_clr= %x\n", ra_gpio72_led_set, ra_gpio72_led_set);
#else
	printk("led9572_set= %x, led9572_clr= %x\n", ra_gpio9572_led_set, ra_gpio9572_led_set);
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	printk("led6332_set= %x, led6332_clr= %x\n", ra_gpio6332_led_set, ra_gpio6332_led_clr);
	printk("led9564_set= %x, led9564_clr= %x\n", ra_gpio9564_led_set, ra_gpio9564_led_set);
#endif
#endif

	ra_gpio_led_set = ra_gpio_led_clr = 0;
#if defined (RALINK_GPIO_HAS_2722)
	ra_gpio2722_led_set = ra_gpio2722_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_4524)
	ra_gpio3924_led_set = ra_gpio3924_led_clr = 0;
	ra_gpio4540_led_set = ra_gpio4540_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_5124)
	ra_gpio3924_led_set = ra_gpio3924_led_clr = 0;
	ra_gpio5140_led_set = ra_gpio5140_led_clr = 0;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	ra_gpio3924_led_set = ra_gpio3924_led_clr = 0;
	ra_gpio7140_led_set = ra_gpio7140_led_clr = 0;
#if defined (RALINK_GPIO_HAS_7224)
	ra_gpio72_led_set = ra_gpio72_led_clr = 0;
#else
	ra_gpio9572_led_set = ra_gpio9572_led_clr = 0;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	ra_gpio6332_led_set = ra_gpio6332_led_clr = 0;
	ra_gpio9564_led_set = ra_gpio9564_led_clr = 0;
#endif

	init_timer(&ralink_gpio_led_timer);
	ralink_gpio_led_timer.expires = jiffies + RALINK_GPIO_LED_FREQ;
	add_timer(&ralink_gpio_led_timer);
}
#endif

void ralink_gpio_led_init_timer(void)
{
	int i;

	for (i = 0; i < RALINK_GPIO_NUMBER; i++)
		ralink_gpio_led_data[i].gpio = -1; //-1 means unused
#if RALINK_GPIO_LED_LOW_ACT
	ra_gpio_led_set = 0xffffffff;
#if defined (RALINK_GPIO_HAS_2722)
	ra_gpio2722_led_set = 0xff;
#elif defined (RALINK_GPIO_HAS_4524)
	ra_gpio3924_led_set = 0xffff;
	ra_gpio4540_led_set = 0xff;
#elif defined (RALINK_GPIO_HAS_5124)
	ra_gpio3924_led_set = 0xffff;
	ra_gpio5140_led_set = 0xfff;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	ra_gpio3924_led_set = 0xffff;
	ra_gpio7140_led_set = 0xffffffff;
#if defined (RALINK_GPIO_HAS_7224)
	ra_gpio72_led_set = 0xffffff;
#else
	ra_gpio9572_led_set = 0xffffff;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	ra_gpio6332_led_set = 0xffffffff;
	ra_gpio9564_led_set = 0xffffffff;
#endif
#else // RALINK_GPIO_LED_LOW_ACT //
	ra_gpio_led_clr = 0xffffffff;
#if defined (RALINK_GPIO_HAS_2722)
	ra_gpio2722_led_clr = 0xff;
#elif defined (RALINK_GPIO_HAS_4524)
	ra_gpio3924_led_clr = 0xffff;
	ra_gpio4540_led_clr = 0xff;
#elif defined (RALINK_GPIO_HAS_5124)
	ra_gpio3924_led_clr = 0xffff;
	ra_gpio5140_led_clr = 0xfff;
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	ra_gpio3924_led_clr = 0xffff;
	ra_gpio7140_led_clr = 0xffffffff;
#if defined (RALINK_GPIO_HAS_7224)
	ra_gpio72_led_clr = 0xffffff;
#else
	ra_gpio9572_led_clr = 0xffffff;
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	ra_gpio6332_led_clr = 0xffffffff;
	ra_gpio9564_led_clr = 0xffffffff;
#endif
#endif // RALINK_GPIO_LED_LOW_ACT //

	init_timer(&ralink_gpio_led_timer);
	ralink_gpio_led_timer.function = ralink_gpio_led_do_timer;
	ralink_gpio_led_timer.expires = jiffies + RALINK_GPIO_LED_FREQ;
	add_timer(&ralink_gpio_led_timer);
}
#endif

#if defined(CONFIG_TP_MODEL_C5V4)
int setGpioData(u32 gpio, u32 data)
{
	u32 bit = 0;
	u32 reg = 0;
	u32 tmp = 0;
	/* Get reg and bit of the reg */
	if (gpio > 72)
	{
		printk(KERN_ERR NAME ": %s, Unsupport GPIO(%d)\n", __FUNCTION__, gpio);
		return -1;
	}
	if (gpio <= 23)
	{
		/* RALINK_REG_PIODATA for GPIO 0~23 */
		reg = RALINK_REG_PIODATA;
		bit = (1 << gpio);
	}
	else if (gpio <= 39)
	{
		/* RALINK_REG_PIO3924DATA for GPIO 24~39 */
		reg = RALINK_REG_PIO3924DATA;
		bit = (1 << (gpio - 24));
	}
	else if (gpio <= 71)
	{
		/* RALINK_REG_PIO7140DATA for GPIO 40~71 */
		reg = RALINK_REG_PIO7140DATA;
		bit = (1 << (gpio - 40));
	}
	else /* gpio 72 */
	{
		/* RALINK_REG_PIO72DATA for GPIO 72 */
		reg = RALINK_REG_PIO72DATA;
		bit = 1;
	}

	/* Set to reg base on bit and data */
	tmp = le32_to_cpu(*(volatile u32 *)(reg));
	if (0 == data)
	{
		tmp &= ~bit;
	}
	else
	{
		tmp |= bit;
	}
	*(volatile u32 *)(reg) = tmp;


#if 1 /* wait for storing data into register */
	{
		int i=0;
		while(i<1000 && *(volatile u32 *)(reg) != tmp)
			i++;
		if(i>=1000)
		{
			printk("setGpioData error: target=%x, actual=%x\r\n", 
				tmp, *(volatile u32 *)(reg));
		}
	}
#endif
	return 0;
}
int getGpioData(u32 gpio, u32 *data)
{
	u32 bit = 0;
	u32 reg = 0;
	u32 tmp = 0;
	/* Get reg and bit of the reg */
	if (gpio > 72)
	{
		printk(KERN_ERR NAME ": %s, Unsupport GPIO(%d)\n", __FUNCTION__, gpio);
		return -1;
	}
	if (gpio <= 23)
	{
		/* RALINK_REG_PIODATA for GPIO 0~23 */
		reg = RALINK_REG_PIODATA;
		bit = (1 << gpio);
	}
	else if (gpio <= 39)
	{
		/* RALINK_REG_PIO3924DATA for GPIO 24~39 */
		reg = RALINK_REG_PIO3924DATA;
		bit = (1 << (gpio - 24));
	}
	else if (gpio <= 71)
	{
		/* RALINK_REG_PIO7140DATA for GPIO 40~71 */
		reg = RALINK_REG_PIO7140DATA;
		bit = (1 << (gpio - 40));
	}
	else /* gpio 72 */
	{
		/* RALINK_REG_PIO72DATA for GPIO 72 */
		reg = RALINK_REG_PIO72DATA;
		bit = 1;
	}

	/* Get to reg base on bit */
	tmp = le32_to_cpu(*(volatile u32 *)(reg));
	if (bit & tmp)
	{
		*data = 1;
	}
	else
	{
		*data = 0;
	}
	return 0;
}

#else
int setGpioData(u32 gpio, u32 data)
{
	u32 bit = 0;
	u32 reg = 0;
	u32 tmp = 0;
	/* Get reg and bit of the reg */
	if (gpio > 95)
	{
		printk(KERN_ERR NAME ": %s, Unsupport GPIO(%d)\n", __FUNCTION__, gpio);
		return -1;
	}
	if (gpio <= 31)
	{
		/* RALINK_REG_PIODATA for GPIO 0~31 */
#if defined(CONFIG_TP_MODEL_C20V4)
	if(2 == gpio)
		reg = RALINK_SYSCTL_BASE + 0x3c;
	else
#endif
		reg = RALINK_REG_PIODATA;
		bit = (1 << gpio);
	}
	else if (gpio <= 63)
	{
		/* RALINK_REG_PIO3924DATA for GPIO 32~63 */
		reg = RALINK_REG_PIO6332DATA;
		bit = (1 << (gpio - 32));
	}
	else if (gpio <= 95)
	{
		/* RALINK_REG_PIO7140DATA for GPIO 64~95 */
		reg = RALINK_REG_PIO9564DATA;
		bit = (1 << (gpio - 64));
	}

	/* Set to reg base on bit and data */
	tmp = le32_to_cpu(*(volatile u32 *)(reg));
	if (0 == data)
	{
		tmp &= ~bit;
	}
	else
	{
		tmp |= bit;
	}
	*(volatile u32 *)(reg) = tmp;
	return 0;
}
int getGpioData(u32 gpio, u32 *data)
{
	u32 bit = 0;
	u32 reg = 0;
	u32 tmp = 0;
	/* Get reg and bit of the reg */
	if (gpio > 95)
	{
		printk(KERN_ERR NAME ": %s, Unsupport GPIO(%d)\n", __FUNCTION__, gpio);
		return -1;
	}
	if (gpio <= 31)
	{
		/* RALINK_REG_PIODATA for GPIO 0~31 */
		reg = RALINK_REG_PIODATA;
		bit = (1 << gpio);
	}
	else if (gpio <= 63)
	{
		/* RALINK_REG_PIO3924DATA for GPIO 32~63 */
		reg = RALINK_REG_PIO6332DATA;
		bit = (1 << (gpio - 32));
	}
	else if (gpio <= 95)
	{
		/* RALINK_REG_PIO7140DATA for GPIO 64~95 */
		reg = RALINK_REG_PIO9564DATA;
		bit = (1 << (gpio - 64));
	}

	/* Get to reg base on bit */
	tmp = le32_to_cpu(*(volatile u32 *)(reg));
	if (bit & tmp)
	{
		*data = 1;
	}
	else
	{
		*data = 0;
	}
	return 0;
}
#endif

enum GPIO_ACT_TYPE
{
	GPIO_ACT_OFF	= 0,
	GPIO_ACT_ON		= 1,
	GPIO_ACT_BLINK	= 2
};

enum GPIO_FREG_TYPE
{
	/* single bit num, like b0001, b0010, b0100, but not b0101 */
	GPIO_FREQ_NO	= 0,
	GPIO_FREQ_FAST	= 1,
	GPIO_FREQ_NORMAL= 4,
	GPIO_FREQ_SLOW = 8
};

typedef struct _TP_LED_CELL
{
	u32 gpioId;
	u32 gpioAction;
	u32 gpioFreq;
	
	u32 gpioStatus;
	u32 gpioTimes;
	
}TP_LED_CELL;



void initLedData(TP_LED_CELL *pGpio, u32 gpio);
void initLedData_ON(TP_LED_CELL *pGpio, u32 gpio);

/* Led numbers by GPIO */
#if defined(CONFIG_TP_MODEL_WR840NV4) || defined(CONFIG_TP_MODEL_WR850NV2) || \
	defined(CONFIG_TP_MODEL_WR850NV3) || defined(CONFIG_TP_MODEL_EN020_F5V1)

#define LED_NUM (5)
#elif defined(CONFIG_TP_MODEL_WR841NV13) || defined(CONFIG_TP_MODEL_WR845NV3)
#define LED_NUM (9)
#elif defined(CONFIG_TP_MODEL_C20V4)
enum GPIO_COMMON {
	GPIO_LED_POWER = 2,
	GPIO_LED_INTERNET_ORANGE = 11,
	GPIO_BUTTON_WLAN = 37,
	GPIO_BUTTON_RESET,
	GPIO_LED_WLAN_2G4,
	GPIO_LED_WLAN_5G,
	GPIO_LED_WPS,
	GPIO_LED_LAN,
	GPIO_LED_INTERNET_GREEN,
	GPIO_END
};

#define LED_NUM (7)
#elif defined(CONFIG_TP_MODEL_C50V3)
#define LED_NUM (7)
/* GPIO
 * GPIO11: POWER LED
 * GPIO35:WLAN 2.4G LED
 * GPIO38:WPS /RESET BUTTON
 * GPIO39:WPS LED
 * GPIO40:WLAN 5G LED
 * GPIO41:LAN LED
 * GPIO42:INTERNET ORANGE LED
 * GPIO43:INTERNET GREEN LED
 * GPIO5:WLAN BUTTON
 */
enum GPIO_COMMON {
	GPIO_BUTTON_WLAN = 5,
	GPIO_LED_POWER = 11,
	GPIO_BUTTON_RESET = 38,
	GPIO_LED_INTERNET_GREEN,
	GPIO_LED_INTERNET_ORANGE,
	GPIO_LED_LAN,
	GPIO_LED_WLAN_5G,
	GPIO_LED_WPS,
	GPIO_LED_WLAN_2G4,
};
#elif defined(CONFIG_TP_MODEL_WA801NDV5)
enum GPIO_COMMON {
	GPIO_LED_POWER = 36,
	GPIO_LED_LAN = 37,
	GPIO_BUTTON_RESET = 38,
	GPIO_LED_WPS_GREEN = 42,
	GPIO_LED_WPS_RED = 43,
	GPIO_LED_WLAN_2G4 = 44,
	GPIO_BUTTON_WPS = 46,
	GPIO_END,
};

#define STA_WPS_STATUS_SUPPORT
#define STA_WPS_SUPPORT

#define LED_NUM (7)

#elif defined(CONFIG_TP_MODEL_WR841HPV5)
 /*
  *
  * GPIO#11: INTERNET ORANGE LED
  * GPIO#36: POWER LED
  * GPIO#40: WPS LED
  * GPIO#41: RE LED
  * GPIO#42: LAN LED
  * GPIO#43: INTERNET WHITE LED
  * GPIO#44: WLAN LED
  *
  * GPIO#5:  WIFI Button
  * GPIO#37: RESET Button
  * GPIO#38: WPS Button
  * GPIO#39: RE Button
  *
  */
enum GPIO_COMMON {
	GPIO_LED_INTERNET_ORANGE = 11, /* ORANGE COLOR */
	GPIO_LED_POWER = 36,
	GPIO_LED_WPS = 40,
	GPIO_LED_RE = 41,
	GPIO_LED_LAN = 42,
	GPIO_LED_INTERNET_GREEN = 43, /* WHITE COLOR */
    GPIO_LED_WLAN_2G4 = 44,

	GPIO_BUTTON_WIFI = 5,
	GPIO_BUTTON_RESET = 37,
	GPIO_BUTTON_WPS = 38,
	GPIO_BUTTON_RE = 39
};

#define LED_NUM 7

#elif defined(CONFIG_TP_MODEL_WR802NV4)
enum GPIO_COMMON {
	GPIO_LED_POWER = 37,
	GPIO_BUTTON_RESET = 38,
	GPIO_END,
};

#define LED_NUM (1)

#elif defined(CONFIG_TP_MODEL_C2V5)
enum GPIO_COMMON {
	GPIO_LED_POWER = 11,
    GPIO_LED_WLAN_2G4 = 52,
    GPIO_LED_WLAN_5G = 53,
    GPIO_LED_LAN = 54,
    GPIO_LED_WPS = 55,
    GPIO_LED_INTERNET_ORANGE = 56,
    GPIO_LED_INTERNET_GREEN = 57,

    GPIO_BUTTON_RESET = 2,
    GPIO_BUTTON_WPS = 13,  /* WPS/WLAN BUTTON */

	GPIO_END
};

#define LED_NUM (7)

#elif defined(CONFIG_TP_MODEL_C5V4)
enum GPIO_COMMON {
	GPIO_LED_POWER = 42,
    GPIO_LED_WLAN_2G4 = 14,
	GPIO_LED_WLAN_5G = 12,
	GPIO_LED_LAN = 8,
	GPIO_LED_USB = 7,
	GPIO_LED_WPS = 1,
	GPIO_LED_INTERNET_ORANGE = 9,
	GPIO_LED_INTERNET_GREEN = 10,

	GPIO_BUTTON_RESET = 13,
	GPIO_BUTTON_WPS = 2,  /* WPS/WLAN BUTTON */
	
	GPIO_END
};

#define LED_NUM (8)
#endif
TP_LED_CELL c2_led[LED_NUM];

#if defined(CONFIG_TP_MODEL_C2V5)
#define RESET_BUTTON_GPIO (2)
#define WLAN_BUTTON_GPIO (13)  /* WPS/WLAN BUTTON */
#elif defined(CONFIG_TP_MODEL_C5V4)
#define RESET_BUTTON_GPIO (13)
#define WLAN_BUTTON_GPIO (2)
#else
#define RESET_BUTTON_GPIO (38)
#define WLAN_BUTTON_GPIO (37)
#endif

void initLedData_W8(void)
{
#if defined(CONFIG_TP_MODEL_WR840NV4) || defined(CONFIG_TP_MODEL_WR850NV2) || \
    defined(CONFIG_TP_MODEL_WR850NV3) || defined(CONFIG_TP_MODEL_EN020_F5V1)

	initLedData(&c2_led[0], 36);
	initLedData(&c2_led[1], 37);
	initLedData(&c2_led[2], 41);
	initLedData(&c2_led[3], 43);
	initLedData(&c2_led[4], 44);
#elif defined(CONFIG_TP_MODEL_WR841NV13) || defined(CONFIG_TP_MODEL_WR845NV3)
	initLedData(&c2_led[0], 11);
	initLedData(&c2_led[1], 36);
	initLedData(&c2_led[2], 39);
	initLedData(&c2_led[3], 40);
	initLedData(&c2_led[4], 41);
	initLedData(&c2_led[5], 42);
	initLedData(&c2_led[6], 43);
	initLedData(&c2_led[7], 44);
	initLedData(&c2_led[8], 46);
#elif defined(CONFIG_TP_MODEL_C20V4)
	initLedData(&c2_led[0], 2);
	initLedData(&c2_led[1], 11);
	initLedData(&c2_led[2], 39);
	initLedData(&c2_led[3], 40);
	initLedData(&c2_led[4], 41);
	initLedData(&c2_led[5], 42);
	initLedData(&c2_led[6], 43);
#elif defined(CONFIG_TP_MODEL_WA801NDV5)
	initLedData(&c2_led[0], 36);
	initLedData(&c2_led[1], 37);
	initLedData_ON(&c2_led[3], 42);
	initLedData_ON(&c2_led[4], 43);
	initLedData(&c2_led[5], 44);
#elif defined(CONFIG_TP_MODEL_WR802NV4)
	initLedData(&c2_led[0], 37);  
#elif defined(CONFIG_TP_MODEL_C2V5)
	initLedData(&c2_led[0], 11);
	initLedData(&c2_led[1], 52);
	initLedData(&c2_led[2], 53);
	initLedData(&c2_led[3], 54);
	initLedData(&c2_led[4], 55);
	initLedData(&c2_led[5], 56);
	initLedData(&c2_led[6], 57);
#elif defined(CONFIG_TP_MODEL_C5V4)
	initLedData(&c2_led[0], 42);
	initLedData(&c2_led[1], 14);
	initLedData(&c2_led[2], 12);
	initLedData(&c2_led[3], 8);
	initLedData(&c2_led[4], 7);
	initLedData(&c2_led[5], 1);
	initLedData(&c2_led[6], 9);
	initLedData(&c2_led[7], 10);
#endif
}

void initGpioDir_W8(void)
{
	u32 tmp;
#if defined(CONFIG_TP_MODEL_WR840NV4) || defined(CONFIG_TP_MODEL_WR850NV2) || \
    defined(CONFIG_TP_MODEL_WR850NV3) || defined(CONFIG_TP_MODEL_EN020_F5V1)


	/* OUTPUT GPIO
	 * GPIO36: Power
	 * GPIO37: WPS
	 * GPIO41: LAN
	 * GPIO43: WAN
	 * GPIO44: WLAN 2.4G
	 */
	/* Set Direction to output */

	/* RALINK_REG_PIO6332DIR for GPIO 32~63 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
	tmp |= (1 << (36-32)) | (1 << (37-32)) | (1 << (41-32)) | (1 << (43-32)) | (1 << (44-32));
	*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;

	/* INPUT GPIO
	 * GPIO38:RESET/WPS
	 */
	/* Set Direction to input */
	/* RALINK_REG_PIODIR for GPIO 0~23 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
	tmp &= ~((1 << (38-32)));
	*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;	
	
#elif defined(CONFIG_TP_MODEL_WR841NV13) || defined(CONFIG_TP_MODEL_WR845NV3)
	/* GPIO
	 * GPIO36: POWER
	 * GPIO46:WPS	
	 * GPIO38:RESET Button(WPS switch for 841v13)
 	 * GPIO39/40/41/42:LAN
 	 * GPIO43:INTERNET_GREEN
 	 * GPIO11:INTERNET_ORANGE
	 * GPIO44:WLAN(2.4G)
	 * GPIO37:WLAN ON/OFF (WPS switch for 845v3)
	 */

	/* Set Direction to output */
	/* GPIO CTRL 0 for GPIO 0~32 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
	tmp |= (1 << (11 - 0));
	*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
	
	/* RALINK_REG_PIO6332DIR for GPIO 32~63 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
	tmp |= ((1 << (36 - 32)) | (1 << (39 - 32)) | (1 << (40 - 32)) | (1 << (41 - 32)) | 
			(1 << (42 - 32)) | (1 << (43 - 32)) | (1 << (44 - 32)) | (1 << (46 - 32)));

	*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;

	/* INPUT GPIO
	 * GPIO37:WLAN ON/OFF (WPS switch for 845v3)
	 * GPIO38:RESET Button(WPS switch for 841v13)
	 */
	/* Set Direction to input */
	/* RALINK_REG_PIODIR for GPIO 0~23 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
	tmp &= ~((1 << (37 - 32)) | (1 << (38 - 32)));
	*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp; 
	
#elif defined(CONFIG_TP_MODEL_C20V4)

	/* Set Direction to output */
	/* RALINK_REG_PIODIR for GPIO 0~32 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIODIR));
	tmp |= (1 << (11 - 0));
	*(volatile u32 *)(RALINK_REG_PIODIR) = tmp;
	
	/* RALINK_REG_PIO6332DIR for GPIO 32~64 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
	tmp |= ((1 << (39 - 32)) | (1 << (40 - 32)) | (1 << (41 - 32)) | (1 << (42 - 32)) | 
			(1 << (43 - 32)) | (1 << (44 - 32)) | (1 << (46 - 32)));
	*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;
	
	/* RALINK_REG_PIO6332DIR for GPIO 32~64 INPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DIR));
	tmp &= ~((1 << (37 - 32)) | (1 << (38 - 32)));
	*(volatile u32 *)(RALINK_REG_PIO6332DIR) = tmp;

#elif defined(CONFIG_TP_MODEL_WA801NDV5) 
	/* GPIO
	 * GPIO36: POWER 
	 * GPIO37:LAN 
	 * GPIO38:RESET Button
	 * GPIO42:WPS GREEN 
	 * GPIO43:WPS RED
	 * GPIO44:WLAN(2.4G)
	 * GPIO46:WPS Button 
	 */

	/* Set Direction to output */
	/* GPIO CTRL 1 for GPIO OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp |= ((1 << (36 - 32)) | (1 << (37 - 32)) | (1 << (42 - 32)) | (1 << (43 - 32)) | (1 << (44 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/* GPIO CTRL 1 for GPIO INPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp &= (~(1 << (38 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp &= (~(1 << (46 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

#elif defined(CONFIG_TP_MODEL_WR802NV4)
    /* GPIO
     * GPIO37:POWER
     * GPIO38:RESET Button
     */

    /* Set Direction to output */
    /* GPIO CTRL 1 for GPIO OUTPUT */
    tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
    tmp |= (1 << (37 - 32));
    *(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

    /* GPIO CTRL 1 for GPIO INPUT */
    tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
    tmp &= (~(1 << (38 - 32)));
    *(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

#elif defined(CONFIG_TP_MODEL_C2V5)
    /* GPIO
	 ****************LED***************
	 * GPIO11:	POWER
	 * GPIO52:	WLAN(2.4G)
	 * GPIO53:	WLAN(5G)
	 * GPIO54:	LAN
	 * GPIO55:	WPS_LED
	 * GPIO56:	INTERNET_GREEN
	 * GPIO57:	INTERNET_ORANGE
	 ****************BTN***************
	 * GPIO2:	RESET
	 * GPIO13:	WPS/WIFI
	 */

	/* Set Direction to output */
	/* RALINK_REG_PIODIR for GPIO 0~23 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x24));
	tmp |= (1 << (11 - 0));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x24) = tmp;
	 
	/* RALINK_REG_PIODIR for GPIO 40~71 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x74));
	tmp |= (1 << (52 - 40)) | (1 << (53 - 40)) | (1 << (54 - 40)) | 
	       (1 << (55 - 40)) | (1 << (56 - 40)) | (1 << (57 - 40));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x74) = tmp;

	/* Set Direction to input */
	/* RALINK_REG_PIODIR for GPIO 0~23 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x24));
	tmp &= (~((1 << (2 - 0)) | (1 << (13 - 0))));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x24) = tmp;

#elif defined(CONFIG_TP_MODEL_C5V4)
	/* GPIO: FUNC
	 ****************LED***************
	 * GPIO42: POWER
	 * GPIO14: 2.4G LED
	 * GPIO12: 5G LED
	 * GPIO09: INTERNET_ORANGE
	 * GPIO10: INTERNET_GREEN
	 * GPIO08: LAN LED
	 * GPIO07: USB LED
	 * GPIO01: WPS LED	
	 ****************BTN***************
	 * GPIO13: RESET
	 * GPIO02: WIFI/WPS
	 ****************CTRL***************
	 * GPIO60:RESET Switch 
	 */
	/* Set Direction to output */
	/* RALINK_REG_PIODIR for GPIO 0~23 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x24));
	tmp |= ((1<<14)|(1<<12)|(1<<9)|(1<<10)|(1<<8)|(1<<7)|(1<<1));
	tmp &= (~((1 <<13) | (1<<2)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x24) = tmp;

	/* RALINK_REG_PIO3924DIR for GPIO 24~39 */

	/* RALINK_REG_PIO7140DIR for GPIO 40~71 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x74));
	tmp |= ((1 << (42-40))|(1 << (60-40)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x74) = tmp;

	/* RALINK_REG_PIO72DIR for GPIO 72 */

	/* Set Polarity*/
	/* RALINK_REG_PIOPOL for GPIO 0~23 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x28));
	tmp |= (1<<8); 
	tmp &= (~((1<<14)|(1<<12)|(1<<9)|(1<<10)|(1<<7)|(1<<1)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x28) = tmp;
	
	/* RALINK_REG_PIO3924POL for GPIO 24~39 */
	
	/* RALINK_REG_PIO7140POL for GPIO 40~71 */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x78));
	tmp &= (~((1 << (42-40)) | (1 << (60-40))));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x78) = tmp;
	
	/* RALINK_REG_PIO72POL for GPIO 72 */

#endif
	return;
}

void initGpioMode_W8(void)
{
    u32 gpiomode;
#if defined(CONFIG_TP_MODEL_C2V5)
    gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
	gpiomode |= (1 << 0) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 19);
	gpiomode &= (~(1 << 18));
#elif defined(CONFIG_TP_MODEL_C5V4)
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
	gpiomode |= (7<<2) | (1<<10) | (1<<15) | (1<<0);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);
#else
	u32 gpiomode2;
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_REG_GPIOMODE));
	gpiomode2 = le32_to_cpu(*(volatile u32 *)(RALINK_REG_GPIOMODE2));

#if defined(CONFIG_TP_MODEL_WR841NV13) || defined(CONFIG_TP_MODEL_WR845NV3) || \
    defined(CONFIG_TP_MODEL_C20V4) || defined(CONFIG_TP_MODEL_WA801NDV5)
	gpiomode &= ~((0x3 << 0) | (0x3 << 24));
	gpiomode |= (1 << 14) | (1 << 16) | (1 << 18) | (1 << 24);
#elif defined(CONFIG_TP_MODEL_WR840NV4) || defined(CONFIG_TP_MODEL_WR802NV4) || \
    defined(CONFIG_TP_MODEL_WR850NV2) || defined(CONFIG_TP_MODEL_WR850NV3) || \
    defined(CONFIG_TP_MODEL_EN020_F5V1)
	gpiomode |= (RALINK_GPIOMODE_REFCLK) | (RALINK_GPIOMODE_WDT) | (RALINK_GPIOMODE_PERST);
#endif	
	*(volatile u32 *)(RALINK_REG_GPIOMODE) = cpu_to_le32(gpiomode);

	/*printk("gpiomode2 %08x.\n", gpiomode);*/
	gpiomode2 &= ((0xf << 12) | (0xf << 28));
	/*printk("gpiomode2 %08x.\n", gpiomode);*/
	gpiomode2 |= (0x555 | (0x555 << 16));
	/*printk("gpiomode2 %08x.\n", gpiomode);*/
	*(volatile u32 *)(RALINK_REG_GPIOMODE2) = cpu_to_le32(gpiomode2);
#endif
	
	return;
}

void initLedData(TP_LED_CELL *pGpio, u32 gpio)
{
	pGpio->gpioId = gpio;
	pGpio->gpioAction = GPIO_ACT_OFF;
	pGpio->gpioFreq = GPIO_FREQ_NO;
	pGpio->gpioStatus = GPIO_ACT_OFF;
	setGpioData(gpio, 1);
	pGpio->gpioTimes = 0;
}

void initLedData_ON(TP_LED_CELL *pGpio, u32 gpio)
{
	pGpio->gpioId = gpio;
	pGpio->gpioAction = GPIO_ACT_ON;
	pGpio->gpioFreq = GPIO_FREQ_NO;
	pGpio->gpioStatus = GPIO_ACT_ON;
	setGpioData(gpio, 0);
	pGpio->gpioTimes = 0;
}

#if defined(CONFIG_TP_MODEL_C50V3)
void gpio_common_init(void)
{
	u32 gpiomode, tmp;
	/* GPIO1 Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)RALINK_REG_GPIOMODE);
	/*						 GPIO11 		   GPIO5 */
	gpiomode &= ~((0x3 << 0) | (0x3 << 20));
	/*					  GPIO38		 GPIO5 */
	gpiomode |= (1 << 14) | (1 << 20);
	
	//printk("gpiomode1 %08x.\n", gpiomode);
	*(volatile u32 *)RALINK_REG_GPIOMODE = cpu_to_le32(gpiomode);
	
	/* GPIO2 Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)RALINK_REG_GPIOMODE2);
	/*				   GPIO39 ~ GPIO44 */
	gpiomode &= ~0x0fff;
	/*				   GPIO39 ~ GPIO44 */
	gpiomode |= 0x0555;
	
	//printk("gpiomode2 %08x.\n", gpiomode);
	*(volatile u32 *)RALINK_REG_GPIOMODE2 = cpu_to_le32(gpiomode);	
	
	/* GPIO
	 * GPIO11: POWER LED
	 * GPIO39:WPS LED
	 * GPIO40:WLAN 5G LED
	 * GPIO41:LAN LED
	 * GPIO42:INTERNET ORANGE LED
	 * GPIO43:INTERNET GREEN LED
	 * GPIO44:WLAN 2.4G LED
	 * GPIO38:WPS /RESET BUTTON
	 * GPIO5:WLAN BUTTON
	 */
	/* Set Direction to output */
	/* GPIO CTRL 0 for GPIO 0~32 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE));
	tmp |= (1 << 11);
	tmp &= ~(1 << 5);
	*(volatile u32 *)(RALINK_PIO_BASE) = tmp;
	
	/* GPIO CTRL 1 for GPIO 32~64 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp |= 0x3f << (39 - 32);
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;
	
	/* GPIO CTRL 1 for GPIO 32~64 INPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp &= ~(1 << (38 - 32));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	initLedData(&c2_led[0], GPIO_LED_POWER);
	initLedData(&c2_led[1], GPIO_LED_WPS);
	initLedData(&c2_led[2], GPIO_LED_WLAN_5G);
	initLedData(&c2_led[3], GPIO_LED_LAN);
	initLedData(&c2_led[4], GPIO_LED_INTERNET_ORANGE);
	initLedData(&c2_led[5], GPIO_LED_INTERNET_GREEN);
	initLedData(&c2_led[6], GPIO_LED_WLAN_2G4);

}
#elif defined(CONFIG_TP_MODEL_WR841HPV5)

#define BIT(n)          (1 << (n))
#define SET_BIT(x, n)   (x |= BIT(n))
#define CLR_BIT(x, n)   (x &= ~(BIT(n)))

void gpio_common_init(void)
{
    /*
     * GPIO1_MODE 0x10000060
     *      GPIOO                   bit[1:0]    =   10B or 00B  (?)     GPIO#11
     *
     *      I2S_MODE                bit[7:6]    =   11B                 GPIO#0
     *                                                                  GPIO#1
     *                                                                  GPIO#2
     *                                                                  GPIO#3
     *
     *      WDT_RST_N               bit[14]     =   1B                  GPIO#38
     *      PERST_N                 bit[16]     =   1B                  GPIO#36
     *      REF_CLKO                bit[18]     =   1B                  GPIO#37
     *      I2C_SD                  bit[21:20]  =   01B                 GPIO#5
     *      UART1_MODE              bit[25:24]  =   11B                 GPIO#147
     *                                                                  GPIO#148
     *
     * GPIO1_MODE 0x10000064
     *      WLED_N                  bit[1:0]    =   01B                 GPIO#44
     *      EPHY_LED0_N_JTDO        bit[3:2]    =   01B                 GPIO#43
     *      EPHY_LED1_N_JTDI        bit[5:4]    =   01B                 GPIO#42
     *      EPHY_LED2_N_JTMS        bit[7:6]    =   01B                 GPIO#41
     *      EPHY_LED3_N_JTCLK       bit[9:8]    =   01B                 GPIO#40
     *      EPHY_LED4_N_JTRST_N     bit[11:10]  =   01B                 GPIO#39
     *
     */
	u32 gpiomode, tmp;

	/* Load GPIO1_Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)RALINK_REG_GPIOMODE);
    /* GPIO #11 */
    CLR_BIT(gpiomode, 0);
    CLR_BIT(gpiomode, 1);

    /* set I2S_MODE to ANTSEL */
    SET_BIT(gpiomode, 6);
    SET_BIT(gpiomode, 7);

    /* GPIO #38 */
    SET_BIT(gpiomode, 14);

    /* GPIO #36 */
    SET_BIT(gpiomode, 16);

    /* GPIO #37 */
    SET_BIT(gpiomode, 18);

    /* GPIO #5 */
    SET_BIT(gpiomode, 20);
    CLR_BIT(gpiomode, 21);

    /* set UART1_MODE to SW_R, SW_T */
    SET_BIT(gpiomode, 24);
    SET_BIT(gpiomode, 25);
	
	/* Store GPIO1_Mode */
	printk("gpio1_mode =  %08x\n", gpiomode);
	*(volatile u32 *)RALINK_REG_GPIOMODE = cpu_to_le32(gpiomode);
	
	/* Load GPIO2 Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)RALINK_REG_GPIOMODE2);

	/* GPIO #44 */
    SET_BIT(gpiomode, 0);
    CLR_BIT(gpiomode, 1);

	/* GPIO #43 */
    SET_BIT(gpiomode, 2);
    CLR_BIT(gpiomode, 3);
    
	/* GPIO #42 */
    SET_BIT(gpiomode, 4);
    CLR_BIT(gpiomode, 5);

	/* GPIO #41 */
    SET_BIT(gpiomode, 6);
    CLR_BIT(gpiomode, 7);

	/* GPIO #40 */
    SET_BIT(gpiomode, 8);
    CLR_BIT(gpiomode, 9);

	/* GPIO #39 */
    SET_BIT(gpiomode, 10);
    CLR_BIT(gpiomode, 11);
 
	/* Store GPIO2_Mode */
	printk("gpio2_mode =  %08x\n", gpiomode);
	*(volatile u32 *)RALINK_REG_GPIOMODE2 = cpu_to_le32(gpiomode);	
	
	/* Set Direction to output */
	/* GPIO CTRL 0 for GPIO 0~32 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE));
    CLR_BIT(tmp, 5);            /* GPIO #5:     OUTPUT  (WIFI Button)*/
    SET_BIT(tmp, 11);           /* GPIO #11:    INPUT   (INTERNET ORANGE LED)*/
	*(volatile u32 *)(RALINK_PIO_BASE) = tmp;
	
	/* GPIO CTRL 1 for GPIO 32~64 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	SET_BIT(tmp, (36-32));      /* GPIO #36:    OUTPUT  (POWER LED) */
    CLR_BIT(tmp, (37-32));      /* GPIO #37:    INPUT   (RESET BUTTON) */
    CLR_BIT(tmp, (38-32));      /* GPIO #38:    INPUT   (WPS BUTTON) */
    CLR_BIT(tmp, (39-32));      /* GPIO #39:    INPUT   (RE BUTTON) */
	SET_BIT(tmp, (40-32));      /* GPIO #40:    OUTPUT  (WPS LED)   */
	SET_BIT(tmp, (41-32));      /* GPIO #41:    OUTPUT  (RE LED)    */
	SET_BIT(tmp, (42-32));      /* GPIO #42:    OUTPUT  (LAN LED)   */
	SET_BIT(tmp, (43-32));      /* GPIO #43:    OUTPUT  (INTERNET WHITE LED) */
	SET_BIT(tmp, (44-32));      /* GPIO #44:    OUTPUT  (WLAN LED) */
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;
	
	initLedData(&c2_led[0], GPIO_LED_POWER);
	initLedData(&c2_led[1], GPIO_LED_INTERNET_ORANGE);
	initLedData(&c2_led[2], GPIO_LED_INTERNET_GREEN);
	initLedData(&c2_led[3], GPIO_LED_WLAN_2G4);
	initLedData(&c2_led[4], GPIO_LED_LAN);
	initLedData(&c2_led[5], GPIO_LED_WPS);
	initLedData(&c2_led[6], GPIO_LED_RE);
}

#undef BIT
#undef SET_BIT
#undef CLR_BIT

#endif
/* single bit num, like b0001, b0010, b0100, but not b0101
 * return 0 is ok
 */
u32 chkLedFreq(u32 freq)
{
	while (freq != 0)
	{
		if (freq == 1)
		{
			freq = 0;
		}
		else if ((freq & 1) == 1)
		{
			break;
		}
		freq = (freq >> 1);
	}

	if (freq != 0)
	{
		printk("LED Freq is not ok(%d)\n", freq);
	}
	return freq;
}

int setLedCfg(u32 action, u32 freq, u32 gpio)
{
	u32 count = 0;
	TP_LED_CELL *pGpio = c2_led;
	while(count < LED_NUM && (pGpio[count].gpioId != gpio))
	{
		count++;
	}
	if (count >= LED_NUM)
	{
		return -1;
	}
	if (action == GPIO_ACT_OFF || (sys_status == 1 && led_status == 0))
	{
		pGpio[count].gpioAction = GPIO_ACT_OFF;
		pGpio[count].gpioFreq = GPIO_FREQ_NO;
	}
	else if (action == GPIO_ACT_ON )
	{
		pGpio[count].gpioAction = GPIO_ACT_ON;
		pGpio[count].gpioFreq = GPIO_FREQ_NO;
	}
	else if (action == GPIO_ACT_BLINK )
	{
		#if 0
		if ((freq == GPIO_FREQ_NO)
			|| (freq == GPIO_FREQ_FAST)
			|| (freq == GPIO_FREQ_NORMAL)
			|| (freq == GPIO_FREQ_SLOW))
		#else
		if (0 == chkLedFreq(freq))
		#endif
		{
			pGpio[count].gpioAction = GPIO_ACT_BLINK;
			pGpio[count].gpioFreq = freq;
		}
		else
		{
			return -1;
		}
	}
	return 0;
	
}


void led_setUsbOn(int index)
{
#if defined(CONFIG_TP_MODEL_C5V4) 
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_USB);
#else
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, 11);
#endif
}
void led_setUsbOff(int index)
{
#if defined(CONFIG_TP_MODEL_C5V4) 
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_USB);
#else
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, 11);
#endif
}

void led_setUsbFlash(int index)
{
#if defined(CONFIG_TP_MODEL_C5V4) 
	setLedCfg(GPIO_ACT_BLINK, GPIO_FREQ_FAST, GPIO_LED_USB);
#else
	setLedCfg(GPIO_ACT_BLINK, GPIO_FREQ_FAST, 11);
#endif
}

void led_setSysOn(void)
{
#if defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_C20V4) || \
    defined(CONFIG_TP_MODEL_WR841HPV5) || defined(CONFIG_TP_MODEL_WR802NV4) || \
    defined(CONFIG_TP_MODEL_C2V5) || defined(CONFIG_TP_MODEL_C5V4) 
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_POWER);
#else
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, 36);
#endif
}
void led_setSysOff(void)
{
#if defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_C20V4) || \
    defined(CONFIG_TP_MODEL_WR841HPV5) || defined(CONFIG_TP_MODEL_WR802NV4) || \
    defined(CONFIG_TP_MODEL_C2V5) || defined(CONFIG_TP_MODEL_C5V4) 
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_POWER);
#else
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, 36);
#endif

}
void led_setSysFlash(void)
{
#if defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_C20V4) || \
    defined(CONFIG_TP_MODEL_WR841HPV5) || defined(CONFIG_TP_MODEL_WR802NV4) || \
    defined(CONFIG_TP_MODEL_C2V5) || defined(CONFIG_TP_MODEL_C5V4) 
	setLedCfg(GPIO_ACT_BLINK, GPIO_FREQ_FAST, GPIO_LED_POWER);
#else
	setLedCfg(GPIO_ACT_BLINK, GPIO_FREQ_FAST, 36);
#endif

}

void led_setWlanOn(void)
{
#if defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_C20V4) || \
    defined(CONFIG_TP_MODEL_WR841HPV5) || defined(CONFIG_TP_MODEL_WR802NV4) || \
    defined(CONFIG_TP_MODEL_C2V5) || defined(CONFIG_TP_MODEL_C5V4)
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_WLAN_2G4);
#else
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, 44);
#endif
}
void led_setWlanOff(void)
{
	if (wlan_24G_status == 0)
	{
#if defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_C20V4) || \
    defined(CONFIG_TP_MODEL_WR841HPV5) || defined(CONFIG_TP_MODEL_C2V5) || \
    defined(CONFIG_TP_MODEL_C5V4) 
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_WLAN_2G4);
#else
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, 44);
#endif
	}	
}

#if defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_C20V4) || \
	defined(CONFIG_TP_MODEL_C2V5) || defined(CONFIG_TP_MODEL_C5V4) 
void led_setWlan5gOn(void)
{
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_WLAN_5G);
}
void led_setWlan5gOff(void)
{
	if (wlan_5G_status == 0)
	{
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_WLAN_5G);
	}	
}
#endif


void led_setLanBits(u32 lanBits)
{
	u32 bit = 0;
	u32 maxLanPortNum = 4;
	u32 ledGpioBase = 42; /* GPIO 42~39 LAN(1~4) leds */

	initGpioMode_W8();

	while(bit < maxLanPortNum)
	{
		if(lanBits & (1 << bit))
		{
			setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, ledGpioBase - bit);
		}
		else
		{
			setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, ledGpioBase - bit);
		}
		bit ++;
	}
}


void led_setLanOn(void)
{
#if defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_C20V4) || \
	defined(CONFIG_TP_MODEL_WA801NDV5) || defined(CONFIG_TP_MODEL_WR841HPV5) || \
	defined(CONFIG_TP_MODEL_C2V5) || defined(CONFIG_TP_MODEL_C5V4) 
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_LAN);
#else
	initGpioMode_W8();
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, 41);
#endif
}
void led_setLanOff(void)
{
#if defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_C20V4) || \
	defined(CONFIG_TP_MODEL_WA801NDV5) || defined(CONFIG_TP_MODEL_WR841HPV5) || \
	defined(CONFIG_TP_MODEL_C2V5) || defined(CONFIG_TP_MODEL_C5V4) 
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_LAN);
#else
	initGpioMode_W8();
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, 41);
#endif

}

void led_setWanOn(u32 internet)
{
#if defined(CONFIG_TP_MODEL_WR840NV4) || defined(CONFIG_TP_MODEL_WR850NV2) || \
	defined(CONFIG_TP_MODEL_WR850NV3) || defined(CONFIG_TP_MODEL_EN020_F5V1)

	if (internet == 1 && wan_status == 1)
	{
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, 43);
	}
	else
	{
		if (wan_status == 1 || MULTIMODE_AP_MODE == wl_mode)
		{
			setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, 43);
		}
		else
		{
			//setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, 43);
		}
	}
#elif defined(CONFIG_TP_MODEL_WR841NV13) || defined(CONFIG_TP_MODEL_WR845NV3)
	if (internet == 1)
	{
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, 43);
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, 11);
	}
	else
	{
		if (wan_status == 1)
		{
			setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, 43);
			setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, 11);			
		}
		else
		{
			setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, 11);
			setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, 43);

		}
	}
#elif defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_C20V4) || \
      defined(CONFIG_TP_MODEL_C2V5)  || defined(CONFIG_TP_MODEL_C5V4) 

	if (internet == 1)
	{
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_INTERNET_GREEN);
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE);
	}
	else
	{
		if (wan_status == 1)
		{
			setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_INTERNET_GREEN);
			setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE);			
		}
		else
		{
			setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE);
			setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_GREEN);

		}
	}
#elif defined(CONFIG_TP_MODEL_WR841HPV5)
    /*
     * WAN LED makes sense only in Router mode.
     */
    if ( MULTIMODE_ROUTER_MODE != wl_mode )
    {
        return;
    }

	if (internet == 1)
	{
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_INTERNET_GREEN);
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE);
	}
	else
	{
		if (wan_status == 1)
		{
			setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_INTERNET_GREEN);
			setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE);			
		}
		else
		{
			setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE);
			setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_GREEN);

		}
	}
#elif defined(CONFIG_TP_MODEL_WR802NV4)
    // hold the power led
    led_setSysOn();    
#endif
}

void led_setWanOff(u32 internet)
{
#if defined(CONFIG_TP_MODEL_WR840NV4) || defined(CONFIG_TP_MODEL_WR850NV2) || \
	defined(CONFIG_TP_MODEL_WR850NV3) || defined(CONFIG_TP_MODEL_EN020_F5V1)

	if (internet == 0)/*unlink*/
	{
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, 43);

	}
	else /*link but no internet*/
	{
		//by Zhao Mengqing, 20180130, TP3低端路由，WAN口有连接但没有Internet时，WAN灯闪烁
		setLedCfg(GPIO_ACT_BLINK, GPIO_FREQ_FAST, 43);
	}
#elif defined(CONFIG_TP_MODEL_WR841NV13) || defined(CONFIG_TP_MODEL_WR845NV3)
	if (internet == 0)/*unlink*/
	{
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, 43);
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, 11);

	}
	else /*link but no internet*/
	{
		setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, 43);
		setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, 11);
	}
#elif defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_C20V4) || \
        defined(CONFIG_TP_MODEL_C2V5) || defined(CONFIG_TP_MODEL_C5V4) 
if (internet == 0)/*unlink*/
{
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_GREEN);
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE);

}
else /*link but no internet*/
{
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_GREEN);
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE);
}

#elif defined(CONFIG_TP_MODEL_WR841HPV5)
    if ( MULTIMODE_ROUTER_MODE == wl_mode  && 0 != internet)
    {
        /* link but no internet */
        setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_GREEN);
        setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE);
    }
    else if (internet == 0) /*unlink*/
    {
        setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_GREEN);
        setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_INTERNET_ORANGE);

    }
#elif defined(CONFIG_TP_MODEL_WR802NV4)
    // blink the power led
    led_setSysFlash();

#endif
}

void led_setWpsOn(void)
{
#if defined(CONFIG_TP_MODEL_WR840NV4) || defined(CONFIG_TP_MODEL_WR850NV2) || \
    defined(CONFIG_TP_MODEL_WR850NV3) || defined(CONFIG_TP_MODEL_EN020_F5V1)
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, 37);
#elif defined(CONFIG_TP_MODEL_WR841NV13) || defined(CONFIG_TP_MODEL_WR845NV3)
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, 46);
#elif defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_C20V4) || \
      defined(CONFIG_TP_MODEL_WR841HPV5) || defined(CONFIG_TP_MODEL_C2V5) || \
      defined(CONFIG_TP_MODEL_C5V4) 
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_WPS);
#elif defined(CONFIG_TP_MODEL_WA801NDV5)
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_WPS_GREEN);
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_WPS_RED);
#endif
}
void led_setWpsOff(void)
{
#if defined(CONFIG_TP_MODEL_WR840NV4) || defined(CONFIG_TP_MODEL_WR850NV2) || \
    defined(CONFIG_TP_MODEL_WR850NV3) || defined(CONFIG_TP_MODEL_EN020_F5V1)
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, 37);
#elif defined(CONFIG_TP_MODEL_WR841NV13) || defined(CONFIG_TP_MODEL_WR845NV3)
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, 46);
#elif defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_C20V4) || \
      defined(CONFIG_TP_MODEL_WR841HPV5) || defined(CONFIG_TP_MODEL_C2V5) || \
      defined(CONFIG_TP_MODEL_C5V4) 
	setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_WPS);
#elif defined(CONFIG_TP_MODEL_WA801NDV5)
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_WPS_GREEN);
	setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_WPS_RED);
#endif
}

/* Added by KangYi for  led schedule of WPS */
void tp_set_wps_success_flag(int flag)
{
    tp_wps_success_flag = flag;
}

int tp_get_wps_success_flag(void)
{
    return tp_wps_success_flag;
}
/* Added end */

typedef irqreturn_t(*sc_callback_t)(int, void *, struct pt_regs *);

static sc_callback_t registered_cb = NULL;
static sc_callback_t registered_cb_5G = NULL;
static void *cb_arg;
static void *cb_arg_5G;

void register_simple_config_callback (void *callback, void *arg)
{
    registered_cb = (sc_callback_t) callback;
    cb_arg = arg;
}
void register_simple_config_callback_5G (void *callback, void *arg)
{
    registered_cb_5G = (sc_callback_t) callback;
    cb_arg_5G = arg;
}

EXPORT_SYMBOL(register_simple_config_callback);
EXPORT_SYMBOL(register_simple_config_callback_5G);

EXPORT_SYMBOL(led_setUsbOn);
EXPORT_SYMBOL(led_setUsbOff);
EXPORT_SYMBOL(led_setWpsOn);
EXPORT_SYMBOL(led_setWpsOff);
EXPORT_SYMBOL(led_setUsbFlash);
/* Added by KangYi for led schedule of WPS */
EXPORT_SYMBOL(tp_set_wps_success_flag);
EXPORT_SYMBOL(tp_get_wps_success_flag);
/* Added end */ 

/* return 0 means Test OK */
u32 (*fp_gpioTestLan)(void) = NULL;
EXPORT_SYMBOL(fp_gpioTestLan);
u32 (*fp_ethTestWan)(void) = NULL;
EXPORT_SYMBOL(fp_ethTestWan);


int resetCount = 0;
int wlanCount = 0;

int dongleDetected = 0;

extern int spi_flash_erase_config(void);
extern int spi_flash_erase_isp_config(void);

static void pollingGpio(void)
{
	u32 count = 0;
	TP_LED_CELL *pGpio = c2_led;
	u32 mask;
	static u32 cycCount = 0;
	u32 buttonStat = 0;
	static u32 isReset = 0;
	static u32 isWlan = 0;
#if defined(CONFIG_TP_MODEL_WR841NV13) || defined(CONFIG_TP_MODEL_WR845NV3) || \
    defined(CONFIG_TP_MODEL_C20V4)
	u32 lanBits = 0;
#endif

#if 0
	static int ct=0;
	ct++;
	if(ct%100==0)
	{
		for (count = 0; count < LED_NUM; count++)
		{
			printk("GPIO %d, act %d, freq %d, status %d, times %d\n",
				pGpio[count].gpioId, 
				pGpio[count].gpioAction,
				pGpio[count].gpioFreq,
				pGpio[count].gpioStatus,
				pGpio[count].gpioTimes);
		}
	}
#endif

	/* Part 1, Do LED Display */
	for (count = 0; count < LED_NUM; count++)
	{
		/*printk("GPIO %d, act %d, freq %d, status %d, times %d\n",
				pGpio[count].gpioId, 
				pGpio[count].gpioAction,
				pGpio[count].gpioFreq,
				pGpio[count].gpioStatus,
				pGpio[count].gpioTimes);*/
	
		switch (pGpio[count].gpioAction)
		{
		case GPIO_ACT_OFF:
			/* turn off */
			setGpioData(pGpio[count].gpioId, 1);
			pGpio[count].gpioStatus = GPIO_ACT_OFF;
			pGpio[count].gpioTimes = 0;
			break;
		case GPIO_ACT_ON:
			/* turn on */
			setGpioData(pGpio[count].gpioId, 0);
			pGpio[count].gpioStatus = GPIO_ACT_ON;
			pGpio[count].gpioTimes = 0;
			break;
		case GPIO_ACT_BLINK:
			mask = ((pGpio[count].gpioFreq << 2) - 1);
			if (mask == 0)
			{
				/* turn off */
				setGpioData(pGpio[count].gpioId, 1);
				pGpio[count].gpioStatus = GPIO_ACT_OFF;
			}
			else
			{	
				if ((pGpio[count].gpioTimes & mask) == 0)
				{
					/*printk("GPIO %d, mask %d, gpioTimes %d\n", pGpio[count].gpioId, mask, pGpio[count].gpioTimes);*/
					setGpioData(pGpio[count].gpioId, (pGpio[count].gpioStatus == GPIO_ACT_ON ? 1:0));
					pGpio[count].gpioStatus = (pGpio[count].gpioStatus == GPIO_ACT_ON ? GPIO_ACT_OFF : GPIO_ACT_ON);
				}
				pGpio[count].gpioTimes++;
			}
			
			break;
		default:
			/* Turn Off */
			setGpioData(pGpio[count].gpioId, 1);
			pGpio[count].gpioStatus = GPIO_ACT_OFF;
			break;
		}
	}

	/* Part 2, Do GPIO Cycle Function */
	cycCount++;

	if (0 == (cycCount & 0xf))/* Freq = 5 * RALINK_GPIO_LED_FREQ = 0.5s */
	{
#if defined(CONFIG_TP_MODEL_WR840NV4) || defined(CONFIG_TP_MODEL_C20V4) || \
    defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_WR802NV4) || \
    defined(CONFIG_TP_MODEL_C2V5) || defined(CONFIG_TP_MODEL_C5V4)
		/* LAN Led Polling */
		if ((NULL != fp_gpioTestLan) && (1 == fp_gpioTestLan()))
		{
			led_setLanOn();
		}
		else /* No func or test failed */
		{
			led_setLanOff();
		}
		/* WAN Led Polling */
		if ((NULL != fp_ethTestWan) && (1 == fp_ethTestWan())
#if defined(CONFIG_TP_MODEL_C5V4) 
			|| dongleDetected
#endif
		)
		{
			led_setWanOn(0);
		}
		else /* No func or test failed */
		{
			led_setWanOff(0);
		}
#elif defined(CONFIG_TP_MODEL_WR850NV2) || defined(CONFIG_TP_MODEL_WR850NV3) || \
      defined(CONFIG_TP_MODEL_EN020_F5V1)
		/* LAN Led Polling */
		if ((NULL != fp_gpioTestLan) && (1 == fp_gpioTestLan()))
		{
			led_setLanOn();
		}
		else /* No func or test failed */
		{
			led_setLanOff();
		}
		/* WAN Led Polling */
		if ((NULL != fp_ethTestWan) && (1 == fp_ethTestWan()))
		{
			if (wan_status == 1 || MULTIMODE_AP_MODE == wl_mode)
			{
				led_setWanOn(0);
			}
			else
			{
				led_setWanOff(1);
			}	
		}
		else /* No func or test failed */
		{
			led_setWanOff(0);
		}
#elif defined(CONFIG_TP_MODEL_WR841HPV5)
        if (1 == sys_status) 
        {
            /*
             * Wan LED only works in ROUTER mode. 
             */
            if (MULTIMODE_ROUTER_MODE == wl_mode) {
                if ((NULL != fp_gpioTestLan) && (1 == fp_gpioTestLan()))
                {
                    led_setLanOn();
                }
                else
                {
                    led_setLanOff();
                }

                /* WAN Led Polling */
                if ((NULL != fp_ethTestWan) && (1 == fp_ethTestWan()))
                {
                    led_setWanOn(0);
                }
                else
                {
                    led_setWanOff(0);
                }

            }
            else
            {
                if  (((NULL != fp_ethTestWan) && (1 == fp_ethTestWan())) ||
                        ((NULL != fp_gpioTestLan) && (1 == fp_gpioTestLan())))
                {
                    led_setLanOn();
                }
                else
                {

                    led_setLanOff();
                }
            }
            if (1 == wlan_24G_status)
            {
                led_setWlanOn();
            }
            else
            {
                led_setWlanOff();
            }
        } 
        else
        {
            led_setLanOff();
            led_setWanOff(0);
            led_setWlanOff();
        }
#elif defined(CONFIG_TP_MODEL_WR841NV13) || defined(CONFIG_TP_MODEL_WR845NV3)

		/* LAN Led Polling */
		if (NULL != fp_gpioTestLan)
		{
			lanBits = fp_gpioTestLan();
			led_setLanBits(lanBits);
		}

		/* WAN Led Polling */
		if ((NULL != fp_ethTestWan) && (1 == fp_ethTestWan()))
		{
			led_setWanOn(0);
		}
		else /* No func or test failed */
		{
			led_setWanOff(0);
		}
#elif defined(CONFIG_TP_MODEL_WA801NDV5)
		/* LAN Led Polling */
		if ((NULL != fp_gpioTestLan) && (1 == fp_gpioTestLan()))
		{
			led_setLanOn();
		}
		else /* No func or test failed */
		{
			led_setLanOff();
		}
#endif

#if defined(CONFIG_TP_MODEL_WR802NV4)
        if (sys_status == 0)
            led_setSysFlash();
#else
		if (sys_status == 0)
			led_setSysFlash();
		else
			led_setSysOn();
#endif
	}

#if defined(CONFIG_TP_MODEL_WR841HPV5)
	if (0 == getGpioData(GPIO_BUTTON_RESET, &buttonStat)) 
    {
		if (1 == buttonStat)  /* Unpressed */
        {
			/* clean Button stat */
            resetCount = 0;
        } 
        else 
        {
            if (resetCount >= RESET_TRIGGER)
			{
				/* Do Reset */
                printk("Factory configuration restored...\n");
                spi_flash_erase_config();
			}
			else
			{
				printk("resetCount=%d\n", resetCount);
				resetCount++;
			}
        }
    }
	if (0 == getGpioData(GPIO_BUTTON_WIFI, &buttonStat)) 
    {
		if (1 == isWlan)
		{
			wlanCount++;
			if (wlanCount >= (WLAN_TRIGGER + WLAN_LOCK_TIMES))
			{
				isWlan = 0;
				wlanCount = 0;
				printk("Switch Wlan Button unlocked\n");
			}
		}
		else
		{
			if (1 == buttonStat) /* Unpressed */
			{
				if (wlanCount >= WLAN_TRIGGER)
				{
                    char buf[] = "WLAN SWITCH";
                    int len = sizeof(buf);
                    printk("Switch WLAN now\n");		
                    //send_wlanSwitch_to_user(buf, len);
                    send_gpiobtn_event_to_user(buf, len);
					
					isWlan = 1;
				}
                wlanCount = 0;
			}
			else /* Pressed */
			{
				wlanCount++;
			}
        }
    }
	if (0 == getGpioData(GPIO_BUTTON_WPS, &buttonStat))
    {
        if (0 == buttonStat) 
        { 
            printk("WPS button pressed\n");
            if (registered_cb)
            {
                printk("WPS 2.4G begin\n");
                registered_cb (0, cb_arg, NULL);
            }
        }
    }
	if (0 == getGpioData(GPIO_BUTTON_RE, &buttonStat))
    {
        /* to do */
        if (0 == buttonStat || 1 == onekey_re_pressed) {

            printk("RE button pressed\n");

            if (MULTIMODE_REPEATER_MODE == wl_mode ||
                    MULTIMODE_CLIENT_MODE == wl_mode) // RE MODE && CLIENT MODE
            {
                registered_cb (1, cb_arg, NULL);
                onekey_re_pressed = 0;
            }
            else if ( 0 == onekey_re_event_sent )
            {
                /* switch to REPEATER mode */
                char buf[] = "WLAN ONEKEY RE";
                int len = sizeof(buf);

                printk("Switch to RE mode now\n");		
                //send_wlanSwitch_to_user(buf, len);
                send_gpiobtn_event_to_user(buf, len);
                onekey_re_event_sent = 1;
            }
        }
    }
#else
	/* Reset/WPS Button */
	if (0 == getGpioData(
#if defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_WA801NDV5)
		GPIO_BUTTON_RESET,
#else
		RESET_BUTTON_GPIO, 
#endif
		&buttonStat))
	{
		/*printk("resetCount %d, isReset %d\n", resetCount, isReset);*/
		if (1 == buttonStat) /* ??Unpressed */
		{
			/* clean Button stat */
			if (isReset == 1)
			{
				resetCount = 0;
				isReset = 0;
			}
		
			if ((resetCount != 0) && (isReset == 0))
			{
				resetCount = 0;
#if defined(CONFIG_TP_MODEL_WR840NV4) || defined(CONFIG_TP_MODEL_WR841NV13) || \
    defined(CONFIG_TP_MODEL_WR850NV2) || defined(CONFIG_TP_MODEL_WR850NV3) || \
    defined(CONFIG_TP_MODEL_EN020_F5V1)
				/* Do WPS */
                if( 1 == manufacture_flag )
				{
					printk("Wps button has been pressed in manufacturing mode\n");
					button_wps_pressed = 1;
				}
				else
				{				
                    printk("Call WPS now\n");
                    if (registered_cb)
                    {
                        printk("wps 2.4G begin\n");
                        registered_cb (0, cb_arg, NULL);
                    }
                    else
                    {
                        printk("register 2.4G func is NULL\n");
                    }
                    if (registered_cb_5G)
                    {
                        printk("wps 5G begin\n");
                        registered_cb_5G (0, cb_arg_5G, NULL);
                    }
                    else
                    {
                        printk("register 5G func is NULL\n");
                    }
                }
#endif	
			}
		
		}
		else /* Pressed */
		{
#ifndef CONFIG_TP_CONFIG_RESET  /* Added by Wang Yang. 2016-12-15 */
			if (resetCount >= RESET_TRIGGER)
#else
			if ((resetCount >= RESET_TRIGGER) && (resetCount < CONFIG_ISP_RESET_TRIGGER))  /* button pressed for 5s-30s */
#endif /* CONFIG_TP_CONFIG_RESET */
			{
#ifdef CONFIG_TP_CONFIG_RESET /* Added by Wang Yang. 2016-12-15 */
#ifndef CONFIG_ISP_2ND_RESET
				if (0 == isReset)/* Avoid more times */
				{
					struct file *filp = NULL;

					filp = filp_open("/var/tmp/cli_resetFlag", O_RDONLY, 0777);
					if (IS_ERR(filp))
					{
						printk("Factory configuration restored..\n");
						spi_flash_erase_config();
						isReset = 1;
					}
					else
					{	  
						printk("TP-LINK Factory configuration restored..\n");
						spi_flash_erase_isp_config();
						isReset = 1;
					}
					/* Fix 5G chip can not work after soft reboot in factory */
					char buf[] = "FACT_RESET";
					int len = sizeof(buf);
					send_gpiobtn_event_to_user(buf, len);
					/* end add */
				}
#else
                if (0 == buttonStat) /* Button is still pressed. */
                {
                    /* printk("TURN ON ALL LEDS\r\n"); */
                    led_setWlanOn();
                    led_setUsbOn();
                    led_setLanBits(0xf);
                    led_setWpsOn();
                    led_setWanOn(1);
                    resetCount++;
                    printk("resetCount ++ %d.\n", resetCount);
                }
                else
#endif /* CONFIG_ISP_2ND_RESET */
#endif /* CONFIG_TP_CONFIG_RESET */
				/* Do Reset */
				if (0 == isReset)/* Avoid more times */
				{
					printk("Factory configuration restored..\n");

					spi_flash_erase_config();

					isReset = 1;
				}
				/* Fix 5G chip can not work after soft reboot in factory */
				char buf[] = "FACT_RESET";
				int len = sizeof(buf);
				send_gpiobtn_event_to_user(buf, len);
				/* end add */
			}
#ifdef CONFIG_TP_CONFIG_RESET /* Added by Wang Yang. 2016-12-15 */
			else if(resetCount >= CONFIG_ISP_RESET_TRIGGER)
			{
				/* Do TP Config Reset */
				if (0 == isReset)/* Avoid more times */
				{
					printk("TP-LINK Factory configuration restored..\n");
					spi_flash_erase_isp_config();
					isReset = 1;
				}
				/* Fix 5G chip can not work after soft reboot in factory */
				char buf[] = "FACT_RESET";
				int len = sizeof(buf);
				send_gpiobtn_event_to_user(buf, len);
				/* end add */
			}
#endif /* CONFIG_TP_CONFIG_RESET */
			else
			{
                 /* Added by KangYi on 1st Nov 2017, for button testing when manufacturing */
                if ( 1 == manufacture_flag )
                {
                    printk("Button reset has been pressed in manufacturing mode\n");
                    button_reset_pressed = 1;
                }
                else
                {
                    /* Added end */
                    printk("resetCount ++ %d.\n", resetCount);
                    resetCount++;
                }
			}
		}
	}
#if defined(CONFIG_TP_MODEL_WR841NV13) || defined(CONFIG_TP_MODEL_WR845NV3) || \
    defined(CONFIG_TP_MODEL_C20V4) || defined(CONFIG_TP_MODEL_C50V3) || \
    defined(CONFIG_TP_MODEL_WA801NDV5) || defined(CONFIG_TP_MODEL_C2V5) || \
    defined(CONFIG_TP_MODEL_C5V4)
    /* WR840NV4 do not have wlan button. */
	/* Wlan Button */
	if (0 == getGpioData(
#if defined(CONFIG_TP_MODEL_C50V3)
		GPIO_BUTTON_WLAN,
#elif defined(CONFIG_TP_MODEL_WA801NDV5)
		GPIO_BUTTON_WPS,
#else
		WLAN_BUTTON_GPIO,
#endif
		&buttonStat))
	{
		if (1 == isWlan)
		{
			if (0 == wlanCount)
			{
				printk("Switch Wlan Button locked\n");
			}
			wlanCount++;
			if (wlanCount >= WLAN_LOCK_TIMES)
			{
				isWlan = 0;
				wlanCount = 0;
				printk("Switch Wlan Button unlock\n");
			}
		}
		else
		{
			if (1 == buttonStat) /* ??Unpressed */
			{		
				if (wlanCount >= WLAN_TRIGGER)
				{
                    if(1 == manufacture_flag )
                    {
                            printk("Wlan Switching on/off does not work when manufacture mode is on\n");
                            wlanCount = 0;
                    }
                    else
                    {
#if !defined(CONFIG_TP_MODEL_WA801NDV5)
    					//if (wlan_button_on)			
	    				{
		    				printk("Switch Wlan up now\n");		
				    		//wlan_button_on(wlan_radio_dev);
					    	char buf[] = "WLAN SWITCH";
    						int len = sizeof(buf);
	    					printk("%s\n", buf);
		    				//send_wlanSwitch_to_user(buf, len);
		    				send_gpiobtn_event_to_user(buf, len);
			    		}
#endif					
				    	wlanCount = 0;
				    	isWlan = 1;
                    }
				}
				else
				{
#if defined(CONFIG_TP_MODEL_WR845NV3) || defined(CONFIG_TP_MODEL_C20V4) || \
    defined(CONFIG_TP_MODEL_WA801NDV5) || defined(CONFIG_TP_MODEL_C50V3) || \
    defined(CONFIG_TP_MODEL_C2V5) || defined(CONFIG_TP_MODEL_C5V4)
                    /* WiFi && WPS button */			
                    /* Added by KangYi on 1st Nov 2017, for button testing when manufacturing */
                if( 1 == manufacture_flag )
                {
			    	if (wlanCount != 0) 
                    {
                         printk("Wlan/Wps button has been pressed in manufacturing mode\n");
                         button_wlan_pressed = 1;
                    }
                }
                else
                {
                    /* Added end */
			    	if ((wlanCount != 0) && (isWlan == 0))
		    		{
	    				/* Do WPS */				
    					printk("Call WPS now\n");
					    if (registered_cb)
				    	{
                    	    printk("wps 2.4G begin\n");
#ifdef STA_WPS_SUPPORT
                            if (MULTIMODE_REPEATER_MODE == wl_mode ||
                                MULTIMODE_CLIENT_MODE == wl_mode) // RE MODE && CLIENT MODE
                            {
                                registered_cb (1, cb_arg, NULL);
                            }
                            else
#endif                            
                            {
                                registered_cb (0, cb_arg, NULL);
                            }						
    					}
					    else
					    {
						    printk("register 2.4G func is NULL\n");
					    }
					    if (registered_cb_5G)					
					    {   
                	     printk("wps 5G begin\n");
						    registered_cb_5G (0, cb_arg_5G, NULL);
					    }
					    else
					    {
					    	printk("register 5G func is NULL\n");
				    	}				
			    	}			
                }
#endif					
					/* Should Re-count */
					wlanCount = 0;
				}
			}
			else /* Pressed */
			{
				wlanCount++;
			}
		}
		
	}
#endif
#endif /* CONFIG_TP_MODEL_WR841HPV5*/
}


static size_t led_usb_read_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	return size;
}
static int led_usb_write_proc(struct file *file, const char *buffer,	
	unsigned long count, void *data)
{
	char val_string[16];
	int index,val, isDongle;
	
	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d %d %d", &index, &val, &isDongle) != 3)
	{
		printk("usage: <action>\n");
		return count;
	}
	if (1==val)
	{
		led_setUsbOn(index);
		if(isDongle)
		{
			dongleDetected = 1;
		}
	}
	else if(2==val)
	{
		led_setUsbFlash(index);
		if(isDongle)
		{
			dongleDetected = 1;
		}
	}
	else
	{
		led_setUsbOff(index);
		dongleDetected = 0;
	}
	
	return count;
}
static ssize_t led_ipv4_internet_read_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	return size;
}
static ssize_t led_ipv4_internet_write_proc(struct file *file, const char *buffer,	
	unsigned long count, void *data)
{
	char val_string[16];
	int val, val1;
	
	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d %d", &val, &val1) != 2)
	{
		printk("usage: <action> <unused>\n");
		return count;
	}

	wan_ipv4_status = val;
    wan_status = wan_ipv4_status | wan_ipv6_status;
	if (wan_status && ((NULL != fp_ethTestWan) && (1 == fp_ethTestWan())))
	{
		led_setWanOn(1);
	}
	else
	{
		if ((NULL != fp_ethTestWan) && (1 == fp_ethTestWan()))
		{
			led_setWanOff(1);
		}
		else
		{
			led_setWanOff(0);
		}		
	}

	return count;
}

static ssize_t led_ipv6_internet_read_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	return size;
}
static ssize_t led_ipv6_internet_write_proc(struct file *file, const char *buffer,	
	unsigned long count, void *data)
{
	char val_string[16];
	int val, val1;
	
	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d %d", &val, &val1) != 2)
	{
		printk("usage: <action> <unused>\n");
		return count;
	}

	wan_ipv6_status = val;
    wan_status = wan_ipv4_status | wan_ipv6_status;
	if (wan_status && ((NULL != fp_ethTestWan) && (1 == fp_ethTestWan())))
	{
		led_setWanOn(1);
	}
	else
	{
		if ((NULL != fp_ethTestWan) && (1 == fp_ethTestWan()))
		{
			led_setWanOff(1);
		}
		else
		{
			led_setWanOff(0);
		}		
	}

	return count;
}

static ssize_t led_wlan_24G_read_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	return size;
}

static ssize_t led_wlan_24G_write_proc(struct file *file, const char *buffer,	
	unsigned long count, void *data)
{
	char val_string[16];
	int val;
	
	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d", &val) != 1)
	{
		printk("usage: <action>\n");
		return count;
	}
	wlan_24G_status = val;
#if defined(CONFIG_TP_MODEL_WR841HPV5)
    if ((1 == sys_status) && (1 == wlan_24G_status))
#else
	if (wlan_24G_status)
#endif
	{
		led_setWlanOn();
	}
	else
	{
		led_setWlanOff();
	}
	
	return count;
}
/* Added by KangYi on 1st Nov 2017, for button testing when manufacturing */ 
static ssize_t button_wlan_read_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    sprintf(buf, "button_wlan_status=%d", button_wlan_pressed);
    size = strlen("button_wlan_status=%d");
	return size;
}

static int button_wlan_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
    char val_string[16];
    int val;

    if (count > sizeof(val_string) - 1)
        return -EINVAL;
    if (copy_from_user(val_string, buffer, count))
        return -EFAULT;

    if(sscanf(val_string, "%d", &val) != 1)
    {
        printk("usage: <action>\n");
        return count;
    }

    button_wlan_pressed = val;

    return count;
}

static ssize_t button_wps_read_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    sprintf(buf, "button_wps_status=%d", button_wps_pressed);
    size = strlen("button_wps_status=%d");
	return size;
}

static int button_wps_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
    char val_string[16];
    int val;

    if (count > sizeof(val_string) - 1)
        return -EINVAL;
    if (copy_from_user(val_string, buffer, count))
        return -EFAULT;

    if(sscanf(val_string, "%d", &val) != 1)
    {
        printk("usage: <action>\n");
        return count;
    }

    button_wps_pressed = val;

    return count;
}

static ssize_t button_reset_read_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    sprintf(buf, "button_reset_status=%d", button_reset_pressed);
    size = strlen("button_reset_status=%d");
	return size;
}

static int button_reset_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
    char val_string[16];
    int val;

    if (count > sizeof(val_string) - 1)
        return -EINVAL;
    if (copy_from_user(val_string, buffer, count))
        return -EFAULT;

    if(sscanf(val_string, "%d", &val) != 1)
    {
        printk("usage: <action>\n");
        return count;
    }

    button_reset_pressed = val;

    return count;
}

static ssize_t manufacture_flag_read_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    sprintf(buf, "manufacture_flag=%d", manufacture_flag);
    size = strlen("manufacture_flag=%d");
	return size;
}

static int manufacture_flag_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
    char val_string[16];
    int val;

    if (count > sizeof(val_string) - 1)
        return -EINVAL;
    if (copy_from_user(val_string, buffer, count))
        return -EFAULT;

    if(sscanf(val_string, "%d", &val) != 1)
    {
        printk("usage: <action>\n");
        return count;
    }

    manufacture_flag = val;
    return count;
}

/* Added end */
static ssize_t led_wlan_5G_read_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	return size;
}
#if defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_C20V4) || \
	defined(CONFIG_TP_MODEL_C2V5) || defined(CONFIG_TP_MODEL_C5V4)
static ssize_t led_wlan_5G_write_proc(struct file *file, const char *buffer,	
	unsigned long count, void *data)
{
	char val_string[16];
	int val;
	
	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d", &val) != 1)
	{
		printk("usage: <action>\n");
		return count;
	}
	wlan_5G_status = val;
#if defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_C20V4) || \
	defined(CONFIG_TP_MODEL_C2V5) || defined(CONFIG_TP_MODEL_C5V4)
	if (wlan_5G_status)
	{
		led_setWlan5gOn();
	}
	else
	{
		led_setWlan5gOff();
	}
#else
	if (wlan_5G_status)
	{
		led_setUsbOn();/*C20???USB,??gpio??wlan5G???*/
	}
	else
	{
		led_setUsbOff();/*C20???USB,??gpio??wlan5G???*/
	}
#endif	
	return count;
}
#endif
static ssize_t led_sys_read_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	return size;
}
static ssize_t led_sys_write_proc(struct file *file, const char *buffer,	
	unsigned long count, void *data)
{

	char val_string[16];
	int val;
	
	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d", &val) != 1)
	{
		printk("usage: <action>\n");
		return count;
	}
	sys_status = val;
	if (sys_status)
	{
		led_setSysOn();
	}
	else
	{
		led_setSysFlash();/*flash*/
	}

	return count;
}

#ifdef STA_WPS_STATUS_SUPPORT
typedef enum
{
	WIFI_RSSI_NONE = 0,
	WIFI_RSSI_LOW,
	WIFI_RSSI_MEDIAN,
	WIFI_RSSI_HIGH
}WIFI_RSSI_VAL;

static ssize_t led_wps_read_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	return size;
}

static ssize_t led_wps_write_proc(struct file *file, const char *buffer,	
	unsigned long count, void *data)
{
	char val_string[16];
	int val, val1;
    int wps_status = 0;
	
	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d", &val) != 1)
	{
		printk("usage: <action>\n");
		return count;
	}

    wps_status = val;
    switch (wps_status)
    {
    /* not connected, turn off */
    case WIFI_RSSI_NONE:
        setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_WPS_GREEN);
        setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_WPS_RED);
        break;
    /* connected, level 1, < -75dBm, red */
    case WIFI_RSSI_LOW:
        setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_WPS_GREEN);
        setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_WPS_RED);
        break;        
    /* connected, level 1, >= -75dBm, <=-40dBm, orange */
    case WIFI_RSSI_MEDIAN:
        setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_WPS_GREEN);
        setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_WPS_RED);
        break;
    /* connected, level 1, > -40dBm, green */
    case WIFI_RSSI_HIGH:
        setLedCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, GPIO_LED_WPS_GREEN);
        setLedCfg(GPIO_ACT_ON, GPIO_FREQ_NO, GPIO_LED_WPS_RED);
        break;
    default:
        // TODO
        break;
    }

	return count;
}
#endif

static ssize_t wl_mode_read_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	return size;
}

static ssize_t wl_mode_write_proc(struct file *file, const char *buffer,	
	unsigned long count, void *data)
{
	char val_string[16];
	int val, val1;
	
	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	if (sscanf(val_string, "%d", &val) != 1)
	{
		printk("usage: <action>\n");
		return count;
	}

    wl_mode = val;

	return count;
}

static ssize_t led_status_read_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	char ret_buf[4];

	sprintf(ret_buf, "%d\n", led_status); 
	copy_to_user(buf, ret_buf, strlen(ret_buf));

	return strlen(ret_buf);
}

static ssize_t led_status_write_proc(struct file *file, const char *buffer,	
	unsigned long count, void *data)
{
	char val_string[6] = {0};
	int val;
	
	if (count > sizeof(val_string) - 1)
    {
        count = sizeof(val_string) - 1;
    }

	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

    val = simple_strtoul(val_string, NULL, 10);
    led_status = (val != 0);

	return count;
}

static ssize_t onekey_re_status_read_proc(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	char ret_buf[4];

	sprintf(ret_buf, "%d\n", onekey_re_pressed); 
	copy_to_user(buf, ret_buf, strlen(ret_buf));

	return strlen(ret_buf);
}

static ssize_t onekey_re_status_write_proc(struct file *file, const char *buffer,	
	unsigned long count, void *data)
{
	char val_string[6] = {0};
	int val;
	
	if (count > sizeof(val_string) - 1)
    {
        count = sizeof(val_string) - 1;
    }

	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

    val = simple_strtoul(val_string, NULL, 10);
    onekey_re_pressed = (val != 0);

	return count;
}

static int rtnetlink_fill_info(struct sk_buff *skb, int type, char *data, int data_len)
{	
	struct ifinfomsg *r;	
	struct nlmsghdr  *nlh;
	unsigned char	 *b = skb_tail_pointer(skb);
	nlh = nlmsg_put(skb, 0, 0, type, sizeof(*r), 0);
	r = NLMSG_DATA(nlh);	
	r->ifi_family = AF_UNSPEC;	
	r->__ifi_pad = 0;	
	r->ifi_type = 0;	
	r->ifi_index = 0;	
	r->ifi_flags = 0;	
	r->ifi_change = 0;	
	/* Wireless changes don't affect those flags */	
	/* Add the wireless events in the netlink packet */	
	RTA_PUT(skb, IFLA_WIRELESS, data_len, data);
	nlh->nlmsg_len = skb_tail_pointer(skb) - b;	
	return skb->len;
	nlmsg_failure:
	rtattr_failure:	
	//nlmsg_trim(skb, b);
	return -1;
}

#ifndef BUF_SIZE_RTNL
#define BUF_SIZE_RTNL 	256
#endif
//void send_wlanSwitch_to_user(char *buf, int len)
void send_gpiobtn_event_to_user(char *buf, int len)
{
	unsigned int size = BUF_SIZE_RTNL;
	int ret = 0;

	struct sk_buff *skb = alloc_skb(size, GFP_ATOMIC);

	if (skb == NULL)
	{
		printk("no enough memory!\n");
		return;
	}

	if (rtnetlink_fill_info(skb, RTM_NEWLINK,
				  buf, len) < 0) 
	{
		printk("fill reset info error!\n");
		kfree_skb(skb);
		return;
	}
	//struct net *net = sock_net(skb->sk);
	struct net *net = dev_net(skb->dev);
	rtnl_notify(skb, net, 0, RTNLGRP_LINK, NULL, GFP_ATOMIC);
	/*ret = rtnl_notify(skb, 0, RTNLGRP_LINK, NULL, GFP_ATOMIC);
	if (ret)
	{
		printk("Err to send\n");
	}*/
	return;
	
}

/* Added by chz to fit the new kernel 3.10.14. 2017-03-24 */
static struct file_operations led_usb_fops = {
	.owner 		= THIS_MODULE,
	.read	 	= led_usb_read_proc,
	.write		= led_usb_write_proc
};

static struct file_operations led_st_fops = {
	.owner 		= THIS_MODULE,
	.read	 	= led_status_read_proc,
	.write		= led_status_write_proc
};

static struct file_operations led_inet_fops = {
	.owner 		= THIS_MODULE,
	.read	 	= led_ipv4_internet_read_proc,
	.write		= led_ipv4_internet_write_proc
};

static struct file_operations led_inet6_fops = {
	.owner 		= THIS_MODULE,
	.read	 	= led_ipv6_internet_read_proc,
	.write		= led_ipv6_internet_write_proc
};

static struct file_operations led_wl24_fops = {
	.owner 		= THIS_MODULE,
	.read	 	= led_wlan_24G_read_proc,
	.write		= led_wlan_24G_write_proc
};

/* Added by KangYi on 1st Nov 2017, for button testing when manufacturing */
static struct file_operations button_wlan_fops= {
    .owner      = THIS_MODULE,
    .read       = button_wlan_read_proc,
    .write      = button_wlan_write_proc
};

static struct file_operations button_wps_fops= {
    .owner      = THIS_MODULE,
    .read       = button_wps_read_proc,
    .write      = button_wps_write_proc
};

static struct file_operations button_reset_fops= {
    .owner      = THIS_MODULE,
    .read       = button_reset_read_proc,
    .write      = button_reset_write_proc
};

static struct file_operations manufacture_flag_fops= {
    .owner      = THIS_MODULE,
    .read       = manufacture_flag_read_proc,
    .write      = manufacture_flag_write_proc
};
/* Added end */
	
#if defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_C20V4) || \
	defined(CONFIG_TP_MODEL_C2V5) || defined(CONFIG_TP_MODEL_C5V4)
static struct file_operations led_wl5g_fops = {
	.owner 		= THIS_MODULE,
	.read	 	= led_wlan_5G_read_proc,
	.write		= led_wlan_5G_write_proc
};
#endif

static struct file_operations led_sys_fops = {
	.owner 		= THIS_MODULE,
	.read	 	= led_sys_read_proc,
	.write		= led_sys_write_proc
};

#ifdef STA_WPS_STATUS_SUPPORT
static struct file_operations led_wps_fops = {
	.owner 		= THIS_MODULE,
	.read	 	= led_wps_read_proc,
	.write		= led_wps_write_proc
};
#endif

static struct file_operations led_wlmode_fops = {
	.owner 		= THIS_MODULE,
	.read	 	= wl_mode_read_proc,
	.write		= wl_mode_write_proc
};

static struct file_operations led_onekey_fops = {
	.owner 		= THIS_MODULE,
	.read	 	= onekey_re_status_read_proc,
	.write		= onekey_re_status_write_proc
};
/* end add */

void init_gpio_tplink(void)
{
	struct proc_dir_entry *led_proc;
#if 0	
	if (simple_config_entry != NULL) 
	{
		printk("Already have a proc entry for /proc/simple_config!\n");
		return;
	}

	simple_config_entry = proc_mkdir("tplink", NULL);
	if (!simple_config_entry)
	{
		return;
	}
#endif

	led_proc = proc_create("tplink/led_usb", 0, NULL, &led_usb_fops);
	led_proc = proc_create("tplink/led_status", 0, NULL, &led_st_fops);
	led_proc = proc_create("tplink/led_ipv4_internet",  0, NULL, &led_inet_fops);
	led_proc = proc_create("tplink/led_ipv6_internet", 0, NULL, &led_inet6_fops);
	led_proc = proc_create("tplink/led_wlan_24G",  0, NULL, &led_wl24_fops);
    /* Added by KangYi on 1st Nov 2017, for button testing when manufactoring */
    led_proc = proc_create("tplink/button_wlan_status", 0, NULL, &button_wlan_fops);
    led_proc = proc_create("tplink/button_wps_status", 0, NULL, &button_wps_fops);
    led_proc = proc_create("tplink/button_reset_status", 0, NULL, &button_reset_fops);
    led_proc = proc_create("tplink/manufacture_flag", 0, NULL, &manufacture_flag_fops);
    /* Added end */
#if defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_C20V4) || \
	defined(CONFIG_TP_MODEL_C2V5) || defined(CONFIG_TP_MODEL_C5V4)
	led_proc = proc_create("tplink/led_wlan_5G", 0, NULL, &led_wl5g_fops);
#endif
	led_proc = proc_create("tplink/led_sys",  0, NULL, &led_sys_fops);
#ifdef STA_WPS_STATUS_SUPPORT
    led_proc = proc_create("tplink/led_wps",  0, NULL, &led_wps_fops);
#endif
    led_proc = proc_create("tplink/wl_mode",  0, NULL, &led_wlmode_fops);
    led_proc = proc_create("tplink/onekey_re_status", 0, NULL, &led_onekey_fops);

#if defined(CONFIG_TP_MODEL_C50V3) || defined(CONFIG_TP_MODEL_WR841HPV5)
	gpio_common_init();
#else
	initGpioMode_W8();
	initGpioDir_W8();
	initLedData_W8();
#endif
#if 0
	/* test */
	setGpioCfg(GPIO_ACT_OFF, GPIO_FREQ_NO, 1);
	setGpioCfg(GPIO_ACT_ON, GPIO_FREQ_NO, 11);
	setGpioCfg(GPIO_ACT_BLINK, GPIO_FREQ_FAST, 39);
	setGpioCfg(GPIO_ACT_BLINK, GPIO_FREQ_NORMAL, 40);
	setGpioCfg(GPIO_ACT_BLINK, GPIO_FREQ_SLOW, 72);
	/* test end */
#endif


}

int __init ralink_gpio_init(void)
{
	unsigned int i;
#if 0
	u32 gpiomode;
#endif


#ifdef  CONFIG_DEVFS_FS
	if (devfs_register_chrdev(ralink_gpio_major, RALINK_GPIO_DEVNAME,
				&ralink_gpio_fops)) {
		printk(KERN_ERR NAME ": unable to register character device\n");
		return -EIO;
	}
	devfs_handle = devfs_register(NULL, RALINK_GPIO_DEVNAME,
			DEVFS_FL_DEFAULT, ralink_gpio_major, 0,
			S_IFCHR | S_IRUGO | S_IWUGO, &ralink_gpio_fops, NULL);
#else
	int r = 0;
	r = register_chrdev(ralink_gpio_major, RALINK_GPIO_DEVNAME,
			&ralink_gpio_fops);
	if (r < 0) {
		printk(KERN_ERR NAME ": unable to register character device\n");
		return r;
	}
	if (ralink_gpio_major == 0) {
		ralink_gpio_major = r;
		printk(KERN_DEBUG NAME ": got dynamic major %d\n", r);
	}
#endif

#if 0
	//config these pins to gpio mode
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_REG_GPIOMODE));
#if !defined (CONFIG_RALINK_RT2880)
	gpiomode &= ~0x1C;  //clear bit[2:4]UARTF_SHARE_MODE
#endif
#if defined (CONFIG_RALINK_MT7620)
	gpiomode &= ~0x2000;  //clear bit[13] WLAN_LED
#endif
	gpiomode |= RALINK_GPIOMODE_DFT;
	*(volatile u32 *)(RALINK_REG_GPIOMODE) = cpu_to_le32(gpiomode);
#else
		init_gpio_tplink();
#endif

	//enable gpio interrupt
	*(volatile u32 *)(RALINK_REG_INTENA) = cpu_to_le32(RALINK_INTCTL_PIO);
	for (i = 0; i < RALINK_GPIO_NUMBER; i++) {
		ralink_gpio_info[i].irq = i;
		ralink_gpio_info[i].pid = 0;
	}	

#ifdef CONFIG_RALINK_GPIO_LED
	ralink_gpio_led_init_timer();
#endif
	printk("Ralink gpio driver initialized\n");
	return 0;
}

void __exit ralink_gpio_exit(void)
{
#ifdef  CONFIG_DEVFS_FS
	devfs_unregister_chrdev(ralink_gpio_major, RALINK_GPIO_DEVNAME);
	devfs_unregister(devfs_handle);
#else
	unregister_chrdev(ralink_gpio_major, RALINK_GPIO_DEVNAME);
#endif

	//config these pins to normal mode
	*(volatile u32 *)(RALINK_REG_GPIOMODE) &= ~RALINK_GPIOMODE_DFT;
	//disable gpio interrupt
	*(volatile u32 *)(RALINK_REG_INTDIS) = cpu_to_le32(RALINK_INTCTL_PIO);
#ifdef CONFIG_RALINK_GPIO_LED
	del_timer(&ralink_gpio_led_timer);
#endif
	printk("Ralink gpio driver exited\n");
}

/*
 * send a signal(SIGUSR1) to the registered user process whenever any gpio
 * interrupt comes
 * (called by interrupt handler)
 */
void ralink_gpio_notify_user(int usr)
{
	struct task_struct *p = NULL;

	if (ralink_gpio_irqnum < 0 || RALINK_GPIO_NUMBER <= ralink_gpio_irqnum) {
		printk(KERN_ERR NAME ": gpio irq number out of range\n");
		return;
	}

	//don't send any signal if pid is 0 or 1
	if ((int)ralink_gpio_info[ralink_gpio_irqnum].pid < 2)
		return;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,35)
	p = find_task_by_vpid(ralink_gpio_info[ralink_gpio_irqnum].pid);
#else
	p = find_task_by_pid(ralink_gpio_info[ralink_gpio_irqnum].pid);
#endif

	if (NULL == p) {
		printk(KERN_ERR NAME ": no registered process to notify\n");
		return;
	}

	if (usr == 1) {
		#if defined (CONFIG_FB_MEDIATEK_ILITEK) || defined (CONFIG_FB_MEDIATEK_TRULY)&& defined (CONFIG_RALINK_MT7621)
		#else
		printk(KERN_NOTICE NAME ": sending a SIGUSR1 to process %d\n",
				ralink_gpio_info[ralink_gpio_irqnum].pid);
		#endif
		send_sig(SIGUSR1, p, 0);
	}
	else if (usr == 2) {
		#if defined (CONFIG_FB_MEDIATEK_ILITEK) || defined (CONFIG_FB_MEDIATEK_TRULY)&& defined (CONFIG_RALINK_MT7621)
		#else
		printk(KERN_NOTICE NAME ": sending a SIGUSR2 to process %d\n",
				ralink_gpio_info[ralink_gpio_irqnum].pid);
		#endif
		send_sig(SIGUSR2, p, 0);
	}
}

/*
 * 1. save the PIOINT and PIOEDGE value
 * 2. clear PIOINT by writing 1
 * (called by interrupt handler)
 */
void ralink_gpio_save_clear_intp(void)
{
	ralink_gpio_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOINT));
	ralink_gpio_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIOEDGE));
	
#if defined (RALINK_GPIO_HAS_9532)	
	*(volatile u32 *)(RALINK_REG_PIOINT) = cpu_to_le32(0xFFFFFFFF);
	*(volatile u32 *)(RALINK_REG_PIOEDGE) = cpu_to_le32(0xFFFFFFFF);
#else
	*(volatile u32 *)(RALINK_REG_PIOINT) = cpu_to_le32(0x00FFFFFF);
	*(volatile u32 *)(RALINK_REG_PIOEDGE) = cpu_to_le32(0x00FFFFFF);
#endif
#if defined (RALINK_GPIO_HAS_2722)
	ralink_gpio2722_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722INT));
	ralink_gpio2722_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO2722EDGE));
	*(volatile u32 *)(RALINK_REG_PIO2722INT) = cpu_to_le32(0x0000FFFF);
	*(volatile u32 *)(RALINK_REG_PIO2722EDGE) = cpu_to_le32(0x0000FFFF);
#elif defined (RALINK_GPIO_HAS_4524)
	ralink_gpio3924_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924INT));
	ralink_gpio3924_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924EDGE));
	*(volatile u32 *)(RALINK_REG_PIO3924INT) = cpu_to_le32(0x0000FFFF);
	*(volatile u32 *)(RALINK_REG_PIO3924EDGE) = cpu_to_le32(0x0000FFFF);
	ralink_gpio4540_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540INT));
	ralink_gpio4540_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO4540EDGE));
	*(volatile u32 *)(RALINK_REG_PIO4540INT) = cpu_to_le32(0x00000FFF);
	*(volatile u32 *)(RALINK_REG_PIO4540EDGE) = cpu_to_le32(0x00000FFF);
#elif defined (RALINK_GPIO_HAS_5124)
	ralink_gpio3924_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924INT));
	ralink_gpio3924_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924EDGE));
	*(volatile u32 *)(RALINK_REG_PIO3924INT) = cpu_to_le32(0x0000FFFF);
	*(volatile u32 *)(RALINK_REG_PIO3924EDGE) = cpu_to_le32(0x0000FFFF);
	ralink_gpio5140_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140INT));
	ralink_gpio5140_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO5140EDGE));
	*(volatile u32 *)(RALINK_REG_PIO5140INT) = cpu_to_le32(0x00000FFF);
	*(volatile u32 *)(RALINK_REG_PIO5140EDGE) = cpu_to_le32(0x00000FFF);
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	ralink_gpio3924_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924INT));
	ralink_gpio3924_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO3924EDGE));
	*(volatile u32 *)(RALINK_REG_PIO3924INT) = cpu_to_le32(0x0000FFFF);
	*(volatile u32 *)(RALINK_REG_PIO3924EDGE) = cpu_to_le32(0x0000FFFF);
	ralink_gpio7140_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140INT));
	ralink_gpio7140_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO7140EDGE));
	*(volatile u32 *)(RALINK_REG_PIO7140INT) = cpu_to_le32(0xFFFFFFFF);
	*(volatile u32 *)(RALINK_REG_PIO7140EDGE) = cpu_to_le32(0xFFFFFFFF);
#if defined (RALINK_GPIO_HAS_7224)
	ralink_gpio72_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72INT));
	ralink_gpio72_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO72EDGE));
	*(volatile u32 *)(RALINK_REG_PIO72INT) = cpu_to_le32(0x00FFFFFF);
	*(volatile u32 *)(RALINK_REG_PIO72EDGE) = cpu_to_le32(0x00FFFFFF);
#else
	ralink_gpio9572_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572INT));
	ralink_gpio9572_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9572EDGE));
	*(volatile u32 *)(RALINK_REG_PIO9572INT) = cpu_to_le32(0x00FFFFFF);
	*(volatile u32 *)(RALINK_REG_PIO9572EDGE) = cpu_to_le32(0x00FFFFFF);
#endif
#elif defined (RALINK_GPIO_HAS_9532)
	ralink_gpio6332_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332INT));
	ralink_gpio6332_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332EDGE));
	*(volatile u32 *)(RALINK_REG_PIO6332INT) = cpu_to_le32(0xFFFFFFFF);
	*(volatile u32 *)(RALINK_REG_PIO6332EDGE) = cpu_to_le32(0xFFFFFFFF);


	ralink_gpio9564_intp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564INT));
	ralink_gpio9564_edge = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO9564EDGE));
	*(volatile u32 *)(RALINK_REG_PIO9564INT) = cpu_to_le32(0xFFFFFFFF);
	*(volatile u32 *)(RALINK_REG_PIO9564EDGE) = cpu_to_le32(0xFFFFFFFF);

#endif
}
#if defined (CONFIG_FB_MEDIATEK_ILITEK) || defined (CONFIG_FB_MEDIATEK_TRULY)&& defined (CONFIG_RALINK_MT7621)
int lcdtimes=0;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
void ralink_gpio_irq_handler(unsigned int irq, struct irqaction *irqaction)
#else
irqreturn_t ralink_gpio_irq_handler(int irq, void *irqaction)
#endif
{
	struct gpio_time_record {
		unsigned long falling;
		unsigned long rising;
	};
	static struct gpio_time_record record[RALINK_GPIO_NUMBER];
	unsigned long now;
	int i;
	ralink_gpio_save_clear_intp();
	now = jiffies;
#if defined (RALINK_GPIO_HAS_2722)
	for (i = 0; i < 22; i++) {
		if (! (ralink_gpio_intp & (1 << i)))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio_edge & (1 << i)) { //rising edge
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
				/*
				 * If the interrupt comes in a short period,
				 * it might be floating. We ignore it.
				 */
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					//one click
					schedule_work(&gpio_event_click);
				}
				else {
					//press for several seconds
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else { //falling edge
			record[i].falling = now;
		}
		break;
	}
	for (i = 22; i < 28; i++) {
		if (! (ralink_gpio2722_intp & (1 << (i - 22))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio2722_edge & (1 << (i - 22))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#elif defined (RALINK_GPIO_HAS_9532)
	for (i = 0; i < 32; i++) {
		if (! (ralink_gpio_intp & (1 << i)))
			continue;
			ralink_gpio_irqnum = i;
		if (ralink_gpio_edge & (1 << i)) { //rising edge
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
				/*
				 * If the interrupt comes in a short period,
				 * it might be floating. We ignore it.
				 */
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					//one click
					printk("one click\n");
					schedule_work(&gpio_event_click);
				}
				else {
					//press for several seconds
					printk("press for several seconds\n");
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else { //falling edge
			 record[i].falling = now;
			#if defined (CONFIG_FB_MEDIATEK_ILITEK) || defined (CONFIG_FB_MEDIATEK_TRULY)&& defined (CONFIG_RALINK_MT7621)
			
			  lcdtimes++;
			  if(lcdtimes <=4 ){
			    schedule_work(&gpio_event_click);
				}
				if (lcdtimes > 4){
					schedule_work(&gpio_event_hold);
					lcdtimes=0;
		  	}
			#endif
		}
		break;
	}
	for (i = 32; i < 64; i++) {
		if (! (ralink_gpio6332_intp & (1 << (i - 32))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio6332_edge & (1 << (i - 32))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
	for (i = 64; i < RALINK_GPIO_NUMBER; i++) {
		if (! (ralink_gpio9564_intp & (1 << (i - 64))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio9564_edge & (1 << (i - 64))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#else
	for (i = 0; i < 24; i++) {
		if (! (ralink_gpio_intp & (1 << i)))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio_edge & (1 << i)) { //rising edge
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
				/*
				 * If the interrupt comes in a short period,
				 * it might be floating. We ignore it.
				 */
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					//one click
					printk("i=%d, one click\n", i);
					schedule_work(&gpio_event_click);
				}
				else {
					//press for several seconds
					printk("i=%d, push several seconds\n", i);
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else { //falling edge
			record[i].falling = now;
		}
		break;
	}
#if defined (RALINK_GPIO_HAS_4524)
	for (i = 24; i < 40; i++) {
		if (! (ralink_gpio3924_intp & (1 << (i - 24))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio3924_edge & (1 << (i - 24))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
	for (i = 40; i < RALINK_GPIO_NUMBER; i++) {
		if (! (ralink_gpio4540_intp & (1 << (i - 40))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio4540_edge & (1 << (i - 40))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#elif defined (RALINK_GPIO_HAS_5124)
	for (i = 24; i < 40; i++) {
		if (! (ralink_gpio3924_intp & (1 << (i - 24))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio3924_edge & (1 << (i - 24))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
	for (i = 40; i < RALINK_GPIO_NUMBER; i++) {
		if (! (ralink_gpio5140_intp & (1 << (i - 40))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio5140_edge & (1 << (i - 40))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					schedule_work(&gpio_event_click);
				}
				else {
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#elif defined (RALINK_GPIO_HAS_9524) || defined (RALINK_GPIO_HAS_7224)
	for (i = 24; i < 40; i++) {
		if (! (ralink_gpio3924_intp & (1 << (i - 24))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio3924_edge & (1 << (i - 24))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					printk("i=%d, one click\n", i);
					schedule_work(&gpio_event_click);
				}
				else {
					printk("i=%d, push several seconds\n", i);
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
	for (i = 40; i < 72; i++) {
		if (! (ralink_gpio7140_intp & (1 << (i - 40))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio7140_edge & (1 << (i - 40))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					printk("i=%d, one click\n", i);
					schedule_work(&gpio_event_click);
				}
				else {
					printk("i=%d, push several seconds\n", i);
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#if defined (RALINK_GPIO_HAS_7224)
	for (i = 72; i < RALINK_GPIO_NUMBER; i++) {
		if (! (ralink_gpio72_intp & (1 << (i - 72))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio72_edge & (1 << (i - 72))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					printk("i=%d, one click\n", i);
					schedule_work(&gpio_event_click);
				}
				else {
					printk("i=%d, push several seconds\n", i);
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#else
	for (i = 72; i < RALINK_GPIO_NUMBER; i++) {
		if (! (ralink_gpio9572_intp & (1 << (i - 72))))
			continue;
		ralink_gpio_irqnum = i;
		if (ralink_gpio9572_edge & (1 << (i - 72))) {
			if (record[i].rising != 0 && time_before_eq(now,
						record[i].rising + 40L)) {
			}
			else {
				record[i].rising = now;
				if (time_before(now, record[i].falling + 200L)) {
					printk("i=%d, one click\n", i);
					schedule_work(&gpio_event_click);
				}
				else {
					printk("i=%d, push several seconds\n", i);
					schedule_work(&gpio_event_hold);
				}
			}
		}
		else {
			record[i].falling = now;
		}
		break;
	}
#endif
#endif
#endif

	return IRQ_HANDLED;
}

struct irqaction ralink_gpio_irqaction = {
	.handler = ralink_gpio_irq_handler,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35)
	.flags = IRQF_DISABLED,
#else
	.flags = SA_INTERRUPT,
#endif
	.name = "ralink_gpio",
};

void __init ralink_gpio_init_irq(void)
{
	setup_irq(SURFBOARDINT_GPIO, &ralink_gpio_irqaction);
}

module_init(ralink_gpio_init);
module_exit(ralink_gpio_exit);

