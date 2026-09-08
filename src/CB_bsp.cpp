#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include "stdafx.h"
#else
#include "replenish_type.hpp"
#include "nuctool.h"
#include <map>
#include <vector>
#endif
#include "including.h"
 

//收包率，每收到一次数据包调用一次本函数。
time_t PRC_used_time = 0;
INT32U PRC_used_pkg_cnt = 0;
void Pkg_Rate_Calcution_called_per_pkg(void)
{
	time_t current_t;
	current_t = time(0);
	PRC_used_pkg_cnt++;
	if(current_t>PRC_used_time)
	{
		PRC_used_time = current_t;
		printf("package rate: %d\n",PRC_used_pkg_cnt);
		PRC_used_pkg_cnt = 0;
	}
}

//带宽计算，每收到一次数据调用一次本函数。
time_t BWC_used_time = 0;
INT32U BWC_used_byte_cnt = 0;
void band_width_calcution(INT32U bytes)
{
	time_t current_t;
	current_t = time(0);
	BWC_used_byte_cnt += bytes;
	if(current_t>BWC_used_time)
	{
		BWC_used_time = current_t;
		printf("band width: %d Mbps\n",BWC_used_byte_cnt*8/1000000);
		BWC_used_byte_cnt = 0;
	}

}

//----------------------------------------------------------
//     Delay function
//----------------------------------------------------------
//参数说明：
//	second_n 指定延时的时间长度，单位秒
void delay_in_second(INT32U second_n)
{
	INT32U s_t,c_t,d_t;
	s_t = (INT32U) time(0); 
	while(1)
	{
		c_t = (INT32U) time(0); 
		d_t = c_t - s_t;
		if (d_t > second_n)
			break;
	}

}
//----------------------------------------------------------
//参数说明：
//	milliseconds 指定延时的时间长度，单位毫秒
void delay_in_millisecond(INT32U millisecond_n)
{
    DWORD tTick1 = NucTool::GetTickCount();
    DWORD tTick2 = NucTool::GetTickCount();
	while(1)
	{
        tTick2 = NucTool::GetTickCount();
		if((tTick2 - tTick1) > millisecond_n)
			break;
	}
}
//--------------------------------------------------------------
//Read data
//parameter:
//	timeout:	最长等待时间。为0表示最长等待时间无穷大，其它值表示相应的最长等待时间，单位S
//	fp_raw_data_r: 读取文件指针。为0的话，从以太网接收数据；非0的话，从该文件读取数据。
//return：
//  当完成指定长度的数据读取或发生超时，函数返回。此时，返回值等于参数len。
//  返回-1，表示函数执行出错。
//--------------------------------------------------------------

INT32S Common_Data_Read( INT8U * rd_buf, INT32S len, INT32U timeout, FILE* fp_raw_data_r)
{
	INT32S rd_len;	
	if(fp_raw_data_r == 0)
	{
		rd_len = ETH_Data_Read(rd_buf,len,timeout);
		if(rd_len < 0) return -1;
	}
	else
	{
	
		rd_len = (INT32S)(fread(rd_buf,1,len,fp_raw_data_r));
		if(rd_len <= 0)	return -1;
	}
	return rd_len;
}

//--------------------------------------------------------------
//      全局数据读取缓存区
//--------------------------------------------------------------
#define GLB_DATA_BUF_SIZE	(64*1024*1024)	//不能超过32位数的表示范围，必须大于1个SC数据包长度
//
INT8U glb_data_buf[MAX_MCBNUM][GLB_DATA_BUF_SIZE];
std::vector<unsigned char> glb_data_buf_vetor[MAX_MCBNUM];
//std::map<int,std::vector<INT8U> > glb_data_buf;
// HANDLE hMutex;
ULONGLONG glb_data_AccWrBytes[MAX_MCBNUM] = {0};//记录总接收数据字节数
ULONGLONG glb_data_AccRdBytes[MAX_MCBNUM] = {0};//记录总读出处理字节数
INT32U gbl_data_buf_wraddr[MAX_MCBNUM] = {0};			//记录数据缓存区下一个数据的写入位置
INT32U gbl_data_buf_rdaddr[MAX_MCBNUM] = {0};			//记录下一个数据的读取地址
INT32S gbl_sc_pkg_index[MAX_MCBNUM] = { 0};			//记录各个DB Line上一个sc pkg的包编号
//
//一个DBLine的size= 10字节逻辑 + DB数*一个DB包size + 1个CB包字节
INT32U gbl_DBLine_Size[MAX_MCBNUM] = { 0 };

//一个MCB的size= 该MCB的所有DBLine的size之和
INT32U gbl_MCBTotal_Size[MAX_MCBNUM] = { 0 };

const INT8U XB_NAME_STR[3][20] = {"CB","DB","Unknown"};

