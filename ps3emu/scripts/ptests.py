#!/usr/bin/python3

import asyncio
import argparse
import os
import sys
import subprocess
import concurrent.futures as cf
import re
import json

UnitTestDuration = 1
TimesFileName = 'times.json'

parser = argparse.ArgumentParser()
parser.add_argument('--path', type=str)
parser.add_argument('--unit', action='store_true')
parser.add_argument('--serial', action='store_true')
parser.add_argument('--threads', type=int, default=3)
args = parser.parse_args()

class CatchRunner:
    async def read_test_list(self):
        output, _ = await read_output(['--list-tests'])
        return output

    def parse_test_list(self, text):
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

    def get_elapsed_time(self, output):
        return float(re.findall(r'(\d+\.\d+) s:', output)[0])

    def run_test(self, name):
        return read_output(['-d', 'yes', '"' + name + '"'])

class GtestRunner(CatchRunner):
    async def read_test_list(self):
        output, code = await read_output(['--gtest_list_tests'])
        assert code == 0
        return output

    def parse_test_list(self, text):
        lines = text.split('\n')
        serial = []
        parallel = []
        i = 0
        while i < len(lines):
            try:
                if not lines[i].startswith('  '):
                    fixture = lines[i]
                    continue
                name = lines[i][2:].replace(',', r'\,')
                if '[serial]' in lines[i + 1]:
                    serial.append(fixture + name)
                    i += 1
                else:
                    parallel.append(fixture + name)
            finally:
                i += 1
        def filter_disabled(l): return [x for x in l if '.DISABLED_' not in x]
        return filter_disabled(serial), filter_disabled(parallel)

    def get_elapsed_time(self, output):
        return float(re.findall(r'\[       OK \].*?\((\d+)', output)[0])

    def run_test(self, name):
        return read_output([f'--gtest_filter={name}'])

async def make_runner():
    output, code = await read_output([f'--help'])
    assert code == 0
    if 'Google Test' in output:
        return GtestRunner()
    if 'Catch' in output:
        return CatchRunner()
    assert False

async def read_output(tests_args):
    all_args = ' '.join([args.path] + tests_args)
    proc = await asyncio.create_subprocess_shell(all_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = await proc.communicate()
    return stdout.decode(), proc.returncode

def report_test(output, exitcode):
    print('.', end='')
    sys.stdout.flush()
    if exitcode != 0:
        print('')
        print(output)
        return False
    return True

def dump_elapsed_times(times, prev_times):
    d = dict(prev_times)
    for name in times:
        d[name] = times[name]
    with open(TimesFileName, 'w') as f:
        json.dump(d, f)

def read_elapsed_times():
    try:
        with open(TimesFileName, 'r') as f:
            return json.load(f)
    except:
        return []

async def run_tests():
    runner = await make_runner()
    test_list = await runner.read_test_list()
    serial, parallel = runner.parse_test_list(test_list)

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
            times[test_name] = runner.get_elapsed_time(output)

    while True:
        while len(parallel) > 0 and len(futures) < args.threads:
            name = parallel[0]
            parallel = parallel[1:]
            async def f(name=name): return (name, await runner.run_test(name))
            futures.add(asyncio.create_task(f()))
        if len(futures) == 0:
            break
        done, futures = await asyncio.wait(futures, return_when=cf.FIRST_COMPLETED)
        for future in done:
            name, output_and_code = future.result()
            output, code = output_and_code
            handle_result(name, output, code)
    for name in serial:
        output, code = await runner.run_test(name)
        handle_result(name, output, code)

    print('')
    print(completed, 'tests completed;', len(failed), 'tests failed')
    print('failed list:', ','.join(failed))
    dump_elapsed_times(times, prev_times)

loop = asyncio.get_event_loop()
loop.run_until_complete(run_tests())
loop.close()
