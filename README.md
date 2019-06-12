# cfe_edit
Broadcom cfe configuration (nvram) viewer and editor.
Configuration stored in first flash block with cferom code.

Examples of configuration found in nvram.* (use "-p 0" argument to read them)
Explanation of CFEROM console prints found in cfe_debug_prints.txt (retreived from ASUS GPL code)

For Broadcom 68380 and others.
Tested on Ubiquiti ufiber nano, Sercomm RV6699, Eltex NTU1402.

Many values are still unknown. Patches are welcome.

Usage:

-v     nvram verify

-e     nvram edit (if no edit option given, just recalculate checksum)

-i     <input file>

-o     <output file> (for edit or recalculate options)

-p     <nvram offset> (hex nvram offset in input cferom or fullflash, default 0x580)

-t     <vendor_structure_type>

       0: common (default)

       1: eltex

       2: ubiquiti

-L     <bootline>    "e=192.168.1.1:ffffff00 h=192.168.1.100 g= r=f f=vmlinux i=bcm963xx_fs_kernel d=1 p=0 c= a= " (255 chars max)

-B     <board id>    "968380GERG"

-M     <MAC base>    "3C9872010203"

-S     <GPON SN>     "ELTX556677AA"

-P     <GPON PASS>   "          " (10 chars max)

-V     <VOICE ID>    "LE9540"

-W     <WPS PIN>     "1234567" (7 chars max)

-I     <PSI size>    "0x18"

-N     <MAC num>     "0x10"

-A     <backup PSI>  "0x0" or "0x1"

-Y     <syslog size> "0x0"

-F     <WLAN enable> "0x0" or "0x1"

