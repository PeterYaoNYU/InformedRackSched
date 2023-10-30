#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <doca_buf.h>
#include <doca_buf_inventory.h>
#include <doca_dma.h>
#include <doca_error.h>
#include <doca_log.h>

#include "dma_common.h"

DOCA_LOG_REGISTER(DMA_COPY_DPU);

#define SLEEP_IN_NANOS (10 * 1000)	/* Sample the job every 10 microseconds  */
#define RECV_BUF_SIZE 256		/* Buffer which contains config information */
#define BUF_SIZE 256            /* Buffer for holding the data transmitted*/

/*
 * Saves export descriptor and buffer information content into memory buffers
 *
 * @export_desc_file_path [in]: Export descriptor file path
 * @buffer_info_file_path [in]: Buffer information file path
 * @export_desc [in]: Export descriptor buffer
 * @export_desc_len [in]: Export descriptor buffer length
 * @remote_addr [in]: Remote buffer address
 * @remote_addr_len [in]: Remote buffer total length
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t
save_config_info_to_buffers(const char *export_desc_file_path, const char *buffer_info_file_path, char *export_desc,
			    size_t *export_desc_len, char **remote_addr, size_t *remote_addr_len)
{
	FILE *fp;
	long file_size;
	char buffer[RECV_BUF_SIZE];

	fp = fopen(export_desc_file_path, "r");
	if (fp == NULL) {
		DOCA_LOG_ERR("Failed to open %s", export_desc_file_path);
		return DOCA_ERROR_IO_FAILED;
	}

	if (fseek(fp, 0, SEEK_END) != 0) {
		DOCA_LOG_ERR("Failed to calculate file size");
		fclose(fp);
		return DOCA_ERROR_IO_FAILED;
	}

	file_size = ftell(fp);
	if (file_size == -1) {
		DOCA_LOG_ERR("Failed to calculate file size");
		fclose(fp);
		return DOCA_ERROR_IO_FAILED;
	}

	if (file_size > RECV_BUF_SIZE)
		file_size = RECV_BUF_SIZE;

	*export_desc_len = file_size;

	if (fseek(fp, 0L, SEEK_SET) != 0) {
		DOCA_LOG_ERR("Failed to calculate file size");
		fclose(fp);
		return DOCA_ERROR_IO_FAILED;
	}

	if (fread(export_desc, 1, file_size, fp) != (size_t)file_size) {
		DOCA_LOG_ERR("Failed to allocate memory for source buffer");
		fclose(fp);
		return DOCA_ERROR_IO_FAILED;
	}

	fclose(fp);

	/* Read source buffer information from file */
	fp = fopen(buffer_info_file_path, "r");
	if (fp == NULL) {
		DOCA_LOG_ERR("Failed to open %s", buffer_info_file_path);
		return DOCA_ERROR_IO_FAILED;
	}

	/* Get source buffer address */
	if (fgets(buffer, RECV_BUF_SIZE, fp) == NULL) {
		DOCA_LOG_ERR("Failed to read the source (host) buffer address");
		fclose(fp);
		return DOCA_ERROR_IO_FAILED;
	}
	*remote_addr = (char *)strtoull(buffer, NULL, 0);

	memset(buffer, 0, RECV_BUF_SIZE);

	/* Get source buffer length */
	if (fgets(buffer, RECV_BUF_SIZE, fp) == NULL) {
		DOCA_LOG_ERR("Failed to read the source (host) buffer length");
		fclose(fp);
		return DOCA_ERROR_IO_FAILED;
	}
	*remote_addr_len = strtoull(buffer, NULL, 0);

	fclose(fp);

	return DOCA_SUCCESS;
}

