import enum

FONT_HEADER_SIZE = 4
CHAR_TABLE_ENTRY_SIZE = 2
ID_POSTFIX_CHAR_TABLE = 'char_table'
ID_POSTFIX_SEG_TABLE = 'seg_table'
ID_POSTFIX_GLPYH_DATA = 'glyph_data'
SEG_HEIGHT = 8
LOG_INDENT = '    '

class Char:
    def __init__(self, x: int, y: int, w: int, code:int, pix: list[int]):
        self.x = x
        self.y = y
        self.width = w
        self.code = code
        self.pix = pix

class Font:
    def __init__(self):
        self.height = -1
        self.chars: dict[int, Char] = {}

class OpCode(enum.IntEnum):
    LD = 0x00
    SLC = 0x40
    SLS = 0x50
    SRC = 0x60
    SRS = 0x70
    CPY = 0x80
    REV = 0xc0
    RPT = 0xe0
    XOR = 0xf0
    HDR = -1

class ShiftDir(enum.IntEnum):
    LEFT = 0
    RIGHT = 1

priority = {
    OpCode.REV: 1,
    OpCode.CPY: 2,
    OpCode.LD: 3,
    OpCode.SLC: 4,
    OpCode.SRC: 4,
    OpCode.SLS: 5,
    OpCode.SRS: 5,
    OpCode.XOR: 6,
    OpCode.RPT: 8,
    OpCode.HDR: -1,
}

class InstBase:
    def __init__(self, op: OpCode, size: int):
        self.op = op
        self.size = size

    def op_offset(self) -> int:
        return None
    
    def get_byte(self) -> int:
        return self.op.value + self.op_offset()

    def score(self) -> int:
        return self.size * 0x100 + priority[self.op]

class RawByte(InstBase):
    def __init__(self, value: int):
        super().__init__(OpCode.HDR, 0)
        self.value = value

    def get_byte(self) -> int:
        return self.value

class SingleOp(InstBase):
    def __init__(self, op: OpCode):
        super().__init__(op, 1)

class LoadOp(SingleOp):
    def __init__(self, seg_data: int):
        super().__init__(OpCode.LD)
        self.seg_data = seg_data
        self.seg_index = -1

    def op_offset(self):
        return self.seg_index

class XorOp(SingleOp):
    def __init__(self, width: int, pos: int):
        super().__init__(OpCode.XOR)
        self.width = width
        self.bit = pos

    def op_offset(self):
        return (self.width - 1) * 8 + self.bit
    
class BlockOp(InstBase):
    def __init__(self, op: OpCode, size: int):
        super().__init__(op, size)

    def op_offset(self):
        return self.size - 1

class CopyOp(BlockOp):
    def __init__(self, size: int, offset: int):
        super().__init__(OpCode.CPY, size)
        self.offset = offset

    def op_offset(self):
        return \
            self.offset * 16 + \
            super().op_offset()

class RepeatOp(BlockOp):
    def __init__(self, size: int):
        super().__init__(OpCode.RPT, size)

class ShiftOp(BlockOp):
    def __init__(self, size: int, shift_dir: ShiftDir, post_op: int, shift_size: int):
        if shift_dir == ShiftDir.LEFT:
            if post_op == 0:
                op = OpCode.SLC
            else:
                op = OpCode.SLS
        else:
            if post_op == 0:
                op = OpCode.SRC
            else:
                op = OpCode.SRS
        super().__init__(op, size)
        self.post_op = post_op
        self.shift_dir = shift_dir
        self.shift_size = shift_size

    def op_offset(self):
        return \
            (self.shift_size - 1) * 0x8 + \
            super().op_offset()

def is_red(pix: list[int]) -> bool:
    return pix[0] >= 192 and pix[1] < 64 and pix[2] < 64

def is_green(pix: list[int]) -> bool:
    return pix[0] < 64 and pix[1] >= 192 and pix[2] < 64

def is_white(pix: list[int]) -> bool:
    return pix[0] >= 192 and pix[1] >= 192 and pix[2] >= 192

def format_char(c: int) -> str:
    if c >= 0x20 and c <= 0x7E:
        return f"'{chr(c)}' (0x{c:x})"
    else:
        return f'\\x{c:02x}'

def verbose_print(s: str = '') -> None:
    print(s)
        
