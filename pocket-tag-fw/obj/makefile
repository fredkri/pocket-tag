################################################################################
# MRS Version: 2.3.0
# Automatically-generated file. Do not edit!
################################################################################
-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include User/subdir.mk
-include Startup/subdir.mk
-include Peripheral/src/subdir.mk
-include Debug/subdir.mk
-include Core/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
pocket_tag.hex \

SECONDARY_LIST += \
pocket_tag.lst \

SECONDARY_SIZE += \
pocket_tag.siz \


# All Target
all: 
	$(MAKE) --no-print-directory main-build 

main-build: pocket_tag.elf secondary-outputs

# Tool invocations
pocket_tag.elf: $(OBJS) $(USER_OBJS_ESCAPE)
	@	riscv-none-embed-gcc -march=rv32ecxw -mabi=ilp32e -msmall-data-limit=0 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -T "/home/fredrika-kringberg/projects/pocket-tag/pocket-tag-fw/Ld/Link.ld" -nostartfiles -Xlinker --gc-sections -Wl,-Map,"pocket_tag.map" --specs=nano.specs --specs=nosys.specs -o "pocket_tag.elf" $(OBJS) $(LIBS) -lprintf
pocket_tag.hex: pocket_tag.elf
	@	riscv-none-embed-objcopy -O ihex "pocket_tag.elf" "pocket_tag.hex"
pocket_tag.lst: pocket_tag.elf
	@	riscv-none-embed-objdump --all-headers --demangle --disassemble -M xw "pocket_tag.elf" > "pocket_tag.lst"
pocket_tag.siz: pocket_tag.elf
	riscv-none-embed-size --format=berkeley "pocket_tag.elf"

# Other Targets
clean:
	-$(RM) $(DIR_OBJS) $(SECONDARY_FLASH)$(SECONDARY_LIST)$(SECONDARY_SIZE) pocket_tag.elf
	-$(RM) $(DIR_EXPANDS) $(CALLGRAPH_DOT)
	-$(RM) $(DIR_DEPS)
secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_LIST) $(SECONDARY_SIZE)

.PHONY: all clean dependents

-include ../makefile.targets