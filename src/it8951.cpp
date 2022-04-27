#include "it8951.h"

IT8951DevInfo gstI80DevInfo;
uint8_t* gpFrameBuf;
uint32_t gulImgBufAddr;

SPISettings _spi_settings = SPISettings(24000000, MSBFIRST, SPI_MODE0);

uint8_t IT8951_Init() {
    pinMode(SS, OUTPUT);
    digitalWrite(SS, HIGH);
    pinMode(HRDY, INPUT);
    pinMode(RESET, OUTPUT);
    SPI.begin();

    Serial.printf("****** IT8951 ******\n");

    digitalWrite(RESET, LOW);
    delay(200);
    digitalWrite(RESET, HIGH);
    delay(200);
    LCDWaitForReady();

    GetIT8951SystemInfo(&gstI80DevInfo);
    
    if (!gstI80DevInfo.usPanelW || !gstI80DevInfo.usPanelH) {
        return 1;
    }

    gulImgBufAddr = gstI80DevInfo.usImgBufAddrL | ((uint32_t)gstI80DevInfo.usImgBufAddrH << 16);

    IT8951WriteReg(I80CPCR, 0x0001);
    
    if (VCOM != IT8951GetVCOM()) {
		IT8951SetVCOM(VCOM);
		Serial.printf("VCOM = -%.02fV\n",(float)IT8951GetVCOM()/1000);
	}

   
   gpFrameBuf = (uint8_t*) malloc((gstI80DevInfo.usPanelW * gstI80DevInfo.usPanelH) / 8);
   Serial.printf("Size of gpFrameBuf: %d\n", gstI80DevInfo.usPanelW * gstI80DevInfo.usPanelH / 8);
	if (!gpFrameBuf)
	{
		Serial.println("malloc error!");
		return 1;
	}
    

    return 0;
}

//-----------------------------------------------------------
// Host controller function 1---Wait for host data Bus Ready
//-----------------------------------------------------------
void LCDWaitForReady() {
    uint8_t ulData = digitalRead(HRDY);
    while (ulData == 0) {
        ulData = digitalRead(HRDY);
    }
}

//-----------------------------------------------------------
// Host controller function 2---Write command code to host data Bus
//-----------------------------------------------------------
void LCDWriteCmdCode(uint16_t usCmdCode) {
    // Set Preamble for Write Command
    uint16_t wPreamble = 0x6000;

    LCDWaitForReady();
    SPI.beginTransaction(_spi_settings);
    digitalWrite(SS, LOW);

    SPI.transfer(wPreamble >> 8);
    SPI.transfer(wPreamble);

    LCDWaitForReady();

    SPI.transfer(usCmdCode >> 8);
    SPI.transfer(usCmdCode);

    //delay_nop();

    digitalWrite(SS, HIGH);
    SPI.endTransaction();
}

//-----------------------------------------------------------
// Host controller function 3---Write Data to host data Bus
//-----------------------------------------------------------
void LCDWriteData(uint16_t usData) {
    // Set Preamble for Write Data
    uint16_t wPreamble = 0x0000;

    LCDWaitForReady();

    SPI.beginTransaction(_spi_settings);
    digitalWrite(SS, LOW);

    SPI.transfer(wPreamble >> 8);
    SPI.transfer(wPreamble);

    LCDWaitForReady();

    SPI.transfer(usData >> 8);
    SPI.transfer(usData);

    digitalWrite(SS, HIGH);
    SPI.endTransaction();
}

void LCDWriteNData(uint16_t* pwBuf, uint32_t ulSizeWordCnt) {
    uint32_t i;

    uint16_t wPreamble = 0x0000;

    LCDWaitForReady();

    SPI.beginTransaction(_spi_settings);
    digitalWrite(SS, LOW);

    SPI.transfer(wPreamble >> 8);
    SPI.transfer(wPreamble);

    LCDWaitForReady();

    for (i = 0; i < ulSizeWordCnt; i++) {
        SPI.transfer(pwBuf[i] >> 8);
        SPI.transfer(pwBuf[i]);
    }

    digitalWrite(SS, HIGH);
    SPI.endTransaction();
}

//-----------------------------------------------------------
// Host controller function 4---Read Data from host data Bus
//-----------------------------------------------------------
uint16_t LCDReadData() {
    uint16_t wRData;

    uint16_t wPreamble = 0x1000;

    LCDWaitForReady();

    SPI.beginTransaction(_spi_settings);
    digitalWrite(SS, LOW);

    SPI.transfer(wPreamble >> 8);
    SPI.transfer(wPreamble);

    LCDWaitForReady();

    wRData = SPI.transfer(0x00);  // dummy
    wRData = SPI.transfer(0x00);  // dummy

    LCDWaitForReady();

    wRData = SPI.transfer(0x00) << 8;
    wRData |= SPI.transfer(0x00);

    digitalWrite(SS, HIGH);
    SPI.endTransaction();

    return wRData;
}

