Param($MCU, $CABLE_NAME)

&"C:\Program Files (x86)\Atmel\Studio\7.0\atbackend\atprogram.exe" `
    -t $CABLE_NAME `
    -i isp `
    -d $MCU `
    --clock 1MHz `
    read `
    --fuses `
    read `
    --lockbits
