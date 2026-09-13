#include "../internal.h"
int cmd_testspawn(void*player,void*level,void*game,const char*p){
    char target[48];char out[MAX_TEXT+1];unsigned n=0;
    void*tv[SELECTOR_TARGET_CAPACITY];unsigned nt,i;
    (void)game;
    p=word(p,target,sizeof(target));
    if(!target[0]){tv[0]=player;nt=1;}
    else{
        nt=command_resolve_targets(player,level,game,target,tv,SELECTOR_TARGET_CAPACITY);
        if(!nt){result_text("No targets matched search");return 1;}
    }
    out[0]=0;append(out,&n,"Found ");
    for(i=0;i<nt;i++){
        char pname[32];
        if(i)append(out,&n,", ");
        entity_name(tv[i],pname,sizeof(pname));
        append(out,&n,pname);
    }
    result_text(out);
    return 1;
}
