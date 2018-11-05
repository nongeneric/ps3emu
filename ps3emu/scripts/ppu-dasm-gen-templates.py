#!/usr/bin/python3

import re
import argparse
import itertools

parser = argparse.ArgumentParser()
parser.add_argument('--path', type=str)
args = parser.parse_args()

with open(args.path, 'r') as f:
    lines = [l for l in f]

defines = []
for i, line in enumerate(lines):
    m = re.search(r"^#define _([^\(]+)\(([^\)]*)\)", line)
    if m:
        name = m.group(1)
        args_str = m.group(2)
        args = [a.strip() for a in args_str.split(',')]
        defines.append((i, name, args))

def keyf(t): return t[1]
groups = itertools.groupby(defines, key=keyf)

for name, g in groups:
    i, _, args = list(g)[0]
    print('template<' + ', '.join(['auto ' + a for a in args]) + '>')
    print('void t_' + name + '(PPUThread* thread) {')
    print('    _' + name + '(' + ', '.join(args) + ')')
    print('}')
    print()
