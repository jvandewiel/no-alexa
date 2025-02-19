// parse preloader/bootloader flash dump
// we want the actual files

import * as  utils from './modules/utils.js';

// headers, magics, maps

// ----------------------------------------------------------------------------

// https://android.googlesource.com/platform/external/u-boot/+/master/tools/mtk_image.h

const GFH_HEADER_MAGIC = 'MMM';

const gfh_common_header = {
    magic: 3,
    version: 1,
    size: 2,
    type: 2,
}

const GFH_FILE_INFO_NAME = "FILE_INFO";

const gfh_file_info = {
    name: 12,
    unused: 4,
    filetype: 2,
    flashtype: 1,
    sigtype: 1,
    loadaddr: 4,
    totalsize: 4,
    maxsize: 4,
    hdrsize: 4,
    sigsize: 4,
    jumpoffset: 4,
    processed: 4
}

const gfh_bl_info = {
    attr: 4
}

const gfh_anti_clone = {
    ac_b2k: 1,
    ac_b2c: 1,
    pad: 2,
    ac_offset: 4,
    ac_len: 4
}

const gfh_brom_cfg = {
    cfg_bits: 4,
    usbdl_by_auto_detect_timeout_ms: 4,
    unused: 45,
    jump_bl_arm64: 1,
    unused2: 2,
    usbdl_by_kcol0_timeout_ms: 4,
    usbdl_by_flag_timeout_ms: 4,
    pad: 4
}

let gfh_brom_sec_cfg = {
    cfg_bits: 4,
    customer_name: 20,
    pad: 4,
}

// ----------------------------------------------------------------------------

// https://android.googlesource.com/kernel/mediatek/+/android-mediatek-sprout-3.4-kitkat-mr2/drivers/misc/mediatek/masp/mt6582/asf/asf_inc/sec_rom_info.h

const ANDROID_ROMINFO_MAGIC = "AND_ROMINFO_v";
const AND_ROM_INFO_SIZE = 960;
const AND_ROMINFO_T = {
    m_id: 16,
    m_rom_info_ver: 2,
    m_platform_id: 16,
    m_project_id: 16,

    m_sec_ro_exist: 2,
    m_sec_ro_offset: 2,
    m_sec_ro_length: 2,
    m_ac_offset: 2,
    m_ac_length: 2,
    m_sec_cfg_offset: 2,
    m_sec_cfg_length: 2,

    m_reserve1: 128,
    m_SEC_CTRL: 52, // AND_SEC_CTRL_SIZE
    m_reserve2: 18,
    m_SEC_BOOT_CHECK_PART: 90, // AND_SEC_BOOT_CHECK_PART_SIZE
    m_SEC_KEY: 592 // AND_SEC_KEY_SIZE
}

// https://android.googlesource.com/kernel/mediatek/+/android-mediatek-sprout-3.4-kitkat-mr2/drivers/misc/mediatek/masp/mt6582/asf/asf_inc/sec_ctrl.h
const ROM_INFO_SEC_CTRL_ID = "AND_SECCTRL_v";
const AND_SEC_CTRL_SIZE = 52;
const AND_SECCTRL_T = {
    m_id: 16,
    m_sec_cfg_ver: 2,
    m_sec_usb_dl: 2,
    m_sec_boot: 2,
    m_sec_modem_auth: 2,
    m_sec_sds_en: 2,

    m_seccfg_ac_en: 1,
    m_sec_aes_legacy: 1,
    m_secro_ac_en: 1,
    m_sml_aes_key_ac_en: 1,

    reserve: 6 // 3 x int? 
}

// https://android.googlesource.com/kernel/mediatek/+/android-mediatek-sprout-3.4-kitkat-mr2/drivers/misc/mediatek/masp/mt6582/asf/asf_inc/sec_key.h
// https://github.com/andr3jx/MTK6577/blob/master/mediatek/platform/mt6577/preloader/src/security/inc/sec_key.h
const ROM_INFO_SEC_KEY_ID = "AND_SECRO_v"; // not AND_SECKEY_v !
const AND_SEC_KEY_SIZE = 592;
const AND_SECKEY_T = {
    m_id: 16,
    m_sec_key_ver: 2,
    img_auth_rsa_n: 256,
    img_auth_rsa_e: 5,
    sml_aes_key: 32,
    crypto_seed: 16,
    sml_auth_rsa_n: 256,
    sml_auth_rsa_e: 5
}

// https://android.googlesource.com/kernel/mediatek/+/android-mediatek-sprout-3.4-kitkat-mr2/drivers/misc/mediatek/masp/mt6582/asf/asf_inc/sec_boot.h


// ----------------------------------------------------------------------------
// https://github.com/ni/u-boot/blob/master/tools/mtk_image.h
// https://github.com/ni/u-boot/blob/master/tools/mtk_image.c

