Param($AVRDUDE_EXE, $MCU, $CABLE_NAME, $CABLE_PORT)

&"$AVRDUDE_EXE" `
    -p $MCU `
    -c $CABLE_NAME `
    -x rtsdtr=low `
    -P $CABLE_PORT `
    -U osccfg:r:-:h `
    -U syscfg0:r:-:h
