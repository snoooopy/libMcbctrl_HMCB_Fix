#ifndef ECS_FUNC_H
#define ECS_FUNC_H

#include "type_def.h"

//--------------------------------------------------------------
//     通用函数
//--------------------------------------------------------------
void loop_delay(void);


//--------------------------------------------------------------
//   CB通用复位函数
//--------------------------------------------------------------
//功能：发出CB FPGA逻辑复位请求
//参数：无
//返回值：0 = No error, others = Find errors.
INT8U EDAQ_CB_logic_reset(unsigned short nMCB);


//--------------------------------------------------------------
//    固件信息读取函数
//--------------------------------------------------------------
//功能：打印CB FPGA固件版本号和生成时间
//参数：无
//返回值：无
void EDAQ_read_and_dump_firmware_version(void);

//--------------------------------------------------------------
//     CB通用状态读取函数
//--------------------------------------------------------------
//功能：读取CB FPGA FIFO错误
//参数：无
//返回值：FIFO错误
INT32U EDAQ_FIFO_error_status_rd(unsigned short nMCB);


//功能：打印CB FPGA FIFO错误
//参数：无
//返回值：无
void EDAQ_FIFO_error_status_dump(unsigned short nMCB);


//--------------------------------------------------------------
//     DB FPGA 程序烧写相关函数
//--------------------------------------------------------------

//功能：读取DB FPGA程序烧写完成标志
//参数：无
//返回值：DB FPGA程序烧写完成标志
INT16U EDAQ_DB_FPGA_cfg_finished_flag_rd(unsigned short nMCB);


//功能：打印DB FPGA程序烧写完成标志
//参数：无
//返回值：无
void EDAQ_DB_FPGA_cfg_finished_flag_dump(unsigned short nMCB);


//功能：发出DB FPGA程序烧写请求
//参数：无
//返回值：无
void EDAQ_cfg_fpga_req(unsigned short nMCB);

//功能：使能DB FPGA程序烧写信号
//参数：
//	enable = 1: The RS, HS are used to config DB fpga. 
//			 0: The RS, HS are used to control DB.
//返回值：无
void EDAQ_DB_FPGA_cfg_enable(INT32U enable, unsigned short nMCB);

//功能：读取DB FPGA程序烧写使能信号
//参数：无		
//返回值：
//	1: The RS, HS are used to config DB fpga.
//  0: The RS, HS are used to control DB.
INT16U EDAQ_DB_FPGA_cfg_enable_rd(unsigned short nMCB);

//功能：打印DB FPGA程序烧写使能信号
//参数：无		
//返回值：无
void EDAQ_DB_FPGA_cfg_enable_dump(unsigned short nMCB);

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
INT8U EDAQ_DB_Line_FPGA_program_enable(INT32U enable_x4, unsigned short nMCB);

//功能：写入DB FPGA烧写程序
//返回值: 0 = No error, others = Find errors.
INT8U EDAQ_DB_fpga_cfg_data_wr_via_ECS(INT32U data, unsigned short nMCB) ;

//功能：设置RB缓存工作模式
//参数：
//	Mode =	1: CMD mode. DB数据缓存采用该模式。
//			0: Auto mode. 测试数据和DB FPGA下载程序的缓存采用该模式。
//返回值: 0 = No error, others = Find errors.
INT8U EDAQ_RB_working_mode_set(INT32U mode, unsigned short nMCB);

//功能：读取并打印RB缓存工作模式
//参数：无
//返回值: 0 = No error, others = Find errors.
INT8U EDAQ_RB_working_mode_dump(unsigned short nMCB);


//功能：使能/除能SC data pkg monitor
//参数：
//	en=	1: Enable
//		0: Disable
//返回值: 0 = No error, others = Find errors.
INT8U EDAQ_SC_data_pkg_monitor_enable(INT32U en, unsigned short nMCB);


//功能：使能/除能TEST_DATA_generator
//参数：
//	en=	1: Enable
//		0: Disable
//返回值：0 = No error, others = Find errors.
INT8U  EDAQ_TEST_DATA_enable(INT32U en, unsigned short nMCB);

