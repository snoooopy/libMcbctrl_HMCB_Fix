#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include "stdafx.h"
#else
#include <unistd.h>
#include <thread>
#endif

#include "including.h"

#define gCmdReptN	(1)

//
EDAQ_DB_COMM_CTRL_REG gCtrl_Reg_WriteV[MAX_MCBNUM];
//

char EDAQ_reg_name_list[EDAQ_REG_NUM ][100]=
{
	"EDAQ_FIRMWARE_VERSION_REG",//Reg 0
	"EDAQ_GENERAL_CTRL_REG",	//1
	"EDAQ_GENERAL_STATUS_REG",  //2
	"EDAQ_ECS_TEST_REG",		//3
	"EDAQ_EEPROM_CTRL_REG",      //4
	"EDAQ_EEPROM_RDDATA_REG",    //5
	"EDAQ_DB_COMM_CTRL_REG",    //6
	"EDAQ_DB_COMM_STATUS_REG",  //7
	"EDAQ_DB_NUM_SET_REG",		//8
	"EDAQ_DB_NUM_STATUS_REG",	//9

	"EDAQ_DB_DAQ_CMD_EXCT_TIME_SET_REG",	//10
	"EDAQ_DB_NON_DAQ_CMD_EXCT_TIME_SET_REG",//11
	"EDAQ_DB_INTEGRATION_TIME_SET_REG",		//12
	"EDAQ_INT_TRIG_PERIOD_SET_REG",			//13
	"EDAQ_INT_TRIG_PULSE_WIDTH_SET_REG",	//14

	"EDAQ_RB_ACCESS_CTRL_REG",			//15
	"EDAQ_DB_FPGA_CFG_DATA_WR_REG",		//16	
	"EDAQ_DDI_LATCH_LATENCY_SET_REG",	//17
	
	"CB_FIFO_ERR_LOG_REG",	//18
	"CB_ERR_LOG_REG1",		//19
	"CB_ERR_LOG_REG2",		//20
	"DB_FPGA_CFG_DATA_BYTE_NUM_REG",	//21

	"SC_PKG_MONITOR_ST_REG",	//22
	"REV",	//23

	"REV_CTRL_REG1",	//24
	"REV_CTRL_REG2",	//25
	"REV_CTRL_REG3",	//26
	"REV_CTRL_REG4",	//27
	"REV_CTRL_REG5",	//28
	"REV_CTRL_REG6",	//29
	"REV_CTRL_REG7",	//30
	"REV_CTRL_REG8",	//31

	"REV_STATUS_REG1",	//32
	"REV_STATUS_REG2",	//
	"REV_STATUS_REG3",	//
	"REV_STATUS_REG4",	//
	"REV_STATUS_REG5",	//
	"REV_STATUS_REG6",	//
	"REV_STATUS_REG7",	//
	"REV_STATUS_REG8",	//39

	"ETH_FIRMWARE_VERSION_REG",	//40
	"ETH_STATUS_REG1(b23-12&11-0=retx_cnt1&0)"	//41
};


//--------------------------------------------------------------
//     通用函数
//--------------------------------------------------------------
void loop_delay(void)
{
	INT32U i;
	for(i=0;i<0xFFFFFF;i++)
	{
	}
}

void EDAQ_Ctrl_Reg_WriteValueInit()
{
	for(int i = 0; i < MAX_MCBNUM; i++)
	{
		gCtrl_Reg_WriteV[i].All = 0;
	}
}
//--------------------------------------------------------------
//   MCB通用复位函数
//--------------------------------------------------------------
//功能：发出MCB FPGA逻辑复位请求
//参数：无
//返回值：0 = No error, others = Find errors.
INT8U EDAQ_CB_logic_reset(unsigned short nMCB)
{
	INT8U ret = 0;
	EDAQ_AUTO_RESET_REG reg;
	reg.All = 0;
	reg.Sepe.CB_logic_reset = 1;	
	ret = ECS_A32D32_write_confirmed(EDAQ_AUTO_RESET_REG_ADDR,reg.All, nMCB);
	return ret;
	/////////////////////////////////////////////////////////////////////////
	//INT8U  ret = 0;
	////重复执行次数
	//INT8U  RePrcCnt = 0;
	////
	//EDAQ_AUTO_RESET_REG regWrite;
	//EDAQ_AUTO_RESET_REG regRead;
	////
	//regWrite.All = 0;
	//regWrite.Sepe.CB_logic_reset = 1;
	////
	//do{
	//	//持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
	//	ETH_CTRL_read_buffer_residual_data(nMCB);
	//	//写寄存器值
	//	ret = ECS_A32D32_write_confirmed(EDAQ_AUTO_RESET_REG_ADDR, regWrite.All, nMCB);
	//	//寄存器值回读，使回读值等于设置值
	//	if(0 == ret)
	//	{
	//		regRead.All = 0;
	//		ret = ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR, &regRead.All, nMCB);
	//	}
	//	if(regWrite.Sepe.CB_logic_reset != regRead.Sepe.CB_logic_reset)
	//	{
	//		ret = -1;
	//	}
	//	/////////////////////////////
	//	RePrcCnt += 1;
	//	if(0 != ret)
	//	{
	//		Sleep(10);
	//	}
	//}while((0 != ret) && (RePrcCnt < gCmdReptN));
	////
	//return ret;
}

//--------------------------------------------------------------
//    固件信息读取函数
//--------------------------------------------------------------
//功能：打印MCB FPGA固件版本号和生成时间
//参数：无
//返回值：无
void EDAQ_read_and_dump_firmware_version(unsigned short nMCB)
{
	EDAQ_FIRMWARE_VERSION_REG reg;
	ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR,&reg.All, nMCB);
	printf("HMCB firmware version: %d.%d.%d.%d\n\r",reg.Sepe.major_ver,reg.Sepe.minor_ver,reg.Sepe.revision,reg.Sepe.build_num);
}

//--------------------------------------------------------------
//     MCB通用状态读取函数
//--------------------------------------------------------------
//功能：读取MCB FPGA FIFO错误
//参数：无
//返回值：FIFO错误
INT32U EDAQ_FIFO_error_status_rd(unsigned short nMCB)
{
	INT32U data;
	ECS_A32D32_read(CB_FIFO_ERR_LOG_REG_ADDR,&data, nMCB);
	return data;
}

//功能：打印MCB FPGA FIFO错误
//参数：无
//返回值：无
void EDAQ_FIFO_error_status_dump(unsigned short nMCB)
{
	printf("EDAQ FIFO status: 0x%X\n\r",EDAQ_FIFO_error_status_rd(nMCB));
}


//--------------------------------------------------------------
//     DB FPGA 程序烧写相关函数
//--------------------------------------------------------------

//功能：读取DB FPGA程序烧写完成标志
//参数：无
//返回值：DB FPGA程序烧写完成标志
INT16U EDAQ_DB_FPGA_cfg_finished_flag_rd(unsigned short nMCB)
{
	DB_COMM_STATUS_REG reg;
	ECS_A32D32_read(DB_COMM_STATUS_REG_ADDR,&reg.All, nMCB);
	return reg.Sepe.DB_FPGA_cfg_finished;
}

//功能：打印DB FPGA程序烧写完成标志
//参数：无
//返回值：无
void EDAQ_DB_FPGA_cfg_finished_flag_dump(unsigned short nMCB)
{
	printf("DB_FPGA_cfg_finished = %d\n\r",EDAQ_DB_FPGA_cfg_finished_flag_rd(nMCB));
}


//功能：发出DB FPGA程序烧写请求
//参数：无
//返回值：无
void EDAQ_cfg_fpga_req(unsigned short nMCB)
{	
	EDAQ_AUTO_RESET_REG reg;
	reg.All = 0;
	reg.Sepe.cfg_fpga_req = 1;
	ECS_A32D32_write_confirmed(EDAQ_AUTO_RESET_REG_ADDR,reg.All, nMCB);
}

//功能：使能DB FPGA程序烧写信号
//参数：
//	enable = 1: The RS, HS are used to config DB fpga. 
//			 0: The RS, HS are used to control DB.
//返回值：无
void EDAQ_DB_FPGA_cfg_enable(INT32U enable, unsigned short nMCB)
{
	EDAQ_GENERAL_CTRL_REG reg;
	ECS_A32D32_read(EDAQ_GENERAL_CTRL_REG_ADDR,&reg.All, nMCB);
	reg.Sepe.DB_fpga_cfg_enable = enable;
	ECS_A32D32_write_confirmed(EDAQ_GENERAL_CTRL_REG_ADDR,reg.All, nMCB);
}

//功能：读取DB FPGA程序烧写使能信号
//参数：无		
//返回值：
//	1: The RS, HS are used to config DB fpga.
//  0: The RS, HS are used to control DB.
INT16U EDAQ_DB_FPGA_cfg_enable_rd(unsigned short nMCB)
{
	EDAQ_GENERAL_CTRL_REG reg;
	ECS_A32D32_read(EDAQ_GENERAL_CTRL_REG_ADDR,&reg.All, nMCB);
	return reg.Sepe.DB_fpga_cfg_enable;
}

//功能：打印DB FPGA程序烧写使能信号
//参数：无		
//返回值：无
void EDAQ_DB_FPGA_cfg_enable_dump(unsigned short nMCB)
{
	printf("DB_FPGA_cfg_enable = %d\n\r",EDAQ_DB_FPGA_cfg_enable_rd(nMCB));
}


