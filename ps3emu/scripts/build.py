#!/usr/bin/env python3

import subprocess
import argparse
import os
from dataclasses import dataclass

parser = argparse.ArgumentParser()
parser.add_argument('--build_dir', type=str, required=True)
parser.add_argument('--config', type=str)
cmd_args = parser.parse_args()

@dataclass
class Config:
    compiler: str
    address_sanitizer: bool
    thread_sanitizer: bool
    debug: bool
    tests: bool
    log: bool
    execmap: bool
    vtune: bool
    gldebug: bool
    name: str

configs = [
    Config(name = "gcc_debug",
        compiler = "g++",
        address_sanitizer = False,
        thread_sanitizer = False,
        debug = True,
        tests = True,
        log = True,
        execmap = False,
        vtune = False,
        gldebug = True),
    Config(name = "clang_debug",
        compiler = "clang++",
        address_sanitizer = False,
        thread_sanitizer = False,
        debug = True,
        tests = True,
        log = True,
        execmap = False,
        vtune = False,
        gldebug = True),
    Config(name = "clang_release",
        compiler = "g++",
        address_sanitizer = False,
        thread_sanitizer = False,
        debug = False,
        tests = True,
        log = True,
        execmap = False,
        vtune = False, # enable
        gldebug = True),
    Config(name = "clang_release",
        compiler = "clang++",
        address_sanitizer = False,
        thread_sanitizer = False,
        debug = False,
        tests = True,
        log = True,
        execmap = False,
        vtune = False,
        gldebug = True),
    Config(name = "gcc_address_debug",
        compiler = "g++",
        address_sanitizer = True,
        thread_sanitizer = False,
        debug = True,
        tests = True,
        log = True,
        execmap = False,
        vtune = False,
        gldebug = True),
    Config(name = "clang_address_debug",
        compiler = "clang++",
        address_sanitizer = True,
        thread_sanitizer = False,
        debug = True,
        tests = True,
        log = True,
        execmap = False,
        vtune = False,
        gldebug = True),
    Config(name = "gcc_thread_debug",
        compiler = "g++",
        address_sanitizer = False,
        thread_sanitizer = True,
        debug = True,
        tests = True,
        log = True,
        execmap = False,
        vtune = False,
        gldebug = True),
    Config(name = "clang_thread_debug",
        compiler = "clang++",
        address_sanitizer = False,
        thread_sanitizer = True,
        debug = True,
        tests = True,
        log = True,
        execmap = False,
        vtune = False,
        gldebug = True)
]

selected = [c for c in configs if c.name == cmd_args.config] if cmd_args.config else []

if len(selected) == 0:
    for c in configs:
        print(c.name)

for c in selected:
    print(f'configuring {c.name}\n')
    build_dir = f'{cmd_args.build_dir}/{c.name}'
    source_dir = os.path.realpath(f'{__file__}/../../..')
    print(f'build_dir: {build_dir}')
    print(f'source_dir: {source_dir}')
    subprocess.check_call(['mkdir', '-p', build_dir])
    args = [
        'cmake', '-GNinja',
        f'-DCMAKE_CXX_COMPILER={c.compiler}',
        f'-DCMAKE_BUILD_TYPE={"Debug" if c.debug else "Release"}',
        f'-DEMU_CONFIGURATION_BUILD_NAME={c.name}',
        f'-DEMU_CONFIGURATION_ADDRESS_SANITIZER={1 if c.address_sanitizer else 0}',
        f'-DEMU_CONFIGURATION_UNDEFINED_SANITIZER=0',
        f'-DEMU_CONFIGURATION_THREAD_SANITIZER={1 if c.thread_sanitizer else 0}',
        f'-DEMU_CONFIGURATION_DEBUG={1 if c.debug else 0}',
        f'-DEMU_CONFIGURATION_TESTS={1 if c.tests else 0}',
        f'-DEMU_CONFIGURATION_PAUSE=1',
        f'-DEMU_CONFIGURATION_LOG={1 if c.log else 0}',
        f'-DEMU_CONFIGURATION_EXECMAP={1 if c.execmap else 0}',
        f'-DEMU_CONFIGURATION_VTUNE={1 if c.vtune else 0}',
        f'-DEMU_CONFIGURATION_GLDEBUG={1 if c.gldebug else 0}',
        source_dir
    ]
    print('running:\n', ' \\\n  '.join(args))
    subprocess.check_output(args, cwd=build_dir)