//--------------------------------------------------------------
//      全局数据读取缓存区初始化函数
//--------------------------------------------------------------
//返回值：0=初始化成功；其它值=初始化失败
void gbl_data_buf_init(void)
{
	INT8U i;
	for(i = 0; i < MAX_MCBNUM; i++)
	{		
		glb_data_AccWrBytes[i] = 0;		//记录总接收数据字节数
		glb_data_AccRdBytes[i] = 0;		//记录总读出处理字节数
		gbl_data_buf_wraddr[i] = 0;		//记录数据缓存区下一个数据的写入位置
		gbl_data_buf_rdaddr[i] = 0;		//记录下一个数据的读取地址	
		gbl_sc_pkg_index[i] = -1;		//记录各个DB Line上一个sc pkg的包编号
	}	
}

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
INT8S Read_and_parse_one_SC_pkg(INT8U cmd_code, INT8U cmd_mismatched_pkg_skip_en, FILE* fp_parse_data_w, FILE* fp_raw_data_r)
{

return 0;
}



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
//				Others = Data package parsing errors

INT8S ECM_read_and_parse_data(INT32U pkg_num, INT8U cmd_code, INT8U cmd_mismatched_pkg_skip_en, INT8U cmd_exct_result_report_en, INT8U pkg_rate_cal_en, FILE* fp_parse_data_w, FILE* fp_raw_data_r)
{
	INT32U parsed_pkg_n=0;		//已经解析的数据包个数

	INT8S err,overall_err = 0;	//总错误
	char cmd_str[50];
	INT32U i;
	
	//init data buffer
	gbl_data_buf_init();

	//读取并解析数据
	i=0;
	while((i<pkg_num) || (pkg_num == 0))
	{
		i++;
		//读取并解析单次命令执行返回的数据包
		err = Read_and_parse_one_SC_pkg(cmd_code,cmd_mismatched_pkg_skip_en, fp_parse_data_w,fp_raw_data_r);
		err = Read_and_parse_one_SC_pkg(cmd_code,cmd_mismatched_pkg_skip_en, fp_parse_data_w,fp_raw_data_r);
		err = Read_and_parse_one_SC_pkg(cmd_code,cmd_mismatched_pkg_skip_en, fp_parse_data_w,fp_raw_data_r);
		err = Read_and_parse_one_SC_pkg(cmd_code,cmd_mismatched_pkg_skip_en, fp_parse_data_w,fp_raw_data_r);
		
		if(err < 0)
		{
			if(fp_raw_data_r == 0)  //满足条件表示以太网读取出错，否则，表示文件读取结束
			{
				overall_err = -1;
				break;
			}
			else
				break;
		}
		if(err > 0)
				overall_err = err;

		//
		if (pkg_rate_cal_en == 1)
			Pkg_Rate_Calcution_called_per_pkg();

	}

	//打印解析是否成功
	if (cmd_exct_result_report_en != 0)			
	{
		conv_cmd_code_to_string(cmd_code, cmd_str);
		if(overall_err != 0)
			printf("%s command is executed with errors!\n",cmd_str);
		else
			printf("%s command is executed successfully!\n",cmd_str);
	}


	return overall_err;
}

//----------------------------------------------------------
//      Read parse and dump data into an auto generated file 
//----------------------------------------------------------
//Parameters:
//	pkg_num:  0 = Read forever. Others = The number of packages to be read.
//	cmd_code: 0 = Don't check the package type. Others = Check the package type. 主要是用于判断读取的数据包是否是指定命令执行时预期返回的数据包。
//	fp_raw_data_r:	Point to the file that saves the raw data. 该指针为0的话，从以太网读取数据，否则，从该指针指向的文件读取数据。
void ECM_read_parse_dump_data_into_auto_generated_file(INT32U pkg_num, INT8U cmd_code, FILE* fp_raw_data_r)
{
	FILE*   fprecord;

	char file_name[100];
	char time_str[30];
	time_t t;

	 //Open a file with a time related name.
	 t = time(0); 
	 sprintf(file_name,"./Data/Parsed_data");

	 strftime(time_str, sizeof(time_str), "_%Y%m%d_%H%M%S.txt",localtime(&t) ); 
	 strcat(file_name,time_str);
	 fprecord= fopen(file_name,"w"); 

	 printf("The parsed data is being written into %s!\n",file_name);

	 //ECM_read_and_parse_data(INT32U pkg_num, INT8U cmd_code, INT8U cmd_exct_result_report_en, INT8U pkg_rate_cal_en, FILE* fp_parse_data_w,FILE* fp_raw_data_r)
	 ECM_read_and_parse_data(pkg_num, cmd_code, 0, 1, 1, fprecord,fp_raw_data_r);
	 
	fclose(fprecord);					
}


