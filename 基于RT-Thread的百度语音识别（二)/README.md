## 一. 前言
本篇分享的是项目介绍中的第7点：
* 使用SUFD组件+FAL软件包读取存储在外部flash的中文字库，实现LCD的中文显示，用来显示语音识别结果。

为什么要实现这部分？是因为考虑到语音识别的应用可以不仅仅局限于控制简单的外设。最初的想法是，我不用来控制rgb灯了，除了控制台，我是不是还可以用LCD来显示识别结果；待到可以显示识别结果了，更多的应用场景也就随之打开，比如显示个天气预报什么的...
## 二. 准备工作
1. 首先你必须完成了连载（一）中讲解的内容；
2. 学习汉字显示原理，如何制作字库，如何将字库烧写进flash中（推荐学习正点原子的教程）；
3. 因为本篇分享的是如何读取字库文件，所以你需要自行将字库文件烧写进外部Flash中（字库文件为正点原子FONT文件夹下的内容）；

正点原子提供的中文字库包含了12，16，24，32四种字体大小的字库文件以及unicode和gbk的互换表。这里我为了方便直接运行原子的汉字显示例程，该程序里包含将字库烧写进spi flash的部分。无论你使用何种方式烧写字库，请记住你烧录的地址。

这次我们还将用到RT-Thread的SUFD组件和FAL软件包，简单介绍一下：
#### SUFD（串行闪存通用驱动库）
看中文名就知道了，用来驱动spi flash的。SFUD是一种开源的串行SPI Flash通用驱动库，使用这个库，你就不必自己编写flash驱动了，基本市面上绝大多数的flash，都可以轻轻松松地驱动起来，非常方便。
#### FAL（Flash抽象层）
简单来说，使用该软件包，你可以方便地使用API对flash进行分区管理，读写操作等，支持自定义分区表，不得不说，很强！

fal软件包的使用是本次工程的重点，但使用fal前我们还需要做些准备工作，对fal进行移植：
* 首先你需要对 Flash 设备进行定义；
* 然后定义 Flash 设备表，根据你字库文件在外部Flash的存放位置，为字库文件划分相应的分区；

参考：https://github.com/RT-Thread-packages/fal/blob/master/samples/porting/README.md

