// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2014-2015, Antmicro Ltd <www.antmicro.com>
 * Copyright (c) 2015, AW-SOM Technologies <www.aw-som.com>
 */

#include <asm/arch/clock.h>
#include <asm/io.h>
#include <common.h>
#include <config.h>
#include <nand.h>
#include <linux/bitops.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/mtd/rawnand.h>

/* registers */
#define NFC_CTL                    0x00000000
#define NFC_ST                     0x00000004
#define NFC_INT                    0x00000008
#define NFC_TIMING_CTL             0x0000000C
#define NFC_TIMING_CFG             0x00000010
#define NFC_ADDR_LOW               0x00000014
#define NFC_ADDR_HIGH              0x00000018
#define NFC_SECTOR_NUM             0x0000001C
#define NFC_CNT                    0x00000020
#define NFC_CMD                    0x00000024
#define NFC_RCMD_SET               0x00000028
#define NFC_WCMD_SET               0x0000002C
#define NFC_IO_DATA                0x00000030
#define NFC_ECC_CTL                0x00000034
#define NFC_ECC_ST                 0x00000038
#define NFC_DEBUG                  0x0000003C
#define NFC_ECC_CNT0               0x00000040
#define NFC_ECC_CNT1               0x00000044
#define NFC_ECC_CNT2               0x00000048
#define NFC_ECC_CNT3               0x0000004C
#define NFC_USER_DATA_BASE         0x00000050
#define NFC_EFNAND_STATUS          0x00000090
#define NFC_SPARE_AREA             0x000000A0
#define NFC_PATTERN_ID             0x000000A4
#define NFC_RAM0_BASE              0x00000400
#define NFC_RAM1_BASE              0x00000800

#define NFC_CTL_EN                 (1 << 0)
#define NFC_CTL_RESET              (1 << 1)
#define NFC_CTL_RAM_METHOD         (1 << 14)
#define NFC_CTL_PAGE_SIZE_MASK     (0xf << 8)
#define NFC_CTL_PAGE_SIZE(a)       ((fls(a) - 11) << 8)


#define NFC_ECC_EN                 (1 << 0)
#define NFC_ECC_PIPELINE           (1 << 3)
#define NFC_ECC_EXCEPTION          (1 << 4)
#define NFC_ECC_BLOCK_SIZE         (1 << 5)
#define NFC_ECC_RANDOM_EN          (1 << 9)
#define NFC_ECC_RANDOM_DIRECTION   (1 << 10)


#define NFC_ADDR_NUM_OFFSET        16
#define NFC_SEND_ADDR              (1 << 19)
#define NFC_ACCESS_DIR             (1 << 20)
#define NFC_DATA_TRANS             (1 << 21)
#define NFC_SEND_CMD1              (1 << 22)
#define NFC_WAIT_FLAG              (1 << 23)
#define NFC_SEND_CMD2              (1 << 24)
#define NFC_SEQ                    (1 << 25)
#define NFC_DATA_SWAP_METHOD       (1 << 26)
#define NFC_ROW_AUTO_INC           (1 << 27)
#define NFC_SEND_CMD3              (1 << 28)
#define NFC_SEND_CMD4              (1 << 29)
#define NFC_RAW_CMD                (0 << 30)
#define NFC_ECC_CMD                (1 << 30)
#define NFC_PAGE_CMD               (2 << 30)

#define NFC_ST_CMD_INT_FLAG        (1 << 1)
#define NFC_ST_DMA_INT_FLAG        (1 << 2)
#define NFC_ST_CMD_FIFO_STAT       (1 << 3)

#define NFC_READ_CMD_OFFSET         0
#define NFC_RANDOM_READ_CMD0_OFFSET 8
#define NFC_RANDOM_READ_CMD1_OFFSET 16

#define NFC_CMD_RNDOUTSTART        0xE0
#define NFC_CMD_RNDOUT             0x05
#define NFC_CMD_READSTART          0x30






#define NFC_REG_CTL		0x0000
#define NFC_REG_ST		0x0004
#define NFC_REG_INT		0x0008
#define NFC_REG_TIMING_CTL	0x000C
#define NFC_REG_TIMING_CFG	0x0010
#define NFC_REG_ADDR_LOW	0x0014
#define NFC_REG_ADDR_HIGH	0x0018
#define NFC_REG_SECTOR_NUM	0x001C
#define NFC_REG_CNT		0x0020
#define NFC_REG_CMD		0x0024
#define NFC_REG_RCMD_SET	0x0028
#define NFC_REG_WCMD_SET	0x002C
#define NFC_REG_IO_DATA		0x0030
#define NFC_REG_ECC_CTL		0x0034
#define NFC_REG_ECC_ST		0x0038
#define NFC_REG_DEBUG		0x003C
#define NFC_REG_ECC_ERR_CNT(x)	((0x0040 + (x)) & ~0x3)
#define NFC_REG_USER_DATA(x)	(0x0050 + ((x) * 4))
#define NFC_REG_SPARE_AREA	0x00A0
#define NFC_REG_PAT_ID		0x00A4
//#define NFC_RAM0_BASE		0x0400
//#define NFC_RAM1_BASE		0x0800