//把命令码转换命令字符串
void conv_cmd_code_to_string(INT8U cmd_code, char* cmd_str)
{
	switch(cmd_code)
	{					 
		case DB_DAQ_PKG  :  
			strcpy(cmd_str,"DB DAQ");
			break;
		case DB_RST_PKG  :  
			strcpy(cmd_str,"DB rst");
			break;
		case DB_LCT_PKG  :  
			strcpy(cmd_str,"DB lct");
			break;
		case DB_PRM_CFG_PKG  :  
			strcpy(cmd_str,"DB prm cfg");
			break;
		case DB_ERR_PKG  :  
			strcpy(cmd_str,"Err: DB received cmd pkg is wrong!");
			break;
		case CB_PKG  :  
			strcpy(cmd_str,"CB pkg");
			break;
		default:
			strcpy(cmd_str,"Unknown data pkg");
			break;
	}
}




//----------------------------------------------------------
//      Create a log file to record the non-daq command returned package
//----------------------------------------------------------
FILE* cmd_pkg_parse_log_file_create(void)
{
	FILE*  fprecord;

	char file_name[100];
	char time_str[30];
	time_t t;

	 //Open a file with a time related name.
	 t = time(0); 
	 sprintf(file_name,"./log/cmd_pkg_parse_log_file");

	 strftime(time_str, sizeof(time_str), "_%Y%m%d_%H%M%S.txt",localtime(&t) ); 
	 strcat(file_name,time_str);
	 fprecord= fopen(file_name,"w"); 
	 printf("Create a log file: %s\n",file_name);
	 
	 return fprecord;
	 //fclose(fprecord);					
}





//-----------------------------------
//      ETH data read link test function
//-----------------------------------

//Parameter:
//			test_data_n: Assign the number of data comparation test to be done.
//          return value: The number of errors that have been found.
			
INT32S ETH_count_data_read_link_test(INT32S test_data_n)
{
   INT32S i;
   //INT8U  seed = 0x78;
   INT32S byte_cnt=0;
   INT32S last_data=0,current_data=1,check_cnt=0,err_cnt=0;
   INT8U  rd_buf[1024];
   INT32S rd_len;
   INT8U buf_data;

	while(check_cnt<test_data_n)
	{
	    
		//读取TCP端口的数据
		rd_len = ETH_Data_Read (rd_buf,1024,5);
		//if(rd_len != 1024) printf("\nRead Data Error: return data len is %d",rd_len);
		if(rd_len < 0) 
			printf("\nRead Data Error");
		else
		{  
			
			for(i=0;i<rd_len;i++)
			{   
				buf_data = rd_buf[i];
				if(byte_cnt ==0)
				{
					
					current_data = buf_data<<24;
					byte_cnt = 1;
				}
				else if (byte_cnt ==1)
				{
					current_data += buf_data<<16;
					byte_cnt = 2;
				}
				else if (byte_cnt ==2)
				{		
					current_data += buf_data<<8;
					byte_cnt = 3;
				}
				else if (byte_cnt ==3)
				{
					current_data += buf_data;
					byte_cnt = 0;
					if(current_data != (last_data+1))
					{
						if(check_cnt != 0)	
						{
							printf("Error: Index = %d, last data = %08X, current data = %08X\n",check_cnt,last_data,current_data);
							err_cnt++;
						}
					}
					else
					{
						//if((check_cnt%10000) == 0)
						//	printf("Test cnt = %d, Error cnt = %d, Current data: %8X\n",check_cnt,err_cnt,current_data);
					}
					last_data = current_data;
					check_cnt++;
				}
				
			}

		}
		fflush(stdout);
	}
	if(err_cnt > 0)
	{
		printf("Total finished. Find errors!\n");
	}
	else
	{
		printf("Total finished. No error!\n");
	}
	return err_cnt;

}


//----------------------------------------------------
//      Long time ETH data read test function
//----------------------------------------------------

//Parameter:
//			total_tst_time_in_hour: Total test time in hours.
//          return value: 0= No test error; 1= Found test error.
			