/*
 * Run DOCA DMA DPU copy
 *
 * @export_desc_file_path [in]: Export descriptor file path
 * @buffer_info_file_path [in]: Buffer info file path
 * @pcie_addr [in]: Device PCI address
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t
dma_copy_dpu(char *export_desc_file_path, char *buffer_info_file_path, struct doca_pci_bdf *pcie_addr)
{
	struct program_core_objects state = {0};
	struct doca_event event = {0};
	struct doca_dma_job_memcpy dma_job;
	struct doca_dma *dma_ctx;
    // the doca buf on the host side
	struct doca_buf *host_doca_buf;
    // the doca buf on the dpu side
	struct doca_buf *dpu_doca_buf;
    /*remote mmap, will create DPU mmap later*/
	struct doca_mmap *remote_mmap;
	doca_error_t result;
	struct timespec ts = {0};
	uint32_t max_bufs = 2;
	char export_desc[1024] = {0};
	char *remote_addr = NULL;
	char *dpu_buffer;
	size_t dpu_buffer_size, remote_addr_len = 0, export_desc_len = 0;

	result = doca_dma_create(&dma_ctx);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Unable to create DMA engine: %s", doca_get_error_string(result));
		return result;
	}
    DOCA_LOG_INFO("Context created successfully");

	state.ctx = doca_dma_as_ctx(dma_ctx);
    if (state.ctx == NULL){
		DOCA_LOG_ERR("Unable to create DMA context: %s", doca_get_error_string(result));
    }

	result = open_doca_device_with_pci(pcie_addr, &dma_jobs_is_supported, &state.dev);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Unable to open_doca_device_with_pci: %s", doca_get_error_string(result));
		doca_dma_destroy(dma_ctx);
		return result;
	}

	result = init_core_objects(&state, WORKQ_DEPTH, max_bufs);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Unable to init core objects: %s", doca_get_error_string(result));
		dma_cleanup(&state, dma_ctx);
		return result;
	}

	/* Copy all relevant information into local buffers */
	save_config_info_to_buffers(export_desc_file_path, buffer_info_file_path, export_desc, &export_desc_len,
				    &remote_addr, &remote_addr_len);

	/* Copy the entire host buffer */
	dpu_buffer_size = remote_addr_len;
	dpu_buffer = (char *)malloc(dpu_buffer_size);
	if (dpu_buffer == NULL) {
		DOCA_LOG_ERR("Failed to allocate buffer memory");
		dma_cleanup(&state, dma_ctx);
		return DOCA_ERROR_NO_MEMORY;
	}

    DOCA_LOG_INFO("Remote buffer length is: %d", remote_addr_len);

    // test:
    // populate the DPU buffer with things I want to write to the host
    strcpy(dpu_buffer, "Hello from DPU");
    DOCA_LOG_INFO("The test message is: %s", dpu_buffer);


	result = doca_mmap_set_memrange(state.dpu_mmap, dpu_buffer, dpu_buffer_size);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to set memrange for DPU buffer");
		free(dpu_buffer);
		dma_cleanup(&state, dma_ctx);
		return result;
	}
	result = doca_mmap_start(state.dpu_mmap);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to start DPU mmap");
		free(dpu_buffer);
		dma_cleanup(&state, dma_ctx);
		return result;
	}

	/* Create a local DOCA mmap from exported data */
	result = doca_mmap_create_from_export(NULL, (const void *)export_desc, export_desc_len, state.dev,
					   &remote_mmap);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to create mmap from remote");
		free(dpu_buffer);
		dma_cleanup(&state, dma_ctx);
		return result;
	}

	/* Construct DOCA buffer for each address range */
	result = doca_buf_inventory_buf_by_addr(state.buf_inv, remote_mmap, remote_addr, remote_addr_len,
						&host_doca_buf);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Unable to acquire DOCA buffer representing remote buffer: %s",
			     doca_get_error_string(result));
		doca_mmap_destroy(remote_mmap);
		free(dpu_buffer);
		dma_cleanup(&state, dma_ctx);
		return result;
	}

	/* Construct DOCA buffer for each address range */
	result = doca_buf_inventory_buf_by_addr(state.buf_inv, state.dpu_mmap, dpu_buffer, dpu_buffer_size,
						&dpu_doca_buf);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Unable to acquire DOCA buffer representing destination buffer: %s",
			     doca_get_error_string(result));
		doca_buf_refcount_rm(host_doca_buf, NULL);
		doca_mmap_destroy(remote_mmap);
		free(dpu_buffer);
		dma_cleanup(&state, dma_ctx);
		return result;
	}

	/* Construct DMA job */
	dma_job.base.type = DOCA_DMA_JOB_MEMCPY;
	dma_job.base.flags = DOCA_JOB_FLAGS_NONE;
	dma_job.base.ctx = state.ctx;
	dma_job.dst_buff = host_doca_buf;
	dma_job.src_buff = dpu_doca_buf;


    // /* Set data position in src_buf */
	// result = doca_buf_get_data(dpu_doca_buf, &data);
	// if (result != DOCA_SUCCESS) {
	// 	DOCA_LOG_ERR("Failed to get data address from DOCA buffer: %s", doca_get_error_string(result));
	// 	return result;
	// }

    // set the doca_buf position at the DPU side
	result = doca_buf_set_data(dpu_doca_buf, dpu_buffer, dpu_buffer_size);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to set data for DOCA buffer: %s", doca_get_error_string(result));
		return result;
	}

	/* Set data position in host_buff */
	result = doca_buf_set_data(host_doca_buf, remote_addr, remote_addr_len);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to set data for DOCA buffer: %s", doca_get_error_string(result));
		return result;
	}

    DOCA_LOG_INFO("The data in the DPU buffer is: %s", dpu_buffer);

    DOCA_LOG_INFO("Begin pushing the job to the queue");

	/* Enqueue DMA job */
	result = doca_workq_submit(state.workq, &dma_job.base);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to submit DMA job: %s", doca_get_error_string(result));
		doca_buf_refcount_rm(dpu_doca_buf, NULL);
		doca_buf_refcount_rm(host_doca_buf, NULL);
		doca_mmap_destroy(remote_mmap);
		free(dpu_buffer);
		dma_cleanup(&state, dma_ctx);
		return result;
	}

    DOCA_LOG_INFO("Job submitted to the queue");
    

	/* Wait for job completion */
	while ((result = doca_workq_progress_retrieve(state.workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE)) ==
	       DOCA_ERROR_AGAIN) {
		ts.tv_nsec = SLEEP_IN_NANOS;
		nanosleep(&ts, &ts);
	}

	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to retrieve DMA job: %s", doca_get_error_string(result));
		return result;
	}

	/* event result is valid */
	result = (doca_error_t)event.result.u64;
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("DMA job event returned unsuccessfully: %s", doca_get_error_string(result));
		return result;
	}

	DOCA_LOG_INFO("Remote DMA copy was done Successfully");
	// dpu_buffer[dpu_buffer_size - 1] = '\0';
	// DOCA_LOG_INFO("Memory content: %s", dpu_buffer);

	if (doca_buf_refcount_rm(dpu_doca_buf, NULL) != DOCA_SUCCESS)
		DOCA_LOG_ERR("Failed to remove DOCA dpu buffer reference count");

	if (doca_buf_refcount_rm(host_doca_buf, NULL) != DOCA_SUCCESS)
		DOCA_LOG_ERR("Failed to remove DOCA host buffer reference count");

	/* Destroy remote memory map */
	if (doca_mmap_destroy(remote_mmap) != DOCA_SUCCESS)
		DOCA_LOG_ERR("Failed to destroy remote memory map");

	/* Inform host that DMA operation is done */
	DOCA_LOG_INFO("Host sample can be closed, DMA copy ended");

	/* Clean and destroy all relevant objects */
	dma_cleanup(&state, dma_ctx);

	free(dpu_buffer);

	return result;
}



