#include <dji_platform.h>
#include <dji_logger.h>
#include <dji_core.h>


#include <utils/util_misc.hpp>
#include <errno.h>
#include "monitor/sys_monitor.hpp"
#include "osal/osal.hpp"
#include "osal/osal_fs.hpp"
#include "osal/osal_socket.hpp"
#include "hal_uart.hpp"
#include "hal_network.hpp"
#include "hal_usb_bulk.hpp"
#include "dji_sdk_app_info.hpp"
#include "dji_aircraft_info.h"
#include "dji_sdk_config.hpp"



/* Private constants ---------------------------------------------------------*/
#define DJI_LOG_PATH                    "Logs/DJI"
#define DJI_LOG_INDEX_FILE_NAME         "Logs/index"
#define DJI_LOG_FOLDER_NAME             "Logs"
#define DJI_LOG_PATH_MAX_SIZE           (128)
#define DJI_LOG_FOLDER_NAME_MAX_SIZE    (32)
#define DJI_LOG_MAX_COUNT               (10)
#define DJI_SYSTEM_CMD_STR_MAX_SIZE     (64)
#define DJI_SYSTEM_RESULT_STR_MAX_SIZE  (128)

#define DJI_USE_WIDGET_INTERACTION       0


/* Private types -------------------------------------------------------------*/
typedef struct {
    pid_t tid;
    char name[16];
    float pcpu;
} T_ThreadAttribute;



/* Private functions declaration ---------------------------------------------*/
T_DjiReturnCode DjiUser_PrepareSystemEnvironment(void);
static T_DjiReturnCode DjiUser_CleanSystemEnvironment(void);
T_DjiReturnCode DjiUser_FillInUserInfo(T_DjiUserInfo *userInfo);
static T_DjiReturnCode DjiUser_PrintConsole(const uint8_t *data, uint16_t dataLen);
static T_DjiReturnCode DjiUser_LocalWrite(const uint8_t *data, uint16_t dataLen);
static T_DjiReturnCode DjiUser_LocalWriteFsInit(const char *path);
static void *DjiUser_MonitorTask(void *argument);
static T_DjiReturnCode DjiTest_HighPowerApplyPinInit();
void DjiUser_NormalExitHandler(int signalNum);