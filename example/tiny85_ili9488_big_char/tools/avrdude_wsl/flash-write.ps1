Param($MCU, $CABLE_NAME, $ELF)

&"C:\Program Files (x86)\Atmel\Studio\7.0\atbackend\atprogram.exe" `
    -t $CABLE_NAME `
    -i isp `
    -d $MCU `
    program `
    -c `
    --flash `
    -f $ELF `
    --format elf `
    --verify