/*
 * DPU side function for submitting DMA job into the work queue, the direction of the communication is determined by the parameter
 *
 * @cfg [in]: Application configuration
 * @core_state [in]: DOCA core structure
 * @bytes_to_copy [in]: Number of bytes to DMA copy
 * @buffer [in]: local DMA buffer
 * @local_doca_buf [in]: local DOCA buffer
 * @remote_doca_buf [in]: remote DOCA buffer
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t
dpu_submit_dma_job(struct dma_copy_cfg *cfg, struct program_core_objects *core_state, size_t bytes_to_copy, char *buffer,
		   struct doca_buf *local_doca_buf, struct doca_buf *remote_doca_buf)
{
	struct doca_event event = {0};
	struct doca_dma_job_memcpy dma_job = {0};
	doca_error_t result;
	struct timespec ts = {0};
	void *data;
	struct doca_buf *src_buf;
	struct doca_buf *dst_buf;

	/* Construct DMA job */
	dma_job.base.type = DOCA_DMA_JOB_MEMCPY;
	dma_job.base.flags = DOCA_JOB_FLAGS_NONE;
	dma_job.base.ctx = core_state->ctx;

	/* Determine DMA copy direction */
	if (cfg->send_to_host) {
		src_buf = local_doca_buf;
		dst_buf = remote_doca_buf;
	} else {
		src_buf = remote_doca_buf;
		dst_buf = local_doca_buf;
	}

	/* Set data position in src_buf */
	result = doca_buf_get_data(src_buf, &data);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to get data address from DOCA buffer: %s", doca_get_error_string(result));
		return result;
	}
	result = doca_buf_set_data(src_buf, data, bytes_to_copy);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to set data for DOCA buffer: %s", doca_get_error_string(result));
		return result;
	}

	dma_job.src_buff = src_buf;
	dma_job.dst_buff = dst_buf;

	/* Enqueue DMA job */
	result = doca_workq_submit(core_state->workq, &dma_job.base);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to submit DMA job: %s", doca_get_error_string(result));
		return result;
	}

	/* Wait for job completion */
	while ((result = doca_workq_progress_retrieve(core_state->workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE)) ==
	       DOCA_ERROR_AGAIN) {
		ts.tv_nsec = SLEEP_IN_NANOS;
		nanosleep(&ts, &ts);
	}

	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to retrieve DMA job: %s", doca_get_error_string(result));
		return result;
	}

	/* event result is valid */
	result = (doca_error_t)event.result.u64;
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("DMA job event returned unsuccessfully: %s", doca_get_error_string(result));
		return result;
	}

	DOCA_LOG_INFO("DMA copy was done Successfully");

	return result;
}
