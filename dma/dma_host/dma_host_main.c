#include <stdlib.h>
#include <string.h>

#include <doca_argp.h>
#include <doca_dev.h>
#include <doca_log.h>

#include <doca_utils.h>

#include "dma_common.h"

DOCA_LOG_REGISTER(DMA_COPY_HOST::MAIN);

/* Sample's Logic */
doca_error_t dma_copy_host(struct doca_pci_bdf *pcie_addr, char *src_buffer, size_t src_buffer_size,
				  char *export_desc_file_path, char *buffer_info_file_path);

/*
 * Sample main function
 *
 * @argc [in]: command line arguments size
 * @argv [in]: array of command line arguments
 * @return: EXIT_SUCCESS on success and EXIT_FAILURE otherwise
 */
int
main(int argc, char **argv)
{
	struct dma_config dma_conf;
	struct doca_pci_bdf pcie_dev;
	char *src_buffer;
	size_t length;
	doca_error_t result;
	struct doca_logger_backend *stdout_logger = NULL;

	/* Set the default configuration values (Example values) */
	strcpy(dma_conf.pci_address, "b1:00.0");
	// strcpy(dma_conf.cpy_txt, "This is a sample piece of text");
	strcpy(dma_conf.export_desc_path, "/tmp/export_desc.txt");
	strcpy(dma_conf.buf_info_path, "/tmp/buffer_info.txt");

	/* Create a logger backend that prints to the standard output */
	result = doca_log_create_file_backend(stdout, &stdout_logger);
	if (result != DOCA_SUCCESS)
		return EXIT_FAILURE;

	result = doca_argp_init("doca_dma_copy_host", &dma_conf);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to init ARGP resources: %s", doca_get_error_string(result));
		return EXIT_FAILURE;
	}
	result = register_dma_params();
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to register DMA sample parameters: %s", doca_get_error_string(result));
		return EXIT_FAILURE;
	}
	result = doca_argp_start(argc, argv);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to parse sample input: %s", doca_get_error_string(result));
		return EXIT_FAILURE;
	}

#ifndef DOCA_ARCH_HOST
	DOCA_LOG_ERR("Sample can run only on the Host");
	doca_argp_destroy();
	return EXIT_FAILURE;
#endif
	// length = strlen(dma_conf.cpy_txt) + 1;
	// set the packet length to 1500 bytes. 
	length = 1500;
	max_num_packets_per_core = 1000;
	
	
	src_buffer = (char *)malloc(length);
	if (src_buffer == NULL) {
		DOCA_LOG_ERR("Source buffer allocation failed");
		doca_argp_destroy();
		return EXIT_FAILURE;
	}


	result = doca_pci_bdf_from_string(dma_conf.pci_address, &pcie_dev);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to parse pci address: %s", doca_get_error_string(result));
		free(src_buffer);
		doca_argp_destroy();
		return EXIT_FAILURE;
	}

	result = dma_copy_host(&pcie_dev, src_buffer, length, dma_conf.export_desc_path, dma_conf.buf_info_path);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Sample function has failed: %s", doca_get_error_string(result));
		free(src_buffer);
		doca_argp_destroy();
		return EXIT_FAILURE;
	}

	free(src_buffer);
	doca_argp_destroy();

	return EXIT_SUCCESS;
}