//--------------------------------------------------------------
//      DB专用控制与状态读取函数
//--------------------------------------------------------------

//功能：发出DB FPGA复位请求
//返回值：0 = No error, others = Find errors.
INT8U EDAQ_DB_reset_req(unsigned short nMCB);

//功能：发出DB定位请求
INT8U EDAQ_DB_locate_req(unsigned short nMCB);

//功能：发出DB参数配置请求
INT8U EDAQ_DB_prm_cfg_req(unsigned short nMCB);

//功能：发出DB DAQ命令传输请求
INT8U EDAQ_DB_daq_cmd_tx_req(unsigned short nMCB);


//功能：读取DB communication cmd processing busy flag
INT16U EDAQ_DB_comm_cmd_process_busy_flag_rd(unsigned short nMCB);


//功能：打印DB communication cmd processing busy flag
void EDAQ_DB_comm_cmd_process_busy_flag_dump(unsigned short nMCB);


//功能：一直等待，直到DB communication cmd processing busy flag清零
void EDAQ_wait_until_cmd_process_busy_flag_clr(unsigned short nMCB);


//功能：设置DB板子数目
//返回值：0 = No error, others = Find errors.
INT8U EDAQ_DB_num_set(INT8U DL0_DB_num,INT8U DL1_DB_num,INT8U DL2_DB_num,INT8U DL3_DB_num, unsigned short nMCB);

//功能：读取DB板子数目
INT32U EDAQ_DB_num_setting_rd(unsigned short nMCB);


//功能：打印DB板子数目
void EDAQ_DB_num_setting_dump(unsigned short nMCB);

void EDAQ_Ctrl_Reg_WriteValueInit();
//
INT8U EDAQ_daq_SetDBCommCtrlReg(unsigned short nMCB);
//功能：使能DAQ模式
//参数：
//	enable = 1: DAQ mode is enabled.
//           0: DAQ mode is disabled. 
//返回值：0 = No error, others = Find errors.
INT8U EDAQ_daq_mode_enable(INT32U enable, unsigned short nMCB) ;


//功能：读取DAQ模式
INT16U EDAQ_daq_mode_enable_rd(unsigned short nMCB);

//功能：打印DAQ模式
void EDAQ_daq_mode_enable_dump(unsigned short nMCB);


//功能：设置DAQ触发
INT8U EDAQ_daq_req_sel(INT32U sel, unsigned short nMCB) ;

//功能：读取DAQ触发
INT16U EDAQ_daq_req_sel_rd(unsigned short nMCB);

INT8U EDAQ_daq_req_sel_And_mode_enable(INT32U enable, INT32U sel, unsigned short nMCB) ;

//功能：打印DAQ触发
void EDAQ_daq_req_sel_dump(unsigned short nMCB);


//功能：设置DB Line数目
//参数：
//	DB_line_num: DB Line数目的设定值。注意：DB Line实际使能的数目受系统最大支持数目限制。
//返回值：0 = No error, others = Find errors.
INT8U EDAQ_DB_line_num_set(INT8U DB_line_num, unsigned short nMCB);

//功能：读取DB Line数目的设定值
INT8U EDAQ_DB_line_num_rd(unsigned short nMCB);

//功能：打印DB Line数目的设定值
void EDAQ_DB_line_num_dump(unsigned short nMCB);

//功能：设置MCB工作模式 
//参数:
// 1: Work as Master HMCB
// 0: Work as Cascaded HMCB
//返回值：0 = No error, others = Find errors.
INT8U EDAQ_HMCB_work_mode_set(INT32U master_en, unsigned short nMCB) ;

//功能：读取MCB工作模式 
INT16U EDAQ_HMCB_work_mode_rd(unsigned short nMCB);

//功能：打印MCB工作模式 
void EDAQ_HMCB_work_mode_dump(unsigned short nMCB);

//功能：设置DAQ命令执行时间 
//参数: 
//	t: 命令执行时间，单位uS.
void EDAQ_daq_cmd_exct_time_set(INT32U t, unsigned short nMCB);

//功能：读取DAQ命令执行时间 
INT32U EDAQ_daq_cmd_exct_time_rd(unsigned short nMCB);

