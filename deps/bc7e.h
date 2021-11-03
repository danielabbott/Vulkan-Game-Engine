// Modified version of the exported header
// .obj not included in repo.
// To enable BC7, set BC7E_OBJ on the makefile command line (points to _avx.obj)

#pragma once
#include <stdint.h>


struct $anon4 {
    uint32_t m_max_mode13_partitions_to_try;
    uint32_t m_max_mode0_partitions_to_try;
    uint32_t m_max_mode2_partitions_to_try;
    bool m_use_mode[7];
    bool m_unused1;
};

struct $anon5 {
    uint32_t m_max_mode7_partitions_to_try;
    uint32_t m_mode67_error_weight_mul[4];
    bool m_use_mode4;
    bool m_use_mode5;
    bool m_use_mode6;
    bool m_use_mode7;
    bool m_use_mode4_rotation;
    bool m_use_mode5_rotation;
    bool m_unused2;
    bool m_unused3;
};

struct bc7e_compress_block_params {
    uint32_t m_max_partitions_mode[8];
    uint32_t m_weights[4];
    uint32_t m_uber_level;
    uint32_t m_refinement_passes;
    uint32_t m_mode4_rotation_mask;
    uint32_t m_mode4_index_mask;
    uint32_t m_mode5_rotation_mask;
    uint32_t m_uber1_mask;
    bool m_perceptual;
    bool m_pbit_search;
    bool m_mode6_only;
    bool m_unused0;
    struct $anon4 m_opaque_settings;
    struct $anon5 m_alpha_settings;
};


void bc7e_compress_block_init_avx(void);
void bc7e_compress_block_params_init_ultrafast_avx(struct bc7e_compress_block_params * p, bool perceptual);
void bc7e_compress_block_params_init_veryfast_avx(struct bc7e_compress_block_params * p, bool perceptual);
void bc7e_compress_blocks_avx(uint32_t num_blocks, uint64_t * pBlocks, const uint32_t * pPixelsRGBA, const struct bc7e_compress_block_params * pComp_params);

