#include <rtthread.h>	

__packed typedef struct
{
    uint8_t fontok;				//字库存在标志，0XAA，字库正常；其他，字库不存在
    uint32_t ugbkaddr; 			//unigbk的地址
    uint32_t ugbksize;			//unigbk的大小
    uint32_t f12addr;			//gbk12地址
    uint32_t gbk12size;			//gbk12的大小
    uint32_t f16addr;			//gbk16地址
    uint32_t gbk16size;			//gbk16的大小
    uint32_t f24addr;			//gbk24地址
    uint32_t gbk24size;			//gbk24的大小
    uint32_t f32addr;			//gbk32地址
    uint32_t gbk32size;			//gbk32的大小
} _font_info;