//功能：打印DAQ命令执行时间 
void EDAQ_daq_cmd_exct_time_dump(unsigned short nMCB);

//功能：设置non-DAQ命令执行时间 
//参数: 
//	t: 命令执行时间，单位uS.
void EDAQ_non_daq_cmd_exct_time_set(INT32U t, unsigned short nMCB);

//功能：读取non-DAQ命令执行时间 
INT32U EDAQ_non_daq_cmd_exct_time_rd(unsigned short nMCB);

//功能：打印non-DAQ命令执行时间 
void EDAQ_non_daq_cmd_exct_time_dump(unsigned short nMCB);

//功能：设置DB积分时间
//参数: 
//	t: 积分时间，单位uS.
void EDAQ_DB_integration_time_set(INT32U t, unsigned short nMCB);

//功能：读取DB积分时间
INT32U EDAQ_DB_integration_time_rd(unsigned short nMCB);

//功能：打印DB积分时间
void EDAQ_DB_integration_time_dump(unsigned short nMCB);

//功能：设置内部触发周期
//参数: 
//	t: 触发周期，单位uS.
void internal_trigger_period_set(INT32U t, unsigned short nMCB);

//功能：读取内部触发周期
INT32U internal_trigger_period_rd(unsigned short nMCB);

//功能：打印内部触发周期
void internal_trigger_period_dump(unsigned short nMCB);

//功能：设置内部触发脉宽
//参数: 
//	t: 触发脉宽，单位uS. Max value is 65535 us
void internal_trigger_pulse_width_set(INT32U d, unsigned short nMCB);

//功能：读取内部触发脉宽
INT32U internal_trigger_pulse_width_rd(unsigned short nMCB);

//功能：打印内部触发脉宽
void internal_trigger_pulse_width_dump(unsigned short nMCB);

//功能：设置DDI锁存延迟时间
//参数: 
//	t: 延迟时间，单位uS. Max value is 65535 us
void DDI_latch_latency_set(INT32U d, unsigned short nMCB);

//功能：读取DDI锁存延迟时间
INT32U DDI_latch_latency_rd(unsigned short nMCB);

//功能：打印DDI锁存延迟时间
void DDI_latch_latency_dump(unsigned short nMCB);

//--------------------------------------------------------------
//     DB通讯 状态读取函数
//--------------------------------------------------------------
//功能：打印DB数据传输时钟
//参数：无
//返回值：无
void EDAQ_read_and_dump_DB_CLK_FREQ(unsigned short nMCB);


//功能：打印DB Line使能向量
//参数：无
//返回值：无
void EDAQ_read_and_dump_DB_line_enable_vector(unsigned short nMCB);

//功能：读取实际使能的DB Line数目
//参数：无
//返回值：实际使能的DB line数目：1，2，3，4
INT8U EDAQ_read_DB_line_enable_num(unsigned short nMCB);

//功能：打印实际使能的DB Line数目
//参数：无
//返回值：无
void EDAQ_read_and_dump_DB_line_enable_num(unsigned short nMCB);

//功能：读取当前模块支持的最大DB Line数目
//参数：无
//返回值：当前模块支持的最大DB Line数目
INT8U EDAQ_read_DB_line_max_num(unsigned short nMCB);


//功能：打印当前模块支持的最大DB Line数目
//参数：无
//返回值：无
void EDAQ_read_and_dump_DB_line_max_num(unsigned short nMCB);


//功能：打印对数据流解析得到的DB数目
//参数：无
//返回值：无
void EDAQ_read_and_dump_parsed_DB_num(unsigned short nMCB);

//--------------------------------------------------------------
//     Ring buffer control functions
//--------------------------------------------------------------
//功能：读取Ring buffer almost full flag
INT16U EDAQ_RB_almost_full_flag_rd(unsigned short nMCB);

//功能：打印Ring buffer almost full flag
void EDAQ_RB_almost_full_flag_dump(unsigned short nMCB);


//功能：复位Ring buffer的输入和输出缓存区
void EDAQ_RBxB_reset(unsigned short nMCB);


