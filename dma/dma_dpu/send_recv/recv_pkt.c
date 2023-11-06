#include "recv_pkt.h"

int lcore_recv_pkt(struct lcore_params *p)
{
	const int socket_id = rte_socket_id();
	printf("Core %u doing RX dequeue.\n", rte_lcore_id());

	uint64_t pkt_cnt = 0;

	while (1){
		struct rte_mbuf *bufs[BURST_SIZE];
		uint16_t nb_rx = rte_eth_rx_burst(0, p->rx_queue_id, bufs, BURST_SIZE);

		if (unlikely(nb_rx==0)){
			continue;
		}

		rte_log(RTE_LOG_DEBUG, RTE_LOGTYPE_USER1, "Received %u packets\n", nb_rx);

		uint16_t i;
		for (i = 0; i < nb_rx; i++){
			rte_log(RTE_LOG_DEBUG, RTE_LOGTYPE_USER1, "Enqueuing %d th packet into the ring\n", nb_rx);
			int ret = rte_ring_enqueue(p->shared_ring, bufs[i]);
			if (ret != 0){
				rte_log(RTE_LOG_ERR, RTE_LOGTYPE_USER1, "Failed to enqueue packet on ring: %s\n", rte_strerror(ret));
				rte_pktmbuf_free(bufs[i]);
			}
			rte_log(RTE_LOG_DEBUG, RTE_LOGTYPE_USER1, "Finish enqueuing %d th packet into the ring\n", nb_rx);

            if (unlikely(ret == -ENOBUFS)) {
                // The ring is full, you need to decide what to do in this case.
                // For example, you could drop the packet or implement some back-pressure mechanism.
                rte_log(RTE_LOG_ERR, RTE_LOGTYPE_USER1, "Failed to enqueue packet on ring: %s\n", rte_strerror(ret));
                rte_pktmbuf_free(bufs[i]);
            }
			rte_pktmbuf_free(bufs[i]);
		}
	}
	return 0;
}

