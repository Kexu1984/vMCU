/**
 * @file dmav2_driver.h
 * @brief DMAv2 Driver Layer interface definitions
 *
 * This module provides high-level API functions for the DMAv2 device,
 * implementing business logic and configuration management.
 * 
 * Features:
 * - 16 independent DMA channels
 * - Multiple transfer modes: mem2mem, mem2peri, peri2mem
 * - Address increment/fixed modes
 * - Circular buffer support
 * - Priority levels and DMAMUX configuration
 */

#ifndef DMAV2_DRIVER_H
#define DMAV2_DRIVER_H

#include "dmav2_hal.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DMAv2 driver version information
 */
#define DMAV2_DRIVER_VERSION_MAJOR  2
#define DMAV2_DRIVER_VERSION_MINOR  0
#define DMAV2_DRIVER_VERSION_PATCH  0

/**
 * @brief DMAv2 driver status codes
 */
typedef enum {
    DMAV2_SUCCESS = 0,                  /**< Operation successful */
    DMAV2_ERROR_INVALID_PARAM,          /**< Invalid parameter */
    DMAV2_ERROR_NOT_INITIALIZED,        /**< Driver not initialized */
    DMAV2_ERROR_HARDWARE_FAULT,         /**< Hardware error */
    DMAV2_ERROR_TIMEOUT,                /**< Operation timeout */
    DMAV2_ERROR_BUSY,                   /**< Channel busy */
    DMAV2_ERROR_CHANNEL_INVALID,        /**< Invalid channel number */
    DMAV2_ERROR_CONFIG_INVALID,         /**< Invalid configuration */
    DMAV2_ERROR_TRANSFER_FAILED         /**< Transfer operation failed */
} dmav2_status_t;

/**
 * @brief DMAv2 transfer modes
 */
typedef enum {
    DMAV2_MODE_MEM2MEM = 0,             /**< Memory to Memory */
    DMAV2_MODE_MEM2PERI,                /**< Memory to Peripheral */
    DMAV2_MODE_PERI2MEM,                /**< Peripheral to Memory */
    DMAV2_MODE_PERI2PERI                /**< Peripheral to Peripheral */
} dmav2_transfer_mode_t;

/**
 * @brief DMAv2 data width types
 */
typedef enum {
    DMAV2_DATA_WIDTH_8BIT = 0,          /**< 8-bit data width */
    DMAV2_DATA_WIDTH_16BIT,             /**< 16-bit data width */
    DMAV2_DATA_WIDTH_32BIT              /**< 32-bit data width */
} dmav2_data_width_t;

/**
 * @brief DMAv2 priority levels
 */
typedef enum {
    DMAV2_PRIORITY_LOW = 0,             /**< Low priority */
    DMAV2_PRIORITY_MEDIUM,              /**< Medium priority */
    DMAV2_PRIORITY_HIGH,                /**< High priority */
    DMAV2_PRIORITY_VERY_HIGH            /**< Very high priority */
} dmav2_priority_t;

/**
 * @brief DMAv2 address increment modes
 */
typedef enum {
    DMAV2_ADDR_FIXED = 0,               /**< Fixed address mode */
    DMAV2_ADDR_INCREMENT                /**< Address increment mode */
} dmav2_addr_mode_t;

/**
 * @brief DMAv2 channel states
 */
typedef enum {
    DMAV2_CHANNEL_IDLE = 0,             /**< Channel idle */
    DMAV2_CHANNEL_ACTIVE,               /**< Channel active/transferring */
    DMAV2_CHANNEL_PAUSED,               /**< Channel paused */
    DMAV2_CHANNEL_ERROR                 /**< Channel in error state */
} dmav2_channel_state_t;

/**
 * @brief DMAv2 channel configuration structure
 */
typedef struct {
    dmav2_transfer_mode_t mode;         /**< Transfer mode */
    dmav2_data_width_t src_width;       /**< Source data width */
    dmav2_data_width_t dst_width;       /**< Destination data width */
    dmav2_priority_t priority;          /**< Channel priority */
    dmav2_addr_mode_t src_addr_mode;    /**< Source address mode */
    dmav2_addr_mode_t dst_addr_mode;    /**< Destination address mode */
    bool circular_mode;                 /**< Enable circular mode */
    bool half_complete_interrupt;       /**< Enable half-complete interrupt */
    bool complete_interrupt;            /**< Enable transfer complete interrupt */
    bool error_interrupt;               /**< Enable error interrupt */
    uint8_t dmamux_req_id;              /**< DMAMUX request ID */
    bool dmamux_trigger_enable;         /**< DMAMUX trigger enable */
} dmav2_channel_config_t;

/**
 * @brief DMAv2 transfer configuration structure
 */
typedef struct {
    uint32_t src_addr;                  /**< Source address */
    uint32_t dst_addr;                  /**< Destination address */
    uint32_t dst_start_addr;            /**< Destination start address (for circular mode) */
    uint32_t dst_end_addr;              /**< Destination end address (for circular mode) */
    uint16_t src_offset;                /**< Source address offset */
    uint16_t dst_offset;                /**< Destination address offset */
    uint16_t transfer_count;            /**< Number of data transfers */
} dmav2_transfer_config_t;

/**
 * @brief DMAv2 channel status structure
 */
typedef struct {
    dmav2_channel_state_t state;        /**< Channel state */
    bool transfer_complete;             /**< Transfer complete flag */
    bool half_complete;                 /**< Half transfer complete flag */
    bool error_occurred;                /**< Error flag */
    uint8_t error_flags;                /**< Detailed error flags */
    uint16_t transferred_count;         /**< Number of transfers completed */
    uint8_t fifo_data_left;             /**< Data left in FIFO */
} dmav2_channel_status_t;

