/**
 ********************************************************************
 * @file    main.c
 * @brief
 *
 * @copyright (c) 2021 DJI. All rights reserved.
 *
 * All information contained herein is, and remains, the property of DJI.
 * The intellectual and technical concepts contained herein are proprietary
 * to DJI and may be covered by U.S. and foreign patents, patents in process,
 * and protected by trade secret or copyright law.  Dissemination of this
 * information, including but not limited to data and other proprietary
 * material(s) incorporated within the information, in any form, is strictly
 * prohibited without the express written consent of DJI.
 *
 * If you receive this source code without DJI’s authorization, you may not
 * further disseminate the information, and you must immediately remove the
 * source code and notify DJI of its removal. DJI reserves the right to pursue
 * legal actions against you for any loss(es) or damage(s) caused by your
 * failure to do so.
 *
 *********************************************************************
 */

/* Includes ------------------------------------------------------------------*/



#include <dji_high_speed_data_channel.h>
#include <dji_mop_channel.h>

#include <errno.h>
#include <signal.h>

#include "upgrade_platform_opt/upgrade_platform_opt_linux.hpp"
#include <env/env.hpp>
#include <utils/cJSON.hpp>


#include "rplidar/s2_demo.hpp"

/* Private constants ---------------------------------------------------------*/



/* Private types -------------------------------------------------------------*/


/* Private values -------------------------------------------------------------*/


/* Private functions declaration ---------------------------------------------*/



bool ctrl_c_pressed;
void ctrlc(int)
{
    ctrl_c_pressed = true;
}

/* Exported functions definition ---------------------------------------------*/
int main(int argc, char **argv)
{
    T_DjiReturnCode returnCode;
    T_DjiUserInfo userInfo;
    T_DjiAircraftInfoBaseInfo aircraftInfoBaseInfo;
    T_DjiAircraftVersion aircraftInfoVersion;
    T_DjiFirmwareVersion firmwareVersion = {
        .majorVersion = 1,
        .minorVersion = 0,
        .modifyVersion = 0,
        .debugVersion = 0,
    };

    USER_UTIL_UNUSED(argc);
    USER_UTIL_UNUSED(argv);

    // attention: when the program is hand up ctrl-c will generate the coredump file
    signal(SIGTERM, DjiUser_NormalExitHandler);

    /*!< Step 1: Prepare system environment, such as osal, hal uart, console function and so on. */
    returnCode = DjiUser_PrepareSystemEnvironment();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Prepare system environment error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    /*!< Step 2: Fill your application information in dji_sdk_app_info.h and use this interface to fill it. */
    returnCode = DjiUser_FillInUserInfo(&userInfo);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Fill user info error, please check user info config");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    /*!< Step 3: Initialize the Payload SDK core by your application information. */
    returnCode = DjiCore_Init(&userInfo);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("Core init error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    returnCode = DjiCore_SetAlias("PSDK_APPALIAS");
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("set alias error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    returnCode = DjiCore_SetFirmwareVersion(firmwareVersion);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("set firmware version error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    returnCode = DjiCore_SetSerialNumber("PSDK12345678XX");
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("set serial number error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }


    /*!< Step 5: Tell the DJI Pilot you are ready. */
    returnCode = DjiCore_ApplicationStart();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("start sdk application error");
    }

    // create the driver instance
	ILidarDriver * drv = *createLidarDriver();
    if(!drv){
        USER_LOG_ERROR("can not creat lidar driver");
    }
    IChannel* _channel=NULL;
    _channel = (*createSerialPortChannel("/dev/ttyUSB1",1000000));

    if (SL_IS_OK((drv)->connect(_channel))) {
                USER_LOG_INFO("lidar is connected");
                signal(SIGINT, ctrlc);
                drv->setMotorSpeed();
                // start scan...
                drv->startScan(0,1);
            }

    sl_result op_result;
    DjiLowSpeedDataChannel_Init();
    sl_u32 min_distance;
    int which=0;

    cJSON* root = cJSON_CreateObject();
    char* json_str=NULL;


    while (1) {
        sl_lidar_response_measurement_node_hq_t nodes[3500];
        size_t   count = _countof(nodes);

        op_result = drv->grabScanDataHq(nodes, count);
        // USER_LOG_INFO("count of data:%d",count);

        if (SL_IS_OK(op_result)) {
            drv->ascendScanData(nodes, count);
            for (int pos = 0; pos < (int)count ; pos++) {
                // USER_LOG_INFO("theta:%d Dist:%d",nodes[pos].angle_z_q14,nodes[pos].dist_mm_q2);
                //270=49152*90/16384
                // data filter (angle>=270)
                if(nodes[pos].angle_z_q14>=49152){
                    
                    min_distance=nodes[pos].dist_mm_q2;
                    for(int i=pos;i<(int)count;i++){
                        if(nodes[i].dist_mm_q2<min_distance){
                            min_distance=nodes[i].dist_mm_q2;
                            which=i;
                            }
                    }
                    // USER_LOG_INFO("angle:%03.2f distance:%08.2f mindistance:%08.2f",
                    //         (nodes[which].angle_z_q14*90.f)/16384.f,
                    //         nodes[which].dist_mm_q2/4.0f,
                    //         min_distance/4.0f);
                    cJSON_DeleteItemFromObject(root,"angle");
                    cJSON_AddNumberToObject(root,"angle",(nodes[which].angle_z_q14*90.f)/16384.f);
                    cJSON_DeleteItemFromObject(root,"distance");
                    cJSON_AddNumberToObject(root,"distance",nodes[which].dist_mm_q2/4.0f);
                    json_str=cJSON_Print(root);
                    USER_LOG_INFO("%s",json_str);
                    DjiLowSpeedDataChannel_SendData(DJI_CHANNEL_ADDRESS_MASTER_RC_APP,(uint8_t*)json_str,strlen(json_str));

                    break;

                }
                                
            }

        }

        if (ctrl_c_pressed){ 
            break;
        }

        
    }

        drv->stop();
        drv->setMotorSpeed(0);
        if(drv) {
        delete drv;
        drv = NULL;
        }
        return 0;
}
// }

/* Private functions definition-----------------------------------------------*/



/****************** (C) COPYRIGHT DJI Innovations *****END OF FILE****/
