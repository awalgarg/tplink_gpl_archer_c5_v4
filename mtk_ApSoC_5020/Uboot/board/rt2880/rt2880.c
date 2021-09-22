/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <asm/addrspace.h>
//#include "LzmaDecode.h"

//#define MAX_SDRAM_SIZE	(64*1024*1024)
//#define MIN_SDRAM_SIZE	(8*1024*1024)
#define MAX_SDRAM_SIZE	(256*1024*1024)
#define MIN_SDRAM_SIZE	(8*1024*1024)

#ifdef SDRAM_CFG_USE_16BIT
#define MIN_RT2880_SDRAM_SIZE	(16*1024*1024)
#else
#define MIN_RT2880_SDRAM_SIZE	(32*1024*1024)
#endif


/*
 * Check memory range for valid RAM. A simple memory test determines
 * the actually available RAM size between addresses `base' and
 * `base + maxsize'.
 */
long get_ram_size(volatile long *base, long maxsize)
{
	volatile long *addr;
	long           save[32];
	long           cnt;
	long           val;
	long           size;
	int            i = 0;

	for (cnt = (maxsize / sizeof (long)) >> 1; cnt > 0; cnt >>= 1) {
		addr = base + cnt;	/* pointer arith! */
		save[i++] = *addr;
		
		*addr = ~cnt;

		
	}

	addr = base;
	save[i] = *addr;

	*addr = 0;

	
	if ((val = *addr) != 0) {
		/* Restore the original data before leaving the function.
		 */
		*addr = save[i];
		for (cnt = 1; cnt < maxsize / sizeof(long); cnt <<= 1) {
			addr  = base + cnt;
			*addr = save[--i];
		}
		return (0);
	}

	for (cnt = 1; cnt < maxsize / sizeof (long); cnt <<= 1) {
		addr = base + cnt;	/* pointer arith! */

	//	printf("\n retrieve addr=%08X \n",addr);
			val = *addr;
		*addr = save[--i];
		if (val != ~cnt) {
			size = cnt * sizeof (long);
			
		//	printf("\n The Addr[%08X],do back ring  \n",addr);
			
			/* Restore the original data before leaving the function.
			 */
			for (cnt <<= 1; cnt < maxsize / sizeof (long); cnt <<= 1) {
				addr  = base + cnt;
				*addr = save[--i];
			}
			return (size);
		}
	}

	return (maxsize);
}



long int initdram(int board_type)
{
	ulong size, max_size       = MAX_SDRAM_SIZE;
	ulong our_address;
#ifndef CONFIG_MIPS16
	asm volatile ("move %0, $25" : "=r" (our_address) :);
#endif
	/* Can't probe for RAM size unless we are running from Flash.
	 */
#if 0	 
	#if defined(CFG_RUN_CODE_IN_RAM)

	printf("\n In RAM run \n"); 
    return MIN_SDRAM_SIZE;
	#else

	printf("\n In FLASH run \n"); 
    return MIN_RT2880_SDRAM_SIZE;
	#endif
#endif 
    
#if defined (RT2880_FPGA_BOARD) || defined (RT2880_ASIC_BOARD)
	if (PHYSADDR(our_address) < PHYSADDR(PHYS_FLASH_1))
	{
	    
		//return MIN_SDRAM_SIZE;
		//fixed to 32MB
		printf("\n In RAM run \n");
		return MIN_SDRAM_SIZE;
	}
#endif
	 


	size = get_ram_size((ulong *)CFG_SDRAM_BASE, MAX_SDRAM_SIZE);
	if (size > max_size)
	{
		max_size = size;
	//	printf("\n Return MAX size!! \n");
		return max_size;
	}
//	printf("\n Return Real size =%d !! \n",size);
	return size;
	
}

int checkboard (void)
{
	puts ("Board: Ralink APSoC ");
	return 0;
}

#include <rt_mmap.h>

#define u32 u_long