/* define bit use in NFC_CTL */
#define NFC_EN			BIT(0)
#define NFC_RESET		BIT(1)
#define NFC_BUS_WIDTH_MSK	BIT(2)
#define NFC_BUS_WIDTH_8		(0 << 2)
#define NFC_BUS_WIDTH_16	(1 << 2)
#define NFC_RB_SEL_MSK		BIT(3)
#define NFC_RB_SEL(x)		((x) << 3)
#define NFC_CE_SEL_MSK		(0x7 << 24)
#define NFC_CE_SEL(x)		((x) << 24)
#define NFC_CE_CTL		BIT(6)
#define NFC_PAGE_SHIFT_MSK	(0xf << 8)
#define NFC_PAGE_SHIFT(x)	(((x) < 10 ? 0 : (x) - 10) << 8)
#define NFC_SAM			BIT(12)
#define NFC_RAM_METHOD		BIT(14)
#define NFC_DEBUG_CTL		BIT(31)

/* define bit use in NFC_ST */
#define NFC_RB_B2R		BIT(0)
#define NFC_CMD_INT_FLAG	BIT(1)
#define NFC_DMA_INT_FLAG	BIT(2)
#define NFC_CMD_FIFO_STATUS	BIT(3)
#define NFC_STA			BIT(4)
#define NFC_NATCH_INT_FLAG	BIT(5)
#define NFC_RB_STATE(x)		BIT(x + 8)

/* define bit use in NFC_INT */
#define NFC_B2R_INT_ENABLE	BIT(0)
#define NFC_CMD_INT_ENABLE	BIT(1)
#define NFC_DMA_INT_ENABLE	BIT(2)
#define NFC_INT_MASK		(NFC_B2R_INT_ENABLE | \
				 NFC_CMD_INT_ENABLE | \
				 NFC_DMA_INT_ENABLE)

/* define bit use in NFC_TIMING_CTL */
#define NFC_TIMING_CTL_EDO	BIT(8)

/* define NFC_TIMING_CFG register layout */
/*
#define NFC_TIMING_CFG(tWB, tADL, tWHR, tRHW, tCAD)		\
	(((tWB) & 0x3) | (((tADL) & 0x3) << 2) |		\
	(((tWHR) & 0x3) << 4) | (((tRHW) & 0x3) << 6) |		\
	(((tCAD) & 0x7) << 8))
*/
/* define bit use in NFC_CMD */
#define NFC_CMD_LOW_BYTE_MSK	0xff
#define NFC_CMD_HIGH_BYTE_MSK	(0xff << 8)
//#define NFC_CMD(x)		(x)
#define NFC_ADR_NUM_MSK		(0x7 << 16)
#define NFC_ADR_NUM(x)		(((x) - 1) << 16)
#define NFC_SEND_ADR		BIT(19)
/*
#define NFC_ACCESS_DIR		BIT(20)
#define NFC_DATA_TRANS		BIT(21)
#define NFC_SEND_CMD1		BIT(22)
#define NFC_WAIT_FLAG		BIT(23)
#define NFC_SEND_CMD2		BIT(24)
#define NFC_SEQ			BIT(25)
#define NFC_DATA_SWAP_METHOD	BIT(26)
#define NFC_ROW_AUTO_INC	BIT(27)
#define NFC_SEND_CMD3		BIT(28)
#define NFC_SEND_CMD4		BIT(29)
#define NFC_CMD_TYPE_MSK	(0x3 << 30)
#define NFC_NORMAL_OP		(0 << 30)
#define NFC_ECC_OP		(1 << 30)
#define NFC_PAGE_OP		(2 << 30)
*/
/* define bit use in NFC_RCMD_SET */
#define NFC_READ_CMD_MSK	0xff
#define NFC_RND_READ_CMD0_MSK	(0xff << 8)
#define NFC_RND_READ_CMD1_MSK	(0xff << 16)

/* define bit use in NFC_WCMD_SET */
#define NFC_PROGRAM_CMD_MSK	0xff
#define NFC_RND_WRITE_CMD_MSK	(0xff << 8)
#define NFC_READ_CMD0_MSK	(0xff << 16)
#define NFC_READ_CMD1_MSK	(0xff << 24)

