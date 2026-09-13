#include "../mob_effect_service.h"
#include "../../internal.h"

static int effect_present(const void *effect){return ((int(*)(const void*,const void*))SEAM_MobEffectInstance_operatorNotEqual)(effect,(const void*)SEAM_MobEffectInstance_empty);}
static void effect_clear(void *effect){cp(effect,(const void*)SEAM_MobEffectInstance_empty,0x10);}
static void effect_removed(void *mob,void *effect){void **vtable=mob?*(void***)mob:0;if(vtable&&vtable[SEAM_Mob_onEffectRemovedVtableOffset/sizeof(void*)])((void(*)(void*,void*))vtable[SEAM_Mob_onEffectRemovedVtableOffset/sizeof(void*)])(mob,effect);}

int mob_effect_remove(void *mob,int effect_id){u8 *begin,*end,*effect;if(!mob||effect_id<0)return 0;begin=*(u8**)((u8*)mob+SEAM_Mob_effectVectorBegin);end=*(u8**)((u8*)mob+SEAM_Mob_effectVectorEnd);if(!begin||!end||end<begin||(unsigned)effect_id>=(unsigned)(end-begin)/0x10)return 0;effect=begin+(unsigned)effect_id*0x10;if(!effect_present(effect))return 0;effect_removed(mob,effect);effect_clear(effect);return 1;}

unsigned mob_effect_remove_all(void *mob){u8 *effect,*end;unsigned removed=0;if(!mob)return 0;effect=*(u8**)((u8*)mob+SEAM_Mob_effectVectorBegin);end=*(u8**)((u8*)mob+SEAM_Mob_effectVectorEnd);if(!effect||!end||end<effect||(unsigned)(end-effect)%0x10)return 0;while(effect!=end){if(effect_present(effect)){effect_removed(mob,effect);effect_clear(effect);removed++;}effect+=0x10;}return removed;}