int setGpioData(u32 gpio, u32 data)
{
	u32 bit = 0;
	u32 reg = 0;
	u32 tmp = 0;
#if defined(TP_MODEL_C2V5) || defined(TP_MODEL_C5V4)
	/* Get reg and bit of the reg */
	if (gpio > 72)
	{
		puts("Boot: setGpioData() Unsupport GPIO\n");
		return -1;
	}
	if (gpio <= 23)
	{
		/* RALINK_REG_PIODATA for GPIO 0~23 */
		reg = RALINK_PIO_BASE + 0x20;
		bit = (1 << gpio);
	}
	else if (gpio <= 39)
	{
		/* RALINK_REG_PIO3924DATA for GPIO 24~39 */
		reg = RALINK_PIO_BASE + 0x48;
		bit = (1 << (gpio - 24));
	}
	else if (gpio <= 71)
	{
		/* RALINK_REG_PIO7140DATA for GPIO 40~71 */
		reg = RALINK_PIO_BASE + 0x70;
		bit = (1 << (gpio - 40));
	}
	else /* gpio 72 */
	{
		/* RALINK_REG_PIO72DATA for GPIO 72 */
		reg = RALINK_PIO_BASE + 0x98;
		bit = 1;
	}
#else
	/* Get reg and bit of the reg */
	if (gpio > 95)
	{
		puts("Boot: setGpioData() Unsupport GPIO\n");
		return -1;
	}
	if (gpio <= 31)
	{
		/* RALINK_REG_PIODATA for GPIO 0~31 */
#if defined(TP_MODEL_C20V4)
	if(2 == gpio)
		reg = RALINK_SYSCTL_BASE + 0x3c;
	else
#endif
		reg = RALINK_PIO_BASE + 0x20;
		bit = (1 << gpio);
	}
	else if (gpio <= 63)
	{
		/* RALINK_REG_PIO3924DATA for GPIO 32~63 */
		reg = RALINK_PIO_BASE + 0x24;
		bit = (1 << (gpio - 32));
	}
	else if (gpio <= 95)
	{
		/* RALINK_REG_PIO7140DATA for GPIO 64~95 */
		reg = RALINK_PIO_BASE + 0x28;
		bit = (1 << (gpio - 64));
	}
#endif

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
			while(i<100 && *(volatile u32 *)(reg) != tmp)
				i++;
			if(i>=100)
			{
				printf("setGpioData error: target=%x, actual=%x\r\n", 
					tmp, *(volatile u32 *)(reg));
			}
		}
#endif

	return 0;
}


