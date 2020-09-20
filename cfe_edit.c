#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <inttypes.h>

#include <getopt.h>

typedef struct  bcm68380_nvram {	// default offset in flash: 0x580
    uint32_t    version;		// 0x6
    char        bootline[0x100];	/* "e=192.168.1.1:ffffff00 h=192.168.1.100 r=f f=vmlinux i=bcm963xx_fs_kernel d=1 p=0 " */
    char        board_id[0x10];		/* RV6699, NTU1402: 968380GERG; UFIBER NANO G: UBNT_SFU */
    uint32_t    main_thread;		/* 0 (or 1) */
    uint32_t    psi_size;		// 0x18
    uint32_t    mac_num;		// 0x0A or 0x10
    unsigned char basemac[6];		/* 0xA8F94B010203 */
    char        salign;		// reserved?
    uint8_t	backup_psi;		// Backup PSI: 0 disable, 1 - enable
    uint32_t    oldcksum;		// old BROADCOM CRC (not used)
    char        gponsn[13];		// "ELTX02010203", null-terminated
    char        gponpass[11];		// "00000000" or "        ", null-terminated
    char        wpspin[8];
    char        wlanparams[0xFF];
    uint8_t	wlan_feature;		// default 0, only ubnt bootloader has this
    uint32_t    syslog_size;		// 0
    uint32_t    nandpart_ofs[5];	// boot, rootfs1, rootfs2, data, bbt
    uint32_t    nandpart_size[5];
    char        voice_id[0x10];		// LE9530
    char        afe_id[8];
    char        salign1[65];
    uint8_t	aux_percent;		// Auxilary filesystem size percent: 0
    char	salign1a[6];
    uint8_t	mem_tm;			// TM memory allocation RV6699-MGTS, UFIBER NANO G: 0x14; RV6699-RT: 0x2C | flow memory allocation
    uint8_t	mem_mc;			// MC memory allocation: 0x04 | buffer memory allocation

    char        salign2[10];		// Only UBNT has mappings?
    uint8_t	part0_size;		// Partition 1 Size
    uint8_t	part1_size;		// Partition 2 Size
    uint8_t	part2_size;		// Partition 3 Size
    uint8_t	part3_size;		// Partition 4 Size (DATA)

    // Broadcom dongle host driver (dhd_linux.c)
    uint8_t	mem_dhd0;		// DHD0 memory allocation RV6699-MGTS: 0; RV6699-RT: 14
    uint8_t	mem_dhd1;		// DHD1 memory allocation
    uint8_t	mem_dhd2;		// DHD2 memory allocation

    char        salign2a;	// ??

    char        vendor_params[272];

    uint32_t    crc;			/* common CRC */
} bcm68380_nvram_t;

#ifdef ENABLE_VENDOR_ELTX
#define VENDOR_TYPE_ELTX 1
typedef struct vendor_params_eltx {
    char	primary_version[12];	// Version of primary firmware in flash
    char        salign2b[52];
    char	backup_version[12];	// Version of backup firmware in flash
    char        salign2c[52];
    uint32_t	primary_crc;		// CRC of primary
    uint32_t	backup_crc;		// CRC of backup
    char        reserved[134];
} vendor_params_eltx_t;
#endif

#ifdef ENABLE_VENDOR_UBNT
#define VENDOR_TYPE_UBNT 2
typedef struct vendor_params_ubnt {
    uint8_t	olt_mode;		// UFIBER: OLT Vendor ID: 1 - UBNT, 2 - HUAWEI
    char        salign2b;
    uint8_t	onu_mode;		// UFIBER: ONU work mode: 0 - ENFORCE_BY_OLT, 1 - BRIDGE, 2 - ROUTER; default 0xFF
    char        reserved[269];
} vendor_params_ubnt_t;
#endif

