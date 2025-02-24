#!/usr/bin/env python3

from PIL import Image
import argparse
import numpy as np
from itertools import product
from mamefont import *
import re
import os

class CompressContext:
    def __init__(self, char: Char):
        self.char: Char = char
        self.pos: int = 0
        self.last: int = 0
        self.end_pos: int = len(char.pix)
        self.program: list[InstBase] = [
            RawByte(char.width),
        ]
        
class Png2Cpp:
    def __init__(self):
        self.font = Font()
        self.offset_y = 0
        self.input_path: str = None
        self.cpp_output_path: str = None
        self.hpp_output_path: str = None
        self.include_dir: str = None
        self.name: str = None
        self.cpp_namespace: str = None
        self.seg_util: dict[int, int] = {}
        self.char_dict: dict[int, Char] = {}
        self.min_code: int = 0
        self.max_code: int = 255
        self.num_rows = 0
        self.seg_table = []
        self.char_table = []
        self.glyph_data = []

    def report_util(self):
        verbose_print(f"Num of segments: {len(self.seg_util)}")
        verbose_print(f"Num of chars: {len(self.char_dict)}")
        #verbose_print(f"Raw data size: {self.num_raw_segs}")

    def load_image(self):
        # 画像を配列化
        img = Image.open(self.input_path)
        img_w, img_h = img.size
        pix = np.array([[img.getpixel((x,y)) for x in range(img_w)] for y in range(img_h)])

        # 赤いピクセルを見つけてフォントの高さを決定
        self.offset_y = -1
        self.font.height = -1
        for base_y in range(img_h):
            for char_x in range(img_w):
                # 赤いピクセルより先に白いピクセルが見つかった場合はそこを高さの基準点とする
                if self.offset_y < 0 and self.font.height < 0 and is_white(pix[base_y][char_x]):
                    self.offset_y = base_y
                # 基準点から赤いピクセルまでの距離をフォントの高さとする
                if is_red(pix[base_y][char_x]):
                    self.font.height = base_y - self.offset_y
                    break
            if self.font.height > 0:
                break

        verbose_print(f'Recognizing characters...')
        
        if self.offset_y < 0:
            self.offset_y = 0
        verbose_print(f'{LOG_INDENT}Y offset: {self.offset_y}')

        if self.font.height <= 0:
            raise Exception('Red pixel not found')
        self.num_rows = (self.font.height + SEG_HEIGHT - 1) // SEG_HEIGHT
        verbose_print(f'{LOG_INDENT}Font height: {self.font.height} (num rows: {self.num_rows})')

        # 文字を列挙する
        code = 0x20
        for base_y in range(self.font.height, img_h):
            base_x = 0
            while base_x < img_w:
                char_y = base_y - self.font.height
                
                # 赤い線を認識する
                if not is_red(pix[base_y][base_x]):
                    base_x += 1
                    continue
                char_x = base_x
                base_x += 1
                char_w = 1
                while base_x < img_w and is_red(pix[base_y][base_x]):
                    base_x += 1
                    char_w += 1

                # 文字コードの読み取り
                if is_green(pix[base_y + 2][char_x]):
                    code = 0
                    code_x = char_x
                    while code_x < img_w and is_green(pix[base_y + 2][code_x]):
                        code <<= 1
                        if is_green(pix[base_y + 1][code_x]):
                            code |= 1
                        code_x += 1
                    verbose_print(f'{LOG_INDENT}Code offset changed: {format_char(code)}')
                
                # セグメントの抽出
                data: list[int] = []
                for row in range(self.num_rows):
                    seg_y = char_y + row * SEG_HEIGHT
                    for col in range(char_w):
                        seg_x = char_x + col
                        
                        # セグメントのビット列を抽出
                        sreg = 0
                        for i in range(SEG_HEIGHT):
                            pix_y = seg_y + i
                            sreg <<= 1
                            if pix_y < base_y and is_white(pix[pix_y][seg_x]):
                                sreg |= 1
                        data.append(sreg)
                        
                        # セグメントの出現数を数える
                        if sreg in self.seg_util:
                            self.seg_util[sreg] += 1    
                        else:
                            self.seg_util[sreg] = 1
                
                assert len(data) == self.num_rows * char_w
                #self.num_raw_segs += self.num_rows * char_w

                # 文字辞書に追加        
                if code in self.char_dict:
                    raise Exception(f'Duplicate character code: {format_char(code)}')
                
                self.char_dict[code] = Char(char_x, base_y, char_w, code, data)

                verbose_print(f'{LOG_INDENT}Character defined: code={format_char(code)}, width={char_w}')

                code += 1
        verbose_print()
        
    def compress(self) -> None:
        codes = sorted(self.char_dict.keys())
        self.min_code = codes[0]
        self.max_code = codes[-1]

        program: list[InstBase] = []

        # self.char_dict のキーを小さい順に取り出す
        used_codes = sorted(self.char_dict.keys())

        # 圧縮
        verbose_print(f'Compressing...')
        char_table_depth = (self.max_code - self.min_code + 1)
        char_table_dummy_value = (1 << (8 * CHAR_TABLE_ENTRY_SIZE)) - 1
        self.char_table = [char_table_dummy_value] * char_table_depth
        for code in used_codes:
            ctx = CompressContext(self.char_dict[code])
            self.compress_char(ctx)
            self.char_table[code - self.min_code] = len(program)
            program += ctx.program
        verbose_print()

        # セグメントテーブル生成
        verbose_print(f'Generating segment table...')
        seg_util = {}
        for inst in program:
            if inst.op == OpCode.LD:
                ld: LoadOp = inst
                if ld.seg_data in seg_util:
                    seg_util[ld.seg_data] += 1
                else:
                    seg_util[ld.seg_data] = 1
        self.seg_table = sorted(seg_util.keys(), key=lambda x: seg_util[x], reverse=True)
        seg_table_inv = {}
        for i in range(len(self.seg_table)):
            seg_table_inv[self.seg_table[i]] = i

        # グリフデータ生成
        verbose_print(f'Generating glyph data...')
        self.glyph_data = []
        for inst in program:
            if inst.op == OpCode.LD:
                ld: LoadOp = inst
                ld.seg_index = seg_table_inv[ld.seg_data]
            self.glyph_data.append(inst.get_byte())
        
        # 頻出セグメント
        verbose_print(f'Segment Table:')
        for seg in self.seg_util:
            if seg not in self.seg_table:
                self.seg_table.append(seg)
        verbose_print(f'{LOG_INDENT}Index | Bits       | Before | After')
        verbose_print(f'{LOG_INDENT}------+------------+--------+-------')
        for i in range(len(self.seg_table)):
            seg = self.seg_table[i]
            pre = self.seg_util[seg]
            post = 0
            if seg in seg_util:
                post = seg_util[seg]
            msg = LOG_INDENT
            if post > 0:
                msg += f'{i:5}'
            else:
                msg += f'     '
            msg += f' | 0x{seg:08b} | {pre:6}'
            if post > 0:
                msg += f' | {post:5}'
            elif post == 0:
                msg += ' | (Deleted)'
            verbose_print(msg)
        verbose_print()    
        
        total_pre = FONT_HEADER_SIZE
        total_post = FONT_HEADER_SIZE
        
        code_table_size = char_table_depth * CHAR_TABLE_ENTRY_SIZE
        seg_table_size = len(seg_util.keys())
        total_pre += code_table_size
        total_post += code_table_size
        
        total_post += seg_table_size + len(program)
        
        pre_util: dict[OpCode, int] = {}
        post_util: dict[OpCode, int] = {}
        for inst in program:
            if inst.op == OpCode.HDR:
                pass
            elif inst.op in post_util:
                pre_util[inst.op] += inst.size
                post_util[inst.op] += 1
            else:
                pre_util[inst.op] = inst.size
                post_util[inst.op] = 1
            total_pre += inst.size

        verbose_print(f'Compress performance:')
        verbose_print(f'{LOG_INDENT}Inst. | Before (Util) | After  (Util) | Diff (Comp Ratio)')
        verbose_print(f'{LOG_INDENT}------+---------------+---------------+-------------------')
        for op in pre_util:
            post = post_util[op]
            pre = pre_util[op]
            msg = LOG_INDENT
            msg += f'{op.name:5} |'
            msg += f' {pre:4} ({pre / total_pre:6.1%})'
            msg += f' | {post:4} ({post / total_post:6.1%})'
            msg += f' | {post-pre:+5} ({1 - post / pre:6.1%})'
            verbose_print(msg)
        verbose_print(f'{LOG_INDENT}C-Tbl | {code_table_size:4} ({code_table_size / total_pre:6.1%}) | {code_table_size:4} ({code_table_size / total_post:6.1%}) | {0:+5}')
        verbose_print(f'{LOG_INDENT}S-Tbl | {0:4} ({0:6.1%}) | {seg_table_size:4} ({seg_table_size / total_post:6.1%}) | {seg_table_size:+5}')
        verbose_print(f'{LOG_INDENT}------+---------------+---------------+-------------------')
        msg = LOG_INDENT
        msg += f'Total |'
        msg += f' {total_pre:4} (100.0%)'
        msg += f' | {total_post:4} (100.0%)'
        msg += f' | {total_post-total_pre:+5} ({1 - total_post / total_pre:6.1%})'
        verbose_print(msg)
        verbose_print()

    def compress_char(self, ctx: CompressContext) -> None:
        while ctx.pos < ctx.end_pos:
            cands: list[InstBase] = []
            cands += self.suggest_load(ctx)
            cands += self.suggest_shift(ctx)
            cands += self.suggest_copy(ctx)
            cands += self.suggest_repeat(ctx)
            cands += self.suggest_xor(ctx)

            # 最適なものを選ぶ
            cands.sort(key=lambda x: x.score())
            best = cands[-1]
            
            ctx.program.append(best)
            
            ctx.pos += best.size
            ctx.last = ctx.char.pix[ctx.pos - 1]

        msg = f'{LOG_INDENT}{format_char(ctx.char.code)}:'
        for inst in ctx.program:
            msg += f' {inst.op.name}'
        verbose_print(msg)
        
    def suggest_shift(self, ctx: CompressContext) -> None:
        cands: list[InstBase] = []
        max_size = min(8, ctx.end_pos - ctx.pos)
        for shift_dir, post_op, shift_size in product(ShiftDir, range(0, 2), range(1, 3)):
            work = ctx.last
            size = 0
            while True:
                for i in range(shift_size):
                    if shift_dir == ShiftDir.LEFT:
                        if post_op == 0:
                            work = (work << 1) & 0xfe
                        else:
                            work = (work << 1) | 0x01
                    else:
                        if post_op == 0:
                            work = (work >> 1) & 0x7f
                        else:
                            work = (work >> 1) | 0x80
                if size < max_size and ctx.char.pix[ctx.pos + size] == work:
                    size += 1
                else:
                    break
            if size > 0:
                cands.append(ShiftOp(size, shift_dir, post_op, shift_size))
        return cands

    def suggest_repeat(self, ctx: CompressContext) -> list[InstBase]:
        cands: list[InstBase] = []
        max_size = min(16, ctx.end_pos - ctx.pos)
        size = 0
        while size < max_size and ctx.char.pix[ctx.pos + size] == ctx.last:
            size += 1
        if size > 0:
            cands.append(RepeatOp(size))
        return cands
    
    def suggest_copy(self, ctx: CompressContext) -> list[InstBase]:
        cands: list[InstBase] = []
        max_size = min(16, ctx.end_pos - ctx.pos)
        for size in range(max_size, 0, -1):
            for ofst in range(0, 4):
                if ctx.pos - ofst - size < 0:
                    break
                for i in range(size):
                    i_src = ctx.pos - ofst - size + i
                    i_dst = ctx.pos + i
                    if ctx.char.pix[i_dst] != ctx.char.pix[i_src]:
                        break
                    if i == size - 1:
                        cands.append(CopyOp(size, ofst))
            if len(cands) > 0:
                break
        return cands
    
    def suggest_load(self, ctx: CompressContext) -> list[InstBase]:
        return [LoadOp(ctx.char.pix[ctx.pos])]
        
    def suggest_xor(self, ctx: CompressContext) -> list[InstBase]:
        for width in range(1, 3):
            mask = (1 << width) - 1
            for pos in range(SEG_HEIGHT - width + 1):
                seg = ctx.char.pix[ctx.pos]
                if seg == ctx.last ^ (mask << pos):
                    return [XorOp(width, pos)]
        return []
    
    def get_file_name(self, cpp: bool) -> str:
        path = self.cpp_output_path if cpp else self.hpp_output_path

        if not path:
            if self.cpp_output_path:
                path = self.cpp_output_path
            elif self.hpp_output_path:
                path = self.hpp_output_path
            else:
                path = self.input_path
            
            path = os.path.splitext(path)[0]
            path = path + ('.cpp' if cpp else '.hpp')

        return path
    
    def get_font_name(self) -> str:
        name = self.name
        if not name:
            name = os.path.basename(self.get_file_name(False))
            name = os.path.splitext(name)[0]
        return name
    
    def write_includes(self, f, cpp: bool) -> None:
        f.write('#include <stdint.h>\n')
        f.write('#include "mamefont/mamefont.hpp"\n')
        if cpp:
            hpp_path = os.path.basename(self.get_file_name(False))
            if self.include_dir:
                hpp_path = os.path.join(self.include_dir, hpp_path)
            f.write(f'#include "{hpp_path}"\n')
            
        f.write('\n')
    
    def write_namespace_start(self, f) -> None:
        if self.cpp_namespace:
            f.write(f'namespace {self.cpp_namespace} {{\n\n')
    
    def write_namespace_end(self, f) -> None:
        if self.cpp_namespace:
            f.write('}\n')
    
    def write_array_content(self, f, elem_size: int, id: str, with_content: bool, array: list[int], cols: int = 0):
        if cols <= 0:
            cols = 16 // elem_size
        type_name = {1: 'uint8_t', 2: 'uint16_t', 4: 'uint32_t'}[elem_size]
        f.write(f'const {type_name} {id}[]')
        if with_content:
            f.write(' = {\n')
            n = len(array)
            for i_coarse in range(0, n, cols):
                f.write('  ')
                for i in range(i_coarse, min(n, i_coarse + cols)):
                    if elem_size == 1:
                        f.write(f'0x{array[i]:02x}, ')
                    elif elem_size == 2:
                        f.write(f'0x{array[i]:04x}, ')
                    elif elem_size == 4:
                        f.write(f'0x{array[i]:08x}, ')
                f.write('\n')
            f.write('}')
        f.write(';\n\n')
    
    def write_char_table(self, f, with_content: bool):
        id = f'{self.get_font_name()}_{ID_POSTFIX_CHAR_TABLE}'
        self.write_array_content(f, CHAR_TABLE_ENTRY_SIZE, id, with_content, self.char_table)
    
    def write_seg_table(self, f, with_content: bool):
        id = f'{self.get_font_name()}_{ID_POSTFIX_SEG_TABLE}'
        self.write_array_content(f, 1, id, with_content, self.seg_table)
    
    def write_glyph_data(self, f, with_content: bool):
        id = f'{self.get_font_name()}_{ID_POSTFIX_GLPYH_DATA}'
        self.write_array_content(f, 1, id, with_content, self.glyph_data)
    
    def write_font_inst(self, f, cpp: bool):
        f.write(f'extern const mfnt::Font {self.get_font_name()}')
        if cpp:
            f.write('(\n')
            f.write(f'  {self.font.height},\n')
            f.write(f'  {self.max_code - self.min_code + 1},\n')
            f.write(f'  {self.min_code},\n')
            f.write(f'  0x00,\n')
            f.write(f'  {self.get_font_name()}_{ID_POSTFIX_CHAR_TABLE},\n')
            f.write(f'  {self.get_font_name()}_{ID_POSTFIX_SEG_TABLE},\n')
            f.write(f'  {self.get_font_name()}_{ID_POSTFIX_GLPYH_DATA}\n')
            f.write(')')
        f.write(';\n\n')
    
    def write_hpp(self):
        with open(self.get_file_name(False), 'w') as f:
            f.write('#pragma once\n\n')
            self.write_includes(f, False)
            self.write_namespace_start(f)
            self.write_font_inst(f, False)
            self.write_namespace_end(f)
    
    def write_cpp(self):
        with open(self.get_file_name(True), 'w') as f:
            self.write_includes(f, True)
            self.write_namespace_start(f)
            self.write_char_table(f, True)
            self.write_seg_table(f, True)
            self.write_glyph_data(f, True)
            self.write_font_inst(f, True)
            self.write_namespace_end(f)
    
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input', required=True)
    parser.add_argument('-n', '--name', default=None)
    parser.add_argument('-o', '--output_dir', default=None)
    parser.add_argument('--output_cpp', default=None)
    parser.add_argument('--output_hpp', default=None)
    parser.add_argument('--include_dir', default=None)
    parser.add_argument('--cpp_namespace', default=None)
    args = parser.parse_args()
    
    conv = Png2Cpp()
    conv.input_path = args.input
    
    if args.name:
        conv.name = args.name
    elif args.input:
        conv.name = os.path.basename(args.input)
        conv.name = os.path.splitext(conv.name)[0]
    
    conv.cpp_output_path = conv.name + '.cpp'
    conv.hpp_output_path = conv.name + '.hpp'
    if args.output_dir:
        conv.cpp_output_path = os.path.join(args.output_dir, conv.cpp_output_path)
        conv.hpp_output_path = os.path.join(args.output_dir, conv.hpp_output_path)
    if args.output_cpp:
        conv.cpp_output_path = args.output_cpp
    if args.output_hpp:
        conv.hpp_output_path = args.output_hpp
    
    conv.include_dir = args.include_dir
    conv.cpp_namespace = args.cpp_namespace

    conv.load_image()
    conv.compress()
    
    conv.write_hpp()
    conv.write_cpp()
