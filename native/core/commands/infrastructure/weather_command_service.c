#include "../weather_command_service.h"
#include "../../internal.h"
#include "../command_random.h"

typedef void (*WeatherBroadcastFn)(void *,u32,const float *,int,int);

void *command_weather_get(void *level){
    void *dimension;
    if(!level)return 0;
    dimension=((GetDimensionFn)SEAM_Level_getDimension)(level,0);
    return dimension?*(void **)((u8 *)dimension+SEAM_Dimension_weatherOffset):0;
}

static void command_weather_set_live_state(void *weather,float rain,float lightning){
    if(!weather)return;
    *(float *)((u8 *)weather+0x20)=rain;
    *(float *)((u8 *)weather+0x2c)=lightning;
}

void command_weather_apply(void *level,int raining,int thunder,int duration_ticks){
    u8 *live=(u8 *)level,*level_data;float rain=raining?1.0f:0.0f,lightning=thunder?1.0f:0.0f;
    if(!level)return;
    level_data=live+SEAM_Level_levelDataOffset;
    *(float *)(live+SEAM_Level_liveRainLevel)=rain;*(int *)(live+SEAM_Level_liveRainTime)=duration_ticks;
    *(float *)(live+SEAM_Level_liveLightningLevel)=lightning;*(int *)(live+SEAM_Level_liveLightningTime)=duration_ticks;
    *(float *)(level_data+0xa8)=rain;*(int *)(level_data+0xac)=duration_ticks;
    *(float *)(level_data+0xb0)=lightning;*(int *)(level_data+0xb4)=duration_ticks;
    command_weather_set_live_state(command_weather_get(level),rain,lightning);
}

void command_weather_broadcast(void *level,int raining,int thunder){
    static const float zero_pos[3]={0.0f,0.0f,0.0f};
    typedef void (*broadcast_fn)(void *,int,const float *,int,void *);
    broadcast_fn broadcast=(broadcast_fn)0x005C9F48u;
    if(!level)return;
    if(raining){
        broadcast(level,3001,zero_pos,65535,0);
    }else{
        broadcast(level,3003,zero_pos,0,0);
    }
    if(thunder){
        broadcast(level,3002,zero_pos,65535,0);
    }else{
        broadcast(level,3004,zero_pos,0,0);
    }
}

int command_weather_toggle_downfall(void *level){
    u8 *live,*level_data;void *weather;float rain;int was_raining,rain_time;
    if(!level)return 0;
    live=(u8 *)level;
    level_data=live+SEAM_Level_levelDataOffset;
    weather=command_weather_get(level);
    rain=*(float *)(live+SEAM_Level_liveRainLevel);
    was_raining=rain>0.0f;
    if(!was_raining){
        rain_time=12000+(int)(command_random_next_u32(level)%12000u);
        *(float *)(live+SEAM_Level_liveRainLevel)=1.0f;
        *(int *)(live+SEAM_Level_liveRainTime)=rain_time;
        *(float *)(level_data+0xa8)=1.0f;
        *(int *)(level_data+0xac)=rain_time;
        if(weather)*(float *)((u8 *)weather+0x20)=1.0f;
        command_weather_broadcast(level,1,0);
        return 100;
    }
    rain_time=12000+(int)(command_random_next_u32(level)%168000u);
    *(float *)(live+SEAM_Level_liveRainLevel)=0.0f;
    *(int *)(live+SEAM_Level_liveRainTime)=rain_time;
    *(float *)(live+SEAM_Level_liveLightningLevel)=0.0f;
    *(int *)(live+SEAM_Level_liveLightningTime)=6000+(int)(command_random_next_u32(level)%600u)*20;
    *(float *)(level_data+0xa8)=0.0f;
    *(int *)(level_data+0xac)=rain_time;
    *(float *)(level_data+0xb0)=0.0f;
    *(int *)(level_data+0xb4)=*(int *)(live+SEAM_Level_liveLightningTime);
    if(weather){
        *(float *)((u8 *)weather+0x20)=0.0f;
        *(float *)((u8 *)weather+0x2c)=0.0f;
    }
    command_weather_broadcast(level,0,0);
    return 0;
}
