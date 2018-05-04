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

ENUM(CellAudioOutCopyControl,
    (CELL_AUDIO_OUT_COPY_CONTROL_COPY_FREE, 0),
    (CELL_AUDIO_OUT_COPY_CONTROL_COPY_ONCE, 1),
    (CELL_AUDIO_OUT_COPY_CONTROL_COPY_NEVER, 2));

int32_t cellAudioOutSetCopyControl(uint32_t audioOut, CellAudioOutCopyControl control);
int32_t cellAudioOutGetSoundAvailability(uint32_t audioOut,
                                         uint32_t type,
                                         uint32_t fs,
                                         uint32_t option);
int32_t cellAudioOutGetSoundAvailability2(
    uint32_t audioOut, uint32_t type, uint32_t fs, uint32_t ch, uint32_t option);
int32_t cellAudioOutConfigure(uint32_t audioOut,
                              const CellAudioOutConfiguration* config,
                              ps3_uintptr_t option,
                              uint32_t waitForEvent);
int32_t cellAudioOutGetState(uint32_t audioOut,
                             uint32_t deviceIndex,
                             CellAudioOutState* state);
int32_t cellAudioOutGetNumberOfDevice(uint32_t audioOut);
int32_t cellVideoOutGetNumberOfDevice(uint32_t videoOut);
int32_t cellAudioOutGetConfiguration(uint32_t audioOut,
                                     CellAudioOutConfiguration* config,
                                     uint32_t* option);
