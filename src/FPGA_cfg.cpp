#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include "stdafx.h"
#else
#include <stdio.h>
#endif
#include "including.h"
 
//-----------------------------------
//ECS writing FPGA config data test function
//-----------------------------------
void EDAQ_ECS_wr_fpga_cfg_data_test(unsigned short nMCB)
{
	INT32U i;
	FILE*   fprecord;

	char file_name[100];
	char time_str[30];
	time_t t;

	 //Open a file with a time related name.
	 t = time(0); 
	 sprintf(file_name,"./log/EDAQ_ECS_wr_fpga_cfg_data");

	 strftime(time_str, sizeof(time_str), "_%Y%m%d_%H%M%S.txt",localtime(&t) ); 
	 strcat(file_name,time_str);
	 fprecord= fopen(file_name,"w"); 


	EDAQ_RB_wr_port_sel(EDAQ_RB_WR_SEL_NO_DATA, nMCB);  //Stop writing RBIB.
	EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_NO_DATA, nMCB);  //Stop reading RBOB.
	EDAQ_RBxB_reset(nMCB);		//reset RBIB AND RBOB

	EDAQ_RB_wr_port_sel(EDAQ_RB_WR_SEL_ECS_FPGA_CFG_DATA, nMCB); //Slect ECS writing FPGA cfg data

	for(i=0;i<100000;i++)
	{
		EDAQ_DB_fpga_cfg_data_wr_via_ECS(i, nMCB); 
	}

	EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_ETH_DATA, nMCB); //RBOB read data link to ETH data read link

	ETH_data_read_and_dump_into_file(10000,1,fprecord);

	fclose(fprecord);
					 
}


//-------------------------------------------------------------
//功能：Read data from DB FPGA program file and then write the data into EDAQ RB via ECS
//参数：filename指定配置文件。
//返回值：FPGA配置文件的长度，单位为字节。如果返回值为-1，说明文件不存在。
int glb_DB_fpga_cfg_file_len;
INT32S EDAQ_read_and_write_DB_fpga_program_file_into_RB_via_ECS(char *filename, unsigned short nMCB)
{
	int i;
	INT32U data32;
	int            Index = 0;
	unsigned short length;
	int            fileLength;
	unsigned short blockLength = 64;  //Must be multiple of 4
	unsigned char  fpgaData[64];
	FILE *         fpgaFile;
    FILE *           fh;


	EDAQ_RB_wr_port_sel(EDAQ_RB_WR_SEL_NO_DATA, nMCB);  //Stop writing RBIB.	
	EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_NO_DATA, nMCB);  //Stop reading RBOB.

	EDAQ_RBxB_reset(nMCB);		//reset RBIB and RBOB

	EDAQ_RB_wr_port_sel(EDAQ_RB_WR_SEL_ECS_FPGA_CFG_DATA, nMCB); //Slect ECS writing FPGA cfg data

    /*=== Calculate the data length  ===*/
   // if ((fh = _open(filename, _O_BINARY | _O_RDONLY, 0))  != -1)

    fh = fopen(filename,"r");
    if (fh  != NULL)
	{
        fseek(fh,0,SEEK_END);
        fileLength = ftell(fh);
        fclose( fh );
    }
    else 
	{
		  printf("Unable to compute length of the file [%s].",filename);
		  fileLength = 0;
		  return -1;
	   
	}

	glb_DB_fpga_cfg_file_len = fileLength;
		  
  
   fpgaFile = fopen(filename,"rb");
   while (Index < fileLength) 
   {
		/*=== Read the data  ===*/
		if ((fileLength - Index) < blockLength) {
			length = fileLength - Index;
		}
		else {
			length = blockLength;
		}

		/*===  Read the data from the file  ===*/
		fread(fpgaData, length, 1, fpgaFile);
       

		/*=== Update the counters  ===*/
		Index += length;
  
		/*=== Update the progress bar  ===*/
		if((Index &0xFFF)==0) printf(".");

		//check if the file has been read out.
		if (Index == fileLength)  //full fill the array with zero becasue of the RB working scheme.
		{
			for(i=length;i<blockLength;i++)
				fpgaData[i] = 0;
		}

		//Write fpga cfg data via ECS
		for(i=0;i<(blockLength/4);i++)
		{
			data32 = (fpgaData[4*i]<<24) + (fpgaData[4*i+1]<<16) + (fpgaData[4*i+2]<<8) + fpgaData[4*i+3];
			EDAQ_DB_fpga_cfg_data_wr_via_ECS(data32, nMCB); 
		}
		
		//Write more data becasue of the RB working scheme.
		if (Index == fileLength)  
		{
			for(i=0;i<2048;i++)
				EDAQ_DB_fpga_cfg_data_wr_via_ECS(0, nMCB);
		}
	}
   printf("\nThe DB fpga cfg data has been read from bit file and written into RB via ECS!\n");

   fclose(fpgaFile);

   return fileLength;
					 
}

//-------------------------------------------------------------
// Read DB FPGA program data from EDAQ RB via ETH and dump into file
//------------------------------------------------------------
void EDAQ_read_DB_fpga_program_data_from_RB_via_ETH_and_dump_into_file(unsigned short nMCB)
{
	FILE*   fprecord;

	char file_name[100];
	char time_str[30];
	time_t t;

	 //Open a file with a time related name.
	 t = time(0); 
	 sprintf(file_name,"./log/EDAQ_ECS_wr_fpga_cfg_data");

	 strftime(time_str, sizeof(time_str), "_%Y%m%d_%H%M%S.txt",localtime(&t) ); 
	 strcat(file_name,time_str);
	 fprecord= fopen(file_name,"w"); 

	EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_NO_DATA, nMCB);  //Stop reading RB.
	EDAQ_RBOB_reset(nMCB);		//reset RBOB

	EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_ETH_DATA, nMCB); //RBOB read data link to ETH data read link

	ETH_data_read_and_dump_into_file(glb_DB_fpga_cfg_file_len,0,fprecord);

	printf("The DB fpga cfg data has been read from RB via ETH data link and written into log file!\n");

	fclose(fprecord);					
}

//-------------------------------------------------------------
// Request the EDAQ fpga to program DB fpga.
//------------------------------------------------------------
void EDAQ_DB_fpga_config_request(unsigned short nMCB)
{
	EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_NO_DATA, nMCB);  //Stop reading RB.
	EDAQ_RBOB_reset(nMCB);		//reset RBOB

	EDAQ_RB_rd_port_sel(EDAQ_RB_RD_SEL_FPGA_CFG_DATA, nMCB); //RBOB read data link to DB fpga config

	EDAQ_DB_FPGA_cfg_enable(1, nMCB);

	EDAQ_cfg_fpga_req(nMCB);
	printf("The request to program DB fpga has been issued!\n");
					
}
