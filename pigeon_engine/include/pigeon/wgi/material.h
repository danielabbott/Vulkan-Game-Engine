#pragma once

typedef struct PigeonWGIMaterialImport
{
    char * name;

    // Vertex/index
    unsigned int first;
    unsigned int count;

    float colour[3];

    char * texture;
    char * normal_map_texture;
} PigeonWGIMaterialImport;
