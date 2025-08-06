
.PHONY: flash-write fuse-read fuse-write

DIR_TOOLS_AVRDUDE = ../tools/avrdude_wsl

AVRDUDE_EXE := "D:\\Apps\\avr-gcc-14.1.0-x64-windows\\bin\\avrdude.exe"
CABLE_NAME := serialupdi
CABLE_PORT := COMxx

flash-write: $(HEX)
	powershell.exe \
		-executionpolicy Bypass \
		-File $(DIR_TOOLS_AVRDUDE)/flash-write.ps1 \
		"$(AVRDUDE_EXE)" $(MCU) $(CABLE_NAME) $(CABLE_PORT) $(HEX)

fuse-read:
	powershell.exe \
		-executionpolicy Bypass \
		-File $(DIR_TOOLS_AVRDUDE)/fuse-read.ps1 \
		"$(AVRDUDE_EXE)" $(MCU) $(CABLE_NAME) $(CABLE_PORT)

fuse-write:
	powershell.exe \
		-executionpolicy Bypass \
		-File $(DIR_TOOLS_AVRDUDE)/fuse-write.ps1 \
		"$(AVRDUDE_EXE)" $(MCU) $(CABLE_NAME) $(CABLE_PORT)
