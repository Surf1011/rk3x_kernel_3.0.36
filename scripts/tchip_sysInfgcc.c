/*
 *  Convert a logo in ASCII PNM format to C source suitable for inclusion in
 *  the Linux kernel
 *
 *  (C) Copyright 2001-2003 by Geert Uytterhoeven <geert@linux-m68k.org>
 *
 *  --------------------------------------------------------------------------
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of the Linux
 *  distribution for more details.
 */
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
//#include <sys/time.h>

#define	MKLINUXLOG
#include "../include/generated/autoconf.h"
#include "../drivers/video/display/screen/screen.h"
#include "../arch/arm/mach-rk30/include/mach/irqs.h"
#include "../arch/arm/mach-rk30/include/mach/gpio.h"
#include "../include/linux/tchip_sysinf.h"
//#include "../arch/arm/mach-rk30/include/mach/rk29_sensor.h"
//#include "../drivers/media/video/rk29_sensor.c"

#include "tchip_devices.c"
//char hardwareStr[200];
#ifndef ARRAYSIZE
#define ARRAYSIZE(a)		(sizeof(a)/sizeof(a[0]))
#endif

#define SET_CAMERA_POS(info, pos)		info = (pos)

#define BAT_ADC_TABLE_LEN               (11 + 1)

#if defined(CONFIG_TCHIP_MACH_TR101)
static int adc_raw_table_bat[BAT_ADC_TABLE_LEN] = 
{
 6806, 7031, 7197, 7265, 7314, 7363, 7402, 7539, 7636, 7744, 7988
};
static int adc_raw_table_ac[BAT_ADC_TABLE_LEN] = 
{
 7060, 7585, 7741, 7929, 7968, 8007, 8066, 8144, 8232, 8349, 8466
};
#else
static int adc_raw_table_bat[BAT_ADC_TABLE_LEN] = 
{
 6806, 7031, 7197, 7265, 7314, 7363, 7402, 7539, 7636, 7744, 7988
};
static int adc_raw_table_ac[BAT_ADC_TABLE_LEN] = 
{
 7060, 7685, 7841, 7929, 7968, 8007, 8066, 8144, 8232, 8349, 8466
};
#endif

int ComposeCameraInfo(char * Info);
int ComposeHDMIInfo(char * Info);
int ComposeWifiInfo(char * Info);
int ComposeCodecInfo(char * Info);
int ComposeModemInfo(char * Info);

char tchip_version[100];
#define SET_ALL_ACTIVE_DEVICE_VERSION(list)	set_all_active_device_version(list, (sizeof(list)/sizeof(list[0])), tchip_version)

static void init_boardVersion(void)
{
       struct tchip_device * cur = GET_CUR_DEVICE(tchip_boards);

       if(0 != cur)
               add2versionex(tchip_version, cur, "");
}

static void init_codecVersion(void)
{
	SET_ALL_ACTIVE_DEVICE_VERSION(tchip_codecs);
}

static void init_touchVersion(void)
{
	SET_ALL_ACTIVE_DEVICE_VERSION(tchip_touchs);
}

static void init_wifiVersion(void)
{
	SET_ALL_ACTIVE_DEVICE_VERSION(tchip_wifis);
}

static void init_modemVersion(void)
{
	SET_ALL_ACTIVE_DEVICE_VERSION(tchip_modems);
}

static void init_hdmiVersion(void)
{
	SET_ALL_ACTIVE_DEVICE_VERSION(tchip_hdmis);
}

static void init_cameraVersion(void)
{
	char str[20] = "_";
#if 0
	cur_sensor_init();

#ifdef CONFIG_TCHIP_MACH_SOC_CAMERA_SP2518_B
       strcat(tchip_version,"_SP2518B");
#endif

#ifdef CONFIG_TCHIP_MACH_SOC_CAMERA_SP2518_F
       strcat(tchip_version,"_SP2518F");
#endif
	if(cur_sensor[SENSOR_BACK]->addr)
	{
		strupper(&str[1], cur_sensor[SENSOR_BACK]->name);
		strcat(tchip_version, str);
	}
	if(cur_sensor[SENSOR_FRONT]->addr)
	{
		strupper(&str[1], cur_sensor[SENSOR_FRONT]->name);
		strcat(tchip_version, str);
	}
#endif
}

static void init_encryptVersion(void)
{
    char str[20] ;

#if defined (CONFIG_ENCRYPTION_DEVICE)
    sprintf(str,"_ATx8", CONFIG_ATXX_DEVICE_TYPE);
#else
    strcpy(str,"_NOAT");
#endif

        strcat(tchip_version, str);
}

