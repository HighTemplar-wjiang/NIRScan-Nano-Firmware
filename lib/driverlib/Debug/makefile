################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.9

GEN_OPTS__FLAG := 
GEN_CMDS__FLAG := 

ORDERED_OBJS += \
"./adc.obj" \
"./aes.obj" \
"./can.obj" \
"./comp.obj" \
"./cpu.obj" \
"./crc.obj" \
"./des.obj" \
"./eeprom.obj" \
"./emac.obj" \
"./epi.obj" \
"./epi_workaround_ccs.obj" \
"./flash.obj" \
"./fpu.obj" \
"./gpio.obj" \
"./hibernate.obj" \
"./i2c.obj" \
"./interrupt.obj" \
"./lcd.obj" \
"./mpu.obj" \
"./pwm.obj" \
"./qei.obj" \
"./shamd5.obj" \
"./ssi.obj" \
"./sw_crc.obj" \
"./sysctl.obj" \
"./sysexc.obj" \
"./systick.obj" \
"./timer.obj" \
"./uart.obj" \
"./udma.obj" \
"./usb.obj" \
"./watchdog.obj" \
$(GEN_CMDS__FLAG) \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
LIB_OUTPUTS += \
driverlib.lib \

LIB_OUTPUTS__QUOTED += \
"driverlib.lib" \


# All Target
all: driverlib.lib

# Tool invocations
driverlib.lib: $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: ARM Archiver'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.9/bin/armar" r "driverlib.lib" $(ORDERED_OBJS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(LIB_OUTPUTS__QUOTED)
	-$(RM) "adc.d" "aes.d" "can.d" "comp.d" "cpu.d" "crc.d" "des.d" "eeprom.d" "emac.d" "epi.d" "flash.d" "fpu.d" "gpio.d" "hibernate.d" "i2c.d" "interrupt.d" "lcd.d" "mpu.d" "pwm.d" "qei.d" "shamd5.d" "ssi.d" "sw_crc.d" "sysctl.d" "sysexc.d" "systick.d" "timer.d" "uart.d" "udma.d" "usb.d" "watchdog.d" 
	-$(RM) "epi_workaround_ccs.d" 
	-$(RM) "adc.obj" "aes.obj" "can.obj" "comp.obj" "cpu.obj" "crc.obj" "des.obj" "eeprom.obj" "emac.obj" "epi.obj" "epi_workaround_ccs.obj" "flash.obj" "fpu.obj" "gpio.obj" "hibernate.obj" "i2c.obj" "interrupt.obj" "lcd.obj" "mpu.obj" "pwm.obj" "qei.obj" "shamd5.obj" "ssi.obj" "sw_crc.obj" "sysctl.obj" "sysexc.obj" "systick.obj" "timer.obj" "uart.obj" "udma.obj" "usb.obj" "watchdog.obj" 
	-@echo 'Finished clean'
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

