/**
 * @file dma_driver.h
 * @brief DMA Driver Layer interface definitions
 *
 * This module provides high-level DMA driver functions with business logic
 * and configuration management. It builds on top of the HAL layer.
 */

#ifndef DMA_DRIVER_H
#define DMA_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DMA driver status codes
 */
typedef enum {
    DMA_DRV_OK = 0,                 /**< Operation successful */
    DMA_DRV_ERROR,                  /**< General error */
    DMA_DRV_INVALID_PARAM,          /**< Invalid parameter */
    DMA_DRV_NOT_INITIALIZED,        /**< Driver not initialized */
    DMA_DRV_CHANNEL_BUSY,           /**< Channel is busy */
    DMA_DRV_CHANNEL_ERROR,          /**< Channel error */
    DMA_DRV_TRANSFER_COMPLETE,      /**< Transfer completed */
    DMA_DRV_TRANSFER_HALF_COMPLETE, /**< Transfer half completed */
    DMA_DRV_TIMEOUT                 /**< Operation timeout */
} dma_drv_status_t;

/**
 * @brief DMA transfer direction
 */
typedef enum {
    DMA_TRANSFER_MEM2MEM = 0,       /**< Memory to memory */
    DMA_TRANSFER_MEM2PERI,          /**< Memory to peripheral */
    DMA_TRANSFER_PERI2MEM,          /**< Peripheral to memory */
    DMA_TRANSFER_PERI2PERI          /**< Peripheral to peripheral */
} dma_transfer_direction_t;

/**
 * @brief DMA channel priority levels
 */
typedef enum {
    DMA_PRIORITY_LOW = 0,           /**< Low priority */
    DMA_PRIORITY_MEDIUM,            /**< Medium priority */
    DMA_PRIORITY_HIGH,              /**< High priority */
    DMA_PRIORITY_VERY_HIGH          /**< Very high priority */
} dma_priority_t;

/**
 * @brief DMA data size
 */
typedef enum {
    DMA_DATA_SIZE_BYTE = 0,         /**< 8-bit data size */
    DMA_DATA_SIZE_HALFWORD,         /**< 16-bit data size */
    DMA_DATA_SIZE_WORD              /**< 32-bit data size */
} dma_data_size_t;

/**
 * @brief DMA channel state
 */
typedef enum {
    DMA_CHANNEL_STATE_IDLE = 0,     /**< Channel is idle */
    DMA_CHANNEL_STATE_READY,        /**< Channel is ready */
    DMA_CHANNEL_STATE_BUSY,         /**< Channel is busy */
    DMA_CHANNEL_STATE_COMPLETE,     /**< Transfer completed */
    DMA_CHANNEL_STATE_HALF_COMPLETE,/**< Transfer half completed */
    DMA_CHANNEL_STATE_ERROR         /**< Channel in error state */
} dma_channel_state_t;

/**
 * @brief DMA address mode
 */
typedef enum {
    DMA_ADDR_MODE_FIXED = 0,        /**< Fixed address mode */
    DMA_ADDR_MODE_INCREMENT         /**< Increment address mode */
} dma_addr_mode_t;

/**
 * @brief DMA channel configuration structure
 */
typedef struct {
    dma_transfer_direction_t direction;     /**< Transfer direction */
    dma_priority_t priority;                /**< Channel priority */
    dma_data_size_t src_data_size;          /**< Source data size */
    dma_data_size_t dst_data_size;          /**< Destination data size */
    dma_addr_mode_t src_addr_mode;          /**< Source address mode */
    dma_addr_mode_t dst_addr_mode;          /**< Destination address mode */
    uint32_t src_address;                   /**< Source address */
    uint32_t dst_address;                   /**< Destination address */
    uint32_t data_length;                   /**< Data transfer length */
    uint16_t src_offset;                    /**< Source address offset */
    uint16_t dst_offset;                    /**< Destination address offset */
    uint32_t dst_start_addr;                /**< Destination start address (circular mode) */
    uint32_t dst_end_addr;                  /**< Destination end address (circular mode) */
    uint8_t req_id;                         /**< DMAMUX request ID */
    bool trig_enable;                       /**< DMAMUX trigger enable */
    bool circular_mode;                     /**< Circular mode enable */
    bool half_complete_interrupt;           /**< Half complete interrupt enable */
    bool complete_interrupt;                /**< Complete interrupt enable */
    bool error_interrupt;                   /**< Error interrupt enable */
    bool debug_enable;                      /**< Debug mode enable */
} dma_channel_config_t;

