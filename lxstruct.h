#pragma once
#include"lxerror.h"
#pragma pack(1)
//分区表DPT
typedef struct DPT {
	uint8 ActParSig;//活动分区标志,0x80代表活动分区
	uint8 StartHead;//开始磁头
	uint16 StartSec : 6;//开始扇区
	uint16 StartCyl : 10;//开始柱面
	uint8 ParType;//分区类型
	uint8 EndHead;//结束磁头
	uint16 EndSec : 6;//结束扇区
	uint16 EndCyl : 10;//结束柱面
	uint32 OffUnit;//偏移单元数
	uint32 NumUnit;//分区总扇区数
}DPT, * _dpt;

//引导扇区结构
//512BYTES
#define BOOTCodeSize (512-64-2-82)
typedef struct BOOTLoder {
	/*0*/	uint8 jmpBOOT[3];			//跳转代码
	/*3*/	char OEM[8];				//此文件系统开发者名字
	/*11*/	uint16 BytePerSec;			//每扇区字节数
	/*13*/	uint8 Unit;					//文件系统的单元,为BytePerSec的倍数,例:若UNIT=8,BytePerSec=512,则文件系统的单位为4kb
	/*14*/	uint16 ResvdSecCnt;			//BOOT记录占用的单元数
	/*16*/	uint8 Resvered0;
	/*17*/	uint16 RootEntCnt;			//根目录文件最大数
	/*19*/	uint16 TotUnit16;			//单元总数
	/*21*/	uint8 Media;				//介质描述符
	/*22*/	uint16 BlockSize;			//每个块占用的单元数,最大为32MB
	/*24*/	uint16 SecPerTrk;			//每个磁道的扇区数
	/*26*/	uint16 NumHeads;			//磁头数
	/*28*/	uint32 HiddSec;				//隐藏单元数
	/*32*/	uint32 TotUnit32;			//如果TotUnit16=0,则由这里给出单元总数
	/*36*/	uint8 DrvNum;				//驱动器号
	/*37*/	uint8 Resvered1;			//保留字节,置空
	/*38*/	uint8 BootSig;				//扩展引导标记,0x29
	/*39*/	uint32 VolID;				//卷序列号
	/*43*/	char VolLab[11];			//卷标
	/*54*/	char FileSysType[8];		//文件系统属性
	/*62*/	uint16 VerNum[2];			//版本号，VerNum[0]为主版本号，VerNum[1]为子版本号
	/*66*/	uint16 BNodeSize;			//内部节点长度(byte)
	/*68*/	uint16 LNodeSize;			//叶节点长度(byte)
	/*70*/	uint32 LogBlockAddr;		//日志块地址，单位：块
	/*74*/	uint32 LBNum;				//一个日志块占用块数
	/*78*/	uint32 InfoBlockAddr;		//信息块地址
	/*82*/	uint8 boot[BOOTCodeSize];	//boot代码
	/*446*/	DPT dpt[4];					//4个分区表
	/*510*/	uint16 BOOTSign;			//引导扇区标志,0xAA55
}BOOTLoder, * _bootloder;

//负责记录文件系统的相关信息
#define LX_INFO_STRING "LX-lindorx2020"
#define LX_INFO_ROOT_TYPE_LNODE 0x80
#define LX_INFO_ROOT_TYPE_BNODE 0X00
typedef struct LX_INFO_BLOCK {
	uint32 LXInfoBackup;//本块备份位置，默认为分区最后的倒数第二块
	uint32 ROOTNodeAddr;//根节点所在块
	uint32 BOOTBackup;//引导扇区(块)的备份位置，默认为最后一个可以写入数据的块（最后的位图之前）
	uint32 ROOTNodeBackup;//根节点备份位置
	char FileSystem[sizeof(LX_INFO_STRING)];//固定为"LX-lindorx"+此版本的发布年份，例：“LX-lindorx2020”
	byte ROOTType;//根节点类型，0x00=内部节点，0x80=叶节点
}LX_INFO_BLOCK, * _lx_info_block;

//地址的计算方法:以单元为单位,物理地址=Unit*BytePerSec*给出的地址
//例:根文件描述块起始地址=LXSz16*Unit*BytePerSec
//如果unit=8,那么理论上可以表示的磁盘大小为512TB
/*扇区值最大可以为2^16=65536,unit值最大为256，因此单元块的最大值可以为16MB=2^24bytes
*/

//位图，用于文件分配表，使用位区分已分配和未分配的区域，1：已分配，0未分配
extern uint32 DDBMP_SIZE;//单个位图的大小，等于一个块的大小
typedef struct {
	byte bit0 : 1;
	byte bit1 : 1;
	byte bit2 : 1;
	byte bit3 : 1;
	byte bit4 : 1;
	byte bit5 : 1;
	byte bit6 : 1;
	byte bit7 : 1;
}byteBit;
typedef union DABMP {
	byteBit bb;
	byte ch;
}DABMP, * _dabmp;//data allocation bitmap;

