#ifndef __FAT12_H__
#define __FAT12_H__

#include "TYPE.h"
#include "_STC8X_.h"
#include "W25QXX.h"
#include "oled.h"
#define FAT12_ClustSize 4096
#define FatBufferSize 300 //FAT表项缓冲，务必务必取3的倍数！！！否则会出错
#define FAT12_CCCB_Size 16
#define FAT12_CCCB_Num 4
/*
typedef struct{
	uint8_t BS_JmpBoot[3];
	uint8_t BS_OEMName[8];
	uint8_t BPB_BytesPerSec[2];
	uint8_t SecPerClus[1];
	uint8_t BPB_ResvdSecCnt[2];
	uint8_t BPB_NumFATs[1];
	uint8_t BPB_RootEntCnt[2];
	uint8_t BPB_TotSec16[2];
	uint8_t BPB_Media[1];
	uint8_t BPB_FATSz16[2];
	uint8_t BPB_SecPerTrk[2];
	uint8_t BPB_NumHeads[2];
    uint8_t BPB_HiddSec[4];
	uint8_t BPB_TotSec32[4];
	uint8_t BS_DrvNum[1];
	uint8_t BS_Reserved1[1];
	uint8_t BS_BootSig[1];
	uint8_t BS_VolID[4];
	uint8_t BS_VolLab[11];
	uint8_t BS_FileSysType[8];
	uint8_t ASMCode [448];
	uint8_t EndSig[2];
}FAT12_DBR;
*/
typedef struct{
 uint8_t Name[8];         // 文件名，不足部分以空格补充
 uint8_t Extension[3]; 	// 扩展名，不足部分以空格补充
 uint8_t Attributes;   	// 文件属性
 uint8_t LowerCase;    	// 0
 uint8_t CTime10ms;   	// 创建时间的10毫秒位
 uint8_t CTime[2];     	// 创建时间
 uint8_t CDate[2];     	// 创建日期
 uint8_t ADate[2];     	// 访问日期
 uint8_t HighClust[2];    // 开始簇的高字
 uint8_t MTime[2];     	// 最近的修改时间
 uint8_t MDate[2];     	// 最近的修改日期
 uint8_t LowClust[2]; 	// 开始簇的低字
 uint8_t FileSize[4];     // 文件大小
}FAT12_FDI;
typedef struct{
	uint16_t File_Id;//ID！=0xffff时认为是合法文件,ID指的是在FDI中的位置 
	uint8_t  File_Name[12];
	uint16_t File_FirstClus;
	uint16_t File_CurrentClusID;//当前簇 
	uint16_t File_CurrentPos;//当前簇内偏移量 
    uint32_t File_CurrentOffset;//当前文件内偏移量 
	uint32_t File_Size; 
	uint8_t File_CCCBId;
}FAT12_File;            //文件的核心
typedef struct{
    uint16_t Block_FirstClust;
    uint16_t Block_Size;
}FAT12_ClustBlock;

void FAT12_ReadSector(uint32_t addr,uint8_t *dat); 
void FAT12_WriteSector(uint32_t addr,uint8_t *dat);

uint32_t Bytes2Value(uint8_t dat[],uint8_t lenth);
uint16_t ConvertFats(uint8_t dat[],uint8_t op);//fat表项解析
uint8_t FAT12_IsAscii(uint8_t c);  
uint8_t FAT12_CheckNameOK(uchar str[]);
void FAT12_MemoryCopy(uint8_t *from,uint8_t *to,uint32_t lenth);
uint8_t FAT12_MemoryCompare(uint8_t *a,uint8_t *b,uint32_t lenth);
uint16_t FAT12_GetFats(uint16_t FatNum);
uint8_t FAT12_GetFreeCCCB();
void FAT12_FreeCCCB(uint8_t i);
uint16_t FAT12_NthClust(FAT12_File *p,uint16_t n);
//----------------------UserApi----------------------------
uint8_t FAT12_Init();
uint8_t FAT12_Fopen(FAT12_File *target,uint8_t FileName[]); //不进行权限设置
uint8_t FAT12_Fclose(FAT12_File *target);
uint32_t FAT12_Fseek(FAT12_File *f,uint32_t offset,uint8_t op);  
uint8_t FAT12_IsEOF(FAT12_File *f);      
uint32_t FAT12_Fread(FAT12_File *f,uint8_t *dat,uint32_t num);
uint16_t FAT12_SearchForApp(uint16_t num,FAT12_File *target);//寻找APP文件（BIN,Hex），返回App总个数,传入第n个App程序,用于枚举
uint16_t FAT12_SearchForFile(uint16_t num,FAT12_File *target,uint8_t *tpye);
void ShowFileInfo(FAT12_File *p);
void test();
#endif

//FAT12_2.0