#pragma once
#include"lxerror.h"
#include"lxstruct.h"
#include"radix_tree.h"
#include"stack.h"
//全局变量头文件
extern _bootloder DiskBOOT;//指向从磁盘读取的引导扇区
//系统基础变量
extern unsigned char UNIT;//储存UNIT值
extern uint16 BytePerSec;//单个扇区占用字节数
extern uint16 RootEntCnt;//根节点最大文件数
extern uint32 BLOCK_SIZE;//块的大小
extern uint32 BLOCK_GROUP_SIZE;//块组占用块数
extern uint32 DISK_SIZE;//硬盘容量,块
extern uint64 DISK_SIZE_BYTES;//硬盘容量：字节
//数据位图
extern uint32 sizeDAB;//数据位图占用字节数
extern uint32 BGSize;//块组大小，block group size
extern uint64 DAB_0;//0号位图位置，单位块
extern _dabmp DataBMP;//位图存储块，将磁盘的位图全部存入内存
extern uint32 DataBMPNum;//位图数量
//根节点
extern uint32 sizeLFDB;//叶节点文件描述块占用字节数
extern uint32 sizeBFDB;//内部节点文件描述块占用字节数
//日志块
extern uint32 addrLB_0;//日志块首地址
extern uint32 sizeLB;//日志块占用字节数
//B+树相关
extern uint32 RootAddr;//根节点地址，该节点必须放在某个文件描述块的起始处，这样才可以被索引
extern _lx_info_block _lxinfoblock;//信息块指针
extern uint32 mem_lnode_num;//内存中当前拥有的叶节点节点数量
extern uint32 mem_bnode_num;//内存中当前拥有的内部节点数量
//格式化所需要参数
extern uint32 format_block_size;			//块长度
extern uint8 format_unit;					//单元值
extern uint16 format_bytepersec;			//扇区长度
extern uint32 format_root;					//根节点地址
extern uint64 format_root_addr;
extern uint32 format_backup_root;			//备份根节点地址
extern uint64 format_backup_root_addr;
extern uint32 format_lxinfo;				//lx信息块地址
extern uint64 format_lxinfo_addr;
extern uint32 format_backup_lxinfo;			//备份信息块地址
extern uint64 format_backup_lxinfo_addr;
extern uint32 format_boot;					//引导扇区地址
extern uint64 format_boot_addr;
extern uint32 format_backup_boot;			//备份引导扇区地址
extern uint64 format_backup_boot_addr;

extern uint32 format_disk_n;				//硬盘容量，单位：块
extern uint64 format_disk_size;				//硬盘容量，单位：byte
extern uint32 block_group_n;				//块组占用块数，单位：块
extern uint64 block_group_size;				//块组占用块数，单位：byte
extern uint32 end_block_group_n;			//最后的块组占的块数，单位：块
extern uint64 end_block_group_size;			//最后的块组占的块数，单位：byte

//文件分配函数变量
extern char* filenamebuf;//用于储存从节点中提出的文件名，首先要初始化
extern int fnb_p;//储存filenamebuf的有效长度
extern FileTreeRoot _file_tree_root;//储存根节点指针

//基数树
extern radix_tree_t* radix_node_ptr;//储存节点指针，内存指针存入基数树叶节点，磁盘指针作为索引
//堆栈
extern Stack DataBMP_Stack;//位图堆栈