#pragma once
typedef int tree_error;//错误类型
typedef unsigned char byte;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long uint64;
typedef unsigned char uint8;

#ifndef NULL
#define NULL 0
#endif
#define TRUE 1
#define FALSE 0
//文件函数错误代码
#define ERR_NODE_NULL -16			//节点指针为空  
#define ERR_SAME_FILE_NAME -17		//文件名相同冲突
#define ERR_NOT_TAKE_NAME -18		//从节点取出文件名错误
#define ERR_LNODE_SPLIT_FAIL -19		//叶节点分裂失败
#define ERR_BNODE_SPLIT_FAIL -20		//内部节点分裂失败
#define ERR_NODE_SPLIT_FAIL -21		//节点分裂失败
#define ERR_NOT_FOUND_P -22			//寻找p值所在off数组下标出错
#define ERR_OFF_SIZE_ZERO -23		//off数组大小为0
#define ERR_OFF_SIZE_EXCEEDED -24	//off数组大小超出界限
#define ERR_NOT_FOUND_FIRST_FT -25	//没有找到起始文件描述符
#define ERR_INSERT_EN_IS_NULL -26	//待插入的扩展文件名描述符为空
#define ERR_NOT_INSERT -27			//插入失败
#define ERR_TAKE_MEM_FAIL -28		//获取内存空间失败
#define ERR_NOT_TAKE_TIME -29		//获取时间失败
#define ERR_NOT_FOUND_DELFILE -30		//未找到可以删除的文件
#define ERR_NOT_DELETE_FILE -31			//没有删除文件
#define ERR_DISKPOINTER_EXCEEDED -32		//硬盘指针超界
#define ERR_MEMCPY_ERROR -33				//memcpy函数返回错误
#define ERR_NOT_FOUND_PTR_IN_PARNODE -34	//没有在父节点中找到本节点的链接
#define ERR_RADIX_FIND -35
#define ERR_NOT_FOUND_FILE_NAME -36			//没有在节点找到对应的文件名
#define ERR_NOT_FOUND_ROOTNODE -37			//没有找到根节点
#define ERR_NOT_CREATE_BNODE -38			//没有创建内部节点
#define ERR_NOT_CREATE_LNODE -39			//没有创建叶节点
#define ERR_NOT_FOUND_MEMPTR_IN_RADIX -40		//没有在基数树中找到磁盘指针对应的内存指针
#define ERR_NODE_NOT_IN_MEM -41				//节点不在内存中
#define ERR_NODE_TYPE_ERROR -42				//节点类型判断出错
#define ERR_NOT_FOUND_SPACE_IN_DISK -43		//没有在磁盘中找到合适的储存位置

//错误号对应的字符串
typedef struct errString {
	tree_error ernum;
	char str[36];
}errString;

/*const errString ERR_String[] = {
{NULL,"NULL"},{TRUE,"TRUE"},{FALSE,"FALSE"},
{ERR_NODE_NULL,"ERR_NODE_NULL"},
{ERR_SAME_FILE_NAME,"ERR_SAME_FILE_NAME"},
{ERR_NOT_TAKE_NAME,"ERR_NOT_TAKE_NAME"},
{ERR_LNODE_SPLIT_FAIL,"ERR_LNODE_SPLIT_FAIL"},
{ERR_BNODE_SPLIT_FAIL,"ERR_BNODE_SPLIT_FAIL"},
{ERR_NODE_SPLIT_FAIL,"ERR_NODE_SPLIT_FAIL"},
{ERR_NOT_FOUND_P,"ERR_NOT_FOUND_P"},
{ERR_OFF_SIZE_ZERO,"ERR_OFF_SIZE_ZERO"},
{ERR_OFF_SIZE_EXCEEDED,"ERR_OFF_SIZE_EXCEEDED"},
{ERR_NOT_FOUND_FIRST_FT,"ERR_NOT_FOUND_FIRST_FT"},
{ERR_INSERT_EN_IS_NULL,"ERR_INSERT_EN_IS_NULL"},
{ERR_NOT_INSERT,"ERR_NOT_INSERT"},
{ERR_TAKE_MEM_FAIL,"ERR_TAKE_MEM_FAIL"},
{ERR_NOT_TAKE_TIME,"ERR_NOT_TAKE_TIME"},
{ERR_NOT_FOUND_DELFILE,"ERR_NOT_FOUND_DELFILE"},
{ERR_NOT_DELETE_FILE,"ERR_NOT_DELETE_FILE"},
{ERR_DISKPOINTER_EXCEEDED,"ERR_DISKPOINTER_EXCEEDED"},
{ERR_MEMCPY_ERROR,"ERR_MEMCPY_ERROR"},
{ERR_NOT_FOUND_PTR_IN_PARNODE,"ERR_NOT_FOUND_PTR_IN_PARNODE"},
{ERR_RADIX_FIND,"ERR_RADIX_FIND"},
{ERR_NOT_FOUND_FILE_NAME,"ERR_NOT_FOUND_FILE_NAME"},
{ERR_NOT_FOUND_ROOTNODE,"ERR_NOT_FOUND_ROOTNODE"},
{ERR_NOT_CREATE_BNODE,"ERR_NOT_CREATE_BNODE"},
{ERR_NOT_CREATE_LNODE,"ERR_NOT_CREATE_LNODE"},
{ERR_NOT_FOUND_MEMPTR_IN_RADIX,"ERR_NOT_FOUND_MEMPTR_IN_RADIX"},
{ERR_NODE_NOT_IN_MEM,"ERR_NODE_NOT_IN_MEM"},
{ERR_NODE_TYPE_ERROR,"ERR_NODE_TYPE_ERROR"},
{ERR_NOT_FOUND_SPACE_IN_DISK,"ERR_NOT_FOUND_SPACE_IN_DISK"}
};
const int ERR_String_Num = sizeof(ERR_String) / sizeof(errString);*/