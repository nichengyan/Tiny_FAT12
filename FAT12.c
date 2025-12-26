#include "FAT12.h"
#include "W25QXX.h"

static uint8_t FAT12_SharedBuffer[FAT12_ClustSize];
static uint8_t FatBuffer[FatBufferSize];
static uint16_t FATBufferID = 0xffff; // 缓冲区记录
static uint16_t FAT12_ClustBuffer[FAT12_ClustBufferSize];
static uint16_t BlockID = 0xffff;

void FAT12_ReadSector(uint32_t addr, uint8_t *dat)
{ /*扇区地址*/
	uint16_t i;
	for (i = 0; i < 16; i++)
		W25QXX_ReadPage(addr * 4096 + i * 256, (unsigned char *)(dat + i * 256));
}
uint32_t Bytes2Value(uint8_t dat[], uint8_t lenth)
{
	uint32_t res = 0;
	if (lenth >= 1)
		res |= ((uint32_t)(dat[0]));
	if (lenth >= 2)
		res |= ((uint32_t)(dat[1])) << 8;
	if (lenth >= 3)
		res |= ((uint32_t)(dat[2])) << 16;
	if (lenth >= 4)
		res |= ((uint32_t)(dat[3])) << 24;
	return res;
}
uint16_t ConvertFats(uint8_t dat[], uint8_t op)
{ // fat表项解析
	uint16_t res = 0;
	if (!op)
	{
		res |= (dat[1] & 0x0f);
		res <<= 8;
		res |= dat[0];
	}
	if (op)
	{
		res |= dat[2];
		res <<= 4;
		res |= (dat[1] >> 4);
	}
	return res;
}
uint8_t FAT12_IsAscii(uint8_t c)
{
	return (c >= ' ' && c <= '~');
}
void FAT12_MemoryCopy(uint8_t *from, uint8_t *to, uint32_t lenth)
{
	uint32_t i;
	for (i = 0; i < lenth; i++)
		(*(to + i * sizeof(uint8_t))) = (*(from + i * sizeof(uint8_t)));
}
uint8_t FAT12_MemoryCompare(uint8_t *a, uint8_t *b, uint32_t lenth)
{
	uint32_t i;
	for (i = 0; i < lenth; i++)
		if ((*(a + i * sizeof(uint8_t))) != (*(b + i * sizeof(uint8_t))))
			return 0x00;
	return 0xff;
}
uint8_t FAT12_CheckNameOK(uchar str[])
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		if (str[i] == '.' || str[i] == '/' || str[i] == '\\')
			return 0;
	}
	return 0xff;
}
uint16_t FAT12_GetFats(uint16_t FatNum)
{
	uint16_t res = 0, tmp = 0, BID;
	uint8_t flg = 0;
	// FatNum+=2;//跳过前两个标记0xfff和0xff8
	if (FatNum <= 2)
		return 0x03;
	BID = (uint16_t)(FatNum / 2) / (FatBufferSize / 3);
	if (BID != BlockID)
	{
		BlockID = BID;
		FAT12_ReadSector(2, FAT12_SharedBuffer);
		FAT12_MemoryCopy(FAT12_SharedBuffer + BlockID * FatBufferSize, FatBuffer, FatBufferSize);
	}
	flg = FatNum & 0x01;
	tmp = (uint16_t)(FatNum / 2) % (FatBufferSize / 3);
	res = ConvertFats(FatBuffer + tmp * sizeof(uint8_t) * 3, flg);
	return res;
}

//----------------------UserApi----------------------------
uint8_t FAT12_Init()
{
	// Memory_Init();
	FAT12_ReadSector(0, FAT12_SharedBuffer);
	if (FAT12_SharedBuffer[510] == 0x55 && FAT12_SharedBuffer[511] == 0xAA)
		return 0;
	// windows7格式化MB大小的优盘使用FAT2，其核心参数已完全清楚，故不用再解析DBR,象征性的判断一下就好了
	else
		return 0xff;
}
/// @brief 打开一个fat1`
/// @param target 要打开的
/// @param FileName
/// @return
uint8_t FAT12_Fopen(FAT12_File *target, uint8_t FileName[])
{
	uint16_t SecID = 3, Nth = 0, cnt = 0, ClustNow = 0;
	FAT12_FDI *pstr;
	uint8_t NameTmp[15], i;
	uint8_t Flg = 0; // 是否查找到目标文件
	target->File_Id = 0xffff;
	FAT12_ReadSector(SecID, FAT12_SharedBuffer);
	for (i = 0; i < 15; i++)
		NameTmp[i] = 0;
	while (SecID <= 6)
	{
		pstr = (FAT12_FDI *)(FAT12_SharedBuffer + (Nth % 128) * 32);
		if (pstr->Attributes != 0x0F && pstr->Name[0] != 0xE5 && pstr->Name[0] != 0)
		{
			FAT12_MemoryCopy(pstr->Name, NameTmp, 8);
			NameTmp[8] = '.';
			FAT12_MemoryCopy(pstr->Extension, NameTmp + 9 * sizeof(uint8_t), 3);
			if (FAT12_MemoryCompare(NameTmp, FileName, 12) == 0xff)
			{
				target->File_Id = Nth;
				target->File_FirstClus = Bytes2Value(pstr->LowClust, 2);
				target->File_Size = Bytes2Value(pstr->FileSize, 4);
				FAT12_MemoryCopy(NameTmp, target->File_Name, 12);
				target->File_CurrentClusID = 0;
				target->File_CurrentPos = 0;
				target->File_CurrentOffset = 0;
				Flg = 1;
				break;
			}
			// for(int k=0;k<12;k++) printf("%c",NameTmp[k]);
			// printf("\n");
		}
		Nth++;
		if (Nth && Nth % 128 == 0)
		{
			SecID++;
			FAT12_ReadSector(SecID, FAT12_SharedBuffer);
		}
	}
	if (Flg)
	{
		FAT12_ReadSector(1, FAT12_SharedBuffer);
		ClustNow = target->File_FirstClus;
		while (1)
		{
			if (cnt >= FAT12_ClustBufferSize)
				break;
			if (ClustNow == 0x0fff)
				break;
			FAT12_ClustBuffer[cnt++] = ClustNow;
			ClustNow = FAT12_GetFats(ClustNow);
		} // 缓冲簇链
		return 0;
	}
	return 0xff;
}