//功能：使能全部或部分DB Line的DB FPGA程序烧写信号
//	通过多次调用本函数，选择部分DB Line进行FPGA程序烧写，可以实现向不同DB Line写入不同的DB FPGA程序。
//参数：
//	enable_x4: 4 bits. 
//			b3 = 1：允许向DB Line #3的DB FPGA烧写程序
//			b2 = 1：允许向DB Line #2的DB FPGA烧写程序
//			b1 = 1：允许向DB Line #1的DB FPGA烧写程序
//			b0 = 1：允许向DB Line #0的DB FPGA烧写程序
//返回值：无
//注意：
//	要向某个或某些DB Line的DB FPGA烧写程序，除了调用本函数使能相应的DB Line，还必须调用EDAQ_DB_FPGA_cfg_enable(1)函数。
//返回值: 0 = No error, others = Find errors.
INT8U EDAQ_DB_Line_FPGA_program_enable(INT32U enable_x4, unsigned short nMCB)
{
	INT8U  ret = 0;
	EDAQ_GENERAL_CTRL_REG reg;
	ret = ECS_A32D32_read(EDAQ_GENERAL_CTRL_REG_ADDR,&reg.All, nMCB);
	if(0 == ret)
	{
	reg.Sepe.DB_FPGA_PROG_enable_x4 = enable_x4;
	ret = ECS_A32D32_write_confirmed(EDAQ_GENERAL_CTRL_REG_ADDR,reg.All, nMCB);
	}
	return ret;
}


//功能：写入DB FPGA烧写程序
//返回值: 0 = No error, others = Find errors.
INT8U EDAQ_DB_fpga_cfg_data_wr_via_ECS(INT32U data, unsigned short nMCB) 
{
	INT8U ret = 0;
	ret = ECS_A32D32_write(EDAQ_DB_FPGA_CFG_DATA_WR_SET_REG_ADDR,data, nMCB);
	return ret;
}


//功能：设置RB缓存工作模式
//参数：
//	Mode =	1: CMD mode. DB数据缓存采用该模式。
//			0: Auto mode. 测试数据和DB FPGA下载程序的缓存采用该模式。
//返回值: 0 = No error, others = Find errors.
INT8U EDAQ_RB_working_mode_set(INT32U mode, unsigned short nMCB)
{
	INT8U ret = 0;
	EDAQ_GENERAL_CTRL_REG reg;
	//重复执行次数
	INT8U  RePrcCnt = 0;
	do{
		ret = ECS_A32D32_read(EDAQ_GENERAL_CTRL_REG_ADDR,&reg.All, nMCB);
		if(0 == ret)
		{
			reg.Sepe.RB_working_mode = mode;
			ret = ECS_A32D32_write_confirmed(EDAQ_GENERAL_CTRL_REG_ADDR,reg.All, nMCB);
		}
		/////////////////////////////
		RePrcCnt += 1;
		if(0 != ret)
		{

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            Sleep(10);
#else
            //usleep(10*1000);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
		}
	}while((0 != ret) && (RePrcCnt < gCmdReptN));
	//
	return ret;
}

//功能：读取并打印RB缓存工作模式
//参数：无
//返回值: 0 = No error, others = Find errors.
INT8U EDAQ_RB_working_mode_dump(unsigned short nMCB)
{
	INT8U ret = 0;
	EDAQ_GENERAL_CTRL_REG reg;
	ret = ECS_A32D32_read(EDAQ_GENERAL_CTRL_REG_ADDR,&reg.All, nMCB);
	if(reg.Sepe.RB_working_mode == 0)
		printf("RB working mode: Auto mode (For DB FPGA config and test data.)\n\r");
	else
		printf("RB working mode: CMD mode (For DB data.)\n\r");
	/////////////
	return ret;
}


//功能：使能/除能SC data pkg monitor
//参数：
//	en=	1: Enable
//		0: Disable
//返回值: 0 = No error, others = Find errors.
INT8U EDAQ_SC_data_pkg_monitor_enable(INT32U en, unsigned short nMCB)
{
	INT8U ret = 0;
	EDAQ_GENERAL_CTRL_REG reg;
	ret = ECS_A32D32_read(EDAQ_GENERAL_CTRL_REG_ADDR,&reg.All, nMCB);
	if(0 == ret)
	{
		reg.Sepe.sc_pkg_monitor_en = en;
		ret = ECS_A32D32_write_confirmed(EDAQ_GENERAL_CTRL_REG_ADDR,reg.All, nMCB);
	}
	return 	ret;
}


//功能：使能/除能TEST_DATA_generator
//参数：
//	en=	1: Enable
//		0: Disable
//返回值: 0 = No error, others = Find errors.
 INT8U EDAQ_TEST_DATA_enable(INT32U en, unsigned short nMCB)
{
	INT8U ret = 0;
	EDAQ_GENERAL_CTRL_REG reg;
	ret = ECS_A32D32_read(EDAQ_GENERAL_CTRL_REG_ADDR,&reg.All, nMCB);
	if(0 == ret)
	{
		reg.Sepe.TEST_DATA_enable = en;
		ret = ECS_A32D32_write_confirmed(EDAQ_GENERAL_CTRL_REG_ADDR,reg.All, nMCB);
	}
	return ret;
}


//--------------------------------------------------------------
//      DB专用控制与状态读取函数
//--------------------------------------------------------------

//功能：发出DB FPGA复位请求
//返回值: 0 = No error, others = Find errors.
INT8U EDAQ_DB_reset_req(unsigned short nMCB)
{	
	INT8U ret = 0;
	EDAQ_AUTO_RESET_REG reg;
	reg.All = 0;
	reg.Sepe.DB_reset_req = 1;
	ret = ECS_A32D32_write_confirmed(EDAQ_AUTO_RESET_REG_ADDR,reg.All, nMCB);
	return ret;
}

//功能：发出DB定位请求
//返回值: 0 = No error, others = Find errors.
INT8U EDAQ_DB_locate_req(unsigned short nMCB)
{	
	INT8U ret = 0;
	EDAQ_AUTO_RESET_REG reg;
	reg.All = 0;
	reg.Sepe.DB_locate_req = 1;
	ret = ECS_A32D32_write_confirmed(EDAQ_AUTO_RESET_REG_ADDR,reg.All, nMCB);
	return ret;
}


//功能：发出DB参数配置请求
INT8U EDAQ_DB_prm_cfg_req(unsigned short nMCB)
{	
	INT8U ret = 0;
	EDAQ_AUTO_RESET_REG reg;
	reg.All = 0;
	reg.Sepe.DB_prm_cfg_req = 1;
	ret = ECS_A32D32_write_confirmed(EDAQ_AUTO_RESET_REG_ADDR,reg.All, nMCB);
	return ret;
}

////功能：发出DB DAQ命令传输请求
//void EDAQ_DB_daq_cmd_tx_req(unsigned short nMCB)
//{	
//	EDAQ_AUTO_RESET_REG reg;
//	reg.All = 0;
//	reg.Sepe.DB_daq_cmd_tx_req = 1;
//	ECS_A32D32_write_confirmed(EDAQ_AUTO_RESET_REG_ADDR,reg.All, nMCB);
//}

//功能：发出DB DAQ命令传输请求
INT8U EDAQ_DB_daq_cmd_tx_req(unsigned short nMCB)
{	
	INT8U  ret = 0;
	//重复执行次数
	INT8U  RePrcCnt = 0;
	//
	EDAQ_AUTO_RESET_REG reg;
	do
	{
		reg.All = 0;
		reg.Sepe.DB_daq_cmd_tx_req = 1;
		//
		ret = ECS_A32D32_write_confirmed(EDAQ_AUTO_RESET_REG_ADDR,reg.All, nMCB);
		/////////////////////////////
		RePrcCnt += 1;
		if(0 != ret)
		{

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            Sleep(10);
#else
            //usleep(10*1000);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
		}
	}while((0 != ret) && (RePrcCnt < gCmdReptN));
	//
	return ret;
	////////////////////////////////////////////////////////////////////
	//INT8U  ret = 0;
	////重复执行次数
	//INT8U  RePrcCnt = 0;
	////
	//EDAQ_AUTO_RESET_REG regWrite;
	//EDAQ_AUTO_RESET_REG regRead;
	////
	//regWrite.All = 0;
	//regWrite.Sepe.DB_daq_cmd_tx_req = 1;
	////
	//do{
	//	//持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
	//	ETH_CTRL_read_buffer_residual_data(nMCB);
	//	//发出DB DAQ命令传输请求
	//	ret = ECS_A32D32_write_confirmed(EDAQ_AUTO_RESET_REG_ADDR, regWrite.All, nMCB);
	//	//寄存器值回读，使回读值等于设置值
	//	if(0 == ret)
	//	{
	//		regRead.All = 0;
	//		ret = ECS_A32D32_read(EDAQ_AUTO_RESET_REG_ADDR, &regRead.All, nMCB);
	//	}
	//	//判断发送与回读值是否一致
	//	if(regWrite.Sepe.DB_daq_cmd_tx_req != regRead.Sepe.DB_daq_cmd_tx_req)
	//	{
	//		ret = -1;
	//	}
	//	/////////////////////////////
	//	RePrcCnt += 1;
	//	if(0 != ret)
	//	{
	//		Sleep(10);
	//	}
	//}while((0 != ret) && (RePrcCnt < gCmdReptN));
	////
	//return ret;
}


//功能：读取DB communication cmd processing busy flag
INT16U EDAQ_DB_comm_cmd_process_busy_flag_rd(unsigned short nMCB)
{
	DB_COMM_STATUS_REG reg;
	ECS_A32D32_read(DB_COMM_STATUS_REG_ADDR,&reg.All, nMCB);
	return reg.Sepe.DB_comm_cmd_process_busy;
}

//功能：打印DB communication cmd processing busy flag
void EDAQ_DB_comm_cmd_process_busy_flag_dump(unsigned short nMCB)
{
	printf("DB_comm_cmd_process_busy = %d\n\r",EDAQ_DB_comm_cmd_process_busy_flag_rd(nMCB));
}

//功能：一直等待，直到DB communication cmd processing busy flag清零
void EDAQ_wait_until_cmd_process_busy_flag_clr(unsigned short nMCB)
{
	while(1)
	{
		if (EDAQ_DB_comm_cmd_process_busy_flag_rd(nMCB) == 0)
			return;
	}
}