const NAND_BOOT_NAME = "BOOTLOADER!";
const NAND_BOOT_VERSION = "V006";
const NAND_BOOT_ID = "NFIINFO";

/* BootROM layout header */
const brom_layout_header = {
    name: 8,
    version: 4,
    header_size: 4,
    total_size: 4,
    magic: 4,
    type: 4,
    header_size_2: 4,
    total_size_2: 4,
    unused: 4
}
// ----------------------------------------------------------------------------

class Bootfile {

    filename = '';
    data = {};

    /**
     * constructor, loads binary file and assigns to data prop
     * @param {*} filename 
     */
    constructor(filename) {
        this.filename = filename;
        let data = utils.loadBinFile(filename);
        if (data !== undefined) {
            this.log('loaded binfile');
            console.log(data);
            this.data = data;
        }
    }

    /**
     * parse parse_gfh_header / MMM headers
     * see https://elixir.bootlin.com/u-boot/v2023.04-rc3/source/tools/mtk_image.h
     */
    parse_gfh_header() {
        let gfh_header = {};
        let offset = 0;
        // find instances
        while (true) {
            offset = this.data.indexOf(GFH_HEADER_MAGIC, offset + 3);
            this.log(`found ${GFH_HEADER_MAGIC} at offset: ${offset}`);
            // exit if not found
            if (offset === -1) { break }

            // parse common header
            let size = utils.structSize(gfh_common_header);
            let buf = this.data.subarray(offset, offset + size);
            let hdr = utils.parseBuf(buf, gfh_common_header);
            this.log(`header type: ${hdr.type} size: ${hdr.size}`);

            // get data after the common header, depending on type
            // watch the . and _!
            let tmpBuf = this.data.subarray(offset + size, offset + size + hdr.size);
            let data = {};
            console.log(tmpBuf)

            switch (hdr.type) {
                case 0:
                    this.log('handling file info');
                    // 8 + 48 = 56 bytes
                    data = utils.parseBuf(tmpBuf, gfh_file_info);
                    data.hdr = hdr;
                    gfh_header.bl_info = data;
                    break;
                case 1:
                    // 8 + 4 = 12 bytes
                    this.log('handling bl info');
                    data = utils.parseBuf(tmpBuf, gfh_bl_info);
                    data.hdr = hdr;
                    gfh_header.gfh_bl_info = data
                    break;
                case 2:
                    // 8 + 12 = 20
                    this.log('handling anti clone');
                    data = utils.parseBuf(tmpBuf, gfh_anti_clone);
                    data.hdr = hdr;
                    gfh_header.anti_clone = data;
                    break;
                case 7:
                    // 8 + 92 = 100 bytes
                    this.log('handling brom cfg');
                    data = utils.parseBuf(tmpBuf, gfh_brom_cfg);
                    data.hdr = hdr;
                    gfh_header.brom_sig = data;
                    break;
                case 8:
                    // 8 + 28 = 36
                    this.log('handling brom sec cfg');
                    data = utils.parseBuf(tmpBuf, gfh_brom_sec_cfg);
                    data.hdr = hdr;
                    gfh_header.brom_sec_cfg = data;
                    break;
                default:
                    this.log(`not handled for ${hdr.type}`);
            }
        }
        console.log(gfh_header);
    }

    /**
     * ANDRIOD ROM INFO FORMAT
     * see https://android.googlesource.com/kernel/mediatek/+/android-mediatek-sprout-3.4-kitkat-mr2/drivers/misc/mediatek/masp/mt6582/asf/asf_inc/sec_rom_info.h
     */
    parse_android_rominfo() {
        let offset = this.data.indexOf(ANDROID_ROMINFO_MAGIC);
        this.log(`found ${ANDROID_ROMINFO_MAGIC} at offset: ${offset}`);
        // exit if not found
        if (offset === -1) { return }

        // parse structure
        let buf = this.data.subarray(offset, offset + AND_ROM_INFO_SIZE);
        let hdr = utils.parseBuf(buf, gfh_common_header);
        this.log(`header type: ${hdr.type} size: ${hdr.size}`);

        // parse main        
        let rom_info = utils.parseBuf(buf, AND_ROMINFO_T);

        // parse sub structures, overwrite the data
        // SEC_CTRL
        offset = buf.indexOf(ROM_INFO_SEC_CTRL_ID, 1);
        let tmpBuf = buf.subarray(offset, offset + AND_SEC_CTRL_SIZE);
        rom_info.m_SEC_CTRL = utils.parseBuf(tmpBuf, AND_SECCTRL_T);

        // SEC_KEY
        offset = buf.indexOf(ROM_INFO_SEC_KEY_ID, 1);
        tmpBuf = buf.subarray(offset, offset + AND_SEC_KEY_SIZE);
        console.log(offset);
        rom_info.m_SEC_KEY = utils.parseBuf(tmpBuf, AND_SECKEY_T);

        // SEC_BOOT_CHECK_PART, 90 bytes, array of 9 x 10 bytes
        // https://android.googlesource.com/kernel/mediatek/+/android-mediatek-sprout-3.4-kitkat-mr2/drivers/misc/mediatek/masp/mt6582/asf/asf_inc/sec_boot.h
        let len = 10;
        let res = [];
        buf = Buffer.from(rom_info.m_SEC_BOOT_CHECK_PART);
        for (let i = 0; i < 9; i++) {
            res.push(buf.subarray(i * len, (i + 1) * len).toString())
        }
        rom_info.m_SEC_BOOT_CHECK_PART = res;
        console.log(rom_info);
    }

