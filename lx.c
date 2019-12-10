
#include"lx.h"
char* filenamebuf;
int fnb_p = 0;
_ln mem_start_lnode;
uint32 disk_start_lnode;
_bootloder DiskBOOT;

unsigned char UNIT;
uint16 BytePerSec=512;
uint16 RootEntCnt;
uint32 BLOCK_SIZE;
uint32 BLOCK_GROUP_SIZE;
uint32 DISK_SIZE;
uint64 DISK_SIZE_BYTES;

uint32 sizeDAB;
uint32 BGSize;
uint64 DAB_0;
_dabmp DataBMP;
uint32 DataBMPNum;
Stack DataBMP_Stack;

uint32 addrFDB_0;
uint32 addrFDB_1;
uint32 sizeLFDB;
uint32 sizeBFDB;

uint32 addrLB_0;
uint32 sizeLB;

uint32 lxInfoBlock;
_lx_info_block _lxinfoblock;
uint32 RootAddr;
uint32 mem_lnode_num;
uint32 mem_bnode_num;

FileTreeRoot _file_tree_root;

#define MAX(a,b) ((a>b)?a:b)


uint32 format_block_size;			
uint8 format_unit;					
uint16 format_bytepersec;			
uint32 format_root;					
uint64 format_root_addr;
uint32 format_backup_root;			
uint64 format_backup_root_addr;
uint32 format_lxinfo;				
uint64 format_lxinfo_addr;
uint32 format_backup_lxinfo;			
uint64 format_backup_lxinfo_addr;
uint32 format_boot;					
uint64 format_boot_addr;
uint32 format_backup_boot;			
uint64 format_backup_boot_addr;

uint32 format_disk_n;				
uint64 format_disk_size;				
uint32 block_group_n;				
uint64 block_group_size;				
uint32 end_block_group_n;			
uint64 end_block_group_size;			

_lx_info_block lib;
_bootloder bootloder512;
uint32 DataBMP_Stack_sp = 0;
FILE* wdisk;
FILE* rdisk;

char _disk_filename[260] = { 0 };
radix_tree_t* radix_node_ptr;


#define TEXT_BUF_SIZE 	(FILE_NAME_MAX_LENGTH-FILE_NAME_MAX_LENGTH%4+4)	
#define DATABMP_STACK_MAX_SIZE 4096			



tree_error init_lx()
{
	uint32 i;
	
	DiskBOOT = (_bootloder)malloc(BOOTSIZE);
	if (DiskBOOT == NULL)return -1;
	readData((char*)DiskBOOT, BOOTSIZE, 0);
	UNIT = DiskBOOT->Unit;
	BytePerSec = DiskBOOT->BytePerSec;
	BLOCK_SIZE = UNIT * BytePerSec;
	if (BLOCK_SIZE == 0)return -1;
	BLOCK_GROUP_SIZE = BLOCK_SIZE*8;
	DISK_SIZE = DiskBOOT->TotUnit32;
	DISK_SIZE_BYTES = DISK_SIZE * BLOCK_SIZE;
	sizeDAB = BLOCK_SIZE;
	DataBMPNum = DISK_SIZE / BLOCK_GROUP_SIZE+ (DISK_SIZE % BLOCK_GROUP_SIZE > 0 ? 1 : 0);
	sizeLB = DiskBOOT->LBNum * BLOCK_SIZE;
	RootEntCnt = DiskBOOT->RootEntCnt;
	DAB_0 = (BGSize - BNODE_SIZE) * BLOCK_SIZE;
	
	_lxinfoblock = (_lx_info_block)malloc(BLOCK_SIZE);
	if (_lxinfoblock == NULL)return -1;
	readData((char*)_lxinfoblock, BLOCK_SIZE, DiskBOOT->InfoBlockAddr);
	
	RootAddr = _lxinfoblock->ROOTNodeAddr;
	
	filenamebuf = (char*)malloc(TEXT_BUF_SIZE);
	if (filenamebuf == NULL)return -1;
	fnb_p = 0;
	mem_lnode_num = 0;
	mem_bnode_num = 0;
	radix_node_ptr = radix_tree_create();
	if (radix_node_ptr == NULL)return -1;
	
	DataBMP = (_dabmp)malloc(DataBMPNum * sizeDAB);
	if (DataBMP == NULL)return -1;
	for (i = 0; i < DISK_SIZE / BLOCK_GROUP_SIZE; ++i) {
		readData((char*)((int)DataBMP + i * sizeDAB), sizeDAB, BLOCK_GROUP_SIZE * (i + 1)-1);
	}
	if (DISK_SIZE % BLOCK_GROUP_SIZE > 0) {
		readData((char*)((int)DataBMP + i * sizeDAB), sizeDAB, DISK_SIZE - 1);
	}
	
	DataBMP_Stack = S_CreateStack(DATABMP_STACK_MAX_SIZE);
	return TRUE;
}


tree_error creat_tree(int todisk)
{
	switch (todisk) {
	case CREATE_TREE_NEW: {
		_file_tree_root.node = creatLNode();
		_file_tree_root.type = LNODE_TYPE;
		_ln root = diskPtr_into_LNodePtr(_file_tree_root.node);
		root->next= _file_tree_root.node;
		root->prev= _file_tree_root.node;
	}break;
	case CREATE_TREE_DISK: {
		if (_lxinfoblock->ROOTType == LX_INFO_ROOT_TYPE_LNODE) {
			_ln root = (_ln)malloc(sizeof(LeafNode));
			if (root == NULL) { return -1; }
			_file_tree_root.node = RootAddr;
			_file_tree_root.type = LNODE_TYPE;
			saveNodePtr((Node)root, _file_tree_root.node);
			readData((char*)root, sizeof(LeafNode), RootAddr);
		}
		else if (_lxinfoblock->ROOTType == LX_INFO_ROOT_TYPE_BNODE) {
			_bn root = (_bn)malloc(sizeof(BTreeNode));
			if (root == NULL)return -1;
			_file_tree_root.node = RootAddr;
			_file_tree_root.type = BNODE_TYPE;
			saveNodePtr((Node)root, _file_tree_root.node);
			readData((char*)root, sizeof(BTreeNode), RootAddr);
		}
		else return ERR_NOT_FOUND_ROOTNODE;
	}break;
	}
	
	disk_start_lnode = _file_tree_root.node;
	mem_start_lnode = diskPtr_into_LNodePtr(disk_start_lnode);
	return 0;
}

uint32 creatLNode()
{
	
	uint32 diskptr = disk_alloc(sizeof(LeafNode) / BLOCK_SIZE);
	if (diskptr == (uint32)NULL) return (uint32)NULL;
	_ln node = (_ln)malloc(sizeof(LeafNode));
	if (node != NULL) {
		saveNodePtr((Node)node, diskptr);
		node = (_ln)memset(node, 0, sizeof(LeafNode));
	}
	mem_lnode_num++;
	return diskptr;
}
uint32 creatBNode()
{
	uint32 diskptr = disk_alloc(sizeof(BTreeNode) / BLOCK_SIZE);
	if (diskptr == (uint32)NULL)return (uint32)NULL;
	_bn node = (_bn)malloc(sizeof(BTreeNode));
	if (node != NULL) {
		saveNodePtr((Node)node, diskptr);
		node = (_bn)memset(node, 0, sizeof(BTreeNode));
	}
	mem_bnode_num++;
	return diskptr;
}

tree_error insertLNode(const char* fname, int size, _fileitems fi, int fisize, uint32 disknode,_ln node)
{
	if (node == NULL) {
		node = diskPtr_into_LNodePtr(disknode);
		if (node == NULL)return ERR_NODE_NULL;
	}
	int i = 0, j=0, temp=-1,p=-1, left = 0, right = 0, sret;
	char find = FALSE;
	/*±éÀúÊý×é£¬¼ìË÷Î»ÖÃ */
	if (node->finum + fisize >= LNODE_NUM) {
		
		sret=splitLNode(disknode,node, fname, size, fisize);
		if (sret < 0)return sret;
		
		return insertNode((uint32)node->parent,(uint32)NULL,BNODE_TYPE, fi, fisize, fname, size);
	}
	else {
		
		
		if (node->finum == 0) {
			p = 0; temp = 0;
		}
		else {
			
			temp = found_insert_pos((Node)node, fname, size);
			if (temp < 0) {
				if (temp == -1) {
					p = 0; temp = 0;
					node->file_off[0] = fisize;
				}
				else if (temp == -2) {
					p = node->finum;
					temp = node->file_off_num;
					node->file_off[temp] = p;
				}
				else return temp;
			}
			else {
				p = node->file_off[temp];
			}
			
			for (i = node->file_off_num; i > temp; --i) {
				node->file_off[i] = node->file_off[i - 1] + fisize;
			}
			node->file_off_num++;
		}
		
		for (i =((int) node->finum) - 1, j = i + fisize; i >= p; --i, --j) {
			node->fi[j] = node->fi[i];
		}
		
		for (j=0; j < fisize; ++j) {
			node->fi[p+j] = fi[j];
		}
		node->finum += fisize;
	}
	return 0;
}



