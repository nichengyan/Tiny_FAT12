#include "FAT12_2.h"

static uint8_t FAT12_SharedBuffer[FAT12_ClustSize]; 
static uint8_t FatBuffer[FatBufferSize];
static uint16_t FATBufferID=0xffff;//缓冲区记录
static FAT12_ClustBlock FAT12_CCCB[FAT12_CCCB_Num][FAT12_CCCB_Size];
static uint8_t FAT12_CCCB_Lenth[FAT12_CCCB_Num];//压缩簇链缓冲块长度 
static uint8_t FAT12_ActiveFileNum=0;//活动中的文件数
static uint16_t BlockID=0xffff; 
void FAT12_ReadSector(uint32_t addr,uint8_t *dat){/*扇区地址*/
    if(addr==FATBufferID){
        dat=FAT12_SharedBuffer;
        return;
    }
    FATBufferID=addr;
    W25QXX_ReadPage(addr*4096+0*256,(unsigned char*)(dat+0*256));
    W25QXX_ReadPage(addr*4096+1*256,(unsigned char*)(dat+1*256)); 
    W25QXX_ReadPage(addr*4096+2*256,(unsigned char*)(dat+2*256));
    W25QXX_ReadPage(addr*4096+3*256,(unsigned char*)(dat+3*256)); 
    W25QXX_ReadPage(addr*4096+4*256,(unsigned char*)(dat+4*256));
    W25QXX_ReadPage(addr*4096+5*256,(unsigned char*)(dat+5*256)); 
    W25QXX_ReadPage(addr*4096+6*256,(unsigned char*)(dat+6*256));
    W25QXX_ReadPage(addr*4096+7*256,(unsigned char*)(dat+7*256)); 
    W25QXX_ReadPage(addr*4096+8*256,(unsigned char*)(dat+8*256));
    W25QXX_ReadPage(addr*4096+9*256,(unsigned char*)(dat+9*256)); 
    W25QXX_ReadPage(addr*4096+10*256,(unsigned char*)(dat+10*256));
    W25QXX_ReadPage(addr*4096+11*256,(unsigned char*)(dat+11*256)); 
    W25QXX_ReadPage(addr*4096+12*256,(unsigned char*)(dat+12*256));
    W25QXX_ReadPage(addr*4096+13*256,(unsigned char*)(dat+13*256)); 
    W25QXX_ReadPage(addr*4096+14*256,(unsigned char*)(dat+14*256));
    W25QXX_ReadPage(addr*4096+15*256,(unsigned char*)(dat+15*256));  
}  
uint32_t Bytes2Value(uint8_t dat[],uint8_t lenth){
	uint32_t res=0;
	if(lenth>=1) res|=((uint32_t)(dat[0]));
	if(lenth>=2) res|=((uint32_t)(dat[1]))<<8;
	if(lenth>=3) res|=((uint32_t)(dat[2]))<<16;
	if(lenth>=4) res|=((uint32_t)(dat[3]))<<24;
	return res;
}
uint16_t ConvertFats(uint8_t dat[],uint8_t op){//fat表项解析 
    uint16_t res=0; 
    if(!op){ 
        res|=(dat[1]&0x0f);
		res<<=8;
		res|=dat[0];          	
	}
	if(op){
		res|=dat[2];
	    res<<=4;
		res|=(dat[1]>>4);
	}	
	return res;
}
uint8_t FAT12_IsAscii(uint8_t c){
	return (c>=' '&&c<='~');
}
void FAT12_MemoryCopy(uint8_t *from,uint8_t *to,uint32_t lenth){
    uint32_t i;
	for(i=0;i<lenth;i++) (*(to+i*sizeof(uint8_t)))=(*(from+i*sizeof(uint8_t)));
} 
uint8_t FAT12_MemoryCompare(uint8_t *a,uint8_t *b,uint32_t lenth){
    uint32_t i;
    for(i=0;i<lenth;i++) if((*(a+i*sizeof(uint8_t)))!=(*(b+i*sizeof(uint8_t)))) return 0x00;
    return 0xff;
}
uint8_t FAT12_CheckNameOK(uchar str[]){
    uint8_t i;
    for(i=0;i<8;i++){
        if(str[i]=='.'||str[i]=='/'||str[i]=='\\') return 0;
    }
    return 0xff;
}        
uint16_t FAT12_GetFats(uint16_t FatNum){	
	uint16_t res=0,tmp=0,BID;
	uint8_t flg=0;         
    //FatNum+=2;//跳过前两个标记0xfff和0xff8
    if(FatNum<=2) return 0x03; 
	BID=(uint16_t)(FatNum/2)/(FatBufferSize/3);
	if(BID!=BlockID){
	    BlockID=BID;
		FAT12_ReadSector(2,FAT12_SharedBuffer);	
		FAT12_MemoryCopy(FAT12_SharedBuffer+BlockID*FatBufferSize,FatBuffer,FatBufferSize); 			
	}		
	flg=FatNum&0x01;
    tmp=(uint16_t)(FatNum/2)%(FatBufferSize/3);
	res=ConvertFats(FatBuffer+tmp*sizeof(uint8_t)*3,flg);
	return res;
}
uint8_t FAT12_GetFreeCCCB(){
    uint8_t i;
    for(i=0;i<FAT12_CCCB_Num;i++) if(FAT12_CCCB_Lenth[i]==0) return i;       
    return 0xff;
}
void FAT12_FreeCCCB(uint8_t i){
    if(i>=0&&i<FAT12_CCCB_Num) FAT12_CCCB_Lenth[i]=0;
}    
uint16_t FAT12_NthClust(FAT12_File *p,uint16_t n){
	uint16_t pre[FAT12_CCCB_Size+2];
	uint8_t i;
    i=0;
    while(i<FAT12_CCCB_Lenth[p->File_CCCBId]){
        pre[i]=0;
        ++i;
    }
    i=0;
	while(i<FAT12_CCCB_Lenth[p->File_CCCBId]){
	    if(i==0) pre[i]=FAT12_CCCB[p->File_CCCBId][0].Block_Size;
		else pre[i]=pre[i-1]+FAT12_CCCB[p->File_CCCBId][i].Block_Size;
		if(pre[i]>=n) return FAT12_CCCB[p->File_CCCBId][i].Block_FirstClust+n-(pre[i]-FAT12_CCCB[p->File_CCCBId][i].Block_Size);
        ++i;
	} 
    return 0xffff;
}
//----------------------UserApi----------------------------
uint8_t FAT12_Init(){ 
    uint8_t i,j;
    //read=fopen("img_5.img","rb");
    for(i=0;i<FAT12_CCCB_Num;i++){
        FAT12_CCCB_Lenth[i]=0;
        for(j=0;j<FAT12_CCCB_Size;j++){
            FAT12_CCCB[i][j].Block_FirstClust=0;
            FAT12_CCCB[i][j].Block_Size=0;
        }
    }
    FAT12_ReadSector(0,FAT12_SharedBuffer);  
    if(FAT12_SharedBuffer[510]==0x55&&FAT12_SharedBuffer[511]==0xAA) return 0;
    //windows7格式化MB大小的优盘使用FAT12，其核心参数已完全清楚，故不用再解析DBR,象征性的判断一下就好了
    else return 0xff;
}    
uint8_t FAT12_Fopen(FAT12_File *target,uint8_t FileName[]){
	uint16_t SecID=3,Nth=0,cnt=0,ClustNow=0,ptr=0;
    FAT12_FDI *pstr;
    uint8_t NameTmp[15],i;
    uint8_t Flg=0;//是否查找到目标文件 
    uint8_t End_Flg=0;
    target->File_Id=0xffff;
    FAT12_ReadSector(SecID,FAT12_SharedBuffer);
    for(i=0;i<15;i++) NameTmp[i]=0;
	while(SecID<=6){
	    pstr=(FAT12_FDI*)(FAT12_SharedBuffer+(Nth%128)*32);
		if(pstr->Attributes!=0x0F&&pstr->Name[0]!=0xE5&&pstr->Name[0]!=0){
			FAT12_MemoryCopy(pstr->Name,NameTmp,8);
			NameTmp[8]='.';
			FAT12_MemoryCopy(pstr->Extension,NameTmp+9*sizeof(uint8_t),3);	
			if(FAT12_MemoryCompare(NameTmp,FileName,12)==0xff){
                FAT12_ActiveFileNum++;
			    target->File_Id=Nth;	
				target->File_FirstClus=Bytes2Value(pstr->LowClust,2);
				target->File_Size=Bytes2Value(pstr->FileSize,4);
			    FAT12_MemoryCopy(NameTmp,target->File_Name,12);
			    target->File_CurrentClusID=0;
			    target->File_CurrentPos=0;
			    target->File_CurrentOffset=0; 
                target->File_CCCBId=FAT12_GetFreeCCCB();
                if(FAT12_ActiveFileNum>FAT12_CCCB_Num||target->File_CCCBId==0xff) return 0xff;
			    Flg=1;
			    break;
		    }
			//for(int k=0;k<12;k++) printf("%c",NameTmp[k]);
			//printf("\n");
		}
		Nth++;    
		if(Nth&&Nth%128==0){       
			SecID++;
			FAT12_ReadSector(SecID,FAT12_SharedBuffer);
		}			
	}
	if(Flg){ //查找成功
	    FAT12_ReadSector(1,FAT12_SharedBuffer);	
	    ClustNow=target->File_FirstClus;
	    for(i=0;i<FAT12_CCCB_Size;i++){
	        FAT12_CCCB[target->File_CCCBId][i].Block_FirstClust=0;
			FAT12_CCCB[target->File_CCCBId][i].Block_Size=0;	
		}
        cnt=0;
        ptr=0;
		while(1){
            FAT12_CCCB[target->File_CCCBId][ptr].Block_FirstClust=ClustNow;
            FAT12_CCCB[target->File_CCCBId][ptr].Block_Size=1;
            while(1){
                if(FAT12_GetFats(ClustNow)==ClustNow+1){
                    FAT12_CCCB[target->File_CCCBId][ptr].Block_Size+=1;
                    ClustNow+=1;
                    if(FAT12_GetFats(ClustNow)==0x0fff){
                        End_Flg=1;
                        ptr++;
                        break;
                    }
                }else{
                    ptr++;
                    ClustNow=FAT12_GetFats(ClustNow);
                    break;
                }
            }
            if(End_Flg==1) break;
            if(ptr>=FAT12_CCCB_Size) break;
		}//缓冲簇链 
		FAT12_CCCB_Lenth[target->File_CCCBId]=ptr;
		//printf("%d",ptr);
        return 0;
	} 
	return 0xff;			
}
uint8_t FAT12_Fclose(FAT12_File *target){
    if(target->File_Id!=0xffff){        
        FAT12_FreeCCCB(target->File_CCCBId);
        if(FAT12_ActiveFileNum>0) FAT12_ActiveFileNum--;
        target->File_Id=0xffff;
	    target->File_FirstClus=0;
	    target->File_CurrentClusID=0;//当前簇 
	    target->File_CurrentPos=0;//当前簇内偏移量 
        target->File_CurrentOffset=0;//当前文件内偏移量 
	    target->File_Size=0;
        return 0; 
    }else{
        return 0xff;
    }
}
uint32_t FAT12_Fseek(FAT12_File *f,uint32_t offset,uint8_t op){
	uint16_t ClusCnt=0;
    if(op==0){//从头开始
        if(offset>=f->File_Size) return 0xff;  
        f->File_CurrentOffset=offset;
		f->File_CurrentPos=f->File_CurrentOffset%FAT12_ClustSize;
        f->File_CurrentClusID=f->File_CurrentOffset/FAT12_ClustSize;
		return f->File_CurrentOffset;
	}else if(op==1){
        if(f->File_CurrentOffset+offset>=f->File_Size) return 0xff;  
        f->File_CurrentOffset+=offset;
        f->File_CurrentPos=f->File_CurrentOffset%FAT12_ClustSize;
        f->File_CurrentClusID=f->File_CurrentOffset/FAT12_ClustSize;
		return f->File_CurrentOffset;		
	}	
    return 0xffff;
}
uint8_t FAT12_IsEOF(FAT12_File *f){
	if(f->File_CurrentOffset>=f->File_Size) return 0xff;
	else return 0;
}
uint32_t FAT12_Fread(FAT12_File *f,uint8_t *dat,uint32_t num){
    uint16_t i;
    if(FAT12_IsEOF(f)) return 0xff;
    FAT12_ReadSector(FAT12_NthClust(f,f->File_CurrentClusID)+5,FAT12_SharedBuffer);
    i=0;
    while(i<num&&!FAT12_IsEOF(f)){
    	if(f->File_CurrentPos&&f->File_CurrentPos%FAT12_ClustSize==0){
    		++f->File_CurrentClusID;
    		f->File_CurrentPos=0;
    		FAT12_ReadSector(FAT12_NthClust(f,f->File_CurrentClusID)+5,FAT12_SharedBuffer);
		}
		//printf("0x%02x ",FAT12_SharedBuffer[f->File_CurrentPos]);
		(*(dat+i*sizeof(uint8_t)))=FAT12_SharedBuffer[f->File_CurrentPos];
		f->File_CurrentPos++;
		f->File_CurrentOffset++;
        ++i;
	}
    return f->File_CurrentOffset;
}
uint16_t FAT12_SearchForApp(uint16_t num,FAT12_File *target){
	uint16_t SecID=3,Nth=0,cnt=0,ClustNow=0,total=0;
	uint8_t BIN[]="BIN";
	uint8_t HEX[]="HEX";
    FAT12_FDI *pstr;
    uint8_t NameTmp[15],i;
    for(i=0;i<15;i++) NameTmp[i]=0;
    target->File_Id=0xffff;
    FAT12_ReadSector(SecID,FAT12_SharedBuffer);
	while(SecID<=6){
	    pstr=(FAT12_FDI*)(FAT12_SharedBuffer+(Nth%128)*32);
		if(pstr->Attributes!=0x0F&&pstr->Name[0]!=0xE5&&pstr->Name[0]!=0&&FAT12_CheckNameOK(pstr->Name)){
			if(FAT12_MemoryCompare(pstr->Extension,BIN,3)==0xff||FAT12_MemoryCompare(pstr->Extension,HEX,3)==0xff){
                total++;
                if(total==num){
			        FAT12_MemoryCopy(pstr->Name,NameTmp,8);
			        NameTmp[8]='.';
			        FAT12_MemoryCopy(pstr->Extension,NameTmp+9*sizeof(uint8_t),3);
			        target->File_Id=Nth;	
				    target->File_FirstClus=Bytes2Value(pstr->LowClust,2);
                    target->File_Size=Bytes2Value(pstr->FileSize,4);
                    FAT12_MemoryCopy(NameTmp,target->File_Name,12);
                    target->File_CurrentClusID=0;
                    target->File_CurrentPos=0;
                    target->File_CurrentOffset=0;   
                }                    
            }    
		}
		Nth++;
		if(Nth&&Nth%128==0){
			SecID++;
			FAT12_ReadSector(SecID,FAT12_SharedBuffer);
		}			
	}
    return total;
}  
uint16_t FAT12_SearchForFile(uint16_t num,FAT12_File *target,uint8_t *type){
	uint16_t SecID=3,Nth=0,cnt=0,ClustNow=0,total=0;
    FAT12_FDI *pstr;
    uint8_t NameTmp[15],i;
    for(i=0;i<15;i++) NameTmp[i]=0;
    target->File_Id=0xffff;
    FAT12_ReadSector(SecID,FAT12_SharedBuffer);
	while(SecID<=6){
	    pstr=(FAT12_FDI*)(FAT12_SharedBuffer+(Nth%128)*32);
		if(pstr->Attributes!=0x0F&&pstr->Name[0]!=0xE5&&pstr->Name[0]!=0&&FAT12_CheckNameOK(pstr->Name)){
			if(FAT12_MemoryCompare(pstr->Extension,type,3)==0xff){
                total++;
                if(total==num){
			        FAT12_MemoryCopy(pstr->Name,NameTmp,8);
			        NameTmp[8]='.';
			        FAT12_MemoryCopy(pstr->Extension,NameTmp+9*sizeof(uint8_t),3);
			        target->File_Id=Nth;	
				    target->File_FirstClus=Bytes2Value(pstr->LowClust,2);
                    target->File_Size=Bytes2Value(pstr->FileSize,4);
                    FAT12_MemoryCopy(NameTmp,target->File_Name,12);
                    target->File_CurrentClusID=0;
                    target->File_CurrentPos=0;
                    target->File_CurrentOffset=0;   
                }                    
            }    
		}
		Nth++;
		if(Nth&&Nth%128==0){
			SecID++;
			FAT12_ReadSector(SecID,FAT12_SharedBuffer);
		}			
	}
    return total;
}  
void test(){
    //OLED_ShowNum(0,0,4,
    OLED_ShowNum(0,0,FAT12_CCCB[0][0].Block_FirstClust,7,16);
    OLED_ShowNum(0,2,FAT12_CCCB[0][0].Block_Size,7,16);
    
}
