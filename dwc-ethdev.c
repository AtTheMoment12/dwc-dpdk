#include <sys/queue.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <rte_byteorder.h>
#include <rte_common.h>
#include <rte_cycles.h>

#include <rte_interrupts.h>
#include <rte_log.h>
#include <rte_debug.h>
#include <rte_pci.h>
#include <rte_bus_pci.h>
#include <rte_branch_prediction.h>
#include <rte_memory.h>
#include <rte_eal.h>
#include <rte_alarm.h>
#include <rte_ether.h>
#include <rte_ethdev_driver.h>
#include <rte_ethdev_pci.h>
#include <rte_malloc.h>
#include <rte_random.h>
#include <rte_dev.h>
#include <rte_hash_crc.h>
#ifdef RTE_LIBRTE_SECURITY
#include <rte_security_driver.h>
#endif

#include "dwc-ethdev.h"
#include "dwc-logs.h"
#include "base/dwc-xlgmac.h"
#include "base/dwc-xlgmac-reg.h"


/*
 * The set of PCI devices this driver supports
 */
static const struct rte_pci_id pci_id_dwc_map[] = {
    { RTE_PCI_DEVICE(PCI_VENDOR_ID_SYNOPSYS, 0x7302) },
    { .vendor_id = 0, /* sentinel */ },
}

static const struct eth_dev_ops ixgbe_eth_dev_ops = {
        .dev_configure = dwc_dev_configure,
        .dev_start = dwc_dev_start,
        .dev_stop = ixgbe_dev_stop,
        .dev_set_link_up = ixgbe_dev_set_link_up,
        .dev_set_link_down = ixgbe_dev_set_link_down,
        .dev_close = ixgbe_dev_close,
        .dev_reset = ixgbe_dev_reset,
        
};

static int
dwc_dev_configure(struct rte_eth_dev* dev)
{
    struct dwc_interrupt* intr =
        DWC_DEV_PRIVATE_TO_INTR(dev->data->dev_private);
    struct dwc_adapter* adapter =
        (struct dwc_adapter*)dev->data->dev_private;
    int ret;

    PMD_INIT_FUNC_TRACE();

    /* 检查多队列模式 */
    ret = ixgbe_check_mq_mode(dev);
    if (ret != 0) {
        PMD_DRV_LOG(ERR, "ixgbe_check_mq_mode fails with %d.",
            ret);
        return ret;
    }

    /* set flag to update link status after init */
    intr->flags |= DWC_FLAG_NEED_LINK_UPDATE;
    
    /*
         * Initialize to TRUE. If any of Rx queues doesn't meet the bulk
         * allocation or vector Rx preconditions we will reset it.
         */
    adapter->rx_bulk_alloc_allowed = true;
    adapter->rx_vec_allowed = true;

    return 0;



}

/*
*Configure device link speedand setup link.
* It returns 0 on success.
*/
static int
dwc_dev_start(struct rte_eth_dev* dev)
{
    struct xlgmac_hw_features* hw =
        DWC_DEV_PRIVATE_TO_HW(dev->data->dev_private);
    struct rte_pci_device* pci_dev = RTE_ETH_DEV_TO_PCI(dev);
    struct rte_intr_handle* intr_handle = &pci_dev->intr_handle;
    uint32_t intr_vector = 0;
    int err, link_up = 0, negotiate = 0;
    uint32_t speed = 0;
    uint32_t allowed_speeds = 0;
    int mask = 0;
    int status;
    uint16_t vf, idx;
    uint32_t* link_speeds;
    struct dwc_tm_conf* tm_conf =
        IXGBE_DEV_PRIVATE_TO_TM_CONF(dev->data->dev_private);

    PMD_INIT_FUNC_TRACE();

    /* Stop the link setup handler before resetting the HW. */
    rte_eal_alarm_cancel(ixgbe_dev_setup_link_alarm_handler, dev);


}


