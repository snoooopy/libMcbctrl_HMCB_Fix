#ifndef TYPE_DEF_H
#define TYPE_DEF_H

#include "MCB_param.h"

typedef unsigned int   INT32U;
typedef signed int     INT32S;
typedef unsigned short INT16U;
typedef signed short   INT16S;
typedef unsigned char  INT8U;
typedef signed char    INT8S;
typedef unsigned int INT64U;

typedef struct tagOneDBLineOnePackage
{
INT16U nMCB;
INT8U ADFormat_Bit;//AD转换位数
//UINT nRowNum[MAX_MCBNUM];//每行/列笔数
//UINT nColNum[MAX_MCBNUM];//行列数
//UINT nDMNum[MAX_MCBNUM];//每个DBLine带的有效模块数

long nSocketCounter;//软件包裹计数;
INT8U overall_err;	//总错误
//
INT8U DB_line_i;//DB_line索引(0~3)
INT8U DB_line_num;//有效db_line(1,2,4)
INT8U cmd_code;//是否 Check the package type
INT8U cmd_mismatched_pkg_skip_en;//1 = 如果SC包头的cmd不符，会跳过本数据包检查下一个数据包，直到发现cmd匹配的数据包，并完成数据包解析
//
INT32U SC_pkg_header;  //定位DWord （0x00FF55AA）
INT8U  rx_DB_line_i;	//当前所接收到数据的DB_line索引(0~3)
INT8U  SC_pkg_type;	//
//INT16U SC_pkg_index;
INT32U SC_pkg_index;
//
INT8U  DB_pkg_type[MAX_MCBADNUM];
INT16U DB_pkg_index[MAX_MCBADNUM];
INT8U  DB_board_index[MAX_MCBADNUM];
INT8U  DB_logic_err1[MAX_MCBADNUM];
INT8U  DB_logic_err2[MAX_MCBADNUM];
INT16U DB_data_len[MAX_MCBADNUM];
INT8U  DB_data[MAX_MCBADNUM][MAX_DMROWNUM*MAX_DMCOLNUM*4];//每一个通道最多用4字节表示
INT8U  DB_crc_code[MAX_MCBADNUM];
//
INT8U  Max_DB_board;
//
INT8U  CB_pkg_type;
INT16U CB_pkg_index;
INT8U  CB_board_index;
INT8U  CB_logic_err1;
INT8U  CB_logic_err2;
INT16U CB_data_len;
INT8U  CB_data[16];
INT8U  CB_crc_code;
}OneDBLineOnePackage;


typedef struct tagOneDBLineOnePackageStatue
{
INT16U nMCB;
INT8U ADFormat_Bit;//AD转换位数
//UINT nRowNum[MAX_MCBNUM];//每行/列笔数
//UINT nColNum[MAX_MCBNUM];//行列数
//UINT nDMNum[MAX_MCBNUM];//每个DBLine带的有效模块数

long nSocketCounter;//软件包裹计数;
INT8U overall_err;	//总错误
//
INT8U DB_line_i;//DB_line索引(0~3)
INT8U DB_line_num;//有效db_line(1,2,4)
INT8U cmd_code;//是否 Check the package type
INT8U cmd_mismatched_pkg_skip_en;//1 = 如果SC包头的cmd不符，会跳过本数据包检查下一个数据包，直到发现cmd匹配的数据包，并完成数据包解析
//
INT32U SC_pkg_header;  //定位DWord （0x00FF55AA）
INT8U  rx_DB_line_i;	//当前所接收到数据的DB_line索引(0~3)
INT8U  SC_pkg_type;	//
//INT16U SC_pkg_index;
INT32U SC_pkg_index;
//
INT8U  DB_pkg_type[MAX_MCBADNUM];
INT16U DB_pkg_index[MAX_MCBADNUM];
INT8U  DB_board_index[MAX_MCBADNUM];
INT8U  DB_logic_err1[MAX_MCBADNUM];
INT8U  DB_logic_err2[MAX_MCBADNUM];
INT16U DB_data_len[MAX_MCBADNUM];
INT8U  DB_crc_code[MAX_MCBADNUM];
//
INT8U  Max_DB_board;
//
INT8U  CB_pkg_type;
INT16U CB_pkg_index;
INT8U  CB_board_index;
INT8U  CB_logic_err1;
INT8U  CB_logic_err2;
INT16U CB_data_len;
INT8U  CB_data[16];
INT8U  CB_crc_code;
}OneDBLineOnePackageStatue;

// ============================================================
// 线程安全 32 位原子访问封装
// （移植自 V2.0.1.1 type_def.h 的 Interlocked 封装，Linux 用 GCC __atomic 内建）
// ============================================================
typedef long MCB_LONG;
inline MCB_LONG AtomicLoadL(volatile MCB_LONG* p)
{
    return __atomic_load_n(p, __ATOMIC_SEQ_CST);
}
inline void AtomicStoreL(volatile MCB_LONG* p, MCB_LONG v)
{
    __atomic_store_n(p, v, __ATOMIC_SEQ_CST);
}
inline MCB_LONG AtomicOrL(volatile MCB_LONG* p, MCB_LONG mask)
{
    return __atomic_or_fetch(p, mask, __ATOMIC_SEQ_CST);
}
inline MCB_LONG AtomicAndL(volatile MCB_LONG* p, MCB_LONG mask)
{
    return __atomic_and_fetch(p, mask, __ATOMIC_SEQ_CST);
}
inline bool AtomicTestBitL(volatile MCB_LONG* p, MCB_LONG mask)
{
    return (AtomicLoadL(p) & mask) != 0;
}

#endif
