/*
 *
 * This module has the version number info.
 *
 * Copyright (C) 2015-2016 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#ifndef VERSION_H
#define VERSION_H

#define TIVA_VERSION_MAJOR 2
#define TIVA_VERSION_MINOR 1
#define TIVA_VERSION_BUILD 0

#define MAX_VERSION_STR 11   // Needs to be less than 48 (DIS_MAXIMUM_SUPPORTED_STRING)

#define TIVA_HW_REV	"C.D.B.B.A"	//HW revisions - TIVA.dlpc150.dmd.detector_board.optical_engine

#endif //VERSION_H

/*
----------------------------------------------------------------------
KNOWN ISSUES:
----------------------------------------------------------------------
KST 1.1 NIRScan Nano (NanoScan) iOS App not fully compatible with 2.0.0 TIVA. New
		scan configurations written by the GUI will not be read or interpretted correctly.
		This has been corrected in KST NIRscan Nano (NanoScan) v1.1.1


----------------------------------------------------------------------
VERSION HISTORY:
----------------------------------------------------------------------
 * 2.1.0 - Added two commands to store and read Model Name from EEPROM. If no Model name is stored, Bluetooth 
            defaults to DLPNIRNANOEVM
 * 2.0.6 - While pressing the scan button and applying power, the Tiva boots into DFU mode. 
            Useful if the Tiva firmware is updated and crashes on bootup.
         - Placing and SD card with the file SKIP_CFG in a directory with the serial number 
            of the NIRscan Nano skips the default scan configuration. Useful if a corrupted
            scan configuration is written to EEPROM.
         - Added global #define to remove hardcoded wavelength ranges.
         - Fixed an issue where the ADC PGA gain is not properly set on some scans. Now
           ADC PGA setting changes are read back and confirmed to be set to the appropriate value.
         - Change from reporting median of ADC smaples to compute the mean of ADC samples
            for lower noise.
         - Added #defines to common.h control at compile time the power-up behaviour of:
              Bluetooth On/Off
              Hibernation Mode enabled/disabled
              Bluetooth LED blinking or on during connection
              Routing buttons and LED to expansion connector
         - Corrected pointer arithmetic on DLP Spectrum Library that caused issues in 64-bit processors.
 * 2.0.2 - Added NNO_CMD_SET_FIXED_PGA( true/false, pga_value) to fix the PGA gain for all subsequent scans to 
            the pga_value (1x, 2x, 4x, 8x, 16x, 32x, or 64x)
 * 2.0.1 - Corrected an issue in the extimated scan time returned from NNO_CMD_CALIB_GEN_PTNS in calibration scans
 * 2.0.0 - Keeping the scan button pressed at EVM start-up will skip setting the
            active scan configuration from EEPROM as a potential failure recovery mechanism.
         - Added capability to set any selected Configuration on EVM as the default configuration.
         - Take mean of ADC samples instead of median for better SNR
         - Modified BLE advertisement flags to explicitly remove BD/EDR supported flag.
         - Computation of Scan Time changed to support variable exposure times across sections.
         - Slew Scan capability added that supports different exposure times for different sections.
         - Added commands to let Tiva perform scan interpret (Column Scans only)
         - Hibernation enable read and write commands added.
 * 1.1.8 - BLE transmission bugfix (Header was sometimes overwritten previously)
 * 1.1.7 - UART capability added
 * 1.1.6 - Changed file naming convention of scans stored in SD card to HEX; removed pattern source argument from NNO_CMD_DLPC_ENABLE command
 * 1.1.3 - USB interrupts are now not disabled during scan
 * 1.1.2 - Fixed issues in release build configuration and fixed some static code checker findings
 * 1.1.1 - Fixed some issues in Error status reporting.
 * 1.1.0 - First release candidate for v1.1
 * 1.0.0 - ScanData header version populated for future version tracking. ScanConfig data length fixed (BLE). RTSC set to 3.30.4.52. Corrected DLP Spectrum Library version mismatch
 * 0.35.0 - Hadamard added. Scan data corruption fixed. Automatic PGA setting. Other bugfixes
 * 0.34.8 - Added Button Test command. 
 * 0.34.4 - More comprehensive checking when reading invalid EEPROM data. Added LED Test command
 * 0.34.3 - Fixed crash in USB and SDcard when reading invalid EEPROM Scan Config Data
 * 0.34.2 - Bug in EEPROM clear fixed.
 * 0.34.0 - Serial number setting is mandated before wavelength calibration is allowed.
 * 0.33.3 - Fixed the issue where standalone scan (button) was crashing
 * 0.33.0 - EEPROM layout updated. Scan and cfg serial numbering implemented.
 * 0.31.1 - BLE - GATT profile changes, Notification Handler
 * 0.31.0 - Corrected a duplicate entry in the command table
 * 0.30.0 - Scan command interface cleaned up to not take num_patterns as a parameter.
 * 0.29.0 - Supported added to store scan data in micro SD card and read back.
 * 0.28.1 - While reading DLPC150 version, Tiva times out if DLPC150 is not responding.
 * 0.28.0 - Support added for sequences with black vector after every 24 patterns. To be used with Spectrum Library v0.4
 * 0.27.0 - support for user inputs in wavelength units. DLP spectrum library v0.3
 * 0.26.0 - genPatterns and scan_interpret that takes num_patterns as user input.
 * 0.25.0 - Works with v0.2 of spectrum library that has the inital implementation of scan_interpret function
 * 0.14.0 - DLPCEnable without turning ON the lamp supported now for slit and wavelength alignemnt process.
 * 0.13.0 - Wavelength calibration implemented.
 * 0.12.0 - Scans hang in version 0.11.1 due to ADC being not initialized. It is fixed in 0.12.0.
 * 0.10.0 - PGA gain setting now working. Also added a Get PGA gain command.
 * 0.9.0 - Tiva will now averaging multiple scans as per num_repeat field in scanConfig. But averaging more than
 *         8 scans can lead to overflows as float not yet implemented in Tiva averaging.
 * 0.8.0 - Calibration scans with patterns generated by Tia on-the-fly and streaming via RGB interface to DLPC150 implemented.
 * 0.7.0 - Tiva now retuns serialized ColumnScanDataStruct when scan result file is read back.
 * 0.6.1 - Moved generate patterns functions to spectrum library.
 * 0.6.0 - Import and Export (to device) of user defined scan configs implemented.
 * 0.4.0 - Cleaned up dlpc150 functions and command numbers; also changed USB PID to 0x4200
 * 0.3.1 - Fixed the issue with slit aliggnment calibration rightmost slider not working properly.
 *         SPI clock used to program/read back DLPC150 firmware from flash reduced to 2.5Mhz to avoid occasional checksum mismatches.
 * 0.3.0 - Scan with patterns streaming over RGB interface now working. Also supported scans with > 255 number of patterns.
 * 0.2.4 - Removed the error message pop up during slit alignment calibration when expected number of peaks were not found in scan data.
 *         Added more details to BQ test and added a Bluetooth test.
 * 0.2.2 - Added TMP and HDC tests, added test result log file, fixed the issue of lamp not turning ON on repeated scans
 * 0.2.0 - Version with detector and slit alignment scans working with patterns in flash + a subset of PCB test functions
 * 0.1.0 - Initial version with detector alignement scan working with patterns stored in flash
 *
 */
