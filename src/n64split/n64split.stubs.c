/*
 * n64split stubs for removed SM64-specific functionality
 * These are stub implementations to maintain compatibility
 */

#include <stdio.h>
#include "n64split.h"

/* Collision stub */
int collision2obj(char *binfilename, unsigned int binoffset, char *objfilename,
                  char *name, float scale) {
    (void)binfilename;
    (void)binoffset;
    (void)objfilename;
    (void)name;
    (void)scale;
    
    INFO("Collision data export not supported in generic build\n");
    return 0;
}

char *terrain2str(unsigned int type) {
    static char unknown[16];
    snprintf(unknown, sizeof(unknown), "0x%02X", type);
    return unknown;
}

/* Behavior stub */
void write_behavior(FILE *out, unsigned char *data, rom_config *config, int s,
                    disasm_state *state) {
    (void)out;
    (void)data;
    (void)config;
    (void)s;
    (void)state;
    
    INFO("Behavior script export not supported in generic build\n");
}

/* Geo layout stub */
void write_geolayout(FILE *out, unsigned char *data, unsigned int start,
                     unsigned int end, disasm_state *state) {
    (void)out;
    (void)data;
    (void)start;
    (void)end;
    (void)state;
    
    INFO("Geometry layout export not supported in generic build\n");
}

void generate_geo_macros(arg_config *args) {
    (void)args;
    
    INFO("Geometry macro generation not supported in generic build\n");
}
