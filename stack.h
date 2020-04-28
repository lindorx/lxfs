#pragma once
#include<stdlib.h>
#define ERR_NOT_FOUND_NUM_IN_STACK -50		//没有在堆栈中找到指定值
typedef unsigned int S_ElementType;//堆栈值的类型
typedef S_ElementType* Stack;
//堆栈函数
//创建堆栈，并返回堆栈指针
Stack S_CreateStack(int MaxSize);
//判断堆栈是否已满
char S_IsFull(Stack s);
//压栈
char S_Push(Stack s, S_ElementType x);
//判断堆栈是否为空
char S_IsEmpty(Stack s);
//出栈
S_ElementType S_Pop(Stack s);
//关闭堆栈
void S_CloseStack(Stack s);
//堆栈遍历
int S_FindStack(S_ElementType n, Stack s);