    parse_brom_layout_header() {
        let offset = this.data.indexOf(NAND_BOOT_NAME);        
        this.log(`found ${NAND_BOOT_NAME} at offset: ${offset}`);
        // exit if not found
        if (offset === -1) { return }

        // parse structure, 23 = BOOTLOADER! + ...
        // see also https://github.com/ni/u-boot/blob/master/tools/mtk_image.c

        let buf = this.data.subarray(offset + 23, offset + 23 + utils.structSize(brom_layout_header));
        let hdr = utils.parseBuf(buf, gfh_common_header);
        //this.log(`header type: ${hdr.type} size: ${hdr.size}`);
        console.log(hdr);
    }

    // bloader
    // see 
    parseBloader() {
        // header id
        let ID = "MTK_BLOADER_INFO";
        // find start
        let offset = this.data.indexOf(ID);
        this.log(`found header at ${offset}`);

        let map = {
            header: 27,
            preBin: 61,
            hex_1: 4,
            hex_2: 4,
            hex_3: 4,
            mtk_bin: 8,
            total_custem_chips: 4
        }
        let size = utils.structSize(map);
        let buf = this.data.subarray(offset, offset + size);
        let hdr = utils.parseBuf(buf, map);

        console.log(hdr);

        // chip_map?

        let chip_hdr = {
            sub_version: 4,
            type: 4,
            emmc_id_len: 4,
            fw_id_len: 4,
            emmc_id: 16,
            fw_id: 8,
            emi_cona_val: 4,
            dramc_drvctl0_val: 4,
            dramc_drvctl1_val: 4,
            dramc_actim_val: 4,
            dramc_gddr3ctl1_val: 4,
            dramc_conf1_val: 4,
            dramc_ddr2ctl_val: 4,
            dramc_test2_3_val: 4,
            dramc_conf2_val: 4,
            dramc_pd_ctrl_val: 4,
            dramc_padctl3_val: 4,
            dramc_dqodly_val: 4,
            dramc_addr_output_dly: 4,
            dramc_clk_output_dly: 4,
            dramc_actim1_val: 4,
            dramc_misctl0_val: 4,
            dramc_actim05t_val: 4,
            dram_rank_size: 16,
            reserved: 40,
            lpddr3_mode_reg1: 4,
            lpddr3_mode_reg2: 4,
            lpddr3_mode_reg3: 4,
            lpddr3_mode_reg5: 4,
            lpddr3_mode_reg10: 4,
            lpddr3_mode_reg63: 4,
        }
        hdr.chips = [];
        let chipSize = utils.structSize(chip_hdr);
        console.log(chipSize);
        offset = offset + size;
        for (let i = 0; i < hdr.total_custem_chips; i++) {
            let tmpBuf = this.data.subarray(offset + i * chipSize, offset + (i + 1) * chipSize);
            console.log(tmpBuf);
            hdr.chips.push(utils.parseBuf(tmpBuf, chip_hdr));
        }
        console.log(hdr);
    }

    /**
     * close the file
     */
    close() {
        utils.closeFile();
    }

    /**
     * logging, msg to console
     * @param {*} msg 
     */
    log(msg) {
        console.log(`[Bootrom] ${msg}`);
    }

}
// 
/*
export const ubi_ec_hdr = {
    magic: 4,
    version: 1,
    padding1: 3,
     vid_hdr_offset: 4,
    data_offset: 4,
    image_seq: 4,
    padding2: 32,
    hdr_crc: 4,
}
*/

// --- Execution
let filename = '/home/joost/Projects/gitea/echodotv3/flashdump/exported/partitions/brhgptpl_0.bin';
let bf = new Bootfile(filename);

bf.parse_gfh_header();
bf.parse_android_rominfo();
bf.parse_brom_layout_header();
//bf.parseBloader();

bf.close()