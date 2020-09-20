#include "Tracer.h"

#include "string.h"
#include "ps3emu/log.h"
#include "ps3emu/state.h"
#include "ps3emu/rsx/Rsx.h"

void Tracer::enable(bool enabled) {
    _enabled = enabled;
    if (_enabled) {
        auto path = sformat("/tmp/ps3emu_{}.trace", getpid());
        system(("rm -rf " + path).c_str());
        _db.createOrOpen(path);
    }
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
                   std::vector<GcmCommandArg> const& args) {
    if (!_enabled)
        return;
    
    std::string argStr = "  ";
    for (auto& arg : args) {
        argStr += sformat("{}:{} ", arg.name, printArgHex(arg));
    }
    INFO(rsx) << sformat("{:08x}[{:08x}/{:08x}] | {}{}",
                           rsxOffsetToEa(MemoryLocation::Main, g_state.rsx->getGet()),
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
        case GcmArgType::None: return sformat("NONE(#{:x})", arg.value);
        case GcmArgType::Bool: return arg.value ? "True" : "False";
        case GcmArgType::Float: {
            float value = bit_cast<float>(arg.value);
            return sformat("{}", value);
        }
        case GcmArgType::Int32:
        case GcmArgType::Int16:
        case GcmArgType::UInt8:
        case GcmArgType::UInt16:
        case GcmArgType::UInt32: return sformat("{}", arg.value);
    }
    throw std::runtime_error("unknown type");
}

std::string printArgHex(GcmCommandArg const& arg) {
    switch ((GcmArgType)arg.type) {
        case GcmArgType::None:
        case GcmArgType::Bool:
        case GcmArgType::UInt8: return sformat("#{:02x}", arg.value);
        case GcmArgType::Int16:
        case GcmArgType::UInt16: return sformat("#{:04x}", arg.value);
        case GcmArgType::Float:
        case GcmArgType::Int32:
        case GcmArgType::UInt32: return sformat("#{:08x}", arg.value);
    }
    throw std::runtime_error("unknown type");
}
