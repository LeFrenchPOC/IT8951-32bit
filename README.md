<h1 align="center">Welcome to IT8951 32bit library 👋</h1>
<p>
  <a href="https://github.com/LeFrenchPOC/IT8951-32bit/blob/main/LICENSE" target="_blank">
    <img alt="License: MIT" src="https://img.shields.io/badge/License-MIT-green.svg" />
  </a>
</p>

> Waveshare IT8951 library adaptation for 32bit microcontroller under Arduino framework

## What is it?
Adaptation of the [Waveshare IT8951 library](https://github.com/waveshare/IT8951) for 32bit microcontroller under Arduino framework optimized for low memory. It uses 1 bit per pixel to divide by 8 the total capacity of one image (only 2 greyscale values). All functions and declarations of the buffers are done for 1 bit per pixel.

## How to adapt it to my microcontroller?
Juste redefine the `RESET` and `HDRY` constants to match your microcontroller. You can also change the configuration of the SPI interface in the init function.

## For which EPaper Screen
This library is designed for the Waveshare 6inch HD e-Paper display (aka. ED060KC1). But it can suit for all EPaper displays with the same interface (IT8951), you have just to check the values in the header file.

## How to create 1 bit per pixel (1bpp) images?
You can use the [xbpp-img2array](https://github.com/LeFrenchPOC/xbpp-img2array) utility to convert your image to a 1bpp array.

## Usage
```cpp
#include "it8951.h"

extern uint8_t* gpFrameBuf;
extern IT8951DevInfo gstI80DevInfo;

bool display_begin() {
    uint8_t err = IT8951_Init();
    return err == 0;
}

void display_buffer(const uint8_t* addr, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    gpFrameBuf = (uint8_t *)addr;
    IT8951AreaImgInfo stAreaImgInfo;

    IT8951WaitForDisplayReady();
    stAreaImgInfo.usX = x;
    stAreaImgInfo.usY = y;
    stAreaImgInfo.usWidth = w;
    stAreaImgInfo.usHeight = h;

    IT8951Load1bppImage(gpFrameBuf, stAreaImgInfo.usX, stAreaImgInfo.usY, stAreaImgInfo.usWidth, stAreaImgInfo.usHeight);
    IT8951DisplayArea1bpp(stAreaImgInfo.usX, stAreaImgInfo.usY, stAreaImgInfo.usWidth, stAreaImgInfo.usHeight, 0x01, 0x00, 0xFF);
}

void display_clear() {
    IT8951AreaImgInfo stAreaImgInfo;
    memset(gpFrameBuf, 0xff, (gstI80DevInfo.usPanelW * gstI80DevInfo.usPanelH) / 8);

    IT8951WaitForDisplayReady();

    stAreaImgInfo.usX = 0;
    stAreaImgInfo.usY = 0;
    stAreaImgInfo.usWidth = gstI80DevInfo.usPanelW;
    stAreaImgInfo.usHeight = gstI80DevInfo.usPanelH;

    IT8951Load1bppImage(gpFrameBuf, stAreaImgInfo.usX, stAreaImgInfo.usY, stAreaImgInfo.usWidth, stAreaImgInfo.usHeight);
    IT8951DisplayArea1bpp(0, 0, gstI80DevInfo.usPanelW, gstI80DevInfo.usPanelH, 0x00, 0x00, 0xFF);
}
```

## Author

👤 **Le French POC**

* Github: [@LeFrenchPOC](https://github.com/LeFrenchPOC)
* Website: [https://www.lefrenchpoc.fr/](https://www.lefrenchpoc.fr/)

## Show your support

Give a ⭐️ if this project helped you!

## 📝 License

Copyright © 2021 [Le French POC](https://github.com/LeFrenchPOC).<br />
This project is [MIT](https://github.com/LeFrenchPOC/IT8951-32bit/blob/main/LICENSE) licensed.