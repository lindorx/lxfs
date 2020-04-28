#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<time.h>
#include"lxerror.h"
#include"lxstruct.h"
#include"glovar.h"
#include"stack.h"
#include"radix_tree.h"

typedef unsigned int Node;//节点指针

#define BOOTSIZE 512	//引导扇区长度
#define BASE_YEAR 2019	//基准年

#define DIS_FT 1		//描述符识别值,文件描述符
#define DIS_EN 0		//描述符识别值,扩展文件名描述符
//节点类型
#define LNODE_TYPE 1
#define BNODE_TYPE 0
//create_tree函数参数
#define CREATE_TREE_DISK 1		//从磁盘读取根节点
#define CREATE_TREE_NEW 0		//新建一个根节点

#define LMAX_EN_NUM		15		//叶节点一组描述符中扩展文件名描述符最大数量,此项可修改,修改的值应大于等于15
#define LMAX_FT_NUM		15		//叶节点一组描述符中扩展文件描述符最大数量,此项不可修改
#define FILE_DES_NUM 1+LMAX_EN_NUM+LMAX_FT_NUM	//叶节点中文件描述符最大数量
#define FILE_NAME_MAX_LENGTH (((LMAX_EN_NUM*31+(1+LMAX_FT_NUM)*6)/31)*31)	//文件名的最大长度
#define calcul_en_num(x) (((x)%31>0)?((x)/31+1):((x)/31))		//计算文件名需要占用的扩展文件名描述符数量

#define BASE_BLOCK 4096		//标准块
#define _ROOTFILENAME "$_0_file_by_lindorx"		//根文件名

//四级权限
#define DPL0 0
#define DPL1 1
#define DPL2 2
#define DPL3 3
#define _ADMIN_DPL DPL0	//管理员权限
#define _USER_DPL DPL3	//普通用户权限

//隐藏
#define _HIDE 1
#define _NO_HIDE 0

//删除
#define _DELETE 1
#define _NO_DELETE 0

//文件夹
#define _FLODER 1
#define _NOT_FLODER 0

//字符串复制函数，到指定字符或者'\0'时终止，目标字符串会包含指定字符，返回复制长度
size_t strccpy(char *destinin,char *source,int ch);

