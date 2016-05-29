#pragma once

#include "../sys.h"

typedef struct CellAudioOutConfiguration {
    uint8_t channel;
    uint8_t encoder;
    uint8_t reserved[10];
    big_uint32_t downMixer;
} CellAudioOutConfiguration;

typedef struct CellAudioOutSoundMode {
    uint8_t type;
    uint8_t channel;
    uint8_t fs;
    uint8_t reserved;
    big_uint32_t layout;
} CellAudioOutSoundMode;

typedef struct CellAudioOutState {
    uint8_t state;
    uint8_t encoder;
    uint8_t reserved[6];
    big_uint32_t downMixer;
    CellAudioOutSoundMode soundMode;
} CellAudioOutState;

int32_t cellAudioOutGetSoundAvailability(uint32_t audioOut,
                                         uint32_t type,
                                         uint32_t fs,
                                         uint32_t option);
int32_t cellAudioOutConfigure(uint32_t audioOut,
                              const CellAudioOutConfiguration* config,
                              ps3_uintptr_t option,
                              uint32_t waitForEvent);
int32_t cellAudioOutGetState(uint32_t audioOut,
                             uint32_t deviceIndex,
                             CellAudioOutState* state);