//功能：设置DB板子数目
//参数：
//	DLx_DB_num: The number of DB board on DB Line x
//返回值：0 = No error, others = Find errors.
INT8U EDAQ_DB_num_set(INT8U DL3_DB_num,INT8U DL2_DB_num,INT8U DL1_DB_num,INT8U DL0_DB_num, unsigned short nMCB)
{
	INT8U ret = 0;
	DB_NUM_SET_REG reg;
	ret = ECS_A32D32_read(DB_NUM_SET_REG_ADDR,&reg.All, nMCB);
	if(0 == ret)
	{
		reg.Sepe.DL0_DB_num = DL0_DB_num;
		reg.Sepe.DL1_DB_num = DL1_DB_num;
		reg.Sepe.DL2_DB_num = DL2_DB_num;
		reg.Sepe.DL3_DB_num = DL3_DB_num;
		ret = ECS_A32D32_write_confirmed(DB_NUM_SET_REG_ADDR,reg.All, nMCB);
	}
	return ret;
}

//功能：读取DB板子数目
INT32U EDAQ_DB_num_setting_rd(unsigned short nMCB)
{
	DB_NUM_SET_REG reg;
	ECS_A32D32_read(DB_NUM_SET_REG_ADDR,&reg.All, nMCB);
	return reg.All;
}

//功能：打印DB板子数目
void EDAQ_DB_num_setting_dump(unsigned short nMCB)
{
	DB_NUM_SET_REG reg;
	ECS_A32D32_read(DB_NUM_SET_REG_ADDR,&reg.All, nMCB);
	printf("The setting value of DB num of DB-Line[3..0]：%d, %d, %d, %d\n\r",reg.Sepe.DL3_DB_num,reg.Sepe.DL2_DB_num,reg.Sepe.DL1_DB_num,reg.Sepe.DL0_DB_num);
}


//功能：设置DB Line数目
//参数：
//	DB_line_num: DB Line数目的设定值。注意：DB Line实际使能的数目受系统最大支持数目限制。
//返回值：0 = No error, others = Find errors.
INT8U EDAQ_DB_line_num_set(INT8U DB_line_num, unsigned short nMCB)
{
	INT8U  ret = 0;
	//
	gCtrl_Reg_WriteV[nMCB].Sepe.DB_line_num = DB_line_num;
	//
	ret = EDAQ_daq_SetDBCommCtrlReg(nMCB);
	//
	return ret;

	//INT8U ret = 0;
	//EDAQ_DB_COMM_CTRL_REG reg;
	//ret = ECS_A32D32_read(EDAQ_DB_COMM_CTRL_REG_ADDR,&reg.All, nMCB);
	//if(0 == ret)
	//{
	//	reg.Sepe.DB_line_num = DB_line_num;
	//	ret = ECS_A32D32_write_confirmed(EDAQ_DB_COMM_CTRL_REG_ADDR,reg.All, nMCB);
	//}
	//return ret;
}

//功能：读取DB Line数目的设定值
INT8U EDAQ_DB_line_num_rd(unsigned short nMCB)
{
	EDAQ_DB_COMM_CTRL_REG reg;
	ECS_A32D32_read(EDAQ_DB_COMM_CTRL_REG_ADDR,&reg.All, nMCB);
	return reg.Sepe.DB_line_num;
}

//功能：打印DB Line数目的设定值
void EDAQ_DB_line_num_dump(unsigned short nMCB)
{
	printf("The setting value of DB line number via ECS: %d\n\r",EDAQ_DB_line_num_rd(nMCB));
	
	//打印系统支持的DB Line数目的最大值
	EDAQ_read_and_dump_DB_line_max_num(nMCB);

	//打印实际使能的DB Line数目
	EDAQ_read_and_dump_DB_line_enable_num(nMCB);
	
	//
	EDAQ_read_and_dump_DB_line_enable_vector(nMCB);
}

//功能：设置 EDAQ_DB_COMM_CTRL_REG_ADDR 寄存器值
//包括DB_line_num、DAQ_mode_enable、HMCB_work_mode、DB_DAQ_req_sel参数
//参数：
//	enable = 1: DAQ mode is enabled.
//           0: DAQ mode is disabled.
//	sel = (触发模式)
//#define  EDAQ_DAQ_TRIG_SEL_BEAM_PULSE		1
//#define  EDAQ_DAQ_TRIG_SEL_PERIOD_TRIG	2
//#define  EDAQ_DAQ_TRIG_SEL_CASCADE_TRIG	3
//#define  EDAQ_DAQ_TRIG_SEL_NONE			0
//DB_line_num: DB Line数目的设定值。注意：DB Line实际使能的数目受系统最大支持数目限制。
//master_en 设置MCB工作模式 
	// 1: Work as Master HMCB
	// 0: Work as Cascaded HMCB
//返回值：0 = No error, others = Find errors.
//注意：该函数返回时，可以确保ECS写操作已经执行。
//INT8U EDAQ_SetDBCommCtrlReg(INT32U enable, INT32U sel, INT8U DB_line_num, INT32U master_en, unsigned short nMCB)
INT8U EDAQ_daq_SetDBCommCtrlReg(unsigned short nMCB)
{
     int  ret = 0;
	EDAQ_DB_COMM_CTRL_REG regread;
	regread.All = 0;
	//重复执行次数
	INT8U  RePrcCnt = 0;
       do{
		//持续从_ETH_CTRL_socket读取数据，直到清空缓存中的数据。
		ETH_CTRL_read_buffer_residual_data(nMCB);

		//写寄存器值
        ret = ECS_A32D32_write_confirmed(EDAQ_DB_COMM_CTRL_REG_ADDR, gCtrl_Reg_WriteV[nMCB].All, nMCB);
		//寄存器值回读，使回读值等于设置值
		if(0 == ret)
		{
            ret = ECS_A32D32_read(EDAQ_DB_COMM_CTRL_REG_ADDR, &regread.All, nMCB);
		}
		if((regread.Sepe.DAQ_mode_enable != gCtrl_Reg_WriteV[nMCB].Sepe.DAQ_mode_enable) &&
		   (regread.Sepe.DB_DAQ_req_sel != gCtrl_Reg_WriteV[nMCB].Sepe.DB_DAQ_req_sel) &&
		   (regread.Sepe.DB_line_num != gCtrl_Reg_WriteV[nMCB].Sepe.DB_line_num) &&
		   (regread.Sepe.HMCB_work_mode != gCtrl_Reg_WriteV[nMCB].Sepe.HMCB_work_mode))
		{
            ret = -1;
		}
		/////////////////////////////
		RePrcCnt += 1;
		if(0 != ret)
		{

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            Sleep(10);
#else
            //usleep(10*1000);
             std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
        }
    }while((0 != ret) && (RePrcCnt < gCmdReptN));
    //
	return ret;
}

//功能：使能DAQ模式
//参数：
//	enable = 1: DAQ mode is enabled.
//           0: DAQ mode is disabled. 
//返回值：0 = No error, others = Find errors.
//注意：该函数返回时，可以确保ECS写操作已经执行。
INT8U EDAQ_daq_mode_enable(INT32U enable, unsigned short nMCB) 
{
	INT8U  ret = 0;
	//
	gCtrl_Reg_WriteV[nMCB].Sepe.DAQ_mode_enable = enable;
	//
	ret = EDAQ_daq_SetDBCommCtrlReg(nMCB);
	//
	return ret;
}

//功能：读取DAQ模式
INT16U EDAQ_daq_mode_enable_rd(unsigned short nMCB)
{
	EDAQ_DB_COMM_CTRL_REG reg;
	ECS_A32D32_read(EDAQ_DB_COMM_CTRL_REG_ADDR,&reg.All, nMCB);
	return reg.Sepe.DAQ_mode_enable;
}

//功能：打印DAQ模式
void EDAQ_daq_mode_enable_dump(unsigned short nMCB)
{
	if(EDAQ_daq_mode_enable_rd(nMCB) == 1)
		printf("The DB DAQ mode has been enabled!\n\r");
	else
		printf("The DB DAQ mode has been disabled!\n\r");

}


INT8U EDAQ_daq_req_sel_And_mode_enable(INT32U enable, INT32U sel, unsigned short nMCB) 
{
	INT8U  ret = 0;
	//
	gCtrl_Reg_WriteV[nMCB].Sepe.DAQ_mode_enable = enable;
	//
	gCtrl_Reg_WriteV[nMCB].Sepe.DB_DAQ_req_sel = sel;
	//
	ret = EDAQ_daq_SetDBCommCtrlReg(nMCB);
	//
	return ret;
}

//功能：设置DAQ触发
//	sel = (触发模式)
//#define  EDAQ_DAQ_TRIG_SEL_BEAM_PULSE		1
//#define  EDAQ_DAQ_TRIG_SEL_PERIOD_TRIG	2
//#define  EDAQ_DAQ_TRIG_SEL_CASCADE_TRIG	3
//#define  EDAQ_DAQ_TRIG_SEL_NONE			0
//返回值：0 = No error, others = Find errors.
INT8U EDAQ_daq_req_sel(INT32U sel, unsigned short nMCB) 
{
	INT8U  ret = 0;
	//
	gCtrl_Reg_WriteV[nMCB].Sepe.DB_DAQ_req_sel = sel;
	//
	ret = EDAQ_daq_SetDBCommCtrlReg(nMCB);
	//
	return ret;
	//EDAQ_DB_COMM_CTRL_REG reg;
	//ECS_A32D32_read(EDAQ_DB_COMM_CTRL_REG_ADDR,&reg.All, nMCB);
	//reg.Sepe.DB_DAQ_req_sel = sel;
	//ECS_A32D32_write_confirmed(EDAQ_DB_COMM_CTRL_REG_ADDR,reg.All, nMCB);
}

//功能：读取DAQ触发
INT16U EDAQ_daq_req_sel_rd(unsigned short nMCB)
{
	EDAQ_DB_COMM_CTRL_REG reg;
	ECS_A32D32_read(EDAQ_DB_COMM_CTRL_REG_ADDR,&reg.All, nMCB);
	return reg.Sepe.DB_DAQ_req_sel;
}



