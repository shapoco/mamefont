# mamefont

## Decompression

|Byte|Mnemonic|Operation|
|:---|:---|:---|
|0x00-3f|`LD`|`buff[pos++] = table[i]`|
|0x40-4f|`SLC`|`repeat(len) buff[pos++] = (buff[pos-1] << i) & 0xfe`|
|0x40-4f|`SLS`|`repeat(len) buff[pos++] = (buff[pos-1] << i) \| 0x01`|
|0x50-5f|`SRC`|`repeat(len) buff[pos++] = (buff[pos-1] >> i) & 0x7f`|
|0x50-5f|`SRS`|`repeat(len) buff[pos++] = (buff[pos-1] >> i) \| 0x80`|
|0x80-bf|`CPY`|`memcpy(buff+pos, buff+(pos-len-offset), len);`<br>`pos += len`|
|0xc0-cf|`REV0`|`for(i=0; i<len; i++) buff[pos+i] = buff[pos-1-i];`<br>`pos += len`|
|0xd0-df|`REV1`|`for(i=0; i<len; i++) buff[pos+i] = buff[pos-2-i];`<br>`pos += len`|
|0xe0-ef|`RPT`|`memset(buff+pos, buff[pos-1], len);`<br>`pos += len`|
|0xf0-f7|`XOR1`|`buff[pos++] = buff[pos-1] ^ (1 << i)`|
|0xf8-fe|`XOR2`|`buff[pos++] = buff[pos-1] ^ (3 << i)`|