//打印指定文件列表,node为指定的节点,i为该文件夹在lnode的偏移数组下标，print为操作函数，指定对提取出的字符串的处理方式
int fileListPrint(_ln lnode,int i,char dpl);
//跳过指定的文件名,返回下一个描述符组的下标
int skip_folder(_ln* node, int i, int length);
//初始化文件系统
//方法：读取boot信息，为文件系统所需变量赋值
tree_error init_lx();
//创建树，从硬盘读取根节点，存入_file_tree_root
tree_error creat_tree(int todisk);//逻辑：CREATE_TREE_NEW=创建树，CREATE_TREE_DISK=从硬盘读取树
//创建一个空的叶节点 
uint32 creatLNode();
//创建空内部节点
uint32 creatBNode();
//向树插入文件描述符
tree_error insertNode(uint32 disknode, Node node, uint32 nodetype, _fileitems fi, int fisize, const char* fname, int size);
//向叶节点插入文件描述符
tree_error insertLNode(const char* fname, int size, _fileitems fi, int fisize, uint32 disknode, _ln node);
//向内部节点插入文件名
tree_error insertBNode(uint32 disknode, _bn node, _extname in, int length, uint32 child, const char* fname, int size);
//分裂叶节点函数
tree_error splitLNode(uint32 disknode, _ln node, const char* insertname, int insize, int fisize);
//分裂内部节点
tree_error splitBNode(uint32 disknode, _bn node, const char* fname, int size);
//叶节点合并
tree_error mergeLNode(uint32 diskn1, _ln n1, uint32 diskn2, _ln n2);
//在内部节点删除指定文件描述符组以及对应的子节点
tree_error deleteBNode(uint32 disknode, _bn node, const char* fname, int length, uint32 child);
//合并内部节点
tree_error mergeBNode(uint32 diskn1, _bn n1, uint32 diskn2, _bn n2);
//页节点删除
tree_error deleteLNode(uint32 disknode, _ln node, const char* fname, int length);
//删除
tree_error deleteNode(uint32 disknode, Node node, uint32 nodetype, const char* str, size_t length);
//计算叶节点指定位置的文件描述符组长度
int LNodeFTNum(_fileitems fi);
//将文件名在p指定的位置加入内部节点
//str：文件名;size：文件名长度；node：内部节点；p：插入位置
tree_error textInsertBNode(const char* str, int size, _bn node, int p);
//寻找描述符插入位置，返回该位置在off数组的下标
//int found_insert_pos(Node node, const char* fname, int length);
//从叶节点指定位置提取文件名
char* takeFileName(_fileitems fis);
//从内部节点提取文件名
char* takeEnName(_extname en);
//文件名比较函数
#define LARGER_STR1 1		//第一个字符串大
#define LARGER_STR2 -1		//第二个字符串大
#define STR_SAME 0		//两个字符串相同
int cmp_str(const char* a, int asize, const char* b, int bsize);
//在节点的off数组中寻找指定值的下标，算法：折半查找
//p：带查找值；off：数组；left：左边界；right：右边界
int off_find(int p, uint16 off[], int left, int right);
//创建文件描述符组
int creatFileDes(_fileitems fis,//一个用于储存的文件描述符组指针
	const char fname[],//文件名
	size_t fnlen,//文件名长度
	uint64 fsize[],//文件块长度数组
	uint32 fpos[],//文件位置数组
	uint32 length,//fsize,fpos,offset数组长度,这三个数组要拥有相同的数量
	char dpl,//文件权限
	char hide,//隐藏
	char del,//删除
	char en_floder//文件目录项
);
//在内部节点的子节点数组查找指定子节点指针,返回其在数组中的下标
int findPtr(_bn bnode, uint32 diskptr);
//在页节点中寻找指定文件名相同的位置
//返回-1，说明是以fi[0]开始的组，其他值则是在file_off数组的偏移位置
int LNode_FindFileName(_ln node, const char* str, int length);
//在内部节点查找，返回文件描述符组下标
int BNode_FindFileName(_bn node, const char* fname, size_t length);
//节点查找
tree_error findNode(uint32 disknode,Node node,uint32 nodetype,_fileitems fis,const char*fname,int length,_ln *lnode);
//从根节点开始向下寻找指定的文件描述符组，并返回偏移数组下标
//同时会修改lnode指向的节点指针，使该指针指向找到的叶节点
tree_error findNode_i(const char* fname, int length, _ln* lnode);
//返回节点的类型
int discernNodeType(Node node);
//返回子节点下标
int BNodeSearchChild_i(_bn node, const char* str, size_t length);
//在树中寻找首节点地址（即所有值最小的节点，根据分裂的规则，首节点的地址一旦确定便不会改变）
uint32 findStartLNode(Node node);
//文件写入函数
tree_error FileWrite(const char* filename, size_t namesize, char* data, uint64 datasize, char dpl, char hide, char folder);
//文件删除函数
tree_error FileClear(const char* filename);
//打印文件描述符组描述的内容
void printFileTable(_fileitems fis);
//读取根文件目录列表
//fun：根目录名处理函数，此函数不可以操作文件节点，否则会造成节点出错
//每次会将一条文件名作为参数传进fun中。
tree_error findRootDir(int(*fun)(const char* str));

#ifndef DISK_H
#define DISK_H

#define SECSIZE 512
#define BOOTUNIT 8
#define INIT_BLOCK_SIZE SECSIZE*BOOTUNIT

#define LXBOOOT_ADDR 0		//本文件系统BOOT的位置，单位：块
#define LXINFOB_ADDR 1		//lx信息块位置
#define INIT_ROOT_ADDR 33	//初始根节点位置

//备份块
#define BACKUP_ROOT (4+LNODE_SIZE/format_block_size)		//备份根节点块
#define BACKUP_BOOT 2		//备份引导扇区
#define BACKUP_LXINFO 3		//备份信息块

//位图标识符
#define BLOCK_OCC 1			//该块被占用 
#define BLOCK_NOT_OCC 0		//该块未被占用
#define outBitType(src,pos) ((int)(((unsigned char*)(src))[(pos)/8]>>((pos)%8))&0x01)	//输出指定位的值
#define calBlocks(x,b) ((x)/(b)+(((x)%(b)>0)?1:0))		//计算x中包含b的数量，不足一个b算一个b
#define setValueBit(value,pos,x) (((0xffffffff^(0x3<<(pos)*BITS))&(value))|((x)<<(pos)*BITS))

#define HD_SIZE (1024*1024*64+4096)		//硬盘大小
#define HD_PAGE 512					//块的大小
#define HD_SEC 512					//扇区大小
#endif // ! DISK_H

