/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2014 Intel Corporation
 */

#ifndef _RTE_KNI_H_
#define _RTE_KNI_H_

/**
 * @file
 * RTE KNI
 *
 * The KNI library provides the ability to create and destroy kernel NIC
 * interfaces that may be used by the RTE application to receive/transmit
 * packets from/to Linux kernel net interfaces.
 *
 * This library provides two APIs to burst receive packets from KNI interfaces,
 * and burst transmit packets to KNI interfaces.
 */

#include <rte_pci.h>
#include <rte_memory.h>
#include <rte_mempool.h>
#include <rte_ether.h>

#include <exec-env/rte_kni_common.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rte_kni;
struct rte_mbuf;

/**
 * Structure which has the function pointers for KNI interface.
 */
struct rte_kni_ops {
	uint16_t port_id; /* Port ID */

	/* Pointer to function of changing MTU */
	int (*change_mtu)(uint16_t port_id, unsigned int new_mtu);

	/* Pointer to function of configuring network interface */
	int (*config_network_if)(uint16_t port_id, uint8_t if_up);

	/* Pointer to function of configuring mac address */
	int (*config_mac_address)(uint16_t port_id, uint8_t mac_addr[]);

	/* Pointer to function of configuring promiscuous mode */
	int (*config_promiscusity)(uint16_t port_id, uint8_t to_on);
};

/**
 * Structure for configuring KNI device.
 */
struct rte_kni_conf {
	/*
	 * KNI name which will be used in relevant network device.
	 * Let the name as short as possible, as it will be part of
	 * memzone name.
	 */
	char name[RTE_KNI_NAMESIZE];
	uint32_t core_id;   /* Core ID to bind kernel thread on */
	uint16_t group_id;  /* Group ID */
	unsigned mbuf_size; /* mbuf size */
	struct rte_pci_addr addr;
	struct rte_pci_id id;

	__extension__
	uint8_t force_bind : 1; /* Flag to bind kernel thread */
	char mac_addr[ETHER_ADDR_LEN]; /* MAC address assigned to KNI */
	uint16_t mtu;
};

/**
 * Initialize and preallocate KNI subsystem
 *
 * This function is to be executed on the MASTER lcore only, after EAL
 * initialization and before any KNI interface is attempted to be
 * allocated
 *
 * @param max_kni_ifaces
 *  The maximum number of KNI interfaces that can coexist concurrently
 */
void rte_kni_init(unsigned int max_kni_ifaces);


/**
 * Allocate KNI interface according to the port id, mbuf size, mbuf pool,
 * configurations and callbacks for kernel requests.The KNI interface created
 * in the kernel space is the net interface the traditional Linux application
 * talking to.
 *
 * The rte_kni_alloc shall not be called before rte_kni_init() has been
 * called. rte_kni_alloc is thread safe.
 *
 * The mempool should have capacity of more than "2 x KNI_FIFO_COUNT_MAX"
 * elements for each KNI interface allocated.
 *
 * @param pktmbuf_pool
 *  The mempool for allocating mbufs for packets.
 * @param conf
 *  The pointer to the configurations of the KNI device.
 * @param ops
 *  The pointer to the callbacks for the KNI kernel requests.
 *
 * @return
 *  - The pointer to the context of a KNI interface.
 *  - NULL indicate error.
 */
struct rte_kni *rte_kni_alloc(struct rte_mempool *pktmbuf_pool,
		const struct rte_kni_conf *conf, struct rte_kni_ops *ops);

/**
 * Release specified KNI interface. This will stop data transfer to and from
 * this interface and will remove the paired KNI interface in kernel space.
 *
 * @note This function will trigger the kernel to remove the interface, which
 * may trigger the RTE_KNI_REQ_CFG_NETWORK_IF KNI callback. This function will
 * block until this callback is handled or times out. The user should ensure
 * that rte_kni_handle_request() is called for this interface in a separate
 * thread to handle this callback to avoid this delay.
 *
 * rte_kni_release() is thread safe, but should not be called from the same
 * thread as rte_kni_handle_request().
 *
 * @param kni
 *  The pointer to the context of an existent KNI interface.
 *
 * @return
 *  - 0 indicates success.
 *  - negative value indicates failure.
 */
