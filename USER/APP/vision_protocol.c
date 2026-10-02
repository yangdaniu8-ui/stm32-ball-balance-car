#include "vision_protocol.h"
#include <string.h>

/* Strict ASCII decimal parser: no atof(), no allocation, no NaN/Inf acceptance. */
static int parse_position(const char *text, unsigned int length, float *position)
{
    unsigned int i = 0, digits = 0;
    int sign = 1, fraction = 0;
    float value = 0.0f, scale = 0.1f;
    if (length && (text[0] == '+' || text[0] == '-')) {
        if (text[0] == '-') sign = -1;
        i++;
    }
    for (; i < length; ++i) {
        if (text[i] >= '0' && text[i] <= '9') {
            digits++;
            if (fraction) {
                value += (text[i] - '0') * scale;
                scale *= 0.1f;
            } else value = value * 10.0f + (text[i] - '0');
            if (value > APP_ROD_HALF_CM) return 0;
        } else if (text[i] == '.' && !fraction) fraction = 1;
        else return 0;
    }
    if (!digits) return 0;
    *position = sign * value;
    return 1;
}

void VisionParser_Init(VisionParser *parser)
{
    memset(parser, 0, sizeof(*parser));
}

void VisionParser_Discard(VisionParser *parser)
{
    parser->length = 0;
    parser->discarding = 1;
}

int VisionParser_Push(VisionParser *parser, char ch, uint32_t timestamp_ms,
                      BallSample *sample)
{
    float position;
    if (ch == '\r' || ch == '\n') {
        if (parser->discarding) {
            parser->discarding = 0;
            parser->length = 0;
            parser->rejected++;
            return 0;
        }
        if (!parser->length) return 0;
        if (!parse_position(parser->buffer, parser->length, &position)) {
            parser->length = 0;
            parser->rejected++;
            return 0;
        }
        parser->length = 0;
        sample->position_cm = position;
        sample->timestamp_ms = timestamp_ms;
        sample->sequence = ++parser->sequence;
        return 1;
    }
    if (parser->discarding) return 0;
    if (parser->length == sizeof(parser->buffer)) {
        VisionParser_Discard(parser);
        return 0;
    }
    parser->buffer[parser->length++] = ch;
    return 0;
}
