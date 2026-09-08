#ifndef CB_BSP_H
#define CB_BSP_H

#include "type_def.h"


//----------------------------------------------------------
//收包率，每收到一次数据包调用一次本函数。
//----------------------------------------------------------
void Pkg_Rate_Calcution_called_per_pkg(void);

//----------------------------------------------------------
//带宽计算，每收到一次数据调用一次本函数。
//----------------------------------------------------------
void band_width_calcution(INT32U bytes);

//----------------------------------------------------------
//     Delay function
//----------------------------------------------------------
//参数说明：
//	second_n 指定延时的时间长度，单位秒
void delay_in_second(INT32U second_n);
//参数说明：
//	milliseconds 指定延时的时间长度，单位毫秒
void delay_in_millisecond(INT32U millisecond_n);

//--------------------------------------------------------------
//      全局数据读取缓存区初始化函数
//--------------------------------------------------------------
//返回值：0=初始化成功；其它值=初始化失败
void gbl_data_buf_init(void);


//Read data
//parameter:
//	timeout:	 最长等待时间。为0表示最长等待时间无穷大，其它值表示相应的最长等待时间，单位S
//	fp_raw_data: 读取文件指针。为0的话，从以太网接收数据；非0的话，从该文件读取数据。
//return：
//  当完成指定长度的数据读取或发生超时，函数返回。此时，返回值等于参数len。
//  返回-1，表示函数执行出错。

INT32S Common_Data_Read( INT8U * rd_buf, INT32S len, INT32U timeout, FILE* fp_raw_data);


//--------------------------------------------------------------
//      读取从以太网接收的数据，并解析1个数据包
//--------------------------------------------------------------
//Parameters:
//	cmd_code: 0 = Don't check the package type. Others = Check the package type. 主要是用于判断读取的数据包是否是指定命令执行时预期返回的数据包。
//  cmd_mismatched_pkg_skip_en:  1 = 如果SC包头的cmd不符，会跳过本数据包检查下一个数据包，直到发现cmd匹配的数据包，并完成数据包解析。
//	fp_parse_data_w:	Point to the file that records the parsed data package.
//	fp_raw_data_r:	Point to the file that saves the raw data. 该指针为0的话，从以太网读取数据，否则，从该指针指向的文件读取数据。
//Return value: 0 = No error; 
//				-1= 发生以太网数据读取错误，或者，读完了指定的RAW data file。
//				其它值 = 数据解析出错。
//注意：本函数会读取所有DB LINE的数据，写入缓存，但只会解析特定DB line的一个数据包
INT8S Read_and_parse_one_SC_pkg(INT8U cmd_code, INT8U cmd_mismatched_pkg_skip_en, FILE* fp_parse_data_w, FILE* fp_raw_data_r);


//--------------------------------------------------------------------------------------
//      Read and parse a certain number of SC packge received from Ethernet
//--------------------------------------------------------------------------------------
//The parsed data package will be  
//Parameters:
//	pkg_num:  0 = Read forever. Others = The number of packages to be read. 使能多个DB Line时，读取一个数据包是指每个DB Line都读取一个数据包。
//	cmd_code: 0 = Don't check the package type. Others = Check the package type. 主要是用于判断读取的数据包是否是指定命令执行时预期返回的数据包。
//  cmd_mismatched_pkg_skip_en:  1 = 如果SC包头的cmd不符，会跳过本数据包检查下一个数据包，直到发现cmd匹配的数据包，并完成数据包解析。
//  cmd_exct_result_report_en: 1=Report the command execution result.
//  pkg_rate_cal_en: 1= Permit to calculate the package rate.
//	fp_parse_data_w:	Point to the file that records the parsed data package.
//	fp_raw_data_r:	Point to the file that saves the raw data. 该指针为0的话，从以太网读取数据，否则，从该指针指向的文件读取数据。
//Return value: 0 = No error; 
//				-1= Ethernet read error
//         Others = Errors is found.

INT8S ECM_read_and_parse_data(INT32U pkg_num, INT8U cmd_code, INT8U cmd_mismatched_pkg_skip_en, INT8U cmd_exct_result_report_en, INT8U pkg_rate_cal_en, FILE* fp_parse_data_w, FILE* fp_raw_data_r);


//--------------------------------------------------------------
//      读取从以太网接收的数据，并解析指定DB Line的1个数据包
//--------------------------------------------------------------
//Parameters:
//	DB_line_i: The index of DB Line. 实际解析的DB Line，可能不是指定的DB Line。SC DB Line打印时会提示是否相符。
//	cmd_code: 0 = Don't check the package type. Others = Check the package type. 主要是用于判断读取的数据包是否是指定命令执行时预期返回的数据包。
//  cmd_mismatched_pkg_skip_en:  1 = 如果SC包头的cmd不符，会跳过本数据包检查下一个数据包，直到发现cmd匹配的数据包，并完成数据包解析。
//	fp:		  Point to the file that records the parsed read data package.
//Return value: 0 = No error; 
//         Others = Errors is found.
//注意：本函数会读取所有DB LINE的数据，写入缓存，但只会解析特定DB line的一个数据包

