################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CFG_SRCS += \
../appNano.cfg 


# Each subdirectory must supply rules for building sources it contributes
configPkg/linker.cmd: ../appNano.cfg
	@echo 'Building file: $<'
	@echo 'Invoking: XDCtools'
	"/xs" --xdcpath="C:/ti/tirtos_tivac_2_10_01_38/packages;C:/ti/tirtos_tivac_2_10_01_38/products/bios_6_41_00_26/packages;C:/ti/tirtos_tivac_2_10_01_38/products/ndk_2_24_01_18/packages;C:/ti/tirtos_tivac_2_10_01_38/products/uia_2_00_02_39/packages;" xdc.tools.configuro -o configPkg -t ti.targets.arm.elf.M4F -p ti.platforms.tiva:TM4C129XNCZAD -r debug -c "$<"
	@echo 'Finished building: $<'
	@echo ' '


