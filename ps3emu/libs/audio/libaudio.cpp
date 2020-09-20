#include "libaudio.h"
#include "ps3emu/state.h"
#include "ps3emu/log.h"
#include "ps3emu/Process.h"
#include "ps3emu/HeapMemoryAlloc.h"
#include "ps3emu/libs/sys.h"
#include "AudioAttributes.h"
#include "pulse.h"
#include <boost/chrono.hpp>
#include <boost/thread.hpp>


namespace {
    struct {
        boost::thread audioThread;
        AudioAttributes attributes;
    } context;
}

ENUM(AudioControlCommand,
    (INIT, 0x01),
    (QUIT, 0x04),
    (UNK, 0x80),
    (QUIT2, 0x2),
    (PORT_CREATE, 0x3),
    (PORT_CONFIG, 0x86),
    (PORT_OPEN, 0x87),
    (PORT_LEVEL, 0x85),
    (PORT_START, 0x89),
    (PORT_CLOSE, 0x88),
    (SET_NOTIFY_QUEUE, 0x84)
)

union AudioControl {
    uint32_t u32;
    BitField<0, 11> Command;
    BitField<11, 15> AckQueue;
    BitField<15, 20> Ack;
    BitField<20, 32> Id;
};

void initAudio() {
    const sys_ipc_key_t mioCQKeyBase = 0x80004d494f323200ull;
    const uint64_t baseMemId = 0x80004D494F323211;

    sys_event_queue_attr attr { 0 };
    attr.attr_protocol = SYS_SYNC_FIFO;
    strncpy(attr.name, "AudIntr", SYS_SYNC_NAME_SIZE);
    sys_event_queue_t queueId;
    auto res = sys_event_queue_create(&queueId, &attr, mioCQKeyBase, 8);
    assert(res == CELL_OK); (void)res;

    context.audioThread = boost::thread([=]() {
        AudioAttributes attributes;
        std::unique_ptr<PulseBackend> pulse;

        sys_event_port_t ackQueuePort;
        sys_ipc_key_t initialMioCQKey = 0;
        uint32_t curPortId = AudioAttributes::portHwBase;
        std::map<uint32_t, uint32_t> portSizes;

        for (;;) {
            std::string message;

            auto event = sysQueueReceive(queueId);
            message += sformat("(in {:016x} {:016x} {:016x}) ",
                                       event.data1,
                                       event.data2,
                                       event.data3);
            AudioControl control = { (uint32_t)((uint64_t)event.data1 >> 32u) };
            auto command = enum_cast<AudioControlCommand>(control.Command.u());
            message += sformat("(control; command={} ackq={:x} ack={:x} id={:x}) ",
                                       ::to_string(command),
                                       control.AckQueue.u(),
                                       control.Ack.u(),
                                       control.Id.u());
            sys_ipc_key_t mioCQKey = mioCQKeyBase + control.AckQueue.u() + 1;
            assert(initialMioCQKey == 0 || initialMioCQKey == mioCQKey);

            auto ack = [&](uint32_t arg1 = 0,
                           uint32_t arg2 = 0,
                           uint32_t arg3 = 0,
                           uint32_t arg4 = 0,
                           uint32_t arg5 = 0) {
                assert(control.Ack.u());
                uint64_t data1 = ((uint64_t)control.u32 << 32u) | arg1;
                uint64_t data2 = ((uint64_t)arg2 << 32u) | arg3;
                uint64_t data3 = ((uint64_t)arg4 << 32u) | arg5;
                sys_event_port_send(ackQueuePort, data1, data2, data3);
                message += sformat("(ack {:016x} {:016x} {:016x}) ", data1, data2, data3);
            };

            if (command == AudioControlCommand::INIT) {
                initialMioCQKey = mioCQKey;
                auto res = sys_event_port_create(&ackQueuePort, SYS_EVENT_PORT_LOCAL, 0);
                assert(!res); (void)res;
                auto ackQueue = getQueueByKey(mioCQKey);
                res = sys_event_port_connect_local(ackQueuePort, ackQueue);
                assert(!res);
                ack(0,
                    0x00010000, // memory required
                    0x01000300, // audio server process id
                    (uint32_t)(baseMemId >> 32),
                    (uint32_t)baseMemId);
            } else if (command == AudioControlCommand::UNK) {
                uint32_t arg = event.data1;
                if (arg) {
                    auto baseMemInfo = emuFindSharedMemoryInfo(baseMemId);
                    assert(baseMemInfo);
                    attributes = AudioAttributes(
                        g_state.mm->getMemoryPointer(baseMemInfo->va, 0));
                    pulse.reset(new PulseBackend(&attributes));
                    ack(0x10000, (uint32_t)(baseMemId >> 32), (uint32_t)baseMemId);
                } else {
                    ack(3, 0, 0, -1, 0);
                }
            } else if (command == AudioControlCommand::PORT_CREATE) {
                ack(curPortId);
                curPortId++;
            } else if (command == AudioControlCommand::QUIT2) {
                ack(0, 0);
            } else if (command == AudioControlCommand::PORT_CONFIG) {
                uint32_t size = event.data1;
                uint32_t base = (uint64_t)event.data2 >> 32;
                uint32_t readIndex = control.Id.u() - AudioAttributes::portHwBase + 1;
                portSizes[control.Id.u()] = size;
                INFO(audio) << sformat(
                    "AUDIO_COMMAND_PORT_CONFIG size={:x} base={:x} readIndex={:x}",
                    size,
                    base,
                    readIndex);
                ack(0, 0, 0, 0, readIndex);
            } else if (command == AudioControlCommand::PORT_OPEN) {
                float level = bit_cast<float>(static_cast<uint32_t>(event.data1));
                uint32_t blocks = (uint64_t)event.data2 & 0xffff;
                uint32_t channels = ((uint64_t)event.data2 >> 16) & 0xffff;
                uint32_t addr = (uint64_t)event.data2 >> 32;
                uint64_t memId = event.data3;
                auto memInfo = emuFindSharedMemoryInfo(memId);
                assert(memInfo);
                auto ptr = g_state.mm->getMemoryPointer(memInfo->va, 4);
                INFO(audio)
                    << sformat("AUDIO_COMMAND_PORT_OPEN level={} blocks={:x} "
                                 "channels={:x} addr={:x} memId={:x} memVa={:x}",
                                 level,
                                 blocks,
                                 channels,
                                 addr,
                                 memId,
                                 memInfo->va);
                PulsePortInfo info;
                info.ptr = ptr;
                info.blocks = blocks;
                info.channels = channels;
                info.level = level;
                pulse->openPort(control.Id.u(), info);
                ack(0, 0, blocks, 0, portSizes[control.Id.u()] / blocks);
            } else if (command == AudioControlCommand::PORT_START) {
                uint32_t start = event.data1;
                if (start) {
                    pulse->start(control.Id.u());
                } else {
                    pulse->stop(control.Id.u());
                    ack(0, 0);
                }
            } else if (command == AudioControlCommand::SET_NOTIFY_QUEUE) {
                auto notifyQueueKey = event.data2;
                pulse->setNotifyQueue(notifyQueueKey);
                ack(0, 0);
            } else if (command == AudioControlCommand::PORT_LEVEL) {
                WARNING(audio) << "AudioControlCommand::PORT_LEVEL not implemented";
                if (control.Ack.u())
                    ack(0, 0);
            } else if (command == AudioControlCommand::PORT_CLOSE) {
                WARNING(audio) << "AudioControlCommand::PORT_CLOSE not implemented";
                ack(0, 0);
            } else if (command == AudioControlCommand::QUIT) {
                pulse->quit();
            } else {
                assert(false);
            }

            INFO(audio) << message;
        }

        return;
    });
}
