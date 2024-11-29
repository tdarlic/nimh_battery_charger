PREFIX?=riscv64-unknown-elf
OUTDIR ?= build
NEWLIB?=/usr/include/newlib

TARGET_MCU?=CH32V003
TARGET_EXT?=c

CH32V003FUN?=$(dir $(lastword $(MAKEFILE_LIST)))
MINICHLINK?=$(CH32V003FUN)/../minichlink

WRITE_SECTION?=flash
SYSTEM_C?=$(CH32V003FUN)/ch32v003fun.c

ifeq ($(DEBUG),1)
	EXTRA_CFLAGS+=-DFUNCONF_DEBUG=1
endif

CFLAGS?=-g -Os -flto -ffunction-sections -fdata-sections -fmessage-length=0 -msmall-data-limit=8
LDFLAGS+=-Wl,--print-memory-usage

ifeq ($(TARGET_MCU),CH32V003)
	CFLAGS_ARCH+=-march=rv32ec -mabi=ilp32e -DCH32V003=1
	GENERATED_LD_FILE?=$(CH32V003FUN)/generated_ch32v003.ld
	TARGET_MCU_LD:=0
	LINKER_SCRIPT?=$(GENERATED_LD_FILE)
	LDFLAGS+=-L$(CH32V003FUN)/../misc -lgcc
else
	MCU_PACKAGE?=1

	ifeq ($(findstring CH32V10,$(TARGET_MCU)),CH32V10) # CH32V103
		TARGET_MCU_PACKAGE?=CH32V103R8T6
		CFLAGS_ARCH+=	-march=rv32imac \
			-mabi=ilp32 \
			-DCH32V10x=1

		# MCU Flash/RAM split
		ifeq ($(findstring R8, $(TARGET_MCU_PACKAGE)), R8)
			MCU_PACKAGE:=1
		else ifeq ($(findstring C8, $(TARGET_MCU_PACKAGE)), C8)
			MCU_PACKAGE:=1
		else ifeq ($(findstring C6, $(TARGET_MCU_PACKAGE)), C6)
			MCU_PACKAGE:=2
		endif

		TARGET_MCU_LD:=1
	else
		$(error Unknown MCU $(TARGET_MCU))
	endif

	LDFLAGS+=-lgcc
	GENERATED_LD_FILE:=$(CH32V003FUN)/generated_$(TARGET_MCU_PACKAGE)_$(TARGET_MCU_MEMORY_SPLIT).ld
	LINKER_SCRIPT:=$(GENERATED_LD_FILE)
endif

CFLAGS+= \
	$(CFLAGS_ARCH) -static-libgcc \
	-I$(NEWLIB) \
	-I$(CH32V003FUN)/../extralibs \
	-I$(CH32V003FUN) \
	-nostdlib \
	-I. -Wall $(EXTRA_CFLAGS)

LDFLAGS+=-T $(LINKER_SCRIPT) -Wl,--gc-sections
FILES_TO_COMPILE:=$(SYSTEM_C) $(TARGET).$(TARGET_EXT) $(ADDITIONAL_C_FILES) 

DEPS := $(FILES_TO_COMPILE:%.c=$(OUTDIR)/%.d)
-include $(DEPS)

$(OUTDIR)/%.d: %.c
	mkdir -p $(OUTDIR)
	$(PREFIX)-gcc -M $(CFLAGS) $< > $@

$(OUTDIR)/$(TARGET).bin : $(OUTDIR)/$(TARGET).elf
	mkdir -p $(OUTDIR)
	$(PREFIX)-objdump -S $^ > $(OUTDIR)/$(TARGET).lst
	$(PREFIX)-objdump -t $^ > $(OUTDIR)/$(TARGET).map
	$(PREFIX)-objcopy -O binary $< $(OUTDIR)/$(TARGET).bin
	$(PREFIX)-objcopy -O ihex $< $(OUTDIR)/$(TARGET).hex

ifeq ($(OS),Windows_NT)
closechlink :
	-taskkill /F /IM minichlink.exe /T
else
closechlink :
	-killall minichlink
endif

terminal : monitor

monitor :
	$(MINICHLINK)/minichlink -T

unbrick :
	$(MINICHLINK)/minichlink -u

gdbserver : 
	-$(MINICHLINK)/minichlink -baG

clangd :
	make clean
	bear -- make build
	@echo "CompileFlags:" > .clangd
	@echo "  Remove: [-march=*, -mabi=*]" >> .clangd

clangd_clean :
	rm -f compile_commands.json .clangd
	rm -rf .cache

FLASH_COMMAND?=$(MINICHLINK)/minichlink -w $< $(WRITE_SECTION) -b

.PHONY : $(GENERATED_LD_FILE)
$(GENERATED_LD_FILE) :
	$(PREFIX)-gcc -E -P -x c -DTARGET_MCU=$(TARGET_MCU) -DMCU_PACKAGE=$(MCU_PACKAGE) -DTARGET_MCU_LD=$(TARGET_MCU_LD) -DTARGET_MCU_MEMORY_SPLIT=$(TARGET_MCU_MEMORY_SPLIT) $(CH32V003FUN)/ch32v003fun.ld > $(GENERATED_LD_FILE)

$(OUTDIR)/$(TARGET).elf : $(FILES_TO_COMPILE) $(LINKER_SCRIPT) $(EXTRA_ELF_DEPENDENCIES)
	$(PREFIX)-gcc -o $@ $(FILES_TO_COMPILE) $(CFLAGS) $(LDFLAGS)

# Rule for independently building ch32v003fun.o indirectly, instead of recompiling it from source every time.
# Not used in the default 003fun toolchain, but used in more sophisticated toolchains.
ch32v003fun.o : $(SYSTEM_C)
	$(PREFIX)-gcc -c -o $@ $(SYSTEM_C) $(CFLAGS)

cv_flash : $(OUTDIR)/$(TARGET).bin
	make -C $(MINICHLINK) all
	$(FLASH_COMMAND)

cv_clean :
	rm -rf $(OUTDIR)/$(TARGET).elf $(OUTDIR)/$(TARGET).bin $(OUTDIR)/$(TARGET).hex $(OUTDIR)/$(TARGET).lst $(OUTDIR)/$(TARGET).map $(OUTDIR)/$(TARGET).hex $(OUTDIR)/$(GENERATED_LD_FILE) || true

build : $(OUTDIR)/$(TARGET).bin
