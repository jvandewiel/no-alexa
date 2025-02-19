notes.MD_INFO

We have a preloader that we boot into after the bootrom

From https://github.com/keesj/alps-fairphone-gpl/blob/master/mediatek/platform/mt6589/preloader/src/core/download.c
when booting handshake is: a0 0a 50 05

```c
#define CMD_GET_HW_SW_VER          0xfc
#define CMD_GET_HW_CODE            0xfd
#define CMD_GET_BL_VER             0xfe

#define CMD_LEGACY_WRITE           0xa1
#define CMD_LEGACY_READ            0xa2

#define CMD_I2C_INIT               0xB0
#define CMD_I2C_DEINIT             0xB1
#define CMD_I2C_WRITE8             0xB2
#define CMD_I2C_READ8              0xB3
#define CMD_I2C_SET_SPEED          0xB4

#define CMD_PWR_INIT               0xC4
#define CMD_PWR_DEINIT             0xC5
#define CMD_PWR_READ16             0xC6
#define CMD_PWR_WRITE16            0xC7          

#define CMD_READ16                 0xD0
#define CMD_READ32                 0xD1
#define CMD_WRITE16                0xD2
#define CMD_WRITE16_NO_ECHO        0xD3
#define CMD_WRITE32                0xD4
#define CMD_JUMP_DA                0xD5
#define CMD_JUMP_BL                0xD6
#define CMD_SEND_DA                0xD7
#define CMD_GET_TARGET_CONFIG      0xD8
#define CMD_UART1_LOG_EN           0xDB

#define TGT_CFG_SBC_EN             0x00000001
#define TGT_CFG_SLA_EN             0x00000002
#define TGT_CFG_DAA_EN             0x00000004

#define DA_ARG_MAGIC               0x58885168
#define DA_ARG_VER                 1

#define DA_FLAG_SKIP_PLL_INIT      0x00000001
#define DA_FLAG_SKIP_EMI_INIT      0x00000002

#if CFG_I2C_SUPPORT
#define BROM_I2C_CMD_LEN           32
#define BROM_I2C_DATA_LEN          32
#define B_OK                       0
#define E_I2C_CMD_BUF_OVERFLOW     0x1D1E
#define E_I2C_DATA_BUF_OVERFLOW    0x1D1F
#define E_I2C_SPEED_MODE_INVALID   0x1D20
static u8 g_I2C_CmdBuf[BROM_I2C_CMD_LEN];
static u8 g_I2C_DataBuf[BROM_I2C_DATA_LEN]
```

0x21847-0x6200

=== 
from https://github.com/ni/u-boot/blob/master/tools/mtk_image.h

#define NAND_BOOT_NAME		"BOOTLOADER!"
#define NAND_BOOT_VERSION	"V006"
#define NAND_BOOT_ID		"NFIINFO"

/* BootROM layout header */
struct brom_layout_header {
	char name[8]; 
	__le32 version; 4
	__le32 header_size; 4
	__le32 total_size; 4
	__le32 magic; 4
	__le32 type; 4
	__le32 header_size_2; 4 
	__le32 total_size_2; 4
	__le32 unused; 4
};