//-----------------------------------------------------------
//  Read Burst N words Data
//-----------------------------------------------------------
void LCDReadNData(uint16_t* pwBuf, uint32_t ulSizeWordCnt) {
    uint32_t i;

    uint16_t wPreamble = 0x1000;

    LCDWaitForReady();

    SPI.beginTransaction(_spi_settings);
    digitalWrite(SS, LOW);

    SPI.transfer(wPreamble >> 8);
    SPI.transfer(wPreamble);

    LCDWaitForReady();

    pwBuf[0] = SPI.transfer(0x00);  // dummy
    pwBuf[0] = SPI.transfer(0x00);  // dummy

    LCDWaitForReady();

    for (i = 0; i < ulSizeWordCnt; i++) {
        pwBuf[i] = SPI.transfer(0x00) << 8;
        pwBuf[i] |= SPI.transfer(0x00);
    }

    digitalWrite(SS, HIGH);
    SPI.endTransaction();
}

//-----------------------------------------------------------
// Host controller function 5---Write command to host data Bus with aruments
//-----------------------------------------------------------
void LCDSendCmdArg(uint16_t usCmdCode, uint16_t* pArg, uint16_t usNumArg) {
    uint16_t i;
    // Send Cmd code
    LCDWriteCmdCode(usCmdCode);
    // Send Data
    for (i = 0; i < usNumArg; i++) {
        LCDWriteData(pArg[i]);
    }
}

//-----------------------------------------------------------
// Host Cmd 1---SYS_RUN
//-----------------------------------------------------------
void IT8951SystemRun() {
    LCDWriteCmdCode(IT8951_TCON_SYS_RUN);
}

//-----------------------------------------------------------
// Host Cmd 2---STANDBY
//-----------------------------------------------------------
void IT8951StandBy() {
    LCDWriteCmdCode(IT8951_TCON_STANDBY);
}

//-----------------------------------------------------------
// Host Cmd 3---SLEEP
//-----------------------------------------------------------
void IT8951Sleep() {
    LCDWriteCmdCode(IT8951_TCON_SLEEP);
}

//-----------------------------------------------------------
// Host Cmd 4---REG_RD
//-----------------------------------------------------------
uint16_t IT8951ReadReg(uint16_t usRegAddr) {
    uint16_t usData;

    // Send Cmd and Register Address
    LCDWriteCmdCode(IT8951_TCON_REG_RD);
    LCDWriteData(usRegAddr);
    // Read data from Host Data bus
    usData = LCDReadData();
    return usData;
}
//-----------------------------------------------------------
// Host Cmd 5---REG_WR
//-----------------------------------------------------------
void IT8951WriteReg(uint16_t usRegAddr, uint16_t usValue) {
    // Send Cmd , Register Address and Write Value
    LCDWriteCmdCode(IT8951_TCON_REG_WR);
    LCDWriteData(usRegAddr);
    LCDWriteData(usValue);
}

//-----------------------------------------------------------
// Host Cmd 6---MEM_BST_RD_T
//-----------------------------------------------------------
void IT8951MemBurstReadTrigger(uint32_t ulMemAddr, uint32_t ulReadSize) {
    uint16_t usArg[4];
    // Setting Arguments for Memory Burst Read
    usArg[0] = (uint16_t)(ulMemAddr & 0x0000FFFF);           // addr[15:0]
    usArg[1] = (uint16_t)((ulMemAddr >> 16) & 0x0000FFFF);   // addr[25:16]
    usArg[2] = (uint16_t)(ulReadSize & 0x0000FFFF);          // Cnt[15:0]
    usArg[3] = (uint16_t)((ulReadSize >> 16) & 0x0000FFFF);  // Cnt[25:16]
    // Send Cmd and Arg
    LCDSendCmdArg(IT8951_TCON_MEM_BST_RD_T, usArg, 4);
}
//-----------------------------------------------------------
// Host Cmd 7---MEM_BST_RD_S
//-----------------------------------------------------------
void IT8951MemBurstReadStart() {
    LCDWriteCmdCode(IT8951_TCON_MEM_BST_RD_S);
}
//-----------------------------------------------------------
// Host Cmd 8---MEM_BST_WR
//-----------------------------------------------------------
void IT8951MemBurstWrite(uint32_t ulMemAddr, uint32_t ulWriteSize) {
    uint16_t usArg[4];
    // Setting Arguments for Memory Burst Write
    usArg[0] = (uint16_t)(ulMemAddr & 0x0000FFFF);            // addr[15:0]
    usArg[1] = (uint16_t)((ulMemAddr >> 16) & 0x0000FFFF);    // addr[25:16]
    usArg[2] = (uint16_t)(ulWriteSize & 0x0000FFFF);          // Cnt[15:0]
    usArg[3] = (uint16_t)((ulWriteSize >> 16) & 0x0000FFFF);  // Cnt[25:16]
    // Send Cmd and Arg
    LCDSendCmdArg(IT8951_TCON_MEM_BST_WR, usArg, 4);
}
//-----------------------------------------------------------
// Host Cmd 9---MEM_BST_END
//-----------------------------------------------------------
void IT8951MemBurstEnd(void) {
    LCDWriteCmdCode(IT8951_TCON_MEM_BST_END);
}

