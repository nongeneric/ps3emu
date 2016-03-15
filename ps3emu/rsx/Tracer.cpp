#include "Tracer.h"

#include "string.h"

void Tracer::enable() {
    _enabled = true;
    system("rm -rf /tmp/ps3emu.trace");
    _db.createOrOpen("/tmp/ps3emu.trace");
}
void Tracer::pushBlob(const void* ptr, uint32_t size) {
    if (!_enabled)
        return;
    _blobSet = true;
    _blob.resize(size);
    memcpy(&_blob[0], ptr, size);
}
void Tracer::trace(uint32_t frame, uint32_t num, CommandId command, std::vector< GcmCommandArg > args) {
    if (!_enabled)
        return;
    GcmCommand gcmCommand;
    gcmCommand.frame = frame;
    gcmCommand.num = num;
    gcmCommand.id = (uint32_t)command;
    gcmCommand.args = args;
    if (_blobSet) {
        _blobSet = false;
        gcmCommand.blob = _blob;
    }
    _db.insertCommand(gcmCommand);
}

#define X(x) #x,
const char* commandNames[] = { CommandIdX };

const char* printCommandId(CommandId id) {
    return commandNames[(int)id];
}