#include "Tracer.h"

#include "string.h"
#include "ps3emu/log.h"
#include "ps3emu/state.h"
#include "ps3emu/rsx/Rsx.h"

void Tracer::enable() {
    _enabled = true;
    system("rm -rf /tmp/ps3emu.trace");
    _db.createOrOpen("/tmp/ps3emu.trace");
}

bool Tracer::isEnabled() {
    return _enabled;
}

void Tracer::pushBlob(const void* ptr, uint32_t size) {
    if (!_enabled)
        return;
    _blobSet = true;
    _blob.resize(size);
    memcpy(&_blob[0], ptr, size);
}

void Tracer::trace(uint32_t frame,
                   uint32_t num,
                   CommandId command,
                   std::vector<GcmCommandArg> args) {
    if (!_enabled)
        return;
    
    std::string argStr = "  ";
    for (auto& arg : args) {
        argStr += ssnprintf("%s:%s ", arg.name, printArgHex(arg));
    }
    INFO(rsx) << ssnprintf("%08x/%08x | %s%s",
                           g_state.rsx->getGet(),
                           g_state.rsx->getPut(),
                           printCommandId(command),
                           argStr);
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

std::string printArgDecimal(GcmCommandArg const& arg) {
    switch ((GcmArgType)arg.type) {
        case GcmArgType::None: return ssnprintf("NONE(#%x)", arg.value);
        case GcmArgType::Bool: return arg.value ? "True" : "False";
        case GcmArgType::Float: {
            float value = union_cast<uint32_t, float>(arg.value);
            return ssnprintf("%g", value);
        }
        case GcmArgType::Int32:
        case GcmArgType::Int16:
        case GcmArgType::UInt8:
        case GcmArgType::UInt16:
        case GcmArgType::UInt32: return ssnprintf("%d", arg.value);
    }
    throw std::runtime_error("unknown type");
}

std::string printArgHex(GcmCommandArg const& arg) {
    switch ((GcmArgType)arg.type) {
        case GcmArgType::None:
        case GcmArgType::Bool:
        case GcmArgType::UInt8: return ssnprintf("#%02x", arg.value);
        case GcmArgType::Int16:
        case GcmArgType::UInt16: return ssnprintf("#%04x", arg.value);
        case GcmArgType::Float:
        case GcmArgType::Int32:
        case GcmArgType::UInt32: return ssnprintf("#%08x", arg.value);
    }
    throw std::runtime_error("unknown type");
}
