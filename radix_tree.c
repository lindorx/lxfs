#include"radix_tree.h"
static radix_leafnode_t* last_leafnode;
static int radix_tree_height = sizeof(ptr_t)* 8 / BITS;
//内存池扩大函数，num：新内存池的大小，=-1使用默认值,单位：页
pool_t get_new_pool(radix_tree_t*t, int num)
{
	if (num <= 0)num = INIT_POOL_SIZE;
	else num *= MEMPAGE;
	pool_t pool = (pool_t)malloc(num);
	if (pool == NULL)return NULL;
	pool->next = t->pool->next;
	pool->prev = t->pool;
	t->pool->next->prev = pool;
	t->pool->next = pool;
	t->pool = pool;
	//格式化，将申请的内存全部设置为节点
	radix_node_t * node = (radix_node_t*)((char*)pool + sizeof(radix_pool));
	uint32 i;
	for (i = 0; i < (num - sizeof(radix_pool)-1) / sizeof(radix_node_t); ++i) {
		node[i].parent = &(node[i + 1]);
	}
	node[i++].parent = NULL;
	t->free = node;
	pool->start = (char*)((char*)node + sizeof(radix_node_t)*i);
	pool->size = num - sizeof(radix_node_t)*i - sizeof(radix_pool);
	return pool;
}

pool_t get_new_leafpool(radix_tree_t* t, int num)
{
	if (num <= 0)num = INIT_POOL_SIZE;
	else { num *= MEMPAGE; }
	pool_t leafpool = (pool_t)malloc(num);
	if (leafpool == NULL)return NULL;
	leafpool->next = t->leafpool->next;
	leafpool->prev = t->leafpool;
	t->leafpool->next->prev = leafpool;
	t->leafpool->next = leafpool;
	t->leafpool = leafpool;
	radix_leafnode_t* node = (radix_leafnode_t*)((char*)leafpool + sizeof(radix_pool));
	uint32 i;
	for (i = 0; i < (num - sizeof(radix_pool)-1) / sizeof(radix_leafnode_t); ++i) {
		node[i].next = &(node[i + 1]);
	}
	node[i++].next = NULL;
	t->lfree = node;
	leafpool->start = (char*)((char*)node + sizeof(radix_leafnode_t)* i);
	leafpool->size = num - sizeof(radix_leafnode_t)* i - sizeof(radix_pool);
	return leafpool;
}

radix_leafnode_t* radix_leafnode_alloc(radix_tree_t* t)
{
	radix_leafnode_t* node;
	if (t->free != NULL) {
		node = t->lfree;
		t->lfree = node->next;
	}
	else {
		if (t->leafpool->size < sizeof(radix_leafnode_t)) {
			get_new_leafpool(t, -1);
		}
		if (t->lfree != NULL) {
			node = t->lfree;
			t->lfree = node->next;
		}
		else {
			node = (radix_leafnode_t*)t->leafpool->start;
			t->leafpool->start += sizeof(radix_leafnode_t);
			t->leafpool->size -= sizeof(radix_leafnode_t);
		}
	}
	node->next = (radix_leafnode_t*)NULL;
	node->key = (uint32)NULL;
	node->value = (ptr_t)NULL;
	return node;
}

//创建一个节点，从内存池中取出可以使用的节点
radix_node_t* radix_node_alloc(radix_tree_t* t)
{
	radix_node_t* node;
	if (t->free != NULL) {//从free中提取节点
		node = t->free;
		t->free = node->parent;
	}
	else {//在内存池中寻找可以使用的内存
		if (t->pool->size < sizeof(radix_node_t)) {//如果剩余空间不够分配，则重新分配
			get_new_pool(t, -1);
		}
		if (t->free != NULL) {//从free中提取节点
			node = t->free;
			t->free = node->parent;
		}
		else {
			node = (radix_node_t*)t->pool->start;
			t->pool->start += sizeof(radix_node_t);
			t->pool->size -= sizeof(radix_node_t);
		}
	}
	node->child[0] = NULL;
	node->child[1] = NULL;
	node->child[2] = NULL;
	node->child[3] = NULL;
	node->parent = NULL;
	return node;
}
//创建管理结构
radix_tree_t* radix_tree_create()
{
	int i;
	radix_tree_t* tree = (radix_tree_t*)malloc(sizeof(radix_tree_t));
	if (tree == NULL)return NULL;
	tree->pool = (pool_t)malloc(INIT_POOL_SIZE);
	if (tree->pool == NULL) { free(tree); return NULL; }
	tree->leafpool = (pool_t)malloc(INIT_POOL_SIZE);
	if (tree->leafpool == NULL) { free(tree->pool); free(tree); return NULL; }

	radix_node_t* node = (radix_node_t*)((char*)tree->pool + sizeof(radix_pool));
	tree->pool->next = tree->pool;
	tree->pool->prev = tree->pool;
	for (i = 1; i < (INIT_POOL_SIZE - sizeof(radix_pool)-1) / sizeof(radix_node_t); ++i) {
		node[i].parent = &node[i + 1];
	}
	node[i++].parent = NULL;
	node[0].child[0] = NULL;
	node[0].child[1] = NULL;
	node[0].child[2] = NULL;
	node[0].child[3] = NULL;
	node[0].parent = NULL;
	i *= sizeof(radix_node_t);
	tree->pool->start = ((char*)node + i);
	tree->pool->size = INIT_POOL_SIZE - sizeof(radix_pool)-i;
	//叶节点
	radix_leafnode_t* lnode = (radix_leafnode_t*)((char*)tree->leafpool + sizeof(radix_pool));
	tree->leafpool->next = tree->leafpool;
	tree->leafpool->prev = tree->leafpool;
	for (i = 1; i < (INIT_POOL_SIZE - sizeof(radix_pool)-1) / sizeof(radix_leafnode_t); ++i) {
		lnode[i].next = &(lnode[i + 1]);
	}
	lnode[i++].next = NULL;
	lnode[0].key = (uint32)NULL;
	lnode[0].next = lnode;
	lnode[0].value = (ptr_t)NULL;
	last_leafnode = lnode;
	i *= sizeof(radix_leafnode_t);
	tree->leafpool->start = ((char*)lnode + i);
	tree->leafpool->size = INIT_POOL_SIZE - sizeof(radix_pool)-i;
	tree->free = &node[1];
	tree->lfree = &lnode[1];
	tree->root = node;
	return tree;
}

