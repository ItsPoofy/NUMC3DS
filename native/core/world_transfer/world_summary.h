#ifndef NUMC3DS_WORLD_SUMMARY_H
#define NUMC3DS_WORLD_SUMMARY_H

/* In-memory world summary session cache. No database sidecar file is used.
 * World sizes are computed via fast native CTR-SDK directory batch enumeration. */
void wt_summary_begin(void);
void wt_summary_end(void);
void wt_summary_invalidate(void);
void wt_summary_invalidate_world(const char *world_id);
int wt_summary_extract_world_id(void *path_object,char *out,unsigned cap);
int wt_summary_lookup_path(void *path_object,unsigned long long *out_size);
void wt_summary_record_path(void *path_object,unsigned long long size);
unsigned long long wt_summary_calculate_world_size(const char *world_id);
unsigned long long wt_summary_calculate_world_size_force_scan(const char *world_id);
int wt_summary_path_is_resource_pack(void *path_object);
int wt_summary_text_is_resource_pack(const char *path_text);
const char *wt_summary_string_text(void *object);
int wt_summary_text_suffix(const char *text, const char *suffix);
int wt_summary_match_substring(const char *haystack, const char *needle);

int wt_summary_vector_valid(void);
int wt_summary_vector_populate(void *vector);
void wt_summary_vector_save(const void *vector);

#endif
