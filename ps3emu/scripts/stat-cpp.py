#!/usr/bin/python3

import re
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--cpp', type=str)
args = parser.parse_args()

with open(args.cpp) as f:
    text = f.read()
    stat = {}
    for line in re.findall(" _(.*?)\(", text):
        if not line in stat:
            stat[line] = 0
        stat[line] += 1
            
statl = []
for k in stat.keys():
    statl.append((k, stat[k]))

for k, v in sorted(statl, key=lambda i: i[1]):
    print(k, v)
