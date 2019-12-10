#include"lxstruct.h"
#include"lxerror.h"
#include"lx.h"

void format_disk();
int init(const char* fname);
void treetest();
int main()
{
	format_disk();//格式化函数，自动创建一个格式化的镜像文件
	treetest();
	return 0;
}

#define MAX_LENGTH 100
void treetest()
{
	//循环创建一系列文件名并写入文件树，然后再循环删除,如果控制它没有打印错误信息，说明工作正常
	init("test.bin");
	_ln lnode;
	_bn bnode;
	lnode = diskPtr_into_LNodePtr(_file_tree_root.node);
	char i;
	char text[] = "filename:test1.txt;filesystem:lx;--by lindorx";
	char *fname =(char*)malloc(MAX_LENGTH+1);
	if (fname == NULL)return;
	memset(fname, 0x20, MAX_LENGTH);
	fname[MAX_LENGTH]='\0';
	int err;
	for (int a = 0; a < MAX_LENGTH; ++a) {
		for (i = 'a'; i <= 'z'; ++i) {
			fname[a] = i;
			err=FileWrite(fname, text, sizeof(text), _ADMIN_DPL, _NO_HIDE, _NOT_FLODER);
			if(err<0){printf("error=%d , write1: %s\n",err,fname);}
		}
		for (i = 'A'; i <= 'Z'; ++i) {
			fname[a] = i;
			err=FileWrite(fname, text, sizeof(text), _ADMIN_DPL, _NO_HIDE, _NOT_FLODER);
			if(err<0){printf("error=%d , write2: %s\n",err,fname);}
		}
		for (i = '0'; i <= '9'; ++i) {
			fname[a] = i;
			err=FileWrite(fname, text, sizeof(text), _ADMIN_DPL, _NO_HIDE, _NOT_FLODER);
			if(err<0){printf("error=%d , write3: %s\n",err,fname);}
		}
	}
	flushDiskCache();
	closeLX();
	return;
	memset(fname, 0x20, MAX_LENGTH); 
	fname[MAX_LENGTH]='\0';
	for (int a = 0; a < MAX_LENGTH; ++a) {
		for (i = 'a'; i <= 'z'; ++i) {
			fname[a] = i;
			err = FileClear(fname);
			if(err<0){printf("error=%d , delete1: %s\n",err,fname);}
		}
		for (i = 'A'; i <= 'Z'; ++i) {
			fname[a] = i;
			err = FileClear(fname);
			if(err<0){printf("error=%d , delete2: %s\n",err,fname);}
		}
		for (i = '0'; i <= '9'; ++i) {
			fname[a] = i;
			err = FileClear(fname);
			if(err<0){printf("error=%d , delete3: %s\n",err,fname);}
		}
	}
	closeLX();
}

void format_disk()
{
	DPT d[4] = { 0 };
	_bootloder bl = creatBootLoder(
		"LINDORX ", HD_SEC, HD_PAGE / HD_SEC,
		32, 1000, 0x80,
		8, 8, 32,
		HD_SIZE / HD_PAGE, 0, 0x12345678,
		"by-lindorx0", "systemlx", 0,
		1, BNODE_SIZE / HD_PAGE, LNODE_SIZE / HD_PAGE,
		2, 1, d);
	format_lx("test.bin",12);
	creatRootFile();
	closeLX();
}

int init(const char * fname)
{
	init_disk(fname);
	init_lx();
	if (creat_tree(CREATE_TREE_DISK) != (uint32)NULL)return 1;
	return 0;
}


