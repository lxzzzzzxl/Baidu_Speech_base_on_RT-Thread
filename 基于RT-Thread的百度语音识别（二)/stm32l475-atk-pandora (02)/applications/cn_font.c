/*********
cn_font.c
**********/

#include <rtthread.h>
#include <rtdevice.h>	
#include <drv_qspi.h>
#include <drv_lcd.h>
#include <string.h>
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
