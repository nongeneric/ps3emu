#!/usr/bin/python3

import argparse
import re

parser = argparse.ArgumentParser()
parser.add_argument('--log', type=str)
args = parser.parse_args()

rxVpts = re.compile('\[(\d+)] getVideoFrame pts = (\d+) currentTime\(from stream beg\) = (\d+)')
rxApts = re.compile('\[(\d+)] audio pts = (\d+) currentTime\(from stream beg\) = (\d+)')
rxVsync = re.compile('\[(\d+)] updateVideoSync\((\d+), (\d+)\)')
rxAsync = re.compile('\[(\d+)] updateAudioSync\((\d+), (\d+)\)')

vpts = []
apts = []
vsync = []
async = []

def printCVS(arr, path):
    with open(path, 'w') as f:
        for t in arr:
            line = ', '.join([str(s) for s in t])
            f.write(line + '\n')

with open(args.log) as f:
    for line in f.readlines():
        m = rxVpts.match(line)
        if m:
            syst = int(m.group(1))
            pts = int(m.group(2))
            strt = int(m.group(3))
            vpts.append((syst, pts, strt))
            continue
        m = rxApts.match(line)
        if m:
            syst = int(m.group(1))
            pts = int(m.group(2))
            strt = int(m.group(3))
            apts.append((syst, pts, strt))
            continue
        m = rxVsync.match(line)
        if m:
            syst = int(m.group(1))
            ts = int(m.group(3))
            vsync.append((syst, ts))
            continue
            m = rxVsync.match(line)
        m = rxAsync.match(line)
        if m:
            syst = int(m.group(1))
            ts = int(m.group(3))
            async.append((syst, ts))
            continue

beg_systime = vsync[-50][0]

vsync = [t for t in vsync if t[0] > beg_systime]
async = [t for t in async if t[0] > beg_systime]

printCVS(vpts, '/tmp/vpts')
printCVS(apts, '/tmp/apts')
printCVS(vsync, '/tmp/vsync')
printCVS(async, '/tmp/async')

with open('/tmp/script.gp', 'w') as f:
    f.write("set xlabel 'systime, ms' \n")
    f.write("set ylabel 'pts, ms' \n")
    f.write("plot '/tmp/async' using ($1/1000):($2/1000) w l, '/tmp/vsync' using ($1/1000):($2/1000) w l \n")


# gnuplot --persist /tmp/script.gp
