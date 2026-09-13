#include "../internal.h"
#include "command_conformance.h"
#include "native_callbacks.h"
#include "native_command_context.h"
#include "native_property_bag.h"
#include "native_registry.h"
#include "entity_teleport.h"
#include "weather_command_service.h"
#include "command_packets.h"
#include "command_test_runner.h"
#include "entity_damage_service.h"

enum { EXPECT_FAILURE=0,EXPECT_SUCCESS=1,EXPECT_EITHER=2 };
enum { TEST_FIXED=0,TEST_SETBLOCK,TEST_FILL,TEST_CLONE,TEST_TESTFORBLOCK,TEST_TESTFORBLOCKS,TEST_KILL };

typedef struct {
    const char *name;
    const char *line;
    unsigned char expected;
    unsigned char kind;
} CommandTest;

typedef struct {
    void *source;
    int position[4][3];
    const char *block_name;
    unsigned char block;
    unsigned char data;
    int available;
} CommandFixture;

typedef struct {
    int time;
    int difficulty;
    void *daylight_rule;
    int daylight;
    int raining;
    int thunder;
    int weather_duration;
} WorldSnapshot;

typedef struct {
    float position[3];
    int permission;
    int game_mode;
} PlayerSnapshot;

typedef struct {
    void *player;
    void *level;
    void *game;
    unsigned first;
    unsigned index;
    unsigned last;
    unsigned passed;
    unsigned mismatches;
    int single;
    int active;
    CommandFixture fixture;
} CommandTestRun;

static CommandTestRun active_run;

static const CommandTest command_tests[] = {
    {"help","/help",EXPECT_SUCCESS,TEST_FIXED},
    {"time add","/time add 1",EXPECT_SUCCESS,TEST_FIXED},
    {"time query","/time query daytime",EXPECT_SUCCESS,TEST_FIXED},
    {"time set","/time set day",EXPECT_SUCCESS,TEST_FIXED},
    {"list","/list",EXPECT_SUCCESS,TEST_FIXED},
    {"gamemode","/gamemode survival @p",EXPECT_SUCCESS,TEST_FIXED},
    {"difficulty","/difficulty normal",EXPECT_SUCCESS,TEST_FIXED},
    {"setworldspawn","/setworldspawn ~ ~ ~",EXPECT_SUCCESS,TEST_FIXED},
    {"say","/say numc3ds-test",EXPECT_SUCCESS,TEST_FIXED},
    {"me","/me numc3ds-test",EXPECT_SUCCESS,TEST_FIXED},
    {"clear","/clear @p stone 0 0",EXPECT_SUCCESS,TEST_FIXED},
    {"xp","/xp 0 @p",EXPECT_SUCCESS,TEST_FIXED},
    {"gamerule","/gamerule doDaylightCycle",EXPECT_SUCCESS,TEST_FIXED},
    {"daylock","/daylock false",EXPECT_FAILURE,TEST_FIXED},
    {"weather","/weather clear 1",EXPECT_SUCCESS,TEST_FIXED},
    {"toggledownfall","/toggledownfall",EXPECT_SUCCESS,TEST_FIXED},
    {"setblock",0,EXPECT_SUCCESS,TEST_SETBLOCK},
    {"fill",0,EXPECT_SUCCESS,TEST_FILL},
    {"clone",0,EXPECT_SUCCESS,TEST_CLONE},
    {"testforblock",0,EXPECT_SUCCESS,TEST_TESTFORBLOCK},
    {"testforblocks",0,EXPECT_SUCCESS,TEST_TESTFORBLOCKS},
    {"locate","/locate village",EXPECT_EITHER,TEST_FIXED},
    {"give","/give @p stone 1",EXPECT_SUCCESS,TEST_FIXED},
    {"effect","/effect @p speed 1",EXPECT_SUCCESS,TEST_FIXED},
    {"enchant","/enchant @p 0 1",EXPECT_SUCCESS,TEST_FIXED},
    {"tell","/tell @p numc3ds-test",EXPECT_SUCCESS,TEST_FIXED},
    {"title","/title @p actionbar numc3ds-test",EXPECT_SUCCESS,TEST_FIXED},
    {"playsound","/playsound random.click @p",EXPECT_SUCCESS,TEST_FIXED},
    {"testfor","/testfor @p",EXPECT_SUCCESS,TEST_FIXED},
    {"summon","/summon pig ~4 ~ ~",EXPECT_SUCCESS,TEST_FIXED},
    {"spawnpoint","/spawnpoint @p",EXPECT_SUCCESS,TEST_FIXED},
    {"op","/op @p",EXPECT_SUCCESS,TEST_FIXED},
    {"deop","/deop @p",EXPECT_SUCCESS,TEST_FIXED},
    {"execute","/execute @p ~ ~ ~ time query daytime",EXPECT_SUCCESS,TEST_FIXED},
    {"stopsound","/stopsound @p",EXPECT_SUCCESS,TEST_FIXED},
    {"replaceitem","/replaceitem entity @p slot.hotbar 0 stone 1",EXPECT_SUCCESS,TEST_FIXED},
    {"tp","/tp @p @p",EXPECT_SUCCESS,TEST_FIXED},
    {"teleport","/teleport @p @p",EXPECT_SUCCESS,TEST_FIXED},
    {"alwaysday","/alwaysday false",EXPECT_FAILURE,TEST_FIXED},
    {"w","/w @p numc3ds-alias-w",EXPECT_SUCCESS,TEST_FIXED},
    {"msg","/msg @p numc3ds-alias-msg",EXPECT_SUCCESS,TEST_FIXED},
    {"gamemode_all","/gamemode survival @a",EXPECT_SUCCESS,TEST_FIXED},
    {"clear_all","/clear @a stone 0 0",EXPECT_SUCCESS,TEST_FIXED},
    {"tp_all","/tp @a ~ ~ ~",EXPECT_SUCCESS,TEST_FIXED},
    {"kill","/kill @p",EXPECT_SUCCESS,TEST_KILL},
};