tree_error splitLNode(uint32 disknode,_ln node,const char* insertname, int insize,int fisize)
{
	if (node==NULL){
		node = diskPtr_into_LNodePtr(disknode);
		if (node == NULL)return ERR_NODE_NULL;
	}
	int mid = LNODE_NUM / 2,i, j,i_off,namesize;
	char found_name = FALSE, cmp_small = 0, insert_pos = 0,isinsert=FALSE;
	char* name;
	
	for (i = mid; i >= 0; --i) {
		
		if (node->fi[i].ft.dis && !node->fi[i].ft.fatt.isext) {
			found_name = TRUE;
			break;
		}
	}
	
	if (!found_name)return ERR_NOT_FOUND_FIRST_FT;
	
	i_off = off_find(i, node->file_off, 0, node->file_off_num-1);
	if (i_off < 0)return ERR_NOT_FOUND_P;
	
	name = takeFileName(&(node->fi[node->file_off[i_off]]));
	if (name == NULL)return ERR_NOT_TAKE_NAME;
	namesize = fnb_p;
	cmp_small = cmp_str(name, namesize, insertname, insize);
	switch (cmp_small) {
	case 1:insert_pos = 0; break;
	case 0:return ERR_SAME_FILE_NAME;
	case -1:insert_pos = 1;
		
		j = i + fisize;
		while (i < j) {
			i = node->file_off[++i_off];
		}
		break;
	}
	
	
	uint32 disknewnode = creatLNode();
	_ln newnode = diskPtr_into_LNodePtr(disknewnode);
	if (newnode==NULL)return ERR_NODE_NULL;
	int ii=i,new_i=0, new_offi = 0,off_off=0;
	
	
	
	for (j = i_off + 1, new_offi; j < node->file_off_num; ++j, new_offi++) {
		
		while (ii < node->file_off[j]) {
			newnode->fi[new_i++] = node->fi[ii++];
		}
		
		newnode->file_off[new_offi] = new_i;
	}
	
	while (ii < node->finum) {
		newnode->fi[new_i++] = node->fi[ii++];
	}
	newnode->finum = node->finum - i;
	newnode->file_off_num = node->file_off_num - i_off - 1;
	node->finum = i;
	node->file_off_num = i_off;
	newnode->next = node->next;
	newnode->prev = disknode;
	_ln n = diskPtr_into_LNodePtr(node->next);
	if (n == NULL)return ERR_NODE_NULL;
	n->prev = disknewnode;
	node->next = disknewnode;
	
	name = takeFileName(&(node->fi[i]));
	if (name == NULL)return ERR_NOT_TAKE_NAME;
	char midname[TEXT_BUF_SIZE];
	//memcpy_s(midname, fnb_p, name, fnb_p);
	memcpy(midname,name,fnb_p);
	midname[fnb_p] = '\0';
	if (name == NULL)return ERR_NOT_TAKE_NAME;
	
	if (node->parent == (uint32)NULL) {
		_file_tree_root.node = creatBNode();
		if (_file_tree_root.node == (uint32)NULL)return ERR_NOT_CREATE_BNODE;
		_file_tree_root.type = BNODE_TYPE;
		node->parent = _file_tree_root.node;
		newnode->parent = _file_tree_root.node;
		_bn par =diskPtr_into_BNodePtr( _file_tree_root.node);
		par->child[0] = disknode;
	}
	else {
		newnode->parent = node->parent;
	}
	return insertBNode((uint32)node->parent, NULL, NULL, 0, (uint32)disknewnode, midname, fnb_p);
}

tree_error splitBNode(uint32 disknode,_bn node,const char*fname,int size)
{
	if (node == NULL) {
		node = diskPtr_into_BNodePtr(disknode);
		if (node == NULL)return ERR_NODE_NULL;
	}
	int mid,i,off_i,child_i,child_mid;
	int mmid,off_mid;
	_bn par;
	uint32 diskpar;
	
	for (mid= BNODE_NUM / 2; mid > 0; --mid) {
		if (node->name[mid].start) break;
	}
	
	uint32 disknewnode = creatBNode();
	_bn newnode = diskPtr_into_BNodePtr(disknewnode);
	if(newnode==NULL)return ERR_NOT_CREATE_BNODE;
	
	off_mid = off_find(mid, node->name_off, 0, node->name_off_num - 1);
	
	mmid = node->name_off[off_mid+1];
	
	
	i = 0; off_i = 0; child_i = 0; child_mid = off_mid + 2;
	while (i < node->namenum - mmid) {
		newnode->child[child_i++] = node->child[child_mid++];
		do {
			newnode->name[i] = node->name[mmid+i];
		} while (node->name[i++].ext);
		newnode->name_off[off_i++] = i;
	}
	newnode->child[child_i++] = node->child[node->name_off_num+1];
	
	newnode->namenum = i;
	newnode->name_off_num = off_i - 1;
	node->namenum = mid;
	node->name_off_num = off_mid;
	
	if (node->parent == (uint32)NULL) {
		diskpar = creatBNode();
		par = diskPtr_into_BNodePtr(diskpar);
		par->child[0] = disknode;
		_file_tree_root.node = diskpar;
		_file_tree_root.type = BNODE_TYPE;
	}
	else {
		diskpar = (uint32)node->parent;
	}
	return insertBNode(diskpar,NULL, &(node->name[mid]), mmid - mid, disknewnode, NULL, 0);
}



tree_error insertBNode(uint32 disknode,_bn node,_extname in, int length,uint32 child,const char *fname,int size)
{
	if (node == NULL) {
		node = diskPtr_into_BNodePtr(disknode);
		if (node == NULL)return ERR_NODE_NULL;
	}
	if (length == 0 && size == 0)return ERR_INSERT_EN_IS_NULL;
	if (in == NULL)length = calcul_en_num(size);
	if (node->namenum > BNODE_NUM - length) {
		tree_error err=splitBNode(disknode,node, fname, size);
		if (err < 0)return err;
		return insertBNode(disknode,node, in, length, child, fname, size);
	}
	
	int off = found_insert_pos((Node)node, fname, size),ins,i;
	if (off < 0) {
		if (off == -1) {
			ins = 0; off = -1;
		}
		else if (off == -2) {
			ins = node->namenum; off = node->name_off_num;
		}
		else return off;
	}
	else {
		ins = node->name_off[off]; 
	}
	
	if (in == NULL) {
		
		textInsertBNode(fname, size, node, ins);
	}
	else {
		
		if(node->namenum>0)for (i = node->namenum-1; i >ins; ++i) {
			node->name[i + length] = node->name[i];
		}
		for (i = 0; i < length; ++i) {
			node->name[ins + i] = in[i];
		}
		node->namenum += length;
		
		if (off >= 0) {
			for (i = node->name_off_num; i > off; --i) {
				node->name_off[i] = node->name_off[i - 1] + length;
			}
			node->name_off_num++;
			node->name_off[off] = ins;
		}
	}
	
	for (i = node->name_off_num + 1; i > off+2; --i) {
		node->child[i] = node->child[i - 1];
	}
	
	node->child[i] = child;
	return 0;
}

tree_error insertNode(uint32 disknode,Node node,uint32 nodetype,_fileitems fi,int fisize,const char*fname,int size)
{
	if (disknode == (uint32)NULL)return ERR_NODE_NULL;
	if (node == (Node)NULL) {
		node = diskPtr_into_memPtr(disknode, nodetype);
		if (node == (uint32)NULL)return ERR_NODE_NULL;
	}
	int i, cmp;
	char* name;
	do {
		if (nodetype == LNODE_TYPE) {
			return insertLNode(fname, size, fi, fisize, disknode, (_ln)node);
		}
		else {
			i = BNodeSearchChild_i((_bn)node, fname, size);
			if (i < 0)return ERR_NOT_INSERT;
			disknode = ((_bn)node)->child[i];
			
			node = diskPtr_into_memPtr(disknode, nodetype);
			if (node == (Node)NULL)return ERR_NODE_NULL;
			if (discernNodeType(node) != nodetype) {
				nodetype = LNODE_TYPE;
				node = (Node)diskPtr_into_LNodePtr(disknode);
			}
		}
	} while (node);
	return ERR_NOT_INSERT;
}


