#include "internal.h"
#include "hevc_cabac.h"

extern const uint8_t LENT_cabac_context_init_I_091[MAX_CABAC_STATE];
extern const uint8_t LENT_cabac_context_init_P_091[MAX_CABAC_STATE];
extern const uint8_t LENT_cabac_context_init_B_091[MAX_CABAC_STATE];
int decode_vps_091(HEVCContext *h);