//功能：打印DAQ触发
void EDAQ_daq_req_sel_dump(unsigned short nMCB)
{
	if(EDAQ_daq_req_sel_rd(nMCB) == EDAQ_DAQ_TRIG_SEL_PERIOD_TRIG)
		printf("The DB DAQ trigger: periodic trigger!\n\r");
	else if(EDAQ_daq_req_sel_rd(nMCB) == EDAQ_DAQ_TRIG_SEL_BEAM_PULSE)
		printf("The DB DAQ trigger: external beam pulse!\n\r");
	else if(EDAQ_daq_req_sel_rd(nMCB) == EDAQ_DAQ_TRIG_SEL_CASCADE_TRIG)
		printf("The DB DAQ trigger: cascaded trigger!\n\r");
	else
		printf("The DB DAQ trigger: no trigger!\n\r");

}

//功能：设置MCB工作模式 
//参数:
// 1: Work as Master HMCB
// 0: Work as Cascaded HMCB
//返回值：0 = No error, others = Find errors.
INT8U EDAQ_HMCB_work_mode_set(INT32U master_en, unsigned short nMCB) 
{
	INT8U  ret = 0;
	//
	gCtrl_Reg_WriteV[nMCB].Sepe.HMCB_work_mode = master_en;
	//
	ret = EDAQ_daq_SetDBCommCtrlReg(nMCB);
	//
	return ret;
	//INT8U ret = 0;
	//EDAQ_DB_COMM_CTRL_REG reg;
	//ret = ECS_A32D32_read(EDAQ_DB_COMM_CTRL_REG_ADDR,&reg.All, nMCB);
	//if(0 == ret)
	//{
	//	reg.Sepe.HMCB_work_mode = master_en;
	//	ret = ECS_A32D32_write_confirmed(EDAQ_DB_COMM_CTRL_REG_ADDR,reg.All, nMCB);
	//}
	//return ret;
}

//功能：读取MCB工作模式 
INT16U EDAQ_HMCB_work_mode_rd(unsigned short nMCB)
{
	EDAQ_DB_COMM_CTRL_REG reg;
	ECS_A32D32_read(EDAQ_DB_COMM_CTRL_REG_ADDR,&reg.All, nMCB);
	return reg.Sepe.HMCB_work_mode;
}

//功能：打印MCB工作模式 
void EDAQ_HMCB_work_mode_dump(unsigned short nMCB)
{
	if(EDAQ_HMCB_work_mode_rd(nMCB) == 1)
		printf("HMCB work mode: master HMCB!\n\r");
	else
		printf("HMCB work mode: cascaded HMCB!\n\r");

}


//功能：设置DAQ命令执行时间 
//参数: 
//	t: 命令执行时间，单位uS.
void EDAQ_daq_cmd_exct_time_set(INT32U t, unsigned short nMCB)
{
	if((t&0xFF000000) != 0)
	{
		t = t & 0x00FFFFFF;
		printf("The daq cmd execute time is too big! The highest 8 bits have been cleared.\n\r");
	}
	ECS_A32D32_write_confirmed(EDAQ_DB_DAQ_CMD_EXCT_TIME_SET_REG_ADDR,t, nMCB);
}

//功能：读取DAQ命令执行时间 
INT32U EDAQ_daq_cmd_exct_time_rd(unsigned short nMCB)
{
	INT32U t;
	ECS_A32D32_read(EDAQ_DB_DAQ_CMD_EXCT_TIME_SET_REG_ADDR,&t, nMCB);
	return t;
}

//功能：打印DAQ命令执行时间 
void EDAQ_daq_cmd_exct_time_dump(unsigned short nMCB)
{
	printf("The daq cmd execute time has been set to %d us.\n\r",EDAQ_daq_cmd_exct_time_rd(nMCB));
}

//功能：设置non-DAQ命令执行时间 
//参数: 
//	t: 命令执行时间，单位uS.
void EDAQ_non_daq_cmd_exct_time_set(INT32U t, unsigned short nMCB)
{
	if((t&0xFF000000) != 0)
	{
		t = t & 0x00FFFFFF;
		printf("The non-daq cmd execute time is too big! The highest 8 bits have been cleared.\n\r");
	}
	ECS_A32D32_write_confirmed(EDAQ_DB_NON_DAQ_CMD_EXCT_TIME_SET_REG_ADDR,t, nMCB);
}

//功能：读取non-DAQ命令执行时间 
INT32U EDAQ_non_daq_cmd_exct_time_rd(unsigned short nMCB)
{
	INT32U t;
	ECS_A32D32_read(EDAQ_DB_NON_DAQ_CMD_EXCT_TIME_SET_REG_ADDR,&t, nMCB);	
	return t;
}

//功能：打印non-DAQ命令执行时间 
void EDAQ_non_daq_cmd_exct_time_dump(unsigned short nMCB)
{
	printf("The non-daq cmd execute time has been set to %d us.\n\r",EDAQ_non_daq_cmd_exct_time_rd(nMCB));
}

//功能：设置DB积分时间
//参数: 
//	t: 积分时间，单位uS.
void EDAQ_DB_integration_time_set(INT32U t, unsigned short nMCB)
{
	if((t&0xFF000000) != 0)
	{
		t = t & 0x00FFFFFF;
		printf("The db integration time is too big! The highest 8 bits have been cleared.\n\r");
	}
	ECS_A32D32_write_confirmed(EDAQ_DB_INTEGRATION_TIME_SET_REG_ADDR,t, nMCB);
}

//功能：读取DB积分时间
INT32U EDAQ_DB_integration_time_rd(unsigned short nMCB)
{
	INT32U t;
	ECS_A32D32_read(EDAQ_DB_INTEGRATION_TIME_SET_REG_ADDR,&t, nMCB);
	return t;
}

//功能：打印DB积分时间
void EDAQ_DB_integration_time_dump(unsigned short nMCB)
{
	printf("The db integration time time has been set to %d us.\n\r",EDAQ_DB_integration_time_rd(nMCB));
}


//功能：设置内部触发周期
//参数: 
//	t: 触发周期，单位uS.
void internal_trigger_period_set(INT32U t, unsigned short nMCB)
{
	ECS_A32D32_write_confirmed(INT_TRIG_PERIOD_SET_REG_ADDR,t, nMCB);
}

//功能：读取内部触发周期
INT32U internal_trigger_period_rd(unsigned short nMCB)
{
	INT32U t;
	ECS_A32D32_read(INT_TRIG_PERIOD_SET_REG_ADDR,&t, nMCB);
	return t;
}

//功能：打印内部触发周期
void internal_trigger_period_dump(unsigned short nMCB)
{
	printf("The internal_trigger_period has been set to %d us.\n\r",internal_trigger_period_rd(nMCB));
}

//功能：设置内部触发脉宽
//参数: 
//	t: 触发脉宽，单位uS. Max value is 65535 us
void internal_trigger_pulse_width_set(INT32U d, unsigned short nMCB)
{
	d = d & 0xFFFF;
	ECS_A32D32_write_confirmed(INT_TRIG_PULSE_WIDTH_SET_REG_ADDR,d, nMCB);
}

//功能：读取内部触发脉宽
INT32U internal_trigger_pulse_width_rd(unsigned short nMCB)
{
	INT32U t;
	ECS_A32D32_read(INT_TRIG_PULSE_WIDTH_SET_REG_ADDR,&t, nMCB);
	return t;
}

//功能：打印内部触发脉宽
void internal_trigger_pulse_width_dump(unsigned short nMCB)
{
	printf("The internal trigger pulse width has been set to %d us.\n\r",internal_trigger_pulse_width_rd(nMCB));
}

//功能：设置DDI锁存延迟时间
//参数: 
//	t: 延迟时间，单位uS. Max value is 65535 us
void DDI_latch_latency_set(INT32U d, unsigned short nMCB)
{
	d = d & 0xFFFF;
	ECS_A32D32_write_confirmed(DDI_LATCH_LATENCY_SET_REG_ADDR,d, nMCB);
}

//功能：读取DDI锁存延迟时间
INT32U DDI_latch_latency_rd(unsigned short nMCB)
{
	INT32U t;
	ECS_A32D32_read(DDI_LATCH_LATENCY_SET_REG_ADDR,&t, nMCB);
	return t;
}

//功能：打印DDI锁存延迟时间
void DDI_latch_latency_dump(unsigned short nMCB)
{
	printf("DDI latch latency has been set to %d us.\n\r",DDI_latch_latency_rd(nMCB));
}


//--------------------------------------------------------------
//     DB通讯 状态读取函数
//--------------------------------------------------------------
//功能：打印DB数据传输时钟
//参数：无
//返回值：无
void EDAQ_read_and_dump_DB_CLK_FREQ(unsigned short nMCB)
{
	DB_COMM_STATUS_REG reg;
	ECS_A32D32_read(DB_COMM_STATUS_REG_ADDR,&reg.All, nMCB);
	printf("DB CLK: %dMHz\n\r",reg.Sepe.DB_CLK_FREQ*10);
}


//功能：打印DB Line使能向量
//参数：无
//返回值：无
void EDAQ_read_and_dump_DB_line_enable_vector(unsigned short nMCB)
{
	DB_COMM_STATUS_REG reg;
	INT8U vec;
	ECS_A32D32_read(DB_COMM_STATUS_REG_ADDR,&reg.All, nMCB);
	vec = reg.Sepe.DB_Line_enable_x4;
	printf("DB-Line[3..0] enabled flags: %d %d %d %d\n\r",(vec>>3)&0x1,(vec>>2)&0x1,(vec>>1)&0x1,(vec>>0)&0x1);
}


//功能：读取实际使能的DB Line数目
//参数：无
//返回值：实际使能的DB line数目：1，2，3，4
INT8U EDAQ_read_DB_line_enable_num(unsigned short nMCB)
{
	DB_COMM_STATUS_REG reg;
	ECS_A32D32_read(DB_COMM_STATUS_REG_ADDR,&reg.All, nMCB);
	return reg.Sepe.DB_Line_enable_num;
}