tree_error mergeLNode(uint32 diskn1,_ln n1,uint32 diskn2, _ln n2)
{
	
	if (n1 == NULL) {
		n1 = diskPtr_into_LNodePtr(diskn1);
		if (n1 == NULL) { return ERR_NODE_NULL; }
	}
	if (n2 == NULL) {
		n2 = diskPtr_into_LNodePtr(diskn2);
		if (n2 == NULL)return ERR_NODE_NULL;
	}
	int i,n1num,n2num, temp;
	n1num = n1->finum;
	n2num = n2->finum;
	n1->finum +=n2num;
	for (i = n1num; i <n1->finum; ++i) {
		n1->fi[i] = n2->fi[i - n1num];
	}
	n1->file_off[n1->file_off_num] = n1num;
	temp = n1->file_off_num + 1;
	n1->file_off_num = temp + n2->file_off_num;
	for (i = temp; i < n1->file_off_num; ++i) {
		n1->file_off[i] = n2->file_off[i - temp]+n1num;
	}
	n1->next = n2->next;
	n1 = diskPtr_into_LNodePtr((uint32)n1->next);
	n1->prev = diskn1;
	
	if (n2->parent != (uint32)NULL) {
		temp = deleteBNode((uint32)n2->parent, NULL, NULL, 0, diskn2);
	}
	else temp = 0;
	free(n2);
	mem_start_lnode--;
	
	return temp;
}


tree_error deleteBNode(uint32 disknode,_bn node, const char* fname, int length,uint32 child)
{
	if (node == NULL) {
		node = diskPtr_into_BNodePtr(disknode);
		if (node == NULL)return ERR_NODE_NULL;
	}
	
	int child_pos,fi_pos, interval,i,temp;
	if (child == (uint32)NULL) {
		temp = BNode_FindFileName(node, fname, length);
		if (temp == -1) {
			fi_pos = 0; child_pos = 1;
			interval = (node->name_off[0] > 0) ? node->name_off[0] : node->namenum;
		}
		else if (temp == -2) {
			return ERR_NOT_DELETE_FILE;
		}
		else if(temp<0)return temp;
		else {
			fi_pos = node->name_off[temp];
			child_pos = temp + 2;
			if (temp < node->name_off_num - 1)interval = node->name_off[temp + 1] - node->name_off[temp];
			else interval = node->namenum - node->name_off[temp];
		}
	}
	else {
		temp = findPtr(node, child);
		if (temp < 1) {
			return ERR_NOT_DELETE_FILE;
		}
		child_pos = temp;
		if (temp == 1) {
			fi_pos = 0; 
			interval = (node->name_off[0] > 0) ? node->name_off[0] : node->namenum;
		}
		else {
			fi_pos = node->name_off[temp - 2];
			if (temp < node->name_off_num - 1)interval = node->name_off[temp + 1] - node->name_off[temp];
			else interval = node->namenum - node->name_off[temp-2];
		}
	}
	
	for (i = fi_pos+interval; i < node->namenum; ++i) {
		node->name[i - interval] = node->name[i];
	}
	node->namenum -= interval;
	
	for (i = child_pos + 1; i < node->name_off_num + 2; ++i) {
		node->child[i - 1] = node->child[i];
	}
	
	for (i = child_pos; i < node->name_off_num; ++i) {
		node->name_off[i-1] = node->name_off[i]-interval;
	}
	node->name_off_num--;
	if (node->parent == (uint32)NULL) {
		if (node->namenum == 0) {
			_ln n = diskPtr_into_LNodePtr(node->child[0]);
			if (n == NULL)return ERR_NODE_NULL;
			if (discernNodeType((Node)n) == BNODE_TYPE){ 
				_file_tree_root.node = node->child[0];
				_file_tree_root.type = BNODE_TYPE;
			}
			else {
				_file_tree_root.node = node->child[0];
				_file_tree_root.type = LNODE_TYPE;
			}
			free(node);
			mem_bnode_num--;
			return 0;
		}
		else return 0;
	}
	
	if (node->namenum > BNODE_NUM / 4) return 0;
	_bn n = diskPtr_into_BNodePtr(node->parent);
	if (n == NULL)return ERR_NOT_FOUND_PTR_IN_PARNODE;
	temp = findPtr(n, disknode);
	if (temp < 0)return ERR_NOT_FOUND_PTR_IN_PARNODE;
	_bn n1,n2;
	uint32 diskn1, diskn2;
	if (temp == 0) {
		n1 = diskPtr_into_BNodePtr(node->child[1]);
		if (n1 == NULL)return 0;
		if (node->namenum + n1->namenum < BNODE_NUM - 31) {
			n1 = node; 
			n2 = NULL;
			diskn1 = disknode;
			diskn2 = node->child[1];
		}
		else return 0;
	}
	else {
		n1 = diskPtr_into_BNodePtr(node->child[temp-1]);
		if (n1 == NULL)return 0;
		if (node->namenum + n1->namenum < BNODE_NUM - 31) {
			n1 = NULL;
			n2 = node;
			diskn1= node->child[temp - 1];
			diskn2 = disknode;
		}
		if ((temp + 1) >= node->name_off_num + 2)return 0;
		n1 = diskPtr_into_BNodePtr(node->child[temp + 1]);
		if (n1 == NULL)return 0;
		if (node->namenum + n1->namenum < BNODE_NUM - 31) {
			n1 = node;
			n2 = NULL;
			diskn1 = disknode;
			diskn2= node->child[temp + 1];
		}
		else return 0;
	}
	
	return mergeBNode(diskn1,n1,diskn2, n2);
}


tree_error mergeBNode(uint32 diskn1,_bn n1,uint32 diskn2, _bn n2)
{
	if (n1 == NULL) {
		n1 = diskPtr_into_BNodePtr(diskn1);
		if (n1 == NULL) { return ERR_NODE_NULL; }
	}
	if (n2 == NULL) {
		n2 = diskPtr_into_BNodePtr(diskn2);
		if(n2==NULL)return ERR_NODE_NULL;
	}
	
	uint32 i,n1num,n2num;
	
	char* str;
	int err,temp;
	_ln n = diskPtr_into_LNodePtr(n2->child[0]);
	if (n == NULL)return ERR_NODE_NULL;
	if (discernNodeType((Node)n) == LNODE_TYPE) {
		str = takeFileName(n->fi);
	}
	else {
		_bn bnode = diskPtr_into_BNodePtr(n2->child[0]);
		str = takeEnName(bnode->name);
	}
	if (str == NULL)return ERR_NOT_TAKE_NAME;
	
	char midname[TEXT_BUF_SIZE];
	memcpy(midname,str,fnb_p);
	err=insertBNode(diskn1,n1, NULL, 0, n2->child[0], midname, fnb_p);
	if (err < 0)return err;
	
	n1num = n1->namenum; n2num = n2->namenum;
	n1->namenum = n1num + n2num;
	for (i = n1num; i < n1->namenum; ++i) {
		n1->name[i] = n2->name[i - n1num];
	}
	
	n1num = n1->name_off_num + 2; n2num = n2->name_off_num + 1;
	for (i = n1num; i < n1num + n2num; ++i) {
		n1->child[i] = n2->child[i - n1num];
	}
	
	n1->name_off[n1->name_off_num++] = n1num;
	n1->name_off[n1->name_off_num++] = calcul_en_num(fnb_p)+n1num;
	n1num = n1->name_off_num;
	n1->name_off_num += n2->name_off_num;
	for (i = n1->name_off_num; i < n1->name_off_num; ++i) {
		n1->name_off[i] = n2->name_off[i - n1->name_off_num]+n1num;
	}
	
	err=deleteBNode((uint32)n2->parent,NULL, NULL, 0, diskn2);
	
	free(n2);
	mem_bnode_num--;
	return err;
}