static void
dwc_dev_setup_link_alarm_handler(void* param)
{
    struct rte_eth_dev* dev = (struct rte_eth_dev*)param;
    struct xlgmac_hw_features* hw = DWC_DEV_PRIVATE_TO_HW(dev->data->dev_private);
    struct dwc_interrupt* intr =
        DWC_DEV_PRIVATE_TO_INTR(dev->data->dev_private);
    u32 speed;
    bool autoneg = false;

    speed = hw->phy.autoneg_advertised;
    if (!speed)
        ixgbe_get_link_capabilities(hw, &speed, &autoneg);

    ixgbe_setup_link(hw, speed, true);

    intr->flags &= ~IXGBE_FLAG_NEED_LINK_CONFIG;
}


static int
dwc_check_mq_mode(struct ete_eth_dev* dev)
{
    struct rte_eth_conf* dev_conf = &dev->data->dev_conf;
    struct ixgbe_hw* hw = IXGBE_DEV_PRIVATE_TO_HW(dev->data->dev_private);
    uint16_t nb_rx_q = dev->data->nb_rx_queues;
    uint16_t nb_tx_q = dev->data->nb_tx_queues;
    return 0;
}



static int
eth_ixgbe_dev_init(struct rte_eth_dev* eth_dev, void* init_params __rte_unused)
{
    struct rte_pci_device* pci_dev = RTE_ETH_DEV_TO_PCI(eth_dev);
    struct rte_intr_handle* intr_handle = &pci_dev->intr_handle;
    struct ixgbe_hw* hw =
        IXGBE_DEV_PRIVATE_TO_HW(eth_dev->data->dev_private);
    struct ixgbe_vfta* shadow_vfta =
        IXGBE_DEV_PRIVATE_TO_VFTA(eth_dev->data->dev_private);
    struct ixgbe_hwstrip* hwstrip =
        IXGBE_DEV_PRIVATE_TO_HWSTRIP_BITMAP(eth_dev->data->dev_private);
    struct ixgbe_dcb_config* dcb_config =
        IXGBE_DEV_PRIVATE_TO_DCB_CFG(eth_dev->data->dev_private);
    struct ixgbe_filter_info* filter_info =
        IXGBE_DEV_PRIVATE_TO_FILTER_INFO(eth_dev->data->dev_private);
    struct ixgbe_bw_conf* bw_conf =
        IXGBE_DEV_PRIVATE_TO_BW_CONF(eth_dev->data->dev_private);
    uint32_t ctrl_ext;
    uint16_t csum;
    int diag, i;

    PMD_INIT_FUNC_TRACE();

    eth_dev->dev_ops = &dwc_eth_dev_ops;
    eth_dev->rx_pkt_burst = &dwc_recv_pkts;
    eth_dev->tx_pkt_burst = &dwc_xmit_pkts;
    eth_dev->tx_pkt_prepare = &dwc_prep_pkts;
}



static int eth_dwc_pci_probe(struct rte_pci_driver* pci_drv __rte_unused,struct rte_pci_device* pci_dev)
{
    char name[RTE_ETH_NAME_MAX_LEN];
    struct rte_eth_dev* pf_ethdev;
    struct rte_eth_devargs eth_da;
    int i, retval;

    if (pci_dev->device.devargs) {
        retval = rte_eth_devargs_parse(pci_dev->device.devargs->args,
            &eth_da);
        if (retval)
            return retval;
    }
    else
        memset(&eth_da, 0, sizeof(eth_da));
    
    retval = rte_eth_dev_create(&pci_dev->device, 
        pci_dev->device.name,
        sizeof(struct ixgbe_adapter),
        eth_dev_pci_specific_init, pci_dev,
        eth_dwc_dev_init, NULL);

    return 0;
}


static struct rte_pci_driver rte_dwc_pmd = {
        .id_table = pci_id_ixgbe_map,
        .drv_flags = RTE_PCI_DRV_NEED_MAPPING | RTE_PCI_DRV_INTR_LSC |
                     RTE_PCI_DRV_IOVA_AS_VA,
        .probe = eth_dwc_pci_probe,
        .remove = eth_ixgbe_pci_remove,
};


RTE_PMD_REGISTER_PCI(net_dwc, rte_dwc_pmd);