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
|0x40-5F|||`LUD`|Double Lookup|
|0x60-6F|||`RPT`|Repeat Last Fragment|
|0x70-7E|||`XOR`|XOR Last Fragment with Mask|
|0x7F|||n/a|Reserved|
|0x80-BF|||`SFT`|Shift Last Fragment|
|0xC0|any||`LDI`|Load Immediate|
|0xC1-DF,<br>0xE1-E7,<br>0xE9-EF,<br>0xF1-F7,<br>0xF9-FF|||`CPY`|Copy Recent|
|0xE0|any|any|`CPX`|Large Copy|
|0xE8|||n/a|Reserved|
|0xF0|||n/a|Reserved|
|0xF8|||n/a|Reserved|

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

### Double Lookup (`LUD`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:5|0b010|
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
|1st.|7:0|0xC0|
|2nd.|7:0|Fragment|

The state machine simply copies the second byte of the instruction code into the glyph buffer. If msb1st=1 is set, the fragment in the instruction code must also be MSB 1st.

![](./img/inst_ldi.svg)

#### Pseudo Code

```c
buff[cursor++] = bytecode[programCounter++];
```

### Repeat Last Fragment (`RPT`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:4|0b0110|
||3:0|`repeatCount` - 1|

![](./img/inst_rpt.svg)

#### Pseudo Code

```c
memset(buff + cursor, buff[cursor - 1], repeatCount);
cursor += repeatCount;
```
### Shift Last Fragment (`SFT`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:6|0b10|
||5|`shiftDir` (0: Left, 1: Right)|
||4|`postOp` (0: Clear, 1: Set)|
||3:2|`shiftSize` - 1|
||1:0|`repeatCount` - 1|

When `shiftDir`=0, SFT shifts the fragment towards the LSB direction, other is the opposite.

![](./img/inst_sxx.svg)

#### Visual direction of pixel movement

|`verticalFragment`|`msb1st`|`shiftDir`=0|`shiftDir`=1|
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

### XOR Last Fragment with Mask (`XOR`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:4|0b0111|
||3|`maskWidth - 1`|
||2:0|`maskPos`|

Combination of `maskWidth=2` and `maskPos=7` (0xFF) is reserved for other instruction or future use.

![](./img/inst_xor.svg)

#### Pseudo Code

```c
int mask = (1 << maskWidth) - 1;
buff[cursor++] = buff[cursor - 1] ^ (mask << maskPos);
```

### Copy Recent (`CPY`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:6|0b11|
||5|`byteReverse`|
||4:3|`offset`|
||2:0|`length` - 1|

- for `byteReverse` = 0:<br>Combination of `offset=0` and `length=1` (0xC0) is reserved for other instruction or future use.
- for `byteReverse` = 1:<br>`length=1` (0xE0, 0xE8, 0xF0, 0xF8) is reserved for other instruction or future use.

![](./img/inst_cpy.svg)

#### Pseudo Code

```c
if (byteReverse) {
    for (int i = 0; i < length; i++) {
        buff[cursor + i] = buff[cursor - offset - i];
    }
}
else {
    memcpy(buff + cursor, buff + (cursor - length - offset), length);
}
cursor += length;
```

### Large Copy (`CPX`)

|Byte|Bit Range|Value|
|:--:|:--:|:--|
|1st.|7:0|0xE0|
|2nd.|7:0|`absOffset[7:0]`|
|3rd.|7|`bitReverse`|
||6|`byteReverse`|
||5:2|(`length` / 4) - 4|
||1|`inverse`|
||0|`absOffset[8]`|

(Specifications under consideration)

# Rendering

## Buffer Model

![](./img/buffer_model.svg)
