#ifndef ETH_FUNC_H
#define ETH_FUNC_H

#include "type_def.h"

//-----------------------------------
//Ethernet base function
//-----------------------------------
INT32S ETH_init(ULONG S_addr, unsigned short nMCB);//UCHAR s_b1, UCHAR s_b2,UCHAR s_b3,UCHAR s_b44

void ETH_close(unsigned short nMCB);

void ETH_CTRL_close(unsigned short nMCB);
bool ETH_CTRL_IsValid(unsigned short nMCB);

INT8U  ETH_CTRL_write(INT32U addr,INT8U * pdata, INT8U  len, unsigned short nMCB);

INT8U  ETH_CTRL_read(INT32U addr,INT8U * pdata, INT8U  len, unsigned short nMCB);

INT32S ETH_Data_Read( INT8U * buf, INT32S len, INT32U timeout, FILE* fp=NULL);

INT32S ETH_WSAStartup();
void ETH_WSACleanup();

bool checksocket_status(unsigned short nMCB);

//
//持续从_ETH_Data_socket数据端口读取数据，直到清空缓存中的数据。
INT32S ETH_DATA_read_buffer_residual_data(unsigned short nMCB);

//持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
INT32S ETH_CTRL_read_buffer_residual_data(unsigned short nMCB);

#endif

