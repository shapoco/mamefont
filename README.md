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
|(4 or 2) \* `numGlyphs`|Character Table|
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
||5:0|`fontHeight` - 1|`fontHeight` is height of glyph in pixels|
|\[1\]|7:6|(Reserved)||
||5:0|`yAdvance` - 1|`yAdvance` is the distance in pixels from the top of the current line to the top of the next line|
|\[2\]|7:6|(Reserved)||
||5:0|`maxGlyphWidth` - 1|`maxGlyphWidth` is `glyphWidth` value of widest glyph in the font|

`yAdvance` may be smaller than `fontHeight`, but whether it is rendered as intended by the font designer depends on the implementation of the graphics engine.

## Glyph Table

### Normal Table Entry (4 Byte)

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
|\[1\]|7:6|(Reserved)||
||5:0|`xAdvance` - 1|`xAdvance` is the distance in pixels from the left edge of the current character to the left edge of the next character|

`xAdvance` may be smaller than `glyphWidth`, but whether it is rendered as intended by the font designer depends on the implementation of the graphics engine.

### Shrinked Table Entry (2 Byte)

The Shrinked Format of the Glyph Table can be applied when all glyphWidth and xAdvance in the font are 16 pixel or less, and the total size of the bytecode block is 512 Byte or less. In this case, all bytecode entry points must be aligned to 2-Byte boundaries.

|Size \[Bytes\]|Value|Description|
|:--:|:--|:--|
|1|`entryPoint` &gt;&gt; 1|The value of entryPoint divided by 2. The true value of entryPoint must be a multiple of 2.|
|1|`packedGriphDimension`|Dimension of the glyph|

#### `packedGriphDimension`

|Bit Range|Value|Description|
|:--:|:--|:--|
|7:4|`xAdvance` - 1|See description of Normal Table Entry.|
|3:0|`glyphWidth` - 1|See description of Normal Table Entry.|

### Missing Glyph

To express that no valid glyph is assigned to a character code, the first two bytes of the Glyph Table Entry should be 0xFFFF.

This also means:

- If not a Shrinked Glyph Table: entryPoint=0xFFFF cannot be used.
- If a Shrinked Glyph Table: the combination of entryPoint=0x1FE, glyphWidth=16, xAdvance=16 cannot be used.

Be careful as these can lead to corner case issues.

## Lookup Table

If the total size does not reach a 2-Byte boundary, a dummy byte must be appended.
The value of `lutSize` includes this dummy byte.

## Bytecode Block

|Size \[Bytes\]|Description|
|:--:|:--|
|(Variable)|Array of instructions|

## Instruction Set

|1st. Byte|2nd. Byte|3rd. Byte|Mnemonic|Description|
|:--:|:--:|:--:|:--:|:--|
|0x00-3F|||`LUP`|Single Lookup|
|0x40-4F|||`SLC`|Shift Left Previous Fragment and Clear Lower Bits|
|0x50-5F|||`SLS`|Shift Left Previous Fragment and Set Lower Bits|
|0x60-6F|||`SRC`|Shift Right Previous Fragment and Clear Upper Bits|
|0x70-7F|||`SRS`|Shift Right Previous Fragment and Set Upper Bits|
|0x80-9F|||`LUD`|Double Lookup|
|0xA0|any||`LDI`|Load Immediate|
|0xA1-BF|||`CPY`|Copy Previous Sequence|
|0xC0|0x00-3F|any|`LUL`|Large Lookup|
|0xC0|0x40-7F|any|`CPL`|Large Copy|
|0xC0|0x80-FF|any|`CPX`|Long Distance Large Copy|
|0xC1-DF<br>(\*)|||`REV`|Reverse Previous Sequence<br>(\*) 0xC8, 0xD0, 0xD8 is prohibited|
|0xE0-EF|||`RPT`|Repeat Previous Fragment|
|0xF0-FE|||`XOR`|XOR Previous Fragment with Mask|
|0xFF|||n/a|Reserved|

### Single Lookup (`LUP`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:6|0b00|
||5:0|`index`|

The state machine simply copies the fragment in the LUT to the glyph buffer. If msb1st=1 is set, the fragments in the LUT must also be MSB 1st.

![](./img/inst_lus.svg)

#### Pseudo Code

```c
buff[cursor++] = lut[index];
```

### Sequential Double Lookup (`LUD`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:5|0b100|
||4|`step`|
||3:0|`index`|

![](./img/inst_lud.svg)

#### Pseudo Code

