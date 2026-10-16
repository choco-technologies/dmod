#!/usr/bin/env python3
"""Add explicit absolute pointer fixups to an ARM ELF32 or x86-64 DMF (stdlib only).

--emit-relocs preserves linker-resolved references. PC-relative references need
no fixup. Existing header/API/GOT/license relocations remain owned by the loader.
Unknown absolute instruction relocations are rejected instead of silently
producing a non-position-independent module.
"""
import struct
import sys
from pathlib import Path


def convert(elf, binary):
    if len(elf) < 64 or elf[:4] != b'\x7fELF' or elf[5] != 1 or elf[4] not in (1,2):
        raise ValueError('Expected little-endian ELF32/ELF64')
    machine = struct.unpack_from('<H', elf, 18)[0]
    wide = elf[4] == 2
    if (machine,wide) not in ((40,False),(62,True)):
        raise ValueError('Expected ARM ELF32 or x86-64 ELF64')
    width = 8 if wide else 4
    shoff = struct.unpack_from('<Q' if wide else '<I',elf,40 if wide else 32)[0]
    entsize,count,names_index = struct.unpack_from('<HHH',elf,58 if wide else 46)
    section_format = '<IIQQQQIIQQ' if wide else '<10I'
    if entsize != struct.calcsize(section_format) or not count or names_index >= count:
        raise ValueError('Invalid section directory')
    sections = [struct.unpack_from(section_format,elf,shoff+i*entsize) for i in range(count)]
    names = sections[names_index]
    strings = elf[names[4]:names[4] + names[5]]
    def name(section):
        start = section[0]
        return strings[start:strings.index(b'\0', start)].decode('ascii')
    by_name = {name(s): s for s in sections}
    header = by_name['.header']
    if header[3] != 0 or header[5] != 256:
        raise ValueError('Expected legacy 240-byte header plus 16-byte relocation extension')
    if struct.unpack_from('<I', binary)[0] != 0x444d4f44 or struct.unpack_from('<I', binary, 12)[0] != width:
        raise ValueError('Invalid DMF pointer size')
    if struct.unpack_from('<2I', binary, 240) != (0x524c4f43, 1):
        raise ValueError('Missing relocation extension')
    excluded = [(s[3], s[3] + s[5]) for n, s in by_name.items()
                if n in ('.header', '.inputs', '.outputs', '.got', '.footer')]
    # License.Text has its own historical loader relocation.
    for section in sections:
        if section[1] != 2:
            continue
        strings_section = sections[section[6]]
        symbol_names = elf[strings_section[4]:strings_section[4] + strings_section[5]]
        for pos in range(section[4], section[4] + section[5], 24 if wide else 16):
            if wide:
                n,info,other,index,value,size = struct.unpack_from('<IBBHQQ',elf,pos)
            else:
                n,value,size,info,other,index = struct.unpack_from('<IIIBBH',elf,pos)
            if symbol_names[n:symbol_names.index(b'\0', n)] == b'License' and size >= width:
                excluded.append((value + size - width, value + size))
    offsets = set()
    for section in sections:
        if section[1] != (4 if wide else 9): # SHT_RELA / SHT_REL
            continue
        target = sections[section[7]]
        if not target[2] & 2:
            continue
        symbols = sections[section[6]]
        record_size = 24 if wide else 8
        if section[9] != record_size:
            raise ValueError('Invalid relocation record size')
        for pos in range(section[4],section[4]+section[5],record_size):
            offset,info = struct.unpack_from('<QQ' if wide else '<II',elf,pos)
            kind,symbol = (info&0xffffffff,info>>32) if wide else (info&255,info>>8)
            if any(start <= offset < end for start,end in excluded):
                continue
            symbol_index = struct.unpack_from('<H',elf,symbols[4]+symbol*(24 if wide else 16)+(6 if wide else 14))[0]
            if symbol_index in (0,0xfff1):
                continue
            absolute = (1,) if wide else (2,38)
            relative = (0,2,4,9,24,25,26,27,28,29,41,42) if wide else (0,3,10,24,25,26,28,29,30,42,96,51,102,103)
            if kind in absolute:
                if offset+width > len(binary):
                    raise ValueError('Pointer relocation outside image')
                value = struct.unpack_from('<Q' if wide else '<I',binary,offset)[0]
                if value >= len(binary):
                    raise ValueError('Absolute pointer outside module')
                offsets.add(offset)
            elif kind not in relative:
                raise ValueError(f'Unsupported relocation {kind} at {offset:#x}')
    result = bytearray(binary)
    while len(result) % 4:
        result.append(0)
    table = len(result)
    for offset in sorted(offsets):
        result.extend(struct.pack('<I', offset))
    struct.pack_into('<I', result, 4, header[5])
    struct.pack_into('<II', result, 248, table, len(offsets))
    return result, len(offsets)


def main():
    elf, output = map(Path, sys.argv[1:])
    result, count = convert(elf.read_bytes(), output.read_bytes())
    output.write_bytes(result)
    print(f'{count} absolute pointer relocations recorded')


if __name__ == '__main__':
    main()