//INT8U ETH_read_and_parse_one_SC_pkg_from_specified_DB_line(INT8U DB_line_i,INT8U DB_line_num,INT8U cmd_code, INT8U cmd_mismatched_pkg_skip_en, FILE* fp);
//
//INT8U ETH_read_and_parse_one_SC_pkg_from_specified_DB_line(INT8U DB_line_i,INT8U DB_line_num,INT8U cmd_code, INT8U cmd_mismatched_pkg_skip_en, OneDBLineOnePackage* pDBDataPackage, FILE* fp);

//INT8U ETH_read_and_parse_one_SC_pkg_from_specified_DB_line(INT8U DB_line_i,INT8U DB_line_num,INT8U cmd_code, INT8U cmd_mismatched_pkg_skip_en, OneDBLineOnePackage* pDBDataPackage, FILE* fp_parse_data_w = NULL, FILE* fp_raw_data_r = NULL);

//DB或CB数据包解析标识
#define CB_PKG_PARSE_SEL		0
#define DB_PKG_PARSE_SEL		1
#define UNKNOWN_PKG_PARSE_SEL   2

//解包状态机的状态列表
#define PM_START					0
#define PM_GET_SC_PKG_LEAD_CODE     1
#define PM_GET_SC_PKG_HEADER        2  
#define PM_GET_SC_PKG_DATA_LOAD     3  
#define PM_GET_SUB_PKG_HEADER   	4 
#define PM_GET_XB_DATA				5  
#define PM_GET_CRC_CODE				6  
#define PM_END_SUCCESS				7  


//----------------------------------------------------------
//   Read parse and dump data into an auto generated file 
//----------------------------------------------------------
//Parameters:
//	pkg_num:  0 = Read forever. Others = The number of packages to be read.
//	cmd_code: 0 = Don't check the package type. Others = Check the package type. 主要是用于判断读取的数据包是否是指定命令执行时预期返回的数据包。
//	fp_raw_data_r:	Point to the file that saves the raw data. 该指针为0的话，从以太网读取数据，否则，从该指针指向的文件读取数据。
void ECM_read_parse_dump_data_into_auto_generated_file(INT32U pkg_num, INT8U cmd_code, FILE* fp_raw_data_r);



//数据包类型
#define DB_DAQ_PKG		0x08   //10001000: 数据采集命令正常执行返回的数据
#define DB_RST_PKG		0x01   //00000001：复位命令正常执行返回的数据
#define DB_LCT_PKG		0x02   //00000010：定位命令正常执行返回的数据
#define DB_PRM_CFG_PKG	0x05   //00000101：配置参数命令正常执行返回的数据
#define DB_ERR_PKG			0xE0   //11110000：命令校验或执行出错时，返回接收到的CMD包
#define CB_PKG				0xC0   //CB数据包

//把命令码转换命令字符串
void conv_cmd_code_to_string(INT8U cmd_code, char* cmd_str);


//----------------------------------------------------------
//      Create a log file to record the non-daq command returned package
//----------------------------------------------------------
FILE* cmd_pkg_parse_log_file_create(void);




//-----------------------------------
//      ETH data read link test function
//-----------------------------------

//Parameter:
//			test_data_n: Assign the number of data comparation test to be done.
//          return value: The number of errors that have been found.
			
INT32S ETH_count_data_read_link_test(INT32S test_data_n);


//----------------------------------------------------
//      Long time ETH data read test function
//----------------------------------------------------

//Parameter:
//			total_tst_time_in_hour: Total test time in hours.
//          return value: 0= No test error; 1= Found test error.
			
INT8U ETH_count_data_read_test_for_long_time(float total_tst_time_in_hour);

//----------------------------------------------------------
//      ETH data read and dump into a specified file 
//----------------------------------------------------------
//Parameters:
//	byte_n: 0 = Read forever. Others = The number of bytes to be read.
//  BWC_en: 1= Permit to calculate the band width.
//  fp: 0= Don't write file. 

void ETH_data_read_and_dump_into_file(INT32U byte_n, INT8U BWC_en, FILE* fp);


//----------------------------------------------------------
//      ETH data read and dump into a auto generated file 
//----------------------------------------------------------
//Parameters:
//	byte_n: 0 = Read forever. Others = The number of bytes to be read.
//  BWC_en: 1= Permit to calculate the band width.
//返回值：无
void ETH_data_read_and_dump_into_auto_generated_file(char * data_file_name, INT32U byte_n, INT8U BWC_en);



#endif