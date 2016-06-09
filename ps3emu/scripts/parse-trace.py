#!/usr/bin/python3

# parse-trace.py --changes /tmp/ps3trace
# parse-trace.py --spu --changes /tmp/ps3trace-spu
# --changes shows what registers change at each line and their new values
# each line is either starts with a # and ignored or has the following format
#     pc:%08x;r0=%016x;...;r31=%016x;v0=%032;...;v31=%032; #optional comment

import argparse

class Trace:
    nip = None
    regs = None
    vregs = None

parser = argparse.ArgumentParser()
parser.add_argument('--spu', action="store_true")
parser.add_argument('--changes', type=str)
args = parser.parse_args()

def parse_regs(pairs):
    regs = []
    for regstr in pairs:
        val = int(regstr.split(':')[1], 16)
        regs.append(val)
    return regs

if args.changes:
    traces = []
    with open(args.changes) as f:
        for line in f.readlines():
            comment = line.find('#')
            if comment != -1:
                line = line[0:comment]
            if comment == 0:
                continue
            split = line.split(';')
            trace = Trace()
            trace.nip = int(split[0].split(':')[1], 16)
            if args.spu:
                trace.regs = parse_regs(split[1:-1])
                trace.vregs = []
            else:
                trace.regs = parse_regs(split[1:33])
                trace.vregs = parse_regs(split[33:-1])
            traces.append(trace)
            
    print(len(traces), "lines")
    for c, n in zip(traces, traces[1:]):
        changed = []
        rnames = ['r' + str(x) for x in range(0, len(n.regs))]
        vnames = ['v' + str(x) for x in range(0, len(n.vregs))]
        for creg, nreg, name in zip(c.regs, n.regs, rnames):
            if creg != nreg: changed.append((name, nreg))
        for creg, nreg, name in zip(c.vregs, n.vregs, vnames):
            if creg != nreg: changed.append((name, nreg))
        print(hex(n.nip), [(name, hex(x)) for name, x in changed])
        