static int floor_block(float value){int whole=(int)value;return value<(float)whole?whole-1:whole;}

static void log_text(const char *prefix,const char *text){
    char line[MAX_TEXT+1];unsigned length=0;
    line[0]=0;append(line,&length,prefix);append(line,&length,text?text:"");
    s->host.debug_string(line,length);
}

static void test_log(unsigned index,const char *state,const CommandTest *test,const char *line){
    char output[MAX_TEXT+1];unsigned length=0;
    output[0]=0;append(output,&length,"[cmdtest ");append_int(output,&length,(int)index);append(output,&length,"] ");
    append(output,&length,state);append(output,&length," ");append(output,&length,test->name);
    append(output,&length," :: ");append(output,&length,line);
    s->host.debug_string(output,length);
}

static int fixture_find(CommandFixture *fixture,void *player,void *level,const NativeCommandContext *context){
    int dx,dy,dz,index,base[3];void **vtable;
    zero(fixture,sizeof(*fixture));
    fixture->source=native_command_context_block_source();
    fixture->block_name="wool";fixture->data=3;
    if(!fixture->source||!resolve_block_arg("wool",&fixture->block))return 0;
    vtable=context&&context->origin?*(void***)context->origin:0;
    if(vtable&&vtable[4])((void(*)(int*,void*))vtable[4])(base,context->origin);
    else for(index=0;index<3;index++)base[index]=floor_block(entity_pos(player,index));
    for(dy=3;dy<=12;dy++)for(dx=4;dx<=20;dx++)for(dz=-4;dz<=4;dz++){
        int clear=1;
        for(index=0;index<4;index++){
            fixture->position[index][0]=base[0]+dx+index;
            fixture->position[index][1]=base[1]+dy;
            fixture->position[index][2]=base[2]+dz;
            if(fixture->position[index][1]<1||fixture->position[index][1]>254||command_block_read_id(fixture->source,fixture->position[index])!=0)clear=0;
        }
        if(clear){fixture->available=1;return 1;}
    }
    return 0;
}

static void fixture_clear(CommandFixture *fixture){
    int index;
    if(!fixture->available)return;
    for(index=0;index<4;index++)command_block_set(fixture->source,fixture->position[index],0,0);
}

static void fixture_prepare(CommandFixture *fixture,unsigned kind){
    fixture_clear(fixture);
    if(kind==TEST_CLONE||kind==TEST_TESTFORBLOCK||kind==TEST_TESTFORBLOCKS)command_block_set(fixture->source,fixture->position[0],fixture->block,fixture->data);
    if(kind==TEST_CLONE||kind==TEST_TESTFORBLOCKS)command_block_set(fixture->source,fixture->position[1],fixture->block,fixture->data);
    if(kind==TEST_TESTFORBLOCKS){command_block_set(fixture->source,fixture->position[2],fixture->block,fixture->data);command_block_set(fixture->source,fixture->position[3],fixture->block,fixture->data);}
}

