#!/usr/bin/python3

import asyncio
import argparse
import os
import sys
import subprocess
import concurrent.futures as cf
import re
import json

NJobs = 3
UnitTestDuration = 1
TimesFileName = 'times.json'

parser = argparse.ArgumentParser()
parser.add_argument('--path', type=str)
parser.add_argument('--unit', action='store_true')
parser.add_argument('--serial', action='store_true')
args = parser.parse_args()

async def read_output(tests_args):
    all_args = ' '.join([args.path] + tests_args)
    proc = await asyncio.create_subprocess_shell(all_args, stdout=subprocess.PIPE)
    await proc.wait()
    output = (await proc.stdout.read()).decode('utf-8')
    return output, proc.returncode
    
def parse_test_list(text):
    lines = text.split('\n')
    serial = []
    parallel = []
    i = 0
    while i < len(lines):
        try:
            if not lines[i].startswith('  '):
                continue
            name = lines[i][2:].replace(',', r'\,')
            if '[serial]' in lines[i + 1]:
                serial.append(name)
                i += 1
            else:
                parallel.append(name)
        finally:
            i += 1
    return serial, parallel

def report_test(output, exitcode):
    print('.', end='')
    sys.stdout.flush()
    if exitcode != 0:
        print('')
        print(output)
        return False
    return True

def get_elapsed_time(output):
    return float(re.findall(r'(\d+\.\d+) s:', output)[0])

def run_test(name):
    return read_output(['-d', 'yes', '"' + name + '"'])

def dump_elapsed_times(times, prev_times):
    d = dict(prev_times)
    for name in times:
        d[name] = times[name]
    with open(TimesFileName, 'w') as f:
        json.dump(d, f)
        
def read_elapsed_times():
    with open(TimesFileName, 'r') as f:
        return json.load(f)

async def run_tests():
    output, code = await read_output(['--list-tests'])
    serial, parallel = parse_test_list(output)
    
    if args.serial:
        serial = serial + parallel
        parallel = []
    
    print(len(serial), 'serial tests found')
    print(len(parallel), 'parallel tests found')
    futures = set()
    completed = 0
    failed = []
    times = dict()
    
    prev_times = read_elapsed_times()
    if args.unit:
        slow_tests = set(name for name in prev_times
                         if prev_times[name] > UnitTestDuration)
        serial = list(set(serial) - slow_tests)
        parallel = list(set(parallel) - slow_tests)
        count = len(serial) + len(parallel)
        print('selected', count, 'unit tests (fast or failed)')
    
    def handle_result(test_name, output, exitcode):
        nonlocal completed, failed, times
        completed += 1
        if not report_test(output, exitcode):
            failed.append(test_name)
        else:
            times[test_name] = get_elapsed_time(output)
    
    while True:
        while len(parallel) > 0 and len(futures) < NJobs:
            name = parallel[0]
            parallel = parallel[1:]
            async def f(name=name): return (name, await run_test(name))
            futures.add(f())
        if len(futures) == 0:
            break;
        done, futures = await asyncio.wait(futures, return_when=cf.FIRST_COMPLETED)
        for future in done:
            name, output_and_code = future.result()
            output, code = output_and_code
            handle_result(name, output, code)
    for name in serial:
        output, code = await run_test(name)
        handle_result(name, output, code)
        
    print('')
    print(completed, 'tests completed;', len(failed), 'tests failed')
    print('failed list:', ','.join(failed))
    dump_elapsed_times(times, prev_times)

loop = asyncio.get_event_loop()  
loop.run_until_complete(run_tests())  
loop.close()  
