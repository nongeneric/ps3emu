# fnid.py --makedb /ppu/lib
# fnid.py --patch /tmp/log
# fnid.py --find 12345

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
args = parser.parse_args()

def calcfnid(s):
    sha = hashlib.sha1()
    sha.update(s)
    sha.update('\x67\x59\x65\x99\x04\x25\x04\x90\x56\x64\x27\x49\x94\x89\x74\x1A')
    d = sha.digest()
    return ord(d[0]) | (ord(d[1]) << 8) | (ord(d[2]) << 16) | (ord(d[3]) << 24)

def find(c, fnid):
    cur = c.cursor()
    cur.execute('select str from fnids where fnid = ?', (fnid,))
    str = cur.fetchone()
    return str[0] if str else "none"

c = sqlite3.connect('fnids.db')

if args.find:
    print(find(c, int(args.find, 16)))

if args.makedb:
    c.execute('create table if not exists fnids (str text, fnid int)')
    c.execute('create index if not exists fnid_index on fnids (fnid)')
    for path in glob.glob(args.makedb + "/*.a"):
        print("working on", path)
        line = "cat " + path + " | strings > /tmp/fnid"
        subprocess.check_output(line, shell=True)
        with open("/tmp/fnid") as f:
            strings = [s.strip() for s in f]
            for s in strings:
                c.execute("insert into fnids values (?, ?)", (s, calcfnid(s)))
        c.commit()

if args.patch:
    with open(args.patch) as f:
        text = f.read()
        rx = re.compile("fnid_([0-9A-F]{8})")
        reps = []
        for m in rx.finditer(text):
            fname = find(c, int(m.group(1), 16))
            repl = "[" + fname + "]"
            reps.append((m.span(), repl))
        for r in reversed(reps):
            s = r[0][0]
            e = r[0][1]
            v = r[1]
            text = text[:s] + v + text[e:]
        print(text)
    
