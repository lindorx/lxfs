#include"lxstruct.h"
#include"lxerror.h"
#include"lx.h"
#include<string.h>

void format_disk(const char* img);
int init(const char* fname);
void treetest();
int strbchr(char *src,int val);
int cfile_ls(const char *ls);
int cfile_insert(const char* sourcefile, const char* path);
//将字符串中的指定字符进行替换
//str:源字符串，chr：需要被替换的字符，val：替换字符
int strrechr(char *str, int chr, int val)
{
	int i = 0,n=0;
	while (str[i] != '\0') {
		if (str[i] == chr) {
			str[i] = val; n++ ;
		}
		i++;
	}
	return n;
}


/*参数说明：
img=***，指定镜像
if=***，将***文件存入镜像中
path=***,指定写入文件在镜像的路径
ls=***，打印指定目录的内容，只有ls默认为打印根目录文件
format，将镜像格式化
del=***,删除指定的目录或者文件，如果是目录，则要放上'/'
*/
int main(int argc,char *argv[])
{
	if(argc<2)return 0;//没有参数直接返回
	int i;
	char *img=NULL,*ifile=NULL,*ls=NULL,*path=NULL,*del=NULL;
	char isformat=FALSE;
	for(i=1;i<argc;++i){
		char* sss = argv[i];
		if(!strncmp(argv[i],"img=",4)){
			img=&(argv[i][4]);
		}
		else if(!strncmp(argv[i],"if=",3)){
			ifile=&(argv[i][3]);
		}
		else if(!strncmp(argv[i],"ls",2)){
			if(argv[i][2]=='='){//如果有‘=’，说明指定了路径
				ls=&(argv[i][3]);
			}
			else{//否则令第一个字符等于/，指定位根目录
				argv[i][0]='/';
				argv[i][1]='\0';
				ls=argv[i];
			}
		}
		else if (!strncmp(argv[i], "path=", 5)) {
			path = &(argv[i][5]);
		}
		else if(!strncmp(argv[i],"del=",4)){
			del = &(argv[i][4]);
		}
		else if(!strncmp(argv[i],"format",6)){
			//需要确认“format”字符均正确
			isformat=TRUE;
		}
	}
	if(img==NULL){//缺少镜像文件
		printf("error: Missing image file!\n");
		return 0;
	}
	if(init(img)>0){//初始化文件系统
		printf("error: not open image file:%s\n",img);
		return 0;
	}
	if(isformat){//格式化
		format_disk(img);
		printf("Image formatting completed.\n");
		return 0;
	}
	else{
		if(ls!=NULL){
			strrechr(ls, '\\', '/');
			cfile_ls(ls);
		}
		if(ifile!=NULL){
			strrechr(ifile, '\\', '/');
			cfile_insert(ifile, path);
			flushDiskCache();
		}
		if (del != NULL) {
			strrechr(del, '\\', '/');
			cfile_delete(del);
			flushDiskCache();
		}
	}
	closeLX();
	return 0;
}

//文件名处理函数
int ls_print(const char*str)
{
	return printf("%s\n", str);

}
int cfile_ls(const char *ls)
{
/*目录的形式为目录名+‘/’，例：test/
		从第一个叶节点链表开始向后查找，寻找符合目录的描述符，提取出来，然后跳过该描述符设定的子单元数量
		*/
		//获取第一个叶节点指针
	_ln lnode=findFirstLNode(),lna;
	uint32 diskaddr=_file_tree_lnode0;
	int err;
	if(ls[0]=='/'){//符合路径格式
		if (strlen(ls) < 2)//打印根目录
		{
			findRootDir(ls_print);
		}
		else {
			_ln node;
			err = findNode_i(++ls, strlen(ls), &node);
			if (err < -1)return err;
			fileListPrint(node, err,3);
		}
	}
	else {//指定了目录，首先在描述符中找到该目录
		printf("error: ls format 'ls' or 'ls=/***'!\n");
	}
	return 0;
}
//从后向前查找，找到最后一个指定的字符，返回该字符下标，没有则返回-1
int strbchr(char *str,int val)
{
	int i,p=-1;
	for(i=0;str[i]!='\0';++i){
		if(str[i]==val)p=i;
	}
	return p;
}

