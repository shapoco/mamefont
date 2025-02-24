# mamefont

## Structure

|Size [Bytes]|Description|
|:--|:--|
|8|Font Header|
|`char_table_depth` * 2|Character Table|
|`seg_table_depth`|Segment Table|
|(Variable)|Character Data Blocks|

## Font Header

|Size [Bytes]|Description|
|:--|:--|
|4|`font_header_0`|

### `font_header_0`

|Bit Range|Value|Description|
|:--:|:--|:--|
|31:24|`font_height`|Font height in pixels|
|23:8|reserved||
|15:8|`num_chars`|Number of entry of Character Table|
|7:0|`code_offset`|Character code offset|

## Character Table Entry

|Size [Bytes]|Description|
|:--|:--|
|2|`char_table_entry`|

### `char_table_entry`

|Bit Range|Value|Description|
|:--:|:--|:--|
|31:24|`font_height`|Font height in pixels|
|23:8|reserved||
|15:8|`num_chars`|Number of entry of Character Table|
|7:0|`code_offset`|Character code offset|

## Character Data Block

|Size [Bytes]|Description|
|:--|:--|
|1|Character Header|
|(Variable)|Microcodes|



## Instruction Set

|Value Range|Mnemonic|Description|
|:---|:---|:---|
|0x00-3f|`LD`|Load from Table|
|0x40-4f|`SLC`|Shift Left and Clear LSB|
|0x50-5f|`SLS`|Shift Left and Set LSB|
|0x60-6f|`SRC`|Shift Right and Clear MSB|
|0x70-7f|`SRS`|Shift Right and Set MSB|
|0x80-bf|`CPY`|Copy Sequence|
|0xc0-df|`REV`|Reverse Sequence|
|0xe0-ef|`RPT`|Repeat Previous Segment|
|0xf0-f7|`XOR`|XOR Previous Segment|
|0xff|-|(Reserved)|

### Load from Table (`LD`)

|Bit Range|Value|
|:--:|:--|
|7:6|0b00|
|5:0|`index`|

```c
buff[pos++] = table[index];
```

### Shift and Clear/Set (`SLC`, `SLS`, `SRC`, `SRS`)

|Bit Range|Value|
|:--:|:--|
|7:6|0b01|
|5|`shift_dir` (0: Left, 1: Right)|
|4|`post_op` (0: Clear, 1: Set)|
|3|`shift_size - 1`|
|2:0|`len - 1`|

```c
for (int i = 0; i < len; i++) {
    if (shift_dir == 0) {
        buff[pos] = buff[pos - 1] << shift_size;
        if (post_op == 0) {
            buff[pos] &= 0xfe;
        }
        else {
            buff[pos] |= 0x01;
        }
    }
    else {
        buff[pos] = buff[pos - 1] >> shift_size;
        if (post_op == 0) {
            buff[pos] &= 0x7f;
        }
        else {
            buff[pos] |= 0x80;
        }
    }
    pos++;
}
```

### Copy Sequence (`CPY`)

|Bit Range|Value|
|:--:|:--|
|7:6|0b10|
|5:4|`offset`|
|3:0|`len - 1`|

```c
memcpy(buff + pos, buff + (pos - len - offset), len);
pos += len;
```

### Reverse Sequence (`REV`)

|Bit Range|Value|
|:--:|:--|
|7:5|0b110|
|4|`offset - 1`|
|3:0|`len - 1`|

```c
for (int i = 0; i < len; i++) {
    buff[pos + i] = buff[pos - offset - i];
}
pos += len;
```

### Repeat Previous Segment (`RPT`)

|Bit Range|Value|
|:--:|:--|
|7:4|0b1110|
|3:0|`len - 1`|

```c
memset(buff + pos, buff[pos - 1], len);
pos += len;
```

### XOR Previous Segment (`XOR`)

|Bit Range|Value|
|:--:|:--|
|7:4|0b1111|
|3|`width - 1`|
|2:0|`bit`|

```c
int mask = width == 1 ? 0x01 : 0x03;
buff[pos++] = buff[pos - 1] ^ (mask << bit);
```