uint16_t IT8951GetVCOM(void) {
	uint16_t vcom;
	
	LCDWriteCmdCode(USDEF_I80_CMD_VCOM);
	LCDWriteData(0);
	//Read data from Host Data bus
	vcom = LCDReadData();
	return vcom;
}

void IT8951SetVCOM(uint16_t vcom) {
	LCDWriteCmdCode(USDEF_I80_CMD_VCOM);
	LCDWriteData(1);
	//Read data from Host Data bus
	LCDWriteData(vcom);
}


//-----------------------------------------------------------
// Example of Memory Burst Write
//-----------------------------------------------------------
// ****************************************************************************************
// Function name: IT8951MemBurstWriteProc( )
//
// Description:
//   IT8951 Burst Write procedure
//
// Arguments:
//      uint32_t ulMemAddr: IT8951 Memory Target Address
//      uint32_t ulWriteSize: Write Size (Unit: Word)
//      uint8_t* pDestBuf - Buffer of Sent data
// Return Values:
//   NULL.
// Note:
//
// ****************************************************************************************
void IT8951MemBurstWriteProc(uint32_t ulMemAddr, uint32_t ulWriteSize, uint16_t* pSrcBuf) {
    uint32_t i;

    // Send Burst Write Start Cmd and Args
    IT8951MemBurstWrite(ulMemAddr, ulWriteSize);

    // Burst Write Data
    for (i = 0; i < ulWriteSize; i++) {
        LCDWriteData(pSrcBuf[i]);
    }

    // Send Burst End Cmd
    IT8951MemBurstEnd();
}

// ****************************************************************************************
// Function name: IT8951MemBurstReadProc( )
//
// Description:
//   IT8951 Burst Read procedure
//
// Arguments:
//      uint32_t ulMemAddr: IT8951 Read Memory Address
//      uint32_t ulReadSize: Read Size (Unit: Word)
//      uint8_t* pDestBuf - Buffer for storing Read data
// Return Values:
//   NULL.
// Note:
//
// ****************************************************************************************
void IT8951MemBurstReadProc(uint32_t ulMemAddr, uint32_t ulReadSize, uint16_t* pDestBuf) {
    // Send Burst Read Start Cmd and Args
    IT8951MemBurstReadTrigger(ulMemAddr, ulReadSize);

    // Burst Read Fire
    IT8951MemBurstReadStart();

    // Burst Read Request for SPI interface only
    LCDReadNData(pDestBuf, ulReadSize);

    // Send Burst End Cmd
    IT8951MemBurstEnd();  // the same with IT8951MemBurstEnd()
}

//-----------------------------------------------------------
// Host Cmd 10---LD_IMG
//-----------------------------------------------------------
void IT8951LoadImgStart(IT8951LdImgInfo* pstLdImgInfo) {
    uint16_t usArg;
    // Setting Argument for Load image start
    usArg = (pstLdImgInfo->usEndianType << 8) | (pstLdImgInfo->usPixelFormat << 4) | (pstLdImgInfo->usRotate);
    // Send Cmd
    LCDWriteCmdCode(IT8951_TCON_LD_IMG);
    // Send Arg
    LCDWriteData(usArg);
}
//-----------------------------------------------------------
// Host Cmd 11---LD_IMG_AREA
//-----------------------------------------------------------
void IT8951LoadImgAreaStart(IT8951LdImgInfo* pstLdImgInfo, IT8951AreaImgInfo* pstAreaImgInfo) {
    uint16_t usArg[5];
    // Setting Argument for Load image start
    usArg[0] = (pstLdImgInfo->usEndianType << 8) | (pstLdImgInfo->usPixelFormat << 4) | (pstLdImgInfo->usRotate);
    usArg[1] = pstAreaImgInfo->usX;
    usArg[2] = pstAreaImgInfo->usY;
    usArg[3] = pstAreaImgInfo->usWidth;
    usArg[4] = pstAreaImgInfo->usHeight;
    // Send Cmd and Args
    LCDSendCmdArg(IT8951_TCON_LD_IMG_AREA, usArg, 5);
}
//-----------------------------------------------------------
// Host Cmd 12---LD_IMG_END
//-----------------------------------------------------------
void IT8951LoadImgEnd(void) {
    LCDWriteCmdCode(IT8951_TCON_LD_IMG_END);
}

