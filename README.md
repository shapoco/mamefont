# [WIP] MameFont

Compressed font format specification and library for tiny-footprint embedded projects.

"Mame" (まめ, 豆) means "bean(s)" in Japanese.

# Concept

![](./img/concept.svg)

![](./img/scope.svg)

# Format Specification

## Blob Structure

|Size \[Bytes\]|Name|
|:--:|:--|
|8|Font Header|
|(4 or 2) \* `numGlyphs`|Glyph Table|
|`lutSize`|Fragment Lookup Table (LUT)|
|(Variable)|Bytecode Blocks|

## Font Header

A structure that provides information common to the entire font.

|Size \[Bytes\]|Value|Description|
|:--:|:--|:--|
|1|`formatVersion`|0x01|
|1|`fontFlags`|Format Flags|
|1|`firstCode`|ASCII code of the first entry of Glyph Table|
|1|`numGlyphs` - 1|`numGlyphs` is number of entries of Glyph Table|
|1|`lutSize` - 1|`lutSize` is number of entries of LUT.<br>2 ≦ `lutSize` ≦ 64, and multiple of 2|
|1|`fontDimension[0]`|Dimension of Font|
|1|`fontDimension[1]`|Dimension of Font|
|1|`fontDimension[2]`|Dimension of Font|

### `fontFlags`

`fontFlags` provides basic flags for the format of the font file.

|Bit Range|Value|Description|
|:--:|:--|:--|
|7|`verticalFragment`|0: Horizontal Fragment,<br>1: Vertical Fragment|
|6|`msb1st`|0: LSB=Near Pixel,<br>1: LSB=Far Pixel|
|5|`shrinkedGlyphTable`|0: Normal Format,<br>1: Shrinked Format|
|4:0|(Reserved)||

![](./img/frag_shape.svg)

### `fontDimension`

|Index|Bit Range|Value|Description|
|:--:|:--:|:--|:--|
|\[0\]|7:6|(Reserved)||
||5:0|`glyphHeight` - 1|`glyphHeight` is height of glyph in pixels|
|\[1\]|7:6|(Reserved)||
||5:0|`ySpacing`|`ySpacing` is the distance in pixels from the bottom of the current line to the top of the next line|
|\[2\]|7:6|(Reserved)||
||5:0|`maxGlyphWidth` - 1|`maxGlyphWidth` is `glyphWidth` value of widest glyph in the font.|

`glyphHeight` + `ySpacing` is same as `yAdvance` of GFXfont.

`maxGlyphWidth` will be used by decompressor to determine size of Glyph Buffer. If the font does not contain any valid glyphs, then `maxGlyphWidth` must have a value of 1 (`fontDimension[2]` = 0x00).

## Glyph Table

### Normal Table Entry (4 Bytes)

|Size \[Bytes\]|Value|Description|
|:--:|:--|:--|
|2|`entryPoint`|Offset from start of Bytecode Block in bytes|
|1|`glyphDimension[0]`|Dimension of the glyph|
|1|`glyphDimension[1]`|Dimension of the glyph|

#### `glyphDimension`

|Index|Bit Range|Value|Description|
|:--:|:--:|:--|:--|
|\[0\]|7:6|(Reserved)||
||5:0|`glyphWidth` - 1|`glyphWidth` is glyph width in pixels|
|\[1\]|7:5|`xNegativeOffset`|The glyph is rendered shifted to the left by the number of pixels specified by `xNegativeOffset`.|
||4:0|`xSpacing` + 16|`xSpacing` is the distance in pixels from the right edge of the current glyph to the left edge of the next glyph. This must grater than `glyphWidth`.|

`glyphWidth` - `xNegativeOffset` + `xSpacing` is same as `xAdvance` of GFXfont. Depending on these values, rendered characters can overlap, but it is up to the renderer implementation to render this as the font designer expected.

### Shrinked Table Entry (2 Bytes)

The Shrinked Format of the Glyph Table can be applied when all of the following conditions are met:

- All `glyphWidth` in the font are in the range of 1 to 16.
- All `xSpacing` in the font are in the range of 0 to 3.
- All `xNegativeOffset` in the font are in the range of 0 to 3.
- Total size of the Bytecode Block is 512 Byte or less.

In this case, all of `entryPoint` must be aligned to 2-Byte boundaries.

|Size \[Bytes\]|Value|Description|
|:--:|:--|:--|
|1|`entryPoint` &gt;&gt; 1|The value of `entryPoint` divided by 2. `entryPoint` must be a multiple of 2.|
|1|`packedGlyphDimension`|Dimension of the glyph|

#### `packedGlyphDimension`

