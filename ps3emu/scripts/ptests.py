#!/usr/bin/python3

import asyncio
import argparse
import os
import sys
import subprocess
import concurrent.futures as cf

NJobs = 3

parser = argparse.ArgumentParser()
parser.add_argument('--path', type=str)
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
            name = lines[i][2:]
            if '[serial]' in lines[i + 1]:
                serial.append(name)
                i += 1
            else:
                parallel.append(name)
        finally:
            i += 1
    return serial, parallel

def report_test(exception, result):
    print('.', end='')
    sys.stdout.flush()
    if exception != None:
        print(future.exception())
        return False
    output, exitcode = result
    if exitcode != 0:
        print('')
        print(output)
        return False
    return True

def run_test(name):
    return read_output(['-d', 'yes', '"' + name + '"'])

async def speak_async():
    output, code = await read_output(['--list-tests'])
    serial, parallel = parse_test_list(output)
    print(len(serial), 'serial tests found')
    print(len(parallel), 'parallel tests found')
    futures = set()
    completed = 0
    failed = 0
    while True:
        while len(parallel) > 0 and len(futures) < NJobs:
            name = parallel[0]
            parallel = parallel[1:]
            futures.add(run_test(name))
        if len(futures) == 0:
            break;
        done, futures = await asyncio.wait(futures, return_when=cf.FIRST_COMPLETED)
        for future in done:
            completed += 1
            if not report_test(future.exception(), future.result()):
                failed += 1
    for name in serial:
        completed += 1
        if not report_test(None, await run_test(name)):
            failed += 1
    print('')
    print(completed, 'tests completed;', failed, 'tests failed')

loop = asyncio.get_event_loop()  
loop.run_until_complete(speak_async())  
loop.close()  
