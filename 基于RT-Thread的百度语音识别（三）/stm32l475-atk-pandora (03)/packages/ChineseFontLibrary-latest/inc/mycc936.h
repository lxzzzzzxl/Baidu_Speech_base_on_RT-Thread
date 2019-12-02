#include <rtthread.h>	

#define swap16(x) (x&0XFF)<<8|(x&0XFF00)>>8	//高低字节交换宏定义

int Utf82Unicode(char* pInput, char* pOutput);

unsigned short myff_convert (	/* Converted code, 0 means conversion error */
	unsigned short	src,	/* Character code to be converted */
	unsigned int	dir		/* 0: Unicode to OEMCP, 1: OEMCP to Unicode */
);
	
void unicode2gbk(uint8_t *pInput,uint8_t *pOutput);
