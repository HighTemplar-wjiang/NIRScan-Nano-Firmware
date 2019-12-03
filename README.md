# NIRScan-Nano-Firmware
Customized firmware for NIRScan Nano. 

## Customized features
- Turn Bluetooth on at start (original codes have bugs).

## Dependencies
- Code Composer Studio (CCS) v6
- TI compilier 5.2.x
- TI-RTOS 2.10.1.38
- xdctools 3.30.4.52

Make sure that CCS discovers all the above installed products. If not, go to Window -> Preferences -> Code Composer Studio -> Products -> Rediscover. 

## Compile
- Import _driverlib_, _usblib_ and _dlpspeclib_ in the _lib_ folder into CCS workspace. 
- Copy the ⁨lib⁩/⁨xdctools_3_30_04_52_core⁩/packages⁩/⁨ti⁩/⁨platforms/tiva folder into your installation path of _xdctools_.
- Make the project. 

## License
Following TI's original License.
