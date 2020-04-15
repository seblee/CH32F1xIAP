/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2019/10/15
 * Description        : Main program body.
 *******************************************************************************/
#include "debug.h"
#include "string.h"
#include "CH32F103UFI.H"
//#include "stdint.h"
/* Global define */
#define PAGE_WRITE_START_ADDR ((UINT32)0x08005000) /* Start from 20K */
#define PAGE_WRITE_END_ADDR ((UINT32)0x08010000)   /* End at 63K */
#define FLASH_PAGE_SIZE 1024
#define FLASH_PAGES_TO_BE_PROTECTED                                                                              \
    (FLASH_WRProt_Pages20to23 | FLASH_WRProt_Pages24to27 | FLASH_WRProt_Pages28to31 | FLASH_WRProt_Pages32to35 | \
     FLASH_WRProt_Pages36to39 | FLASH_WRProt_Pages40to43 | FLASH_WRProt_Pages44to47 | FLASH_WRProt_Pages48to51 | \
     FLASH_WRProt_Pages52to55 | FLASH_WRProt_Pages56to59 | FLASH_WRProt_Pages60to63)

#define APPLICATION_ADDRESS PAGE_WRITE_START_ADDR
/* Global Variable */
__align(4) UINT8 RxBuffer[MAX_PACKET_SIZE];  // IN, must even address
__align(4) UINT8 TxBuffer[MAX_PACKET_SIZE];  // OUT, must even address
__align(4) UINT8 Com_Buffer[128];
typedef enum
{
    FAILED = 0,
    PASSED = !FAILED
} TestStatus;

volatile TestStatus MemoryProgramStatus = PASSED;
volatile TestStatus MemoryEraseStatus   = PASSED;

