Param($AVRDUDE_EXE, $MCU, $CABLE_NAME, $CABLE_PORT, $HEX)

&"$AVRDUDE_EXE" `
    -p $MCU `
    -c $CABLE_NAME `
    -x rtsdtr=low `
    -P $CABLE_PORT `
    -U flash:w:$HEX