//功能：打印实际使能的DB Line数目
//参数：无
//返回值：无
void EDAQ_read_and_dump_DB_line_enable_num(unsigned short nMCB)
{
	printf("The number of DB Lines enabled acutually: %d\n\r",EDAQ_read_DB_line_enable_num(nMCB));
}

//功能：读取当前模块支持的最大DB Line数目
//参数：无
//返回值：当前模块支持的最大DB Line数目
INT8U EDAQ_read_DB_line_max_num(unsigned short nMCB)
{
	DB_COMM_STATUS_REG reg;
	INT8U DB_Line_max_num;
	ECS_A32D32_read(DB_COMM_STATUS_REG_ADDR,&reg.All, nMCB);
	DB_Line_max_num = reg.Sepe.DB_Line_max_num;
	if (DB_Line_max_num == 0)
		DB_Line_max_num = 1;
	return DB_Line_max_num;
}


//功能：打印当前模块支持的最大DB Line数目
//参数：无
//返回值：无
void EDAQ_read_and_dump_DB_line_max_num(unsigned short nMCB)
{
	printf("The maximum value of DB line number supported by this module: %d\n\r",EDAQ_read_DB_line_max_num(nMCB));
}


//功能：打印对数据流解析得到的DB数目
//参数：无
//返回值：无
void EDAQ_read_and_dump_parsed_DB_num(unsigned short nMCB)
{
	DB_NUM_STATUS_REG reg;
	ECS_A32D32_read(DB_NUM_STATUS_REG_ADDR,&reg.All, nMCB);
	printf("The parsed result of DB num of DB-Line[3..0]：%d, %d, %d, %d\n\r",reg.Sepe.DL3_DB_num,reg.Sepe.DL2_DB_num,reg.Sepe.DL1_DB_num,reg.Sepe.DL0_DB_num);
}

//--------------------------------------------------------------
//     Ring buffer control functions
//--------------------------------------------------------------
//功能：读取Ring buffer almost full flag
INT16U EDAQ_RB_almost_full_flag_rd(unsigned short nMCB)
{
	EDAQ_GENERAL_STATUS_REG reg;
	ECS_A32D32_read(EDAQ_GENERAL_STATUS_REG_ADDR,&reg.All, nMCB);
	return reg.Sepe.RB_almost_full;
}

//功能：打印Ring buffer almost full flag
void EDAQ_RB_almost_full_flag_dump(unsigned short nMCB)
{
	printf("RB_almost_full_flag = %d\n\r",EDAQ_RB_almost_full_flag_rd(nMCB));
}


//功能：复位Ring buffer的输入和输出缓存区
void EDAQ_RBxB_reset(unsigned short nMCB)
{	
	EDAQ_AUTO_RESET_REG reg;
	reg.All = 0;
	reg.Sepe.RBxB_reset = 1;
	ECS_A32D32_write_confirmed(EDAQ_AUTO_RESET_REG_ADDR,reg.All, nMCB);
}

//功能：复位Ring buffer的输出缓存区
void EDAQ_RBOB_reset(unsigned short nMCB)
{	
	EDAQ_AUTO_RESET_REG reg;
	reg.All = 0;
	reg.Sepe.RBOB_reset = 1;
	ECS_A32D32_write_confirmed(EDAQ_AUTO_RESET_REG_ADDR,reg.All, nMCB);
}


//功能：选择Ring buffer写入端口
//返回值: 0 = No error, others = Find errors.
INT8U EDAQ_RB_wr_port_sel(INT32U port, unsigned short nMCB) 
{
	INT8U ret = 0;
	EDAQ_RB_ACCESS_CTRL_REG reg;
	ret = ECS_A32D32_read(EDAQ_RB_ACCESS_CTRL_REG_ADDR,&reg.All, nMCB);
	if(0 == ret)
	{
		reg.Sepe.RB_wr_port_sel = port;
		ret = ECS_A32D32_write_confirmed(EDAQ_RB_ACCESS_CTRL_REG_ADDR,reg.All, nMCB);
	}
	return ret;
}

//功能：读取Ring buffer写入端口
INT16U EDAQ_RB_wr_port_rd(unsigned short nMCB)
{
	EDAQ_RB_ACCESS_CTRL_REG reg;
	ECS_A32D32_read(EDAQ_RB_ACCESS_CTRL_REG_ADDR,&reg.All, nMCB);
	return reg.Sepe.RB_wr_port_sel;
}

//功能：打印Ring buffer写入端口
void EDAQ_RB_wr_port_dump(unsigned short nMCB)
{
	INT32U port;
	port = EDAQ_RB_wr_port_rd(nMCB);

	switch(port)
	{
		case EDAQ_RB_WR_SEL_NO_DATA:
			printf("RB write port selection: No data.\n\r");
			break;
		case EDAQ_RB_WR_SEL_ECS_FPGA_CFG_DATA:
			printf("RB write port selection: DB FPGA config data written from ECS bus!\n\r");
			break;
		case EDAQ_RB_WR_SEL_DB_RX_DATA:
			printf("RB write port selection: DB daq data received from DB!\n\r");
			break;
		case EDAQ_RB_WR_SEL_INT_GEN_DATA:
			printf("RB write port selection: Counting data generated inside EDAQ FPGA!\n\r");
			break;
		case EDAQ_RB_WR_SEL_ETH_FPGA_CFG_DATA:
			printf("RB write port selection: DB FPGA config data received from ETH data link!\n\r");
			break;
		default:
			printf("RB write port selection: unknownd port!\n\r");
			break;
	}	
}

//功能：选择Ring buffer读取端口
//返回值: 0 = No error, others = Find errors.
INT8U EDAQ_RB_rd_port_sel(INT32U port, unsigned short nMCB) 
{
	INT8U ret = 0;
	EDAQ_RB_ACCESS_CTRL_REG reg;
	//重复执行次数
	INT8U  RePrcCnt = 0;
	do{
		ret = ECS_A32D32_read(EDAQ_RB_ACCESS_CTRL_REG_ADDR,&reg.All, nMCB);
		if(0 == ret)
		{
			reg.Sepe.RB_rd_port_sel = port;
			ret = ECS_A32D32_write_confirmed(EDAQ_RB_ACCESS_CTRL_REG_ADDR,reg.All, nMCB);
		}
		/////////////////////////////
		RePrcCnt += 1;
		if(0 != ret)
		{

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            Sleep(10);
#else
            //usleep(10*1000);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
		}
	}while((0 != ret) && (RePrcCnt < gCmdReptN));
	//
	return ret;
}


//功能：读取Ring buffer读取端口
INT16U EDAQ_RB_rd_port_rd(unsigned short nMCB)
{
	EDAQ_RB_ACCESS_CTRL_REG reg;
	ECS_A32D32_read(EDAQ_RB_ACCESS_CTRL_REG_ADDR,&reg.All, nMCB);
	return reg.Sepe.RB_rd_port_sel;
}

//功能：打印Ring buffer读取端口
void EDAQ_RB_rd_port_dump(unsigned short nMCB)
{
	INT32U port;
	port = EDAQ_RB_rd_port_rd(nMCB);

	switch(port)
	{
		case EDAQ_RB_RD_SEL_NO_DATA:
			printf("RB read port selection: No data.\n\r");
			break;
		case EDAQ_RB_RD_SEL_FPGA_CFG_DATA:
			printf("RB read port selection: Read data is used to config DB FPGA!\n\r");
			break;
		case EDAQ_RB_RD_SEL_ETH_DATA:
			printf("RB read port selection: Read data is sent to ETH data transmission link!\n\r");
			break;
		default:
			printf("RB read port selection: unknownd port!\n\r");
			break;
	}	
}


//-------------------------------------------------------------
//	EEPROM读写函数
//-------------------------------------------------------------

//功能：发出EEPROM_ECS_wr_req
void EDAQ_EEPROM_ECS_wr_req(unsigned short nMCB)
{	
	EDAQ_AUTO_RESET_REG reg;
	reg.All = 0;
	reg.Sepe.EEPROM_ECS_wr_req = 1;
	ECS_A32D32_write_confirmed(EDAQ_AUTO_RESET_REG_ADDR,reg.All, nMCB);
}

//功能：发出EEPROM_ECS_rd_req
void EDAQ_EEPROM_ECS_rd_req(unsigned short nMCB)
{	
	EDAQ_AUTO_RESET_REG reg;
	reg.All = 0;
	reg.Sepe.EEPROM_ECS_rd_req = 1;
	ECS_A32D32_write_confirmed(EDAQ_AUTO_RESET_REG_ADDR,reg.All, nMCB);
}


//功能：设置EEPROM CTRL register
void EDAQ_EEPROM_CTRL_reg_set(INT32U wr_data,INT32U rw_addr, unsigned short nMCB) 
{
	EEPROM_CTRL_REG reg;
	reg.Sepe.EEPROM_ECS_wr_data = wr_data;
	reg.Sepe.EEPROM_ECS_rw_addr = rw_addr;
	reg.Sepe.EEPROM_lcl_rd_addr = 0x78;
	ECS_A32D32_write_confirmed(EEPROM_CTRL_REG_ADDR,reg.All, nMCB);
}

//功能：Write a certain number of data into EEPROM
//参数：
//	addr: 指定EEPROM起始读取地址。由于最小读取数据宽度为16bit，其最低位必须为0。
//	wr_len: 写入16位数的个数
//	p_data: 指向读取数据存储数组
//返回值：
//	0: no error
// -1: error.
INT8U EDAQ_EEPROM_data_write(INT8U addr, INT8U wr_len, INT16U *pdata, unsigned short nMCB)
{
	INT8U ret = 0;
    INT8U i;
	EEPROM_RDDATA_REG reg;

	for(i=0;i<wr_len;i++)
	{		
		EDAQ_EEPROM_CTRL_reg_set(pdata[i],addr + i*2, nMCB);
		EDAQ_EEPROM_ECS_wr_req(nMCB);
		do{	
			ECS_A32D32_read(EEPROM_RDDATA_REG_ADDR,&reg.All, nMCB);
			if(reg.Sepe.EEPROM_ECS_rw_finished == 1)
			{
				if(reg.Sepe.EEPROM_rd_cs_byte != 0xAA)
				{
					printf("EEPROM write error: CS byte error (0x%X).\n\r",reg.Sepe.EEPROM_rd_cs_byte);
					ret = -1;
				}
				
				if(reg.Sepe.OW_bus_idle_err == 1)
				{
					printf("EEPROM write error: EEPROM one wire bus idle error.\n\r");
					ret = -1;
				}
				if(reg.Sepe.OW_slave_present == 0)
				{
					printf("EEPROM write error: EEPROM isn't present.\n\r");
					ret = -1;
				}					
			}
		}while(reg.Sepe.EEPROM_ECS_rw_finished == 0);
	}
	return ret;
}


