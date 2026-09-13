#include "../internal.h"
int cmd_debug(void*player,void*level,void*game,const char*p){
    char dbg[128];unsigned dn=0;
    void *dim,*src,*mg;
    (void)p;(void)game;
    mg=s?s->host.minecraft_game:0;

    /* Dump MinecraftGame pointer chain */
    dbg[0]=0;dn=0;
    append(dbg,&dn,"[dbg] mcg=");append_int(dbg,&dn,(int)(unsigned)mg);
    if(mg){
        void **wb=*(void***)((unsigned char*)mg+0x30);
        void **we=*(void***)((unsigned char*)mg+0x34);
        append(dbg,&dn," +30=");append_int(dbg,&dn,(int)(unsigned)wb);
        append(dbg,&dn," +34=");append_int(dbg,&dn,(int)(unsigned)we);
        /* Scan first 0x100 bytes of MinecraftGame for non-zero pointer pairs */
        {
            unsigned off;
            for(off=0x20;off<0x100;off+=4){
                void *a=*(void**)((unsigned char*)mg+off);
                void *b=*(void**)((unsigned char*)mg+off+4);
                if(a&&b&&a<b&&((unsigned)b-(unsigned)a)<0x10000){
                    dn=0;dbg[0]=0;
                    append(dbg,&dn,"[dbg] mcg+");append_int(dbg,&dn,(int)off);
                    append(dbg,&dn," vec begin=");append_int(dbg,&dn,(int)(unsigned)a);
                    append(dbg,&dn," end=");append_int(dbg,&dn,(int)(unsigned)b);
                    s->host.debug_string(dbg,dn);
                }
            }
        }
    }
    s->host.debug_string(dbg,dn);

    /* Dump Level pointer chain */
    dbg[0]=0;dn=0;
    append(dbg,&dn,"[dbg] lvl=");append_int(dbg,&dn,(int)(unsigned)level);
    s->host.debug_string(dbg,dn);

    dim=(void*)((GetDimensionFn)0x00720854u)(level,0);
    dbg[0]=0;dn=0;
    append(dbg,&dn,"[dbg] dim0=");append_int(dbg,&dn,(int)(unsigned)dim);
    if(dim){
        /* Scan first 0x200 bytes of Dimension for non-zero pointer pairs */
        unsigned off;
        for(off=0x40;off<0x200;off+=4){
            void *a=*(void**)((unsigned char*)dim+off);
            void *b=*(void**)((unsigned char*)dim+off+4);
            if(a&&b&&a<b&&((unsigned)b-(unsigned)a)<0x10000&&(unsigned)a>0x00100000){
                dn=0;dbg[0]=0;
                append(dbg,&dn,"[dbg] dim+");append_int(dbg,&dn,(int)off);
                append(dbg,&dn," vec begin=");append_int(dbg,&dn,(int)(unsigned)a);
                append(dbg,&dn," end=");append_int(dbg,&dn,(int)(unsigned)b);
                s->host.debug_string(dbg,dn);
            }
        }
        /* Also dump some individual pointers */
        for(off=0x40;off<0x100;off+=0x10){
            void *v=*(void**)((unsigned char*)dim+off);
            if(v&&(unsigned)v>0x00100000&&(unsigned)v<0xFF000000){
                dn=0;dbg[0]=0;
                append(dbg,&dn,"[dbg] dim+");append_int(dbg,&dn,(int)off);
                append(dbg,&dn,"=");append_int(dbg,&dn,(int)(unsigned)v);
                s->host.debug_string(dbg,dn);
            }
        }
    }
    s->host.debug_string(dbg,dn);

    src=world_source(level);
    dbg[0]=0;dn=0;
    append(dbg,&dn,"[dbg] bsrc=");append_int(dbg,&dn,(int)(unsigned)src);
    s->host.debug_string(dbg,dn);

    /* Scan Level object for vector pairs (begin<end, heap range) that could
       be entity lists. Level is large so scan first 0x400 bytes. */
    if(level){
        unsigned off;
        for(off=0x10;off<0x400;off+=4){
            void *a=*(void**)((unsigned char*)level+off);
            void *b=*(void**)((unsigned char*)level+off+4);
            if(a&&b&&a<b&&((unsigned)b-(unsigned)a)>=4&&((unsigned)b-(unsigned)a)<0x10000
               &&(unsigned)a>0x00100000&&(unsigned)a<0xFF000000){
                /* Count elements */
                unsigned count=(unsigned)((unsigned)b-(unsigned)a)/4;
                dn=0;dbg[0]=0;
                append(dbg,&dn,"[dbg] lvl+");append_int(dbg,&dn,(int)off);
                append(dbg,&dn," vec n=");append_int(dbg,&dn,(int)count);
                append(dbg,&dn," a=");append_int(dbg,&dn,(int)(unsigned)a);
                s->host.debug_string(dbg,dn);
            }
        }
        /* Also dump individual non-zero pointers at key offsets */
        for(off=0x10;off<0x200;off+=8){
            void *v=*(void**)((unsigned char*)level+off);
            if(v&&(unsigned)v>0x00100000&&(unsigned)v<0xFF000000){
                dn=0;dbg[0]=0;
                append(dbg,&dn,"[dbg] lvl+");append_int(dbg,&dn,(int)off);
                append(dbg,&dn,"=");append_int(dbg,&dn,(int)(unsigned)v);
                s->host.debug_string(dbg,dn);
            }
        }
    }

    result_text("Debug dumped to log");
    return 1;
}
