#include"stack.h"

static int stack_sp;//堆栈指针
static int stack_max_size;//堆栈最大容量

//创建堆栈，并返回堆栈指针
Stack S_CreateStack(int MaxSize)
{
	Stack s = (Stack)malloc(MaxSize * sizeof(S_ElementType));
	stack_sp = 0;
	stack_max_size = MaxSize;
	return s;
}

//判断堆栈是否已满
char S_IsFull(Stack s)
{
	if (stack_sp >= stack_max_size) { return 1; }
	return 0;
}

//压栈
char S_Push(Stack s, S_ElementType x)
{
	if (S_IsFull)return 0;
	s[stack_sp++] = x;
	return 1;
}

//判断堆栈是否为空
char S_IsEmpty(Stack s)
{
	if (stack_sp <= 0)return 1;
	return 0;
}

//出栈
S_ElementType S_Pop(Stack s) 
{
	if (S_IsEmpty)return 0;
	return s[stack_sp--];
}

//关闭堆栈
void S_CloseStack(Stack s)
{
	free(s);
	stack_sp = 0;
	stack_max_size = 0;
}

//堆栈遍历
int S_FindStack(S_ElementType n,Stack s)
{
	for (int i = 0; i < stack_sp; ++i) {
		if (s[i] == n)return i;
	}
	return ERR_NOT_FOUND_NUM_IN_STACK;
}