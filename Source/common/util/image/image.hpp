#ifndef UTIL_IMAGE_HPP
#define UTIL_IMAGE_HPP

#include <stdint.h>

void image_blit (
    void* destData,
    unsigned destW,
    unsigned destH,
    unsigned destBpp,
    void* srcData, 
    unsigned srcW, 
    unsigned srcH,
    unsigned srcBpp,
    unsigned xOffset,
    unsigned yOffset
) {
    unsigned char* dst = (unsigned char*)destData;
    unsigned char* src = (unsigned char*)srcData;
    for(unsigned y = 0; (y + yOffset) < destH && y < srcH; ++y)
    {
        for(unsigned x = 0; (x + xOffset) < destW && x < srcW; ++x)
        {
            unsigned index = ((y + yOffset) * destW + (x + xOffset)) * destBpp;
            unsigned srcIndex = (y * srcW + x) * srcBpp;
            for(unsigned b = 0; b < destBpp && b < srcBpp; ++b)
            {
                dst[index + b] = src[srcIndex + b];
            }
            if(destBpp == 4 && srcBpp < 4)
                dst[index + 3] = 255;
        }
    }
}


#endif
