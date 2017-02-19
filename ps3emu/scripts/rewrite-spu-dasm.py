#!/usr/bin/python3

import re
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--path', type=str)
args = parser.parse_args()

def replaceAndCount(s, old, new, params):
    res = s
    res = res.replace(old + '.u()', new)
    res = res.replace(old, new)
    if res != s:
        p = old + '.u()'
        if (p, new) not in params:
            params.append((p, new))
    return res
    
def replaceAndCountNoSuffix(s, old, new, params):
    res = re.sub(old.replace('(', '\(').replace(')', '\)'), new, s, flags=re.I) 
    if res != s:
        if (old, new) not in params:
            params.append((old, new))
    return res

with open(args.path, 'r') as f:
    lines = [l for l in f]

emus = []
for i, line in enumerate(lines):
    m = re.search(r"^EMU\(([^)]*?)\)", line)
    if m:
        name = m.group(1)
        emus.append((i, name))

repls = []
for i, name in emus:
    body = []
    for j, line in enumerate(lines[i + 1:]):
        if line.rstrip() == '}':
            break
        body.append(line)
    params = []
    for n in range(len(body)):
        body[n] = replaceAndCountNoSuffix(body[n], 'i->I7.u()', '_i7', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->I8.u()', '_i8', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->I10.u()', '_i10', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->I16.u()', '_i16', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->I18.u()', '_i18', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->I7.s()', '_i7', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->I8.s()', '_i8', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->I10.s()', '_i10', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->I16.s()', '_i16', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->I18.s()', '_i18', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->I7', '_i7', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->I8', '_i8', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->I10', '_i10', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->I16', '_i16', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->I18', '_i18', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->StopAndSignalType.u()', '_sast', params)
        
        body[n] = replaceAndCount(body[n], 'i->RT_ABC', '_rt_abc', params)
        body[n] = replaceAndCount(body[n], 'i->RT', '_rt', params)
        body[n] = replaceAndCount(body[n], 'i->RB', '_rb', params)
        body[n] = replaceAndCount(body[n], 'i->RA', '_ra', params)
        body[n] = replaceAndCount(body[n], 'i->RC', '_rc', params)
        body[n] = replaceAndCount(body[n], 'i->CA', '_ca', params)
        if 'i->' in body[n]:
            print(body[n])
            raise 1
    if len(params) == 0:
        params = [('0', '_')]
            
    paramstr = ''
    for k, (old, new) in enumerate(params):
        if k > 0:
            paramstr += ', '
        paramstr += new
    repl = []
    repl.append('#define _%s(%s) { \\' % (name, paramstr))
    for line in body:
        repl.append(line.rstrip() + ' \\')
    repl.append('}')
    paramstr = ''
    for k, (old, new) in enumerate(params):
        paramstr += ', ' + old
    repl.append('EMU_REWRITE(%s%s)' % (name, paramstr));
    repl.append('')
    repls.append((i, len(body) + 1, repl))

for line, length, body in reversed(repls):
    lines[line:line + length + 1] = body
    
for line in lines:
    print(line.rstrip())
