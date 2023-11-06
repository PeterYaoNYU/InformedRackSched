#include "dma_tx.h"


// fetch the packets from the ring, send it to dma
int lcore_dma_tx(struct dma_tx_param * p)
{
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
}