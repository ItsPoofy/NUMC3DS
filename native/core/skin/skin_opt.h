#ifndef NUMC3DS_SKIN_OPT_H
#define NUMC3DS_SKIN_OPT_H

int skin_opt_install_hooks(void);
void skin_opt_on_boot_complete(void);
int skin_opt_ensure_deferred_geometries_loaded(void *repo);

#endif /* NUMC3DS_SKIN_OPT_H */
