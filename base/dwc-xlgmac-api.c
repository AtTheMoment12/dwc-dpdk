#include "dwc-xlgmac.h"
#include "dwc-xlgmac-type.h"
/**
 *  ixgbe_setup_link - Set link speed
 *  @hw: pointer to hardware structure
 *  @speed: new link speed
 *  @autoneg_wait_to_complete: true when waiting for completion is needed
 *
 *  Configures link settings.  Restarts the link.
 *  Performs autonegotiation if needed.
 **/
s32 dwc_setup_link(struct xlgmac_hw_features* hw, dwc_link_speed speed,
    bool autoneg_wait_to_complete)
{
    return dwc_call_func(hw, hw->mac.ops.setup_link, (hw, speed,
        autoneg_wait_to_complete),
        IXGBE_NOT_IMPLEMENTED);
}


/**
 *  ixgbe_get_link_capabilities - Returns link capabilities
 *  @hw: pointer to hardware structure
 *  @speed: link speed capabilities
 *  @autoneg: true when autoneg or autotry is enabled
 *
 *  Determines the link capabilities of the current configuration.
 **/
s32 ixgbe_get_link_capabilities(struct ixgbe_hw* hw, ixgbe_link_speed* speed,
    bool* autoneg)
{
    return ixgbe_call_func(hw, hw->mac.ops.get_link_capabilities, (hw,
        speed, autoneg), IXGBE_NOT_IMPLEMENTED);
}