//文件/分区描述符属性
#pragma pack(1)
typedef struct {
	byte dpl : 2;			//文件权限；分区权限 0，1，2，3
	byte extname : 1;		//下一项描述符类型,=0，文件描述符；=1，扩展文件名描述符
	byte isext : 1;			//本描述符是否为扩展描述符,=1
	byte data : 1;			//=1文件正常，=0此文件可能存在问题；作为分区描述符时，=1说明此分区是系统分区，
	byte en_folder : 1;		//说明floder项有效,以此文件名作为后面文件的目录基准，同目录下的文件应该只有一个文件的此项为1
	byte hide : 1;			//=1隐藏文件；=1隐藏分区
	byte del : 1;			//=1已删除文件，相当于放入回收站，标记删除，为了保证可以恢复；=1分区删除
}fileAtt;

//文件日期结构
typedef struct {
	uint32 s : 6;		//秒
	uint32 m : 6;		//分
	uint32 h : 5;		//时
	uint32 day : 5;		//日
	uint32 month : 4;	//月
	uint32 year : 6;	//年,使用是将此值加上基准年份，暂定为2019，理想为2020
}fDate, * _fdate;

#define FTNAME_SIZE 6
//文件描述符,32byte
typedef struct {
	byte ms : 7;			//创建时间的10毫秒位,ms*10=大约的创建时间
	byte dis : 1;			//=0扩展文件名描述符，=1文件描述符，此项恒等于1
	fileAtt fatt;			//文件属性
	char name[FTNAME_SIZE];			//文件名,占用6字节
	fDate createdate;		//创建日期
	fDate lastVisitDate;	//最后访问日期
	fDate lastModifiedDate;	//最后修改日期
	uint32 size;				//文件长度，单位4kb,因此一个文件最大为4096GB=4TB
	uint32 position;			//在分区内的偏移位置，单位4kb,最大检索16tb，因此一个分区的最大为16tb
	uint16 offset : 12;		//文件占用的最后一个4kb内的偏移
	uint16 extnum : 4;			//扩展描述符数量,最大为15个，当此值=15时，应当查看最后一个扩展描述符，确定是否还有扩展描述符
	uint16 folder;				//文件夹包含数量，如果en_folder=1，说明以此文件名的目录为基准，其后的文件描述符此项连续的值就属于同一目录，直到碰到下一个此项为0的文件描述符或者拥有完整路径的文件描述符
}fileTable, * _filetable;

//文件名扩展描述符,32byte
typedef struct extName {
	byte size : 5;			//本项文本串长度
	byte ext : 1;			//1=有扩展项,此项为了避免文件描述符指出的15项的限制，以期能够储存更多文件项
	byte start : 1;			//=1起始项，为扩展文件名描述符组的首项
	byte dis : 1;			//=0扩展文件名描述符，=1文件描述符,此项应该恒等于0
	char name[31];			//文件名
}extName, * _extname;

//文件项联合体，一个文件项可能是文件描述符或者扩展文件名描述项
typedef union {
	fileTable ft;			//文件描述符
	extName en;			//扩展文件名描述符
}fileItems, * _fileitems;

//以下为两种节点结构，由每个结构中的第一个描述符确定节点类型，如果首位描述符的dis位=1，则是叶节点，dis=0，则是内部节点
//B+树叶节点
//大小计算公式：size=32*x+2+2+4+4+4+2*(x-1)；x=LNODE_NUM；4kb对齐
#define LNODE_NUM 2409

//B+树内部节点,52kb
#define BNODE_NUM 1401
#define BNODE_SIZE 52*1024
#define CHILD_TYPE_BNODE 0	//childType属性，内部节点类型
#define CHILD_TYPE_LNODE 3	//childType属性，叶节点类型
typedef struct BTreeNode {
	extName name[BNODE_NUM];			//文件名描述符数组
	uint32 child[BNODE_NUM + 1];		//子节点指针数组，可能为内部节点或叶节点，使用时，要强制转换为结点指针后使用
	uint16 name_off[BNODE_NUM - 1];		//记录每个文件名的起始下标,从第二个文件名开始
	uint32 parent;			//父节点指针
	uint16 name_off_num:14;				//记录name_off数组的有效长度
	uint16 childType:2;					//子节点类型，0=内部节点，3=叶节点
	uint16 namenum;						//记录name数组的长度
}BTreeNode, * _btreenode, * _bn;

//叶节点,80kb
#define LNODE_SIZE 80*1024
typedef struct LeafNode {
	fileItems fi[LNODE_NUM];			//文件描述符数组
	uint16 file_off[LNODE_NUM - 1];		//文件偏移数组，描述文件的起始描述符位下标，由于第一个文件项的下标肯定为0，因此从第二个文件描述符开始。
	uint16 finum;						//fi数组有效项的长度
	uint16 file_off_num;				//file_off数组有效项的数量
	uint32 prev;				//前驱节点
	uint32 next;				//后继节点
	uint32 parent;			//父节点指针
}LeafNode, * _leafnode, * _ln;

typedef struct {
	uint32 node;
	uint32 type;
}FileTreeRoot;

#pragma pack()