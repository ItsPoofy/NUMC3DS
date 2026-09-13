#include "../command_player_list_service.h"
#include "../../internal.h"

typedef void (*LevelForEachPlayerFn)(void *,void *);

typedef struct {
    char *out;
    unsigned capacity;
    unsigned length;
    int count;
} PlayerListState;

typedef struct {
    PlayerListState *state;
    u32 reserved_04;
    u32 reserved_08;
    int (*accept)(void *,void *);
} PlayerListVisitor;

static int append_player(void *visitor,void *player){
    PlayerListVisitor *closure=(PlayerListVisitor *)visitor;PlayerListState *state=closure->state;char name[48];
    if(!state||!player)return 1;
    entity_name(player,name,sizeof(name));
    if(state->count)append(state->out,&state->length,", ");
    append(state->out,&state->length,name);state->count++;
    return 1;
}

int command_collect_player_names(void *level,char *out,unsigned capacity,int *count){
    PlayerListState state;PlayerListVisitor visitor;
    if(!level||!out||capacity<MAX_TEXT+1||!count)return 0;
    out[0]=0;zero(&state,sizeof(state));zero(&visitor,sizeof(visitor));state.out=out;state.capacity=capacity;visitor.state=&state;visitor.accept=append_player;
    ((LevelForEachPlayerFn)SEAM_Level_forEachPlayer)(level,&visitor);
    *count=state.count;return 1;
}
