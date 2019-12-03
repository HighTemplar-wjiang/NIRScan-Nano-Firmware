#
_XDCBUILDCOUNT = 0
ifneq (,$(findstring path,$(_USEXDCENV_)))
override XDCPATH = C:/ti/tirtos_tivac_2_10_01_38/products/bios_6_41_00_26/packages;C:/ti/tirtos_tivac_2_10_01_38/products/ndk_2_24_01_18/packages;C:/ti/tirtos_tivac_2_10_01_38/packages;C:/ti/tirtos_tivac_2_10_01_38/products/uia_2_00_02_39/packages;C:/ti/tirtos_tivac_2_10_01_38/products/TivaWare_C_Series-2.1.0.12573c;C:/ti/ccsv6/ccs_base
override XDCROOT = C:/ti/xdctools_3_30_04_52_core
override XDCBUILDCFG = ./config.bld
endif
ifneq (,$(findstring args,$(_USEXDCENV_)))
override XDCARGS = 
override XDCTARGETS = 
endif
#
ifeq (0,1)
PKGPATH = C:/ti/tirtos_tivac_2_10_01_38/products/bios_6_41_00_26/packages;C:/ti/tirtos_tivac_2_10_01_38/products/ndk_2_24_01_18/packages;C:/ti/tirtos_tivac_2_10_01_38/packages;C:/ti/tirtos_tivac_2_10_01_38/products/uia_2_00_02_39/packages;C:/ti/tirtos_tivac_2_10_01_38/products/TivaWare_C_Series-2.1.0.12573c;C:/ti/ccsv6/ccs_base;C:/ti/xdctools_3_30_04_52_core/packages;..
HOSTOS = Windows
endif