//功能：复位Ring buffer的输出缓存区
void EDAQ_RBOB_reset(unsigned short nMCB);


//功能：选择Ring buffer写入端口
//返回值: 0 = No error, others = Find errors.
INT8U EDAQ_RB_wr_port_sel(INT32U port, unsigned short nMCB) ;


//功能：读取Ring buffer写入端口
INT16U EDAQ_RB_wr_port_rd(unsigned short nMCB);


//功能：打印Ring buffer写入端口
void EDAQ_RB_wr_port_dump(unsigned short nMCB);


//功能：选择Ring buffer读取端口
//返回值: 0 = No error, others = Find errors.
INT8U EDAQ_RB_rd_port_sel(INT32U port, unsigned short nMCB) ;


//功能：读取Ring buffer读取端口
INT16U EDAQ_RB_rd_port_rd(unsigned short nMCB);


//功能：打印Ring buffer读取端口
void EDAQ_RB_rd_port_dump(unsigned short nMCB);



//-------------------------------------------------------------
//	EEPROM读写函数
//-------------------------------------------------------------

//功能：发出EEPROM_ECS_wr_req
void EDAQ_EEPROM_ECS_wr_req(unsigned short nMCB);

//功能：发出EEPROM_ECS_rd_req
void EDAQ_EEPROM_ECS_rd_req(unsigned short nMCB);


//功能：设置EEPROM CTRL register
void EDAQ_EEPROM_CTRL_reg_set(INT32U wr_data,INT32U rw_addr) ;

//功能：Write a certain number of data into EEPROM
//参数：
//	addr: 指定EEPROM起始读取地址。由于最小读取数据宽度为16bit，其最低位必须为0。
//	wr_len: 写入16位数的个数
//	p_data: 指向读取数据存储数组
//返回值：
//	0: no error
// -1: error.
INT8U EDAQ_EEPROM_data_write(INT8U addr, INT8U wr_len, INT16U *pdata, unsigned short nMCB);


//功能：Write a certain number of zero into EEPROM
//参数：
//	addr: 指定EEPROM起始读取地址。由于最小写入数据宽度为16bit，其最低位必须为0。
//	wr_len: 写入16位零的个数
//返回值：
//	0: no error
// -1: error.
INT8U EDAQ_EEPROM_write_zero(INT8U addr, INT8U wr_len, unsigned short nMCB);


//功能：Read a certain number of EEPROM data
//参数：
//	addr: 指定EEPROM起始读取地址。由于最小读取数据宽度为16bit，其最低位必须为0。
//	rd_len: 读取16位数的个数
//	p_data: 指向读取数据存储数组
//返回值：
//	0: no error
// -1: error.
INT8U EDAQ_EEPROM_data_read(INT8U addr, INT8U rd_len, INT16U *pdata, unsigned short nMCB);



//功能：读取EEPROM ID
//返回值：64位ID
//INT64U EDAQ_EEPROM_ID_read(unsigned short nMCB);

//功能：读取64位EEPROM ID
//	0: no error
// -1: error.
INT8U EDAQ_EEPROM_ID_read(unsigned short nMCB, INT16U *pIDData);
//功能：打印EEPROM ID
//返回值：无
void EDAQ_Board_ID_dump(void);

//功能：向EEPROM写入MAC_addr
//返回值：
//	0: no error
// -1: error.
//INT8U EDAQ_EEPROM_MAC_addr_write(INT64U wrdata, unsigned short nMCB);
INT8U EDAQ_EEPROM_MAC_addr_write(INT16U *wrdata, unsigned short nMCB);

//功能：读取EEPROM中存储的MAC地址
//返回值：48位MAC_addr
//INT64U EDAQ_EEPROM_MAC_addr_read(unsigned short nMCB);

//功能：读取EEPROM中存储的MAC地址
//返回参数值：48位MAC_addr
//返回值：
//	0: no error
// -1: error.
INT8U EDAQ_EEPROM_MAC_addr_read(unsigned short nMCB, INT16U *RdData);


//功能：打印EEPROM中存储的MAC地址
//返回值：无
void EDAQ_EEPROM_MAC_addr_dump(unsigned short nMCB);

