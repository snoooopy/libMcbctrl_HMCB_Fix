#ifndef MEM_MAP_H
#define MEM_MAP_H


#include "type_def.h"

//EDAQ FPGA使用16M byte (24 bit)寻址空间。空间分配如下：
//0x000000-0x00FFFF: Register (Register size is 32bit, Maximum register number is 16K)
//0x010000-0x01FFFF: RAM4K (The maximum size of the RAM is 1K 32-bit words, that is 4K bytes)(The maximum number of the RAM4K is 16)

#define EDAQ_REG_BADDR                   0x000000
#define EDAQ_RAM4K_BADDR                 0x010000

//Number
#define EDAQ_REG_NUM                     42
  
//Register
//Reg 0
#define EDAQ_AUTO_RESET_REG_ADDR              (EDAQ_REG_BADDR + 0*4)

//Read
typedef union {
  INT32U All;
  struct {
	    INT32U build_num	:8;	
		INT32U revision		:8;
		INT32U minor_ver	:8;
		INT32U major_ver	:8;	
	  } Sepe;  
} EDAQ_FIRMWARE_VERSION_REG;

//Write
typedef union {
  INT32U All;
  struct {
		INT32U CB_logic_reset		:1;
		INT32U RBxB_reset			:1;
		INT32U RBOB_reset			:1;
		INT32U RSV1					:1;
		
		INT32U DB_reset_req			:1;
		INT32U DB_locate_req		:1;
		INT32U DB_prm_cfg_req		:1;
		INT32U DB_daq_cmd_tx_req	:1;

		INT32U RSV2   				:4;

		INT32U cfg_fpga_req			:1;
		INT32U RSV3     			:3;

		INT32U EEPROM_ECS_wr_req	:1;	
		INT32U EEPROM_ECS_rd_req	:1;
		INT32U EEPROM_key_data_onboard_rdreq	:1;	//该信号为1，MCB FPGA会读取EEPROM中的关键数据，更新以太网参数，检查加密数据是否匹配。注意，读取操作完全由下位机完成，读取数据不返回到上位机。

		INT32U RSV   				:13;

  } Sepe;  
} EDAQ_AUTO_RESET_REG;

//Reg 1
#define EDAQ_GENERAL_CTRL_REG_ADDR            (EDAQ_REG_BADDR + 1*4)
typedef union {
  INT32U All;
  struct {
		INT32U DB_fpga_cfg_enable :1;
		INT32U RB_working_mode : 1;
		INT32U sc_pkg_monitor_en : 1;
		INT32U TEST_DATA_enable : 1;

		INT32U DB_FPGA_PROG_enable_x4 : 4;

		INT32U RSV   		:24;		
	  } Sepe;  
} EDAQ_GENERAL_CTRL_REG;

//Reg 2
#define EDAQ_GENERAL_STATUS_REG_ADDR          (EDAQ_REG_BADDR + 2*4)
typedef union {
  INT32U All;
  struct {
		INT32U RB_almost_full       				:1;
		INT32U sc_pkg_monitor_err					:1;		
		INT32U RSV   								:6;
		INT32U RSV2   				:24;
	  } Sepe;  
} EDAQ_GENERAL_STATUS_REG;


//Reg 3
#define EDAQ_ECS_TEST_REG_ADDR                (EDAQ_REG_BADDR + 3*4)

//Reg 4
#define EEPROM_CTRL_REG_ADDR            (EDAQ_REG_BADDR + 4*4)
typedef union {
  INT32U All;
  struct {
		INT32U EEPROM_ECS_wr_data			:16;
		INT32U EEPROM_ECS_rw_addr			:8;
		INT32U EEPROM_lcl_rd_addr   			:8;		
	  } Sepe;  
} EEPROM_CTRL_REG;

