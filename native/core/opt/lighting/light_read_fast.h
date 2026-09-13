#ifndef NUMC3DS_LIGHT_READ_FAST_H
#define NUMC3DS_LIGHT_READ_FAST_H

int light_read_fast_install_hooks(void);
void light_read_stock_color(unsigned char *result, const void *source,
                            const int *position, const unsigned char *ambient);

#endif