int cfile_insert(const char *sourcefile,const char *path)
{
	if (sourcefile == NULL&&path!=NULL) {
		return FileWrite(path, strlen(path), NULL, 0, _USER_DPL, _NO_HIDE, _FLODER);
	}
	FILE * fin;
	fin=fopen(sourcefile,"rb");
	if(fin==NULL){
		printf("error: not open input file:%s\n",sourcefile);
		return -1;
	}
	//读取文件名，去除路径
	int p=strbchr(sourcefile,'/');
	char *fname;
	if(p<0){fname=sourcefile;}
	else fname=&(sourcefile[p+1]);
	//将文件内容载入内存
	fseek(fin,0,SEEK_END);
	int fsize=ftell(fin);
	if(fsize>1024*1024*128){
		printf("error: The file size is over 128M!\n");
		return -1;
	}
	char *fdata=(char*)malloc(fsize);
	if (fdata == NULL)return -1;
	fseek(fin,0,SEEK_SET);
	fread(fdata,fsize,1,fin);
	fclose(fin);
	printf("%s size= %d\n",fname,fsize);
	//如果sourcefile为空，说明仅创建文件夹
	//tree_error err;
	//确定path是否指定文件名，如果最后一个字符不是‘/’,则由path指定文件名，否则使用源文件名
	char filename[FILE_NAME_MAX_LENGTH];
	char* f;
	if (path == NULL) {//没有指定路径，直接放到根目录
		f = strrchr(sourcefile, '/');
		if (f == NULL) {
			filename[0] = '/';
			strcpy(&(filename[1]), sourcefile);
		}
		else {
			strcpy(filename, f);
		}
	}
	else {
		size_t pathl = strlen(path);
		if (path[pathl - 1] == '/') {
			f = strrchr(sourcefile, '/');
			strcpy(filename, path);
			if (f == NULL) {
				strcpy(&(filename[pathl]), sourcefile);
			}
			else {
				f++;
				strcpy(&(filename[pathl]), f);
			}
		}
		else {
			return FileWrite(path, pathl, fdata, fsize, _USER_DPL, _NO_HIDE, _NOT_FLODER);

		}
	}
	return FileWrite(filename, strlen(filename), fdata, fsize, _USER_DPL, _NO_HIDE, _NOT_FLODER);
}

tree_error cfile_delete(const char* fname)
{
	return FileClear(fname);
}
/*
int cfile(const char *ls,const char* fname,int ctrl)
{
	//将文件路径和与文件名合并
	int lssize=strlen(ls),fnamesize=strlen(fname),err;
	if(lssize+fnamesize>=FILE_NAME_MAX_LENGTH){
		printf("error: File path name is too long!\n");//文件路径名过长
	}
	char dir[FILE_NAME_MAX_LENGTH]={0};
	strcpy(dir,ls);
	strcpy(&(dir[lssize]),fname);
	switch(ctrl){
	case CTRL_INSERT:{//写入文件
		//打开源文件
		FILE * fin;
		fin=fopen(fname,"rb");
		if(fin ==NULL){
			printf("error: not open input file:%s\n",fname);
			return -1;
		}
		//将文件内容载入内存
		fseek(fin,0,SEEK_END);
		int fsize=ftell(fin);
		if(fsize>1024*1024*128){
			printf("error: The file size is over 128M!\n");
			return -1;
		}
		char *fdata=(char*)malloc(fsize);
		fseek(fin,0,SEEK_SET);
		fread(fdata,fsize,1,fin);
		fclose(fin);
		printf("%s size= %d\n",fname,fsize);
		if(FileWrite(dir,fdata,fsize,0,0,0)<0)return -1;
	}break;
	case CTRL_SHOW_ROOT_DIR:{//显示目录
		int i=-1,num=0,off=0;
		char * dir;
		dir=takeFileName(lnode->fi);
		goto discern_dir;
		for(i;i<lnode->file_off_num;++i){
			//提取字符串
			off=lnode->file_off[i];
			dir=takeFileName(&(lnode->fi[off]));

	discern_dir:
			if(strchr(dir,'/')==NULL){//如果存在字符'/'，说明这是个目录描述符，根据子目录项进行跳转
				printf("%s\n",dir);
				continue;
			}
			printf("%s\n",dir);
			num=lnode->fi[off].ft.folder+i;
			while(num>lnode->file_off_num){//要跳转到下一个节点
				num-=lnode->file_off_num;
				//读取下一个节点
				if(lnode->next==_file_tree_lnode0)return 0;//已到末节点，返回
				lnode=diskPtr_into_LNodePtr(lnode->next);
			
			}
			i=num;
		}
	}break;
	}
	return 0;
}*/
/*
#define MAX_LENGTH 100
void treetest()
{
	//循环创建一系列文件名并写入文件树，然后再循环删除,如果它没有打印错误信息，说明工作正常
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
*/
void format_disk(const char * img)
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
	format_lx(img,strlen(img));
	creatRootFile();
	closeLX();
}

int init(const char * fname)
{
	int err;
	err = init_disk(fname);
	if(err<0)return err;
	init_lx();
	if (err=creat_tree(CREATE_TREE_DISK)<0)return 1;
	return 0;
}


