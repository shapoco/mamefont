import enum

SEG_HEIGHT = 8
LOG_INDENT = '    '

class Char:
    def __init__(self, x: int, y: int, w: int, code:int, pix: list[int]):
        self.x = x
        self.y = y
        self.w = w
        self.code = code
        self.pix = pix

class Font:
    def __init__(self):
        self.height = -1
        self.chars: dict[int, Char] = {}

class OpCode(enum.IntEnum):
    LD = 0x00
    LSL = 0x40
    LSR = 0x50
    ASL = 0x60
    ASR = 0x70
    CPY = 0x80
    REV0 = 0xc0
    REV1 = 0xd0
    RPT = 0xe0
    XOR = 0xf0

class ShiftDir(enum.IntEnum):
    LEFT = 0
    RIGHT = 1

class ShiftMode(enum.IntEnum):
    LOGICAL = 0
    ARITHMETIC = 1

priority = {
    OpCode.REV1: 0,
    OpCode.REV0: 1,
    OpCode.CPY: 2,
    OpCode.LD: 3,
    OpCode.LSL: 4,
    OpCode.LSR: 4,
    OpCode.ASL: 5,
    OpCode.ASR: 5,
    OpCode.XOR: 6,
    OpCode.RPT: 8,
}

class InstBase:
    def __init__(self, op: OpCode, size: int):
        self.op = op
        self.size = size

    def op_offset(self) -> int:
        return None
    
    def get_inst_code(self) -> int:
        return self.op.value + self.op_offset()

    def score(self) -> int:
        return self.size * 0x100 + priority[self.op]

class SingleOp(InstBase):
    def __init__(self, op: OpCode):
        super().__init__(op, 1)

class LoadOp(SingleOp):
    def __init__(self, seg: int):
        super().__init__(OpCode.LD)
        self.seg = seg

    def op_offset(self):
        return self.seg

class XorOp(SingleOp):
    def __init__(self, bit: int):
        super().__init__(OpCode.XOR)
        self.bit = bit

    def op_offset(self):
        return self.bit
    
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
    def __init__(self, size: int, mode: ShiftMode, dir: ShiftDir, shift_size: int):
        if mode == ShiftMode.LOGICAL:
            if dir == ShiftDir.LEFT:
                op = OpCode.LSL
            else:
                op = OpCode.LSR
        else:
            if dir == ShiftDir.LEFT:
                op = OpCode.ASL
            else:
                op = OpCode.ASR
        super().__init__(op, size)
        self.mode = mode
        self.dir = dir
        self.shift_size = shift_size

    def op_offset(self):
        return \
            self.mode.value * 5 + \
            self.dir.value * 4 + \
            (self.shift_size - 1) * 3 + \
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
        