void IAP_Erase()
{
    UINT32 EraseCounter = 0x0;
    UINT32 WRPR_Value   = 0xFFFFFFFF;
    UINT16 NbrOfPage;
    UINT32 Address;
    FLASH_Status FLASHStatus     = FLASH_COMPLETE;
    TestStatus MemoryEraseStatus = PASSED;

    FLASH_Unlock();
    WRPR_Value = FLASH_GetWriteProtectionOptionByte();
    NbrOfPage  = (PAGE_WRITE_END_ADDR - PAGE_WRITE_START_ADDR) / FLASH_PAGE_SIZE;
    if ((WRPR_Value & FLASH_PAGES_TO_BE_PROTECTED) != 0)
    {
        FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

        for (EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
        {
            FLASHStatus = FLASH_ErasePage(PAGE_WRITE_START_ADDR + (FLASH_PAGE_SIZE * EraseCounter));
            if (FLASHStatus != FLASH_COMPLETE)
            {
                printf("FLASH Erase ERR at Page%ld\r\n", EraseCounter + 20);
            }
            printf("FLASH Erase Page%ld...\r\n", EraseCounter + 20);
        }
    }
    else
    {
        printf("Error to program the flash : The desired pages are write protected\r\n");
    }
    FLASH_Lock();
    Address = PAGE_WRITE_START_ADDR;
    printf("Erase Cheking...\r\n");
    while ((Address < PAGE_WRITE_END_ADDR) && (MemoryEraseStatus != FAILED))
    {
        if ((*(__IO uint16_t *)Address) != 0xFFFF)
        {
            MemoryEraseStatus = FAILED;
            printf("Address:%d,data:%04x\r\n", Address, (*(__IO uint16_t *)Address));
        }
        Address += 2;
    }
    printf("Erase MemoryEraseStatus:%d...\r\n", MemoryEraseStatus);
}

void IAP_PROM(UINT8 *DataBuf, UINT32 offset, UINT16 l)
{
    UINT32 Address;
    UINT16 len, Data;
    FLASH_Status FLASHStatus = FLASH_COMPLETE;
    len                      = (l + 1) >> 1;  //必须为2的整数倍，按照半字进行操作
    Address                  = PAGE_WRITE_START_ADDR + offset;
    FLASH_Unlock();
    printf("Programing..Address:%08x\r\n", Address);

    for (uint16_t i = 0; i < len; i++)
    {
        Data        = (DataBuf[2 * i] | ((UINT16)DataBuf[2 * i + 1] << 8));
        FLASHStatus = FLASH_ProgramHalfWord(Address, Data);
        Address     = Address + 2;
        if (FLASHStatus != FLASH_COMPLETE)
            break;
    }
    FLASH_Lock();
}
typedef void (*pFunction)(void);
pFunction Jump_To_Application;
void IAP_END(void)
{
    UINT32 JumpAddress;

    if (((*(__IO UINT32 *)APPLICATION_ADDRESS) & 0x2FFE0000) == 0x20000000)
    {
        printf("Jump:%08x\r\n", APPLICATION_ADDRESS);
        /* Jump to user application */
        JumpAddress         = *(__IO UINT32 *)(APPLICATION_ADDRESS + 4);
        Jump_To_Application = (pFunction)JumpAddress;
        /* Initialize user application's Stack Pointer */
        __set_MSP(*(__IO UINT32 *)APPLICATION_ADDRESS);
        Jump_To_Application();
    }
    else
    {
        printf("APPLICATION_ADDRESS:%08x\r\n", (*(__IO UINT32 *)APPLICATION_ADDRESS));
    }
}

/*******************************************************************************
 * Function Name  : main
 * Description    : Main program.
 * Input          : None
 * Return         : None
 *******************************************************************************/
int main()
{
    UINT8 s, i;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    USART_Printf_Init(115200);
    printf("Start @Chip_ID:%08x\r\n", DBGMCU->IDCODE);
    printf("USBHD   HOST Test\r\n");
    mDelaymS(500);
    Set_USBConfig();
    pHOST_RX_RAM_Addr = RxBuffer;
    pHOST_TX_RAM_Addr = TxBuffer;
    USB_HostInit();
    CH103LibInit();
    printf("Wait Device In\n");
    FoundNewDev = 0;
    printf("APPLICATION_ADDRESS:%08x\r\n", (*(__IO UINT32 *)APPLICATION_ADDRESS));
    // IAP_END();
    while (1)
    {
        UINT32 counter = 0;
        s              = ERR_SUCCESS;
        counter++;
        if (R8_USB_INT_FG & RB_UIF_DETECT)
        {
            printf("counter:%ld\n", counter);
            R8_USB_INT_FG = RB_UIF_DETECT;
            printf("RB_UIF_DETECT In\n");
            s = AnalyzeRootHub();
            if (s == ERR_USB_CONNECT)
                FoundNewDev = 1;
        }

        if (FoundNewDev || s == ERR_USB_CONNECT)
        {
            FoundNewDev = 0;
            mDelaymS(200);
            s = InitRootDevice(Com_Buffer);
            if (s == ERR_SUCCESS)
            {
                CH103DiskStatus = DISK_USB_ADDR;
                for (i = 0; i < 10; i++)
                {
                    printf("Wait DiskReady\n");
                    s = CH103DiskReady();
                    if (s == ERR_SUCCESS)
                    {
                        break;
                    }
                    else
                    {
                        printf("%02x\n", (UINT16)s);
                        mDelaymS(50);
                    }
                }
                if (CH103DiskStatus >= DISK_MOUNTED)
                {
                    strcpy((PCHAR)mCmdParam.Open.mPathName, "/DOWNLOAD.BIN");  //设置将要操作的文件路径和文件名
                    s = CH103FileOpen();                                       //打开文件
                    if (s == ERR_MISS_DIR || s == ERR_MISS_FILE)               //没有找到文件
                    {
                        printf("do not find file DOWNLOAD.BIN s:%x\n", s);
                    }
                    else
                    {
                        UINT32 TotalCount = CH103vFileSize;  //设置准备读取总长度100字节
                        printf("TotalCount:%ld\n", TotalCount);
                        IAP_Erase();
                        while (TotalCount)
                        {
                            UINT32 c;
                            UINT8 temp[512];
                            //如果文件比较大,一次读不完,可以再调用CH579ByteRead继续读取,文件指针自动向后移动
                            if (TotalCount > sizeof(temp))
                            {  //剩余数据较多,限制单次读写的长度不能超过 sizeof( mCmdParam.Other.mBuffer )
                                c = sizeof(temp);
                            }
                            else
                                c = TotalCount;                  //最后剩余的字节数
                            mCmdParam.ByteRead.mByteCount  = c;  //请求读出几十字节数据
                            mCmdParam.ByteRead.mByteBuffer = &temp[0];
                            printf("CH103ByteRead count:%ld\n", c);
                            s = CH103ByteRead();  //以字节为单位读取数据块,单次读写的长度不能超过MAX_BYTE_IO,第二次调用时接着刚才的向后读
                            IAP_PROM(mCmdParam.ByteRead.mByteBuffer, (CH103vFileSize - TotalCount), c);
                            TotalCount -= mCmdParam.ByteRead.mByteCount;  //计数,减去当前实际已经读出的字符数
                            // for (i = 0; i != mCmdParam.ByteRead.mByteCount; i++)
                            //     printf("%02x ", mCmdParam.ByteRead.mByteBuffer[i]);  //显示读出的字符
                            // printf("\n");
                            if (mCmdParam.ByteRead.mByteCount < c)
                            {  //实际读出的字符数少于要求读出的字符数,说明已经到文件的结尾
                                printf("\n");
                                printf("file end\n");
                                break;
                            }
                            IAP_END();
                        }
                    }
                    s = CH103FileClose();
                    IAP_END();
                }
            }
        }
    }
}