//Reg 5
#define EEPROM_RDDATA_REG_ADDR              (EDAQ_REG_BADDR + 5*4)
typedef union {
  INT32U All;
  struct {
		INT32U EEPROM_ECS_rd_data			:16;
		INT32U EEPROM_rd_cs_byte				:8;
		INT32U OW_bus_idle_err   			:1;	
		INT32U OW_slave_present   			:1;	
		INT32U EEPROM_ECS_rw_finished   		:1;	
		INT32U EEPROM_lcl_rd_finished   		:1;	
		INT32U EEPROM_ID_matched   	:1;	
		INT32U RSV   		:3;
	  } Sepe;  
} EEPROM_RDDATA_REG;

//Reg 6
#define EDAQ_DB_COMM_CTRL_REG_ADDR            (EDAQ_REG_BADDR + 6*4)
typedef union {
  INT32U All;
  struct {
		INT32U DB_line_num						:3;
		INT32U RSV1   		:5;		
		INT32U DAQ_mode_enable					:1;
		INT32U HMCB_work_mode					:1;
		INT32U DB_DAQ_req_sel                   :2;
		INT32U RSV   		:20;		
	  } Sepe;  
} EDAQ_DB_COMM_CTRL_REG;

#define  EDAQ_DAQ_TRIG_SEL_BEAM_PULSE		1
#define  EDAQ_DAQ_TRIG_SEL_PERIOD_TRIG		2
#define  EDAQ_DAQ_TRIG_SEL_CASCADE_TRIG		3
#define  EDAQ_DAQ_TRIG_SEL_NONE				0

//Reg 7
#define DB_COMM_STATUS_REG_ADDR                 (EDAQ_REG_BADDR + 7*4)

typedef union {
  INT32U All;
  struct {
		INT32U DB_FPGA_cfg_finished			:1;
		INT32U DB_comm_cmd_process_busy		:1;		
		INT32U RSV   						:2;

		INT32U DB_Line_enable_num			:3;
		INT32U RSV1   						:1;

		INT32U DB_Line_enable_x4			:4;
		INT32U DB_CLK_FREQ					:4;

		INT32U DB_Line_max_num				:3;
		INT32U RSV2   						:13;
	  } Sepe;  
} DB_COMM_STATUS_REG;

//Reg 8
#define DB_NUM_SET_REG_ADDR					(EDAQ_REG_BADDR + 8*4)
typedef union {
  INT32U All;
  struct {
		INT32U DL0_DB_num				:8;
		INT32U DL1_DB_num				:8;
		INT32U DL2_DB_num				:8;
		INT32U DL3_DB_num				:8;	
	  } Sepe;  
} DB_NUM_SET_REG;

//Reg 9
#define DB_NUM_STATUS_REG_ADDR                 (EDAQ_REG_BADDR + 9*4)
typedef union {
  INT32U All;
  struct {
		INT32U DL0_DB_num				:8;
		INT32U DL1_DB_num				:8;
		INT32U DL2_DB_num				:8;
		INT32U DL3_DB_num				:8;	
	  } Sepe;  
} DB_NUM_STATUS_REG;

//Reg 10
#define EDAQ_DB_DAQ_CMD_EXCT_TIME_SET_REG_ADDR                (EDAQ_REG_BADDR + 10*4)

//Reg 11
#define EDAQ_DB_NON_DAQ_CMD_EXCT_TIME_SET_REG_ADDR                (EDAQ_REG_BADDR + 11*4)

//Reg 12
#define EDAQ_DB_INTEGRATION_TIME_SET_REG_ADDR                (EDAQ_REG_BADDR + 12*4)

//Reg 13
#define INT_TRIG_PERIOD_SET_REG_ADDR                 (EDAQ_REG_BADDR + 13*4)

//Reg 14
#define INT_TRIG_PULSE_WIDTH_SET_REG_ADDR              (EDAQ_REG_BADDR + 14*4)


//Reg 15
#define EDAQ_RB_ACCESS_CTRL_REG_ADDR            (EDAQ_REG_BADDR + 15*4)
typedef union {
  INT32U All;
  struct {
		INT32U RB_wr_port_sel				:3;
		INT32U rsv1							:1;
		INT32U RB_rd_port_sel				:3;
		INT32U rsv2							:1;
		INT32U RSV3   						:24;		
	  } Sepe;  
} EDAQ_RB_ACCESS_CTRL_REG;

