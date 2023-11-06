#include "dma_tx.h"


// fetch the packets from the ring, send it to dma
int lcore_dma_tx(struct dma_tx_param * p)
{
    rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER1, "Core %u doing DMA TX dequeue.\n", rte_lcore_id());

    

}