void GetIT8951SystemInfo(void* pBuf) {
    uint16_t* pusWord = (uint16_t*)pBuf;
    IT8951DevInfo* pstDevInfo;

    // Send I80 CMD
    LCDWriteCmdCode(USDEF_I80_CMD_GET_DEV_INFO);

    // Burst Read Request for SPI interface only
    LCDReadNData(pusWord, sizeof(IT8951DevInfo) / 2);  // Polling HRDY for each words(2-bytes) if possible

    // Show Device information of IT8951
    pstDevInfo = (IT8951DevInfo*)pBuf;
    Serial.printf("Panel(W,H) = (%d,%d)\r\n",
           pstDevInfo->usPanelW, pstDevInfo->usPanelH);
    Serial.printf("Image Buffer Address = %X\r\n",
           pstDevInfo->usImgBufAddrL | (pstDevInfo->usImgBufAddrH << 16));
    // Show Firmware and LUT Version
    Serial.printf("FW Version = %s\r\n", (uint8_t*)pstDevInfo->usFWVersion);
    Serial.printf("LUT Version = %s\r\n", (uint8_t*)pstDevInfo->usLUTVersion);
}

//-----------------------------------------------------------
// Initial function 2---Set Image buffer base address
//-----------------------------------------------------------
void IT8951SetImgBufBaseAddr(uint32_t ulImgBufAddr) {
    uint16_t usWordH = (uint16_t)((ulImgBufAddr >> 16) & 0x0000FFFF);
    uint16_t usWordL = (uint16_t)(ulImgBufAddr & 0x0000FFFF);
    // Write LISAR Reg
    IT8951WriteReg(LISAR + 2, usWordH);
    IT8951WriteReg(LISAR, usWordL);
}

//-----------------------------------------------------------
// 3.6. Display Functions
//-----------------------------------------------------------

//-----------------------------------------------------------
// Display function 1---Wait for LUT Engine Finish
//                     Polling Display Engine Ready by LUTNo
//-----------------------------------------------------------
void IT8951WaitForDisplayReady() {
    // Check IT8951 Register LUTAFSR => NonZero Busy, 0 - Free
    while (IT8951ReadReg(LUTAFSR))
        ;
}

//-----------------------------------------------------------
// Display function 2---Load Image Area process
//-----------------------------------------------------------
void IT8951HostAreaPackedPixelWrite(IT8951LdImgInfo* pstLdImgInfo, IT8951AreaImgInfo* pstAreaImgInfo) {
    uint32_t i, j;
    // Source buffer address of Host
    uint16_t* pusFrameBuf = (uint16_t*)pstLdImgInfo->ulStartFBAddr;

    // Set Image buffer(IT8951) Base address
    IT8951SetImgBufBaseAddr(pstLdImgInfo->ulImgBufBaseAddr);
    // Send Load Image start Cmd
    IT8951LoadImgAreaStart(pstLdImgInfo, pstAreaImgInfo);
    // Host Write Data
    for (j = 0; j < pstAreaImgInfo->usHeight; j++) {
        for (i = 0; i < pstAreaImgInfo->usWidth / 2; i++) {
            // Write a Word(2-Bytes) for each time
            LCDWriteData(*pusFrameBuf);
            //LCDWriteData(0x0000);
            pusFrameBuf++;
        }
    }
    // Send Load Img End Command
    IT8951LoadImgEnd();
}

//-----------------------------------------------------------
// Display functions 3---Application for Display panel Area
//-----------------------------------------------------------
void IT8951DisplayArea(uint16_t usX, uint16_t usY, uint16_t usW, uint16_t usH, uint16_t usDpyMode) {
    // Send I80 Display Command (User defined command of IT8951)
    LCDWriteCmdCode(USDEF_I80_CMD_DPY_AREA);  // 0x0034
    // Write arguments
    LCDWriteData(usX);
    LCDWriteData(usY);
    LCDWriteData(usW);
    LCDWriteData(usH);
    LCDWriteData(usDpyMode);
}

