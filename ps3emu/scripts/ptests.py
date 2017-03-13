#!/usr/bin/python3

import asyncio
import argparse
import os
import sys
import subprocess
import concurrent.futures as cf

NJobs = 8

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
    return [x[2:] for x in text.split('\n') if x.startswith('  ')]

async def speak_async():
    output, code = await read_output(['--list-tests'])
    names = parse_test_list(output)
    print(len(names), 'tests found')
    futures = set()
    completed = 0
    failed = 0
    while True:
        while len(names) > 0 and len(futures) < NJobs:
            name = names[0]
            names = names[1:]
            futures.add(read_output(['-d', 'yes', '"' + name + '"']))
        if len(futures) == 0:
            break;
        done, futures = await asyncio.wait(futures, return_when=cf.FIRST_COMPLETED)
        for future in done:
            completed += 1
            print('.', end='')
            sys.stdout.flush()
            if future.exception() != None:
                print(future.exception())
            output, exitcode = future.result()
            if exitcode != 0:
                print(exitcode, output)
                failed += 1
    print(completed, 'tests completed;', failed, 'tests failed')

loop = asyncio.get_event_loop()  
loop.run_until_complete(speak_async())  
loop.close()  
