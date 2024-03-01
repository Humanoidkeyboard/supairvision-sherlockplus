#include <env.hpp>



/* Private values -------------------------------------------------------------*/
static FILE *s_djiLogFile;
static FILE *s_djiLogFileCnt;
static pthread_t s_monitorThread = 0;

/* Private functions definition-----------------------------------------------*/
T_DjiReturnCode DjiUser_PrepareSystemEnvironment(void)
{
    T_DjiReturnCode returnCode;
    T_DjiOsalHandler osalHandler = {
        .TaskCreate = Osal_TaskCreate,
        .TaskDestroy = Osal_TaskDestroy,
        .TaskSleepMs = Osal_TaskSleepMs,
        .MutexCreate= Osal_MutexCreate,
        .MutexDestroy = Osal_MutexDestroy,
        .MutexLock = Osal_MutexLock,
        .MutexUnlock = Osal_MutexUnlock,
        .SemaphoreCreate = Osal_SemaphoreCreate,
        .SemaphoreDestroy = Osal_SemaphoreDestroy,
        .SemaphoreWait = Osal_SemaphoreWait,
        .SemaphoreTimedWait = Osal_SemaphoreTimedWait,
        .SemaphorePost = Osal_SemaphorePost,
        .GetTimeMs = Osal_GetTimeMs,
        .GetTimeUs = Osal_GetTimeUs,
        .GetRandomNum  = Osal_GetRandomNum,
        .Malloc = Osal_Malloc,
        .Free = Osal_Free,
    };

    T_DjiLoggerConsole printConsole = {
        .func = DjiUser_PrintConsole,
        .consoleLevel = DJI_LOGGER_CONSOLE_LOG_LEVEL_INFO,
        .isSupportColor = true,
    };

    T_DjiLoggerConsole localRecordConsole = {
        .func = DjiUser_LocalWrite,
        .consoleLevel = DJI_LOGGER_CONSOLE_LOG_LEVEL_DEBUG,
        .isSupportColor = true,
    };

    T_DjiHalUartHandler uartHandler = {
        .UartInit = HalUart_Init,
        .UartDeInit = HalUart_DeInit,
        .UartWriteData = HalUart_WriteData,
        .UartReadData = HalUart_ReadData,
        .UartGetStatus = HalUart_GetStatus,
    };

    T_DjiHalNetworkHandler networkHandler = {
        .NetworkInit = HalNetWork_Init,
        .NetworkDeInit = HalNetWork_DeInit,
        .NetworkGetDeviceInfo = HalNetWork_GetDeviceInfo,
    };

    T_DjiHalUsbBulkHandler usbBulkHandler = {
        .UsbBulkInit = HalUsbBulk_Init,
        .UsbBulkDeInit = HalUsbBulk_DeInit,
        .UsbBulkWriteData = HalUsbBulk_WriteData,
        .UsbBulkReadData = HalUsbBulk_ReadData,
        .UsbBulkGetDeviceInfo = HalUsbBulk_GetDeviceInfo,
    };

    T_DjiFileSystemHandler fileSystemHandler = {
        .FileOpen = Osal_FileOpen,
        .FileClose = Osal_FileClose,
        .FileWrite = Osal_FileWrite,
        .FileRead = Osal_FileRead,
        .FileSeek = Osal_FileSeek,
        .FileSync = Osal_FileSync,
        .DirOpen = Osal_DirOpen,
        .DirClose = Osal_DirClose,
        .DirRead = Osal_DirRead,
        .Mkdir = Osal_Mkdir,
        .Unlink = Osal_Unlink,
        .Rename = Osal_Rename,
        .Stat = Osal_Stat,
    };

    T_DjiSocketHandler socketHandler = {
        .Socket = Osal_Socket,
        .Close = Osal_Close,
        .Bind = Osal_Bind,
        .UdpSendData = Osal_UdpSendData,
        .UdpRecvData = Osal_UdpRecvData,
        .TcpListen = Osal_TcpListen,
        .TcpAccept = Osal_TcpAccept,
        .TcpConnect = Osal_TcpConnect,
        .TcpSendData = Osal_TcpSendData,
        .TcpRecvData = Osal_TcpRecvData,
    };

    returnCode = DjiPlatform_RegOsalHandler(&osalHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("register osal handler error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    returnCode = DjiPlatform_RegHalUartHandler(&uartHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("register hal uart handler error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

#ifdef LOG
    if (DjiUser_LocalWriteFsInit(DJI_LOG_PATH) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("file system init error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }
#endif
    returnCode = DjiLogger_AddConsole(&printConsole);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("add printf console error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    returnCode = DjiLogger_AddConsole(&localRecordConsole);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("add printf console error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }


#if (CONFIG_HARDWARE_CONNECTION == DJI_USE_UART_AND_USB_BULK_DEVICE)
    returnCode = DjiPlatform_RegHalUsbBulkHandler(&usbBulkHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("register hal usb bulk handler error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }
#elif (CONFIG_HARDWARE_CONNECTION == DJI_USE_UART_AND_NETWORK_DEVICE)
    returnCode = DjiPlatform_RegHalNetworkHandler(&networkHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("register hal network handler error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    //Attention: if you want to use camera stream view function, please uncomment it.
    returnCode = DjiPlatform_RegSocketHandler(&socketHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("register osal socket handler error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }
#elif (CONFIG_HARDWARE_CONNECTION == DJI_USE_ONLY_UART)
    /*!< Attention: Only use uart hardware connection.
     */
#endif

    returnCode = DjiPlatform_RegFileSystemHandler(&fileSystemHandler);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        printf("register osal filesystem handler error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode DjiUser_CleanSystemEnvironment(void)
{
    T_DjiReturnCode returnCode;
    T_DjiAircraftInfoBaseInfo aircraftInfoBaseInfo;

    returnCode = DjiAircraftInfo_GetBaseInfo(&aircraftInfoBaseInfo);
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        USER_LOG_ERROR("get aircraft base info error");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }


    returnCode = DjiCore_DeInit();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        perror("Core deinit failed.");
    }

    return returnCode;
}

T_DjiReturnCode DjiUser_FillInUserInfo(T_DjiUserInfo *userInfo)
{
    if (userInfo == NULL) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
    }

    memset(userInfo->appName, 0, sizeof(userInfo->appName));
    memset(userInfo->appId, 0, sizeof(userInfo->appId));
    memset(userInfo->appKey, 0, sizeof(userInfo->appKey));
    memset(userInfo->appLicense, 0, sizeof(userInfo->appLicense));
    memset(userInfo->developerAccount, 0, sizeof(userInfo->developerAccount));
    memset(userInfo->baudRate, 0, sizeof(userInfo->baudRate));

    if (strlen(USER_APP_NAME) >= sizeof(userInfo->appName) ||
        strlen(USER_APP_ID) > sizeof(userInfo->appId) ||
        strlen(USER_APP_KEY) > sizeof(userInfo->appKey) ||
        strlen(USER_APP_LICENSE) > sizeof(userInfo->appLicense) ||
        strlen(USER_DEVELOPER_ACCOUNT) >= sizeof(userInfo->developerAccount) ||
        strlen(USER_BAUD_RATE) > sizeof(userInfo->baudRate)) {
        USER_LOG_ERROR("Length of user information string is beyond limit. Please check.");
        sleep(1);
        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
    }

    if (!strcmp(USER_APP_NAME, "your_app_name") ||
        !strcmp(USER_APP_ID, "your_app_id") ||
        !strcmp(USER_APP_KEY, "your_app_key") ||
        !strcmp(USER_BAUD_RATE, "your_app_license") ||
        !strcmp(USER_DEVELOPER_ACCOUNT, "your_developer_account") ||
        !strcmp(USER_BAUD_RATE, "your_baud_rate")) {
        USER_LOG_ERROR(
            "Please fill in correct user information to 'samples/sample_c/platform/linux/manifold2/application/dji_sdk_app_info.h' file.");
        sleep(1);
        return DJI_ERROR_SYSTEM_MODULE_CODE_INVALID_PARAMETER;
    }

    strncpy(userInfo->appName, USER_APP_NAME, sizeof(userInfo->appName) - 1);
    memcpy(userInfo->appId, USER_APP_ID, USER_UTIL_MIN(sizeof(userInfo->appId), strlen(USER_APP_ID)));
    memcpy(userInfo->appKey, USER_APP_KEY, USER_UTIL_MIN(sizeof(userInfo->appKey), strlen(USER_APP_KEY)));
    memcpy(userInfo->appLicense, USER_APP_LICENSE,
           USER_UTIL_MIN(sizeof(userInfo->appLicense), strlen(USER_APP_LICENSE)));
    memcpy(userInfo->baudRate, USER_BAUD_RATE, USER_UTIL_MIN(sizeof(userInfo->baudRate), strlen(USER_BAUD_RATE)));
    strncpy(userInfo->developerAccount, USER_DEVELOPER_ACCOUNT, sizeof(userInfo->developerAccount) - 1);

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode DjiUser_PrintConsole(const uint8_t *data, uint16_t dataLen)
{
    USER_UTIL_UNUSED(dataLen);

    printf("%s", data);

    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

static T_DjiReturnCode DjiUser_LocalWrite(const uint8_t *data, uint16_t dataLen)
{
    uint32_t realLen;

    if (s_djiLogFile == NULL) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }

    realLen = fwrite(data, 1, dataLen, s_djiLogFile);
    fflush(s_djiLogFile);
    if (realLen == dataLen) {
        return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    } else {
        return DJI_ERROR_SYSTEM_MODULE_CODE_UNKNOWN;
    }
}

static T_DjiReturnCode DjiUser_LocalWriteFsInit(const char *path)
{
    T_DjiReturnCode djiReturnCode = DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
    char filePath[DJI_LOG_PATH_MAX_SIZE];
    char systemCmd[DJI_SYSTEM_CMD_STR_MAX_SIZE];
    char folderName[DJI_LOG_FOLDER_NAME_MAX_SIZE];
    time_t currentTime = time(NULL);
    struct tm *localTime = localtime(&currentTime);
    uint16_t logFileIndex = 0;
    uint16_t currentLogFileIndex;
    uint8_t ret;

    if (localTime == NULL) {
        printf("Get local time error.\r\n");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    if (access(DJI_LOG_FOLDER_NAME, F_OK) != 0) {
        sprintf(folderName, "mkdir %s", DJI_LOG_FOLDER_NAME);
        ret = system(folderName);
        if (ret != 0) {
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
    }

    s_djiLogFileCnt = fopen(DJI_LOG_INDEX_FILE_NAME, "rb+");
    if (s_djiLogFileCnt == NULL) {
        s_djiLogFileCnt = fopen(DJI_LOG_INDEX_FILE_NAME, "wb+");
        if (s_djiLogFileCnt == NULL) {
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
    } else {
        ret = fseek(s_djiLogFileCnt, 0, SEEK_SET);
        if (ret != 0) {
            printf("Seek log count file error, ret: %d, errno: %d.\r\n", ret, errno);
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }

        ret = fread((uint16_t *) &logFileIndex, 1, sizeof(uint16_t), s_djiLogFileCnt);
        if (ret != sizeof(uint16_t)) {
            printf("Read log file index error.\r\n");
        }
    }

    currentLogFileIndex = logFileIndex;
    logFileIndex++;

    ret = fseek(s_djiLogFileCnt, 0, SEEK_SET);
    if (ret != 0) {
        printf("Seek log file error, ret: %d, errno: %d.\r\n", ret, errno);
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    ret = fwrite((uint16_t *) &logFileIndex, 1, sizeof(uint16_t), s_djiLogFileCnt);
    if (ret != sizeof(uint16_t)) {
        printf("Write log file index error.\r\n");
        fclose(s_djiLogFileCnt);
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    fclose(s_djiLogFileCnt);

    sprintf(filePath, "%s_%04d_%04d%02d%02d_%02d-%02d-%02d.log", path, currentLogFileIndex,
            localTime->tm_year + 1900, localTime->tm_mon + 1, localTime->tm_mday,
            localTime->tm_hour, localTime->tm_min, localTime->tm_sec);

    s_djiLogFile = fopen(filePath, "wb+");
    if (s_djiLogFile == NULL) {
        USER_LOG_ERROR("Open filepath time error.");
        return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
    }

    if (logFileIndex >= DJI_LOG_MAX_COUNT) {
        sprintf(systemCmd, "rm -rf %s_%04d*.log", path, currentLogFileIndex - DJI_LOG_MAX_COUNT);
        ret = system(systemCmd);
        if (ret != 0) {
            printf("Remove file error, ret:%d.\r\n", ret);
            return DJI_ERROR_SYSTEM_MODULE_CODE_SYSTEM_ERROR;
        }
    }

    sprintf(systemCmd, "ln -sfrv %s " DJI_LOG_FOLDER_NAME "/latest.log", filePath);
    system(systemCmd);
    return djiReturnCode;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#pragma GCC diagnostic ignored "-Wreturn-type"

static void *DjiUser_MonitorTask(void *argument)
{
    unsigned int i = 0;
    unsigned int threadCount = 0;
    pid_t *tidList = NULL;
    T_ThreadAttribute *threadAttribute = NULL;
    T_DjiOsalHandler *osalHandler = DjiPlatform_GetOsalHandler();

    USER_UTIL_UNUSED(argument);

    while (1) {
        threadCount = Monitor_GetThreadCountOfProcess(getpid());
        tidList = osalHandler->Malloc(threadCount * sizeof(pid_t));
        if (tidList == NULL) {
            USER_LOG_ERROR("malloc fail.");
            goto delay;
        }
        Monitor_GetTidListOfProcess(getpid(), tidList, threadCount);

        threadAttribute = osalHandler->Malloc(threadCount * sizeof(T_ThreadAttribute));
        if (threadAttribute == NULL) {
            USER_LOG_ERROR("malloc fail.");
            goto freeTidList;
        }
        for (i = 0; i < threadCount; ++i) {
            threadAttribute[i].tid = tidList[i];
        }

        USER_LOG_DEBUG("thread pcpu:");
        USER_LOG_DEBUG("tid\tname\tpcpu");
        for (i = 0; i < threadCount; ++i) {
            threadAttribute[i].pcpu = Monitor_GetPcpuOfThread(getpid(), tidList[i]);
            Monitor_GetNameOfThread(getpid(), tidList[i], threadAttribute[i].name, sizeof(threadAttribute[i].name));
            USER_LOG_DEBUG("%d\t%15s\t%f %%.", threadAttribute[i].tid, threadAttribute[i].name,
                           threadAttribute[i].pcpu);
        }

        USER_LOG_DEBUG("heap used: %d B.", Monitor_GetHeapUsed(getpid()));
        USER_LOG_DEBUG("stack used: %d B.", Monitor_GetStackUsed(getpid()));

        osalHandler->Free(threadAttribute);
freeTidList:
        osalHandler->Free(tidList);

delay:
        sleep(10);
    }
}

static T_DjiReturnCode DjiTest_HighPowerApplyPinInit()
{
    return DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS;
}

void DjiUser_NormalExitHandler(int signalNum)
{
    T_DjiReturnCode returnCode;

    USER_UTIL_UNUSED(signalNum);

    returnCode = DjiUser_CleanSystemEnvironment();
    if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
        perror("Clean up system environment failed.");
    }

    exit(0);
}

#pragma GCC diagnostic pop