// Display Area with bitmap on EPD
//-----------------------------------------------------------
//  Display Function 4---for Display Area for 1-bpp mode format
//    the bitmap(1bpp) mode will be enable when Display
//    and restore to Default setting (disable) after displaying finished
//-----------------------------------------------------------
void IT8951DisplayArea1bpp(uint16_t usX, uint16_t usY, uint16_t usW, uint16_t usH, uint16_t usDpyMode, uint8_t ucBGGrayVal, uint8_t ucFGGrayVal) {
    // Set Display mode to 1 bpp mode - Set 0x18001138 Bit[18](0x1800113A Bit[2])to 1
    IT8951WriteReg(UP1SR + 2, IT8951ReadReg(UP1SR + 2) | (1 << 2));

    // Set BitMap color table 0 and 1 , => Set Register[0x18001250]:
    // Bit[7:0]: ForeGround Color(G0~G15)  for 1
    // Bit[15:8]:Background Color(G0~G15)  for 0
    IT8951WriteReg(BGVR, (ucBGGrayVal << 8) | ucFGGrayVal);

    // Display
    IT8951DisplayArea(usX, usY, usW, usH, usDpyMode);
    IT8951WaitForDisplayReady();

    // Restore to normal mode
    IT8951WriteReg(UP1SR + 2, IT8951ReadReg(UP1SR + 2) & ~(1 << 2));
}

//-------------------------------------------------------------------------------------------------------------
//  Command - 0x0037 for Display Base addr by User
//  uint32_t ulDpyBufAddr - Host programmer need to indicate the Image buffer address of IT8951
//                                         In current case, there is only one image buffer in IT8951 so far.
//                                         So Please set the Image buffer address you got  in initial stage.
//                                         (gulImgBufAddr by Get device information 0x0302 command)
//
//-------------------------------------------------------------------------------------------------------------
void IT8951DisplayAreaBuf(uint16_t usX, uint16_t usY, uint16_t usW, uint16_t usH, uint16_t usDpyMode, uint32_t ulDpyBufAddr) {
    // Send I80 Display Command (User defined command of IT8951)
    LCDWriteCmdCode(USDEF_I80_CMD_DPY_BUF_AREA);  // 0x0037

    // Write arguments
    LCDWriteData(usX);
    LCDWriteData(usY);
    LCDWriteData(usW);
    LCDWriteData(usH);
    LCDWriteData(usDpyMode);
    LCDWriteData((uint16_t)ulDpyBufAddr);          // Display Buffer Base address[15:0]
    LCDWriteData((uint16_t)(ulDpyBufAddr >> 16));  // Display Buffer Base address[26:16]
}


void IT8951_Cancel() {
    free(gpFrameBuf);
}

//-----------------------------------------------------------
// Load 1bpp image flow (must display with IT8951DisplayArea1bpp()
//-----------------------------------------------------------

void IT8951Load1bppImage(uint8_t* p1bppImgBuf, uint16_t usX, uint16_t usY, uint16_t usW, uint16_t usH) {
    IT8951LdImgInfo stLdImgInfo;
    IT8951AreaImgInfo stAreaImgInfo;

    // Setting Load image information
    stLdImgInfo.ulStartFBAddr = (uint32_t)p1bppImgBuf;
    stLdImgInfo.usEndianType = IT8951_LDIMG_L_ENDIAN;
    stLdImgInfo.usPixelFormat = IT8951_8BPP;  // we use 8bpp because IT8951 dose not support 1bpp mode for load image?Aso we use Load 8bpp mode ,but the transfer size needs to be reduced to Size/8
    stLdImgInfo.usRotate = IT8951_ROTATE_0;
    stLdImgInfo.ulImgBufBaseAddr = gulImgBufAddr;
    // Set Load Area
    stAreaImgInfo.usX = usX / 8;
    stAreaImgInfo.usY = usY;
    stAreaImgInfo.usWidth = usW / 8;  // 1bpp, Chaning Transfer size setting to 1/8X of 8bpp mode
    stAreaImgInfo.usHeight = usH;
    Serial.printf("IT8951HostAreaPackedPixelWrite [wait]\n\r");
    // Load Image from Host to IT8951 Image Buffer
    IT8951HostAreaPackedPixelWrite(&stLdImgInfo, &stAreaImgInfo);  // Display function 2
}


void EPD_Clear(uint8_t Color) {
    //memset(gpFrameBuf, Color, gstI80DevInfo.usPanelW * gstI80DevInfo.usPanelH);
}