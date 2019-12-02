#include <fal.h>
#include <mycc936.h>
#include <cn_port.h>


extern _font_info ftinfo;

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
