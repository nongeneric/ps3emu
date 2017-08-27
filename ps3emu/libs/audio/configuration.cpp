#include "configuration.h"

typedef enum CellAudioOutFs {
    CELL_AUDIO_OUT_FS_32KHZ = 0x01,
    CELL_AUDIO_OUT_FS_44KHZ = 0x02,
    CELL_AUDIO_OUT_FS_48KHZ = 0x04,
    CELL_AUDIO_OUT_FS_88KHZ = 0x08,
    CELL_AUDIO_OUT_FS_96KHZ = 0x10,
    CELL_AUDIO_OUT_FS_176KHZ = 0x20,
    CELL_AUDIO_OUT_FS_192KHZ = 0x40
} CellAudioOutFs;

typedef enum CellAudioOut {
    CELL_AUDIO_OUT_PRIMARY,
    CELL_AUDIO_OUT_SECONDARY
} CellAudioOut;

typedef enum CellVideoOut {
    CELL_VIDEO_OUT_PRIMARY,
    CELL_VIDEO_OUT_SECONDARY
} CellVideoOut;

typedef enum CellAudioOutCodingType {
        CELL_AUDIO_OUT_CODING_TYPE_LPCM = 0,
        CELL_AUDIO_OUT_CODING_TYPE_AC3,
        CELL_AUDIO_OUT_CODING_TYPE_MPEG1,
        CELL_AUDIO_OUT_CODING_TYPE_MP3,
        CELL_AUDIO_OUT_CODING_TYPE_MPEG2,
        CELL_AUDIO_OUT_CODING_TYPE_AAC,
        CELL_AUDIO_OUT_CODING_TYPE_DTS,
        CELL_AUDIO_OUT_CODING_TYPE_ATRAC,
        CELL_AUDIO_OUT_CODING_TYPE_DOLBY_DIGITAL_PLUS = 9,
        CELL_AUDIO_OUT_CODING_TYPE_BITSTREAM = 0xff
} CellAudioOutCodingType;

enum CellAudioOutOutputState {
    CELL_AUDIO_OUT_OUTPUT_STATE_ENABLED,
    CELL_AUDIO_OUT_OUTPUT_STATE_DISABLED,
    CELL_AUDIO_OUT_OUTPUT_STATE_PREPARING
};

typedef enum CellAudioOutDownMixer {
    CELL_AUDIO_OUT_DOWNMIXER_NONE,
    CELL_AUDIO_OUT_DOWNMIXER_TYPE_A,
    CELL_AUDIO_OUT_DOWNMIXER_TYPE_B
} CellAudioOutDownMixer;

typedef enum CellAudioOutDeviceMode {
    CELL_AUDIO_OUT_SINGLE_DEVICE_MODE = 0,
    CELL_AUDIO_OUT_MULTI_DEVICE_MODE = 1,
    CELL_AUDIO_OUT_MULTI_DEVICE_MODE_2 = 2,
} CellAudioOutDeviceMode;

typedef enum CellAudioOutChnum {
    CELL_AUDIO_OUT_CHNUM_2 = 2,
    CELL_AUDIO_OUT_CHNUM_4 = 4,
    CELL_AUDIO_OUT_CHNUM_6 = 6,
    CELL_AUDIO_OUT_CHNUM_8 = 8
} CellAudioOutChnum;

typedef enum CellAudioOutSpeakerLayout {
    CELL_AUDIO_OUT_SPEAKER_LAYOUT_DEFAULT = 0x00000000,
    CELL_AUDIO_OUT_SPEAKER_LAYOUT_2CH = 0x00000001,
    CELL_AUDIO_OUT_SPEAKER_LAYOUT_6CH_LREClr = 0x00010000,
    CELL_AUDIO_OUT_SPEAKER_LAYOUT_8CH_LREClrxy = 0x40000000
} CellAudioOutSpeakerLayout;

int32_t cellAudioOutGetSoundAvailability(uint32_t audioOut,
                                         uint32_t type,
                                         uint32_t fs,
                                         uint32_t option) {
    return cellAudioOutGetSoundAvailability2(audioOut, type, fs, 2, option);
}

int32_t cellAudioOutGetSoundAvailability2(
    uint32_t audioOut, uint32_t type, uint32_t fs, uint32_t ch, uint32_t option) {
    assert(fs == CELL_AUDIO_OUT_FS_48KHZ);
    assert(option == 0);
    if (audioOut == CELL_AUDIO_OUT_PRIMARY &&
        type == CELL_AUDIO_OUT_CODING_TYPE_LPCM &&
        ch == 2)
        return 2;
    return 0;
}

int32_t cellAudioOutConfigure(uint32_t audioOut,
                              const CellAudioOutConfiguration* config,
                              ps3_uintptr_t option,
                              uint32_t waitForEvent) {
    assert(audioOut == CELL_AUDIO_OUT_PRIMARY);
    assert(option == 0);
    return 0;
}

int32_t cellAudioOutGetState(uint32_t audioOut,
                             uint32_t deviceIndex,
                             CellAudioOutState* state) {
    assert(deviceIndex == 0);
    if (audioOut != CELL_AUDIO_OUT_PRIMARY) {
        state->state = CELL_AUDIO_OUT_OUTPUT_STATE_DISABLED;
        return 0;
    }
    state->state = CELL_AUDIO_OUT_OUTPUT_STATE_ENABLED;
    state->encoder = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
    state->downMixer = CELL_AUDIO_OUT_DOWNMIXER_NONE;
    state->soundMode.type = CELL_AUDIO_OUT_CODING_TYPE_LPCM;
    state->soundMode.channel = CELL_AUDIO_OUT_CHNUM_2;
    state->soundMode.fs = CELL_AUDIO_OUT_FS_48KHZ;
    state->soundMode.layout = CELL_AUDIO_OUT_SPEAKER_LAYOUT_DEFAULT;
    return 0;
}

int32_t cellAudioOutGetNumberOfDevice(uint32_t audioOut) {
    assert(audioOut == CELL_AUDIO_OUT_PRIMARY || audioOut == CELL_AUDIO_OUT_SECONDARY);
    return 1;
}

int32_t cellVideoOutGetNumberOfDevice(uint32_t videoOut) {
    assert(videoOut == CELL_AUDIO_OUT_PRIMARY);
    return 1;
}
