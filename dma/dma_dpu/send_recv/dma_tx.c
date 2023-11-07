#include "dma_tx.h"


// fetch the packets from the ring, send it to dma
int lcore_dma_tx(struct dma_tx_param * p)
{  
    int ret;
    ret = set_up_doca_dma_tx();
    if (ret != 0){
        rte_log(RTE_LOG_ERR, RTE_LOGTYPE_USER1, "Failed to set up DMA DOCA");
    }
    
    rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER1, "Core %u doing DMA TX dequeue.\n", rte_lcore_id());
    
    struct rte_mbuf *bufs[MAX_PACKETS_DMA_BURST];
    
    unsigned num_pkts;

    while (1){
        num_pkts = rte_ring_dequeue_burst(p->shared_ring, (void *)bufs, MAX_PACKETS_DMA_BURST, NULL);
        if (num_pkts == 0){
            continue;
        }
        rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER1, "Dequeued %u packets from the ring\n", num_pkts);
    }

    // after dequeing from the queue, send packets to the host per core buffer via DMA:


}

int set_up_doca_dma_tx(){
    struct dma_config dma_conf;
	struct doca_pci_bdf pcie_dev;
	doca_error_t result;
	struct doca_logger_backend *stdout_logger = NULL;

	// Set the default configuration values
    // Will extend it to allow for more configuration options
	strcpy(dma_conf.pci_address, "03:00.0");
	strcpy(dma_conf.export_desc_path, "/tmp/export_desc.txt");
	strcpy(dma_conf.buf_info_path, "/tmp/buffer_info.txt");

	result = register_dma_params();
	if (result != DOCA_SUCCESS) {
        rte_log(RTE_LOG_ERR, RTE_LOGTYPE_USER1, "Failed to register DMA sample parameters: %s", doca_get_error_string(result));
		return EXIT_FAILURE;
	}
    
#ifndef DOCA_ARCH_DPU
    rte_log(RTE_LOG_ERR, RTE_LOGTYPE_USER1, "Sample can run only on the DPU");
	return EXIT_FAILURE;
#endif
}