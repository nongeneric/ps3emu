#!/usr/bin/python3

# parse-trace.py --changes /tmp/ps3trace
# parse-trace.py --spu --changes /tmp/ps3trace-spu
# --changes shows what registers change at each line and their new values
# each line is either starts with a # and ignored or has the following format
#     pc:%08x;r0=%016x;...;r31=%016x;v0=%032;...;v31=%032; #optional comment

import sys
import argparse
import bz2

class Trace:
    nip = None
    regs = None
    vregs = None

parser = argparse.ArgumentParser()
parser.add_argument('--spu', action="store_true")
parser.add_argument('--rebase_script', type=str)
parser.add_argument('--changes', type=str)
parser.add_argument('--rebase', type=str)
parser.add_argument('--source_base', type=str)
parser.add_argument('--target_base', type=str)
args = parser.parse_args()

if args.rebase_script:
    source_base = int(args.source_base, 16)
    target_base = int(args.target_base, 16)
    with open(args.rebase_script) as f:
        for line in f.readlines():
            split = line.split(' ')
            command = split[0]
            va = int(split[1], 16)
            print(command, hex(va - source_base + target_base)[2:].upper())

def parse_regs(pairs):
    regs = []
    for regstr in pairs:
        val = int(regstr.split(':')[1], 16)
        regs.append(val)
    return regs

imagebase = 0
if args.rebase:
    imagebase = int(args.rebase, 16)

if args.changes:
    lineNum = 1
    with open(args.changes) as fobj:
        f = fobj
        if args.changes.endswith('.bz2'):
            f = bz2.open(args.changes)
        c = None
        n = None
        while True:
            line = f.readline()
            if not line:
                break
            try:
                line = line.decode()
            except: pass
            try:
                i = line.find('#')
                if i != -1:
                    comment = line[i + 1:].strip()
                    line = line[0:i].strip()
                else:
                    comment = ''
                if line == '':
                    continue
                split = line.split(';')
                trace = Trace()
                split2 = split[0].split(':')
                if len(split2) != 2:
                    sys.stderr.write('parsing error (:) line ' + str(lineNum) + '\n')
                    exit(0)
                trace.nip = int(split2[1], 16)
                trace.nip -= imagebase
                if args.spu:
                    trace.regs = parse_regs(split[1:-1])
                    trace.vregs = []
                else:
                    trace.regs = parse_regs(split[1:34])
                    trace.vregs = parse_regs(split[34:-1])
                lineNum += 1
                          
                c = n
                n = trace
                    
                changed = []
                rnames = ['r' + str(x) for x in range(0, len(n.regs))]
                vnames = ['v' + str(x) for x in range(0, len(n.vregs))]
                if c:
                    for creg, nreg, name in zip(c.regs, n.regs, rnames):
                        if creg != nreg: changed.append((name, nreg))
                    for creg, nreg, name in zip(c.vregs, n.vregs, vnames):
                        if creg != nreg: changed.append((name, nreg))
                else:
                    for nreg, name in zip(n.regs, rnames):
                        changed.append((name, nreg))
                    for nreg, name in zip(n.vregs, vnames):
                        changed.append((name, nreg))
                print(hex(n.nip), [(name, hex(x)) for name, x in changed], '"' + comment + '"')
                
            except:
                raise
