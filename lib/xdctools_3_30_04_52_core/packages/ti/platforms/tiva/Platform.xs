/*
 *  Copyright (c) 2014 by Texas Instruments and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 *
 *  Contributors:
 *      Texas Instruments - initial implementation
 *
 * */

/*
 *  ======== Platform.xs ========
 *  Generic platform support for Tiva devices.
 */

var Boot = null;
var Utils = xdc.useModule('xdc.platform.Utils');

/*
 *  ======== Platform.getCpuDataSheet ========
 *  This Platform's implementation xdc.platform.IPlatform.getCpuDataSheet.
 *
 *  Return the xdc.platform.ICpuDataSheet.Instance object that corresponds
 *  to the device that runs executables loaded on the specified cpuId.
 */
function getCpuDataSheet(cpuId)
{
    var cpuAttrs = this.CPU.$copy();

    cpuAttrs.deviceName = this.deviceName;

    return (Utils.getCpuDataSheet(cpuAttrs));
}

/*
 *  ======== Platform.getExeContext ========
 *  This Platform's implementation xdc.platform.IPlatform.getExeContext.
 *
 *  Return the xdc.platform.IExeContext.Instance object that will run the
 *  specified program prog.
 */
function getExeContext(prog)
{
    /* create a default ExeContext execution context */
    var ExeContext = xdc.useModule('xdc.platform.ExeContext');

    var cpu = ExeContext.create(this.CPU, this.$module.BOARD);
    var revision = this.CPU.revision;

    /* This is where we have peripherals available, so we can get their base
     * addresses and other info.
     */

    cpu.memoryMap = Utils.assembleMemoryMap(cpu, this);
    cpu.deviceName = revision;
    cpu.revision = "";

    if (prog.build.target.$name.match(/gnu/)) {
        if (this.codeMemory == undefined) {
            this.codeMemory = "REGION_TEXT";
        }
        if (this.dataMemory == undefined) {
            this.dataMemory = "REGION_DATA";
        }
        if (this.stackMemory == undefined) {
            this.stackMemory = "REGION_STACK";
        }
    }
    /* check for the overlap in the memory map */
    var overlap = Utils.checkOverlap(cpu.memoryMap);

    if (overlap != null) {
        this.$module.$logError("Memory objects " + overlap + " overlap", this,
            overlap);
    }

    this.$seal();
    return (cpu);
}

/*
 *  ======== Platform.getExecCmd ========
 *  This Platform's implementation xdc.platform.IPlatform.getExecCmd.
 */
function getExecCmd(prog, packagePath)
{
    var os = environment["xdc.hostOS"];
    var doNotKnow = "@$(ECHO) ti.platforms.tiva platform package cannot "
        + "execute " + prog.name + " on " + os + "\n";
    return (doNotKnow);
}

/*
 *  ======== Platform.getLinkTemplate ========
 *  This Platform's implementation xdc.platform.IPlatform.getLinkTemplate
 */
function getLinkTemplate(prog)
{
    if (prog.build.target.$name.match(/gnu/)) {
        return ("ti/platforms/tiva/linkcmd_gnu.xdt");
    }
    else if (prog.build.target.$name.match(/iar/)) {
        return ("ti/platforms/tiva/linkcmd_iar.xdt");
    }
    else {
        return ("ti/platforms/tiva/linkcmd.xdt");
    }
}

/*
 *  ======== Platform.instance$meta$init ========
 *  This function is called to initialize a newly created instance of a
 *  platform.  Platform instances are created just prior to running
 *  program configuration scripts.
 *
 *  Platform instances may also be created in the build domain.
 *
 *  @param(name)        the name used to identify this instance (without
 *                      the package name prefix)
 *
 *  For this platform, 'name' must identify a device. Besides the catalog
 *  module, this parameter may encode values for other configuration
 *  parameters, as defined by `nameFormat`. The values are separated by ':'.
 */