/* define bit use in NFC_ECC_CTL */
#define NFC_ECC_EN		BIT(0)
#define NFC_ECC_PIPELINE	BIT(3)
#define NFC_ECC_EXCEPTION	BIT(4)
#define NFC_ECC_BLOCK_SIZE_MSK	BIT(5)
#define NFC_ECC_BLOCK_512	(1 << 5)
#define NFC_RANDOM_EN		BIT(9)
#define NFC_RANDOM_DIRECTION	BIT(10)
#define NFC_ECC_MODE_MSK	(0xf << 12)
#define NFC_ECC_MODE(x)		((x) << 12)
#define NFC_RANDOM_SEED_MSK	(0x7fff << 16)
#define NFC_RANDOM_SEED(x)	((x) << 16)

/* define bit use in NFC_ECC_ST */
#define NFC_ECC_ERR(x)		BIT(x)
#define NFC_ECC_PAT_FOUND(x)	BIT(x + 16)
#define NFC_ECC_ERR_CNT(b, x)	(((x) >> ((b) * 8)) & 0xff)

#define NFC_DEFAULT_TIMEOUT_MS	1000

#define NFC_SRAM_SIZE		1024

#define NFC_MAX_CS		7





struct nfc_config {
	int page_size;
	int ecc_strength;
	int ecc_size;
	int addr_cycles;
	int nseeds;
	bool randomize;
	bool valid;
};

/* minimal "boot0" style NAND support for Allwinner A20 */

/* random seed used by linux */
const uint16_t random_seed[128] = {
	0x2b75, 0x0bd0, 0x5ca3, 0x62d1, 0x1c93, 0x07e9, 0x2162, 0x3a72,
	0x0d67, 0x67f9, 0x1be7, 0x077d, 0x032f, 0x0dac, 0x2716, 0x2436,
	0x7922, 0x1510, 0x3860, 0x5287, 0x480f, 0x4252, 0x1789, 0x5a2d,
	0x2a49, 0x5e10, 0x437f, 0x4b4e, 0x2f45, 0x216e, 0x5cb7, 0x7130,
	0x2a3f, 0x60e4, 0x4dc9, 0x0ef0, 0x0f52, 0x1bb9, 0x6211, 0x7a56,
	0x226d, 0x4ea7, 0x6f36, 0x3692, 0x38bf, 0x0c62, 0x05eb, 0x4c55,
	0x60f4, 0x728c, 0x3b6f, 0x2037, 0x7f69, 0x0936, 0x651a, 0x4ceb,
	0x6218, 0x79f3, 0x383f, 0x18d9, 0x4f05, 0x5c82, 0x2912, 0x6f17,
	0x6856, 0x5938, 0x1007, 0x61ab, 0x3e7f, 0x57c2, 0x542f, 0x4f62,
	0x7454, 0x2eac, 0x7739, 0x42d4, 0x2f90, 0x435a, 0x2e52, 0x2064,
	0x637c, 0x66ad, 0x2c90, 0x0bad, 0x759c, 0x0029, 0x0986, 0x7126,
	0x1ca7, 0x1605, 0x386a, 0x27f5, 0x1380, 0x6d75, 0x24c3, 0x0f8e,
	0x2b7a, 0x1418, 0x1fd1, 0x7dc1, 0x2d8e, 0x43af, 0x2267, 0x7da3,
	0x4e3d, 0x1338, 0x50db, 0x454d, 0x764d, 0x40a3, 0x42e6, 0x262b,
	0x2d2e, 0x1aea, 0x2e17, 0x173d, 0x3a6e, 0x71bf, 0x25f9, 0x0a5d,
	0x7c57, 0x0fbe, 0x46ce, 0x4939, 0x6b17, 0x37bb, 0x3e91, 0x76db,
};

#define DEFAULT_TIMEOUT_US	100000


static int check_value_inner(int offset, int expected_bits,
			     int timeout_us, int negation)
{
	do {
		int val = readl(offset) & expected_bits;
		if (negation ? !val : val)
			return 1;
		udelay(1);
	} while (--timeout_us);

	return 0;
}


static inline int check_value_negated(int offset, int unexpected_bits,
				      int timeout_us)
{
	return check_value_inner(offset, unexpected_bits, timeout_us, 1);
}

static inline int check_value(int offset, int expected_bits,
			      int timeout_us)
{
	return check_value_inner(offset, expected_bits, timeout_us, 0);
}