/**
 * @brief DMA channel status information
 */
typedef struct {
    dma_channel_state_t state;              /**< Channel state */
    uint32_t status_flags;                  /**< Raw status flags */
    uint32_t transferred_count;             /**< Number of data transferred */
    uint32_t fifo_left_count;               /**< FIFO data left count */
    bool finish_flag;                       /**< Transfer finish flag */
    bool half_finish_flag;                  /**< Half transfer finish flag */
    bool dst_bus_error;                     /**< Destination bus error */
    bool src_bus_error;                     /**< Source bus error */
    bool dst_offset_error;                  /**< Destination offset error */
    bool src_offset_error;                  /**< Source offset error */
    bool dst_addr_error;                    /**< Destination address error */
    bool src_addr_error;                    /**< Source address error */
    bool channel_length_error;              /**< Channel length error */
} dma_channel_status_t;

/**
 * @brief DMA driver version information
 */
typedef struct {
    uint8_t major;                          /**< Major version */
    uint8_t minor;                          /**< Minor version */
    uint8_t patch;                          /**< Patch version */
} dma_driver_version_t;

/**
 * @brief Maximum number of DMA channels
 */
#define DMA_MAX_CHANNELS            16

/**
 * @brief Default DMA configuration values
 */
#define DMA_DEFAULT_PRIORITY        DMA_PRIORITY_LOW
#define DMA_DEFAULT_DATA_SIZE       DMA_DATA_SIZE_WORD
#define DMA_DEFAULT_ADDR_MODE       DMA_ADDR_MODE_INCREMENT

/**
 * @brief Status flag bit definitions
 */
#define DMA_STATUS_FINISH           (1U << 0)
#define DMA_STATUS_HALF_FINISH      (1U << 1)
#define DMA_STATUS_DBE              (1U << 2)
#define DMA_STATUS_SBE              (1U << 3)
#define DMA_STATUS_DOE              (1U << 4)
#define DMA_STATUS_SOE              (1U << 5)
#define DMA_STATUS_DAE              (1U << 6)
#define DMA_STATUS_SAE              (1U << 7)
#define DMA_STATUS_CLE              (1U << 8)

/**
 * @brief Configuration register bit definitions
 */
#define DMA_CONFIG_PRIORITY_MASK    (0x3U << 0)
#define DMA_CONFIG_PRIORITY_SHIFT   0
#define DMA_CONFIG_SSIZE_MASK       (0x3U << 2)
#define DMA_CONFIG_SSIZE_SHIFT      2
#define DMA_CONFIG_DSIZE_MASK       (0x3U << 4)
#define DMA_CONFIG_DSIZE_SHIFT      4
#define DMA_CONFIG_SINC             (1U << 6)
#define DMA_CONFIG_DINC             (1U << 7)
#define DMA_CONFIG_DIRECTION_MASK   (0x3U << 8)
#define DMA_CONFIG_DIRECTION_SHIFT  8
#define DMA_CONFIG_CIRCULAR         (1U << 10)
#define DMA_CONFIG_HALF_INT_EN      (1U << 11)
#define DMA_CONFIG_COMPLETE_INT_EN  (1U << 12)
#define DMA_CONFIG_ERROR_INT_EN     (1U << 13)

/**
 * @brief Enable register bit definitions
 */
#define DMA_ENABLE_CHAN_EN          (1U << 0)
#define DMA_ENABLE_EDBG             (1U << 1)

