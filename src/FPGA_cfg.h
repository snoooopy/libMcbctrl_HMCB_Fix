#ifndef FPGA_CFG_H
#define FPGA_CFG_H

#include "type_def.h"

//-----------------------------------
//ECS writing FPGA config data test function
//-----------------------------------
void EDAQ_ECS_wr_fpga_cfg_data_test(unsigned short nMCB);

//功能：Read data from DB FPGA program file and then write the data into EDAQ RB via ECS
//参数：filename指定配置文件。
//返回值：FPGA配置文件的长度，单位为字节。如果返回值为-1，说明文件不存在。
INT32S EDAQ_read_and_write_DB_fpga_program_file_into_RB_via_ECS(char *filename, unsigned short nMCB);

//-------------------------------------------------------------
// Read DB FPGA program data from EDAQ RB via ETH and dump into file
//------------------------------------------------------------
void EDAQ_read_DB_fpga_program_data_from_RB_via_ETH_and_dump_into_file(unsigned short nMCB);

//-------------------------------------------------------------
// Request the EDAQ fpga to program DB fpga.
//------------------------------------------------------------
void EDAQ_DB_fpga_config_request(unsigned short nMCB);

#endif

