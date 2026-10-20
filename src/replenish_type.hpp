#ifndef REPLENISH_TYPE_H
#define REPLENISH_TYPE_H

typedef char                CHAR;
typedef signed char         INT8;
typedef unsigned char       UCHAR;
typedef unsigned char       UINT8;
typedef unsigned char       BYTE;
typedef short               SHORT;
typedef signed short        INT16;
typedef unsigned short      USHORT;
typedef unsigned short      UINT16;
typedef unsigned short      WORD;
typedef int                 INT;
typedef signed int          INT32;
typedef unsigned int        UINT;
typedef unsigned int        UINT32;
typedef int                 LONG; // windows 下的long 和 linux 下的 int 字节数相同，而 linux 下的 long 是win的2倍 ， 故linux 下采用 int
typedef unsigned long       ULONG;
typedef unsigned long       DWORD;
typedef long long           LONGLONG;
typedef unsigned long long  ULONGLONG;
typedef int                 BOOL;   // V2.0.1.1 BOOL on Linux
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif



#endif // REPLENISH_TYPE_H