//插入
int radix_tree_insert(radix_tree_t* t, uint32 key, ptr_t value)
{
	int i, temp;
	radix_node_t* node, *child;
	radix_leafnode_t* lnode;
	node = t->root;
	for (i = 0; i < radix_tree_height - 1; i++) {
		temp = CHECK_BITS(key, i);
		if (!node->child[temp]) {
			child = radix_node_alloc(t);
			if (child == NULL)return -1;
			child->parent = node;
			node->child[temp] = child;
			node = node->child[temp];
		}
		else {
			node = node->child[temp];
		}
	}
	temp = CHECK_BITS(key, i);
	lnode = (radix_leafnode_t*)node->child[temp];
	if (lnode != NULL)return -1;
	lnode = radix_leafnode_alloc(t);
	if (lnode == NULL)return -1;

	lnode->next = last_leafnode->next;
	last_leafnode->next = lnode;
	last_leafnode = lnode;
	node->child[temp] = (radix_node_t*)lnode;
	if (lnode->value == value)return RADIX_INSERT_VALUE_SAME;
	if (lnode->value != (ptr_t)NULL)return RADIX_INSERT_VALUE_OCCUPY;
	lnode->value = value;
	lnode->key = key;
	return 0;
}

//由于插入时会创建很多节点，为了提高速度这里只会删除最底层的指定节点
int radix_tree_delete(radix_tree_t* t, uint32 key)
{
	radix_node_t* node = t->root;
	int i = 0, temp = 0;
	for (i = 0; i < radix_tree_height - 1; ++i) {
		temp = CHECK_BITS(key, i);
		node = node->child[temp];
		if (node == NULL) return RADIX_DELETE_ERROR;
	}
	temp = CHECK_BITS(key, i);
	radix_leafnode_t* lnode;
	lnode = (radix_leafnode_t*)node->child[temp];
	if (lnode == NULL)return RADIX_DELETE_ERROR;
	node->child[temp] = NULL;
	//将lnode回归内存池
	lnode->next = t->lfree->next;
	t->lfree->next = lnode;
	return 0;
}

//节点查找函数
//key为索引，返回叶节点被查找到的值
ptr_t radix_tree_find(radix_tree_t* t, uint32 key)
{
	int i = 0, temp;
	radix_node_t* node;
	node = t->root;
	for (i; i < radix_tree_height - 1; ++i) {
		temp = CHECK_BITS(key, i);
		node = node->child[temp];
		if (node == NULL)return 0;
	}
	temp = CHECK_BITS(key, i);
	radix_leafnode_t* lnode;
	lnode = (radix_leafnode_t*)node->child[temp];
	if (lnode == NULL)return 0;
	return lnode->value;
}

//遍历叶节点，返回叶节点数量
int radix_tree_traversal(radix_tree_t* t)
{
	int i = 0;
	radix_leafnode_t* temp = last_leafnode, *node = last_leafnode;
	do {
#ifdef _RADIX_NDOE_PRINT
		printf("key:%x, value:%x\n", node->key, node->value);
#endif
		node = node->next;
		i++;
	} while (node != temp);
	return i;
}

//遍历叶节点，可以传入一个函数进行处理
int radix_tree_traversal_fun(radix_tree_t* t, void(*fun)(uint32, uint32))
{
	int i = 0;
	radix_leafnode_t* temp = last_leafnode, *node = last_leafnode;
	do {
		//printf("key:%x, value:%x\n", node->key, node->value);
		fun(node->key, node->value);
		node = node->next;
		i++;
	} while (node != temp);
	return i;
}