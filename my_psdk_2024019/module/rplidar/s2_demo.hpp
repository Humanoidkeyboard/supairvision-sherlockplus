#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "sl_lidar.h" 
#include "sl_lidar_driver.h"
#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif
using namespace sl;
bool checkSLAMTECLIDARHealth(ILidarDriver * drv);