/**
 * @brief DMAMUX configuration bit definitions
 */
#define DMA_DMAMUX_REQ_ID_MASK      (0x7FU << 0)
#define DMA_DMAMUX_REQ_ID_SHIFT     0
#define DMA_DMAMUX_TRIG_EN          (1U << 7)

/**
 * @brief DMAMUX request IDs for common peripherals
 */
#define DMA_REQ_ALWAYS_ENABLED      0x3F    /**< Always enabled (mem2mem) */
#define DMA_REQ_UART0_RX            0x01    /**< UART0 RX */
#define DMA_REQ_UART0_TX            0x02    /**< UART0 TX */
#define DMA_REQ_UART1_RX            0x03    /**< UART1 RX */
#define DMA_REQ_UART1_TX            0x04    /**< UART1 TX */
#define DMA_REQ_UART2_RX            0x05    /**< UART2 RX */
#define DMA_REQ_UART2_TX            0x06    /**< UART2 TX */
#define DMA_REQ_UART3_RX            0x07    /**< UART3 RX */
#define DMA_REQ_UART3_TX            0x08    /**< UART3 TX */
#define DMA_REQ_UART4_RX            0x09    /**< UART4 RX */
#define DMA_REQ_UART4_TX            0x0A    /**< UART4 TX */
#define DMA_REQ_UART5_RX            0x0B    /**< UART5 RX */
#define DMA_REQ_UART5_TX            0x0C    /**< UART5 TX */
#define DMA_REQ_EIO_SHIFTER0        0x0D    /**< EIO Shifter 0 */
#define DMA_REQ_EIO_SHIFTER1        0x0E    /**< EIO Shifter 1 */
#define DMA_REQ_EIO_SHIFTER2        0x0F    /**< EIO Shifter 2 */
#define DMA_REQ_EIO_SHIFTER3        0x10    /**< EIO Shifter 3 */

/**
 * @brief Initialize DMA driver
 * @param base_address DMA controller base address
 * @return Driver status code
 */
dma_drv_status_t dma_driver_init(uint32_t base_address);

/**
 * @brief Deinitialize DMA driver
 * @return Driver status code
 */
dma_drv_status_t dma_driver_deinit(void);

/**
 * @brief Get driver version information
 * @param version Pointer to store version information
 * @return Driver status code
 */
dma_drv_status_t dma_driver_get_version(dma_driver_version_t *version);

/**
 * @brief Configure DMA channel
 * @param channel Channel number (0-15)
 * @param config Pointer to channel configuration
 * @return Driver status code
 */
dma_drv_status_t dma_channel_configure(uint32_t channel, const dma_channel_config_t *config);

/**
 * @brief Get default channel configuration
 * @param config Pointer to store default configuration
 * @return Driver status code
 */
dma_drv_status_t dma_channel_get_default_config(dma_channel_config_t *config);

/**
 * @brief Start DMA transfer on a channel
 * @param channel Channel number (0-15)
 * @return Driver status code
 */
dma_drv_status_t dma_channel_start(uint32_t channel);

/**
 * @brief Stop DMA transfer on a channel
 * @param channel Channel number (0-15)
 * @return Driver status code
 */
dma_drv_status_t dma_channel_stop(uint32_t channel);

/**
 * @brief Pause DMA transfer on a channel
 * @param channel Channel number (0-15)
 * @return Driver status code
 */
dma_drv_status_t dma_channel_pause(uint32_t channel);

/**
 * @brief Resume DMA transfer on a channel
 * @param channel Channel number (0-15)
 * @return Driver status code
 */
dma_drv_status_t dma_channel_resume(uint32_t channel);

/**
 * @brief Get DMA channel status
 * @param channel Channel number (0-15)
 * @param status Pointer to store channel status
 * @return Driver status code
 */
dma_drv_status_t dma_channel_get_status(uint32_t channel, dma_channel_status_t *status);