static void init_tpType(void)
{
	struct tchip_device * cur = GET_CUR_DEVICE(tchip_tps);

	if(0 != cur)
		add2version(tchip_version, cur);
}

static void init_otgType(void)
{
#if defined (CONFIG_DWC_OTG_BOTH_HOST_SLAVE)
	strcat(tchip_version,"_OTG");
#endif
}
static void init_JogballType(void)
{
#if defined (CONFIG_TCHIP_JOGBALL)
	strcat(tchip_version,"_J");
#endif
}

static void init_curTime(void)
{
	time_t now = time(0);
	struct tm *nowt = localtime(&now);
	char timeStr[8];
	snprintf(timeStr, 7, "%02d%02d%02d", nowt->tm_year % 100, nowt->tm_mon + 1,
			nowt->tm_mday);
	strcat(tchip_version, "@");
	strcat(tchip_version, timeStr);
}

static void init_gpsVersion(void)
{
		#if defined (CONFIG_RK29_GPS)
			strcat(tchip_version,"_GNS7560");
		#endif
}

static void rk29_init_Version(void)
{
	/*
	 tchip_version = (char*)malloc(50);
	 printk("malloc version==================================\n");
	 if(NULL == tchip_version)
	 return;
	 memset(tchip_version, 0, 50);
	 */
	strcat(tchip_version, "(");
	init_boardVersion();
    init_encryptVersion();
	init_codecVersion();
	init_touchVersion();
	init_wifiVersion();
	init_cameraVersion();
	init_hdmiVersion();
	init_gpsVersion();
	init_modemVersion();
	init_tpType();
	init_otgType();
	init_JogballType();
	init_curTime();
	strcat(tchip_version, ")");
}