function instance$meta$init(name)
{
    var maxParamsLength = this.$module.nameFormat.split(":").length;
    var nameParams = name.split(":");

    if (nameParams.length > maxParamsLength) {
        this.$module.$logWarning("The platform ti.platforms.tiva accepts "
            + "only " + maxParamsLength + " instance parameters in its name. "
            + "The supplied string '" + name + "' contains additional values, "
            + "which will be ignored.", this, this.$module.nameFormat);
    }

    if (nameParams[0] != "" && this.deviceName != null
        && this.deviceName != nameParams[0]) {
        this.$module.$logError(this.$package.$name
            + " was passed two different device names: '" + nameParams[0]
            + "' and '" + this.deviceName + "'.", this, this.deviceName);
    }
    else if (nameParams[0] != "") {
        this.deviceName = nameParams[0];
    }

    if (this.deviceName == null) {
        this.$module.$logFatal("A device name must be specified for the "
            + "platform '" + this.$package.$name + "'. A device name can be "
            + "specified in the platform name, as "
            + "'ti.platforms.tiva:TM4XYZW', where TM4XYZW is the name "
            + "of the selected device.", this, this.deviceName);
    }

    if (nameParams[1] != "false" && Boolean(nameParams[1]).valueOf()) {
        this.includeLinkCmdFile = true;
    }

    if (nameParams[2] != undefined && !isNaN(parseFloat(nameParams[2]))) {
        this.clockRate = parseFloat(nameParams[2]);
    }

    if (this.externalMemoryMap.length != 0) {
        this.$module.$logWarning(this.$package.$name + " does not support "
            + "external memory map, and the parameter 'externalMemoryMap' is "
            + "ignored.", this, this.externalMemoryMap);
    }

    if (Program.build.target.$name.match(/gnu/)) {
        this.codeMemory = "REGION_TEXT";
        this.dataMemory = "REGION_DATA";
        this.stackMemory = "REGION_STACK";
    }

    /* if in the configuration model ... */
    if (xdc.om.$name == "cfg") {
        /* We use 'revision' to pass 'the real device name' to the generic
         * Tiva device.
         */
        this.CPU.revision = this.deviceName;

        /* figure out the clock configuration */
        if (this.deviceName.match(/^CC2538/)) {
            this.CPU.catalogName = "ti.catalog.arm.cortexm3";
            this.CPU.deviceName = "Stellaris";
            this.CPU.clockRate = 16;
        }
        else if (this.deviceName.match(/^CC26/)) {
            this.CPU.catalogName = "ti.catalog.arm.cortexm3";
            this.CPU.deviceName = "Stellaris";
            this.CPU.clockRate = 48;
        }
        else if (this.deviceName.match(/^TM4L/)) {
            Boot = xdc.useModule('ti.catalog.arm.cortexm4.tiva.l.Boot');
            Boot.configureClocks = true;
            this.CPU.catalogName = "ti.catalog.arm.cortexm4";
            this.CPU.clockRate = 10;
        }
        else if (this.deviceName.match(/^LM3/)) {
            Boot = xdc.useModule('ti.catalog.arm.cortexm3.lm3init.Boot');

            Boot.configureClock = true;
            Boot.oscSrc = Boot.OSCSRC_MAIN;     /* use main oscillator */
            Boot.pllBypass = false;             /* use PLL to drive sys clock */
            Boot.pllOutEnable = false;          /* no PLL output */
            Boot.ioscDisable = false;           /* IOSC is enabled */
            Boot.moscDisable = false;           /* MOSC is enabled */

            Boot.configureLdo = false;

            var clockRate;

            if (this.deviceName == "LM3S301"
                || this.deviceName.match(/^LM3S10\d$/)) {
                Boot.sysClockDiv = Boot.SYSDIV_10; /* 20 MHz */
                Boot.pwmClockDiv = Boot.PWMDIV_1;
                Boot.xtal = Boot.XTAL_8MHZ ;       /* XTAL = 8 MHz */
                clockRate = 20;
            }
            else if (this.deviceName.match(/^LM3S3\d\d$/)
                && this.deviceName != "LM3S301") {
                Boot.sysClockDiv = Boot.SYSDIV_8; /* 25 MHz */
                Boot.pwmClockDiv = Boot.PWMDIV_1;
                Boot.xtal = Boot.XTAL_8MHZ ;       /* XTAL = 8 MHz */
                clockRate = 25;
            }
            else if (this.deviceName.match(/^LM3S\d\d(1|2)(0|2)$/)
                || this.deviceName == "LM3S6100"
                || this.deviceName == "LM3S2139") {
                Boot.sysClockDiv = Boot.SYSDIV_8; /* 25 MHz */
                Boot.pwmClockDiv = Boot.PWMDIV_1;
                Boot.xtal = Boot.XTAL_8MHZ ;       /* XTAL = 8 MHz */
                clockRate = 25;
            }
            else if (this.deviceName.match(/^LM3S9\w\d\d/)
                || this.deviceName.match(/^LM3S5\D\d\d/)
                || this.deviceName.match(/^LM3S5\d\d1/)
                || this.deviceName.match(/^LM3S5\d\d6/)
                || this.deviceName.match(/^LM3S2\w93/)) {
                Boot.sysClockDiv = Boot.SYSDIV_2_5; /* 80 MHz */
                Boot.pwmClockDiv = Boot.PWMDIV_1;
                Boot.xtal = Boot.XTAL_16MHZ ;       /* XTAL = 16 MHz */
                clockRate = 80;
            }
            else if (this.deviceName.match(/^LM3S1\D\d\d/)
                && !this.deviceName.match(/^LM3S1\D\1(1|6)/)) {
                Boot.sysClockDiv = Boot.SYSDIV_2_5; /* 80 MHz */
                Boot.pwmClockDiv = Boot.PWMDIV_1;
                Boot.xtal = Boot.XTAL_16MHZ ;       /* XTAL = 16 MHz */
                clockRate = 80;
            }
            else if (this.deviceName == "LM3S1621"
                || this.deviceName == "LM3S1651") {
                Boot.sysClockDiv = Boot.SYSDIV_2_5; /* 80 MHz */
                Boot.pwmClockDiv = Boot.PWMDIV_1;
                Boot.xtal = Boot.XTAL_16MHZ ;       /* XTAL = 16 MHz */
                clockRate = 80;
            }
            else {
                Boot.sysClockDiv = Boot.SYSDIV_4;   /* 50 MHz */
                Boot.pwmClockDiv = Boot.PWMDIV_8;   /* div by 8 */
                Boot.xtal = Boot.XTAL_8MHZ ;        /* XTAL = 8 MHz */
                Boot.configureLdo = false;
                Boot.ldoOut = Boot.LDOPCTL_2_75V;   /* LDO = 2.75V */
                clockRate = 50;
            }

            this.CPU.catalogName = "ti.catalog.arm.cortexm3";
            this.CPU.clockRate = clockRate;
        }
        else {
            Boot = xdc.useModule('ti.catalog.arm.cortexm4.tiva.ce.Boot');
            Boot.configureClock = true;
            Boot.oscSrc = Boot.OSCSRC_MAIN;     /* use main oscillator */
            Boot.pllBypass = false;            /* use PLL to drive sys clock */

            Boot.sysClockDiv = Boot.SYSDIV_2_5; /* 80 MHz */
            Boot.pwmClockDiv = Boot.PWMDIV_1;

            /* snowflake parts require 25MHz for ethernet */
            if (this.deviceName.match(/^TM4.129/)) {
                Boot.xtal = Boot.XTAL_25MHZ;        /* XTAL = 25 MHz */
                Boot.vcoFreq = Boot.VCO_480;        /* VCO = 480MHz */
                Boot.cpuFrequency = 120000000;      /* shoot for 120MHz */
                this.CPU.clockRate = 120;
            }
            else {
                Boot.xtal = Boot.XTAL_16MHZ;        /* XTAL = 16 MHz */
                Boot.vcoFreq = Boot.VCO_320;        /* VCO = 320MHz */
                Boot.cpuFrequency = 80000000;       /* shoot for 80MHz */
                this.CPU.clockRate = 80;
            }

            Boot.pllOutEnable = false;          /* no PLL output */
            Boot.ioscDisable = false;           /* IOSC is enabled */
            Boot.moscDisable = false;           /* MOSC is enabled */

            Boot.configureLdo = false;          /* no LDO */

            Boot.enhancedClockMode = true;

            /* legacy devices do not support enhancedClockMode */
            for each (var deviceName in m4LegacyDevices) {
                if (deviceName == this.deviceName) {
                    Boot.enhancedClockMode = false;
                    break;
                }
            }

            this.CPU.catalogName = "ti.catalog.arm.cortexm4";
        }
    }
}

