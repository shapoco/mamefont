
.PHONY: flash-write fuse-read fuse-write

DIR_TOOLS_AVRDUDE = ../tools/avrdude_wsl

CABLE_NAME := avrispmk2

flash-write: $(ELF)
	powershell.exe \
		-executionpolicy Bypass \
		-File $(DIR_TOOLS_AVRDUDE)/flash-write.ps1 \
		$(MCU) $(CABLE_NAME) $(ELF)

fuse-read:
	powershell.exe \
		-executionpolicy Bypass \
		-File $(DIR_TOOLS_AVRDUDE)/fuse-read.ps1 \
		$(MCU) $(CABLE_NAME)

fuse-write:
	powershell.exe \
		-executionpolicy Bypass \
		-File $(DIR_TOOLS_AVRDUDE)/fuse-write.ps1 \
		$(MCU) $(CABLE_NAME)
