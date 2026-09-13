#ifndef NUMC3DS_GAME_MODE_SERVICE_H
#define NUMC3DS_GAME_MODE_SERVICE_H

int command_game_mode_label(char *out,unsigned capacity,int game_mode);
int command_game_mode_apply(void *player,int mode);
unsigned command_game_mode_apply_multiple(void **players,unsigned count,int mode,void **applied_out);

#endif