//功能：Write a certain number of zero into EEPROM
//参数：
//	addr: 指定EEPROM起始读取地址。由于最小写入数据宽度为16bit，其最低位必须为0。
//	wr_len: 写入16位零的个数
//返回值：
//	0: no error
// -1: error.
INT8U EDAQ_EEPROM_write_zero(INT8U addr, INT8U wr_len, unsigned short nMCB)
{
	INT8U ret = 0;
    INT8U i;
	INT16U data[64];
	for(i=0;i<64;i++)
		data[i] = 0;
	ret = EDAQ_EEPROM_data_write(addr, wr_len, data, nMCB);
	
	return ret;
}


//功能：Read a certain number of EEPROM data
//参数：
//	addr: 指定EEPROM起始读取地址。由于最小读取数据宽度为16bit，其最低位必须为0。
//	rd_len: 读取16位数的个数
//	p_data: 指向读取数据存储数组
//返回值：
//	0: no error
// -1: error.
INT8U EDAQ_EEPROM_data_read(INT8U addr, INT8U rd_len, INT16U *pdata, unsigned short nMCB)
{
	INT8U ret = 0;
    INT8U i;
	EEPROM_RDDATA_REG reg;
	//
	INT32U c_t,d_t;
	c_t = (INT32U) time(0); 
	//
	for(i=0;i<rd_len;i++)
	{		
		EDAQ_EEPROM_CTRL_reg_set(0,addr + i*2, nMCB);
		EDAQ_EEPROM_ECS_rd_req(nMCB);
		//
		c_t = (INT32U) time(0); 
		//
		do{	
			d_t = (INT32U) time(0);
			//
			ECS_A32D32_read(EEPROM_RDDATA_REG_ADDR,&reg.All, nMCB);
			if(reg.Sepe.EEPROM_ECS_rw_finished == 1)
			{
				pdata[i] = reg.Sepe.EEPROM_ECS_rd_data;
				if(reg.Sepe.OW_bus_idle_err == 1)
				{
					printf("EEPROM read error: EEPROM one wire bus idle error.\n\r");
					ret = -1;
				}
				if(reg.Sepe.OW_slave_present == 0)
				{
					printf("EEPROM read error: EEPROM isn't present.\n\r");
					ret = -1;
				}					
			}
		}while((reg.Sepe.EEPROM_ECS_rw_finished == 0) && ((d_t - c_t) < 2));
	}
	return ret;
}



//功能：读取EEPROM ID
//返回值：64位ID
//INT64U EDAQ_EEPROM_ID_read(unsigned short nMCB)
//{
//	INT16U data[4];
//	INT64U rd_ID = 0;
//	INT8U ret,i;

//	ret = EDAQ_EEPROM_data_read(0x78, 4, data, nMCB);
//	for(i=0;i<4;i++)
//		rd_ID += (INT64U) data[i]<<(16*i);
	
//	return rd_ID;
//}


INT8U EDAQ_EEPROM_ID_read(unsigned short nMCB, INT16U *pIDData)
{
    INT8U ret = 0;
    ret = EDAQ_EEPROM_data_read(0x78, 4, pIDData, nMCB);
    return ret;
}


//功能：打印EEPROM ID
//返回值：无
void EDAQ_Board_ID_dump(unsigned short nMCB)
{
    INT8U ret = 0;
    INT16U Int16PIDdata[64] = { 0 };
    ret = EDAQ_EEPROM_ID_read(nMCB, Int16PIDdata);
    printf("Board ID: 0x%04X%04X%04X%04X\n\r",Int16PIDdata[3], Int16PIDdata[2], Int16PIDdata[1], Int16PIDdata[0]);

//	INT64U rd_ID;

//	rd_ID = EDAQ_EEPROM_ID_read(nMCB);
//	printf("Board ID	     : 0x%016llX\n\r",rd_ID);
}



//功能：向EEPROM写入MAC_addr
//返回值：无 改为
//返回值：
//	0: no error
// -1: error.

INT8U EDAQ_EEPROM_MAC_addr_write(INT16U *wrdata, unsigned short nMCB)
{
    INT16U data[4];
    INT8U i;
    INT8U ret = 0;
    //
    for(i=0;i<3;i++)
        data[i] = wrdata[i];//(wrdata >> (16*i)) & 0xFFFF;

    ret = EDAQ_EEPROM_data_write(0x38, 3, data, nMCB);
    //
    return ret;
}

//INT8U EDAQ_EEPROM_MAC_addr_write(INT64U wrdata, unsigned short nMCB)
//{
//	INT16U data[4];
//	INT8U i;
//	INT8U ret = 0;
//	//
//	for(i=0;i<3;i++)
//        data[i] = (wrdata >> (16*i)) & 0xFFFF;

//	ret = EDAQ_EEPROM_data_write(0x38, 3, data, nMCB);
//	//
//	return ret;
//}

//功能：读取EEPROM中存储的MAC地址
//返回值：48位MAC_addr
//INT64U EDAQ_EEPROM_MAC_addr_read(unsigned short nMCB)
//{
//	INT16U data[4];
//	INT64U rd_data = 0;
//	INT8U ret,i;

//	ret = EDAQ_EEPROM_data_read(0x38, 3, data, nMCB);
//	for(i=0;i<3;i++)
//		rd_data += (INT64U)data[i]<<(16*i);
	
//	return rd_data;
//}

//功能：读取EEPROM中存储的MAC地址
//返回值：48位MAC_addr
INT8U EDAQ_EEPROM_MAC_addr_read(unsigned short nMCB, INT16U *RdData)
{
    INT8U ret;
    ret = EDAQ_EEPROM_data_read(0x38, 3, RdData, nMCB);
    return ret;
}

//功能：打印EEPROM中存储的MAC地址
//返回值：无
void EDAQ_EEPROM_MAC_addr_dump(unsigned short nMCB)
{
	INT64U rd_data;
    INT16U nRdMAC_addr[4] = { 0 };
    rd_data = EDAQ_EEPROM_MAC_addr_read(nMCB, nRdMAC_addr);
    printf("EEPROM MAC address   : 0x%04X%04X%04X\n\r",nRdMAC_addr[2], nRdMAC_addr[1], nRdMAC_addr[0]);
}

//功能：向EEPROM写入IPv4_addr
//返回值：无
INT8U EDAQ_EEPROM_IPv4_addr_write(INT32U wrdata, unsigned short nMCB)
{
	INT16U data[4];
	INT8U i;
	INT8U ret = 0;

	for(i=0;i<2;i++)
		data[i] = (wrdata >> (16*i)) & 0xFFFF;

	ret = EDAQ_EEPROM_data_write(0x40, 2, data, nMCB);	
	//
	return ret;
}

//功能：读取EEPROM中存储的IPv4地址
//返回值：32位IPv4_addr
INT32U EDAQ_EEPROM_IPv4_addr_read(unsigned short nMCB)
{
	INT16U data[4];
	INT32U rd_data = 0;
	INT8U ret,i;

	ret = EDAQ_EEPROM_data_read(0x40, 2, data, nMCB);
	for(i=0;i<2;i++)
		rd_data += (INT32U)data[i]<<(16*i);
	
	return rd_data;
}

//功能：打印EEPROM中存储的IPv4地址
//返回值：无
void EDAQ_EEPROM_IPv4_addr_dump(unsigned short nMCB)
{
	INT32U rd_data;

	rd_data = EDAQ_EEPROM_IPv4_addr_read(nMCB);
	printf("EEPROM IPv4 address  : 0x%08X\n\r",rd_data);
}

//功能：向EEPROM写入Gateway地址
//返回值：
//	0: no error
// -1: error.
INT8U EDAQ_EEPROM_Gateway_addr_write(INT32U wrdata, unsigned short nMCB)
{
	INT16U data[4];
	INT8U i;
	INT8U ret = 0;
	for(i=0;i<2;i++)
		data[i] = (wrdata >> (16*i)) & 0xFFFF;

	ret = EDAQ_EEPROM_data_write(0x30, 2, data, nMCB);	
	//
	return ret;
}

//功能：读取EEPROM中存储的Gateway地址
//返回值：32位Gateway_addr
INT32U EDAQ_EEPROM_Gateway_addr_read(unsigned short nMCB)
{
	INT16U data[4];
	INT32U rd_data = 0;
	INT8U ret,i;

	ret = EDAQ_EEPROM_data_read(0x30, 2, data, nMCB);
	for(i=0;i<2;i++)
		rd_data += (INT32U)data[i]<<(16*i);
	
	return rd_data;
}

//功能：打印EEPROM中存储的Gateway地址
//返回值：无
void EDAQ_EEPROM_Gateway_addr_dump(unsigned short nMCB)
{
	INT32U rd_data;

	rd_data = EDAQ_EEPROM_Gateway_addr_read(nMCB);
	printf("EEPROM Gateway IP addr:0x%08X\n\r",rd_data);
}

//功能：向EEPROM写入子网掩码
//返回值：无
INT8U EDAQ_EEPROM_subnet_mask_write(INT32U wrdata, unsigned short nMCB)
{
	INT16U data[4];
	INT8U i;
	INT8U ret = 0;

	for(i=0;i<2;i++)
		data[i] = (wrdata >> (16*i)) & 0xFFFF;

	ret = EDAQ_EEPROM_data_write(0x34, 2, data, nMCB);	
	//
	return ret;
}

