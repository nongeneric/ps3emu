#!/usr/bin/python

import subprocess

def build():
	print(dir)
	for ppu2spu in raw_spu_ppu_to_spu:
		for spu2ppu in raw_spu_spu_to_ppu:
			print('ppu2spu: ', ppu2spu, ' spu2ppu', spu2ppu)
			line = 'make -C ' + dir + ' clean'
			subprocess.check_output(line, shell=True)
			line = 'make -C ' + dir + ' PPU_TO_SPU=' + ppu2spu + ' SPU_TO_PPU=' + spu2ppu
			subprocess.check_output(line, shell=True)
			ppu_outdir = 'output/' + name + '/' + ppu2spu + '___' + spu2ppu
			spu_outdir = ppu_outdir + '/app_home'
			subprocess.check_output('mkdir -p ' + ppu_outdir, shell=True)
			subprocess.check_output('mkdir -p ' + spu_outdir, shell=True)
			ppu_elf = dir + '/main.ppu.elf'
			ppu_self = dir + '/main.ppu.self'
			spu_elf = dir + '/' + spu_name + '.spu.elf'
			subprocess.check_output('cp ' + ppu_elf + ' ' + ppu_outdir + '/a.elf', shell=True)
			#subprocess.check_output('cp ' + ppu_self + ' ' + ppu_outdir + '/a.self', shell=True)
			subprocess.check_output('cp ' + spu_elf + ' ' + spu_outdir, shell=True)

spu_name = 'responder'
raw_spu_ppu_to_spu = ['LLR_LOST_EVENT', 'GETLLAR_POLLING', 'SPU_INBOUND_MAILBOX', 'SIGNAL_NOTIFICATION']
raw_spu_spu_to_ppu = ['SPU_OUTBOUND_MAILBOX', 'SPU_OUTBOUND_INTERRUPT_MAILBOX', 'DMA_PUT', 'ATOMIC_PUTLLUC']
dir = 'ppu-side/raw_spu'
name = 'ppu_side_raw_spu'
build()

raw_spu_ppu_to_spu = ['LLR_LOST_EVENT', 'GETLLAR_POLLING', 'SIGNAL_NOTIFICATION']
raw_spu_spu_to_ppu = ['EVENT_QUEUE_SEND', 'EVENT_QUEUE_THROW', 'DMA_PUT', 'ATOMIC_PUTLLUC']
dir = 'ppu-side/spu_thread'
name = 'ppu_side_spu_thread'
build()

spu_name = 'observer'
raw_spu_ppu_to_spu = ['GETLLAR_POLLING', 'SPU_INBOUND_MAILBOX', 'SIGNAL_NOTIFICATION']
raw_spu_spu_to_ppu = ['SPU_OUTBOUND_MAILBOX', 'SPU_OUTBOUND_INTERRUPT_MAILBOX', 'DMA_PUT', 'ATOMIC_PUTLLUC']
dir = 'spu-side/raw_spu'
name = 'spu_side_raw_spu'
build()

spu_name = 'observer'
raw_spu_ppu_to_spu = ['GETLLAR_POLLING']
raw_spu_spu_to_ppu = ['SPU_OUTBOUND_INTERRUPT_MAILBOX_HANDLE']
dir = 'spu-side/raw_spu'
name = 'spu_side_raw_spu'
build()

raw_spu_ppu_to_spu = ['GETLLAR_POLLING', 'SIGNAL_NOTIFICATION']
raw_spu_spu_to_ppu = ['EVENT_QUEUE_SEND', 'EVENT_QUEUE_THROW', 'DMA_PUT', 'ATOMIC_PUTLLUC']
dir = 'spu-side/spu_thread'
name = 'spu_side_spu_thread'
build()

# run with :
# "c:\Program Files (x86)\SN Systems\PS3\bin\ps3run.exe" -r -k -w -p
#     -h c:\usr\local\cell\samples\tutorial\SpeMfcTutorial\ppu_spu_round_trip\output\ppu_side_raw_spu\GETLLAR_POLLING___SPU_OUTBOUND_MAILBOX\app_home
#     -f c:\usr\local\cell\samples\tutorial\SpeMfcTutorial\ppu_spu_round_trip\output\ppu_side_raw_spu\GETLLAR_POLLING___SPU_OUTBOUND_MAILBOX\app_home
#     c:\usr\local\cell\samples\tutorial\SpeMfcTutorial\ppu_spu_round_trip\output\ppu_side_raw_spu\GETLLAR_POLLING___SPU_OUTBOUND_MAILBOX\a.self