int rte_kni_release(struct rte_kni *kni);

/**
 * Free specified KNI interface. It will also free the KNI interface resources
 * in kernel space. No KNI functions for this interface should be called after
 * or at the same time as calling this function. rte_kni_release() must be
 * called before this function to release the kernel interface.
 *
 * @param kni
 *  The pointer to the context of an existent KNI interface.
 *
 * @return
 *  - 0 indicates success.
 *  - -EINVAL: Invalid kni structure.
 *  - -EBUSY: KNI interface still in use.  Must call rte_kni_release().
 */
int __rte_experimental
rte_kni_free(struct rte_kni *kni);

/**
 * It is used to handle the request mbufs sent from kernel space.
 * Then analyzes it and calls the specific actions for the specific requests.
 * Finally constructs the response mbuf and puts it back to the resp_q.
 *
 * Thread Safety: This function should be called in a separate thread from the
 * thread which calls rte_kni_release() for this KNI.  This function must not
 * be called simultaneously with rte_kni_free().
 *
 * @param kni
 *  The pointer to the context of an existent KNI interface.
 *
 * @return
 *  - 0
 *  - negative value indicates failure.
 */
int rte_kni_handle_request(struct rte_kni *kni);

/**
 * Retrieve a burst of packets from a KNI interface. The retrieved packets are
 * stored in rte_mbuf structures whose pointers are supplied in the array of
 * mbufs, and the maximum number is indicated by num. It handles allocating
 * the mbufs for KNI interface alloc queue.
 *
 * @param kni
 *  The KNI interface context.
 * @param mbufs
 *  The array to store the pointers of mbufs.
 * @param num
 *  The maximum number per burst.
 *
 * @return
 *  The actual number of packets retrieved.
 */
unsigned rte_kni_rx_burst(struct rte_kni *kni, struct rte_mbuf **mbufs,
		unsigned num);

/**
 * Send a burst of packets to a KNI interface. The packets to be sent out are
 * stored in rte_mbuf structures whose pointers are supplied in the array of
 * mbufs, and the maximum number is indicated by num. It handles the freeing of
 * the mbufs in the free queue of KNI interface.
 *
 * @param kni
 *  The KNI interface context.
 * @param mbufs
 *  The array to store the pointers of mbufs.
 * @param num
 *  The maximum number per burst.
 *
 * @return
 *  The actual number of packets sent.
 */
unsigned rte_kni_tx_burst(struct rte_kni *kni, struct rte_mbuf **mbufs,
		unsigned num);

/**
 * Get the KNI context of its name.
 *
 * @param name
 *  pointer to the KNI device name.
 *
 * @return
 *  On success: Pointer to KNI interface.
 *  On failure: NULL.
 */
struct rte_kni *rte_kni_get(const char *name);

/**
 * Get the name given to a KNI device
 *
 * @param kni
 *   The KNI instance to query
 * @return
 *   The pointer to the KNI name
 */
const char *rte_kni_get_name(const struct rte_kni *kni);

/**
 * Register KNI request handling for a specified port,and it can
 * be called by master process or slave process.
 *
 * @param kni
 *  pointer to struct rte_kni.
 * @param ops
 *  pointer to struct rte_kni_ops.
 *
 * @return
 *  On success: 0
 *  On failure: -1
 */
int rte_kni_register_handlers(struct rte_kni *kni, struct rte_kni_ops *ops);

/**
 *  Unregister KNI request handling for a specified port.
 *
 *  @param kni
 *   pointer to struct rte_kni.
 *
 *  @return
 *   On success: 0
 *   On failure: -1
 */
int rte_kni_unregister_handlers(struct rte_kni *kni);

/**
 *  Close KNI device.
 */
void rte_kni_close(void);

#ifdef __cplusplus
}
#endif

#endif /* _RTE_KNI_H_ */
