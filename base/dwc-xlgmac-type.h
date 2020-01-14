#define dwc_call_func(hw, func, params, error) \
                (func != NULL) ? func params : error


typedef u32 dwc_link_speed;