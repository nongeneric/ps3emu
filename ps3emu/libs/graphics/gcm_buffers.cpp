#include "gcm.h"

uint8_t gcmFlipCommands[] = {
    0x00, 0x04, 0xe9, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x60,
    0x56, 0x61, 0x66, 0x61, 0x00, 0x04, 0x00, 0x64, 0x00, 0x00, 0x00, 0x30,
    0x00, 0x04, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x64,
    0x00, 0x00, 0x00, 0x30, 0x00, 0x04, 0x00, 0x68, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x02, 0x00, 0x04, 0x00, 0x64, 0x00, 0x00, 0x00, 0x10,
    0x00, 0x04, 0x00, 0x6c, 0xff, 0xff, 0xff, 0xff, 0x00, 0x04, 0xe9, 0x24,
    0x80, 0x00, 0x01, 0x0f, 0x00, 0x04, 0x00, 0x64, 0x00, 0x00, 0x00, 0xf0,
    0x00, 0x04, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x0f,
};

uint8_t gcmResetCommands[] = {
    0x00, 0x04, 0x00, 0x00, 0x31, 0x33, 0x70, 0x00, 0x00, 0x3c, 0x01, 0x80, 
    0x66, 0x60, 0x42, 0x00, 0xfe, 0xed, 0x00, 0x00, 0xfe, 0xed, 0x00, 0x01, 
    0xfe, 0xed, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xed, 0x00, 0x00, 
    0xfe, 0xed, 0x00, 0x00, 0xfe, 0xed, 0x00, 0x00, 0xfe, 0xed, 0x00, 0x01, 
    0x66, 0x60, 0x66, 0x60, 0x66, 0x62, 0x66, 0x60, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0xfe, 0xed, 0x00, 0x00, 0xfe, 0xed, 0x00, 0x00, 
    0x00, 0x04, 0x00, 0x60, 0x66, 0x61, 0x66, 0x61, 0x00, 0x34, 0x02, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x21, 
    0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x40, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x02, 0x80, 0x00, 0x00, 0x00, 0x40, 
    0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x1d, 0x80, 0x00, 0x00, 0x00, 0x03, 0x00, 0x48, 0x02, 0xb8, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 
    0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 
    0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 
    0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 
    0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 
    0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 
    0x00, 0x08, 0x1d, 0x98, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 
    0x00, 0x04, 0x1d, 0xa4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x03, 0xb0, 
    0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x14, 0x54, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x1f, 0xf4, 0x00, 0x3f, 0xff, 0xff, 0x00, 0x18, 0x1f, 0xc0, 
    0x00, 0x00, 0x00, 0x00, 0x06, 0x14, 0x43, 0x21, 0xed, 0xcb, 0xa9, 0x87, 
    0x00, 0x00, 0x00, 0x6f, 0x00, 0x17, 0x16, 0x15, 0x00, 0x1b, 0x1a, 0x19, 
    0x00, 0x28, 0x0b, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x0a, 0x0c, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0a, 0x60, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x0a, 0x78, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x14, 0x28, 
    0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x1d, 0x88, 0x00, 0x00, 0x10, 0x00, 
    0x00, 0x04, 0x1e, 0x94, 0x00, 0x00, 0x00, 0x11, 0x00, 0x04, 0x14, 0x50, 
    0x00, 0x08, 0x00, 0x03, 0x00, 0x04, 0x1d, 0x64, 0x02, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x14, 0x5c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x1f, 0xe0, 
    0x00, 0x00, 0x00, 0x01, 0x00, 0x40, 0x0b, 0x00, 0x00, 0x00, 0x2d, 0xc8, 
    0x00, 0x00, 0x2d, 0xc8, 0x00, 0x00, 0x2d, 0xc8, 0x00, 0x00, 0x2d, 0xc8, 
    0x00, 0x00, 0x2d, 0xc8, 0x00, 0x00, 0x2d, 0xc8, 0x00, 0x00, 0x2d, 0xc8, 
    0x00, 0x00, 0x2d, 0xc8, 0x00, 0x00, 0x2d, 0xc8, 0x00, 0x00, 0x2d, 0xc8, 
    0x00, 0x00, 0x2d, 0xc8, 0x00, 0x00, 0x2d, 0xc8, 0x00, 0x00, 0x2d, 0xc8, 
    0x00, 0x00, 0x2d, 0xc8, 0x00, 0x00, 0x2d, 0xc8, 0x00, 0x00, 0x2d, 0xc8, 
    0x00, 0x10, 0x08, 0xcc, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x02, 0x40, 
    0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0xc0, 0x00, 0x01, 0x01, 0x01, 
    0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 
    0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 
    0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 
    0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 
    0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 
    0x00, 0x00, 0x74, 0x21, 0x00, 0x00, 0x74, 0x21, 0x00, 0x00, 0x74, 0x21, 
    0x00, 0x00, 0x74, 0x21, 0x00, 0x00, 0x74, 0x21, 0x00, 0x00, 0x74, 0x21, 
    0x00, 0x00, 0x74, 0x21, 0x00, 0x00, 0x74, 0x21, 0x00, 0x00, 0x74, 0x21, 
    0x00, 0x00, 0x74, 0x21, 0x00, 0x00, 0x74, 0x21, 0x00, 0x00, 0x74, 0x21, 
    0x00, 0x00, 0x74, 0x21, 0x00, 0x00, 0x74, 0x21, 0x00, 0x00, 0x74, 0x21, 
    0x00, 0x00, 0x74, 0x21, 0x9a, 0xab, 0xaa, 0x98, 0x66, 0x66, 0x67, 0x89, 
    0x98, 0x76, 0x66, 0x66, 0x89, 0xaa, 0xba, 0xa9, 0x99, 0x99, 0x99, 0x99, 
    0x88, 0x88, 0x88, 0x89, 0x98, 0x88, 0x88, 0x88, 0x99, 0x99, 0x99, 0x99, 
    0x56, 0x67, 0x66, 0x54, 0x33, 0x33, 0x33, 0x45, 0x54, 0x33, 0x33, 0x33, 
    0x45, 0x66, 0x76, 0x65, 0xaa, 0xbb, 0xba, 0x99, 0x66, 0x66, 0x78, 0x99, 
    0x99, 0x87, 0x66, 0x66, 0x99, 0xab, 0xbb, 0xaa, 0x00, 0x08, 0x17, 0x38, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xe0, 0x00, 
    0xca, 0xfe, 0xba, 0xbe, 0x00, 0x08, 0x03, 0x08, 0x00, 0x00, 0x02, 0x07, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x0c, 0x03, 0x50, 0x00, 0x00, 0x02, 0x07, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0xff, 0x00, 0x04, 0x03, 0x4c, 0x00, 0x00, 0x00, 0xff, 
    0x00, 0x0c, 0x03, 0x5c, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x1e, 0x00, 
    0x00, 0x00, 0x1e, 0x00, 0x00, 0x04, 0x03, 0x1c, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x03, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x03, 0x10, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x03, 0x6c, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x03, 0x20, 0x80, 0x06, 0x80, 0x06, 0x00, 0x08, 0x03, 0x14, 
    0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1d, 0x8c, 
    0xff, 0xff, 0xff, 0x00, 0x00, 0x04, 0x1d, 0x94, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x03, 0x24, 
    0x01, 0x01, 0x01, 0x01, 0x00, 0x04, 0x18, 0x3c, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x18, 0x30, 0x00, 0x00, 0x04, 0x05, 0x00, 0x08, 0x03, 0x84, 
    0x00, 0x00, 0x00, 0x00, 0x3f, 0x80, 0x00, 0x00, 0x00, 0x04, 0x03, 0x80, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x0a, 0x6c, 0x00, 0x00, 0x02, 0x01, 
    0x00, 0x04, 0x0a, 0x70, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x0a, 0x74, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 
    0x00, 0x04, 0x1f, 0xec, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1f, 0xc0, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x18, 0x34, 0x00, 0x00, 0x09, 0x01, 
    0x00, 0x04, 0x03, 0xb8, 0x00, 0x00, 0x00, 0x08, 0x00, 0x04, 0x03, 0x74, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x03, 0x78, 0x00, 0x00, 0x15, 0x03, 
    0x00, 0x04, 0x1e, 0xe0, 0x3f, 0x80, 0x00, 0x00, 0x00, 0x04, 0x0a, 0x68, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x0a, 0x78, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1d, 0xac, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x1d, 0xb0, 0xff, 0xff, 0xff, 0xff, 0x00, 0x08, 0x08, 0xc0, 
    0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x04, 0x03, 0x68, 
    0x00, 0x00, 0x1d, 0x01, 0x00, 0x0c, 0x03, 0x30, 0x00, 0x00, 0x02, 0x07, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x04, 0x03, 0x2c, 
    0x00, 0x00, 0x00, 0xff, 0x00, 0x0c, 0x03, 0x3c, 0x00, 0x00, 0x1e, 0x00, 
    0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x04, 0x03, 0x28, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1a, 0x08, 0x00, 0x03, 0x01, 0x01, 
    0x00, 0x04, 0x1a, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1a, 0x0c, 
    0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x1a, 0x14, 0x02, 0x05, 0x20, 0x00, 
    0x00, 0x04, 0x1a, 0x28, 0x00, 0x03, 0x01, 0x01, 0x00, 0x04, 0x1a, 0x3c, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1a, 0x2c, 0x00, 0x06, 0x00, 0x00, 
    0x00, 0x04, 0x1a, 0x34, 0x02, 0x05, 0x20, 0x00, 0x00, 0x04, 0x1a, 0x48, 
    0x00, 0x03, 0x01, 0x01, 0x00, 0x04, 0x1a, 0x5c, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x1a, 0x4c, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x1a, 0x54, 
    0x02, 0x05, 0x20, 0x00, 0x00, 0x04, 0x1a, 0x68, 0x00, 0x03, 0x01, 0x01, 
    0x00, 0x04, 0x1a, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1a, 0x6c, 
    0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x1a, 0x74, 0x02, 0x05, 0x20, 0x00, 
    0x00, 0x04, 0x1a, 0x88, 0x00, 0x03, 0x01, 0x01, 0x00, 0x04, 0x1a, 0x9c, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1a, 0x8c, 0x00, 0x06, 0x00, 0x00, 
    0x00, 0x04, 0x1a, 0x94, 0x02, 0x05, 0x20, 0x00, 0x00, 0x04, 0x1a, 0xa8, 
    0x00, 0x03, 0x01, 0x01, 0x00, 0x04, 0x1a, 0xbc, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x1a, 0xac, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x1a, 0xb4, 
    0x02, 0x05, 0x20, 0x00, 0x00, 0x04, 0x1a, 0xc8, 0x00, 0x03, 0x01, 0x01, 
    0x00, 0x04, 0x1a, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1a, 0xcc, 
    0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x1a, 0xd4, 0x02, 0x05, 0x20, 0x00, 
    0x00, 0x04, 0x1a, 0xe8, 0x00, 0x03, 0x01, 0x01, 0x00, 0x04, 0x1a, 0xfc, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1a, 0xec, 0x00, 0x06, 0x00, 0x00, 
    0x00, 0x04, 0x1a, 0xf4, 0x02, 0x05, 0x20, 0x00, 0x00, 0x04, 0x1b, 0x08, 
    0x00, 0x03, 0x01, 0x01, 0x00, 0x04, 0x1b, 0x1c, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x1b, 0x0c, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x1b, 0x14, 
    0x02, 0x05, 0x20, 0x00, 0x00, 0x04, 0x1b, 0x28, 0x00, 0x03, 0x01, 0x01, 
    0x00, 0x04, 0x1b, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1b, 0x2c, 
    0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x1b, 0x34, 0x02, 0x05, 0x20, 0x00, 
    0x00, 0x04, 0x1b, 0x48, 0x00, 0x03, 0x01, 0x01, 0x00, 0x04, 0x1b, 0x5c, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1b, 0x4c, 0x00, 0x06, 0x00, 0x00, 
    0x00, 0x04, 0x1b, 0x54, 0x02, 0x05, 0x20, 0x00, 0x00, 0x04, 0x1b, 0x68, 
    0x00, 0x03, 0x01, 0x01, 0x00, 0x04, 0x1b, 0x7c, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x1b, 0x6c, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x1b, 0x74, 
    0x02, 0x05, 0x20, 0x00, 0x00, 0x04, 0x1b, 0x88, 0x00, 0x03, 0x01, 0x01, 
    0x00, 0x04, 0x1b, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1b, 0x8c, 
    0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x1b, 0x94, 0x02, 0x05, 0x20, 0x00, 
    0x00, 0x04, 0x1b, 0xa8, 0x00, 0x03, 0x01, 0x01, 0x00, 0x04, 0x1b, 0xbc, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1b, 0xac, 0x00, 0x06, 0x00, 0x00, 
    0x00, 0x04, 0x1b, 0xb4, 0x02, 0x05, 0x20, 0x00, 0x00, 0x04, 0x1b, 0xc8, 
    0x00, 0x03, 0x01, 0x01, 0x00, 0x04, 0x1b, 0xdc, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x1b, 0xcc, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x1b, 0xd4, 
    0x02, 0x05, 0x20, 0x00, 0x00, 0x04, 0x1b, 0xe8, 0x00, 0x03, 0x01, 0x01, 
    0x00, 0x04, 0x1b, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1b, 0xec, 
    0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x1b, 0xf4, 0x02, 0x05, 0x20, 0x00, 
    0x00, 0x04, 0x03, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x17, 0x40, 
    0x00, 0x00, 0x00, 0x02, 0x00, 0x04, 0x16, 0x80, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x17, 0x44, 0x00, 0x00, 0x00, 0x02, 0x00, 0x04, 0x16, 0x84, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x17, 0x48, 0x00, 0x00, 0x00, 0x02, 
    0x00, 0x04, 0x16, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x17, 0x4c, 
    0x00, 0x00, 0x00, 0x02, 0x00, 0x04, 0x16, 0x8c, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x17, 0x50, 0x00, 0x00, 0x00, 0x02, 0x00, 0x04, 0x16, 0x90, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x17, 0x54, 0x00, 0x00, 0x00, 0x02, 
    0x00, 0x04, 0x16, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x17, 0x58, 
    0x00, 0x00, 0x00, 0x02, 0x00, 0x04, 0x16, 0x98, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x17, 0x5c, 0x00, 0x00, 0x00, 0x02, 0x00, 0x04, 0x16, 0x9c, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x17, 0x60, 0x00, 0x00, 0x00, 0x02, 
    0x00, 0x04, 0x16, 0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x17, 0x64, 
    0x00, 0x00, 0x00, 0x02, 0x00, 0x04, 0x16, 0xa4, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x17, 0x68, 0x00, 0x00, 0x00, 0x02, 0x00, 0x04, 0x16, 0xa8, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x17, 0x6c, 0x00, 0x00, 0x00, 0x02, 
    0x00, 0x04, 0x16, 0xac, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x17, 0x70, 
    0x00, 0x00, 0x00, 0x02, 0x00, 0x04, 0x16, 0xb0, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x17, 0x74, 0x00, 0x00, 0x00, 0x02, 0x00, 0x04, 0x16, 0xb4, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x17, 0x78, 0x00, 0x00, 0x00, 0x02, 
    0x00, 0x04, 0x16, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x17, 0x7c, 
    0x00, 0x00, 0x00, 0x02, 0x00, 0x04, 0x16, 0xbc, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x08, 0x0a, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 
    0x00, 0x08, 0x03, 0x94, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x80, 0x00, 0x00, 
    0x00, 0x20, 0x0a, 0x20, 0x45, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 
    0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 
    0x45, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x20, 0x0a, 0x20, 0x45, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 
    0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 
    0x45, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x1d, 0x7c, 0xff, 0xff, 0x00, 0x00, 0x00, 0x04, 0x18, 0x2c, 
    0x00, 0x00, 0x1b, 0x02, 0x00, 0x04, 0x1d, 0x90, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x03, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x18, 0x28, 
    0x00, 0x00, 0x1b, 0x02, 0x00, 0x04, 0x03, 0xbc, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x1d, 0xb4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1e, 0xe4, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1e, 0xe8, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x18, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x14, 0x7c, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1e, 0x98, 0x01, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x14, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1f, 0xf0, 
    0x00, 0x00, 0xff, 0xff, 0x00, 0x04, 0x17, 0xcc, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x09, 0x08, 0x00, 0x00, 0x01, 0x01, 0x00, 0x04, 0x09, 0x1c, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x09, 0x0c, 0x00, 0x06, 0x00, 0x00, 
    0x00, 0x04, 0x09, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x09, 0x28, 
    0x00, 0x00, 0x01, 0x01, 0x00, 0x04, 0x09, 0x3c, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x09, 0x2c, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x09, 0x34, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x09, 0x48, 0x00, 0x00, 0x01, 0x01, 
    0x00, 0x04, 0x09, 0x5c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x09, 0x4c, 
    0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x09, 0x54, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x09, 0x68, 0x00, 0x00, 0x01, 0x01, 0x00, 0x04, 0x09, 0x7c, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x09, 0x6c, 0x00, 0x06, 0x00, 0x00, 
    0x00, 0x04, 0x09, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x02, 0x38, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1d, 0x78, 0x00, 0x00, 0x00, 0x01, 
    0x00, 0x04, 0x14, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1f, 0xf8, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1f, 0xe8, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x20, 0x00, 0x31, 0x33, 0x73, 0x03, 0x00, 0x0c, 0x21, 0x80, 
    0x66, 0x60, 0x42, 0x00, 0xfe, 0xed, 0x00, 0x01, 0xfe, 0xed, 0x00, 0x00, 
    0x00, 0x04, 0x60, 0x00, 0x31, 0x33, 0x71, 0xc3, 0x00, 0x0c, 0x61, 0x80, 
    0x66, 0x60, 0x42, 0x00, 0xfe, 0xed, 0x00, 0x00, 0xfe, 0xed, 0x00, 0x00, 
    0x00, 0x04, 0xa0, 0x00, 0x31, 0x33, 0x78, 0x08, 0x00, 0x20, 0xa1, 0x80, 
    0x66, 0x60, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x31, 0x33, 0x71, 0xc3, 0x00, 0x08, 0xa2, 0xfc, 
    0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x04, 0x80, 0x00, 
    0x31, 0x33, 0x7a, 0x73, 0x00, 0x08, 0x81, 0x80, 0x66, 0x60, 0x42, 0x00, 
    0xfe, 0xed, 0x00, 0x00, 0x00, 0x04, 0xc0, 0x00, 0x31, 0x37, 0xaf, 0x00, 
    0x00, 0x04, 0xc1, 0x80, 0x66, 0x60, 0x42, 0x00, 0x00, 0x02, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00,
};