static void append_fixture_position(char *line,unsigned *length,const CommandFixture *fixture,int index){
    append_int(line,length,fixture->position[index][0]);append(line,length," ");
    append_int(line,length,fixture->position[index][1]);append(line,length," ");
    append_int(line,length,fixture->position[index][2]);
}

static void append_fixture_block(char *line,unsigned *length,const CommandFixture *fixture){
    append(line,length," ");append(line,length,fixture->block_name);append(line,length," ");append_int(line,length,fixture->data);
}

static int build_line(const CommandTest *test,const CommandFixture *fixture,char line[MAX_TEXT+1]){
    unsigned length=0;
    line[0]=0;
    if(test->kind==TEST_FIXED){copy_text(line,test->line,MAX_TEXT);return 1;}
    if(test->kind==TEST_KILL){copy_text(line,"/kill @p",MAX_TEXT);return 1;}
    if(!fixture->available)return 0;
    if(test->kind==TEST_SETBLOCK){append(line,&length,"/setblock ");append_fixture_position(line,&length,fixture,0);append_fixture_block(line,&length,fixture);}
    else if(test->kind==TEST_FILL){append(line,&length,"/fill ");append_fixture_position(line,&length,fixture,0);append(line,&length," ");append_fixture_position(line,&length,fixture,1);append_fixture_block(line,&length,fixture);}
    else if(test->kind==TEST_CLONE){append(line,&length,"/clone ");append_fixture_position(line,&length,fixture,0);append(line,&length," ");append_fixture_position(line,&length,fixture,1);append(line,&length," ");append_fixture_position(line,&length,fixture,2);}
    else if(test->kind==TEST_TESTFORBLOCK){append(line,&length,"/testforblock ");append_fixture_position(line,&length,fixture,0);append_fixture_block(line,&length,fixture);}
    else if(test->kind==TEST_TESTFORBLOCKS){append(line,&length,"/testforblocks ");append_fixture_position(line,&length,fixture,0);append(line,&length," ");append_fixture_position(line,&length,fixture,1);append(line,&length," ");append_fixture_position(line,&length,fixture,2);}
    else return 0;
    return 1;
}

static int fixture_matches(const CommandFixture *fixture,unsigned kind){
    unsigned char expected[4]={0,0,0,0};unsigned char expected_data[4]={0,0,0,0};int index;
    if(kind==TEST_SETBLOCK||kind==TEST_FILL||kind==TEST_CLONE||kind==TEST_TESTFORBLOCK||kind==TEST_TESTFORBLOCKS){expected[0]=fixture->block;expected_data[0]=fixture->data;}
    if(kind==TEST_FILL||kind==TEST_CLONE||kind==TEST_TESTFORBLOCKS){expected[1]=fixture->block;expected_data[1]=fixture->data;}
    if(kind==TEST_CLONE||kind==TEST_TESTFORBLOCKS){expected[2]=fixture->block;expected[3]=fixture->block;expected_data[2]=fixture->data;expected_data[3]=fixture->data;}
    for(index=0;index<4;index++){
        unsigned char actual=command_block_read_id(fixture->source,fixture->position[index]);
        unsigned char actual_data=command_block_read_data(fixture->source,fixture->position[index]);
        if(actual!=expected[index]||actual_data!=expected_data[index]){
            char output[MAX_TEXT+1];unsigned length=0;
            output[0]=0;append(output,&length,"[cmdtest] block[");append_int(output,&length,index);append(output,&length,"] at ");
            append_int(output,&length,fixture->position[index][0]);append(output,&length,",");append_int(output,&length,fixture->position[index][1]);append(output,&length,",");append_int(output,&length,fixture->position[index][2]);
            append(output,&length," expected=");append_int(output,&length,expected[index]);append(output,&length,":");append_int(output,&length,expected_data[index]);
            append(output,&length," actual=");append_int(output,&length,actual);append(output,&length,":");append_int(output,&length,actual_data);
            s->host.debug_string(output,length);return 0;
        }
    }
    return 1;
}