tree_error deleteLNode(uint32 disknode,_ln node,const char * fname,int length)
{
	
	
	if (node == NULL) {
		node = diskPtr_into_LNodePtr(disknode);
		if (node == NULL)return ERR_NODE_NULL;
	}
	int i,num,pos,temp;
	int off_pos;
	num = LNode_FindFileName(node, fname, length);
	if (num == -1) { pos = 0; num = 0; }
	else if (num < 0)return ERR_NOT_FOUND_DELFILE;
	else{ pos = node->file_off[num]; }
	temp = LNodeFTNum(&(node->fi[pos]));
	if (temp == -1)return ERR_NOT_DELETE_FILE;
	node->file_off_num--;
	if (num != node->file_off_num) {
		
		for (i = pos + temp; i < node->finum; ++i) {
			node->fi[i-temp] = node->fi[i];
		}
		
		for (i = num+1; i < node->file_off_num; ++i) {
			node->file_off[i-1] = node->file_off[i] - temp;
		}
	}
	node->finum -= temp;
	if ((uint32)node->prev == disknode)return 0;
	_ln node1, node2;
	uint32 diskn1, diskn2;
	
	if (node->finum > LNODE_NUM/4) return 0;
	
	node1 = diskPtr_into_LNodePtr((uint32)node->prev);
	node2 = diskPtr_into_LNodePtr((uint32)node->next);
	if (node1 == NULL)return 0;
	if (disknode!=disk_start_lnode && node1->finum + node->finum < LNODE_NUM - 31) {
		node1 = NULL;
		node2 = node;
		diskn1= (uint32)node->prev;
		diskn2 = disknode;
	}
	else if (node->next == disk_start_lnode)return 0;
	else if (node2->finum + node->finum < LNODE_NUM - 31) {
		node1 = node;
		node2 = NULL;
		diskn1 = disknode;
		diskn2= (uint32)node->next;
	}
	else return 0;
	
	return mergeLNode(diskn1,node1,diskn2, node2);
}


tree_error deleteNode(uint32 disknode,Node node,uint32 nodetype, const char* str, size_t length)
{
	if (disknode == (uint32)NULL)return ERR_NODE_NULL;
	if (node == (Node)NULL) {
		node = diskPtr_into_memPtr(disknode, nodetype);
		if (node == (Node)NULL)return ERR_NODE_NULL;
	}
	int temp;
	do {
		if (nodetype == LNODE_TYPE) {
			return deleteLNode(disknode,(_ln) node, str, length);
		}
		else {
			temp = BNodeSearchChild_i((_bn)node, str, length);
			if (temp < 0)return temp;
			disknode = ((_bn)node)->child[temp];
			node = diskPtr_into_memPtr(disknode, nodetype);
			if (node == (Node)NULL)return ERR_NODE_NULL;
			if (discernNodeType(node) != nodetype) {
				nodetype = LNODE_TYPE;
				node = (Node)diskPtr_into_LNodePtr(disknode);
			}
		}
	} while (node);
	return ERR_NOT_DELETE_FILE;
}


int LNodeFTNum(_fileitems fi)
{
	if (fi->ft.dis == DIS_EN)return -1;
	int num = fi->ft.extnum;
	if (!fi->ft.fatt.extname) {
		num += fi[num].ft.extnum;
	}
	return num+1;
}
 


tree_error textInsertBNode(const char* str,int size,_bn node,int p)
{
	int i=0, j,n,k=0;
	int en_size = calcul_en_num(size);
	if(node->namenum!=0)
	if (p<node->namenum) {
		
		for (j = node->namenum - 1, i = j + en_size; j>=p; i--,j--) {
			node->name[i] = node->name[j];
		}
		
		n = off_find(p, node->name_off, 0, node->name_off_num - 1);
		if (n == ERR_NOT_FOUND_P)return n;
		for (i = node->name_off_num; i > ((n < 0) ? 0:n); --i) {
			node->name_off[i] = node->name_off[i - 1] + en_size;
		}
		if (n == -1)node->name_off[0] = en_size;
		node->name_off_num++;
	}
	else {
		node->name_off[node->name_off_num] = p;
		node->name_off_num++;
	}
	node->name[p].start = 1;
	for (i = p; i < p+en_size ; ++i) {
		for (j = 0; j < 31 && k < size; ++j,++k) {
			node->name[i].name[j] = str[k];
		}
		node->name[i].size = j;
		node->name[i].dis = 0;
		node->name[i].ext = 1;
		if (j < 31) { break; }
	}
	node->name[i].ext = 0;
	node->namenum +=en_size;
	return TRUE;
}


int found_insert_pos(Node node,const char*fname,int length)
{
	int i;
	char* name;
	char cmp_small;
	if (discernNodeType(node)==LNODE_TYPE) {
		name = takeFileName(&(((_ln)node)->fi[0]));
		if (name == NULL)return -1;
		cmp_small = cmp_str(fname, length, name, fnb_p);
		if (cmp_small == 0)return ERR_SAME_FILE_NAME;
		if (cmp_small == -1) {
			return cmp_small;
		}
		for (i=0; i < ((_ln)node)->file_off_num; ++i) {
			name = takeFileName(&(((_ln)node)->fi[((_ln)node)->file_off[i]]));
			if (name == NULL)return ERR_NOT_TAKE_NAME;
			
			cmp_small = cmp_str(fname, length, name, fnb_p);
			if (cmp_small == 0)return ERR_SAME_FILE_NAME;
			if (cmp_small == -1) {
				return i;
			}
		}
	}
	else {
		name = takeEnName(&(((_bn)node)->name[0]));
		if (name == NULL)return -1;
		cmp_small = cmp_str(fname, length, name, fnb_p);
		if (cmp_small == 0)return ERR_SAME_FILE_NAME;
		if (cmp_small == -1) {
			return cmp_small;
		}
		for (i = 0; i < ((_bn)node)->name_off_num; ++i) {
			name = takeEnName(&(((_bn)node)->name[((_bn)node)->name_off[i]]));
			if (name == NULL)return ERR_NOT_TAKE_NAME;
			cmp_small = cmp_str(fname, length, name, fnb_p);
			if (cmp_small == 0)return ERR_SAME_FILE_NAME;
			if (cmp_small == -1)return i;
		}
	}
	return -2;
}


char* takeFileName(_fileitems fis)
{
	int i, j,t=0;
	fnb_p = 0;
	char noext = FALSE;
	
	if (!fis[0].ft.dis || fis[0].ft.fatt.isext)return NULL;
	for (j = 0; j < FTNAME_SIZE; ++j) {
		filenamebuf[fnb_p++] = fis[0].ft.name[j];
	}
	if (fis[0].ft.extnum > 0) {
		if (!fis[0].ft.fatt.extname) {
			for (i=1; i <= fis[0].ft.extnum; ++i) {
				for (j = 0; j < FTNAME_SIZE; ++j) {
					filenamebuf[fnb_p++] = fis[i].ft.name[j];
				}
			}
		}
		else {
			i = 1;
		}
		
		if ( fis[i - 1].ft.extnum > 0) {
			t = fis[i-1].ft.extnum + i;
			
			if (fis[i - 1].ft.extnum< 15)noext = TRUE;
			for (i; i < t; ++i) {
				for (j = 0; j < fis[i].en.size; ++j) {
					filenamebuf[fnb_p++] = fis[i].en.name[j];
				}
			}
			
			if (noext)goto exit_takename;
			while (fis[i - 1].en.ext && !fis[i].en.dis && i < FILE_DES_NUM) {
				for (j = 0; j < fis[i].en.size; ++j) {
					filenamebuf[fnb_p++] = fis[i].en.name[j];
				}
				i++;
			}
		}
	}
exit_takename:
	return filenamebuf;
}


char* takeEnName(_extname en)
{
	if (!en->start)return NULL;
	int i = 0, j = 0;
	fnb_p = 0;
	do {
		for (j = 0; j < en[i].size; ++j) {
			filenamebuf[fnb_p++] = en[i].name[j];
		}
	} while (en[i++].ext);
	return filenamebuf;
}


int cmp_str(const char* a,int asize,const char* b, int bsize)
{
	int i;
	for (i = 0; i < ((asize<bsize)?asize:bsize); ++i) {
		if (a[i] > b[i])return 1;
		if (a[i] < b[i])return -1;
	}
	if (asize > bsize)return 1;
	else if (asize < bsize)return -1;
	return 0;
}



int off_find(int p,uint16 off[],int left,int right)
{
	int mid;
	if (p == 0)return -1;
	while (left <= right) {
		mid = (left + right) / 2;
		if (p == off[mid])
			return mid;
		else if (p > off[mid])
			left = mid + 1;
		else right = mid - 1;
	}
	return ERR_NOT_FOUND_P;
}

