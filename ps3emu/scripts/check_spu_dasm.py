#!/usr/bin/python3

import re
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--path', type=str)
args = parser.parse_args()

with open(args.path, 'r') as f:
    lines = [l for l in f]
    
print('immediate without s() or u()')
for i, line in enumerate(lines):
    if line.startswith('EMU_REWRITE'):
        m = re.search(r"i->I\d+[,)]", line)
        if m:
            print(i + 1)
            
print('cia in macros')
for i, line in enumerate(lines):
    if line.endswith(" \\\n"):
        if 'cia' in line and not 'cia_lsa' in line:
            print(i + 1)