int initTpProduct(void)
{
	u32 gpiomode;
	u32 tmp;
#if defined(TP_MODEL_WR840NV4) || defined(TP_MODEL_WR850NV2) ||\
	defined(TP_MODEL_WR850NV3) || defined(TP_MODEL_EN020_F5V1) 
	/* GPIO Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
	gpiomode |= (1 << 14) | (1 << 16) | (1 << 18);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64));
	printf("gpiomode2 %08x.\n", gpiomode);
	gpiomode &= ((0xf << 12) | (0xf << 28));
	printf("gpiomode2 %08x.\n", gpiomode);
	gpiomode |= (0x555 | (0x555 << 16));
	printf("gpiomode2 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64) = cpu_to_le32(gpiomode);	

	/* GPIO
	 * GPIO36: POWER
	 * GPIO37:WPS
	 * GPIO38:WPS/RESET Button
	 * GPIO41:LAN
	 * GPIO43:INTERNET
	 * GPIO44:WLAN(2.4G)
	 */
	/* Set Direction to output */
	/* GPIO CTRL 1 for GPIO 32~64 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp |= ((1 << (36 - 32)) | (1 << (37 - 32)) | (1 << (41 - 32)) | (1 << (43 - 32)) | (1 << (44 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/* GPIO CTRL 1 for GPIO 32~64 INPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp &= (~(1 << (38 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/* Led */
	setGpioData(36, 0);
	setGpioData(37, 0);
	setGpioData(41, 0);
	setGpioData(43, 0);
	setGpioData(44, 0);
	udelay (1000 * 100 * 10); /* 1s */
	setGpioData(36, 0);
	setGpioData(37, 1);
	setGpioData(41, 1);
	setGpioData(43, 1);
	setGpioData(44, 1);

#elif defined(TP_MODEL_WR841NV13) || defined(TP_MODEL_WR845NV3)

	/* GPIO Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
	gpiomode &= ~((0x3 << 0) | (0x3 << 24));
	gpiomode |= (1 << 14) | (1 << 16) | (1 << 18) | (1 << 24);
	printf("gpiomode1 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64));
	printf("gpiomode2 %08x.\n", gpiomode);
	gpiomode &= ((0xf << 12) | (0xf << 28));
	printf("gpiomode2 %08x.\n", gpiomode);
	gpiomode |= (0x555 | (0x555 << 16));
	printf("gpiomode2 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64) = cpu_to_le32(gpiomode);	

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
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE));
	tmp |= (1 << (11 - 0));
	*(volatile u32 *)(RALINK_PIO_BASE) = tmp;
	
	/* GPIO CTRL 1 for GPIO 32~64 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp |= ((1 << (36 - 32)) | (1 << (39 - 32)) | (1 << (40 - 32)) | (1 << (41 - 32)) | 
			(1 << (42 - 32)) | (1 << (43 - 32)) | (1 << (44 - 32)) | (1 << (46 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;
	
	/* GPIO CTRL 1 for GPIO 32~64 INPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp &= ~((1 << (37 - 32)) | (1 << (38 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/* Led */
	setGpioData(11, 0);
	setGpioData(36, 0);
	setGpioData(39, 0);
	setGpioData(40, 0);
	setGpioData(41, 0);
	setGpioData(42, 0);
	setGpioData(43, 1);
	setGpioData(44, 0);
	setGpioData(46, 0);
	udelay (500 * 100 * 10); /* 0.5s */
	setGpioData(11, 1);		/* wan color switch */
	setGpioData(43, 0);
	udelay (500 * 100 * 10); /* 0.5s */
	setGpioData(36, 0);
	setGpioData(39, 1);
	setGpioData(40, 1);
	setGpioData(41, 1);
	setGpioData(42, 1);
	setGpioData(43, 1);
	setGpioData(44, 1);
	setGpioData(46, 1);	

#elif defined(TP_MODEL_C20V4)

	/* GPIO Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
	gpiomode &= ~((0x3 << 0) | (0x3 << 24));
	gpiomode |= (1 << 14) | (1 << 16) | (1 << 18) | (1 << 24);
	printf("gpiomode1 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64));
	printf("gpiomode2 %08x.\n", gpiomode);
	gpiomode |= (0x555 | (0x555 << 16));
	printf("gpiomode2 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64) = cpu_to_le32(gpiomode);	

	/* GPIO
	 ****************LED***************
	 * GPIO2:	POWER
	 * GPIO11:	INTERNET_ORANGE
	 * GPIO39:	WLAN(2.4G)
	 * GPIO40:	WLAN(5G)
	 * GPIO41:	WPS_LED
	 * GPIO42:	LAN
	 * GPIO43:	INTERNET_GREEN
	 ****************BTN***************
	 * GPIO36:	WLAN ON/OFF
	 * GPIO37:	RESET/WPS
	 */
	 
	/* Set Direction to output */
	/* GPIO CTRL 0 for GPIO 0~32 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE));
	tmp |= (1 << (11 - 0));
	*(volatile u32 *)(RALINK_PIO_BASE) = tmp;

	/* GPIO CTRL 1 for GPIO 32~64 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp |= ((1 << (39 - 32)) | (1 << (40 - 32)) | (1 << (41 - 32)) | (1 << (42 - 32)) | 
			(1 << (43 - 32)) | (1 << (44 - 32)) | (1 << (46 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/* GPIO CTRL 1 for GPIO 32~64 INPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp &= ~((1 << (37 - 32)) | (1 << (38 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/* Led */
	setGpioData(2,  0);
	setGpioData(11, 1);
	setGpioData(39, 0);
	setGpioData(40, 0);
	setGpioData(41, 0);
	setGpioData(42, 0);
	setGpioData(43, 0);
	udelay (500 * 100 * 10);
	setGpioData(11, 0);
	setGpioData(43, 1);
	udelay (500 * 100 * 10);
	setGpioData(2,  0);//Do not let Power LED off, fix Bug 202562 
	setGpioData(11, 1);
	setGpioData(39, 1);
	setGpioData(40, 1);
	setGpioData(41, 1);
	setGpioData(42, 1);
	setGpioData(43, 1);

#elif defined(TP_MODEL_C50V3)

/* GPIO1 Mode */
gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
/*                       GPIO11            GPIO5 */
gpiomode &= ~((0x3 << 0) | (0x3 << 20));
/*                    GPIO38         GPIO5 */
gpiomode |= (1 << 14) | (1 << 20);

printf("gpiomode1 %08x.\n", gpiomode);
*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

/* GPIO2 Mode */
gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64));
/*                 GPIO39 ~ GPIO44 */
gpiomode &= ~0x0fff;
/*                 GPIO39 ~ GPIO44 */
gpiomode |= 0x0555;

