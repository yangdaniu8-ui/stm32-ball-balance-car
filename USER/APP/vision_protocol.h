#ifndef VISION_PROTOCOL_H
#define VISION_PROTOCOL_H
#include "app_core.h"
typedef struct {
    char buffer[24];
    uint8_t length;
    uint8_t discarding;
    uint32_t sequence;
    uint32_t rejected;
} VisionParser;
void VisionParser_Init(VisionParser *parser);
void VisionParser_Discard(VisionParser *parser);
int VisionParser_Push(VisionParser *parser, char ch, uint32_t timestamp_ms,
                      BallSample *sample);
#endif