//功能：读取EEPROM中存储的子网掩码
//返回值：32位subnet_mask
INT32U EDAQ_EEPROM_subnet_mask_read(unsigned short nMCB)
{
	INT16U data[4];
	INT32U rd_data = 0;
	INT8U ret,i;

	ret = EDAQ_EEPROM_data_read(0x34, 2, data, nMCB);
	for(i=0;i<2;i++)
		rd_data += (INT32U)data[i]<<(16*i);
	
	return rd_data;
}

//功能：打印EEPROM中存储的子网掩码
//返回值：无
void EDAQ_EEPROM_subnet_mask_dump(unsigned short nMCB)
{
	INT32U rd_data;

	rd_data = EDAQ_EEPROM_subnet_mask_read(nMCB);
	printf("EEPROM SUBNET MASK   : 0x%08X\n\r",rd_data);
}

//功能：向EEPROM写入ECS TCP PORT
//返回值：
//	0: no error
// -1: error.
INT8U EDAQ_EEPROM_ECS_TCP_PORT_write(INT16U wrdata, unsigned short nMCB)
{
	INT8U ret = 0;
	ret = EDAQ_EEPROM_data_write(0x50, 1, &wrdata, nMCB);	
	return ret;
}

//功能：读取EEPROM中存储的ECS TCP PORT
//返回值：16位ECS_TCP_PORT
INT16U EDAQ_EEPROM_ECS_TCP_PORT_read(unsigned short nMCB)
{
	INT16U data;
	EDAQ_EEPROM_data_read(0x50, 1, &data, nMCB);
	return data;
}

//功能：打印EEPROM中存储的ECS TCP PORT
//返回值：无
void EDAQ_EEPROM_ECS_TCP_PORT_dump(unsigned short nMCB)
{
	INT16U rd_data;

	rd_data = EDAQ_EEPROM_ECS_TCP_PORT_read(nMCB);
	printf("EEPROM ECS TCP PORT  : 0x%04X\n\r",rd_data);
}

//功能：向EEPROM写入DATA TCP PORT
//返回值：
//	0: no error
// -1: error.
INT8U EDAQ_EEPROM_DATA_TCP_PORT_write(INT16U wrdata, unsigned short nMCB)
{
	return EDAQ_EEPROM_data_write(0x52, 1, &wrdata, nMCB);	
}

//功能：读取EEPROM中存储的DATA TCP PORT
//返回值：16位DATA_TCP_PORT
INT16U EDAQ_EEPROM_DATA_TCP_PORT_read(unsigned short nMCB)
{
	INT16U data;
	EDAQ_EEPROM_data_read(0x52, 1, &data, nMCB);
	return data;
}

//功能：打印EEPROM中存储的DATA TCP PORT
//返回值：无
void EDAQ_EEPROM_DATA_TCP_PORT_dump(unsigned short nMCB)
{
	INT16U rd_data;

	rd_data = EDAQ_EEPROM_DATA_TCP_PORT_read(nMCB);
	printf("EEPROM DATA TCP PORT : 0x%04X\n\r",rd_data);
}


//功能：请求MCB FPGA从EEPROM中读取关键数据，并检查加密数据是否匹配
void EDAQ_EEPROM_key_data_onboard_rdreq(unsigned short nMCB)
{	
	EDAQ_AUTO_RESET_REG reg;
	reg.All = 0;
	reg.Sepe.EEPROM_key_data_onboard_rdreq = 1;
	ECS_A32D32_write_confirmed(EDAQ_AUTO_RESET_REG_ADDR,reg.All, nMCB);
}



//功能：读取EEPROM中存储的板子生产时间
//返回值：32位production_time
INT32U EDAQ_production_time_read(unsigned short nMCB)
{
	INT16U data[4];
	INT32U rd_data = 0;
	INT8U ret,i;

	ret = EDAQ_EEPROM_data_read(0x64, 2, data, nMCB);
	for(i=0;i<2;i++)
		rd_data += (INT32U)data[i]<<(16*i);
	
	return rd_data;
}

//功能：打印EEPROM中存储的板子生产时间
//返回值：无
void EDAQ_production_time_dump(unsigned short nMCB)
{
	char time_str[30];
	time_t t;
	t = (time_t) EDAQ_production_time_read(nMCB);
	strftime(time_str, sizeof(time_str), "%Y/%m/%d %H:%M:%S",localtime(&t) ); 
	printf("Board production time: %s\n\r",time_str);
}

//功能：打印EEPROM中存储的关键数据
//返回值：无
void EDAQ_EEPROM_Key_DATA_dump(unsigned short nMCB)
{
	EDAQ_EEPROM_MAC_addr_dump(nMCB);
	EDAQ_EEPROM_IPv4_addr_dump(nMCB);
	EDAQ_EEPROM_Gateway_addr_dump(nMCB);
	EDAQ_EEPROM_subnet_mask_dump(nMCB);
	EDAQ_EEPROM_ECS_TCP_PORT_dump(nMCB);
	EDAQ_EEPROM_DATA_TCP_PORT_dump(nMCB);
}

//--------------------------------------------------------------
//      DB FPGA CFG文件长度设置、读取、打印相关函数
//--------------------------------------------------------------

//功能：设置DB FPGA CFG文件长度
//参数：DB_FPGA_CFG_data_byte_num = DB FPGA CFG文件长度（单位：字节, 最大值：16M字节）
//返回值: 0 = No error, others = Find errors.
INT8U EDAQ_set_DB_FPGA_CFG_data_byte_num(INT32U DB_FPGA_CFG_data_byte_num, unsigned short nMCB)
{	
	INT8U ret = 0;
	ret = ECS_A32D32_write_confirmed(DB_FPGA_CFG_DATA_BYTE_NUM_REG_ADDR,(DB_FPGA_CFG_data_byte_num & 0xFFFFFF), nMCB);
	return ret;
}

//功能：读取DB FPGA CFG文件长度
//返回值：设置的DB FPGA CFG文件长度（单位：字节）
INT32U EDAQ_read_DB_FPGA_CFG_data_byte_num(unsigned short nMCB)
{	
	INT32U len;
	ECS_A32D32_read(DB_FPGA_CFG_DATA_BYTE_NUM_REG_ADDR,&len, nMCB);
	return (len & 0xFFFFFF);
}

//功能：打印DB FPGA CFG文件长度
void EDAQ_dump_DB_FPGA_CFG_data_byte_num(unsigned short nMCB)
{	
	printf("DB FPGA CFG data len has been set to %d bytes.\n\r",EDAQ_read_DB_FPGA_CFG_data_byte_num(nMCB));
}


//--------------------------------------------------------------
//      慢控制读写函数
//--------------------------------------------------------------
//功能：读取ECS命令包解析结果，如果不为0，说明ECS命令包有问题，会打印提示信息
//		由于该函数会读取下位机信息，根据慢控制读写数据流设计机制，这样可以确保之前的慢控制写操作已经完成。
//		如果在一个ECS写函数之后没有ECS读取函数，那么，就不能确保在执行下一个语句前，写操作能够完成。
//		如果在执行某句话之前要确保写操作已经完成，就必须执行该函数或其它ECS读函数。
INT8U EDAQ_check_ecs_err_to_ensure_ecs_wr_cmd_executed(INT32U* pdata,unsigned short nMCB)
{
	INT8U  ret = 0;
	EDAQ_GENERAL_STATUS_REG reg;
    //功能：从某个地址读取一个32bit数值
    //返回值：0=No error,others= Find errors.
    //注意： 该函数返回时，可以确保ESC读操作已经执行。
    //INT8U  ECS_A32D32_read(INT32U  addr,INT32U* pdata, unsigned short nMCB)
    ret = ECS_A32D32_read(EDAQ_GENERAL_STATUS_REG_ADDR,&reg.All, nMCB);
    //
    *pdata = reg.All;
    //
	return ret;
}

//功能：向某个地址写入一个32bit数值
//返回值: 0 = No error, others = Find errors.
//注意：该函数只是把基于TCP包的ECS写命令包写入到计算机发送缓存中。函数返回时，不能确保写入操作已经完成。
//		但TCP的可靠性传输可以保证在之后的某一刻，该ECS写命令包会被发送到目标板。
//		如果想利用ECS向目标板快速连续发送大量数据，可以调用该函数。在调用完所有函数，可以调用EDAQ_check_ecs_err_to_ensure_ecs_wr_cmd_executed()来确保写操作已经全部完成。
INT8U ECS_A32D32_write(INT32U  addr, INT32U  data, unsigned short nMCB)
{
     INT8U  buf[4],ret;
	 buf[0] = (data&0xFF000000)>>24; //dataH
	 buf[1] = (data&0x00FF0000)>>16; //
	 buf[2] = (data&0x0000FF00)>>8;  //
	 buf[3] = (data&0x000000FF)   ;  //dataL
     ret = ETH_CTRL_write(addr,buf,4, nMCB);
	 return ret;
}

//功能：向某个地址写入一个32bit数值
//返回值: 0 = No error, others = Find errors.
//注意：该函数返回时，可以确保ECS写操作已经执行。
INT8U ECS_A32D32_write_confirmed(INT32U  addr, INT32U  data, unsigned short nMCB)
{	
	INT8U ret;
    ret = ECS_A32D32_write(addr, data, nMCB);
    EDAQ_GENERAL_STATUS_REG reg;
	if( 0 == ret)
	{
        ret = EDAQ_check_ecs_err_to_ensure_ecs_wr_cmd_executed(&reg.All,nMCB); //可以确保之前的ECS写操作完成，并能够检查写操作是否成功完成。
	}
	return ret;
}