static FILE *out;
static void write_Inf(void)
{
	char hardwareStr[1024];
	unsigned char ts_config_data[108] =
	{
#if defined (CONFIG_TCHIP_MACH_TR810)
			// Goodix-v1R06 for TR810 <New Firmware IC>
			0x30, 0x19, 0x05, 0x05, 0x28, 0x02, 0x14, 0x20, 0x10, 0x2D, 0xB0,
			0x14, 0x00, 0x1E, 0x00, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD,
			0xE1, 0x00, 0x00, 0x27, 0x2C, 0x4D, 0xC4, 0x20, 0x05, 0x03, 0x83,
			0x50, 0x3C, 0x1E, 0xB4, 0x00, 0x26, 0x2B, 0x01, 0xA9, 0x00, 0x3C,
			0x32, 0x71, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 //
#elif defined (CONFIG_TCHIP_TP_01)
			0x30,0x14,0x05,0x07,0x28,0x02,0x14,0x14,0x10,0x2D,
			0xBA,0x14,0x00,0x1E,0x00,0x01,0x23,0x45,0x67,0x89,
			0xAB,0xCD,0xE1,0x00,0x00,0x36,0x2E,0x4F,0xCF,0x20,
			0x01,0x01,0x81,0x64,0x3C,0x1E,0x28,0x00,0x34,0x2D,
			0x01,0xF7,0x50,0x3C,0x32,0x71,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x01,
#elif  defined (CONFIG_TCHIP_TP_02) //tp
			0x30,0x19,0x05,0x03,0x28,0x02,0x14,0x14,0x10,0x50,
			0xB8,0x14,0x00,0x1E,0x00,0xED,0xCB,0xA9,0x87,0x65,
			0x43,0x21,0x01,0x00,0x00,0x00,0x00,0x4D,0xC1,0x20,
			0x01,0x01,0x83,0x50,0x3C,0x1E,0xB4,0x00,0x0A,0x50,
			0x82,0x1E,0x00,0x6E,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x01,
#endif
			};

	/* open logo file */
	out = fopen(".//drivers/video/logo/tchip_sysInf.c", "w");

	fputs("#include <linux/linux_logo.h>\n", out);
	fputs("#include <linux/stddef.h>\n", out);
	fputs("#include <linux/module.h>\n", out);
	//fputs("#include \"../display/screen/screen.h\"\n", out);
	fputs("const unsigned char TCHIPSYSINFO[1024] = \n", out);

	fputs("{\n", out);

	//添加工具识别android版本的功能，0x11表示android2.3及以下，0x12表示android4.0.3
	//fputs("	0x20,0x11,0x07,0x14,0x41,0x70,0x11,0x02,\n", out);   //android2.3及以下
	//fputs("	0x20,0x12,0x07,0x14,0x41,0x70,0x12,0x02,\n", out);   //android4.0.3
	fputs(" 0x20,0x12,0x06,0x11,0x11,0x60,0x21,0x02,\n", out);        //RK3066 android 4.0.4

	sprintf(hardwareStr, "	0x%02x,0x%02x,0x%02x,0x%02x,\n", H_VD & 0xff, H_VD
			>> 8, V_VD & 0xff, V_VD >> 8);
	//fputs("	H_VD&0xff,H_VD>>8,V_VD&0xff,V_VD>>8,\n",out);	
	fputs(hardwareStr, out);
	fputs("//	USB vendor 12:20\n",out);
	{
		int i=0,j=0;
		char* usbVendor=CONFIG_TCHIP_USB_VENDOR;
		char usbVendorStr[41]="";
		for(i=0;i<7;i++)
		{
			if(i<strlen(usbVendor))
			{
				usbVendorStr[j++]='\'';
				usbVendorStr[j++]=usbVendor[i];
				usbVendorStr[j++]='\'';
				usbVendorStr[j++]=',';
			}
			else
			{
				usbVendorStr[j++]='\'';
				usbVendorStr[j++]='\\';
				usbVendorStr[j++]='0';
				usbVendorStr[j++]='\'';
				usbVendorStr[j++]=',';
			}
		}
		usbVendorStr[j++]='\'';
		usbVendorStr[j++]='\\';
		usbVendorStr[j++]='0';
		usbVendorStr[j++]='\'';
		usbVendorStr[j++]=',';

		fputs("	",out);
		fputs(usbVendorStr,out);
		fputs("\n",out);
	}
	fputs("//	USB product \n",out);

	{
		int i=0,j=0;
		char* usbProduct=CONFIG_TCHIP_USB_PRODUCT;
		char usbProductStr[81]="";
		for(i=0;i<11;i++)
		{
			if(i<strlen(usbProduct))
			{
				usbProductStr[j++]='\'';
				usbProductStr[j++]=usbProduct[i];
				usbProductStr[j++]='\'';
				usbProductStr[j++]=',';
			}
			else
			{
				usbProductStr[j++]='\'';
				usbProductStr[j++]='\\';
				usbProductStr[j++]='0';
				usbProductStr[j++]='\'';
				usbProductStr[j++]=',';
			}
		}
		usbProductStr[j++]='\'';
		usbProductStr[j++]='\\';
		usbProductStr[j++]='0';
		usbProductStr[j++]='\'';
		usbProductStr[j++]=',';

		fputs("	",out);
		fputs(usbProductStr,out);
		fputs("\n",out);
	}

	fputs("//版本信息 32:100\n", out);
	rk29_init_Version();
	{
		int i;
		int j = 0;
		memset(hardwareStr, 0, sizeof(hardwareStr));
		for (i = 0; i < sizeof(tchip_version); i++)
		{
			if (i % 10 == 0)
			{
				hardwareStr[j++] = '\n';
				hardwareStr[j++] = '	';
			}
			hardwareStr[j++] = '\'';
			if (tchip_version[i])
				hardwareStr[j++] = tchip_version[i];
			else
			{
				hardwareStr[j++] = '\\';
				hardwareStr[j++] = '0';
			}
			hardwareStr[j++] = '\'';
			hardwareStr[j++] = ',';
		}
	}

	fputs(hardwareStr, out);
	fputs("\n//硬件开关标志 132:8\n", out);
	int camflg = 0, hdmiflg = 0, gpsflg =0;
#ifdef CONFIG_VIDEO_RK29
	camflg = 1;
#endif

#ifdef CONFIG_HDMI
	hdmiflg = 1;
#endif

#if defined(CONFIG_TCHIP_RK29_GPS)
       gpsflg = 1;
#endif

	sprintf(hardwareStr, "	%d,%d,'\\0',%d, %d,%d, %d,%d,\n", camflg,hdmiflg,
			PIN_BASE & 0xff, (PIN_BASE >> 8) & 0xff,
			/*cur_sensor[SENSOR_FRONT]->gpio_powerdown - PIN_BASE, cur_sensor[SENSOR_BACK]->gpio_powerdown - PIN_BASE,*/0,0,
			gpsflg);
	fputs(hardwareStr, out);

	//输入屏的信处
	fputs("//调屏信息 140:36\n", out);
	sprintf(hardwareStr, "	0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\n",
			OUT_FACE, OUT_CLK & 0xff, (OUT_CLK >> 8) & 0xff, (OUT_CLK >> 16)
					& 0xff, (OUT_CLK >> 24) & 0xff, LCDC_ACLK & 0xff);
	fputs(hardwareStr, out);

	sprintf(hardwareStr, "	0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\n",
			(LCDC_ACLK >> 8) & 0xff, (LCDC_ACLK >> 16) & 0xff,
			(LCDC_ACLK >> 24) & 0xff, H_PW & 0xff, (H_PW >> 8) & 0xff, (H_BP)
					& 0xff);
	fputs(hardwareStr, out);

	sprintf(hardwareStr, "	0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\n", (H_BP
			>> 8) & 0xff, H_VD & 0xff, (H_VD >> 8) & 0xff, (H_FP) & 0xff, (H_FP
			>> 8) & 0xff, V_PW & 0xff);
	fputs(hardwareStr, out);

	sprintf(hardwareStr, "	0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\n", (V_PW
			>> 8) & 0xff, (V_BP) & 0xff, (V_BP >> 8) & 0xff, (V_VD) & 0xff,
			(V_VD >> 8) & 0xff, V_FP & 0xff);
	fputs(hardwareStr, out);

	sprintf(hardwareStr, "	0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\n", (V_FP
			>> 8) & 0xff, LCD_WIDTH & 0xff, (LCD_WIDTH >> 8) & 0xff,
			(LCD_HEIGHT) & 0xff, (LCD_HEIGHT >> 8) & 0xff, DCLK_POL);
	fputs(hardwareStr, out);

	sprintf(hardwareStr, "	0x%02x,0x0,0x0,0x0,0x0,0x0,\n", (SWAP_RB));
	fputs(hardwareStr, out);

	fputs("//电容屏信息 176:108\n", out);
	{
		unsigned char i = 0;
		for (i = 0; i < 108 / 9; i++)
		{
			sprintf(
					hardwareStr,
					"	0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\n",
					ts_config_data[i * 9 + 0], ts_config_data[i * 9 + 1],
					ts_config_data[i * 9 + 2], ts_config_data[i * 9 + 3],
					ts_config_data[i * 9 + 4], ts_config_data[i * 9 + 5],
					ts_config_data[i * 9 + 6], ts_config_data[i * 9 + 7],
					ts_config_data[i * 9 + 8]);
			fputs(hardwareStr, out);
		}
	}

	fputs("//电池容量信息 284:48\n", out);
	{
		unsigned char i = 0;
		for (i = 0; i < 4; i++)
		{
			sprintf(hardwareStr,
					"	0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\n",
\
					adc_raw_table_bat[i * 3] & 0xff, (adc_raw_table_bat[i * 3]
							>> 8) & 0xff, adc_raw_table_bat[i * 3 + 1] & 0xff,
					(adc_raw_table_bat[i * 3 + 1] >> 8) & 0xff,
					adc_raw_table_bat[i * 3 + 2] & 0xff, (adc_raw_table_bat[i
							* 3 + 2] >> 8) & 0xff);
			fputs(hardwareStr, out);
		}

		for (i = 0; i < 4; i++)
		{
			sprintf(hardwareStr,
					"	0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\n",
\
					adc_raw_table_ac[i * 3] & 0xff, (adc_raw_table_ac[i * 3]
							>> 8) & 0xff, adc_raw_table_ac[i * 3 + 1] & 0xff,
					(adc_raw_table_ac[i * 3 + 1] >> 8) & 0xff,
					adc_raw_table_ac[i * 3 + 2] & 0xff, (adc_raw_table_ac[i * 3
							+ 2] >> 8) & 0xff);
			fputs(hardwareStr, out);
		}
	}
	int cameraN, HdmiN, WifiN, CodecN, ModemN;
	memset(hardwareStr, 0, sizeof(hardwareStr));

	cameraN = ComposeCameraInfo(hardwareStr);
	HdmiN = ComposeHDMIInfo(&hardwareStr[100]);
	WifiN = ComposeWifiInfo(&hardwareStr[200]);
	CodecN = ComposeCodecInfo(&hardwareStr[300]);
	ModemN = ComposeModemInfo(&hardwareStr[400]);

	fputs("//硬件单项数目 332:8\n", out);
	char HardInfo[100];
	sprintf(HardInfo, "0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x00,0x00,0x00,\n",
			cameraN, HdmiN, ModemN, WifiN, CodecN);
	fputs(HardInfo, out);

	fputs("//摄像头信息 340:100\n", out);
	memcpy(HardInfo, hardwareStr, 100);
	//memset(HardInfo,0,100);
	//ComposeCameraInfo(HardInfo);
	{
		int i = 0;
		for (i = 0; i < 100; i++)
		{
			if (i % 10 == 0)
				fputs("\n", out);
			char putArr[512];
			sprintf(putArr, "0x%02x,", HardInfo[i]);
			fputs(putArr, out);

		}

		fputs("\n", out);
	}
	fputs("//HDMI信息 440:100\n", out);
	memcpy(HardInfo, &hardwareStr[100], 100);
	//memset(HardInfo,0,100);
	//ComposeHDMIInfo(HardInfo);
	{
		int i = 0;
		for (i = 0; i < 100; i++)
		{
			if (i % 10 == 0)
				fputs("\n", out);
			char putArr[512];
			sprintf(putArr, "0x%02x,", HardInfo[i]);
			fputs(putArr, out);

		}

		fputs("\n", out);
	}
	fputs("//WIFI信息 540:100\n", out);
	memcpy(HardInfo, &hardwareStr[200], 100);
	//memset(HardInfo,0,100);
	//ComposeWifiInfo(HardInfo);
	{
		int i = 0;
		for (i = 0; i < 100; i++)
		{
			if (i % 10 == 0)
				fputs("\n", out);
			char putArr[512];
			sprintf(putArr, "0x%02x,", HardInfo[i]);
			fputs(putArr, out);

		}

		fputs("\n", out);
	}
	fputs("//codec信息 640:100\n", out);
	memcpy(HardInfo, &hardwareStr[300], 100);
	//memset(HardInfo,0,100);
	//ComposeCodecInfo(HardInfo);
	{
		int i = 0;
		for (i = 0; i < 100; i++)
		{
			if (i % 10 == 0)
				fputs("\n", out);
			char putArr[512];
			sprintf(putArr, "0x%02x,", HardInfo[i]);
			fputs(putArr, out);

		}

		fputs("\n", out);
	}

	fputs("//modem信息 740:100\n", out);
	memcpy(HardInfo, &hardwareStr[400], 100);
	{
		int i = 0;
		for (i = 0; i < 100; i++)
		{
			if (i % 10 == 0)
				fputs("\n", out);
			char putArr[512];
			sprintf(putArr, "0x%02x,", HardInfo[i]);
			fputs(putArr, out);

		}

		fputs("\n", out);
	}

	fputs("\n", out);
	fputs("};\n", out);
	fclose(out);

}

