#pragma once
#include<stdlib.h>
#include<stdio.h>
#define _RADIX_NDOE_PRINT	//如果定义了此宏，radix_tree_traversal()函数将向屏幕打印遍历结果

#define MEMPAGE 4096
#define INIT_POOL_SIZE (MEMPAGE*1)	//初始内存池大小
#define RADIX_INSERT_VALUE_OCCUPY -1		//该节点已被占用
#define RADIX_INSERT_VALUE_SAME -2		//插入了相同的值
#define RADIX_DELETE_ERROR -3			//删除错误
typedef unsigned int ptr_t;
typedef unsigned int uint32;

#define BITS 2
//返回key指定位的值，位数由BITS指定
#define CHECK_BITS(key,pos) ((((unsigned int)(key))<<sizeof(int)*8-(pos+1)*BITS)>>(sizeof(int)*8-BITS))
//基数树节点
typedef struct radix_node_t radix_node_t;
struct radix_node_t {
	radix_node_t* child[4];
	radix_node_t* parent;
};
typedef struct radix_leafnode_t {
	struct radix_leafnode_t* next;
	uint32 key;//路径
	ptr_t value;//值
}radix_leafnode_t;

//内存池结构，放在内存池的前段
typedef struct radix_pool {
	struct radix_pool* next;
	struct radix_pool* prev;
	char* start;
	size_t size;
}radix_pool, * pool_t;
//基数树管理结构
typedef struct radix_tree_t {
	//指向根节点
	radix_node_t* root;
	//内存池指针，(一页内存)
	pool_t pool;
	//储存已分配但不在树中的节点（双向链表，这里储存其中的一个节点）
	radix_node_t* free;
	//叶节点内存池
	pool_t leafpool;
	//储存已分配但不在树中的叶节点
	radix_leafnode_t* lfree;
}radix_tree_t;

//内存池扩大函数，num：新内存池的大小，=-1使用默认值,单位：页
pool_t get_new_pool(radix_tree_t* t, int num);

//创建一个节点，从内存池中取出可以使用的节点
radix_node_t* radix_node_alloc(radix_tree_t* t);

//创建管理结构
radix_tree_t* radix_tree_create();

//插入
int radix_tree_insert(radix_tree_t* t, uint32 key, ptr_t value);

//由于插入时会创建很多节点，为了提高速度这里只会删除最底层的指定节点
int radix_tree_delete(radix_tree_t* t, uint32 key);

//打印函数，会打印出所有底层节点的长度
//void radix_print(radix_node_t* node);

//节点查找函数
ptr_t radix_tree_find(radix_tree_t * t, uint32 key);
pool_t get_new_leafpool(radix_tree_t* t, int num);
radix_leafnode_t* radix_leafnode_alloc(radix_tree_t* t);
//遍历叶节点，返回叶节点数量
int radix_tree_traversal(radix_tree_t* t);
//遍历叶节点，并指定操作
int radix_tree_traversal_fun(radix_tree_t* t, void(*fun)(uint32, uint32));
