#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>

#include <pthread.h>
#include <string.h>

#include "port_init.h"
#include "send_pkt.h"
#include "recv_pkt.h"
#include "dma_tx.h"

int main(int argc, char *argv[])
{
    rte_log_set_global_level(RTE_LOG_INFO);
    struct rte_mempool *mbuf_pool;
    struct rte_ring *tx_ring[NUM_TX_QUEUE];
    struct rte_ring *rx_ring[NUM_RX_QUEUE];
    struct lcore_params *lp_tx[NUM_TX_QUEUE];
    struct lcore_params *lp_rx[NUM_RX_QUEUE];
    struct rx_params *rxp[NUM_RX_QUEUE];
    uint16_t portid;

    struct rte_ring * shared_ring;

    unsigned lcore_id;

    // init the env
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "initlize fail!");

    // Verify that we have enough cores
    if (rte_lcore_count() < (NUM_RX_CORES + NUM_TX_CORES + NUM_SCHED_CORES)) {
        rte_exit(EXIT_FAILURE, "Not enough cores available\n");
    }

    shared_ring = rte_ring_create("SHARED_RING", RING_SIZE, rte_socket_id(), RING_F_MP_HTS_ENQ | RING_F_MC_HTS_DEQ);
    if (shared_ring == NULL) {
        rte_exit(EXIT_FAILURE, "Cannot create ring buffer\n");
    }

    /* Creates a new mempool in memory to hold the mbufs. */
    // mbuf pool is the ACTUAL place where the mbufs are stored
    // not just the pointers to the mbuf
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS,
                                        MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    if (port_init(0, mbuf_pool) != 0)
        rte_exit(EXIT_FAILURE, "Cannot init port 0\n");


    lcore_id = rte_get_next_lcore(-1, 1, 0); // Start with the first available core, do not wrap around
    rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER1, "Starting lcore_id: %u", lcore_id);
    if (lcore_id == RTE_MAX_LCORE) {
        rte_exit(EXIT_FAILURE, "Not enough cores available\n");
    }

    /* Start rx core */
    for (int i = 0; i < NUM_RX_QUEUE; ++i)
    {
        rte_log(RTE_LOG_INFO, RTE_LOGTYPE_USER1, "Starting rx core: %u", lcore_id);
        lp_rx[i] = rte_malloc(NULL, sizeof(*lp_rx[i]), 0);
        if (!lp_rx[i])
            rte_panic("malloc failure\n");
        lp_rx[i]->shared_ring = shared_ring;
        lp_rx[i]->rx_queue_id = i;
        lp_rx[i]->tx_queue_id = i;
        lp_rx[i]->mem_pool = mbuf_pool;
        rte_eal_remote_launch((lcore_function_t *)lcore_recv_pkt, lp_rx[i], lcore_id++);
    }

    
    /* Start tx core */
    for (int i = 0; i < NUM_TX_QUEUE; ++i)
    {
        lp_tx[i] = rte_malloc(NULL, sizeof(*lp_tx[i]), 0);
        if (!lp_tx[i])
            rte_panic("malloc failure\n");
        *lp_tx[i] = (struct lcore_params){i, i, mbuf_pool};
        rte_eal_remote_launch((lcore_function_t *)lcore_send_pkt, lp_tx[i], lcore_id++);
    }

    /* Start DMA core */
    struct dma_tx_param * dma_tx_p = rte_malloc(NULL, sizeof(*dma_tx_p), 0);
    if (!dma_tx_p)
        rte_panic("malloc failure\n");
    dma_tx_p->shared_ring = shared_ring;
    rte_eal_remote_launch((lcore_function_t *)lcore_dma_tx, dma_tx_p, lcore_id++);

    rte_eal_wait_lcore(lcore_id - 1);
    return 0;
}