int ComposeCameraInfo(char * Info)
{
	int i = 0;
#if 0
	if(TCSI_GET_SENSOR_INDEX(TCSI_CAMERA_END) > 9)
	{
		printf("  %-8sThe number of sensors is overflow(%d > 9)!!!\n", "ERROR:", TCSI_GET_SENSOR_INDEX(TCSI_CAMERA_END), stderr);
		exit(1);
	}

	for(i = 0; i < (TCSI_GET_SENSOR_INDEX(TCSI_CAMERA_END)); i++)
	{
		if(sensors[i].addr)
		{
			if(sensors[i].active)
				Info[i] = sensors[i].pos;

			strcpy(&Info[10 + i * 9], sensors[i].name);
		}
		else
			strcpy(&Info[10 + i * 9], "illegal");

		//printf("-->%9s:%d\n", &Info[10 + i * 9], Info[i]);
	}
#endif
	return i;
}

int ComposeHDMIInfo(char * Info)
{
	int num = GET_DEVICE_LIST(Info, tchip_hdmis);

	return num;
}

int ComposeWifiInfo(char * Info)
{
	int num = GET_DEVICE_LIST(Info, tchip_wifis);

	return num;
}

int ComposeCodecInfo(char * Info)
{
	int num = GET_DEVICE_LIST(Info, tchip_codecs);

	return num;
}

int ComposeModemInfo(char * Info)
{
	int num = GET_DEVICE_LIST(Info, tchip_modems);

	return num;
}


int main(int argc, char *argv[])
{	
	if(2 == argc && !strcmp(argv[1],"pv"))
	{
		// 打印版本号
		rk29_init_Version();
		printf("  %-8s%s\n", "Version:", tchip_version, stderr);
		goto out;
	}

	write_Inf();
out:
	exit(0);
}