static void snapshot_world(WorldSnapshot *snapshot,void *level){
    void *rules=((void*(*)(void*))SEAM_Level_getGameRules)(level);
    snapshot->time=((LevelGetInt)SEAM_Level_getTime)(level);
    snapshot->difficulty=((LevelGetInt)SEAM_Level_getDifficulty)(level);
    snapshot->daylight_rule=rules?rule_dfs(rules_root(rules),"dodaylightcycle"):0;
    snapshot->daylight=snapshot->daylight_rule?rule_value_bool(snapshot->daylight_rule):0;
    snapshot->raining=*(const float *)((const u8 *)level+SEAM_Level_liveRainLevel)>0.0f;
    snapshot->thunder=*(const float *)((const u8 *)level+SEAM_Level_liveLightningLevel)>0.0f;
    snapshot->weather_duration=*(const int *)((const u8 *)level+SEAM_Level_liveRainTime);
}

static void restore_world(const WorldSnapshot *snapshot,void *level,void *game){
    void *options=((MinecraftGameGetOptionsFn)SEAM_MinecraftGame_getOptions)(game);
    void **vtable=*(void***)level;
    if(((LevelGetInt)SEAM_Level_getTime)(level)!=snapshot->time){((LevelSetInt)SEAM_Level_setTime)(level,snapshot->time);command_packet_broadcast_time(level,snapshot->time);}
    if(((LevelGetInt)SEAM_Level_getDifficulty)(level)!=snapshot->difficulty){
        if(vtable)((LevelSetInt)vtable[SEAM_Level_setDifficultyVtableOffset/4])(level,snapshot->difficulty);
        if(options)((OptionsSetIntFn)SEAM_Options_setInt)(options,(const void*)SEAM_Options_difficulty,snapshot->difficulty);
    }
    if(snapshot->daylight_rule&&rule_value_bool(snapshot->daylight_rule)!=snapshot->daylight)((RuleSetBoolFn)SEAM_GameRule_setBool)(snapshot->daylight_rule,snapshot->daylight);
    if((*(const float *)((const u8 *)level+SEAM_Level_liveRainLevel)>0.0f)!=snapshot->raining||
       (*(const float *)((const u8 *)level+SEAM_Level_liveLightningLevel)>0.0f)!=snapshot->thunder||
       *(const int *)((const u8 *)level+SEAM_Level_liveRainTime)!=snapshot->weather_duration)
        command_weather_apply(level,snapshot->raining,snapshot->thunder,snapshot->weather_duration);
}

static void snapshot_player(PlayerSnapshot *snapshot,void *player){
    unsigned index;
    for(index=0;index<3;index++)snapshot->position[index]=entity_pos(player,index);
    snapshot->permission=((GetPermissionsLevel)SEAM_Player_getPermissionsLevel)(player);
    snapshot->game_mode=*(int*)((u8*)player+0x196c);
}

static void restore_player(const PlayerSnapshot *snapshot,void *player){
    if(*(int*)((u8*)player+0x196c)!=snapshot->game_mode)((PlayerSetGameType)SEAM_Player_setGameType)(player,snapshot->game_mode);
    if(((GetPermissionsLevel)SEAM_Player_getPermissionsLevel)(player)!=snapshot->permission)((SetPermissionsLevel)SEAM_Player_setPermissionsLevel)(player,snapshot->permission);
    if(entity_pos(player,0)!=snapshot->position[0]||entity_pos(player,1)!=snapshot->position[1]||entity_pos(player,2)!=snapshot->position[2])native_entity_teleport(player,snapshot->position);
}

