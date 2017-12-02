#!/usr/bin/python3

import re

maxOffset = 0

text = open('/tmp/methods').read()
for m in re.findall(r'#define\s+(.+?)\s+\((.+?)\)', text):
    h = int(m[1], 16)
    if h > maxOffset:
        maxOffset = h
    print(m[0] + ' = ' + m[1] + ',')
print('\n\n\n')
for m in re.findall(r'#define\s+(.+?)\s+\((.+?)\)', text):
    print('void ' + m[0] + '_impl(int index);')
print('\n\n\n')
for m in re.findall(r'#define\s+(.+?)\s+\((.+?)\)', text):
    print('void Rsx::' + m[0] + '_impl(int index) {')
    print(f'    WARNING(rsx) << "method not implemented: {m[0]}";')
    print('}')
    print()
print('\n\n\n')
for m in re.findall(r'#define\s+(.+?)\s+\((.+?)\)', text):
    print(f'SINGLE({m[0]})')
    
print('\n\n\n')
text = open('/tmp/enums').read()
for m in re.findall(r'(.+?)\s*=\s*\(?(.+?)[),]', text):
    print(m[0] + ' = ' + m[1] + ',')

print('maxOffset', hex(maxOffset // 4 + 1))