uint8_t gcmInitCommands[] = {
    0x00, 0x04, 0x1a, 0x10, 0x00, 0x00, 0xaa, 0xe4, 0x00, 0x08, 0x1a, 0x20, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x84, 0x29, 0x00, 0x04, 0x1a, 0x38, 
    0x00, 0x08, 0x00, 0x08, 0x00, 0x04, 0x18, 0x44, 0x00, 0x10, 0x00, 0x08, 
    0x00, 0x04, 0x1a, 0x30, 0x00, 0x00, 0xaa, 0xe4, 0x00, 0x08, 0x1a, 0x40, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x84, 0x29, 0x00, 0x04, 0x1a, 0x58, 
    0x00, 0x08, 0x00, 0x08, 0x00, 0x04, 0x18, 0x48, 0x00, 0x10, 0x00, 0x08, 
    0x00, 0x04, 0x1a, 0x50, 0x00, 0x00, 0xaa, 0xe4, 0x00, 0x08, 0x1a, 0x60, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x84, 0x29, 0x00, 0x04, 0x1a, 0x78,
    0x00, 0x08, 0x00, 0x08, 0x00, 0x04, 0x18, 0x4c, 0x00, 0x10, 0x00, 0x08, 
    0x00, 0x04, 0x1a, 0x70, 0x00, 0x00, 0xaa, 0xe4, 0x00, 0x08, 0x1a, 0x80, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x84, 0x29, 0x00, 0x04, 0x1a, 0x98, 
    0x00, 0x08, 0x00, 0x08, 0x00, 0x04, 0x18, 0x50, 0x00, 0x10, 0x00, 0x08, 
    0x00, 0x04, 0x1a, 0x90, 0x00, 0x00, 0xaa, 0xe4, 0x00, 0x08, 0x1a, 0xa0, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x84, 0x29, 0x00, 0x04, 0x1a, 0xb8, 
    0x00, 0x08, 0x00, 0x08, 0x00, 0x04, 0x18, 0x54, 0x00, 0x10, 0x00, 0x08, 
    0x00, 0x04, 0x1a, 0xb0, 0x00, 0x00, 0xaa, 0xe4, 0x00, 0x08, 0x1a, 0xc0, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x84, 0x29, 0x00, 0x04, 0x1a, 0xd8, 
    0x00, 0x08, 0x00, 0x08, 0x00, 0x04, 0x18, 0x58, 0x00, 0x10, 0x00, 0x08, 
    0x00, 0x04, 0x1a, 0xd0, 0x00, 0x00, 0xaa, 0xe4, 0x00, 0x08, 0x1a, 0xe0, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x84, 0x29, 0x00, 0x04, 0x1a, 0xf8, 
    0x00, 0x08, 0x00, 0x08, 0x00, 0x04, 0x18, 0x5c, 0x00, 0x10, 0x00, 0x08, 
    0x00, 0x04, 0x1a, 0xf0, 0x00, 0x00, 0xaa, 0xe4, 0x00, 0x08, 0x1b, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x84, 0x29, 0x00, 0x04, 0x1b, 0x18, 
    0x00, 0x08, 0x00, 0x08, 0x00, 0x04, 0x18, 0x60, 0x00, 0x10, 0x00, 0x08, 
    0x00, 0x04, 0x1b, 0x10, 0x00, 0x00, 0xaa, 0xe4, 0x00, 0x08, 0x1b, 0x20, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x84, 0x29, 0x00, 0x04, 0x1b, 0x38, 
    0x00, 0x08, 0x00, 0x08, 0x00, 0x04, 0x18, 0x64, 0x00, 0x10, 0x00, 0x08, 
    0x00, 0x04, 0x1b, 0x30, 0x00, 0x00, 0xaa, 0xe4, 0x00, 0x08, 0x1b, 0x40, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x84, 0x29, 0x00, 0x04, 0x1b, 0x58, 
    0x00, 0x08, 0x00, 0x08, 0x00, 0x04, 0x18, 0x68, 0x00, 0x10, 0x00, 0x08, 
    0x00, 0x04, 0x1b, 0x50, 0x00, 0x00, 0xaa, 0xe4, 0x00, 0x08, 0x1b, 0x60, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x84, 0x29, 0x00, 0x04, 0x1b, 0x78, 
    0x00, 0x08, 0x00, 0x08, 0x00, 0x04, 0x18, 0x6c, 0x00, 0x10, 0x00, 0x08, 
    0x00, 0x04, 0x1b, 0x70, 0x00, 0x00, 0xaa, 0xe4, 0x00, 0x08, 0x1b, 0x80, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x84, 0x29, 0x00, 0x04, 0x1b, 0x98, 
    0x00, 0x08, 0x00, 0x08, 0x00, 0x04, 0x18, 0x70, 0x00, 0x10, 0x00, 0x08, 
    0x00, 0x04, 0x1b, 0x90, 0x00, 0x00, 0xaa, 0xe4, 0x00, 0x08, 0x1b, 0xa0, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x84, 0x29, 0x00, 0x04, 0x1b, 0xb8, 
    0x00, 0x08, 0x00, 0x08, 0x00, 0x04, 0x18, 0x74, 0x00, 0x10, 0x00, 0x08, 
    0x00, 0x04, 0x1b, 0xb0, 0x00, 0x00, 0xaa, 0xe4, 0x00, 0x08, 0x1b, 0xc0, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x84, 0x29, 0x00, 0x04, 0x1b, 0xd8, 
    0x00, 0x08, 0x00, 0x08, 0x00, 0x04, 0x18, 0x78, 0x00, 0x10, 0x00, 0x08, 
    0x00, 0x04, 0x1b, 0xd0, 0x00, 0x00, 0xaa, 0xe4, 0x00, 0x08, 0x1b, 0xe0, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x84, 0x29, 0x00, 0x04, 0x1b, 0xf8, 
    0x00, 0x08, 0x00, 0x08, 0x00, 0x04, 0x18, 0x7c, 0x00, 0x10, 0x00, 0x08, 
    0x00, 0x04, 0x1b, 0xf0, 0x00, 0x00, 0xaa, 0xe4, 0x00, 0x08, 0x09, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xbc, 0x21, 0x00, 0x04, 0x09, 0x10, 
    0x00, 0x00, 0x00, 0x08, 0x00, 0x04, 0x09, 0x18, 0x00, 0x08, 0x00, 0x08, 
    0x00, 0x08, 0x09, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xbc, 0x21, 
    0x00, 0x04, 0x09, 0x30, 0x00, 0x00, 0x00, 0x08, 0x00, 0x04, 0x09, 0x38, 
    0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x09, 0x40, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x01, 0xbc, 0x21, 0x00, 0x04, 0x09, 0x50, 0x00, 0x00, 0x00, 0x08, 
    0x00, 0x04, 0x09, 0x58, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x09, 0x60, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xbc, 0x21, 0x00, 0x04, 0x09, 0x70, 
    0x00, 0x00, 0x00, 0x08, 0x00, 0x04, 0x09, 0x78, 0x00, 0x08, 0x00, 0x08, 
    0x00, 0x04, 0x23, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x23, 0x10, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x23, 0x14, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x01, 0x01, 0x00, 0x10, 0x63, 0x00, 0x00, 0x00, 0x00, 0x0a, 
    0x00, 0x40, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x08, 0x83, 0x00, 0x00, 0x01, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0xc1, 0x84, 0xfe, 0xed, 0x00, 0x00, 0x00, 0x04, 0xc1, 0x98, 
    0x31, 0x33, 0x71, 0xc3, 0x00, 0x24, 0xc2, 0xfc, 0x00, 0x00, 0x00, 0x01, 
    0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xc4, 0x00, 
    0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0xa3, 0x04, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x50, 
    0xff, 0xff, 0xff, 0xff,
};

int gcmResetCommandsSize = sizeof(gcmResetCommands);
int gcmInitCommandsSize = sizeof(gcmInitCommands);
static_assert(sizeof(gcmResetCommands) == 0x1000, "");