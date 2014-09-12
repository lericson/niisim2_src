#ifndef _TYPES_H__
#define _TYPES_H__


typedef int INT;
typedef unsigned int UINT;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;

#ifndef WINNT

typedef unsigned int DWORD;
typedef int* INT_PTR;
typedef bool BOOL;
typedef long long int __int64;
typedef struct _POINT {
	UINT x, y;
} POINT;

#endif

#endif