tree_error assignTime(time_t num,_fdate fd)
{
	struct tm* t;
	t=localtime(&num);
	fd->year = t->tm_year+1900-BASE_YEAR;
	fd->month = t->tm_mon;
	fd->day = t->tm_mday;
	fd->h = t->tm_hour;
	fd->m = t->tm_min;
	fd->s = t->tm_sec;
	return 0;
}

int creatFileDes(_fileitems fis,
	const char fname[],
	uint64 fsize[],
	uint32 fpos[],
	uint32 length,
	char dpl,
	char hide,
	char del,
	char en_floder
){
	uint32 fnlen = strlen(fname);
	int i, j, k,temp;
	
	time_t ts = time(NULL);
	assignTime(ts, &(fis[0].ft.createdate));
	assignTime(ts, &(fis[0].ft.lastModifiedDate));
	assignTime(ts, &(fis[0].ft.lastVisitDate));
	if (DiskBOOT->Unit > 8){
		if (length > 8)length = 8;
		
		for (i = 0,temp=0; i < length; ++i,++temp) {
			fis[temp].ft.offset = (fsize[i]%BLOCK_SIZE)&0xfff;
			fis[temp].ft.position = fpos[i];
			fis[temp].ft.size = calBlocks(fsize[i],BLOCK_SIZE);
			++temp;
			fis[temp].ft.offset = ((fsize[i] % BLOCK_SIZE) >> 12)&0xfff;
			fis[temp].ft.position = 0;
			fis[temp].ft.size = 0;
		}
	}
	else {
		if (length > 16)length = 16;
		
		for (i = 0; i < length; ++i) {
			fis[i].ft.offset = (fsize[i] % BLOCK_SIZE) & 0xfff;
			fis[i].ft.position = fpos[i];
			fis[i].ft.size = calBlocks(fsize[i], BLOCK_SIZE);
		}
	}
	temp=((DiskBOOT->Unit > 8) ? 2 : 1)* length - 1;
	
	for (i = 0,k=0; i < temp +1; ++i) {
		for (j = 0; j < FTNAME_SIZE && k< fnlen; ++j) {
			fis[i].ft.name[j] = fname[k++];
		}
		for (; j < FTNAME_SIZE; ++j) {
			fis[i].ft.name[j] = '\0';
		}
		fis[i].ft.dis = DIS_FT;
		fis[i].ft.fatt.extname = FALSE;
		fis[i].ft.fatt.isext = TRUE;
		fis[i].ft.extnum = 0;
		fis[i].ft.folder = 0;
		fis[i].ft.fatt.en_folder = en_floder;
		fis[i].ft.fatt.hide = hide;
		fis[i].ft.fatt.del = del;
		fis[i].ft.fatt.dpl = dpl;
		fis[i].ft.fatt.data = 1;
	}
	fis[0].ft.fatt.isext = FALSE;
	fis[0].ft.extnum =temp ;
	int ret = temp+1;
	if (k < fnlen) {
		fis[i - 1].ft.fatt.extname = TRUE;
		temp = fis[0].ft.extnum;
		fis[temp].ft.fatt.extname = TRUE;
		for (i = temp +1; i < (uint32)LMAX_EN_NUM + temp+1&&k< fnlen; ++i) {
			for (j = 0; j < 31 && k< fnlen; ++j) {
				fis[i].en.name[j]=fname[k++];
			}
			fis[i].en.size = j;
			fis[i].en.ext = TRUE;
			fis[i].en.dis = DIS_EN;
			for (; j < 31; ++j) {
				fis[i].en.name[j] = '\n';
			}
		}
		fis[i-1].en.ext = FALSE;
		fis[temp].ft.extnum = i-temp-1;
		ret += fis[temp].ft.extnum;
	}
	else {
		fis[i - 1].ft.fatt.extname = FALSE;
	}
	return ret;
}

int findPtr(_bn bnode, uint32 diskptr)
{
	int i;
	for (i = 0; i < bnode->name_off_num + 2; ++i) {
		if (bnode->child[i] == diskptr)return i;
	}
	return -1;
}

int LNode_FindFileName(_ln node, const char* str, int length)
{
	int i;
	char* a;
	char cmp;
	a = takeFileName(node->fi);
	if (a == NULL)return -2;
	cmp = cmp_str(a, fnb_p, str, length);
	if (cmp == 0) return -1;
	for (i = 0; i < node->file_off_num; ++i) {
		a = takeFileName(&(node->fi[node->file_off[i]]));
		if (a == NULL)continue;
		cmp = cmp_str(a, fnb_p, str, length);
		if (cmp == 0)return i;
	}
	return ERR_NOT_FOUND_FILE_NAME;
}

int BNode_FindFileName(_bn node, const char* fname, size_t length)
{
	int i;
	char* name = takeEnName(node->name);
	if (name == NULL)return ERR_NOT_TAKE_NAME;
	if (cmp_str(name, fnb_p, fname, length)==0)return -1;
	for (i = 0; i < node->name_off_num; ++i) {
		name = takeEnName(&(node->name[node->name_off[i]]));
		if (name == NULL)return ERR_NOT_TAKE_NAME;
		if (cmp_str(name, fnb_p, fname, length) == 0)return i;
	}
	return -2;
}

tree_error findNode(uint32 disknode,Node node,uint32 nodetype,_fileitems fis,const char*fname,int length)
{
	if (disknode == (uint32)NULL)return ERR_NODE_NULL;
	if (node == (Node)NULL) {
		node = diskPtr_into_memPtr(disknode, nodetype);
		if (node == (Node)NULL)return ERR_NODE_NULL;
	}
	int temp, fi,i;
	do {
		if (nodetype == LNODE_TYPE) {
			temp = LNode_FindFileName((_ln)node, fname, length);
			if (temp < -1)return temp;
			fi = (temp == -1) ? 0 : (((_ln)node)->file_off[temp]);
			
			temp = (temp == -1) ?
				(((_ln)node)->file_off[0]) : ((temp == ((_ln)node)->file_off_num - 1) ?
				(((_ln)node)->finum - ((_ln)node)->file_off[temp]) : (((_ln)node)->file_off[temp + 1] - ((_ln)node)->file_off[temp])
					);
			if (fi == 0)return ERR_NOT_FOUND_FILE_NAME;
			for (i = 0; i < temp; ++i) {
				fis[i] = ((_ln)node)->fi[fi++];
			}
			return 0;
		}
		else {
			temp = BNodeSearchChild_i((_bn)node, fname, length);
			if (temp < 0)return temp;
			disknode = ((_bn)node)->child[temp];
			
			node = diskPtr_into_memPtr(disknode, nodetype);
			if (node == (Node)NULL)return ERR_NODE_NULL;
			if (discernNodeType(node) != nodetype) {
				nodetype = LNODE_TYPE;
				node = (Node)diskPtr_into_LNodePtr(disknode);
			}
		}
	} while (node);
	return ERR_NOT_FOUND_FILE_NAME;
}

int discernNodeType(Node node)
{
	if (((_bn)node)->name->dis == DIS_EN)return BNODE_TYPE;
	return LNODE_TYPE;
}

int BNodeSearchChild_i(_bn node,const char*str,size_t length)
{
	char* s = takeEnName(node->name);
	int cmp;
	if (s == NULL)return ERR_NOT_TAKE_NAME;
	cmp = cmp_str(s, fnb_p, str, length);
	if (cmp == 1) return 0;
	if (cmp == 0)return 1;
	int i;
	for (i = 0; i < node->name_off_num; ++i) {
		s = takeEnName(&(node->name[node->name_off[i]]));
		if (s == NULL)return ERR_NOT_TAKE_NAME;
		cmp = cmp_str(s, fnb_p, str, length);
		if (cmp == 1)return i + 1;
		if (cmp == 0)return i + 2;
	}
	return node->name_off_num + 1;
}

uint32 findStartLNode(Node node)
{
	while (node) {
		if (discernNodeType(node) == LNODE_TYPE) { return node; }
		else {
			node = ((_bn)node)->child[0];
		}
	}
	return (uint32)NULL;
}

tree_error FileWrite(const char* filename, char* data, uint64 datasize, char dpl, char hide, char floder)
{
	
	static uint32 pos[16];
	static uint64 fsize[16];
	int length = diskAutoAlloc(pos, fsize, 16, datasize);
	if (length < 0)return length;
	tree_error err;
	fileItems fisbuf[FILE_DES_NUM];
	err=creatFileDes(fisbuf, filename, fsize, pos, length, dpl, hide, _NO_DELETE, floder);
	if (err < 0)return err;
	err=insertNode(_file_tree_root.node, (Node)NULL, _file_tree_root.type, fisbuf, err, filename, strlen(filename));
	if (err < 0)return err;
	for (int i = 0; i < length; ++i) {
		writeData(pos[i]*BLOCK_SIZE, fsize[i], data);
		data += fsize[i];
	}
	return 0;
}

