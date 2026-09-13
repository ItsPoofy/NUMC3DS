#include "../internal.h"

/* Safe entity probe: checks vtable is in .text/.rodata range before
   attempting name/position reads. */
static unsigned debug_i;
static void safe_probe(void*ent,const char*label,unsigned idx){
    char dbg[160];unsigned dn=0;
    void *vt;
    if(!ent||(unsigned)ent<0x00100000||(unsigned)ent>0xFF000000){
        dn=0;dbg[0]=0;
        append(dbg,&dn,label);append(dbg,&dn,"[");append_int(dbg,&dn,(int)idx);
        append(dbg,&dn,"]=");append_int(dbg,&dn,(int)(unsigned)ent);
        append(dbg,&dn," (invalid ptr)");
        s->host.debug_string(dbg,dn);
        return;
    }
    vt=*(void**)ent;
    /* Vtable should be in .rodata (0x00919000-0x00A29000) for game objects */
    if((unsigned)vt<0x00100000||(unsigned)vt>0x00B00000){
        dn=0;dbg[0]=0;
        append(dbg,&dn,label);append(dbg,&dn,"[");append_int(dbg,&dn,(int)idx);
        append(dbg,&dn,"] ent=");append_int(dbg,&dn,(int)(unsigned)ent);
        append(dbg,&dn," vt=");append_int(dbg,&dn,(int)(unsigned)vt);
        append(dbg,&dn," (bad vtable)");
        s->host.debug_string(dbg,dn);
        return;
    }
    /* Safe to read position */
    {
        float px=*(float*)((unsigned char*)ent+SEAM_Entity_posOffset);
        float py=*(float*)((unsigned char*)ent+SEAM_Entity_posOffset+4);
        float pz=*(float*)((unsigned char*)ent+SEAM_Entity_posOffset+8);
        dn=0;dbg[0]=0;
        append(dbg,&dn,label);append(dbg,&dn,"[");append_int(dbg,&dn,(int)idx);
        append(dbg,&dn,"] ent=");append_int(dbg,&dn,(int)(unsigned)ent);
        append(dbg,&dn," vt=");append_int(dbg,&dn,(int)(unsigned)vt);
        append(dbg,&dn," pos=");append_int(dbg,&dn,(int)px);
        append(dbg,&dn,",");append_int(dbg,&dn,(int)py);
        append(dbg,&dn,",");append_int(dbg,&dn,(int)pz);
        s->host.debug_string(dbg,dn);
    }
}

int cmd_debug2(void*player,void*level,void*game,const char*p){
    char dbg[160];unsigned dn=0;
    void *src,*dim;
    (void)p;(void)game;

    src=world_source(level);
    if(!src){result_text("No BlockSource");return 1;}
    dim=(void*)((GetDimensionFn)0x00720854u)(level,0);

    /* === 1. Scan Level candidate vectors with safe probing === */
    {
        unsigned offsets[] = {488, 512, 116, 120, 248, 436, 440, 488};
        unsigned oi;
        for(oi=0;oi<8;oi++){
            unsigned off=offsets[oi];
            void **begin=*(void***)((unsigned char*)level+off);
            void **end=*(void***)((unsigned char*)level+off+4);
            unsigned count;
            if(!begin||!end||(unsigned)begin>0xFF000000)continue;
            count=(unsigned)((unsigned)end-(unsigned)begin)/4;
            if(count==0||count>50)continue;
            dn=0;dbg[0]=0;
            append(dbg,&dn,"[dbg2] lvl+");append_int(dbg,&dn,(int)off);
            append(dbg,&dn," n=");append_int(dbg,&dn,(int)count);
            s->host.debug_string(dbg,dn);
            for(unsigned ii=0;ii<count&&ii<10;ii++){
                safe_probe(begin[ii],dbg,0); /* label built inside */
            }
        }
    }

    /* === 2. Scan Dimension broadly (0x40-0x400) for pointer pairs === */
    if(dim){
        unsigned off;
        for(off=0x40;off<0x3F8;off+=4){
            void *a=*(void**)((unsigned char*)dim+off);
            void *b=*(void**)((unsigned char*)dim+off+4);
            /* Look for vectors in the heap range that could be entity lists */
            if(a&&b&&a<b&&((unsigned)b-(unsigned)a)>=4&&((unsigned)b-(unsigned)a)<=0x1000
               &&(unsigned)a>0x30000000&&(unsigned)a<0x38000000){
                unsigned count=(unsigned)((unsigned)b-(unsigned)a)/4;
                dn=0;dbg[0]=0;
                append(dbg,&dn,"[dbg2] dim+");append_int(dbg,&dn,(int)off);
                append(dbg,&dn," n=");append_int(dbg,&dn,(int)count);
                append(dbg,&dn," a=");append_int(dbg,&dn,(int)(unsigned)a);
                s->host.debug_string(dbg,dn);
                /* Safe-probe first few elements */
                {
                    unsigned i;
                    for(i=0;i<count&&i<5;i++){
                        safe_probe(((void**)a)[i],dbg,i);
                    }
                }
            }
        }
    }

    /* === 3. Control: Spawn_ItemEntity (verified working path) === */
    {
        numc3ds_u32 inst[48];
        typedef void *(*ItemCountAuxCtorFn)(void*,void*,int,int);
        ItemCountAuxCtorFn ctor_fn=(ItemCountAuxCtorFn)0x001D2510u;
        void *diamond=item_by_id(264);
        if(diamond){
            typedef void *(*SpawnItemFn)(void*,void*,float,float,float,int);
            SpawnItemFn spawn_item=(SpawnItemFn)0x00645218u;
            float px=entity_pos(player,0),py=entity_pos(player,1)+1,pz=entity_pos(player,2);
            void*item_ent;
            zero(inst,sizeof(inst));
            ctor_fn(inst,diamond,1,0);
            item_ent=spawn_item(level,inst,px,py,pz,0);
            dn=0;dbg[0]=0;
            append(dbg,&dn,"[dbg2] SpawnItem=");
            append_int(dbg,&dn,(int)(unsigned)item_ent);
            s->host.debug_string(dbg,dn);
            if(item_ent){
                safe_probe(item_ent,"[dbg2] spawned_item",0);
            }
        }
    }

    result_text("Debug2 complete — check log");
    return 1;
}