//功能：向EEPROM写入IPv4_addr
//返回值：
//	0: no error
// -1: error.
INT8U EDAQ_EEPROM_IPv4_addr_write(INT32U wrdata, unsigned short nMCB);

//功能：读取EEPROM中存储的IPv4地址
//返回值：32位IPv4_addr
INT32U EDAQ_EEPROM_IPv4_addr_read(unsigned short nMCB);

//功能：打印EEPROM中存储的IPv4地址
//返回值：无
void EDAQ_EEPROM_IPv4_addr_dump(unsigned short nMCB);

//功能：向EEPROM写入Gateway地址
//返回值：
//	0: no error
// -1: error.
INT8U EDAQ_EEPROM_Gateway_addr_write(INT32U wrdata, unsigned short nMCB);

//功能：读取EEPROM中存储的Gateway地址
//返回值：32位Gateway_addr
INT32U EDAQ_EEPROM_Gateway_addr_read(unsigned short nMCB);

//功能：打印EEPROM中存储的Gateway地址
//返回值：无
void EDAQ_EEPROM_Gateway_addr_dump(unsigned short nMCB);

//功能：向EEPROM写入子网掩码
//返回值：
//	0: no error
// -1: error.
INT8U EDAQ_EEPROM_subnet_mask_write(INT32U wrdata, unsigned short nMCB);

//功能：读取EEPROM中存储的子网掩码
//返回值：32位subnet_mask
INT32U EDAQ_EEPROM_subnet_mask_read(unsigned short nMCB);

//功能：打印EEPROM中存储的子网掩码
//返回值：无
void EDAQ_EEPROM_subnet_mask_dump(unsigned short nMCB);

//功能：向EEPROM写入ECS TCP PORT
//返回值：
//	0: no error
// -1: error.
INT8U EDAQ_EEPROM_ECS_TCP_PORT_write(INT16U wrdata, unsigned short nMCB);

//功能：读取EEPROM中存储的ECS TCP PORT
//返回值：16位ECS_TCP_PORT
INT16U EDAQ_EEPROM_ECS_TCP_PORT_read(unsigned short nMCB);

//功能：打印EEPROM中存储的ECS TCP PORT
//返回值：无
void EDAQ_EEPROM_ECS_TCP_PORT_dump(unsigned short nMCB);

//功能：向EEPROM写入DATA TCP PORT
//返回值：
//	0: no error
// -1: error.
INT8U EDAQ_EEPROM_DATA_TCP_PORT_write(INT16U wrdata, unsigned short nMCB);

//功能：读取EEPROM中存储的DATA TCP PORT
//返回值：16位DATA_TCP_PORT
INT16U EDAQ_EEPROM_DATA_TCP_PORT_read(unsigned short nMCB);

//功能：打印EEPROM中存储的DATA TCP PORT
//返回值：无
void EDAQ_EEPROM_DATA_TCP_PORT_dump(unsigned short nMCB);

//功能：请求MCB FPGA从EEPROM中读取关键数据，并检查加密数据是否匹配
void EDAQ_EEPROM_key_data_onboard_rdreq(unsigned short nMCB);

//功能：读取EEPROM中存储的板子生产时间
//返回值：32位production_time
INT32U EDAQ_production_time_read(unsigned short nMCB);

//功能：打印EEPROM中存储的板子生产时间
//返回值：无
void EDAQ_production_time_dump(unsigned short nMCB);

//功能：打印EEPROM中存储的关键数据
//返回值：无
void EDAQ_EEPROM_Key_DATA_dump(unsigned short nMCB);


//--------------------------------------------------------------
//      DB FPGA CFG文件长度设置、读取、打印相关函数
//--------------------------------------------------------------

//功能：设置DB FPGA CFG文件长度
//参数：DB_FPGA_CFG_data_byte_num = DB FPGA CFG文件长度（单位：字节）
//返回值: 0 = No error, others = Find errors.
INT8U EDAQ_set_DB_FPGA_CFG_data_byte_num(INT32U DB_FPGA_CFG_data_byte_num, unsigned short nMCB);

