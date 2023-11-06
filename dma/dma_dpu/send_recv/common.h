#ifndef __RING_COMMON_H_
#define __RING_COMMON_H_

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <inttypes.h>
#include <unistd.h>
#include <rte_log.h>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUFS ((64*1024)-1)
#define MBUF_CACHE_SIZE 128
#define SCHED_RX_RING_SZ 8192
#define SCHED_TX_RING_SZ 65536

#define NUM_RX_QUEUE 4
#define NUM_TX_QUEUE 2

#define NUM_RX_CORES 4  // Number of cores for receiving
#define NUM_TX_CORES 2  // Number of cores for sending
#define NUM_SCHED_CORES 2 // Number of cores doing Scheduling & DMA

#define BURST_SIZE 64
#define BURST_SIZE_TX 32

struct lcore_params {
    uint16_t rx_queue_id;
    uint16_t tx_queue_id;
    struct rte_mempool *mem_pool;
    struct rte_ring *shared_ring;
};

#define RING_SIZE 1024

#endif