#ifndef FLEXSPI_NOR_FLASH_H_
#define FLEXSPI_NOR_FLASH_H_

#define EXAMPLE_FLEXSPI                 FLEXSPI
#define FLASH_SIZE                      0x1000
#define EXAMPLE_FLEXSPI_AMBA_BASE       FlexSPI_AMBA_BASE
#define FLASH_PAGE_SIZE                 256
#define EXAMPLE_SECTOR                  20
#define SECTOR_SIZE                     0x1000 /* 4K */
#define EXAMPLE_FLEXSPI_CLOCK           kCLOCK_FlexSpi
#define FLASH_PORT                      kFLEXSPI_PortA1
#define EXAMPLE_FLEXSPI_RX_SAMPLE_CLOCK kFLEXSPI_ReadSampleClkLoopbackFromDqsPad

#define NOR_CMD_LUT_SEQ_IDX_READ_NORMAL        7
#define NOR_CMD_LUT_SEQ_IDX_READ_FAST          13
#define NOR_CMD_LUT_SEQ_IDX_READ_FAST_QUAD     0
#define NOR_CMD_LUT_SEQ_IDX_READSTATUS         1
#define NOR_CMD_LUT_SEQ_IDX_WRITEENABLE        2
#define NOR_CMD_LUT_SEQ_IDX_ERASESECTOR        3
#define NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_SINGLE 6
#define NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_QUAD   4
#define NOR_CMD_LUT_SEQ_IDX_READID             8
#define NOR_CMD_LUT_SEQ_IDX_WRITESTATUSREG     9
#define NOR_CMD_LUT_SEQ_IDX_ENTERQPI           10
#define NOR_CMD_LUT_SEQ_IDX_EXITQPI            11
#define NOR_CMD_LUT_SEQ_IDX_READSTATUSREG      12
#define NOR_CMD_LUT_SEQ_IDX_ERASECHIP          5

#define CUSTOM_LUT_LENGTH        60
#define FLASH_QUAD_ENABLE        0x40
#define FLASH_BUSY_STATUS_POL    1
#define FLASH_BUSY_STATUS_OFFSET 0

/*
 * If cache is enabled, this example should maintain the cache to make sure
 * CPU core accesses the memory, not cache only.
 */
#define CACHE_MAINTAIN 0

#ifdef __cplusplus
extern "C" {
#endif

extern status_t flexspi_nor_flash_erase_sector(FLEXSPI_Type *base, uint32_t address);
extern status_t flexspi_nor_flash_page_program(FLEXSPI_Type *base, uint32_t dstAddr, const uint32_t *src);
extern status_t flexspi_nor_get_vendor_id(FLEXSPI_Type *base, uint8_t *vendorId);
extern status_t flexspi_nor_enable_quad_mode(FLEXSPI_Type *base);
extern status_t flexspi_nor_erase_chip(FLEXSPI_Type *base);
extern void flexspi_nor_flash_init(FLEXSPI_Type *base);

#if (defined CACHE_MAINTAIN) && (CACHE_MAINTAIN == 1)
typedef struct _flexspi_cache_status
{
    volatile bool DCacheEnableFlag;
    volatile bool ICacheEnableFlag;
} flexspi_cache_status_t;
#endif

#ifdef __cplusplus
}
#endif

#endif