printf("gpiomode2 %08x.\n", gpiomode);
*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64) = cpu_to_le32(gpiomode);	

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

/* Led */
setGpioData(11, 0);
setGpioData(39, 1);
setGpioData(40, 0);
setGpioData(41, 0);
setGpioData(42, 0);
setGpioData(43, 0);
setGpioData(44, 0);
udelay (1000 * 100 * 10); /* 1s */
setGpioData(11, 0);
setGpioData(39, 1);
setGpioData(40, 1);
setGpioData(41, 1);
setGpioData(42, 1);
setGpioData(43, 1);
setGpioData(44, 1);
#elif defined(TP_MODEL_WA801NDV5)
	/* GPIO Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
	gpiomode |= (1 << 14) | (1 << 16) | (1 << 18) | (1 << 24);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64));
	gpiomode &= ((0xf << 12) | (0xf << 28));
	gpiomode |= (0x555 | (0x555 << 16));
	//printf("gpiomode2 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64) = cpu_to_le32(gpiomode);	

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
	/* GPIO CTRL 1 for GPIO 32~64 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp |= ((1 << (36 - 32)) | (1 << (37 - 32)) | (1 << (42 - 32)) | (1 << (43 - 32)) | (1 << (44 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/* GPIO CTRL 1 for GPIO 32~64 INPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp &= (~(1 << (38 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp &= (~(1 << (46 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/* Led */
	setGpioData(36, 0);
	setGpioData(37, 0);
	setGpioData(42, 1);
	setGpioData(43, 0);
	setGpioData(44, 0);
	udelay (500 * 100 * 10);
	setGpioData(42, 0);
	setGpioData(43, 1);
	udelay (500 * 100 * 10);
	setGpioData(36, 1);
	setGpioData(37, 1);
	setGpioData(42, 0);
	setGpioData(43, 0);
	setGpioData(44, 1);