void hexDump (char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        printf ("%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}

void hexdump_nonempty(char *name, void *ptr, int size, uint8_t emptychar) {
    void *emptyarray = malloc(size);
    memset(emptyarray, emptychar, size);
    if (memcmp(emptyarray, ptr, size)) {
	hexDump(name, ptr, size);
    } else {
	printf("%s is empty! (0x%02x)\n", name, emptychar);
    }
    free (emptyarray);
}

/* реализация crc32 из openssh */
static const uint32_t crc32tab[] = {
        0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL,
        0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L,
        0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L,
        0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L,
        0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
        0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L,
        0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL,
        0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L,
        0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L,
        0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
        0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L,
        0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L,
        0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L,
        0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L, 0xb8bda50fL,
        0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
        0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL,
        0x76dc4190L, 0x01db7106L, 0x98d220bcL, 0xefd5102aL,
        0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L,
        0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L,
        0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
        0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL,
        0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L,
        0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL,
        0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L,
        0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
        0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL,
        0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L,
        0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L,
        0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L,
        0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
        0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L,
        0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL,
        0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL,
        0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L,
        0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
        0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L,
        0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL,
        0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L,
        0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL,
        0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
        0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L,
        0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL,
        0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L,
        0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L,
        0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
        0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL,
        0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L,
        0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL,
        0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL,
        0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
        0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L,
        0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L,
        0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL,
        0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L,
        0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
        0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L,
        0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L,
        0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL,
        0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L,
        0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
        0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L,
        0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL,
        0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L,
        0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL
};

uint32_t ssh_crc32(const uint8_t *buf, uint32_t size, uint32_t crc)
{
        uint32_t i;

        for (i = 0;  i < size;  i++)
                crc = crc32tab[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
        return crc;
}

int verify_cfe_nvram(unsigned char *src_mem, int vendor_type)
{
    uint32_t calc_crc;
    bcm68380_nvram_t bcm_nvram;
    int retcode = 0;

    memcpy(&bcm_nvram, src_mem, sizeof(bcm_nvram));

    printf("> broadcom nvram\n");
    printf(">> header version    : %#x ", ntohl(bcm_nvram.version));
    if (ntohl(bcm_nvram.version) != 6) {
	printf("UNKNOWN!!!\n");
	retcode++;
    } else {
	printf("(OK)\n");
    }

    printf(">> main thread       : %#x\n", ntohl(bcm_nvram.main_thread));
    printf(">> psi size (KB)     : %u\n", ntohl(bcm_nvram.psi_size));
    printf(">> enable backup PSI : %u\n", bcm_nvram.backup_psi);
    printf(">> wlan feature      : %#x\n", bcm_nvram.wlan_feature);
    printf(">> syslog size (KB)  : %u\n", ntohl(bcm_nvram.syslog_size));
    printf(">> AUX FS percent    : %u\n", bcm_nvram.aux_percent);

    if (memchr(&bcm_nvram.bootline, 0, sizeof(bcm_nvram.bootline))) {
	printf(">> bootline          : \"%s\"\n", bcm_nvram.bootline);
    } else {
	// string terminated with 0xFF
	char *bootline_cleared = malloc(sizeof(bcm_nvram.bootline));
	memcpy(bootline_cleared, &bcm_nvram.bootline, sizeof(bcm_nvram.bootline));
	char *end_ptr = memchr(bootline_cleared, 0xFF, sizeof(bcm_nvram.bootline));
	if (end_ptr) {
	    memset(end_ptr, 0, 1); //null-terminate
	    printf(">> bootline (0xFF end) : \"%s\"\n", bootline_cleared);
	} else {
	    hexDump (">> bootline structure dump (unknown)", &bcm_nvram.bootline, sizeof(bcm_nvram.bootline));
	    retcode++;
	}
	free(bootline_cleared);
    }

    printf(">> board_id          : \"%s\"\n", bcm_nvram.board_id);

    printf(">> MAC num           : %u\n", ntohl(bcm_nvram.mac_num));
    printf(">> MAC base          : %02X%02X%02X%02X%02X%02X\n",
     bcm_nvram.basemac[0], bcm_nvram.basemac[1], bcm_nvram.basemac[2], bcm_nvram.basemac[3], bcm_nvram.basemac[4], bcm_nvram.basemac[5]);

    printf(">> PON SN            : \"%s\"\n", bcm_nvram.gponsn);
    printf(">> PON PASS          : \"%s\"\n", bcm_nvram.gponpass);
    printf(">> voice_id          : \"%s\"\n", bcm_nvram.voice_id);

    printf(">> TM memory (MB)    : %u\n", bcm_nvram.mem_tm);
    printf(">> MC memory (MB)    : %u\n", bcm_nvram.mem_mc);


    printf(">> DHD0 memory (MB)  : %u\n", bcm_nvram.mem_dhd0);
    printf(">> DHD1 memory (MB)  : %u\n", bcm_nvram.mem_dhd1);
    printf(">> DHD2 memory (MB)  : %u\n", bcm_nvram.mem_dhd2);

    // TODO: check offsets
    printf(">>> Nand partitions list (size in kilobytes):\n");
    printf(">>> boot,    offset: %#x,\tsize: %#x\n", ntohl(bcm_nvram.nandpart_ofs[0]), ntohl(bcm_nvram.nandpart_size[0]));
    printf(">>> rootfs1, offset: %#x,\tsize: %#x\n", ntohl(bcm_nvram.nandpart_ofs[1]), ntohl(bcm_nvram.nandpart_size[1]));
    printf(">>> rootfs2, offset: %#x,\tsize: %#x\n", ntohl(bcm_nvram.nandpart_ofs[2]), ntohl(bcm_nvram.nandpart_size[2]));
    printf(">>> data,    offset: %#x,\tsize: %#x\n", ntohl(bcm_nvram.nandpart_ofs[3]), ntohl(bcm_nvram.nandpart_size[3]));
    printf(">>> bbt,     offset: %#x,\tsize: %#x\n", ntohl(bcm_nvram.nandpart_ofs[4]), ntohl(bcm_nvram.nandpart_size[4]));


    switch (vendor_type) {
#ifdef ENABLE_VENDOR_ELTX
	case VENDOR_TYPE_ELTX:;
	    vendor_params_eltx_t vendor_params_eltx;
	    memcpy(&vendor_params_eltx, &bcm_nvram.vendor_params, sizeof(vendor_params_eltx));

	    printf(">> Parsed vendor parameters\n");
	    printf(">>> Primary firmware ver  : \"%s\"\n", vendor_params_eltx.primary_version);
	    printf(">>> Backup  firmware ver  : \"%s\"\n", vendor_params_eltx.backup_version);
	    printf(">>> Primary firmware CRC  : %#08x\n", vendor_params_eltx.primary_crc);
	    printf(">>> Backup  firmware CRC  : %#08x\n", vendor_params_eltx.backup_crc);
	    hexdump_nonempty(">> salign2c structure", &vendor_params_eltx.salign2b, sizeof(vendor_params_eltx.salign2b), 0xFF);
	    hexdump_nonempty(">> salign2b structure", &vendor_params_eltx.salign2c, sizeof(vendor_params_eltx.salign2c), 0xFF);
	    hexdump_nonempty(">> reserved space of vendor_params", &vendor_params_eltx.reserved, sizeof(vendor_params_eltx.reserved), 0xFF);

	    break;
#endif

#ifdef ENABLE_VENDOR_UBNT
	case VENDOR_TYPE_UBNT:;
	    vendor_params_ubnt_t vendor_params_ubnt;
	    memcpy(&vendor_params_ubnt, &bcm_nvram.vendor_params, sizeof(vendor_params_ubnt));

	    printf(">> Parsed vendor parameters\n");
	    printf(">>> ONU work mode     : %#x\n", vendor_params_ubnt.onu_mode);
	    printf(">>> OLT vendor ID     : %#x\n", vendor_params_ubnt.olt_mode);
	    printf(">>> Partition 0 size (MB) : %u\n", bcm_nvram.part0_size);
	    printf(">>> Partition 1 size (MB) : %u\n", bcm_nvram.part1_size);
	    printf(">>> Partition 2 size (MB) : %u\n", bcm_nvram.part2_size);
	    printf(">>> Partition 3 size (MB) : %u\n", bcm_nvram.part3_size);

	    hexdump_nonempty(">> salign2b structure", &vendor_params_ubnt.salign2b, sizeof(vendor_params_ubnt.salign2b), 0xFF);
	    hexdump_nonempty(">> reserved space of vendor_params", &vendor_params_ubnt.reserved, sizeof(vendor_params_ubnt.reserved), 0xFF);

	    break;
#endif
	default:;

	    hexdump_nonempty(">> reserved space of vendor_params", &bcm_nvram.vendor_params, sizeof(bcm_nvram.vendor_params), 0xFF);
	    break;
    }

    // old broadcom CRC (not used in V6, but need to check)
    if (bcm_nvram.oldcksum == 0xFFFFFFFF) {
	printf(">> oldcksum: %#08x (empty, not verified)\n", bcm_nvram.oldcksum);
    } else {
	calc_crc = htonl(ssh_crc32(src_mem, 0x126, 0xFFFFFFFF));
	if (calc_crc == bcm_nvram.oldcksum) {
    	    printf(">> oldcksum: %#08x (no mismatch)\n", bcm_nvram.oldcksum);
	} else {
	    printf("***!!!*** calculated oldcksum mismatch: %#08x, %#08x\n", bcm_nvram.oldcksum, calc_crc);
	    retcode++;
	}
    }

    // new CRC
    memset(src_mem + sizeof(bcm_nvram) - sizeof(bcm_nvram.crc), 0, sizeof(bcm_nvram.crc));
    calc_crc = htonl(ssh_crc32(src_mem, sizeof(bcm_nvram), 0xFFFFFFFF));

    if (calc_crc == bcm_nvram.crc) {
        printf(">> CRC     : %#08x (no mismatch)\n", bcm_nvram.crc);
    } else {
	printf("***!!!*** calculated CRC mismatch: %#08x, %#08x\n", bcm_nvram.crc, calc_crc);
	retcode++;
    }

    hexdump_nonempty(">> afe_id", &bcm_nvram.afe_id, sizeof(bcm_nvram.afe_id), 0xFF);
    hexdump_nonempty(">> wpspin", &bcm_nvram.wpspin, sizeof(bcm_nvram.wpspin), 0xFF);

    // do not print empty large arrays
    hexdump_nonempty(">> wlanparams structure", &bcm_nvram.wlanparams, sizeof(bcm_nvram.wlanparams), 0xFF);
    hexdump_nonempty(">> salign structure", &bcm_nvram.salign, sizeof(bcm_nvram.salign), 0xFF);

    int salign2_extra = sizeof(bcm_nvram.part0_size) + sizeof(bcm_nvram.part1_size) + sizeof(bcm_nvram.part2_size) + sizeof(bcm_nvram.part3_size);

#ifdef ENABLE_VENDOR_UBNT
    if (vendor_type == VENDOR_TYPE_UBNT) salign2_extra = 0;
#endif

    hexdump_nonempty(">> salign1 structure", &bcm_nvram.salign1, sizeof(bcm_nvram.salign1), 0xFF);
    hexdump_nonempty(">> salign1a structure", &bcm_nvram.salign1a, sizeof(bcm_nvram.salign1a), 0xFF);
    hexdump_nonempty(">> salign2 structure", &bcm_nvram.salign2, sizeof(bcm_nvram.salign2) + salign2_extra, 0xFF);
    hexdump_nonempty(">> salign2a structure", &bcm_nvram.salign2a, sizeof(bcm_nvram.salign2a), 0xFF);

    return retcode;
}

int replace_cfe_nvram_crc(unsigned char *src_mem)
{
    uint32_t calc_crc;
    bcm68380_nvram_t bcm_nvram;
    int retcode = 0;

    memcpy(&bcm_nvram, src_mem, sizeof(bcm_nvram));

    if (ntohl(bcm_nvram.version) != 6) {
	fprintf(stderr, "NVRAM version is wrong, dont know how to handle CRC!\n");
	retcode++;
	goto exit;
    }

    memset(src_mem + sizeof(bcm_nvram) - sizeof(bcm_nvram.crc), 0, sizeof(bcm_nvram.crc));
    calc_crc = htonl(ssh_crc32(src_mem, sizeof(bcm_nvram), 0xFFFFFFFF));

    printf(">> OLD CRC     : %#08x\n", bcm_nvram.crc);
    printf(">> NEW CRC     : %#08x\n", calc_crc);

    bcm_nvram.crc = calc_crc;
    memcpy(src_mem, &bcm_nvram, sizeof(bcm_nvram)); // store CRC

    exit:
    return retcode;
}

int edit_cfe_nvram(void *src_mem, int argc, char ** argv, char *options_exist) {

    bcm68380_nvram_t bcm_nvram;
    int retcode = 0;

    memcpy(&bcm_nvram, src_mem, sizeof(bcm_nvram));

    optind = 1; // reset getopt counter
    int c;
    while ( 1 ) {
        c = getopt(argc, argv, options_exist);

        if (c == -1)
                break;

        switch (c) {
                case 'L':  // bootline
			memset(bcm_nvram.bootline, 0, sizeof(bcm_nvram.bootline));
			strncpy((char *)&bcm_nvram.bootline, optarg, 255);
                        break;
                case 'B':  // board id
			memset(bcm_nvram.board_id, 0, sizeof(bcm_nvram.board_id));
			if (!sscanf(optarg, "%15s", (char *)&bcm_nvram.board_id)) retcode++;
                        break;
                case 'M':  // base MAC
			if (sscanf(optarg, "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
			    &bcm_nvram.basemac[0], &bcm_nvram.basemac[1], &bcm_nvram.basemac[2], &bcm_nvram.basemac[3], &bcm_nvram.basemac[4], &bcm_nvram.basemac[5]) != 6) retcode++;
                        break;
                case 'S':  // GPON SN
			if (!sscanf(optarg, "%12s", (char *)&bcm_nvram.gponsn)) retcode++;
			if (strlen(bcm_nvram.gponsn) != (sizeof(bcm_nvram.gponsn) - 1)) retcode++; // check resulting length
                        break;
                case 'P':  // GPON PASS
			if (!sscanf(optarg, "%10s", (char *)&bcm_nvram.gponpass)) retcode++;
                        break;
                case 'V':  // VOICE ID
			if (!sscanf(optarg, "%15s", (char *)&bcm_nvram.voice_id)) retcode++;
                        break;
                case 'W':  // WPS PIN
			if (!sscanf(optarg, "%7s", (char *)&bcm_nvram.wpspin)) retcode++;
                        break;
                case 'I':  // psI_size
			if (!sscanf(optarg, "0x%2hhx", (char *)&bcm_nvram.psi_size)) { retcode++; } else { bcm_nvram.psi_size = htonl(bcm_nvram.psi_size); }
                        break;
                case 'N':  // mac_Num
			if (!sscanf(optarg, "0x%2hhx", (char *)&bcm_nvram.mac_num)) { retcode++; } else { bcm_nvram.mac_num = htonl(bcm_nvram.mac_num); }
                        break;
                case 'A':  // bAckup_psi
			if (!sscanf(optarg, "0x%2hhx", (char *)&bcm_nvram.backup_psi)) retcode++;
                        break;
                case 'Y':  // sYslog_size
			if (!sscanf(optarg, "0x%2hhx", (char *)&bcm_nvram.syslog_size)) { retcode++; } else { bcm_nvram.syslog_size = htonl(bcm_nvram.syslog_size); }
                        break;
                case 'F':  // wlan_Feature
			if (!sscanf(optarg, "0x%2hhx", (char *)&bcm_nvram.wlan_feature)) retcode++;
                        break;

		default:
                        break;
                }
    }

    memcpy(src_mem, &bcm_nvram, sizeof(bcm_nvram)); // store changes
    return retcode;
}


int main(int argc, char ** argv)
{

 int opt_input = 0, opt_output = 0, opt_verify = 0, opt_edit = 0;
 char *input_name = NULL, *output_name = NULL;

 int nvram_offset = 0x580; // offset in cferom mtd or fullflash (0x580 - sercomm, 0x540 - ubnt)
 int vendor_type = 0;

 static char options_exist[] = "vei:o:p:t:L:B:M:S:P:V:W:I:N:A:Y:F:"; // common options set for both getopt()

 int c;
 while ( 1 ) {
        c = getopt(argc, argv, options_exist);
        if (c == -1)
                break;

        switch (c) {
                case 'v':  // verify nvram
                        opt_verify++;
                        break;
                case 'e':  // edit nvram
                        opt_edit++;
                        break;
                case 'i':  // input file
                        input_name = optarg;
                        opt_input++;
                        break;
                case 'o':  // output_file
                        output_name = optarg;
                        opt_output++;
                        break;
                case 'p':  // file position
                        if (!sscanf(optarg, "0x%8x", &nvram_offset)) goto print_usage;
                        break;
                case 't':  // vendor type
                        if (!sscanf(optarg, "%2d", &vendor_type)) goto print_usage;
                        break;
		case 'L':
		case 'B':
		case 'M':
		case 'S':
		case 'P':
		case 'V':
		case 'W':
		case 'I':
		case 'N':
		case 'A':
		case 'Y':
		case 'F':
			break; // skip options, used in edit function
                default:
                        goto print_usage;
                }
 }


    if (!opt_input)				goto print_usage;
    if (opt_verify && opt_edit)			goto print_usage;

    if (opt_verify && !opt_input)		goto print_usage;
    if (opt_edit && !(opt_input && opt_output))	goto print_usage;

    size_t input_size;
    int retcode = 0;

    FILE* source_file = fopen(input_name,"r");
    if (source_file == 0) {
        perror("Cannot open file for read");
        return 1;
    }

    /* будет ли fstat тут более хорош? */
    fseek(source_file, 0, SEEK_END);
    input_size = ftell(source_file);

    unsigned char *src_mem = malloc(input_size);
    memset(src_mem, 0, input_size);

    fseek(source_file, 0, SEEK_SET);
    fread(src_mem, input_size, 1, source_file);
    fclose(source_file);

    if (opt_verify) {
        retcode += verify_cfe_nvram(src_mem + nvram_offset, vendor_type);

    } else if (opt_edit) {
        retcode += edit_cfe_nvram(src_mem + nvram_offset, argc, argv, options_exist);
	if (retcode) {
	    fprintf(stderr, "Error processing input options!\n");
	    goto exit;
	}
        retcode += replace_cfe_nvram_crc(src_mem + nvram_offset);
        retcode += verify_cfe_nvram(src_mem + nvram_offset, vendor_type);

	if (!retcode) {
	    FILE* target_file = fopen(output_name,"w");
	    if (target_file == 0) {
    		perror("Cannot open file for write");
		free(src_mem);
    		return 1;
	    }

	    fseek(target_file, 0, SEEK_SET);
	    fwrite(src_mem, input_size, 1, target_file);
	    fclose(target_file);
	}
    }

    exit:
    free(src_mem);

    if (retcode) fprintf(stderr, "Some errors happen.\n");
    return retcode;

    print_usage:

    fprintf(stderr, "Usage for %s:\n"
    " -v     nvram verify\n"
    " -e     nvram edit (if no edit option given, just recalculate checksum)\n"
    " -i     <input file>\n"
    " -o     <output file> (for edit or recalculate options)\n"
    " -p     <nvram offset> (hex nvram offset in input cferom or fullflash, default 0x580)\n"
    " -t     <vendor_structure_type>\n"
    "           0: common (default)\n"
#ifdef ENABLE_VENDOR_ELTX
    "           1: eltex\n"
#endif
#ifdef ENABLE_VENDOR_UBNT
    "           2: ubiquiti\n"
#endif

    " -L     <bootline>    \"e=192.168.1.1:ffffff00 h=192.168.1.100 g= r=f f=vmlinux i=bcm963xx_fs_kernel d=1 p=0 c= a= \" (255 chars max)\n"
    " -B     <board id>    \"968380GERG\"\n"
    " -M     <MAC base>    \"3C9872010203\"\n"
    " -S     <GPON SN>     \"ELTX556677AA\"\n"
    " -P     <GPON PASS>   \"          \" (10 chars max)\n"
    " -V     <VOICE ID>    \"LE9540\"\n"
    " -W     <WPS PIN>     \"1234567\" (7 chars max)\n"
    " -I     <PSI size>    \"0x18\"\n"
    " -N     <MAC num>     \"0x10\"\n"
    " -A     <backup PSI>  \"0x0\" or \"0x1\"\n"
    " -Y     <syslog size> \"0x0\"\n"
    " -F     <WLAN enable> \"0x0\" or \"0x1\"\n"

    "\n (C) [anp/hsw] 2019, GPLv2 license applied\n"
    " crc32 code (C) openssh team\n"
    "\n", argv[0]);

    return 1;
}