uint32_t FAT12_Fseek(FAT12_File *f, uint32_t offset, uint8_t op)
{
	uint16_t ClusCnt = 0;
	if (op == 0)
	{ // 从头开始
		if (offset >= f->File_Size)
			return 0xff;
		f->File_CurrentOffset = offset;
		f->File_CurrentPos = f->File_CurrentOffset % FAT12_ClustSize;
		f->File_CurrentClusID = f->File_CurrentOffset / FAT12_ClustSize;
		return f->File_CurrentOffset;
	}
	else if (op == 1)
	{
		if (f->File_CurrentOffset + offset >= f->File_Size)
			return 0xff;
		f->File_CurrentOffset += offset;
		f->File_CurrentPos = f->File_CurrentOffset % FAT12_ClustSize;
		f->File_CurrentClusID = f->File_CurrentOffset / FAT12_ClustSize;
		return f->File_CurrentOffset;
	}
}

uint8_t FAT12_IsEOF(FAT12_File *f)
{
	if (f->File_CurrentOffset >= f->File_Size)
		return 0xff;
	else
		return 0;
}

uint32_t FAT12_Fread(FAT12_File *f, uint8_t *dat, uint32_t num)
{
	uint16_t i;
	if (FAT12_IsEOF(f))
		return 0xff;
	FAT12_ReadSector(FAT12_ClustBuffer[f->File_CurrentClusID] + 5, FAT12_SharedBuffer);
	for (i = 0; i < num && !FAT12_IsEOF(f); i++)
	{
		if (f->File_CurrentPos && f->File_CurrentPos % FAT12_ClustSize == 0)
		{
			f->File_CurrentClusID++;
			f->File_CurrentPos = 0;
			FAT12_ReadSector(FAT12_ClustBuffer[f->File_CurrentClusID] + 5, FAT12_SharedBuffer);
		}
		// printf("0x%02x ",FAT12_SharedBuffer[f->File_CurrentPos]);
		(*(dat + i * sizeof(uint8_t))) = FAT12_SharedBuffer[f->File_CurrentPos];
		f->File_CurrentPos++;
		f->File_CurrentOffset++;
	}
	return f->File_CurrentOffset;
}

uint16_t FAT12_SearchForApp(uint16_t num, FAT12_File *target)
{
	uint16_t SecID = 3, Nth = 0, cnt = 0, ClustNow = 0, total = 0;
	FAT12_FDI *pstr;
	uint8_t NameTmp[15], i;
	for (i = 0; i < 15; i++)
		NameTmp[i] = 0;
	target->File_Id = 0xffff;
	FAT12_ReadSector(SecID, FAT12_SharedBuffer);
	while (SecID <= 6)
	{
		pstr = (FAT12_FDI *)(FAT12_SharedBuffer + (Nth % 128) * 32);
		if (pstr->Attributes != 0x0F && pstr->Name[0] != 0xE5 && pstr->Name[0] != 0 && FAT12_CheckNameOK(pstr->Name))
		{
			if (FAT12_MemoryCompare(pstr->Extension, "BIN", 3) == 0xff || FAT12_MemoryCompare(pstr->Extension, "HEX", 3) == 0xff)
			{
				total++;
				if (total == num)
				{
					FAT12_MemoryCopy(pstr->Name, NameTmp, 8);
					NameTmp[8] = '.';
					FAT12_MemoryCopy(pstr->Extension, NameTmp + 9 * sizeof(uint8_t), 3);
					target->File_Id = Nth;
					target->File_FirstClus = Bytes2Value(pstr->LowClust, 2);
					target->File_Size = Bytes2Value(pstr->FileSize, 4);
					FAT12_MemoryCopy(NameTmp, target->File_Name, 12);
					target->File_CurrentClusID = 0;
					target->File_CurrentPos = 0;
					target->File_CurrentOffset = 0;
				}
			}
		}
		Nth++;
		if (Nth && Nth % 128 == 0)
		{
			SecID++;
			FAT12_ReadSector(SecID, FAT12_SharedBuffer);
		}
	}
	return total;
}
//-----------------V2.0--------------------
// 更新于2024/4/20
// 1:更正簇链缓冲数组FAT12_ClustBuffer类型大小（uint16类型，大小1024）
// 2:更正FAT12_GetFats函数 (uint16_t)
// 3:添加ShowFileInfo函数，便与调试
// 前两个是致命错误，会导致簇号大于256的文件读取异常！！！

// 更新于2024/6/5
// 1：修改了只能读第一个扇区FDI表里文件名的BUG：(Nth%128)
// 在文件较多的时候是致命错误