#elif defined(TP_MODEL_WR841HPV5)
	/* 
     * GPIO List
     *
 	 * GPIO#11: INTERNET ORANGE LED
	 * GPIO#36: POWER LED
	 * GPIO#40: WPS	LED
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

    /* GPIO Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
	gpiomode &= ~((0x3 << 0) | (0x3 << 24));
	gpiomode |= (1 << 14) | (1 << 16) | (1 << 18) | (1 << 24);
//	printf("gpiomode1 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64));
//	printf("gpiomode2 %08x.\n", gpiomode);
	gpiomode &= ((0xf << 12) | (0xf << 28));
//	printf("gpiomode2 %08x.\n", gpiomode);
	gpiomode |= (0x555 | (0x555 << 16));
//	printf("gpiomode2 %08x.\n", gpiomode);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x64) = cpu_to_le32(gpiomode);	

	/* Set Direction to output */
	
	/* GPIO CTRL 0 for GPIO 0~32 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE));
	tmp |= (1 << (11 - 0));
	*(volatile u32 *)(RALINK_PIO_BASE) = tmp;
	
	/* GPIO CTRL 1 for GPIO 32~64 OUTPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp |= ((1 << (36 - 32)) | (1 << (39 - 32)) | (1 << (40 - 32)) | (1 << (41 - 32)) | 
			(1 << (42 - 32)) | (1 << (43 - 32)) | (1 << (44 - 32)) | (1 << (46 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;
	
	/* GPIO CTRL 1 for GPIO 32~64 INPUT */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
	tmp &= ~((1 << (37 - 32)) | (1 << (38 - 32)));
	*(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

	/* Led */
//	setGpioData(11, 1);
	setGpioData(36, 0);
	setGpioData(40, 0);
	setGpioData(41, 0);
	setGpioData(42, 0);
	setGpioData(43, 0);
	setGpioData(44, 0);
	udelay (1000000);
	setGpioData(36, 1);
	setGpioData(40, 1);
	setGpioData(41, 1);
	setGpioData(42, 1);
	setGpioData(43, 1);
	setGpioData(44, 1);
#elif defined(TP_MODEL_WR802NV4)
/* GPIO Mode */
    gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
    gpiomode |= (1 << 14) | (1 << 16) | (1 << 18);
    *(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

  /* GPIO
     * GPIO37:Power
     * GPIO38:RESET Button
     */
    /* Set Direction to output */
    /* GPIO CTRL 1 for GPIO 32~64 OUTPUT */
    tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
    tmp |= (1 << (37 - 32));
    *(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

    /* GPIO CTRL 1 for GPIO 32~64 INPUT */
    tmp = le32_to_cpu(*(volatile u32 *)(RALINK_PIO_BASE + 0x4));
    tmp &= (~(1 << (38 - 32)));
    *(volatile u32 *)(RALINK_PIO_BASE + 0x4) = tmp;

    /* Led */
    setGpioData(37, 0);
    udelay (1000 * 100 * 10); /* 1s */
    setGpioData(37, 1);

#elif defined(TP_MODEL_C2V5)

    /* GPIO
	 ****************LED***************
	 * GPIO11:	POWER
	 * GPIO52:	WLAN(2.4G)
	 * GPIO53:	WLAN(5G)
	 * GPIO54:	LAN
	 * GPIO55:	WPS_LED
	 * GPIO56:	INTERNET_ORANGE
	 * GPIO57:	INTERNET_GREEN
	 ****************BTN***************
	 * GPIO2:	RESET
	 * GPIO13:	WPS/WIFI
	 */

	/* 1.GPIO Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));
	gpiomode |= (1 << 0) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 19);
	gpiomode &= (~(1 << 18));
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

	/* 2.GPIO DIR */
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

	/* 3.GPIO DATA */
	/* Led */
	setGpioData(11, 0);
	setGpioData(52, 0);
	setGpioData(53, 0);
	setGpioData(54, 0);
	setGpioData(55, 0);
	setGpioData(56, 0);
	setGpioData(57, 1);
	udelay (500 * 100 * 10);
	setGpioData(56, 1);
	setGpioData(57, 0);
	udelay (500 * 100 * 10);
	setGpioData(11, 1);
	setGpioData(52, 1);
	setGpioData(53, 1);
	setGpioData(54, 1);
	setGpioData(55, 1);
	setGpioData(56, 1);
	setGpioData(57, 1);

#elif defined(TP_MODEL_C5V4)

	/* GPIO Mode */
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60));

	gpiomode |= (7<<2) | (1<<10) | (1<<15) | (1<<0);
	*(volatile u32 *)(RALINK_SYSCTL_BASE + 0x60) = cpu_to_le32(gpiomode);

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
	tmp |= ((1 << (42-40)) | (1 << (60-40)));
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

	/* Led */
	setGpioData(60, 0);

	setGpioData(42, 0);
	setGpioData(14, 0);
	setGpioData(12, 0);
	setGpioData(9, 1);
	setGpioData(10, 0);
	setGpioData(8, 0);  /* autually output 1, because polarity setting */
	setGpioData(7, 0);
	setGpioData(1, 0);
	
	udelay (1000 * 100 * 10);/* 1s */
	
	setGpioData(9, 0);
	setGpioData(10, 1);
	
	udelay (1000 * 100 * 10);/* 1s */
	
	setGpioData(42, 0);//Do not let Power LED off, fix Bug 202562 
	setGpioData(14, 1);
	setGpioData(12, 1);
	setGpioData(9, 1);
	setGpioData(10, 1);
	setGpioData(8, 1); /* autually output 0, because polarity setting */
	setGpioData(7, 1);
	setGpioData(1, 1);

	setGpioData(60, 1);	

#endif

	return 0;
}

