#!/usr/bin/python3

# fnid.py --makedb /ppu/lib
# fnid.py --patch /tmp/log
# fnid.py --find 12345
# fnid.py --fnid _functionName
# fnid.py --eid module_start

import subprocess
import argparse
import glob
import hashlib
import sqlite3
import re

parser = argparse.ArgumentParser()
parser.add_argument('--makedb', type=str)
parser.add_argument('--patch', type=str)
parser.add_argument('--find', type=str)
parser.add_argument('--fnid', type=str)
parser.add_argument('--eid', type=str)
args = parser.parse_args()

fn_suffix = b'\x67\x59\x65\x99\x04\x25\x04\x90\x56\x64\x27\x49\x94\x89\x74\x1A'
export_suffix = b'0xbc5eba9e042504905b64274994d9c41f'

def calcfnid(s, suffix):
    sha = hashlib.sha1()
    sha.update(s.encode('utf-8'))
    sha.update(suffix)
    d = sha.digest()
    return d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24)

def find(c, fnid):
    cur = c.cursor()
    cur.execute('select str from fnids where fnid = ?', (fnid,))
    str = cur.fetchone()
    return str[0] if str else "none"

c = sqlite3.connect('fnids.db')

if args.fnid:
    print("%08x" % calcfnid(args.fnid, fn_suffix))

if args.eid:
    print("%08x" % calcfnid(args.eid, export_suffix))

if args.find:
    print(find(c, int(args.find, 16)))

if args.makedb:
    c.execute('create table if not exists fnids (str text, fnid int)')
    c.execute('create index if not exists fnid_index on fnids (fnid)')
    for path in glob.glob(args.makedb + "/*.a"):
        print("working on", path)
        line = "strings " + path + " > /tmp/fnid"
        subprocess.check_output(line, shell=True)
        with open("/tmp/fnid") as f:
            strings = [s.strip() for s in f]
            for s in strings:
                c.execute("insert into fnids values (?, ?)", (s, calcfnid(s, fn_suffix)))
                c.execute("insert into fnids values (?, ?)", (s, calcfnid(s, export_suffix)))
        c.commit()
    for path in glob.glob(args.makedb + "/*.h"):
        print("working on", path)
        with open(path) as f:
            for line in [s.strip() for s in f]:
                for word in [w.strip() for w in re.split(' |,|\(|\)|\.|->|\[|\]', line)]:
                    c.execute("insert into fnids values (?, ?)", (word, calcfnid(word, fn_suffix)))
                    c.execute("insert into fnids values (?, ?)", (word, calcfnid(word, export_suffix)))
        c.commit()

if args.patch:
    with open(args.patch) as f:
        text = f.read()
        rx = re.compile("fnid_([0-9A-F]{8})")
        reps = []
        for m in rx.finditer(text):
            fname = find(c, int(m.group(1), 16))
            reps.append((m.span(), fname))
        for r in reversed(reps):
            s = r[0][0]
            e = r[0][1]
            v = r[1]
            text = text[:s] + v + text[e:]
        print(text)
    