static int nand_wait_int(void)
{
	if (!check_value(SUNXI_NFC_BASE + NFC_ST, NFC_ST_CMD_INT_FLAG,
			 DEFAULT_TIMEOUT_US)) {
		printf("nand: timeout waiting for interruption\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static int nand_wait_cmd_fifo_empty(void)
{
	if (!check_value_negated(SUNXI_NFC_BASE + NFC_REG_ST, NFC_ST_CMD_FIFO_STAT,
				 DEFAULT_TIMEOUT_US)) {
		printf("nand: timeout waiting for empty cmd FIFO\n");
		return -ETIMEDOUT;
	}

	return 0;
}



void nfc_repeat_mode_enable(void)
{
    __u32 reg_val;

    reg_val = readl(SUNXI_NFC_BASE + NFC_REG_CTL);
	if(((reg_val>>18)&0x3)>1)   //ddr type
	{
    	reg_val |= 0x1<<20;
        writel(reg_val, SUNXI_NFC_BASE + NFC_REG_CTL);
    }

}

void nfc_repeat_mode_disable(void)
{
    __u32 reg_val;

    reg_val = readl(SUNXI_NFC_BASE + NFC_REG_CTL);
	if(((reg_val>>18)&0x3)>1)   //ddr type
	{
    	reg_val &= (~(0x1<<20));
        writel(reg_val, SUNXI_NFC_BASE + NFC_REG_CTL);
    }
}



static int nand_exec_cmd(u32 cmd)
{
	int ret;

	ret = nand_wait_cmd_fifo_empty();
	if (ret)
	{
		mydebug("%s,%d,%s: failed.\r\n",__FILE__, __LINE__, __FUNCTION__);
		return ret;
	}
	unsigned int ctrl = NAND_CTRL_CLE | NAND_CTRL_CHANGE;
	u32 tmp = readl(SUNXI_NFC_BASE + NFC_REG_CTL);
	if(ctrl & NAND_NCE)
	{
		tmp |= NFC_CE_CTL;
	}
	else
	{
		tmp &= ~NFC_CE_CTL;
	}
	writel(tmp, SUNXI_NFC_BASE + NFC_REG_CTL);

	writel(NFC_ST_CMD_INT_FLAG, SUNXI_NFC_BASE + NFC_ST);
	writel(cmd, SUNXI_NFC_BASE + NFC_CMD);

	return nand_wait_int();
}

/*
static int nand_exec_cmd(u32 cmd)
{
	int ret;

	ret = nand_wait_cmd_fifo_empty();
	if (ret)
		return ret;

	writel(NFC_ST_CMD_INT_FLAG, SUNXI_NFC_BASE + NFC_REG_ST);
    // printf("nand cmd: %x [%x]\n", cmd, SUNXI_NFC_BASE + NFC_REG_CMD);
	writel(cmd, SUNXI_NFC_BASE + NFC_REG_CMD);

	return nand_wait_int();
}
*/

int nfc_set_id(void)
{
	__u32 cfg;

    cfg = 0;
    /*set addr*/
	writel(cfg, SUNXI_NFC_BASE + NFC_REG_ADDR_LOW);
	writel(cfg, SUNXI_NFC_BASE + NFC_REG_ADDR_HIGH);

    writel((readl(SUNXI_NFC_BASE + NFC_REG_CTL)) & (~NFC_CTL_RAM_METHOD),
           SUNXI_NFC_BASE + NFC_REG_CTL);
    /*set sequence mode*/
    //cfg |= 0x1<<25;
	writel(6, SUNXI_NFC_BASE + NFC_REG_CNT);

	nand_exec_cmd(NFC_SEND_CMD1 | NFC_SEND_ADDR | NFC_DATA_TRANS | 0x90);

    return 0;
}

int nfc_get_id(char *idbuf)
{
	int ret, i;

    nfc_repeat_mode_enable();
	ret = nfc_set_id();
	if (ret){
		return ret;
	}

	ret = nand_wait_cmd_fifo_empty();
	ret |= nand_wait_int();

	/*get 6 bytes id value*/
    /* Retrieve the data from SRAM */
    // memcpy(idbuf, (void *)(SUNXI_NFC_BASE + NFC_RAM0_BASE),
    //             6);
    printf("nand id [%x]: ", SUNXI_NFC_BASE + NFC_RAM0_BASE);
	for (i = 0; i < 6; i++){
        if(idbuf)
        {
            // *(idbuf + i) = NFC_READ_RAM_B(NFC_RAM0_BASE+i);
            *(idbuf + i) = readb(SUNXI_NFC_BASE + NFC_RAM0_BASE + i);
        }
        printf("%x ", readb(SUNXI_NFC_BASE + NFC_RAM0_BASE + i));
	}
    printf("\n");
    nfc_repeat_mode_disable();

	return ret;
}



static void dump_reg()
{
	for(int i=0;i<20;i++)
	{
		int val = readl(SUNXI_NFC_BASE+4*i);
		mydebug("0x%08x ",val);
		if((i+1)%8 == 0)
		{
			mydebug("\r\n");
		}
	}
	mydebug("\r\n");
}


/*
static uint8_t sunxi_nfc_read_byte()
{

	uint8_t data=0x00;
	int ret = nand_wait_cmd_fifo_empty();
	if(ret)
	{
		mydebug("%s,%d,%s: failed.\r\n",__FILE__, __LINE__, __FUNCTION__);
		return 1;
	}
	writel(1,SUNXI_NFC_BASE + NFC_REG_CNT);
	u32 tmp = NFC_DATA_TRANS | NFC_DATA_SWAP_METHOD;
	writel(tmp, SUNXI_NFC_BASE + NFC_REG_CMD);

	ret = nand_wait_int();

	if(ret)
	{
		mydebug("%s,%d,%s: failed.\r\n",__FILE__, __LINE__, __FUNCTION__);
		return 2;
	}

	memcpy_fromio(&data,SUNXI_NFC_BASE + NFC_RAM0_BASE,1);

	return data;
}
*/
/*
static int nand_readid_op_spl(void *buf,unsigned int len)
{
	u8 *id = buf;
	nand_exec_cmd(NAND_CMD_READID);
	for(unsigned int i=0; i<len; i++)
	{
		id[i] = sunxi_nfc_read_byte();
	}
	return 0;
}
*/


#define NFC_CE_SEL				(7 << 24)
int  nand_get_id()
{
	int ret;
	__u8 i = 0;
	__u32 cfg;
    char idbuf[6] = {0};

	cfg = readl(SUNXI_NFC_BASE + NFC_REG_CTL);
    cfg &= ((~NFC_CE_SEL) & 0xffffffff);
    cfg |= (0 << 24);
	writel(cfg, SUNXI_NFC_BASE + NFC_REG_CTL);

	ret = nfc_get_id(idbuf);
    printf("nand id : ");
	for (i = 0; i < 6; i++){
        printf("%x ", idbuf[i]);
	}
    printf("\n");

	return ret;
}

void nand_init(void)
{
	dump_reg();
	uint32_t val;
	mydebug("%s,%d,%s\r\n",__FILE__,__LINE__,__FUNCTION__);
	board_nand_init();
	writel(0x700,SUNXI_NFC_BASE + NFC_TIMING_CFG);
	dump_reg();
	//如何执行,spl没执行部分的内容(sunxi_nand_init)
	//sunxi_nand_init:
	writel(0, SUNXI_NFC_BASE + NFC_ECC_CTL);
	//writel(NFC_CTL_RESET, SUNXI_NFC_BASE + NFC_CTL);


	val = readl(SUNXI_NFC_BASE + NFC_CTL);
	/* enable and reset CTL */
	writel(val | NFC_CTL_EN | NFC_CTL_RESET,
	       SUNXI_NFC_BASE + NFC_CTL);

	if (!check_value_negated(SUNXI_NFC_BASE + NFC_CTL,
				 NFC_CTL_RESET, DEFAULT_TIMEOUT_US)) {
		printf("Couldn't initialize nand\n");
	}
	dump_reg();
	/* reset NAND */
	nand_exec_cmd(NFC_SEND_CMD1 | NFC_WAIT_FLAG | NAND_CMD_RESET);

	nand_exec_cmd(NAND_CMD_RESET);

	//sunxi_nand_chips_init->sunxi_nand_ecc_init

	nand_get_id();

	dump_reg();
	//读取id信息
	u8 id_data[8]={0};

	nand_exec_cmd(NAND_CMD_READID);
	int ret = nand_wait_cmd_fifo_empty();
	if(ret)
	{
		mydebug("%s,%d,%s: failed.\r\n",__FILE__, __LINE__, __FUNCTION__);
		return;
	}
	dump_reg();
	writel(8,SUNXI_NFC_BASE + NFC_REG_CNT);
	u32 tmp = NFC_DATA_TRANS | NFC_DATA_SWAP_METHOD;
	writel(tmp, SUNXI_NFC_BASE + NFC_REG_CMD);
	dump_reg();
	ret = nand_wait_int();
	dump_reg();
	if(ret)
	{
		mydebug("%s,%d,%s: failed.\r\n",__FILE__, __LINE__, __FUNCTION__);
		return ;
	}
	dump_reg();
	memcpy_fromio(id_data,SUNXI_NFC_BASE + NFC_RAM0_BASE,8);
	dump_reg();
	for(int i=0;i<8;i++)
	{
		mydebug("%02x ",id_data[i]);
	}
	mydebug("\r\n");


}

static void nand_apply_config(const struct nfc_config *conf)
{
	u32 val;

	nand_wait_cmd_fifo_empty();

	val = readl(SUNXI_NFC_BASE + NFC_CTL);
	val &= ~NFC_CTL_PAGE_SIZE_MASK;
	writel(val | NFC_CTL_RAM_METHOD | NFC_CTL_PAGE_SIZE(conf->page_size),
	       SUNXI_NFC_BASE + NFC_CTL);
	writel(conf->ecc_size, SUNXI_NFC_BASE + NFC_CNT);
	writel(conf->page_size, SUNXI_NFC_BASE + NFC_SPARE_AREA);
}

static int nand_load_page(const struct nfc_config *conf, u32 offs)
{
	int page = offs / conf->page_size;

	writel((NFC_CMD_RNDOUTSTART << NFC_RANDOM_READ_CMD1_OFFSET) |
	       (NFC_CMD_RNDOUT << NFC_RANDOM_READ_CMD0_OFFSET) |
	       (NFC_CMD_READSTART << NFC_READ_CMD_OFFSET),
	       SUNXI_NFC_BASE + NFC_RCMD_SET);
	writel(((page & 0xFFFF) << 16), SUNXI_NFC_BASE + NFC_ADDR_LOW);
	writel((page >> 16) & 0xFF, SUNXI_NFC_BASE + NFC_ADDR_HIGH);

	return nand_exec_cmd(NFC_SEND_CMD1 | NFC_SEND_CMD2 | NFC_RAW_CMD |
			     NFC_SEND_ADDR | NFC_WAIT_FLAG |
			     ((conf->addr_cycles - 1) << NFC_ADDR_NUM_OFFSET));
}

static int nand_change_column(u16 column)
{
	int ret;

	writel((NFC_CMD_RNDOUTSTART << NFC_RANDOM_READ_CMD1_OFFSET) |
	       (NFC_CMD_RNDOUT << NFC_RANDOM_READ_CMD0_OFFSET) |
	       (NFC_CMD_RNDOUTSTART << NFC_READ_CMD_OFFSET),
	       SUNXI_NFC_BASE + NFC_RCMD_SET);
	writel(column, SUNXI_NFC_BASE + NFC_ADDR_LOW);

	ret = nand_exec_cmd(NFC_SEND_CMD1 | NFC_SEND_CMD2 | NFC_RAW_CMD |
			    (1 << NFC_ADDR_NUM_OFFSET) | NFC_SEND_ADDR |
			    NFC_CMD_RNDOUT);
	if (ret)
		return ret;

	/* Ensure tCCS has passed before reading data */
	udelay(1);

	return 0;
}

static const int ecc_bytes[] = {32, 46, 54, 60, 74, 88, 102, 110, 116};

static int sunxi_nfc_wait_int(u32 flags,
			      unsigned int timeout_ms)
{
	unsigned int timeout_ticks;
	u32 time_start, status;
	int ret = -ETIMEDOUT;

	if (!timeout_ms)
		timeout_ms = 1000;

	timeout_ticks = (timeout_ms * CONFIG_SYS_HZ) / 1000;

	time_start = get_timer(0);

	do {
		status = readl(SUNXI_NFC_BASE + NFC_ST);
		if ((status & flags) == flags) {
			ret = 0;
			break;
		}

		udelay(1);
	} while (get_timer(time_start) < timeout_ticks);

	writel(status & flags, SUNXI_NFC_BASE + NFC_ST);

	return ret;
}
static int nand_read_page(const struct nfc_config *conf, u32 offs,
			  void *dest, int len)
{
	//mydebug("nand_read_page para:0x%08x,0x%08p,%d\r\n",offs,dest,len);
	//mydebug("nand_read_page conf:%d,%d,%d,%d,%d,%d,%d\r\n",
	//	conf->page_size,conf->ecc_strength,conf->ecc_size,conf->addr_cycles,conf->nseeds,conf->randomize,conf->valid);
	int nsectors = len / conf->ecc_size;
	u16 rand_seed = 0;
	int oob_chunk_sz = ecc_bytes[conf->ecc_strength];
	int page = offs / conf->page_size;
	u32 ecc_st;
	int i,ret;
	//mydebug("sector:%d,oobsize:%d,page:%d\r\n",nsectors,oob_chunk_sz,page);

	if (offs % conf->page_size || len % conf->ecc_size ||
	    len > conf->page_size || len < 0)
		{
			mydebug("nand_read_page:-EINVAL\r\n");
			return -EINVAL;
		}

	/* Choose correct seed if randomized */
	if (conf->randomize)
		rand_seed = random_seed[page % conf->nseeds];

	/* Retrieve data from SRAM (PIO) */
	for (i = 0; i < nsectors; i++) {
		int data_off = i * conf->ecc_size;
		int oob_off = conf->page_size + (i * oob_chunk_sz);
		u8 *data = dest + data_off;

		/* Clear ECC status and restart ECC engine */
		//这里要注意一下
		writel(0, SUNXI_NFC_BASE + NFC_ECC_ST);
		writel((rand_seed << 16) | (conf->ecc_strength << 12) |
		       (conf->randomize ? NFC_ECC_RANDOM_EN : 0) |
		       (conf->ecc_size == 512 ? NFC_ECC_BLOCK_SIZE : 0) |
		       NFC_ECC_EN | NFC_ECC_EXCEPTION,
		       SUNXI_NFC_BASE + NFC_ECC_CTL);

		/* Move the data in SRAM */
		nand_change_column(data_off);
		writel(conf->ecc_size, SUNXI_NFC_BASE + NFC_CNT);
		nand_exec_cmd(NFC_DATA_TRANS);

		
		u8 debugdata1[16]={0xff};
		memcpy_fromio(debugdata1, SUNXI_NFC_BASE + NFC_RAM0_BASE,
			      16);
		for(int si=0;si<16;si++)
		{
			mydebug("%02x ",debugdata1[si]);
		}
		mydebug("\r\n");
		

		/*
		 * Let the ECC engine consume the ECC bytes and possibly correct
		 * the data.
		 */
		nand_change_column(oob_off);
		nand_exec_cmd(NFC_DATA_TRANS | NFC_ECC_CMD);

		ret = sunxi_nfc_wait_int(NFC_ST_CMD_INT_FLAG,0);
		if(ret)
		{
			mydebug("sunxi_nfc_wait_int timeout\r\n");
			return ret;
		}

		


		/* Get the ECC status */
		ecc_st = readl(SUNXI_NFC_BASE + NFC_ECC_ST);

		/* ECC error detected. */
		if (ecc_st & 0xffff)
		{
			mydebug("nand_read_page:-EIO\r\n");
			return -EIO;
		}

		/* 这里读出来的也是空,怪不得认为是空flash
		 * Return 1 if the first chunk is empty (needed for
		 * configuration detection).
		 */
		if (!i && (ecc_st & 0x10000))
		{
			int pattern = readl(SUNXI_NFC_BASE + NFC_PATTERN_ID);
			mydebug("nand_read_page:%d,0x%08x,patten:0x%08x\r\n",i,ecc_st,pattern);
			return -EIO;
			//return 1;
		}

		/* Retrieve the data from SRAM */
		memcpy_fromio(data, SUNXI_NFC_BASE + NFC_RAM0_BASE,
			      conf->ecc_size);

		//TODO:这里读取到的数据,为什么是全0?
		/*
		u8 debugdata[16]={0xff};
		memcpy_fromio(debugdata, SUNXI_NFC_BASE + NFC_RAM0_BASE,
			      16);
		for(int si=0;si<16;si++)
		{
			mydebug("%02x ",debugdata[si]);
		}
		mydebug("\r\n");
		*/
		/* Stop the ECC engine */
		writel(readl(SUNXI_NFC_BASE + NFC_ECC_CTL) & ~NFC_ECC_EN,
		       SUNXI_NFC_BASE + NFC_ECC_CTL);

		if (data_off + conf->ecc_size >= len)
			break;
	}

	return 0;
}

static int nand_max_ecc_strength(struct nfc_config *conf)
{
	int max_oobsize, max_ecc_bytes;
	int nsectors = conf->page_size / conf->ecc_size;
	int i;

	/*
	 * ECC strength is limited by the size of the OOB area which is
	 * correlated with the page size.
	 */
	switch (conf->page_size) {
	case 2048:
		max_oobsize = 64;
		break;
	case 4096:
		max_oobsize = 256;
		break;
	case 8192:
		max_oobsize = 640;
		break;
	case 16384:
		max_oobsize = 1664;
		break;
	default:
		return -EINVAL;
	}

	max_ecc_bytes = max_oobsize / nsectors;

	for (i = 0; i < ARRAY_SIZE(ecc_bytes); i++) {
		if (ecc_bytes[i] > max_ecc_bytes)
			break;
	}

	if (!i)
		return -EINVAL;

	return i - 1;
}

static int nand_detect_ecc_config(struct nfc_config *conf, u32 offs,
				  void *dest)
{
	/* NAND with pages > 4k will likely require 1k sector size. */
	int min_ecc_size = conf->page_size > 4096 ? 1024 : 512;
	int page = offs / conf->page_size;
	int ret;
	mydebug("nand_detect_ecc_config:0x%08x,0x%08p,pagesize:%d\r\n",offs,dest,conf->page_size);
	/*
	 * In most cases, 1k sectors are preferred over 512b ones, start
	 * testing this config first.
	 */
	for (conf->ecc_size = 1024; conf->ecc_size >= min_ecc_size;
	     conf->ecc_size >>= 1) {
		int max_ecc_strength = nand_max_ecc_strength(conf);

		nand_apply_config(conf);

		/*
		 * We are starting from the maximum ECC strength because
		 * most of the time NAND vendors provide an OOB area that
		 * barely meets the ECC requirements.
		 */
		for (conf->ecc_strength = max_ecc_strength;
		     conf->ecc_strength >= 0;
		     conf->ecc_strength--) {
			conf->randomize = false;
			if (nand_change_column(0))
				return -EIO;

			/*
			 * Only read the first sector to speedup detection.
			 */
			ret = nand_read_page(conf, offs, dest, conf->ecc_size);
			if (!ret) {
				mydebug("nand_detect_ecc_config read page ok:0x%08x,0x%08p\r\n",offs,dest);
				return 0;
			} else if (ret > 0) {
				/*
				 * If page is empty we can't deduce anything
				 * about the ECC config => stop the detection.
				 */
				mydebug("nand_detect_ecc_config %d page empty?\r\n",ret);
				return -EINVAL;
			}

			conf->randomize = true;
			conf->nseeds = ARRAY_SIZE(random_seed);
			do {
				if (nand_change_column(0))
					return -EIO;

				if (!nand_read_page(conf, offs, dest,
						    conf->ecc_size))
					return 0;

				/*
				 * Find the next ->nseeds value that would
				 * change the randomizer seed for the page
				 * we're trying to read.
				 */
				while (conf->nseeds >= 16) {
					int seed = page % conf->nseeds;

					conf->nseeds >>= 1;
					if (seed != page % conf->nseeds)
						break;
				}
			} while (conf->nseeds >= 16);
		}
	}

	return -EINVAL;
}

static int nand_detect_config(struct nfc_config *conf, u32 offs, void *dest)
{
	if (conf->valid)
		return 0;

	/*
	 * Modern NANDs are more likely than legacy ones, so we start testing
	 * with 5 address cycles.
	 */
	for (conf->addr_cycles = 5;
	     conf->addr_cycles >= 4;
	     conf->addr_cycles--) {
		int max_page_size = conf->addr_cycles == 4 ? 2048 : 16384;

		/*
		 * Ignoring 1k pages cause I'm not even sure this case exist
		 * in the real world.
		 */
		for (conf->page_size = 2048; conf->page_size <= max_page_size;
		     conf->page_size <<= 1) {
			if (nand_load_page(conf, offs))
			{
				mydebug("nand_load_page failed.\r\n");
				return -1;
			}

			if (!nand_detect_ecc_config(conf, offs, dest)) {
				conf->valid = true;
				return 0;
			}
		}
	}

	return -EINVAL;
}

static int nand_read_buffer(struct nfc_config *conf, uint32_t offs,
			    unsigned int size, void *dest)
{
	int first_seed = 0, page, ret;
	mydebug("nand_read_buffer from 0x%08x (size 0x%08x) to 0x%08p\n",offs,size,dest);
	size = ALIGN(size, conf->page_size);
	page = offs / conf->page_size;
	if (conf->randomize)
		first_seed = page % conf->nseeds;

	for (; size; size -= conf->page_size) {
		if (nand_load_page(conf, offs))
			return -1;

		ret = nand_read_page(conf, offs, dest, conf->page_size);
		/*
		 * The ->nseeds value should be equal to the number of pages
		 * in an eraseblock. Since we don't know this information in
		 * advance we might have picked a wrong value.
		 */
		if (ret < 0 && conf->randomize) {
			int cur_seed = page % conf->nseeds;

			/*
			 * We already tried all the seed values => we are
			 * facing a real corruption.
			 */
			if (cur_seed < first_seed)
				return -EIO;

			/* Try to adjust ->nseeds and read the page again... */
			conf->nseeds = cur_seed;

			if (nand_change_column(0))
				return -EIO;

			/* ... it still fails => it's a real corruption. */
			if (nand_read_page(conf, offs, dest, conf->page_size))
				return -EIO;
		} else if (ret && conf->randomize) {
			mydebug("memset:0x%08p 0xff\r\n",dest);
			memset(dest, 0xff, conf->page_size);
		}

		page++;
		offs += conf->page_size;
		dest += conf->page_size;
	}

	return 0;
}

int nand_spl_load_image(uint32_t offs, unsigned int size, void *dest)
{
	static struct nfc_config conf = { };
	int ret;
	mydebug("nand_spl_load_image\r\n");
	ret = nand_detect_config(&conf, offs, dest);
	if (ret)
	{
		mydebug("nand_spl_load_image:%d\r\n",ret);
		return ret;
	}
	mydebug("nand_detect_config end\r\n");
	return nand_read_buffer(&conf, offs, size, dest);
}

void nand_deselect(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	clrbits_le32(&ccm->ahb_gate0, (CLK_GATE_OPEN << AHB_GATE_OFFSET_NAND0));
#ifdef CONFIG_MACH_SUN9I
	clrbits_le32(&ccm->ahb_gate1, (1 << AHB_GATE_OFFSET_DMA));
#else
	clrbits_le32(&ccm->ahb_gate0, (1 << AHB_GATE_OFFSET_DMA));
#endif
	clrbits_le32(&ccm->nand0_clk_cfg, CCM_NAND_CTRL_ENABLE | AHB_DIV_1);
}
