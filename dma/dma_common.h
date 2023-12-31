#ifndef DMA_COMMON_H_
#define DMA_COMMON_H_

#include <unistd.h>

#include <doca_dma.h>
#include <doca_error.h>

#include "common.h"

#define MAX_USER_ARG_SIZE 256			/* Maximum size of user input argument */
#define MAX_ARG_SIZE (MAX_USER_ARG_SIZE + 1)	/* Maximum size of input argument */
#define MAX_USER_TXT_SIZE 4096			/* Maximum size of user input text */
#define MAX_TXT_SIZE (MAX_USER_TXT_SIZE + 1)	/* Maximum size of input text */
#define PAGE_SIZE sysconf(_SC_PAGESIZE)		/* Page size */
#define WORKQ_DEPTH 32				/* Work queue depth */
#define USER_PCI_ADDR_LEN 7			/* User PCI address string length */
#define PCI_ADDR_LEN (USER_PCI_ADDR_LEN + 1)

/* Configuration struct */
struct dma_config {
	char pci_address[PCI_ADDR_LEN];		/* PCI device address */
	char cpy_txt[MAX_TXT_SIZE];		/* Text to copy between the two local buffers */
	char export_desc_path[MAX_ARG_SIZE];	/* Path to save/read the exported descriptor file */
	char buf_info_path[MAX_ARG_SIZE];	/* Path to save/read the buffer information file */
};


struct dma_copy_cfg {
	// enum dma_copy_mode mode;				/* Node running mode {host, dpu} */
	bool send_to_host;						/* Indicate DMA copy direction */
	uint32_t packet_size;					/* The length of packets in bytes */
};

/*
 * Register the command line parameters for the DOCA DMA samples
 *
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t register_dma_params(void);

/*
 * Initiates all DOCA core structures needed by the Host.
 *
 * @state [in]: Structure containing all DOCA core structures
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t host_init_core_objects(struct program_core_objects *state);

/*
 * Destroys all DOCA core structures
 *
 * @state [in]: Structure containing all DOCA core structures
 */
void host_destroy_core_objects(struct program_core_objects *state);

/*
 * Removes all DOCA core structures
 *
 * @state [in]: Structure containing all DOCA core structures
 * @dma_ctx [in]: DMA context
 */
void dma_cleanup(struct program_core_objects *state, struct doca_dma *dma_ctx);

/**
 * Check if given device is capable of excuting a DOCA_DMA_JOB_MEMCPY.
 *
 * @devinfo [in]: The DOCA device information
 * @return: DOCA_SUCCESS if the device supports DOCA_DMA_JOB_MEMCPY and DOCA_ERROR otherwise.
 */
doca_error_t dma_jobs_is_supported(struct doca_devinfo *devinfo);

#endif