tree_error FileClear(const char* filename)
{
	tree_error err=deleteNode(_file_tree_root.node, (Node)NULL, _file_tree_root.type, filename, strlen(filename));
	if (err < 0)return err;
	return 0;
}

void printFileTable(_fileitems fis)
{
	int i,j,t=0;
	int fsize=0;
	char str[600];
	for (i = 0; i < 1 + ((fis->ft.fatt.extname > 0) ? 0 : fis->ft.extnum);i++) {
		printf("Data Addr[%d]:%d\n", i, fis[i].ft.position);
		printf("Data Size[%d]:%d\n", i, fsize+=((fis[i].ft.size-1)*BLOCK_SIZE+ fis[i].ft.offset));	
		for (j = 0; j < 6; ++j) {
			str[t++] = fis[i].ft.name[j];
		}
	}
	printf("File size:%d\n", fsize);
	printf("Create Date:%d/%d/%d %d:%d:%d:%d\n", fis[0].ft.createdate.year+BASE_YEAR, fis[0].ft.createdate.month+1, fis[0].ft.createdate.day, fis[0].ft.createdate.h, fis[0].ft.createdate.m, fis[0].ft.createdate.s,fis[0].ft.ms);
	printf("Last Modified Date:%d/%d/%d %d:%d:%d\n", fis[0].ft.lastModifiedDate.year + BASE_YEAR, fis[0].ft.lastModifiedDate.month+1, fis[0].ft.lastModifiedDate.day, fis[0].ft.lastModifiedDate.h, fis[0].ft.lastModifiedDate.m, fis[0].ft.lastModifiedDate.s);
	printf("Last Visit Date:%d/%d/%d %d:%d:%d\n", fis[0].ft.lastVisitDate.year + BASE_YEAR, fis[0].ft.lastVisitDate.month+1, fis[0].ft.lastVisitDate.day, fis[0].ft.lastVisitDate.h, fis[0].ft.lastVisitDate.m, fis[0].ft.lastVisitDate.s);
	if (fis[i-1].ft.fatt.extname) {
		for (i = 1; i <= fis->ft.extnum; ++i) {
			for (j = 0; j < fis[i].en.size; ++j) {
				str[t++] = fis[i].en.name[j];
			}
		}
	}
	str[t] = '\0';
	printf("File Name:%s", str);
}

_bootloder creatBootLoder(
	const char* OEM, 
	uint16 BytePerSec,
	uint8 Unit,
	uint16 ResvdSecCnt,
	uint16 RootEntCnt,
	uint8 Media,
	uint16 SecPerTrk,
	uint16 NumHeads,
	uint32 HiddSec,
	uint32 TotUnit,
	uint8 DrvNum,
	uint32 VolID,
	const char* VolLab,
	const char* FileSysType,
	uint16 MainVerNum,
	uint16 LastVerNum,
	uint16 BNodeSize,
	uint16 LNodeSize,			
	uint32 LogBlockAddr,		
	uint32 LogSize,				
	_dpt dpt
)
{
	_bootloder bl = (_bootloder)malloc(sizeof(BOOTLoder));
	if (bl == NULL) {
		bootloder512 = NULL;
		return bl;
	}
	memset(bl, 0, sizeof(BOOTLoder));
	int i;
	bl->jmpBOOT[0] = 80 - 3;
	bl->jmpBOOT[1] = 0xc0;
	bl->jmpBOOT[2] = 0x8e;
	for (i = 0; i < 8; ++i) {
		bl->OEM[i] = OEM[i];
	}
	bl->BytePerSec = BytePerSec;
	bl->Unit = Unit;
	bl->ResvdSecCnt = ResvdSecCnt;
	bl->Resvered0 = (uint32)NULL;
	bl->RootEntCnt = RootEntCnt;
	/*if (TotUnit < 0x10000) {
		bl->TotUnit16 = TotUnit;
		bl->TotUnit32 = NULL;
	}
	else {
		bl->TotUnit16 = NULL;
		bl->TotUnit32 = TotUnit;
	}*/
	bl->TotUnit32 = TotUnit;
	bl->Media = Media;
	bl->BlockSize = Unit * BytePerSec;
	bl->SecPerTrk = SecPerTrk;
	bl->NumHeads = NumHeads;
	bl->HiddSec = HiddSec;
	bl->DrvNum = DrvNum;
	bl->Resvered1 = (uint32)NULL;
	bl->BootSig = 0x29;
	bl->VolID = VolID;
	for (i = 0; i < 11; ++i) {
		bl->VolLab[i] = VolLab[i];
	}
	for (i = 0; i < 8; ++i) {
		bl->FileSysType[i] = FileSysType[i];
	}
	bl->VerNum[0] = MainVerNum;
	bl->VerNum[1] = LastVerNum;
	bl->BNodeSize = BNodeSize;
	bl->LNodeSize = LNodeSize;
	bl->LogBlockAddr = LogBlockAddr;
	bl->LBNum = LogSize;
	bl->InfoBlockAddr = LXINFOB_ADDR;
	for (i = 0; i < BOOTCodeSize; ++i) {
		bl->boot[i] = '\0';
	}
	
	bl->dpt[0] = dpt[0];
	bl->dpt[1] = dpt[1];
	bl->dpt[2] = dpt[2];
	bl->dpt[3] = dpt[3];
	bl->BOOTSign = 0xaa55;
	bootloder512 = bl;
	return bl;
}

int init_disk(const char* fname)
{
	
	strcpy(_disk_filename, fname);
	rdisk=fopen(fname, "rb+");
	if (rdisk == NULL)return -1;
	wdisk = rdisk;
	return 0;
}

void setDBMP1(_dabmp dmp, uint32 num, byte bit)
{
	uint32 i = num / 8;
	switch (num % 8) {
	case 0:dmp[i].bb.bit0 = bit; break;
	case 1:dmp[i].bb.bit1 = bit; break;
	case 2:dmp[i].bb.bit2 = bit; break;
	case 3:dmp[i].bb.bit3 = bit; break;
	case 4:dmp[i].bb.bit4 = bit; break;
	case 5:dmp[i].bb.bit5 = bit; break;
	case 6:dmp[i].bb.bit6 = bit; break;
	case 7:dmp[i].bb.bit7 = bit; break;
	}
	return;
}

void setDBMPs(_dabmp dmp, uint32 start, uint32 bitlength, byte bit)
{
	uint32 i, end = start + bitlength, temp = (bit > 0) ? 0xff : 0;
	if (bitlength < 8) {
		for (i = 0; i < bitlength; ++i) {
			setDBMP1(dmp, start++, bit);
		}
	}
	else {
		bitlength = 8 - ((start % 8 > 0) ? start % 8 : 8);
		for (i = 0; i < bitlength; ++i) {
			setDBMP1(dmp, start++, bit);
		}
		for (; start < end - end % 8; start += 8) {
			dmp[start / 8].ch = temp;
		}
		for (; start < end; ++start) {
			setDBMP1(dmp, start, bit);
		}
	}
}

int printBit(_dabmp dmp, uint32 length)
{
	int i, j = 0;
	for (i = 0; i < length / 8; i++) {
		
		printf("%d", (int)dmp[i].bb.bit0);
		printf("%d", (int)dmp[i].bb.bit1);
		printf("%d", (int)dmp[i].bb.bit2);
		printf("%d", (int)dmp[i].bb.bit3);
		printf("%d", (int)dmp[i].bb.bit4);
		printf("%d", (int)dmp[i].bb.bit5);
		printf("%d", (int)dmp[i].bb.bit6);
		printf("%d", (int)dmp[i].bb.bit7);
		if (i % 2 == 1)printf("\n");
		else printf("\t");
	}
	for (j = 0; j < length % 8; ++j) {
		switch (j) {
		case 0:printf("%d", (int)dmp[i].bb.bit0); break;
		case 1:printf("%d", (int)dmp[i].bb.bit1); break;
		case 2: printf("%d", (int)dmp[i].bb.bit2); break;
		case 3:	printf("%d", (int)dmp[i].bb.bit3); break;
		case 4:	printf("%d", (int)dmp[i].bb.bit4); break;
		case 5:	printf("%d", (int)dmp[i].bb.bit5); break;
		case 6:	printf("%d", (int)dmp[i].bb.bit6); break;
		case 7:	printf("%d\n", (int)dmp[i].bb.bit7); break;
		}
	}
	return 0;
}