/* port from kernel by wanghao  */
#if defined(TP_MODEL_C2V5)
int getGpioData(u32 gpio, u32 *data)
{
	u32 bit = 0;
	u32 reg = 0;
	u32 tmp = 0;

	/* Get reg and bit of the reg */
	if (gpio > 72)
	{
		printf(": %s, Unsupport GPIO(%d)\n", __FUNCTION__, gpio);
		return -1;
	}
	if (gpio <= 23)
	{
		/* RALINK_REG_PIODATA for GPIO 0~23 */
		reg = RALINK_PIO_BASE + 0x20;
		bit = (1 << gpio);
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
#elif defined(TP_MODEL_C5V4)
int getGpioData(u32 gpio, u32 *data)
{	
	u32 bit = 0;
	u32 reg = 0;
	u32 tmp = 0;	/* Get reg and bit of the reg */	
	if (gpio > 72)	
	{
		puts("Boot: getGpioData() Unsupport GPIO\n");
		return -1;
	}

	if (gpio <= 23)	
	{
		/* RALINK_REG_PIODATA for GPIO 0~23 */		
		reg = RALINK_PIO_BASE + 0x20;
		bit = (1 << gpio);
	}
	else if (gpio <= 39)
	{
		/* RALINK_REG_PIO3924DATA for GPIO 24~39 */		
		reg = RALINK_PIO_BASE + 0x48;
		bit = (1 << (gpio - 24));
	}
	else if (gpio <= 71)
	{
		/* RALINK_REG_PIO7140DATA for GPIO 40~71 */		
		reg = reg = RALINK_PIO_BASE + 0x70;
		bit = (1 << (gpio - 40));
	}
	else /* gpio 72 */	
	{
		/* RALINK_REG_PIO72DATA for GPIO 72 */		
		reg = reg = RALINK_PIO_BASE + 0x98;
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
#define RALINK_PRGIO_ADDR		RALINK_PIO_BASE // Programmable I/O
#define RALINK_REG_PIO3100DATA		(RALINK_PRGIO_ADDR + 0x20)
#define RALINK_REG_PIO6332DATA		(RALINK_PRGIO_ADDR + 0x24)
#define RALINK_REG_PIO9564DATA		(RALINK_PRGIO_ADDR + 0x28)

#define RALINK_REG_PIODATA		(RALINK_PRGIO_ADDR + 0x20)
int getGpioData(u32 gpio, u32 *data)
{
	u32 tmp = 0;

	/* INPUT GPIO
	 * GPIO38:Switch IRQ
	 */
	
	/* Get to reg base on bit */
	tmp = le32_to_cpu(*(volatile u32 *)(RALINK_REG_PIO6332DATA));
	if (( 1 << (38 - 32)) & tmp)
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
/* port end  */

#if 0
value = le32_to_cpu(*(volatile u_long *)(RALINK_SYSCTL_BASE + 0x0034));
u32 gpiomode;
	gpiomode = le32_to_cpu(*(volatile u32 *)(RALINK_REG_GPIOMODE));
	
	/* C2, yuanshang, 2013-11-14
	 * GPIO1,GPIO2: I2C GPIO Mode(bit0)
	 * GPIO11,GPIO13:	UART Full(bit4:2)
	 * GPIO39:	SPI GPIO(bit11) & SPI Ref(bit12) [no need to set bit 1] 
	 * GPIO40,GPIO41,GPIO42:	EPHY LED(bit15)
	 * GPIO72:	WLED GPIO(bit13)
	 */
	/*gpiomode |= RALINK_GPIOMODE_DFT;*/
	gpiomode |= (RALINK_GPIOMODE_I2C) | (RALINK_GPIOMODE_UARTF) | (RALINK_GPIOMODE_EPHY) | (RALINK_GPIOMODE_WLED);
	*(volatile u32 *)(RALINK_REG_GPIOMODE) = cpu_to_le32(gpiomode);


#endif