/**
 * @brief DMAv2 global status structure
 */
typedef struct {
    bool dma_idle;                      /**< DMA controller idle status */
    uint16_t active_channels;           /**< Bit mask of active channels */
} dmav2_global_status_t;

/**
 * @brief DMAv2 error flags bit definitions
 */
#define DMAV2_ERROR_CHANNEL_LENGTH      (1U << 0)   /**< Channel length error */
#define DMAV2_ERROR_SOURCE_ADDRESS      (1U << 1)   /**< Source address error */
#define DMAV2_ERROR_DEST_ADDRESS        (1U << 2)   /**< Destination address error */
#define DMAV2_ERROR_SOURCE_OFFSET       (1U << 3)   /**< Source offset error */
#define DMAV2_ERROR_DEST_OFFSET         (1U << 4)   /**< Destination offset error */
#define DMAV2_ERROR_SOURCE_BUS          (1U << 5)   /**< Source bus error */
#define DMAV2_ERROR_DEST_BUS            (1U << 6)   /**< Destination bus error */

/**
 * @brief Driver initialization and management functions
 */
dmav2_status_t dmav2_init(void);
dmav2_status_t dmav2_deinit(void);
dmav2_status_t dmav2_reset(bool hard_reset);
dmav2_status_t dmav2_get_global_status(dmav2_global_status_t *status);

/**
 * @brief Channel configuration functions
 */
dmav2_status_t dmav2_channel_init(uint8_t channel);
dmav2_status_t dmav2_channel_deinit(uint8_t channel);
dmav2_status_t dmav2_channel_configure(uint8_t channel, const dmav2_channel_config_t *config);
dmav2_status_t dmav2_channel_get_config(uint8_t channel, dmav2_channel_config_t *config);

/**
 * @brief Transfer control functions
 */
dmav2_status_t dmav2_start_transfer(uint8_t channel, const dmav2_transfer_config_t *transfer_config);
dmav2_status_t dmav2_stop_transfer(uint8_t channel);
dmav2_status_t dmav2_pause_transfer(uint8_t channel);
dmav2_status_t dmav2_resume_transfer(uint8_t channel);
dmav2_status_t dmav2_abort_transfer(uint8_t channel);

/**
 * @brief Status and monitoring functions
 */
dmav2_status_t dmav2_get_channel_status(uint8_t channel, dmav2_channel_status_t *status);
dmav2_status_t dmav2_clear_channel_status(uint8_t channel, uint8_t flags_to_clear);
dmav2_status_t dmav2_get_transfer_progress(uint8_t channel, uint16_t *transferred_count);
dmav2_status_t dmav2_is_transfer_complete(uint8_t channel, bool *complete);

/**
 * @brief Memory-to-Memory transfer convenience functions
 */
dmav2_status_t dmav2_mem_to_mem_transfer(uint8_t channel, 
                                        uint32_t src_addr, 
                                        uint32_t dst_addr, 
                                        uint16_t count,
                                        dmav2_data_width_t data_width);

dmav2_status_t dmav2_mem_to_mem_transfer_blocking(uint8_t channel, 
                                                 uint32_t src_addr, 
                                                 uint32_t dst_addr, 
                                                 uint16_t count,
                                                 dmav2_data_width_t data_width,
                                                 uint32_t timeout_ms);

/**
 * @brief Circular buffer transfer functions
 */
dmav2_status_t dmav2_setup_circular_transfer(uint8_t channel,
                                            uint32_t src_addr,
                                            uint32_t dst_start_addr,
                                            uint32_t dst_end_addr,
                                            uint16_t src_offset,
                                            uint16_t dst_offset,
                                            const dmav2_channel_config_t *config);

/**
 * @brief Advanced configuration functions
 */
dmav2_status_t dmav2_set_channel_priority(uint8_t channel, dmav2_priority_t priority);
dmav2_status_t dmav2_set_dmamux_config(uint8_t channel, uint8_t req_id, bool trigger_enable);
dmav2_status_t dmav2_set_address_offsets(uint8_t channel, uint16_t src_offset, uint16_t dst_offset);
dmav2_status_t dmav2_enable_debug_mode(uint8_t channel, bool enable);

/**
 * @brief Utility and helper functions
 */
dmav2_status_t dmav2_validate_channel(uint8_t channel);
dmav2_status_t dmav2_validate_config(const dmav2_channel_config_t *config);
dmav2_status_t dmav2_validate_transfer_config(const dmav2_transfer_config_t *transfer_config);
const char* dmav2_get_error_string(dmav2_status_t status);
const char* dmav2_get_version_string(void);

/**
 * @brief Debug and diagnostic functions
 */
dmav2_status_t dmav2_dump_channel_registers(uint8_t channel);
dmav2_status_t dmav2_dump_global_registers(void);
dmav2_status_t dmav2_self_test(void);

/**
 * @brief Advanced transfer modes testing functions
 */
dmav2_status_t dmav2_test_mem2mem_mode(uint8_t channel);
dmav2_status_t dmav2_test_mem2peri_mode(uint8_t channel);
dmav2_status_t dmav2_test_peri2mem_mode(uint8_t channel);
dmav2_status_t dmav2_test_address_fixed_mode(uint8_t channel);
dmav2_status_t dmav2_test_address_increment_mode(uint8_t channel);
dmav2_status_t dmav2_test_circular_mode(uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif /* DMAV2_DRIVER_H */