|Bit Range|Value|Description|
|:--:|:--|:--|
|7:6|`xNegativeOffset`|See description of Normal Table Entry.|
|5:4|`xSpacing`|See description of Normal Table Entry.|
|3:0|`glyphWidth` - 1|See description of Normal Table Entry.|

### Missing Glyph

To express that no valid glyph is assigned to a character code, the first two bytes of the Glyph Table Entry should be 0xFFFF.

This also means:

- If not a Shrinked Glyph Table: `entryPoint`=0xFFFF cannot be used.
- If a Shrinked Glyph Table: the combination of `entryPoint`=0x1FE, `glyphWidth`=16, `xSpacing`=16 cannot be used.

Be careful as these can lead to corner case issues.

## Lookup Table

If the total size does not reach a 2-Byte boundary, a dummy byte must be appended.
The value of `lutSize` includes this dummy byte.

## Bytecode Block

A Bytecode Block is the concatenation of all glyph bytecodes. When a Shrinked Glyph Table is applied, the first instruction in each glyph is aligned to a 2-byte boundary, but subsequent instructions are placed immediately after the previous instruction.

|Size \[Bytes\]|Description|
|:--:|:--|
|(Variable)|Array of instructions|

# Instruction Set

## Summary

Each instruction consists of 1 to 3 bytes. The first byte is the "OpCode", and the following bytes are parameters.

### OpCode Map

The number in parentheses indicates the length of the instruction in bytes.

![](./img/opcode_map.svg)

## Single Lookup (`LUP`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:6|0b10|
||5:0|`index`|

The state machine simply copies the fragment in the LUT to the glyph buffer. If msb1st=1 is set, the fragments in the LUT must also be MSB 1st.

![](./img/inst_lup.svg)

## Double Lookup (`LUD`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:5|0b110|
||4|`step`|
||3:0|`index`|

![](./img/inst_lud.svg)

## Load Immediate (`LDI`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:0|0x60|
|2nd.|7:0|Fragment|

The state machine simply copies the second byte of the instruction code into the glyph buffer. If msb1st=1 is set, the fragment in the instruction code must also be MSB 1st.

![](./img/inst_ldi.svg)

## Repeat Last Fragment (`RPT`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:4|0b1110|
||3:0|`repeatCount` - 1|

![](./img/inst_rpt.svg)

## Shift Last Fragment (`SFT`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:6|0b00|
||5|`shiftDir` (0: Left, 1: Right)|
||4|`postOp` (0: Clear, 1: Set)|
||3:2|`shiftSize` - 1|
||1:0|`repeatCount` - 1|

When `shiftDir`=0, SFT shifts the fragment towards the LSB direction, other is the opposite.

![](./img/inst_sft.svg)

## Shift Last Fragment with Intervals (`SFI`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:0|0x68|
|2nd.|7:6|`interval` - 2|
||5|`shiftDir` (0: Left, 1: Right)|
||4|`postOp` (0: Clear, 1: Set)|
||3|`shift1st`|
||2:0|`repeatCount` - 1|

![](./img/inst_sfi.svg)

## XOR Last Fragment with Mask (`XOR`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:4|0b1111|
||3|`maskWidth - 1`|
||2:0|`maskPos`|

Combination of `maskWidth=2` and `maskPos=7` (0xFF) is reserved for other instruction or future use.

![](./img/inst_xor.svg)

## Copy Recent (`CPY`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:6|0b01|
||5|`byteReverse`|
||4:3|`offset`|
||2:0|`length` - 1|

- for `byteReverse` = 0:<br>Combination of `offset=0` and `length=1` (0x40) is reserved for other instruction or future use.
- for `byteReverse` = 1:<br>`length=1` (0x60, 0x68, 0x70, 0x78) is reserved for other instruction or future use.

![](./img/inst_cpy.svg)

## Long Distance Large Copy (`CPX`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:0|0x40|
|2nd.|7:0|`offset[7:0]`|
|3rd.|7|`bitReverse`|
||6|`byteReverse`|
||5:2|(`length` / 4) - 4|
||1|`inverse`|
||0|`offset[8]`|

![](./img/inst_cpx.svg)

## Abort (`ABO`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:0|0xFF|

The decompressor must abort decompression when it encounters an `ABO` instruction.

If safety is a priority, the blob generator can add a `ABO` instruction to the end of the glyph bytecode. This instruction is normally never executed, since decompression finishes as soon as the glyph buffer is filled. However, if the blob or decompressor logic is corrupted, this instruction can stop a runaway decompression process.

If a Shrinked Glyph Table is applied and the length of the glyph bytecode sequence does not reach a 2-byte boundary, it is recommended that the remaining part be filled with this instruction.

It is recommended to place three `ABO` instructions at the end of Bytecode Block, i.e. at the end of the entire blob.

# Rendering

## Buffer Model

![](./img/buffer_model.svg)
