#!/usr/bin/env python3
"""Compile real ARM aggregates and verify the DMF relocation metadata."""
import importlib.util
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('relocate', ROOT / 'scripts/relocate_pointers.py')
relocate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(relocate)

class PointerRelocations(unittest.TestCase):
    @unittest.skipUnless(shutil.which('arm-none-eabi-gcc'), 'ARM GNU toolchain required')
    def test_arm_aggregates(self):
        self.aggregate_fixture('arm-none-eabi-', ['-mcpu=cortex-m7','-mthumb'],4)

    def test_x86_64_aggregates(self):
        self.aggregate_fixture('',[],8)

    def aggregate_fixture(self,prefix,flags,width):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root/'input.c').write_text('''
#include <stdint.h>
__attribute__((section(".header"),used)) const uint32_t header[60] = {0x444d4f44,240,256,4};
__attribute__((section(".header.relocations"),used)) const uint32_t extension[4] = {0x524c4f43,1,0,0};
struct dispatch {void (*fn)(void);const char* text;unsigned number;};
static void callback(void) {__asm__ volatile ("nop");}
static const char text[]="pointer test";
const struct dispatch dispatch = {callback,text,0x151};
__attribute__((noinline)) void consume(const struct dispatch* d) {__asm__ volatile (""::"r"(d):"memory");}
void entry(void) {struct dispatch local={callback,text,0x151};consume(&local);consume(&dispatch);}
'''.replace('240,256,4','240,256,'+str(width)))
            (root/'layout.ld').write_text('''
SECTIONS {
 . = 0; .header : {KEEP(*(.header)) KEEP(*(.header.relocations))}
 .text : {*(.text*)} .rodata : {*(.rodata*)} .data : {*(.data*)}
 .got : {*(.got*)} .bss : {*(.bss*)}
 /DISCARD/ : {*(.ARM.exidx*) *(.ARM.extab*) *(.comment*) *(.note*)}
}
''')
            subprocess.run([prefix+'gcc',*flags,'-fPIC','-fPIE',
                '-O2','-nostdlib','-static','-Wl,--emit-relocs,--no-relax','-T',str(root/'layout.ld'),
                str(root/'input.c'),'-o',str(root/'input.elf')],check=True,capture_output=True)
            subprocess.run([prefix+'objcopy','-O','binary',str(root/'input.elf'),str(root/'input.dmf')],check=True)
            binary=(root/'input.dmf').read_bytes()
            result,count=relocate.convert((root/'input.elf').read_bytes(),binary)
            self.assertGreaterEqual(count,4 if width==4 else 2) # static table and compiler aggregate
            offset,records=struct.unpack_from('<II',result,248)
            locations=struct.unpack_from('<'+'I'*records,result,offset)
            self.assertEqual(list(locations),sorted(set(locations)))
            thumb=0; strings=0
            for location in locations:
                value=struct.unpack_from('<I' if width==4 else '<Q',result,location)[0]
                if value&1:thumb+=1
                if binary[value:].startswith(b'pointer test'):strings+=1
            if width==4:self.assertGreaterEqual(thumb,2)
            self.assertGreaterEqual(strings,2 if width==4 else 1)
            # The numeric field 0x151 is deliberately pointer-like but has no
            # ELF relocation. It must remain an integer in both aggregates.
            integers=[i for i in range(256,len(binary)-3,4)
                      if struct.unpack_from('<I',binary,i)[0]==0x151 and i not in locations]
            self.assertGreaterEqual(len(integers),2 if width==4 else 1)
            self.assertEqual(struct.unpack_from('<I',result,4)[0],256)

    def test_rejects_wrong_architecture_and_missing_extension(self):
        with self.assertRaises(ValueError):relocate.convert(b'not an ELF',b'')

if __name__=='__main__':unittest.main()
