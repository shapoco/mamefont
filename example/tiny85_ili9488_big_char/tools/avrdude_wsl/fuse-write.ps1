Param($MCU, $CABLE_NAME)

&"C:\Program Files (x86)\Atmel\Studio\7.0\atbackend\atprogram.exe" `
    -t $CABLE_NAME `
    -i isp `
    --clock 1MHz `
    -d $MCU `
    write `
    --fuses `
    --values E1DFFF `
    --verify

# Fuse Extended Byte
#     [7:1]   n/a                                                             0b1111111
#     [0]     SELFPRGEN       Self-programming enabled    (0=Enabled)         0b1
# 
# Fuse High Byte
#     [7]     RSTDISBL        External Reset Disable      (1=Reset Enabled)   0b1
#     [6]     DWEN            DebugWIRE Enable            (0=Enabled)         0b1
#     [5]     SPIEN           Serial program enable       (0=Enabled)         0b0
#     [4]     WDTON           WDT always on               (0=on)              0b1
#     [3]     EESAVE          EEPROM preserve at erase    (0=preserved)       0b1
#     [2:0]   BODLEVEL        Brown-out level                                 0b111
# 
# Fuse Low Byte
#     [7]     CKDIV8          Clock divided by 8          (0=divided)         0b1
#     [6]     CKOUT           Clock output enabled        (0=enabled)         0b1
#     [5:4]   SUT             Start-up time                                   0b10
#     [3:0]   CKSEL           Clock source                                    0b0001
# 
# CKSEL[3:0]
#     0b0000      External
#     0b0001      High Frequency PLL Clock (16MHz)
#     0b0010      8.0MHz
#     0b0011      6.4MHz
#     0b0100      128kHz
#     0b0101      Reserved
#     0b0110      Low Frequency Crystal Oscillator (32.768kHz)
#     0b0111      Reserved
#     0b1000-1111 Crystal Oscillator
# 
# SUT[1:0]
#     0b00    14CK            (Recommended for BOD enabled)
#     0b01    14CK + 4ms      (Recommended for Fast rising power)
#     0b10    14CK + 64ms     (Recommended for Slowly rising power)
#     0b11    reserved
