#include <stdint.h>

#include "base/dwc-xlgmac.h"
#include "base/dwc-xlgmac-reg.h"
#include "base/dwc-bypass.h"

#include <rte_flow.h>
#include <rte_time.h>
#include <rte_hash.h>
#include <rte_pci.h>
#include <rte_bus_pci.h>
#include <rte_tm_driver.h>

/* need update link, bit flag */
#define DWC_FLAG_NEED_LINK_UPDATE (uint32_t)(1 << 0)
#define DWC_FLAG_MAILBOX          (uint32_t)(1 << 1)
#define DWC_FLAG_PHY_INTERRUPT    (uint32_t)(1 << 2)
#define DWC_FLAG_MACSEC           (uint32_t)(1 << 3)
#define DWC_FLAG_NEED_LINK_CONFIG (uint32_t)(1 << 4)



struct dwc_adapter {
    struct dwc_hw             hw;
    struct dwc_hw_stats       stats;
    struct dwc_macsec_stats   macsec_stats;
    struct dwc_hw_fdir_info   fdir;
    struct dwc_interrupt      intr;
    struct dwc_stat_mapping_registers stat_mappings;
    struct dwc_vfta           shadow_vfta;
    struct dwc_hwstrip            hwstrip;
    struct dwc_dcb_config     dcb_config;
    struct dwc_mirror_info    mr_data;
    struct dwc_vf_info* vfdata;
    struct dwc_uta_info       uta_info;
#ifdef RTE_LIBRTE_DWC_BYPASS
    struct dwc_bypass_info    bps;
#endif /* RTE_LIBRTE_DWC_BYPASS */
    struct dwc_filter_info    filter;
    struct dwc_l2_tn_info     l2_tn;
    struct dwc_bw_conf        bw_conf;
#ifdef RTE_LIBRTE_SECURITY
    struct dwc_ipsec          ipsec;
#endif
    bool rx_bulk_alloc_allowed;
    bool rx_vec_allowed;
    struct rte_timecounter      systime_tc;
    struct rte_timecounter      rx_tstamp_tc;
    struct rte_timecounter      tx_tstamp_tc;
    struct dwc_tm_conf        tm_conf;
};


#define DWC_DEV_PRIVATE_TO_INTR(adapter) \
        (&((struct dwc_adapter *)adapter)->intr)
#define DWC_DEV_PRIVATE_TO_TM_CONF(adapter) \
        (&((struct dwc_adapter *)adapter)->tm_conf)



/* structure for interrupt relative data */
struct dwc_interrupt {
    uint32_t flags;
    uint32_t mask;
    /*to save original mask during delayed handler */
    uint32_t mask_original;
};

#define DWC_DEV_PRIVATE_TO_HW(adapter)\ 
(&((struct dwc_adapter*)adapter)->hw)



struct dwc_tm_shaper_profile {
    TAILQ_ENTRY(dwc_tm_shaper_profile) node;
    uint32_t shaper_profile_id;
    uint32_t reference_count;
    struct rte_tm_shaper_params profile;
};

/* Struct to store Traffic Manager node configuration. */
struct dwc_tm_node {
    TAILQ_ENTRY(dwc_tm_node) node;
    uint32_t id;
    uint32_t priority;
    uint32_t weight;
    uint32_t reference_count;
    uint16_t no;
    struct dwc_tm_node* parent;
    struct dwc_tm_shaper_profile* shaper_profile;
    struct rte_tm_node_params params;
};


/* The configuration of Traffic Manager */
struct dwc_tm_conf {
    struct dwc_shaper_profile_list shaper_profile_list;
    struct dwc_tm_node* root; /* root node - port */
    struct dwc_tm_node_list tc_list; /* node list for all the TCs */
    struct dwc_tm_node_list queue_list; /* node list for all the queues */
    /**
     * The number of added TC nodes.
     * It should be no more than the TC number of this port.
     */
    uint32_t nb_tc_node;
    /**
     * The number of added queue nodes.
     * It should be no more than the queue number of this port.
     */
    uint32_t nb_queue_node;
    /**
     * This flag is used to check if APP can change the TM node
     * configuration.
     * When it's true, means the configuration is applied to HW,
     * APP should not change the configuration.
     * As we don't support on-the-fly configuration, when starting
     * the port, APP should call the hierarchy_commit API to set this
     * flag to true. When stopping the port, this flag should be set
     * to false.
     */
    bool committed;
};