//功能：从某个地址读取一个32bit数值
//返回值: 0 = No error, others = Find errors.
//注意：该函数返回时，可以确保ECS读操作已经执行。
INT8U  ECS_A32D32_read(INT32U  addr,INT32U* pdata, unsigned short nMCB)
{
     INT8U  buf[8];
	 INT8U  ret;
	//重复执行次数
	INT8U  RePrcCnt = 0;
	do{
	     ret = ETH_CTRL_read(addr,buf,4, nMCB);
		/////////////////////////////
		RePrcCnt += 1;
		if(0 != ret)
		{

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            Sleep(10);
#else
           // usleep(10*1000);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

#endif
		}
	}while((0 != ret) && (RePrcCnt < gCmdReptN));
	//
     *pdata = (buf[0]<<24) + (buf[1]<<16) + (buf[2]<<8) + buf[3];
     return(ret);
}
//--------------------------------------------------------------
//      ECS test function
//--------------------------------------------------------------
void EDAQ_ECS_test(unsigned short nMCB)
{
	INT32U reg_data;
	int i,test_err=0;
	INT32U check_data[6]={0x0,0xFFFFFFFF,0x55555555,0xAAAAAAAA,0x55555555,0x13572468};
	/*
	ECS_A32D32_read(EDAQ_ECS_TEST_REG_ADDR,&reg_data);
	if(reg_data != 0x1234)
	{
		test_err=1;
		printf("ECS_TEST_REG default value: 0x%X (Should be 0x1234)\n\r",reg_data);
	}
	*/
	
	for(i=0;i<6;i++)
	{
		ECS_A32D32_write(EDAQ_ECS_TEST_REG_ADDR,check_data[i], nMCB);
		ECS_A32D32_read(EDAQ_ECS_TEST_REG_ADDR,&reg_data, nMCB);
		if(reg_data != check_data[i])
		{
			test_err=1;
			printf("ECS_TEST_REG write value: 0x%X, read value: 0x%X\n",check_data[i],reg_data);
		}
	}
	if (test_err == 1)
		printf("ECS bus test failed!!!\n");
	else
		printf("ECS bus test passed!\n");
	
}

//--------------------------------------------------------------
//     寄存器数值打印函数
//--------------------------------------------------------------
//功能：打印所有寄存器的值
void EDAQ_ECS_reg_all_dump(unsigned short nMCB)
{
	int i;
	INT32U data;
	for(i=0;i<EDAQ_REG_NUM;i++) 
	{
		ECS_A32D32_read(EDAQ_REG_BADDR+i*4,&data, nMCB);
		printf("CB Reg[%d]: %s is 0x%08X.\n",i,EDAQ_reg_name_list[i],data, nMCB);
	}
}



//功能：打印所有寄存器的详细值
void EDAQ_ECS_reg_all_dump_detailed(unsigned short nMCB)
{
	
	EDAQ_read_and_dump_firmware_version(nMCB);

	EDAQ_Board_ID_dump(nMCB);
	
	EDAQ_production_time_dump(nMCB);

	EDAQ_read_and_dump_DB_CLK_FREQ(nMCB);

	EDAQ_DB_FPGA_cfg_enable_dump(nMCB);

	//打印DB FPGA CFG文件长度
	EDAQ_dump_DB_FPGA_CFG_data_byte_num(nMCB);

	//DAQ mode enable
	EDAQ_daq_mode_enable_dump(nMCB);

	//RB working mode
	EDAQ_RB_working_mode_dump(nMCB);

	//DB daq req sel
	EDAQ_daq_req_sel_dump(nMCB);


	//DAQ cmd execute time set
	EDAQ_daq_cmd_exct_time_dump(nMCB);

	//non-DAQ cmd execute time set
	EDAQ_non_daq_cmd_exct_time_dump(nMCB);

	//EDAQ DB integration time set
	EDAQ_DB_integration_time_dump(nMCB);

	//Internal trigger setting
	internal_trigger_period_dump(nMCB);
	internal_trigger_pulse_width_dump(nMCB);

	//
	DDI_latch_latency_dump(nMCB);

	//RB write port set
	EDAQ_RB_wr_port_dump(nMCB);

	//RB read port set
	EDAQ_RB_rd_port_dump(nMCB);
	
	//EDAQ FIFO error status dump
	EDAQ_FIFO_error_status_dump(nMCB);

	//DB fpga config finished flag read
	EDAQ_DB_FPGA_cfg_finished_flag_dump(nMCB);

	//DB communication cmd processing busy flag read
	EDAQ_DB_comm_cmd_process_busy_flag_dump(nMCB);

	//DB RB almost full flag read
	EDAQ_RB_almost_full_flag_dump(nMCB);

	EDAQ_HMCB_work_mode_dump(nMCB);

	EDAQ_DB_line_num_dump(nMCB);

	EDAQ_DB_num_setting_dump(nMCB);

	EDAQ_read_and_dump_parsed_DB_num(nMCB);
}


//---------------------------------------------------------
//      DB参数配置函数
//---------------------------------------------------------
//功能：把参数写入到参数配置RAM中
void EDAQ_DB_prm_cfg_RAM_write(INT8U db_line_index, INT8U* p_prm, INT8U prm_len, unsigned short nMCB) 
{
	INT8U i;
	INT32U wr_val;
	for(i=0;i<prm_len;i++) 
	{
		wr_val = p_prm[i];
		ECS_A32D32_write(DB_LINE0_PRM_CFG_RAM_BADDR+db_line_index*4096+i*4,wr_val, nMCB);	
	}
}
//功能：读取参数配置RAM
void EDAQ_DB_prm_cfg_RAM_read(INT8U db_line_index, INT8U* p_prm, INT8U prm_len, unsigned short nMCB) 
{
	INT8U i;
	INT32U rd_val;
	for(i=0;i<prm_len;i++) 
	{
		ECS_A32D32_read(DB_LINE0_PRM_CFG_RAM_BADDR+db_line_index*4096+i*4,&rd_val, nMCB);
		p_prm[i] = rd_val&0xFF;	
	}
}
//功能：把参数写入到参数配置RAM中，并检查写入是否正确
INT8U EDAQ_DB_prm_cfg_RAM_write_with_check(INT8U db_line_index, INT8U* p_prm, INT8U prm_len, unsigned short nMCB) 
{
	INT8U i,err;
	INT8U rd_prm[256] = { 0 };
	//重复执行次数
	INT8U  RePrcCnt = 0;
	//	
	do
	{
        usleep(10*1000);
		memset(rd_prm, 0, sizeof(rd_prm));
        EDAQ_DB_prm_cfg_RAM_write(db_line_index, p_prm, prm_len, nMCB);
        usleep(20*1000);
        EDAQ_DB_prm_cfg_RAM_read(db_line_index, rd_prm, prm_len, nMCB);
		//
		err = 0;
		for(i=0;i<prm_len;i++) 
		{
			if (p_prm[i] != rd_prm[i])
			{
				printf("DB-Line%d DB_prm_cfg_RAM Accessing Error: Address[%d], Write value [0x%02X], Read value [0x%02X]\n",db_line_index,i,p_prm[i],rd_prm[i]);
				err = 1;
			}
		}
		if (err == 0)
			printf("DB-Line%d DB_prm_cfg_RAM has been configured successfully!\n",db_line_index);
		/////////////////////////////////////////////////////////////////
		RePrcCnt++;
	}while((err != 0) && (RePrcCnt < gCmdReptN));

	return err;
}

INT8U EDAQ_DB_prm_cfg_RAM_write_with_check(INT8U db_line_index, INT8U *w_prm, INT8U *ret_prm, INT8U prm_len, unsigned short nMCB)
{
    INT8U i,err;
    INT8U rd_prm[256] = { 0 };
    //重复执行次数
    INT8U  RePrcCnt = 0;
    //
    do
    {
        usleep(10*1000);
        memset(rd_prm, 0, sizeof(rd_prm));
        EDAQ_DB_prm_cfg_RAM_write(db_line_index, w_prm, prm_len, nMCB);
        usleep(20*1000);
        EDAQ_DB_prm_cfg_RAM_read(db_line_index, rd_prm, prm_len, nMCB);
        //
        err = 0;
        for(i = 0;i < prm_len; i++)
        {
            if (w_prm[i] != rd_prm[i])
            {
                printf("DB-Line%d DB_prm_cfg_RAM Accessing Error: Address[%d], Write value [0x%02X], Read value [0x%02X]\n",db_line_index,i,w_prm[i],rd_prm[i]);
                err = 1;
            }
            ret_prm[i] = rd_prm[i];
        }
        if (err == 0)
            printf("DB-Line%d DB_prm_cfg_RAM has been configured successfully!\n",db_line_index);
        /////////////////////////////////////////////////////////////////
        RePrcCnt++;
    }while((err != 0) && (RePrcCnt < gCmdReptN));

    return err;
}


//功能：打印参数配置RAM
void EDAQ_DB_prm_cfg_RAM_read_and_dump(INT8U db_line_index,INT8U prm_len, unsigned short nMCB) 
{
	INT8U i;
	INT8U rd_prm[256];
	EDAQ_DB_prm_cfg_RAM_read(db_line_index,rd_prm, prm_len, nMCB);

	for(i=0;i<prm_len;i++) 
	{
		printf("DB-Line%d DB_prm_cfg_RAM: Address[%d], Read value [0x%02X]\n",db_line_index,i,rd_prm[i]);
	}
}

//功能：测试参数配置RAM
void EDAQ_DB_prm_cfg_RAM_test(unsigned short nMCB)
{
	INT8U i,dbline_i;
	INT8U wr_prm[256];
	INT8U prm_len = 64;
	for(dbline_i = 0;dbline_i<4;dbline_i++)
	{
		for(i=0;i<prm_len;i++) 
		{
			//wr_prm[i] = 0x40*dbline_i+i;
			wr_prm[i] |= 0x10;//20bit
			//wr_prm[i] &= 0xffffffef;//16bit
		}
		EDAQ_DB_prm_cfg_RAM_write_with_check(dbline_i,wr_prm, prm_len, nMCB);
	}
	//EDAQ_DB_prm_cfg_RAM_read_and_dump(prm_len); 
}




