
#define _CRT_SECURE_NO_WARNINGS
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <io.h>                        /* for _open(), _filelength() */
#include <Windows.h>
#else

#endif
#include <fcntl.h>                     /* for _O_BINARY and _O_RDONLY */
#include <iostream>

#include <stdio.h>
#include <time.h>
#include "string.h"

#include "Config.h"
#include "type_def.h"
#include "ETH_func.h"
#include "memory_map.h"
#include "ECS_func.h"
#include "FPGA_cfg.h"
#include "CB_bsp.h"