/**
 * @brief Clear DMA channel status flags
 * @param channel Channel number (0-15)
 * @param flags Status flags to clear
 * @return Driver status code
 */
dma_drv_status_t dma_channel_clear_status(uint32_t channel, uint32_t flags);

/**
 * @brief Check if DMA channel is busy
 * @param channel Channel number (0-15)
 * @param is_busy Pointer to store busy status
 * @return Driver status code
 */
dma_drv_status_t dma_channel_is_busy(uint32_t channel, bool *is_busy);

/**
 * @brief Wait for DMA transfer completion
 * @param channel Channel number (0-15)
 * @param timeout_ms Timeout in milliseconds (0 = no timeout)
 * @return Driver status code
 */
dma_drv_status_t dma_channel_wait_for_completion(uint32_t channel, uint32_t timeout_ms);

/**
 * @brief Perform memory-to-memory transfer
 * @param channel Channel number (0-15)
 * @param src_addr Source address
 * @param dst_addr Destination address
 * @param size Transfer size in bytes
 * @return Driver status code
 */
dma_drv_status_t dma_mem2mem_transfer(uint32_t channel, uint32_t src_addr, uint32_t dst_addr, uint32_t size);

/**
 * @brief Perform memory-to-peripheral transfer
 * @param channel Channel number (0-15)
 * @param src_addr Source address
 * @param peri_addr Peripheral address
 * @param size Transfer size in bytes
 * @param req_id DMAMUX request ID
 * @return Driver status code
 */
dma_drv_status_t dma_mem2peri_transfer(uint32_t channel, uint32_t src_addr, uint32_t peri_addr, uint32_t size, uint8_t req_id);

/**
 * @brief Perform peripheral-to-memory transfer
 * @param channel Channel number (0-15)
 * @param peri_addr Peripheral address
 * @param dst_addr Destination address
 * @param size Transfer size in bytes
 * @param req_id DMAMUX request ID
 * @return Driver status code
 */
dma_drv_status_t dma_peri2mem_transfer(uint32_t channel, uint32_t peri_addr, uint32_t dst_addr, uint32_t size, uint8_t req_id);

/**
 * @brief Setup circular buffer mode
 * @param channel Channel number (0-15)
 * @param src_addr Source address
 * @param dst_start_addr Destination start address
 * @param dst_end_addr Destination end address
 * @param data_size Data size per transfer
 * @return Driver status code
 */
dma_drv_status_t dma_setup_circular_mode(uint32_t channel, uint32_t src_addr, uint32_t dst_start_addr, uint32_t dst_end_addr, dma_data_size_t data_size);

/**
 * @brief Perform DMA global reset (warm reset)
 * @return Driver status code
 */
dma_drv_status_t dma_global_warm_reset(void);

/**
 * @brief Perform DMA global reset (hard reset)
 * @return Driver status code
 */
dma_drv_status_t dma_global_hard_reset(void);

/**
 * @brief Check if DMA is idle
 * @param is_idle Pointer to store idle status
 * @return Driver status code
 */
dma_drv_status_t dma_is_idle(bool *is_idle);

/**
 * @brief Get number of transferred data for a channel
 * @param channel Channel number (0-15)
 * @param transferred_count Pointer to store transferred count
 * @return Driver status code
 */
dma_drv_status_t dma_channel_get_transferred_count(uint32_t channel, uint32_t *transferred_count);

/**
 * @brief Get FIFO data left count for a channel
 * @param channel Channel number (0-15)
 * @param fifo_left_count Pointer to store FIFO left count
 * @return Driver status code
 */
dma_drv_status_t dma_channel_get_fifo_left_count(uint32_t channel, uint32_t *fifo_left_count);

/**
 * @brief Flush channel internal FIFO
 * @param channel Channel number (0-15)
 * @return Driver status code
 */
dma_drv_status_t dma_channel_flush_fifo(uint32_t channel);

#ifdef __cplusplus
}
#endif

#endif /* DMA_DRIVER_H */