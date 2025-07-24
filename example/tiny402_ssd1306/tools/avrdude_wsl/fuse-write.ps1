Param($AVRDUDE_EXE, $MCU, $CABLE_NAME, $CABLE_PORT)

&"$AVRDUDE_EXE" `
    -p $MCU `
    -c $CABLE_NAME `
    -x rtsdtr=low `
    -P $CABLE_PORT `
    -U osccfg:w:0x02:m `
    -U syscfg0:w:0xf3:m
