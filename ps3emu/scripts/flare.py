#!/usr/bin/python3

import subprocess
import argparse
import os

parser = argparse.ArgumentParser()
parser.add_argument('--lib', type=str)
parser.add_argument('--sig', type=str)
args = parser.parse_args()

def shell(x): subprocess.check_output(x, shell=True)

ida_bin = os.environ['IDA_BIN']
pelf = ida_bin + "/pelf"
sigmake = ida_bin + "/sigmake"

shell("rm -rf /tmp/flare")
shell("mkdir /tmp/flare")
shell("( cd /tmp/flare && ar x '{}' )".format(args.lib))