```c
buff[cursor++] = lut[index];
buff[cursor++] = lut[index + step];
```

### Load Immediate (`LDI`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:0|0xA0|
|2nd.|7:0|Fragment|

The state machine simply copies the second byte of the instruction code into the glyph buffer. If msb1st=1 is set, the fragment in the instruction code must also be MSB 1st.

![](./img/inst_ldi.svg)

#### Pseudo Code

```c
buff[cursor++] = bytecode[programCounter++];
```

### Repeat Previous Fragment (`RPT`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:4|0b1110|
||3:0|`repeatCount` - 1|

![](./img/inst_rpt.svg)

#### Pseudo Code

```c
memset(buff + cursor, buff[cursor - 1], repeatCount);
cursor += repeatCount;
```
### Shift Previous Fragment and Clear/Set Lower/Upper Bits (`SLC`, `SLS`, `SRC`, `SRS`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:6|0b01|
||5|`shiftDir` (0: Left, 1: Right)|
||4|`postOp` (0: Clear, 1: Set)|
||3:2|`shiftSize` - 1|
||1:0|`repeatCount` - 1|

`SLx` shifts the fragment towards the LSB direction, `SRx` does the opposite.

![](./img/inst_sxx.svg)

#### Visual direction of pixel movement

|`verticalFragment`|`msb1st`|`SLC`, `SLS`|`SRC`, `SRS`|
|:--:|:--:|:--:|:--:|
|0 (Horizontal)|0 (LSB first)|Right|Left|
|0 (Horizontal)|1 (MSB first)|Left|Right|
|1 (Vertical)|0 (LSB first)|Down|Up|
|1 (Vertical)|1 (MSB first)|Up|Down|

#### Pseudo Code

```c
uint8_t modifier = (1 << shiftSize) - 1;
if (shiftDir != 0) modifier <<= (8 - shiftSize);
if (postOp == 0) modifier = ~modifier;
for (int i = 0; i < repeatCount; i++) {
    if (shiftDir == 0) {
        buff[cursor] = buff[cursor - 1] << shiftSize;
    }
    else {
        buff[cursor] = buff[cursor - 1] >> shiftSize;
    }
    if (postOp == 0) {
        buff[cursor] &= modifier;
    }
    else {
        buff[cursor] |= modifier;
    }
    cursor++;
}
```

### XOR Previous Fragment with Mask (`XOR`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:4|0b1111|
||3|`mask_width - 1`|
||2:0|`mask_pos`|

Combination of `mask_width=2` and `mask_pos=7` (0xFF) is reserved for other instruction or future use.

![](./img/inst_xor.svg)

#### Pseudo Code

```c
int mask = (1 << mask_width) - 1;
buff[cursor++] = buff[cursor - 1] ^ (mask << mask_pos);
```

### Copy Previous Sequence (`CPY`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:5|0b101|
||4:3|`offset`|
||2:0|`length` - 1|

Combination of `offset=0` and `length=1` (0xA0) is reserved for other instruction or future use.

![](./img/inst_cpy.svg)

#### Pseudo Code

```c
memcpy(buff + cursor, buff + (cursor - length - offset), length);
cursor += length;
```

### Reverse Previous Sequence (`REV`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:5|0b110|
||4:3|`offset`|
||2:0|`length` - 1|

`length=1` (0xC0, 0xC8, 0xD0, 0xD8) is reserved for other instruction or future use.

![](./img/inst_rev.svg)

#### Pseudo Code

```c
for (int i = 0; i < length; i++) {
    buff[cursor + i] = buff[cursor - offset - i];
}
cursor += length;
```

### Large Lookup (`LUL`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:0|0b11000000|
|2nd.|7:6|0b00|
||5:0|`index`|
|3rd.|7|`byteReversal`|
||6|`bitReversal`|
||5:0|`length - 1`|

(Specifications under consideration)

### Large Copy (`CPL`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:0|0b11000000|
|2nd.|7:6|0b01|
||5:0|`offset`|
|3rd.|7|`byteReversal`|
||6|`bitReversal`|
||5:0|`length - 1`|

(Specifications under consideration)

### Long Distance Large Copy (`CPX`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:0|0b11000000|
|2nd.|7|0b1|
||6:4|`laneOffset`|
||3:0|`byteOffset + 8`|
|3rd.|7|`byteReversal`|
||6|`bitReversal`|
||5:0|`length - 1`|

(Specifications under consideration)

# Rendering

## Buffer Model

![](./img/buffer_model.svg)