int writeData(uint64 off, uint64 length, void* src)
{
	if (src == NULL)return ERR_NODE_NULL;
	
	unsigned int i, j, l = ((off + length) > DISK_SIZE_BYTES) ? DISK_SIZE_BYTES - off : length;
	
	fseek(wdisk, off, SEEK_SET);
	if (off > DISK_SIZE_BYTES)return ERR_DISKPOINTER_EXCEEDED;
	
	fwrite(src, l, 1, wdisk);
	return 0;
}

int init_format()
{
	
	format_unit = bootloder512->Unit;
	format_bytepersec = bootloder512->BytePerSec;
	format_block_size = format_unit * format_bytepersec;
	format_disk_n = bootloder512->TotUnit32;
	format_disk_size = format_disk_n * format_block_size;
	block_group_n = format_block_size * 8;
	block_group_size = block_group_n * format_block_size;																							  
	
	format_lxinfo = LXINFOB_ADDR;
	format_lxinfo_addr = LXINFOB_ADDR * format_block_size;
	format_boot = LXBOOOT_ADDR;
	format_boot_addr = LXBOOOT_ADDR * format_block_size;
	format_root = INIT_ROOT_ADDR;
	format_root_addr = INIT_ROOT_ADDR * format_block_size;
	
	lib = (_lx_info_block)malloc(format_block_size);
	if (lib == NULL)return -1;
	lib->LXInfoBackup = (format_disk_n - format_disk_n % block_group_n) - BACKUP_LXINFO;
	lib->ROOTNodeBackup = (format_disk_n - format_disk_n % block_group_n) - BACKUP_ROOT;
	lib->BOOTBackup = (format_disk_n - format_disk_n % block_group_n) - BACKUP_BOOT;
	lib->ROOTNodeAddr = format_root;
	lib->ROOTType = LX_INFO_ROOT_TYPE_LNODE;
	memcpy(lib->FileSystem, LX_INFO_STRING, sizeof(LX_INFO_STRING));
	memset((void*)((uint32)lib + sizeof(LX_INFO_BLOCK)), 0, format_block_size - sizeof(LX_INFO_BLOCK));
	
	format_backup_boot = lib->BOOTBackup;
	format_backup_boot_addr = lib->BOOTBackup * format_block_size;
	format_backup_lxinfo = lib->LXInfoBackup;
	format_backup_lxinfo_addr = lib->LXInfoBackup * format_block_size;
	format_backup_root = lib->ROOTNodeBackup;
	format_backup_root_addr = lib->ROOTNodeBackup * format_block_size;
	end_block_group_n = format_disk_n % block_group_n;
	end_block_group_size = end_block_group_n * format_block_size;
	BLOCK_SIZE = format_block_size;
	DISK_SIZE = format_disk_n;
	DISK_SIZE_BYTES = BLOCK_SIZE * DISK_SIZE;
	return 0;
}

int format_lx(const char* fname, int length)
{
	strcpy(_disk_filename, fname);
	wdisk=fopen(fname, "wb");
	memcpy(_disk_filename, fname, length);
	if (init_format() < 0)return -1;
	
	if (bootloder512 != NULL) {
		writeData(format_boot_addr, format_bytepersec, bootloder512);
	}
	else return -1;
	
	if (lib != NULL)writeData(format_lxinfo_addr, format_block_size, lib);
	else return -1;
	
	uint64 i, j = 0;
	DABMP* dmp = (DABMP*)malloc(format_block_size);
	if (dmp == NULL)return -1;
	memset(dmp, 0, format_block_size);
	setDBMP1(dmp, format_block_size * 8 - 1, 1);
	
	setDBMPs(dmp, 0, bootloder512->HiddSec, BLOCK_OCC);
	writeData(block_group_size - format_block_size, format_block_size, dmp);
	
	setDBMPs(dmp, format_root, sizeof(LeafNode) / format_block_size, 1);
	writeData(block_group_size - format_block_size, format_block_size, dmp);
	setDBMPs(dmp, 0, bootloder512->HiddSec, BLOCK_NOT_OCC);
	for (i = block_group_size * 2 - format_block_size; i < format_disk_size - block_group_size; i += block_group_size) {
		writeData(i, format_block_size, dmp);
	}
	
	setDBMPs(dmp, format_backup_boot % block_group_n, 1, 1);
	setDBMPs(dmp, format_backup_root % block_group_n, sizeof(LeafNode) / format_block_size, 1);
	setDBMPs(dmp, format_backup_lxinfo % block_group_n, 1, 1);
	writeData(i, format_block_size, dmp);
	writeData(format_backup_boot_addr, format_bytepersec, bootloder512);
	writeData(format_backup_lxinfo_addr, format_block_size, lib);

	
	if (end_block_group_n != 0) {
		
		setDBMPs(dmp, 0, end_block_group_n, 0);
		setDBMPs(dmp, end_block_group_n, block_group_n - end_block_group_n, 1);
		
		writeData((format_disk_n - 1) * format_block_size, format_block_size, dmp);
	}
	
	if (wdisk != NULL)fclose(wdisk);
	
	return 0;
}

uint32 disk_alloc(uint32 blocks)
{
	
	static uint32 start = 0;
	uint32 databmpsize = DataBMPNum * sizeDAB * 8;
	for (int a = 0; a < 3; ++a) {
		if (start) {
			int i, j = 0;
			for (i = start; i < databmpsize; ++i) {
				if (outBitType(DataBMP, i))continue;
				j = i++;
				while (!outBitType(DataBMP, i) && i - j < blocks)i++;
				if (i - j >= blocks)break;
			}
			if (i < databmpsize) {
				setDBMPs(DataBMP, j, blocks, 1);
				DataBMP_Stack_Push((j / 8) / sizeDAB);
				start = i;
				return j;
			}
			else {
				start = 0;
			}
		}
		else {
			while (DataBMP[start++].ch == 0xff);
			start = (start - 1) * 8;
		}
	}
	return 0;
}

int diskAutoAlloc(uint32 diskptr[], uint64 size[], int length, long datasize)
{
	uint32 i, temp = calBlocks(datasize, BLOCK_SIZE), off = datasize % BLOCK_SIZE;

	for (i = 0; i < length && datasize>0; ++i)
	{
		diskptr[i] = 0;
		while (temp) {
			diskptr[i] = disk_alloc(temp);
			if (diskptr[i] == 0) {
				temp /= 2;
			}
			else { break; }
		}
		if (diskptr[i] == 0) {
			return ERR_NOT_FOUND_SPACE_IN_DISK;
		}
		else {
			size[i] = temp * BLOCK_SIZE;
		}
		datasize -= size[i];
		temp = calBlocks(datasize, BLOCK_SIZE);
	}
	if (datasize > 0)return ERR_NOT_FOUND_SPACE_IN_DISK;
	
	size[i - 1] = size[i - 1] - BLOCK_SIZE + off;
	return i;
}

/*uint32 write_disk(char* src, uint32 length, uint32 blockAddr)
{
	
	uint32 i = 0, j, bits = DataBMPNum * sizeDAB * 8, blocks = length / BLOCK_SIZE + (length % BLOCK_SIZE > 0 ? 1 : 0);
	if (blockAddr == (uint32)NULL) {
		j = disk_alloc(blocks);
	}
	else {
		i = blockAddr;
		while (!outBitType(DataBMP, i) && i < bits && i - blockAddr < blocks)i++;
		if (i == blockAddr || i - blockAddr < blocks)return -1;
		j = blockAddr;
	}
	if (i < bits) {
		writeData(j * BLOCK_SIZE, length, src);
		
		setDBMPs(DataBMP, j, blocks, 1);
		writeData(((j / BLOCK_GROUP_SIZE + 1) * BLOCK_GROUP_SIZE - 1) * sizeDAB, sizeDAB, &(DataBMP[j / BLOCK_GROUP_SIZE]));
		return j;
	}
	return -1;
}*/

uint32 readData(char* des, uint32 length, uint32 blockAddr)
{
	unsigned int i;
	
	fseek(rdisk, blockAddr * BLOCK_SIZE, SEEK_SET);
	fread(des, length, 1, rdisk);
	_dabmp dmp = (_dabmp)des;
	return length;
}