## 三. 动手实践
#### 1. 使用ENV工具把SUFD和FAL添加进工程中：
* 打开SUFD组件：
![](http://ww1.sinaimg.cn/large/93ef9e7bly1g94kfo5evwj20ws0jkgmq.jpg)
* 打开fal软件包：
![](http://ww1.sinaimg.cn/large/93ef9e7bgy1g94kksy3sej20ws0jkt9x.jpg)
![](http://ww1.sinaimg.cn/large/93ef9e7bgy1g94kl34vgoj20ws0jkwf9.jpg)

#### 2. 移植fal
2.1 定义flash设备
```c
/* fal_flash_sufd_port.c */
/* 参考自IoT Board SDK */

#include <fal.h>

#ifdef FAL_FLASH_PORT_DRIVER_SFUD
#include <sfud.h>
#include <spi_flash_sfud.h>

sfud_flash sfud_norflash0;

static int fal_sfud_init(void)
{
    sfud_flash_t sfud_flash0 = NULL;
    sfud_flash0 = (sfud_flash_t)rt_sfud_flash_find("qspi10");
    if (NULL == sfud_flash0)
    {
        return -1;
    }

    sfud_norflash0 = *sfud_flash0;
    return 0;
}

static int read(long offset, uint8_t *buf, size_t size)
{
    sfud_read(&sfud_norflash0, nor_flash0.addr + offset, size, buf);

    return size;
}

static int write(long offset, const uint8_t *buf, size_t size)
{
    if (sfud_write(&sfud_norflash0, nor_flash0.addr + offset, size, buf) != SFUD_SUCCESS)
    {
        return -1;
    }

    return size;
}

static int erase(long offset, size_t size)
{
    if (sfud_erase(&sfud_norflash0, nor_flash0.addr + offset, size) != SFUD_SUCCESS)
    {
        return -1;
    }

    return size;
}
const struct fal_flash_dev nor_flash0 = { "nor_flash", 0, (16 * 1024 * 1024), 4096, {fal_sfud_init, read, write, erase} };
#endif /* FAL_FLASH_PORT_DRIVER_SFUD */
```
2.2 定义flash设备表
```c
/* fal_cfg.h */
/* 参考自IoT Board SDK */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include <rtthread.h>
#include <board.h>

/* enable SFUD flash driver sample */
#define FAL_FLASH_PORT_DRIVER_SFUD

extern const struct fal_flash_dev nor_flash0;

/* flash device table */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &nor_flash0,                                                     \
}
/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */
#define FAL_PART_TABLE                                                                                              \
{                                                                                                                   \
    {FAL_PART_MAGIC_WROD,  "easyflash",    "nor_flash",                                    0,       512 * 1024, 0}, \
    {FAL_PART_MAGIC_WROD,   "download",    "nor_flash",                           512 * 1024,      1024 * 1024, 0}, \
    {FAL_PART_MAGIC_WROD, "wifi_image",    "nor_flash",                  (512 + 1024) * 1024,       512 * 1024, 0}, \
    {FAL_PART_MAGIC_WROD,       "font",    "nor_flash",            (512 + 1024 + 512) * 1024,  7 * 1024 * 1024, 0}, \
    {FAL_PART_MAGIC_WROD, "filesystem",    "nor_flash", (512 + 1024 + 512 + 7 * 1024) * 1024,  7 * 1024 * 1024, 0}, \
}
#endif /* FAL_PART_HAS_TABLE_CFG */
#endif /* _FAL_CFG_H_ */
```
#### 3. cn_font.c / cn_port.c / mycc936.c
##### 3.1 cn_port.c
```c
#include <cn_port.h>

_font_info ftinfo =
{
	.ugbkaddr = 0x0000000+sizeof(ftinfo),
	.ugbksize = 174344,
	.f12addr=0x0002A908+sizeof(ftinfo),
	.gbk12size=574560,
	.f16addr = 0x000B6D68+sizeof(ftinfo),
	.gbk16size = 766080,
	.f24addr = 0x00171DE8+sizeof(ftinfo),
	.gbk24size = 1723680,
	.f32addr = 0x00316B08+sizeof(ftinfo),
	.gbk32size = 3064320
};
```
cn_port.c定义了字库文件相对于font分区的位置及大小信息。
##### 3.2 cn_font.c
```c
/*********
cn_font.c
**********/

#include <rtthread.h>
#include <drv_qspi.h>
#include <drv_lcd.h>
#include <fal.h>
#include <cn_font.h>
#include <mycc936.h>
#include <cn_port.h>

extern _font_info ftinfo;

/* 从字库中找出字模 */
void get_hz_mat(unsigned char *code, unsigned char *mat, uint8_t size)
{	
	unsigned char qh, ql;
	unsigned char i;
	unsigned long foffset;
	const struct fal_partition *partition = fal_partition_find("font");
	
	uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size); //得到字体一个字符对应点阵集所占的字节数
	qh =*code;
	ql = *(++code);
	
	if(qh < 0x81 || ql < 0x40 || ql == 0xff || qh == 0xff) //非常用汉字
    {
        for(i = 0; i < csize; i++)*mat++ = 0x00; //填充满格
        return; //结束访问
    }
	if(ql < 0x7f)ql -= 0x40; //注意!
    else ql -= 0x41;

    qh -= 0x81;
    foffset = ((unsigned long)190 * qh + ql) * csize;	//得到字库中的字节偏移量
		
	switch(size)
    {
        case 12:
						fal_partition_read(partition, foffset + ftinfo.f12addr, mat, csize);
            break;

        case 16:
            fal_partition_read(partition, foffset + ftinfo.f16addr, mat, csize);
            break;

        case 24:
            fal_partition_read(partition, foffset + ftinfo.f24addr, mat, csize);
            break;

        case 32:
            fal_partition_read(partition, foffset + ftinfo.f32addr, mat, csize);
            break;

    }
}
	
/* 显示一个指定大小的汉字 */
void show_font(uint16_t x, uint16_t y, uint8_t *font, uint8_t size)
{
	uint16_t colortemp;
    uint8_t sta;
    uint8_t temp, t, t1;
    uint8_t dzk[128];
    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size);			//得到字体一个字符对应点阵集所占的字节数

    if(size != 12 && size != 16 && size != 24 && size != 32)return;	//不支持的size

    get_hz_mat(font, dzk, size);							//得到相应大小的点阵数据

    if((size == 16) || (size == 24) || (size == 32))	//16、24、32号字体
    {
        sta = 8;

        lcd_address_set(x, y, x + size - 1, y + size - 1);

        for(t = 0; t < csize; t++)
        {
            temp = dzk[t];			//得到点阵数据

            for(t1 = 0; t1 < sta; t1++)
            {
                if(temp & 0x80) colortemp = 0x0000;

                else colortemp = 0xFFFF;

                lcd_write_half_word(colortemp);
                temp <<= 1;
            }
        }
    }
    else if(size == 12)	//12号字体
    {
        lcd_address_set(x, y, x + size - 1, y + size - 1);

        for(t = 0; t < csize; t++)
        {
            temp = dzk[t];			//得到点阵数据

            if(t % 2 == 0)sta = 8;

            else sta = 4;

            for(t1 = 0; t1 < sta; t1++)
            {
                if(temp & 0x80) colortemp = 0x0000;

                else colortemp = 0xFFFF;

                lcd_write_half_word(colortemp);
                temp <<= 1;
            }
        }
    }
}
	
/* 在指定位置开始显示一个字符串 */
void show_str(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *str, uint8_t size)
{
	uint16_t x0 = x;
    uint16_t y0 = y;
    uint8_t bHz = 0;   //字符或者中文

    while(*str != 0) //数据未结束
    {
        if(!bHz)
        {
            if(*str > 0x80)bHz = 1; //中文

            else              //字符
            {
                if(x > (x0 + width - size / 2)) //换行
                {
                    y += size;
                    x = x0;
                }

                if(y > (y0 + height - size))break; //越界返回

                if(*str == 13) //换行符号
                {
                    y += size;
                    x = x0;
                    str++;
                }

                else lcd_show_char(x, y, *str, size); //有效部分写入

                str++;
                x += size / 2; //字符,为全字的一半
            }
        }
        else //中文
        {
            bHz = 0; //有汉字库

            if(x > (x0 + width - size)) //换行
            {
                y += size;
                x = x0;
            }

            if(y > (y0 + height - size))break; //越界返回

            show_font(x, y, str, size); //显示这个汉字,空心显示
            str += 2;
            x += size; //下一个汉字偏移
        }
    }
}

```
注：这里需要将LCD驱动drv_lcd.c小改一下，将下面两个函数前面的static删除，并添加到drv_lcd.h中。
```c
static rt_err_t lcd_write_half_word(const rt_uint16_t da);
static void lcd_show_char(rt_uint16_t x, rt_uint16_t y, rt_uint8_t data, rt_uint32_t size);
```

cn_font.c中实现了三个函数：
```c
get_hz_mat()
show_font()
show_str()
```
这三个函数都是移植自正点原子，后两个基本没有变化，你只需注意get_hz_mat()，因为它涉及到了flash的读写，而我们这里不再是裸机的读写方式，而是使用FAL软件包提供的API，尤其注意读写的位置，FAL软件包读的是分区相对位置！
有了上面这三个函数，初始化LCD完成后，你就可以使用show_str( )显示中文啦，例：
```c
show_str(120, 220, 200, 16, (rt_uint8_t *)"霹雳大乌龙", 16);
```
需要注意的是，你的源文件编码不能使用utf-8，否则将会显示错误的编码，这里提供一种解决方案：
使用notepad++，按如下设置：
![](http://ww1.sinaimg.cn/large/93ef9e7bly1g94mda1nv4j208g08d74c.jpg)
##### 3.3 mycc936.c
现在LCD已经可以显示中文了，但是你以为事情到这就结束了吗，no！！！前面我提到，源文件不能使用utf-8编码，是针对源文件中的中文来说的，中文编码若是utf-8，那么你的LCD显示将会是错误的编码，用我上面提供的解决方案可以避免这个问题，但是！相信有人已经猜到了，我们要显示语音识别结果，它可不是直接写在源文件里的，而是我们接收百度语音返回的数据后，解析出来的，而它恰恰就是utf-8编码，你说悲不悲剧。那么怎么解决？mycc936.c就是来解决这个问题的。

汉字显示的原理简单来说就是根据gbk编码从字库中取出相应的字模进行显示，所以我们需要将解析出的识别结果转换成gbk，这个过程需要经历utf-8 -> unicode -> gbk。utf-8和unicode之间有直接的关系，我们只需根据它们之间的关系，编写转换函数即可。而unicode和gbk两者却没有直接的关系了，想要实现它们的转换，就必须进行查表，unicode和gbk各自对应一个数组，这两个数组组成一个对应表，正点原子的字库文件中也包含了这个表。

那为什么叫mycc936呢，相信学习过FATFS文件系统的小伙伴对cc936这个文件都比较熟悉了，cc936是用来支持长文件名的，该文件里包含了两个数组 oem2uni 和 uni2oem ，其实存放的就是unicode和gbk的互相转换的对照表，同时cc936.c还提供了ff_convert函数实现unicode和gbk的互换。现在我们在外部flash里已经存放了这个表，那我们要做的就是移植cc936.c，实现读取flash中的转换表进行unicode转gbk，rtthread的文件系统已经存在一个cc936.c以及ff_convert函数，所以我把我这个命名为mycc936.c和myff_convert。
```c
/* mycc936.c */

#include <fal.h>
#include <mycc936.h>
#include <cn_port.h>

extern _font_info ftinfo;

/**************************/
/*****utf-8 转 unicode*****/
/**************************/
int Utf82Unicode(char* pInput, char* pOutput)
{
    int outputSize = 0; //记录转换后的Unicode字符串的字节数
    *pOutput = 0;
    while (*pInput)
    {
		if (*pInput > 0x00 && *pInput <= 0x7F) //处理单字节UTF8字符（英文字母、数字）
		{
			*pOutput = *pInput;
			 pOutput++;
			*pOutput = 0; //小端法表示，在高地址填补0
		}
		else if (((*pInput) & 0xE0) == 0xC0) //处理双字节UTF8字符
		//else if(*pInput >= 0xC0 && *pInput < 0xE0)
		{
			char high = *pInput;
			pInput++;
			char low = *pInput;
			if ((low & 0xC0) != 0x80)  //检查是否为合法的UTF8字符表示
			{
				return -1; //如果不是则报错
			}
 
			*pOutput = (high << 6) + (low & 0x3F);
			pOutput++;
			*pOutput = (high >> 2) & 0x07;
		}
		else if (((*pInput) & 0xF0) == 0xE0) //处理三字节UTF8字符
		//else if(*pInput>=0xE0 && *pInput<0xF0)
		{
			char high = *pInput;
			pInput++;
			char middle = *pInput;
			pInput++;
			char low = *pInput;
			if (((middle & 0xC0) != 0x80) || ((low & 0xC0) != 0x80))
			{
				return -1;
			}
			*pOutput = (middle << 6) + (low & 0x3F);//取出middle的低两位与low的低6位，组合成unicode字符的低8位
			pOutput++;
			*pOutput = (high << 4) + ((middle >> 2) & 0x0F); //取出high的低四位与middle的中间四位，组合成unicode字符的高8位
		}
		else //对于其他字节数的UTF8字符不进行处理
		{
			return -1;
		}
		pInput ++;//处理下一个utf8字符
		pOutput ++;
		outputSize +=2;
	}
	//unicode字符串后面，有两个\0
	*pOutput = 0;
	 pOutput++;
	*pOutput = 0;
	return outputSize;
}


/**************************/
/***单个unicode字符转gbk***/
/**************************/
unsigned short myff_convert (	/* Converted code, 0 means conversion error */
	unsigned short	src,	/* Character code to be converted */
	unsigned int	dir		/* 0: Unicode to OEMCP, 1: OEMCP to Unicode */
)
{
	const struct fal_partition *partition = fal_partition_find("font");
	unsigned short t[2];
	unsigned short c;
	uint32_t i, li, hi;
	uint16_t n;			 
	uint32_t gbk2uni_offset=0;		  
						  
	if (src < 0x80)c = src;//ASCII,直接不用转换.
	else 
	{
 		if(dir)	//GBK 2 UNICODE
		{
			gbk2uni_offset=ftinfo.ugbksize/2;	 
		}else	//UNICODE 2 GBK  
		{   
			gbk2uni_offset=0;	
		}    
		/* Unicode to OEMCP */
		hi=ftinfo.ugbksize/2;//对半开.
		hi =hi / 4 - 1;
		li = 0;
		for (n = 16; n; n--)
		{
			i = li + (hi - li) / 2;	
			fal_partition_read(partition, ftinfo.ugbkaddr+i*4+gbk2uni_offset, (uint8_t*)&t, 4);
			//W25QXX_Read((u8*)&t,ftinfo.ugbkaddr+i*4+gbk2uni_offset,4);//读出4个字节  
			if (src == t[0]) break;
			if (src > t[0])li = i;  
			else hi = i;    
		}
		c = n ? t[1] : 0;  	    
	}
	return c;
}		   

/**************************/
/*****unicode 转 gbk*******/
/**************************/
void unicode2gbk(uint8_t *pInput,uint8_t *pOutput)
{
	uint16_t temp; 
	uint8_t buf[2];
	while(*pInput)
	{
			buf[0]=*pInput++;
			buf[1]=*pInput++;
 			temp=(uint16_t)myff_convert((unsigned short)*(uint16_t*)buf,0);
			if(temp<0X80){*pOutput=temp;pOutput++;}
			else {*(uint16_t*)pOutput=swap16(temp);pOutput+=2;}
		} 
	*pOutput=0;//添加结束符
}
```
#### 4.使用方式
修改连载（一）中的json解析函数
```c
void bd_data_parse(uint8_t *data)
{
    cJSON *root = RT_NULL, *object = RT_NULL, *item =RT_NULL;

    root = cJSON_Parse((const char *)data);
    if (!root)
    {
        rt_kprintf("No memory for cJSON root!\n");
        return;
    }
		
    object = cJSON_GetObjectItem(root, "result");

    item = object->child;

    rt_kprintf("\nresult	:%s \r\n", item->valuestring);

    unsigned short* buf = malloc(128);
    rt_memset(buf,0,128);
    int size = Utf82Unicode(item->valuestring, (char*)buf);
    unsigned char *buffer = malloc(size);
    rt_memset(buffer,0,size);
    unicode2gbk((uint8_t*)buf,(uint8_t*)buffer);
	
    show_str(20, 140, 200, 32, (uint8_t*)buffer, 32);

    if (root != RT_NULL)
        cJSON_Delete(root);
}
```
用bd命令发送百度官方提供16k.pcm样例：
![](http://ww1.sinaimg.cn/large/93ef9e7bly1g94ri3acqlj20u9042q2x.jpg)
![](http://ww1.sinaimg.cn/large/93ef9e7bly1g94rin0pqqj22uo221kjl.jpg)
验证成功。