//创建引导扇区
_bootloder creatBootLoder(
	const char* OEM, //厂商名称
	uint16 BytePerSec,//每扇区字节数
	uint8 Unit,//每单元扇区数
	uint16 ResvdSecCnt,//BOOT记录占用的单元数
	uint16 RootEntCnt,//根目录文件最大数
	uint8 Media,//介质
	uint16 SecPerTrk,//每个磁道的扇区数
	uint16 NumHeads,//磁头数
	uint32 HiddSec,//隐藏单元数,以0单元开始
	uint32 TotUnit,//单元总数,即磁盘的最大容量
	uint8 DrvNum,//驱动器号
	uint32 VolID,//卷序列号
	const char* VolLab,//卷标
	const char* FileSysType,//文件系统属性
	uint16 MainVerNum,//主版本号
	uint16 LastVerNum,//副版本号
	uint16 BNodeSize,//内部节点单元数
	uint16 LNodeSize,			//叶节点单元数
	uint32 LogBlockAddr,		//日志表地址
	uint32 LogSize,				//占用块数
	_dpt dpt//分区
);
//从磁盘的指定单元读取数据
//addr:数据要被写入的地址:length:读取长度,单位字节;blockAddr:磁盘被读取的位置
//返回读取的字节数
uint32 readData(char* des, uint32 length, uint32 blockAddr);
//设置一定数量连续块在为图中的指定位
void setDBMPs(_dabmp dmp, uint32 start, uint32 bitlength, byte bit);
//设置位图的指定位
void setDBMP1(_dabmp dmp, uint32 num, byte bit);

int printBit(_dabmp dmp, uint32 length);
//创建一个指定大小的bin镜像文件,用于模拟磁盘
//int init_disk(uint32 size, const char* fname, int length);
int init_disk(const char* fname);

//向硬盘写入数据
//方法:向镜像文件的指定位置写入
//参数:off:硬盘地址;src:源地址;length:写入长度,
int writeData(uint64 off, uint64 length, void* src);
//初始化格式化函数
int init_format();

//对指定镜像进行使用lx文件系统格式化
int format_lx(const char* fname, int length);

//关闭文件系统=》释放镜像文件指针，内存
uint32 closeLX();
int creatRootFile();
//将node指向的节点回写磁盘
//node为磁盘指针
tree_error nodeOutDisk(uint32 disk_node, Node mem_node, uint32 nodetype);
//测试代码，将位图写入一个新文件
void bmpfile(const char* fname);
//void _all_BNode_Out_Disk(radix_node_t* node);
//将全部节点写入磁盘，无返回值
tree_error allNodeOutDisk();
//数据分配管理函数
//blocks：需要分配的块数
uint32 disk_alloc(uint32 blocks);
//节点指针处理函数，使用radix_node_ptr
//方法：将节点的内存指针加入基数树叶节点，使用磁盘指针作为索引
tree_error saveNodePtr(Node node, uint32 diskptr);
//根据磁盘指针读取这个节点的内存指针
//如果节点没有被加载进内存，则进行加载
Node diskPtr_into_memPtr(uint32 diskptr, int nodetype);
_bn diskPtr_into_BNodePtr(uint32 diskptr);
_ln diskPtr_into_LNodePtr(uint32 diskptr);

//自动在磁盘中寻找可以分配数据的地方
//diskptr：数据起始块数组，用来存放可以用来写入的块的位置；
//size：对应块组可以存放的数据量
//length：diskptr和size数组的长度，两个数组应该一样大
//datasize：要分配的数据块占用字节数
//返回数组有效元素数量
int diskAutoAlloc(uint32 diskptr[], uint64 size[], uint32 length, long datasize);
//位图占用堆栈进栈操作，n:发生修改的位图块
tree_error DataBMP_Stack_Push(uint32 n);
//位图回写，将DataBMP回写进磁盘，返回位图占用的块数
int writeDataBMP();
//刷新磁盘缓存，将节点回写磁盘
int flushDiskCache();
//删除指定文件夹下的内容
//off_i指向要删除的目录，如果想要通过文件夹名来索引，则设定为NULL
tree_error del_folder(int off, _ln* node);
//通过根节点获取当前首叶节点
_ln findFirstLNode();
int l_found_name_pos(_ln node,const char *fname,int length);

//文件名定位，根据提供的文件名返回节点中第一个大于等于该文件名的描述符组下标，如果该文件名最大，则返回最大值
int b_found_name_pos(_bn node,const char *fname,int length);