uint32 closeLX()
{
	if (rdisk != NULL)fclose(rdisk);
	if (rdisk == wdisk) {
		rdisk = NULL; wdisk = NULL;
	}
	else {
		if (wdisk != NULL)fclose(wdisk);
	}

	if (DataBMP)free(DataBMP);
	if (bootloder512)free(bootloder512);
	if (DiskBOOT)free(DiskBOOT);
	if (filenamebuf)free(filenamebuf);
	return 0;
}

int creatRootFile()
{
	rdisk=fopen( _disk_filename, "rb+");
	if ( rdisk== (FILE*)NULL)return -1;
	wdisk = rdisk;
	if (init_lx() < 0)return -1;
	char firststr[] = "This is first file in LX_system.";
	size_t fssize = strlen(firststr);
	
	uint32 fpos[16];
	uint64 fsize[16];
	int length = diskAutoAlloc(fpos, fsize, 16, fssize);
	fileItems fisbuf[FILE_DES_NUM];
	int temp = creatFileDes(fisbuf, _ROOTFILENAME, fsize, fpos, length, 0, 0, 0, 0);
	if (temp < 0)return temp;
	
	_ln root = (_ln)malloc(sizeof(LeafNode));
	if (root == (_ln)NULL)return -1;
	memset(root, 0, sizeof(LeafNode));
	insertLNode(_ROOTFILENAME, sizeof(_ROOTFILENAME), fisbuf, temp, _file_tree_root.node, root);
	
	root->next = format_root;
	root->prev = format_root;
	writeData(format_root_addr, sizeof(LeafNode), root);
	writeData(fpos[0] * BLOCK_SIZE, fsize[0], firststr);
	return 0;
}

tree_error nodeOutDisk(uint32 disk_node, Node mem_node, uint32 nodetype)
{
	if (mem_node == (Node)NULL) {
		mem_node = radix_tree_find(radix_node_ptr, disk_node);
		if (mem_node == (Node)NULL)return ERR_NODE_NOT_IN_MEM;
	}
	int err = 0;
	if (nodetype == BNODE_TYPE) {
		err = writeData(disk_node * BLOCK_SIZE, sizeof(BTreeNode), (_bn)mem_node);
	}
	else if (nodetype == LNODE_TYPE) {
		err = writeData(disk_node * BLOCK_SIZE, sizeof(LeafNode), (_ln)mem_node);
	}
	else return ERR_NODE_TYPE_ERROR;
	return err;
}

void _all_BNode_Out_Disk(uint32 disknode,Node node)
{
	if (disknode == (uint32)NULL || node == (Node)NULL)return;
	uint32 temp = discernNodeType(node);
	if (temp == BNODE_TYPE) {
		writeData(disknode * BLOCK_SIZE, sizeof(BTreeNode), (_bn)node);
	}
	else if (temp == LNODE_TYPE) {
		writeData(disknode * BLOCK_SIZE, sizeof(LeafNode), (_ln)node);
	}
	else return;
}

tree_error allNodeOutDisk()
{
	radix_tree_traversal_fun(radix_node_ptr,_all_BNode_Out_Disk);
	if (_file_tree_root.type == BNODE_TYPE) {
		writeData(_lxinfoblock->ROOTNodeBackup * BLOCK_SIZE, sizeof(BTreeNode), diskPtr_into_BNodePtr(_file_tree_root.node));
		printf(">>write ROOT by BNODE: blockADDR:%d,disknode:%d\n",_lxinfoblock->ROOTNodeBackup,_file_tree_root.node);
	}
	else {
		
		writeData(_lxinfoblock->ROOTNodeBackup * BLOCK_SIZE, sizeof(LeafNode), diskPtr_into_LNodePtr(_file_tree_root.node));
		printf(">>write ROOT by LNODE: blockADDR:%d,disknode:%d\n",_lxinfoblock->ROOTNodeBackup,_file_tree_root.node);
	}
	if (_lxinfoblock->ROOTNodeAddr != _file_tree_root.node) {
		printf(">>write lx_info_block\n");
		_lxinfoblock->ROOTNodeAddr = _file_tree_root.node;
		if (_file_tree_root.type == BNODE_TYPE) {
			_lxinfoblock->ROOTType = LX_INFO_ROOT_TYPE_BNODE;
		}
		else {
			_lxinfoblock->ROOTType = LX_INFO_ROOT_TYPE_LNODE;
		}
		writeData(DiskBOOT->InfoBlockAddr * BLOCK_SIZE, sizeof(LX_INFO_BLOCK), _lxinfoblock);
	}
	return 0;
}

void bmpfile(const char* fname)
{
	FILE* fout;
	fout=fopen( fname, "wb");
	if (fout == (FILE*)NULL)return;
	fwrite(DataBMP, DataBMPNum * sizeDAB, 1, fout);
	fclose(fout);
}

tree_error saveNodePtr(Node node, uint32 diskptr)
{
	int err = radix_tree_insert(radix_node_ptr, diskptr, node);
	if (err < 0)return err;
	return err;
}

Node diskPtr_into_memPtr(uint32 diskptr, int nodetype)
{
	char* n = (char*)(Node)radix_tree_find(radix_node_ptr, diskptr);
	if (n != NULL)return (Node)n;
	int size;
	if (nodetype == BNODE_TYPE) {
		size = sizeof(BTreeNode);
	}
	else {
		size = sizeof(LeafNode);
	}
	n = (char*)malloc(size);
	if (n == NULL)return (Node)NULL;
	if (readData(n, size, diskptr) < 0) { free(n); return (Node)NULL; }
	saveNodePtr((Node)n, diskptr);
	return (Node)n;
}

_bn diskPtr_into_BNodePtr(uint32 diskptr)
{
	_bn node = (_bn)radix_tree_find(radix_node_ptr, diskptr);
	if (node != NULL)return node;
	node = (_bn)malloc(sizeof(BTreeNode));
	if (node == NULL)return NULL;
	if (readData((char*)node, sizeof(BTreeNode), diskptr) < 0) {
		free(node); return NULL;
	}
	saveNodePtr((Node)node, diskptr);
	return node;
}

_ln diskPtr_into_LNodePtr(uint32 diskptr)
{
	_ln node = (_ln)radix_tree_find(radix_node_ptr, diskptr);
	if (node != NULL)return node;
	node = (_ln)malloc(sizeof(LeafNode));
	if (node == NULL)return NULL;
	if (readData((char*)node, sizeof(LeafNode), diskptr) < 0) {
		free(node); return NULL;
	}
	saveNodePtr((Node)node, diskptr);
	return node;
}

/*void _print_radix_path_value(radix_node_t* node)
{
	static uint64 nodeaddr;
	static int i = 0;
	if (node == NULL)return;
	if (node->value != (uint32)NULL) {
		printf("memory:%x----disk:%x\n", node->value, nodeaddr);
	}
	nodeaddr = setValueBit(nodeaddr, i, 0); i++;
	_all_BNode_Out_Disk(node->child[0]);
	i--;
	nodeaddr = setValueBit(nodeaddr, i, 1); i++;
	_all_BNode_Out_Disk(node->child[1]);
	i--;
	nodeaddr = setValueBit(nodeaddr, i, 2); i++;
	_all_BNode_Out_Disk(node->child[2]);
	i--;
	nodeaddr = setValueBit(nodeaddr, i, 3); i++;
	_all_BNode_Out_Disk(node->child[3]);
	i--;
}
*/
void printRadixPathValue()
{
	radix_tree_traversal(radix_node_ptr);
}

int writeDataBMP()
{
	uint64 start = (BLOCK_GROUP_SIZE - 1) * BLOCK_SIZE;
	uint64 temp = BLOCK_GROUP_SIZE * BLOCK_SIZE;
	uint32 a;
	for (; DataBMP_Stack_sp > 0; DataBMP_Stack_sp--) {
		a = S_Pop(DataBMP_Stack);
		writeData(start + a * temp, sizeDAB, DataBMP + a * sizeDAB);
	}
	return DataBMP_Stack_sp;
}

tree_error DataBMP_Stack_Push(uint32 n)
{
	
	int err = S_FindStack((S_ElementType)n, DataBMP_Stack);
	if (err == ERR_NOT_FOUND_NUM_IN_STACK) {
		S_Push(DataBMP_Stack, (S_ElementType)n); DataBMP_Stack_sp++;
	}
	return 0;
}

int flushDiskCache()
{
	void printRadixPathValue();
	allNodeOutDisk();
	writeDataBMP();
	return 0;
}