//功能：读取DB FPGA CFG文件长度
//返回值：设置的DB FPGA CFG文件长度（单位：字节）
INT32U EDAQ_read_DB_FPGA_CFG_data_byte_num(unsigned short nMCB);

//功能：打印DB FPGA CFG文件长度
void EDAQ_dump_DB_FPGA_CFG_data_byte_num(unsigned short nMCB);




//--------------------------------------------------------------
//      慢控制读写函数
//--------------------------------------------------------------
//功能：读取ECS命令包解析结果，如果不为0，说明ECS命令包有问题，会打印提示信息
//		由于该函数会读取下位机信息，根据慢控制读写数据流设计机制，这样可以确保之前的慢控制写操作已经完成。
//		如果在一个ECS写函数之后没有ECS读取函数，那么，就不能确保在执行下一个语句前，写操作能够完成。
//		如果在执行某句话之前要确保写操作已经完成，就必须执行该函数或其它ECS读函数。
INT8U EDAQ_check_ecs_err_to_ensure_ecs_wr_cmd_executed(INT32U* pdata,unsigned short nMCB);


//功能：向某个地址写入一个32bit数值
//返回值: 0 = No error, others = Find errors.
//注意：该函数只是把基于TCP包的ECS写命令包写入到计算机发送缓存中。函数返回时，不能确保写入操作已经完成。
//		但TCP的可靠性传输可以保证在之后的某一刻，该ECS写命令包会被发送到目标板。
//		如果想利用ECS向目标板快速连续发送大量数据，可以调用该函数。在调用完所有函数，可以调用EDAQ_check_ecs_err_to_ensure_ecs_wr_cmd_executed()来确保写操作已经全部完成。
INT8U ECS_A32D32_write(INT32U  addr, INT32U  data, unsigned short nMCB);


//功能：向某个地址写入一个32bit数值
//返回值: 0 = No error, others = Find errors.
//注意：该函数返回时，可以确保ECS写操作已经执行。

INT8U ECS_A32D32_write_confirmed(INT32U  addr, INT32U  data, unsigned short nMCB);

//功能：从某个地址读取一个32bit数值
//返回值: 0 = No error, others = Find errors.
//注意：该函数返回时，可以确保ECS读操作已经执行。
INT8U  ECS_A32D32_read(INT32U  addr,INT32U* pdata, unsigned short nMCB);

//--------------------------------------------------------------
//      ECS test function
//--------------------------------------------------------------
void EDAQ_ECS_test(unsigned short nMCB);


//--------------------------------------------------------------
//     寄存器数值打印函数
//--------------------------------------------------------------
//功能：打印所有寄存器的值
void EDAQ_ECS_reg_all_dump(unsigned short nMCB);

//功能：打印所有寄存器的详细值
void EDAQ_ECS_reg_all_dump_detailed(unsigned short nMCB);


//---------------------------------------------------------
//      DB参数配置函数
//---------------------------------------------------------
//功能：把参数写入到参数配置RAM中
void EDAQ_DB_prm_cfg_RAM_write(INT8U db_line_index,INT8U* p_prm, INT8U prm_len, unsigned short nMCB);

//功能：读取参数配置RAM
void EDAQ_DB_prm_cfg_RAM_read(INT8U db_line_index,INT8U* p_prm, INT8U prm_len, unsigned short nMCB);

//功能：把参数写入到参数配置RAM中，并检查写入是否正确
INT8U EDAQ_DB_prm_cfg_RAM_write_with_check(INT8U db_line_index,INT8U* p_prm, INT8U prm_len, unsigned short nMCB) ;
INT8U EDAQ_DB_prm_cfg_RAM_write_with_check(INT8U db_line_index,INT8U* w_prm, INT8U* ret_prm,INT8U prm_len, unsigned short nMCB) ;

//功能：打印参数配置RAM
void EDAQ_DB_prm_cfg_RAM_read_and_dump(INT8U db_line_index,INT8U prm_len, unsigned short nMCB) ;

//功能：测试参数配置RAM
void EDAQ_DB_prm_cfg_RAM_test(unsigned short nMCB);


#endif