static int run_test(unsigned index,const CommandTest *test,CommandFixture *fixture,void *player,void *level,void *game,int single){
    char line[MAX_TEXT+1];int actual,result_match,side_effect_match=1,pass;
    const char *trace;
    unsigned output_index;
    if(test->kind>=TEST_SETBLOCK&&test->kind<=TEST_TESTFORBLOCKS)fixture_prepare(fixture,test->kind);
    if(!build_line(test,fixture,line)){
        test_log(index,"MISMATCH fixture-unavailable",test,"<none>");
        active_run.mismatches++;
        {
            char diag[MAX_TEXT+1];unsigned dlen=0;
            diag[0]=0;
            append(diag,&dlen,test->name);
            append(diag,&dlen,": - FAIL; ");
            append_int(diag,&dlen,(int)active_run.passed);
            append(diag,&dlen,"/");
            append_int(diag,&dlen,(int)(index+1-active_run.first));
            history_add(0,"",diag);
        }
        return 1;
    }
    test_log(index,"RUN",test,line);
    native_command_trace_begin();
    command_conformance_capture_begin();
    actual=native_command_execute_line(player,level,game,line);
    command_conformance_capture_end();
    trace=native_command_trace_end();
    result_match=test->expected==EXPECT_EITHER||actual==(int)test->expected;
    if(actual&&test->kind>=TEST_SETBLOCK&&test->kind<=TEST_TESTFORBLOCKS)side_effect_match=fixture_matches(fixture,test->kind);
    if(actual&&test->kind==TEST_KILL)side_effect_match=!command_entity_is_alive(player);
    pass=result_match&&side_effect_match;
    if(pass){
        active_run.passed++;
        test_log(index,"PASS",test,line);
    }else{
        char mismatch[96];unsigned length=0;
        active_run.mismatches++;
        mismatch[0]=0;append(mismatch,&length,"[cmdtest] MISMATCH expected=");
        append(mismatch,&length,test->expected==EXPECT_SUCCESS?"success":test->expected==EXPECT_FAILURE?"failure":"success-or-failure");
        append(mismatch,&length," actual=");append(mismatch,&length,actual?"success":"failure");
        if(!side_effect_match)append(mismatch,&length," side-effect-mismatch");
        log_text(mismatch,"");
        for(output_index=0;output_index<command_conformance_capture_count();output_index++)log_text("[cmdtest] actual-output: ",command_conformance_capture_message(output_index));
    }
    {
        char diag[MAX_TEXT+1];unsigned dlen=0;
        diag[0]=0;
        append(diag,&dlen,test->name);
        append(diag,&dlen,": ");
        append(diag,&dlen,trace&&trace[0]?trace:"-");
        append(diag,&dlen,pass?" PASS; ":" FAIL; ");
        append_int(diag,&dlen,(int)active_run.passed);
        append(diag,&dlen,"/");
        append_int(diag,&dlen,(int)(index+1-active_run.first));
        history_add(0,"",diag);
    }
    if(test->kind>=TEST_SETBLOCK&&test->kind<=TEST_TESTFORBLOCKS)fixture_clear(fixture);
    (void)single;
    return pass?0:1;
}

int cmd_testall(void *player,void *level,void *game,const char *args){
    const NativeCommandContext *context=native_command_context_current();
    unsigned first=0,last=sizeof(command_tests)/sizeof(command_tests[0]);int requested,single=0;
    if(active_run.active)return fail_command("Command conformance is already running");
    if(context&&native_bag_get_int(context->input_bag,"index",&requested)){
        if(requested<0||(unsigned)requested>=last)return fail_syntax();
        first=(unsigned)requested;last=first+1;single=1;
    }else if(*spaces(args)){
        if(!integer(&args,&requested)||*spaces(args)||requested<0||(unsigned)requested>=last)return fail_syntax();
        first=(unsigned)requested;last=first+1;single=1;
    }
    zero(&active_run,sizeof(active_run));
    active_run.player=player;active_run.level=level;active_run.game=game;active_run.first=first;active_run.index=first;active_run.last=last;active_run.passed=0;active_run.mismatches=0;active_run.single=single;active_run.active=1;
    fixture_find(&active_run.fixture,player,level,context);
    result_text("Command conformance scheduled");
    return 1;
}

void command_testall_tick(void){
    const CommandTest *test;WorldSnapshot snapshot;PlayerSnapshot player_snapshot;char summary[MAX_TEXT+1];unsigned length=0;
    if(!active_run.active)return;
    if(active_run.index>=active_run.last){
        fixture_clear(&active_run.fixture);
        summary[0]=0;
        append(summary,&length,"Conformance complete: ");
        append_int(summary,&length,(int)active_run.passed);
        append(summary,&length,"/");
        append_int(summary,&length,(int)(active_run.last-active_run.first));
        append(summary,&length," passed (");
        append_int(summary,&length,(int)active_run.mismatches);
        append(summary,&length," mismatches)");
        active_run.active=0;
        result_text(summary);
        return;
    }
    test=&command_tests[active_run.index];
    snapshot_world(&snapshot,active_run.level);
    if(test->kind!=TEST_KILL)snapshot_player(&player_snapshot,active_run.player);
    run_test(active_run.index,test,&active_run.fixture,active_run.player,active_run.level,active_run.game,active_run.single);
    restore_world(&snapshot,active_run.level,active_run.game);
    if(test->kind!=TEST_KILL)restore_player(&player_snapshot,active_run.player);
    else active_run.player=0;
    active_run.index++;
}