/*
 * Devices which DO NOT support direct frequency setting
 * (enhanced Clock Mode).
 */

var m4LegacyDevices = [
    "LM4F110B2QR",   "LM4F110C4QR",   "LM4F110E5QR",   "LM4F110H5QR",
    "LM4F111B2QR",   "LM4F111C4QR",   "LM4F111E5QR",   "LM4F111H5QR",
    "LM4F112C4QC",   "LM4F112E5QC",   "LM4F112H5QC",   "LM4F112H5QD",
    "LM4F120B2QR",   "LM4F120C4QR",   "LM4F120E5QR",   "LM4F120H5QR",
    "LM4F121B2QR",   "LM4F121C4QR",   "LM4F121E5QR",   "LM4F121H5QR",
    "LM4F122C4QC",   "LM4F122E5QC",   "LM4F122H5QC",   "LM4F122H5QD",
    "LM4F130C4QR",   "LM4F130E5QR",   "LM4F130H5QR",   "LM4F131C4QR",
    "LM4F131E5QR",   "LM4F131H5QR",   "LM4F132C4QC",   "LM4F132E5QC",
    "LM4F132H5QC",   "LM4F132H5QD",   "LM4F230E5QR",   "LM4F230H5QR",
    "LM4F231E5QR",   "LM4F231H5QR",   "LM4F232E5QC",   "LM4F232H5BB",
    "LM4F232H5QC",   "LM4F232H5QD",

    /* Brainstorm */

    "LM4F110T9QR",   "LM4F111T9QR",   "LM4F112T9QC",   "LM4F112T9QD",
    "LM4F114T9QC",   "LM4F114T9QD",   "LM4F11AT9QR",   "LM4F11BT9QR",
    "LM4F11CT9QC",   "LM4F11CT9QD",   "LM4F130T9QR",   "LM4F131T9QR",
    "LM4F132T9QC",   "LM4F132T9QD",   "LM4F134T9QC",   "LM4F134T9QD",
    "LM4F13AT9QR",   "LM4F13BT9QR",   "LM4F13CT9QC",   "LM4F13CT9QD",
    "LM4F191T9QR",   "LM4F194T9QC",   "LM4F194T9QD",   "LM4F230T9QR",
    "LM4F231T9QR",   "LM4F232T9QC",   "LM4F232T9QD",   "LM4F234T9QC",
    "LM4F234T9QD",   "LM4F23BT9QR",   "LM4F23CT9QC",   "LM4F23CT9QD",
    "LM4F291T9QR",   "LM4F294T9QC",   "LM4F294T9QD",   "LM4F29BT9QR",
    "LM4F29CT9QC",   "LM4F29CT9QD",

    /* Blizzard */

    "LM4F210B2QR",   "LM4F210C4QR",   "LM4F211B2QR",   "LM4F211C4QR",
    "LM4F212C4QC",   "LM4FS11H5BB",   "LM4FS19H5BB",   "LM4FS1AH5BB",
    "LM4FS1BH5BB",   "LM4FS1GE5BB",   "LM4FS1GH5BB",   "LM4FS1PE5QK",
    "LM4FS1PH5QK",   "LM4FS99H5BB",   "LM4FS99H5QD",   "LM4FSXHH5BB",
    "LM4FSXLH5BB",   "SC01120E5QR",   "SC01230H5QR",   "SC01231H5QR",
    "SC01232H5BB",   "SC01232H5QC",
    "LM4FS1EH5BB",

    /* Blizzard Tiva */
    "TM4C1231C3PMI",  "TM4C1231D5PMI",  "TM4C1231E6PMI",  "TM4C1231H6PMI",
    "TM4C1230C3PMI",  "TM4C1230D5PMI",  "TM4C1230E6PMI",  "TM4C1230H6PMI",
    "TM4C1231D5PZI",  "TM4C1231E6PZI",  "TM4C1231H6PZI",  "TM4C1231H6PGEI",
    "TM4C1233C3PMI",  "TM4C1233D5PMI",  "TM4C1233E6PMI",  "TM4C1233H6PMI",
    "TM4C1232C3PMI",  "TM4C1232D5PMI",  "TM4C1232E6PMI",  "TM4C1232H6PMI",
    "TM4C1233D5PZI",  "TM4C1233E6PZI",  "TM4C1233H6PZI",  "TM4C1233H6PGEI",
    "TM4C1237D5PMI",  "TM4C1237E6PMI",  "TM4C1237H6PMI",  "TM4C1236D5PMI",
    "TM4C1236E6PMI",  "TM4C1236H6PMI",  "TM4C1237D5PZI",  "TM4C1237E6PZI",
    "TM4C1237H6PZI",  "TM4C1237H6PGEI", "TM4C123BE6PMI",  "TM4C123BH6PMI",
    "TM4C123AE6PMI",  "TM4C123AH6PMI",  "TM4C123BE6PZI",  "TM4C123BH6ZRBI",
    "TM4C123BH6PZI",  "TM4C123BH6PGEI", "TM4C123GE6PMI",  "TM4C123GH6PMI",
    "TM4C123FE6PMI",  "TM4C123FH6PMI",  "TM4C123GE6PZI",  "TM4C123GH6ZRBI",
    "TM4C123GH6PZI",  "TM4C123GH6PGEI",

    "TM4C1230C3PM",   "TM4C1230D5PM",   "TM4C1230E6PM",   "TM4C1230H6PM",
    "TM4C1231C3PM",   "TM4C1231D5PM",   "TM4C1231D5PZ",   "TM4C1231E6PM",
    "TM4C1231E6PZ",   "TM4C1231H6PGE",  "TM4C1231H6PM",   "TM4C1231H6PZ",
    "TM4C1232C3PM",   "TM4C1232D5PM",   "TM4C1232E6PM",   "TM4C1232H6PM",
    "TM4C1233C3PM",   "TM4C1233D5PM",   "TM4C1233D5PZ",   "TM4C1233E6PM",
    "TM4C1233E6PZ",   "TM4C1233H6PGE",  "TM4C1233H6PM",   "TM4C1233H6PZ",
    "TM4C1236D5PM",   "TM4C1236E6PM",   "TM4C1236H6PM",   "TM4C1237D5PM",
    "TM4C1237D5PZ",   "TM4C1237E6PM",   "TM4C1237E6PZ",   "TM4C1237H6PGE",
    "TM4C1237H6PM",   "TM4C1237H6PZ",   "TM4C123AE6PM",   "TM4C123AH6PM",
    "TM4C123BE6PM",   "TM4C123BE6PZ",   "TM4C123BH6PGE",  "TM4C123BH6PM",
    "TM4C123BH6PZ",   "TM4C123BH6ZRB",  "TM4C123FE6PM",   "TM4C123FH6PM",
    "TM4C123GE6PM",   "TM4C123GE6PZ",   "TM4C123GH6PGE",  "TM4C123GH6PM",
    "TM4C123GH6PZ",   "TM4C123GH6ZRB",

    "TM4E1231H6ZRB",  "TM4E1237H6ZRB",  "TM4E123GH6ZRB",  "TM4E1G31H6ZRB",
];


/*
 *  @(#) ti.platforms.tiva; 1, 0, 0,35; 6-19-2014 05:42:07; /db/ztree/library/trees/platform/platform-o53x/src/
 */

