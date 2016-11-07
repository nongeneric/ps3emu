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
    res = s.replace(old, new)
    if res != s:
        if (old, new) not in params:
            params.append((old, new))
    return res

with open(args.path, 'r') as f:
    lines = [l for l in f]

emus = []
for i, line in enumerate(lines):
    m = re.search(r"^EMU\(([^,]*?), ([^)]*)", line)
    if m:
        name = m.group(1)
        form = m.group(2)
        emus.append((i, name, form))

repls = []
for i, name, form in emus:
    body = []
    for j, line in enumerate(lines[i + 1:]):
        if line.rstrip() == '}':
            break
        body.append(line)
    params = []
    for n in range(len(body)):
        body[n] = replaceAndCountNoSuffix(body[n], 'getNBE(i->sh04, i->sh5)', 'nbe_sh', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'getNBE(i->mb04, i->mb5)', 'nbe_mb', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'getNBE(i->me04, i->me5)', 'nbe_me', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->D.s()', '_ds', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->SIMM.s()', '_simms', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->SI.s()', '_sis', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->OE.u()', '_oe', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->mb.u()', '_mb', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->UI.u()', '_ui', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->BF.u()', '_bf', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->BFA.u()', '_bfa', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->DS.native()', '_ds', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->spr.u()', '_spr', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->me.u()', '_me', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->SH.u()', '_shu', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->SI.u()', '_siu', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->idx.u()', '_idx', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->FXM.u()', '_fxm', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->FLM.u()', '_flm', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->TO.u()', '_to', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->tbr.u()', '_tbr', params)
        body[n] = replaceAndCountNoSuffix(body[n], 'i->SHB.u()', '_shb', params)
        
        body[n] = replaceAndCount(body[n], 'i->RT', '_rt', params)
        body[n] = replaceAndCount(body[n], 'i->RS', '_rs', params)
        body[n] = replaceAndCount(body[n], 'i->RA', '_ra', params)
        body[n] = replaceAndCount(body[n], 'i->RB', '_rb', params)
        body[n] = replaceAndCount(body[n], 'i->RC', '_rc', params)
        body[n] = replaceAndCount(body[n], 'i->BT', '_bt', params)
        body[n] = replaceAndCount(body[n], 'i->BA', '_ba', params)
        body[n] = replaceAndCount(body[n], 'i->BB', '_bb', params)
        body[n] = replaceAndCount(body[n], 'i->UIMM', '_uimm', params)
        body[n] = replaceAndCount(body[n], 'i->rA', '_ra', params)
        body[n] = replaceAndCount(body[n], 'i->rB', '_rb', params)
        body[n] = replaceAndCount(body[n], 'i->rC', '_rc', params)
        body[n] = replaceAndCount(body[n], 'i->vA', '_va', params)
        body[n] = replaceAndCount(body[n], 'i->vB', '_vb', params)
        body[n] = replaceAndCount(body[n], 'i->vC', '_vc', params)
        body[n] = replaceAndCount(body[n], 'i->vD', '_vd', params)
        body[n] = replaceAndCount(body[n], 'i->vS', '_vs', params)
        body[n] = replaceAndCount(body[n], 'i->FRT', '_frt', params)
        body[n] = replaceAndCount(body[n], 'i->FRS', '_frs', params)
        body[n] = replaceAndCount(body[n], 'i->FRA', '_fra', params)
        body[n] = replaceAndCount(body[n], 'i->FRB', '_frb', params)
        body[n] = replaceAndCount(body[n], 'i->FRC', '_frc', params)
        body[n] = replaceAndCount(body[n], 'i->Rc', '_rc', params)
        body[n] = replaceAndCount(body[n], 'i->OE', '_oe', params)
        body[n] = replaceAndCount(body[n], 'i->L', '_l', params)
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
    repl.append('EMU_REWRITE(%s, %s%s)' % (name, form, paramstr));
    repl.append('')
    repls.append((i, len(body) + 1, repl))

for line, length, body in reversed(repls):
    lines[line:line + length + 1] = body
    
for line in lines:
    print(line.rstrip())
