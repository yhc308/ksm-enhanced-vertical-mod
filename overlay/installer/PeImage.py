from pathlib import Path
import struct

class PeImage:
    """Read PE metadata without mapping or executing an image."""
    def __init__(self, data):
        self.data = data
        if self.u16(0) != 0x5a4d: raise ValueError('Invalid DOS signature')
        nt = self.u32(0x3c)
        if self.u32(nt) != 0x4550: raise ValueError('Invalid PE signature')
        if self.u16(nt+4) != 0x8664: raise ValueError('Expected x64 machine')
        count, optional_size = self.u16(nt+6), self.u16(nt+20)
        optional = nt+24
        if not 1 <= count <= 96 or optional_size < 128 or self.u16(optional) != 0x20b:
            raise ValueError('Invalid PE32+ headers')
        self.slice(optional, optional_size)
        self.headers = self.u32(optional+60)
        self.image_size = self.u32(optional+56)
        self.virtual_sections = []
        self.directories = optional+112
        self.directory_count = min(self.u32(optional+108), (optional_size-112)//8)
        self.sections = []
        for i in range(count):
            sec = optional+optional_size+i*40
            self.slice(sec,40)
            self.sections.append((self.u32(sec+12),self.u32(sec+16),self.u32(sec+20)))
            self.virtual_sections.append((self.u32(sec+12),max(self.u32(sec+8),self.u32(sec+16))))
    def slice(self, offset, size):
        if offset < 0 or size < 0 or offset > len(self.data) or size > len(self.data)-offset:
            raise ValueError('Truncated PE image')
        return self.data[offset:offset+size]
    def u16(self, offset): return struct.unpack('<H',self.slice(offset,2))[0]
    def u32(self, offset): return struct.unpack('<I',self.slice(offset,4))[0]
    def u64(self, offset): return struct.unpack('<Q',self.slice(offset,8))[0]
    def rva(self, address, size=1):
        if address < self.headers:
            self.slice(address,size); return address
        for base,raw_size,raw in self.sections:
            if address >= base and address-base < raw_size and size <= raw_size-(address-base):
                result=raw+address-base; self.slice(result,size); return result
        raise ValueError('PE RVA has no file-backed data')
    def string(self, address):
        chars=[]
        for i in range(4096):
            value=self.data[self.rva(address+i)]
            if not value: return bytes(chars).decode('ascii')
            chars.append(value)
        raise ValueError('Unterminated PE string')
    def directory(self, number):
        if number >= self.directory_count: return (0,0)
        return self.u32(self.directories+number*8),self.u32(self.directories+number*8+4)
    def imports(self):
        address,size=self.directory(1)
        if not address: return {}
        result={}
        for i in range(min(size//20,4096)):
            desc=self.rva(address+i*20,20)
            lookup,name,iat=self.u32(desc),self.u32(desc+12),self.u32(desc+16)
            if not (lookup or name or iat): return result
            dll=self.string(name).lower(); symbols=[]
            for j in range(65536):
                value=self.u64(self.rva((lookup or iat)+j*8,8))
                if not value: break
                if value >> 63: raise ValueError('Ordinal import requires explicit support')
                symbols.append(self.string(value+2))
            else: raise ValueError('Unterminated import table')
            result[dll]=sorted(set(symbols))
        raise ValueError('Unterminated import descriptors')
    def exports(self):
        address,size=self.directory(0)
        if not address: return set()
        exp=self.rva(address,40)
        functions,names=self.u32(exp+20),self.u32(exp+24)
        if max(functions,names)>65536: raise ValueError('Excessive export table')
        result=set()
        for i in range(names):
            name=self.u32(self.rva(self.u32(exp+32)+i*4,4))
            ordinal=self.u16(self.rva(self.u32(exp+36)+i*2,2))
            if ordinal>=functions: raise ValueError('Invalid export ordinal')
            value=self.u32(self.rva(self.u32(exp+28)+ordinal*4,4))
            if value:
                if not (value < self.image_size and any(base <= value < base+size for base,size in self.virtual_sections)):
                    raise ValueError('Export address outside the mapped image')
                result.add(self.string(name))
        return result