INT8U ETH_count_data_read_test_for_long_time(float total_tst_time_in_hour)
{
    return 0;
	INT32S i,rd_size;
	INT8U  byte_cnt=0;
	INT64U tst_cnt_in_byte=0;  //In bytes
	INT32U current_data=1,last_data=0,err_cnt=0;
	INT8U  rd_buf[4096];
	INT32S rd_len;
	float tst_ratio;
	time_t start_t,end_t,current_t;
	INT32U total_tst_time_in_sec,pass_time_in_sec;
	float baud_rate; //byte per second.
	float remain_time_in_hour;

	total_tst_time_in_sec = (INT32U)(total_tst_time_in_hour*3600);
	start_t = time(0); 
	current_t = start_t;
	end_t = start_t + total_tst_time_in_sec;
	rd_size = 1024*3;

	while(current_t<end_t)
	{
	    
		//读取TCP端口的数据
		rd_len = ETH_Data_Read (rd_buf,rd_size,5);
		//if(rd_len != 1024) printf("\nRead Data Error: return data len is %d",rd_len);
		if(rd_len < 0) 
			printf("\nRead Data Error!\n");
		else
		{  		
			
			for(i=0;i<rd_len;i++)
			{   
				if(byte_cnt ==0)
				{					
					current_data = rd_buf[i]<<24;
					byte_cnt = 1;
				}
				else if (byte_cnt ==1)
				{
					current_data += rd_buf[i]<<16;
					byte_cnt = 2;
				}
				else if (byte_cnt ==2)
				{		
					current_data += rd_buf[i]<<8;
					byte_cnt = 3;
				}
				else if (byte_cnt ==3)
				{
					current_data += rd_buf[i];
					byte_cnt = 0;
					if(current_data != (last_data+1))
					{
						if(tst_cnt_in_byte != 0)	
						{
							printf("	Error: TstCnt= %I64u, last data= %08X, current data= %08X\n",tst_cnt_in_byte,last_data,current_data);
							err_cnt++;
							if(err_cnt > 100)
								return 1;
						}
					}

					last_data = current_data;
					tst_cnt_in_byte = tst_cnt_in_byte + 4;

				}
				
			}
			
			//tst_cnt_in_byte += rd_len;
			if((tst_cnt_in_byte%10485760) < rd_size)
			{
				//Calculate remained time
				current_t = time(0);
				if(current_t >= end_t)
					remain_time_in_hour = 0;
				else
					remain_time_in_hour = (float)((end_t-current_t)/3600.0);
				
				//Calculate test progress
				tst_ratio = (float)((total_tst_time_in_hour-remain_time_in_hour)/total_tst_time_in_hour*100.0);

				//Calculate baud rate
				pass_time_in_sec = (INT32U) (current_t - start_t);
				if(pass_time_in_sec > 0)
					baud_rate = (float)(tst_cnt_in_byte*8.0/pass_time_in_sec/1000000.0);
				else
					baud_rate = 0;

				printf("TestCnt= %.3e, %0.1f%%, %0.2fh, %0.0fMbps, ErrCnt= %d, CurD= %08X\n",(float)tst_cnt_in_byte,tst_ratio,remain_time_in_hour,baud_rate,err_cnt,current_data);
			}
		}
		fflush(stdout);
	}
	if(err_cnt > 0)
	{
		printf("	Find error!\n");
		return 1;
	}
	else
	{
		printf("	No error! ^_^\n");
		return 0;
	}
	

}

//----------------------------------------------------------
//      ETH data read and dump into a specified file 
//----------------------------------------------------------
//Parameters:
//	byte_n: 0 = Read forever. Others = The number of bytes to be read.
//  BWC_en: 1= Permit to calculate the band width.
//  fp: 0= Don't write file. 

void ETH_data_read_and_dump_into_file(INT32U byte_n, INT8U BWC_en, FILE* fp)
{
   INT32U byte_cnt=0;
   INT8U  rd_buf[2048];
   INT32S rd_len;

	while((byte_n == 0) || (byte_cnt<byte_n))
	{
	    
		//读取TCP端口的数据
		rd_len = ETH_Data_Read (rd_buf,1024,5);  
				
		if(rd_len < 0) 
		{
			return;
		}
		byte_cnt += rd_len;

		
		if (BWC_en == 1)
			band_width_calcution(rd_len);

		if(fp != 0)
		{ 
			fwrite(rd_buf, sizeof(INT8U), rd_len, fp);
		}
	}
	printf("Data read is finished!\n");
}

//----------------------------------------------------------
//      ETH data read and dump into a auto generated file 
//----------------------------------------------------------
//Parameters:
//	byte_n: 0 = Read forever. Others = The number of bytes to be read.
//  BWC_en: 1= Permit to calculate the band width.
//返回值：无
void ETH_data_read_and_dump_into_auto_generated_file(char * data_file_name, INT32U byte_n, INT8U BWC_en)
{
	FILE*   fprecord;

	//char data_file_name[100];
	char time_str[30];
	time_t t;

	 //Open a file with a time related name.
	 t = time(0); 
	 sprintf(data_file_name,"./Data/Raw_data");

	 strftime(time_str, sizeof(time_str), "_%Y%m%d_%H%M%S.bin",localtime(&t) ); 
	 strcat(data_file_name,time_str);
	 fprecord= fopen(data_file_name,"wb"); 

	 printf("The bin data is being written into %s!\n",data_file_name);
	 
	 ETH_data_read_and_dump_into_file(byte_n, BWC_en, fprecord);

	 fclose(fprecord);			
}