//RB_wr_port_sel:
//0: No data (Default,RB_WR_SEL_NO_DATA) 
//1: ECS received FPGA config data port (From ECS, RB_WR_SEL_ECS_FPGA_CFG_DATA)
//2: DB received data port (From DB link, RB_WR_SEL_DB_RX_DATA)
//3: Internal gen test data (From internal, RB_WR_SEL_INT_GEN_DATA)
//4: ETH received FPGA config data port (From ETH, not implemented. RB_WR_SEL_ETH_FPGA_CFG_DATA)
//5-7: Rev
#define  EDAQ_RB_WR_SEL_NO_DATA				0
#define  EDAQ_RB_WR_SEL_ECS_FPGA_CFG_DATA	1
#define  EDAQ_RB_WR_SEL_DB_RX_DATA			2
#define  EDAQ_RB_WR_SEL_INT_GEN_DATA		3
#define  EDAQ_RB_WR_SEL_ETH_FPGA_CFG_DATA	4

//RB_rd_port_sel:
//0: No data (Default, RB_RD_SEL_NO_DATA)
//1: FPGA config data port (To FPGA configuration module, RB_RD_SEL_FPGA_CFG)
//2: ETH TX port (To SiTcp, RB_RD_SEL_ETH)
//3-7: rev
#define  EDAQ_RB_RD_SEL_NO_DATA			0
#define  EDAQ_RB_RD_SEL_FPGA_CFG_DATA	1
#define  EDAQ_RB_RD_SEL_ETH_DATA		2



//Reg 16
#define EDAQ_DB_FPGA_CFG_DATA_WR_SET_REG_ADDR                (EDAQ_REG_BADDR + 16*4)


//Reg 17
#define DDI_LATCH_LATENCY_SET_REG_ADDR              (EDAQ_REG_BADDR + 17*4)




//Reg 18
#define CB_FIFO_ERR_LOG_REG_ADDR					(EDAQ_REG_BADDR + 18*4)

//Reg 19
#define CB_ERR_LOG_REG1_ADDR                (EDAQ_REG_BADDR + 19*4)

//Reg 20
#define CB_ERR_LOG_REG2_ADDR                (EDAQ_REG_BADDR + 20*4)

//Reg 21
#define DB_FPGA_CFG_DATA_BYTE_NUM_REG_ADDR      (EDAQ_REG_BADDR + 21*4)

//Reg 22
#define SC_PKG_MONITOR_ST_REG_ADDR                (EDAQ_REG_BADDR + 22*4)

//Reg 23 rev

//Reg 24-31: REV_CTRL_REG1-REV_CTRL_REG8

//Reg 32-39: REV_STATUS_REG1-REV_STATUS_REG8

//Reg 24 DA data (REV_CTRL_REG1)
#define CB_DAC_UPDATEDATA_REG_ADDR			(EDAQ_REG_BADDR + 24*4)

//Reg 32-39: REV_STATUS_REG1-REV_STATUS_REG8
#define CB_PERI_CIRCUITS_VERSION_REG_ADDR      (EDAQ_REG_BADDR + 32*4)

//Reg 40
#define ETH_FIRMWARE_VERSION_REG_ADDR       (EDAQ_REG_BADDR + 40*4)

//Reg 41
#define ETH_STATUS_REG1_ADDR                (EDAQ_REG_BADDR + 41*4)


//RAM
//DB parameter config RAM
#define DB_LINE0_PRM_CFG_RAM_BADDR    (EDAQ_RAM4K_BADDR+0*4096)
#define DB_LINE1_PRM_CFG_RAM_BADDR    (EDAQ_RAM4K_BADDR+1*4096)
#define DB_LINE2_PRM_CFG_RAM_BADDR    (EDAQ_RAM4K_BADDR+2*4096)
#define DB_LINE3_PRM_CFG_RAM_BADDR    (EDAQ_RAM4K_BADDR+3*4096)


#endif



