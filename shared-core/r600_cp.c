/* r600_cp.c -- CP support for Radeon -*- linux-c -*- */
/*
 * Copyright 2008 Advanced Micro Devices, Inc.
 * Copyright 2008 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *     Dave Airlie <airlied@redhat.com>
 *     Alex Deucher <alexander.deucher@amd.com>
 */

#include "drmP.h"
#include "drm.h"
#include "radeon_drm.h"
#include "radeon_drv.h"
#include "r300_reg.h"

#include "r600_microcode.h"

#define R600_MCD_RD_A_CNTL                                     0x219c
#define R600_MCD_RD_B_CNTL                                     0x21a0

#define R600_MCD_WR_A_CNTL                                     0x21a4
#define R600_MCD_WR_B_CNTL                                     0x21a8

#define R600_MCD_RD_SYS_CNTL                                   0x2200
#define R600_MCD_WR_SYS_CNTL                                   0x2214

#define R600_MCD_RD_GFX_CNTL                                   0x21fc
#define R600_MCD_RD_HDP_CNTL                                   0x2204
#define R600_MCD_RD_PDMA_CNTL                                  0x2208
#define R600_MCD_RD_SEM_CNTL                                   0x220c
#define R600_MCD_WR_GFX_CNTL                                   0x2210
#define R600_MCD_WR_HDP_CNTL                                   0x2218
#define R600_MCD_WR_PDMA_CNTL                                  0x221c
#define R600_MCD_WR_SEM_CNTL                                   0x2220

#       define R600_MCD_L1_TLB                                 (1 << 0)
#       define R600_MCD_L1_FRAG_PROC                           (1 << 1)
#       define R600_MCD_L1_STRICT_ORDERING                     (1 << 2)

#       define R600_MCD_SYSTEM_ACCESS_MODE_MASK                (3 << 6)
#       define R600_MCD_SYSTEM_ACCESS_MODE_PA_ONLY             (0 << 6)
#       define R600_MCD_SYSTEM_ACCESS_MODE_USE_SYS_MAP         (1 << 6)
#       define R600_MCD_SYSTEM_ACCESS_MODE_IN_SYS              (2 << 6)
#       define R600_MCD_SYSTEM_ACCESS_MODE_NOT_IN_SYS          (3 << 6)

#       define R600_MCD_SYSTEM_APERTURE_UNMAPPED_ACCESS_PASS_THRU    (0 << 8)
#       define R600_MCD_SYSTEM_APERTURE_UNMAPPED_ACCESS_DEFAULT_PAGE (1 << 8)

#       define R600_MCD_SEMAPHORE_MODE                         (1 << 10)
#       define R600_MCD_WAIT_L2_QUERY                          (1 << 11)
#       define R600_MCD_EFFECTIVE_L1_TLB_SIZE(x)               ((x) << 12)
#       define R600_MCD_EFFECTIVE_L1_QUEUE_SIZE(x)             ((x) << 15)

#define R700_MC_VM_MD_L1_TLB0_CNTL                             0x2654
#define R700_MC_VM_MD_L1_TLB1_CNTL                             0x2658
#define R700_MC_VM_MD_L1_TLB2_CNTL                             0x265c

#define R700_MC_VM_MB_L1_TLB0_CNTL                             0x2234
#define R700_MC_VM_MB_L1_TLB1_CNTL                             0x2238
#define R700_MC_VM_MB_L1_TLB2_CNTL                             0x223c
#define R700_MC_VM_MB_L1_TLB3_CNTL                             0x2240

#       define R700_ENABLE_L1_TLB                              (1 << 0)
#       define R700_ENABLE_L1_FRAGMENT_PROCESSING              (1 << 1)
#       define R700_SYSTEM_ACCESS_MODE_IN_SYS                  (2 << 3)
#       define R700_SYSTEM_APERTURE_UNMAPPED_ACCESS_PASS_THRU  (0 << 5)
#       define R700_EFFECTIVE_L1_TLB_SIZE(x)                   ((x) << 15)
#       define R700_EFFECTIVE_L1_QUEUE_SIZE(x)                 ((x) << 18)

#define R700_MC_ARB_RAMCFG                                     0x2760
#       define R700_NOOFBANK_SHIFT                             0
#       define R700_NOOFBANK_MASK                              0x3
#       define R700_NOOFRANK_SHIFT                             2
#       define R700_NOOFRANK_MASK                              0x1
#       define R700_NOOFROWS_SHIFT                             3
#       define R700_NOOFROWS_MASK                              0x7
#       define R700_NOOFCOLS_SHIFT                             6
#       define R700_NOOFCOLS_MASK                              0x3
#       define R700_CHANSIZE_SHIFT                             8
#       define R700_CHANSIZE_MASK                              0x1
#       define R700_BURSTLENGTH_SHIFT                          9
#       define R700_BURSTLENGTH_MASK                           0x1
#define R600_RAMCFG                                            0x2408
#       define R600_NOOFBANK_SHIFT                             0
#       define R600_NOOFBANK_MASK                              0x1
#       define R600_NOOFRANK_SHIFT                             1
#       define R600_NOOFRANK_MASK                              0x1
#       define R600_NOOFROWS_SHIFT                             2
#       define R600_NOOFROWS_MASK                              0x7
#       define R600_NOOFCOLS_SHIFT                             5
#       define R600_NOOFCOLS_MASK                              0x3
#       define R600_CHANSIZE_SHIFT                             7
#       define R600_CHANSIZE_MASK                              0x1
#       define R600_BURSTLENGTH_SHIFT                          8
#       define R600_BURSTLENGTH_MASK                           0x1

#define R600_VM_L2_CNTL                                        0x1400
#       define R600_VM_L2_CACHE_EN                             (1 << 0)
#       define R600_VM_L2_FRAG_PROC                            (1 << 1)
#       define R600_VM_ENABLE_PTE_CACHE_LRU_W                  (1 << 9)
#       define R600_VM_L2_CNTL_QUEUE_SIZE(x)                   ((x) << 13)
#       define R700_VM_L2_CNTL_QUEUE_SIZE(x)                   ((x) << 14)

#define R600_VM_L2_CNTL2 0x1404
#       define R600_VM_L2_CNTL2_INVALIDATE_ALL_L1_TLBS         (1 << 0)
#       define R600_VM_L2_CNTL2_INVALIDATE_L2_CACHE            (1 << 1)
#define R600_VM_L2_CNTL3 0x1408
#       define R600_VM_L2_CNTL3_BANK_SELECT_0(x)               ((x) << 0)
#       define R600_VM_L2_CNTL3_BANK_SELECT_1(x)               ((x) << 5)
#       define R600_VM_L2_CNTL3_CACHE_UPDATE_MODE(x)           ((x) << 10)
#       define R700_VM_L2_CNTL3_BANK_SELECT(x)                 ((x) << 0)
#       define R700_VM_L2_CNTL3_CACHE_UPDATE_MODE(x)           ((x) << 6)

#define R600_VM_L2_STATUS                                      0x140c

#define R600_VM_CONTEXT0_CNTL                                  0x1410
#       define R600_VM_ENABLE_CONTEXT                          (1 << 0)
#       define R600_VM_PAGE_TABLE_DEPTH_FLAT                   (0 << 1)

#define R600_VM_CONTEXT0_CNTL2                                 0x1430
#define R600_VM_CONTEXT0_REQUEST_RESPONSE                      0x1470
#define R600_VM_CONTEXT0_INVALIDATION_LOW_ADDR                 0x1490
#define R600_VM_CONTEXT0_INVALIDATION_HIGH_ADDR                0x14b0
#define R600_VM_CONTEXT0_PAGE_TABLE_BASE_ADDR                  0x1574
#define R600_VM_CONTEXT0_PAGE_TABLE_START_ADDR                 0x1594
#define R600_VM_CONTEXT0_PAGE_TABLE_END_ADDR                   0x15b4

#define R700_VM_CONTEXT0_PAGE_TABLE_BASE_ADDR                  0x153c
#define R700_VM_CONTEXT0_PAGE_TABLE_START_ADDR                 0x155c
#define R700_VM_CONTEXT0_PAGE_TABLE_END_ADDR                   0x157c

# define ATI_PCIGART_PAGE_SIZE		4096	/**< PCI GART page size */
# define ATI_PCIGART_PAGE_MASK		(~(ATI_PCIGART_PAGE_SIZE-1))

#define R600_PTE_VALID     (1 << 0)
#define R600_PTE_SYSTEM    (1 << 1)
#define R600_PTE_SNOOPED   (1 << 2)
#define R600_PTE_READABLE  (1 << 5)
#define R600_PTE_WRITEABLE (1 << 6)

#define R600_CP_ME_CNTL                                         0x86d8
#       define R600_CP_ME_HALT                                  (1 << 28)

#define R600_HDP_HOST_PATH_CNTL                                 0x2c00

#define R600_GRBM_CNTL                                   	0x8000
#       define R600_GRBM_READ_TIMEOUT(x)                        ((x) << 0)
#define R600_GRBM_SOFT_RESET                               	0x8020
#       define R600_SOFT_RESET_CP                               (1 << 0)

#define R600_CP_SEM_WAIT_TIMER                             	0x85bc
#define R600_CP_DEBUG                                      	0xc1fc
#define R600_PA_SC_MULTI_CHIP_CNTL                         	0x8b20
#define R600_PA_CL_ENHANCE                                 	0x8a14
#       define R600_CLIP_VTX_REORDER_ENA                        (1 << 0)
#       define R600_NUM_CLIP_SEQ(x)                             ((x) << 1)
#define R600_PA_SC_ENHANCE                                 	0x8bf0
#       define R600_FORCE_EOV_MAX_CLK_CNT(x)                    ((x) << 0)
#       define R600_FORCE_EOV_MAX_TILE_CNT(x)                   ((x) << 12)

#define R600_CP_QUEUE_THRESHOLDS                                0x8760
#       define R600_ROQ_IB1_START(x)                            ((x) << 0)
#       define R600_ROQ_IB2_START(x)                            ((x) << 8)
#define R600_CP_MEQ_THRESHOLDS                                  0x8764
#       define R700_STQ_SPLIT(x)                                ((x) << 0)
#       define R600_MEQ_END(x)                                  ((x) << 16)
#       define R600_ROQ_END(x)                                  ((x) << 24)
#define R600_SX_DEBUG_1                                         0x9054
#       define R600_SMX_EVENT_RELEASE                           (1 << 0)
#       define R600_ENABLE_NEW_SMX_ADDRESS                      (1 << 16)
#define R700_SX_DEBUG_1                                         0x9058
#       define R700_ENABLE_NEW_SMX_ADDRESS                      (1 << 16)
#define R600_DB_DEBUG                                           0x9830
#       define R600_PREZ_MUST_WAIT_FOR_POSTZ_DONE               (1 << 31)
#define R600_DB_WATERMARKS                                      0x9838
#       define R600_DEPTH_FREE(x)                               ((x) << 0)
#       define R600_DEPTH_FLUSH(x)                              ((x) << 5)
#       define R600_DEPTH_PENDING_FREE(x)                       ((x) << 15)
#       define R600_DEPTH_CACHELINE_FREE(x)                     ((x) << 20)
#define R600_VGT_NUM_INSTANCES                                  0x8974
#define R600_VGT_CACHE_INVALIDATION                             0x88c4
#       define R600_CACHE_INVALIDATION(x)                       ((x) << 0)
#       define R600_VC_ONLY                                     0
#       define R600_TC_ONLY                                     1
#       define R600_VC_AND_TC                                   2
#       define R700_AUTO_INVLD_EN(x)                            ((x) << 6)
#       define R700_NO_AUTO                                     0
#       define R700_ES_AUTO                                     1
#       define R700_GS_AUTO                                     2
#       define R700_ES_AND_GS_AUTO                              3
#define R600_VGT_VERTEX_REUSE_BLOCK_CNTL                        0x28c58
#       define R600_VTX_REUSE_DEPTH_MASK                        0xff
#define R600_VGT_OUT_DEALLOC_CNTL                               0x28c5c
#       define R600_DEALLOC_DIST_MASK                           0x7f
#define R600_PA_SC_AA_SAMPLE_LOCS_2S                            0x8b40
#define R600_PA_SC_AA_SAMPLE_LOCS_4S                            0x8b44
#define R600_PA_SC_AA_SAMPLE_LOCS_8S_WD0                        0x8b48
#define R600_PA_SC_AA_SAMPLE_LOCS_8S_WD1                        0x8b4c
#define R600_CB_COLOR0_BASE                                     0x28040
#define R600_CB_COLOR1_BASE                                     0x28044
#define R600_CB_COLOR2_BASE                                     0x28048
#define R600_CB_COLOR3_BASE                                     0x2804c
#define R600_CB_COLOR4_BASE                                     0x28050
#define R600_CB_COLOR5_BASE                                     0x28054
#define R600_CB_COLOR6_BASE                                     0x28058
#define R600_CB_COLOR7_BASE                                     0x2805c
#define R600_TC_CNTL                                            0x9608
#       define R600_TC_L2_SIZE(x)                               ((x) << 5)
#       define R600_L2_DISABLE_LATE_HIT                         (1 << 9)
#define R600_ARB_POP                                            0x2418
#       define R600_ENABLE_TC128                                (1 << 30)
#define R600_ARB_GDEC_RD_CNTL                                   0x246c
#define R600_TA_CNTL_AUX                                        0x9508
#       define R600_DISABLE_CUBE_WRAP                           (1 << 0)
#       define R600_DISABLE_CUBE_ANISO                          (1 << 1)
#       define R700_GETLOD_SELECT(x)                            ((x) << 2)
#       define R600_SYNC_GRADIENT                               (1 << 24)
#       define R600_SYNC_WALKER                                 (1 << 25)
#       define R600_SYNC_ALIGNER                                (1 << 26)
#       define R600_BILINEAR_PRECISION_6_BIT                    (0 << 31)
#       define R600_BILINEAR_PRECISION_8_BIT                    (1 << 31)
#define R600_SMX_DC_CTL0                                        0xa020
#       define R700_USE_HASH_FUNCTION                           (1 << 0)
#       define R700_CACHE_DEPTH(x)                              ((x) << 1)
#       define R700_FLUSH_ALL_ON_EVENT                          (1 << 10)
#       define R700_STALL_ON_EVENT                              (1 << 11)
#define R700_SMX_EVENT_CTL                                      0xa02c
#       define R700_ES_FLUSH_CTL(x)                             ((x) << 0)
#       define R700_GS_FLUSH_CTL(x)                             ((x) << 3)
#       define R700_ACK_FLUSH_CTL(x)                            ((x) << 6)
#       define R700_SYNC_FLUSH_CTL                              (1 << 8)
#define R700_DB_DEBUG3                                          0x98b0
#       define R700_DB_CLK_OFF_DELAY(x)                         ((x) << 11)
#define RV700_DB_DEBUG4                                         0x9b8c
#       define RV700_DISABLE_TILE_COVERED_FOR_PS_ITER           (1 << 6)
#define R600_SX_EXPORT_BUFFER_SIZES                             0x900c
#       define R600_COLOR_BUFFER_SIZE(x)                        ((x) << 0)
#       define R600_POSITION_BUFFER_SIZE(x)                     ((x) << 8)
#       define R600_SMX_BUFFER_SIZE(x)                          ((x) << 16)
#define R600_PA_SC_FIFO_SIZE                                    0x8bd0
#       define R600_SC_PRIM_FIFO_SIZE(x)                        ((x) << 0)
#       define R600_SC_HIZ_TILE_FIFO_SIZE(x)                    ((x) << 8)
#       define R600_SC_EARLYZ_TILE_FIFO_SIZE(x)                 ((x) << 16)
#define R700_PA_SC_FIFO_SIZE_R7XX                               0x8bcc
#       define R700_SC_PRIM_FIFO_SIZE(x)                        ((x) << 0)
#       define R700_SC_HIZ_TILE_FIFO_SIZE(x)                    ((x) << 12)
#       define R700_SC_EARLYZ_TILE_FIFO_SIZE(x)                 ((x) << 20)
#define R600_CP_PERFMON_CNTL                                    0x87fc
#define R600_SQ_MS_FIFO_SIZES                                   0x8cf0
#       define R600_CACHE_FIFO_SIZE(x)                          ((x) << 0)
#       define R600_FETCH_FIFO_HIWATER(x)                       ((x) << 8)
#       define R600_DONE_FIFO_HIWATER(x)                        ((x) << 16)
#       define R600_ALU_UPDATE_FIFO_HIWATER(x)                  ((x) << 24)
#define R700_PA_SC_FORCE_EOV_MAX_CNTS                           0x8b24
#       define R700_FORCE_EOV_MAX_CLK_CNT(x)                    ((x) << 0)
#       define R700_FORCE_EOV_MAX_REZ_CNT(x)                    ((x) << 16)
#define R700_TCP_CNTL                                           0x9610
#define R600_SQ_CONFIG                                          0x8c00
#       define R600_VC_ENABLE                                   (1 << 0)
#       define R600_EXPORT_SRC_C                                (1 << 1)
#       define R600_DX9_CONSTS                                  (1 << 2)
#       define R600_ALU_INST_PREFER_VECTOR                      (1 << 3)
#       define R600_DX10_CLAMP                                  (1 << 4)
#       define R600_CLAUSE_SEQ_PRIO(x)                          ((x) << 8)
#       define R600_PS_PRIO(x)                                  ((x) << 24)
#       define R600_VS_PRIO(x)                                  ((x) << 26)
#       define R600_GS_PRIO(x)                                  ((x) << 28)
#       define R600_ES_PRIO(x)                                  ((x) << 30)
#define R600_SQ_GPR_RESOURCE_MGMT_1                             0x8c04
#       define R600_NUM_PS_GPRS(x)                              ((x) << 0)
#       define R600_NUM_VS_GPRS(x)                              ((x) << 16)
#       define R700_DYN_GPR_ENABLE                              (1 << 27)
#       define R600_NUM_CLAUSE_TEMP_GPRS(x)                     ((x) << 28)
#define R600_SQ_GPR_RESOURCE_MGMT_2                             0x8c08
#       define R600_NUM_GS_GPRS(x)                              ((x) << 0)
#       define R600_NUM_ES_GPRS(x)                              ((x) << 16)
#define R600_SQ_THREAD_RESOURCE_MGMT                            0x8c0c
#       define R600_NUM_PS_THREADS(x)                          ((x) << 0)
#       define R600_NUM_VS_THREADS(x)                          ((x) << 8)
#       define R600_NUM_GS_THREADS(x)                          ((x) << 16)
#       define R600_NUM_ES_THREADS(x)                          ((x) << 24)
#define R600_SQ_STACK_RESOURCE_MGMT_1                           0x8c10
#       define R600_NUM_PS_STACK_ENTRIES(x)                     ((x) << 0)
#       define R600_NUM_VS_STACK_ENTRIES(x)                     ((x) << 16)
#define R600_SQ_STACK_RESOURCE_MGMT_2                           0x8c14
#       define R600_NUM_GS_STACK_ENTRIES(x)                     ((x) << 0)
#       define R600_NUM_ES_STACK_ENTRIES(x)                     ((x) << 16)
#define R700_SQ_DYN_GPR_SIZE_SIMD_AB_0                          0x8db0
#       define R700_SIMDA_RING0(x)                              ((x) << 0)
#       define R700_SIMDA_RING1(x)                              ((x) << 8)
#       define R700_SIMDB_RING0(x)                              ((x) << 16)
#       define R700_SIMDB_RING1(x)                              ((x) << 24)
#define R700_SQ_DYN_GPR_SIZE_SIMD_AB_1                          0x8db4
#define R700_SQ_DYN_GPR_SIZE_SIMD_AB_2                          0x8db8
#define R700_SQ_DYN_GPR_SIZE_SIMD_AB_3                          0x8dbc
#define R700_SQ_DYN_GPR_SIZE_SIMD_AB_4                          0x8dc0
#define R700_SQ_DYN_GPR_SIZE_SIMD_AB_5                          0x8dc4
#define R700_SQ_DYN_GPR_SIZE_SIMD_AB_6                          0x8dc8
#define R700_SQ_DYN_GPR_SIZE_SIMD_AB_7                          0x8dcc
#define R600_VGT_ES_PER_GS                                      0x88cc
#define R600_VGT_GS_PER_ES                                      0x88c8
#define R600_VGT_GS_PER_VS                                      0x88e8
#define R600_VGT_GS_VERTEX_REUSE                                0x88d4
#define R600_PA_SC_LINE_STIPPLE                                 0x28a0c
#define R600_PA_SC_LINE_STIPPLE_STATE                           0x8b10
#define R600_VGT_STRMOUT_EN                                     0x28ab0
#define R600_SX_MISC                                            0x28350
#define R600_PA_SC_MODE_CNTL                                    0x28a4c
#define R700_PA_SC_EDGERULE                                     0x28230
#define R600_PA_SC_CLIPRECT_RULE                                0x2820c
#define R600_SPI_PS_IN_CONTROL_0                                0x286cc
#       define R600_NUM_INTERP(x)                               ((x) << 0)
#       define R600_POSITION_ENA                                (1 << 8)
#       define R600_POSITION_CENTROID                           (1 << 9)
#       define R600_POSITION_ADDR(x)                            ((x) << 10)
#       define R600_PARAM_GEN(x)                                ((x) << 15)
#       define R600_PARAM_GEN_ADDR(x)                           ((x) << 19)
#       define R600_BARYC_SAMPLE_CNTL(x)                        ((x) << 26)
#       define R600_PERSP_GRADIENT_ENA                          (1 << 28)
#       define R600_LINEAR_GRADIENT_ENA                         (1 << 29)
#       define R600_POSITION_SAMPLE                             (1 << 30)
#       define R600_BARYC_AT_SAMPLE_ENA                         (1 << 31)
#define R600_SPI_PS_IN_CONTROL_1                                0x286d0
#       define R600_GEN_INDEX_PIX                               (1 << 0)
#       define R600_GEN_INDEX_PIX_ADDR(x)                       ((x) << 1)
#       define R600_FRONT_FACE_ENA                              (1 << 8)
#       define R600_FRONT_FACE_CHAN(x)                          ((x) << 9)
#       define R600_FRONT_FACE_ALL_BITS                         (1 << 11)
#       define R600_FRONT_FACE_ADDR(x)                          ((x) << 12)
#       define R600_FOG_ADDR(x)                                 ((x) << 17)
#       define R600_FIXED_PT_POSITION_ENA                       (1 << 24)
#       define R600_FIXED_PT_POSITION_ADDR(x)                   ((x) << 25)
#       define R700_POSITION_ULC                                (1 << 30)
#define R600_PA_SC_AA_CONFIG                                    0x28c04
#define R600_SPI_INPUT_Z                                        0x286d8
#define R600_CB_COLOR7_FRAG                                     0x280fc

#define R600_SPI_CONFIG_CNTL                                    0x9100
#       define R600_GPR_WRITE_PRIORITY(x)                       ((x) << 0)
#       define R600_DISABLE_INTERP_1                            (1 << 5)
#define R600_SPI_CONFIG_CNTL_1                                  0x913c
#       define R600_VTX_DONE_DELAY(x)                           ((x) << 0)
#       define R600_INTERP_ONE_PRIM_PER_ROW                     (1 << 4)

#define R600_GB_TILING_CONFIG                                   0x98f0
#       define R600_PIPE_TILING(x)                              ((x) << 1)
#       define R600_BANK_TILING(x)                              ((x) << 4)
#       define R600_GROUP_SIZE(x)                               ((x) << 6)
#       define R600_ROW_TILING(x)                               ((x) << 8)
#       define R600_BANK_SWAPS(x)                               ((x) << 11)
#       define R600_SAMPLE_SPLIT(x)                             ((x) << 14)
#       define R600_BACKEND_MAP(x)                              ((x) << 16)
#define R600_DCP_TILING_CONFIG                                  0x6ca0
#define R600_HDP_TILING_CONFIG                                  0x2f3c

#define R600_CC_RB_BACKEND_DISABLE                              0x98f4
#define R700_CC_SYS_RB_BACKEND_DISABLE                          0x3f88
#       define R600_BACKEND_DISABLE(x)                          ((x) << 16)

#define R600_CC_GC_SHADER_PIPE_CONFIG                           0x8950
#define R600_GC_USER_SHADER_PIPE_CONFIG                         0x8954
#       define R600_INACTIVE_QD_PIPES(x)                        ((x) << 8)
#       define R600_INACTIVE_QD_PIPES_MASK                      (0xff << 8)
#       define R600_INACTIVE_SIMDS(x)                           ((x) << 16)
#       define R600_INACTIVE_SIMDS_MASK                         (0xff << 16)

#define R700_CGTS_SYS_TCC_DISABLE                               0x3f90
#define R700_CGTS_USER_SYS_TCC_DISABLE                          0x3f94
#define R700_CGTS_TCC_DISABLE                                   0x9148
#define R700_CGTS_USER_TCC_DISABLE                              0x914c

/* MAX values used for gfx init */
#define R6XX_MAX_SH_GPRS           256
#define R6XX_MAX_TEMP_GPRS         16
#define R6XX_MAX_SH_THREADS        256
#define R6XX_MAX_SH_STACK_ENTRIES  4096
#define R6XX_MAX_BACKENDS          8
#define R6XX_MAX_BACKENDS_MASK     0xff
#define R6XX_MAX_SIMDS             8
#define R6XX_MAX_SIMDS_MASK        0xff
#define R6XX_MAX_PIPES             8
#define R6XX_MAX_PIPES_MASK        0xff

#define R7XX_MAX_SH_GPRS           256
#define R7XX_MAX_TEMP_GPRS         16
#define R7XX_MAX_SH_THREADS        256
#define R7XX_MAX_SH_STACK_ENTRIES  4096
#define R7XX_MAX_BACKENDS          8
#define R7XX_MAX_BACKENDS_MASK     0xff
#define R7XX_MAX_SIMDS             16
#define R7XX_MAX_SIMDS_MASK        0xffff
#define R7XX_MAX_PIPES             8
#define R7XX_MAX_PIPES_MASK        0xff

static int r600_do_wait_for_idle(drm_radeon_private_t * dev_priv)
{
	u32 gbrm_status;
	int i;

	for (i = 0; i < dev_priv->usec_timeout; i++) {
		gbrm_status = RADEON_READ(R600_GRBM_STATUS);

		if (!(gbrm_status & (1 << 31)))
			return 0;
		DRM_UDELAY(1);
	}
	return -EBUSY;
}

static void r600_page_table_cleanup(struct drm_device *dev, struct drm_ati_pcigart_info *gart_info)
{
	struct drm_sg_mem *entry = dev->sg;
	int max_pages;
	int pages;
	int i;

	if (gart_info->bus_addr) {

		max_pages = (gart_info->table_size / sizeof(u32));
		pages = (entry->pages <= max_pages)
		  ? entry->pages : max_pages;

		for (i = 0; i < pages; i++) {
			if (!entry->busaddr[i])
				break;
			pci_unmap_single(dev->pdev, entry->busaddr[i],
					 PAGE_SIZE, PCI_DMA_TODEVICE);
		}

		if (gart_info->gart_table_location == DRM_ATI_GART_MAIN)
			gart_info->bus_addr = 0;
	}
}

/* R600 has page table setup */
static int r600_page_table_init(struct drm_device *dev)
{
	drm_radeon_private_t *dev_priv = dev->dev_private;
	struct drm_ati_pcigart_info *gart_info = &dev_priv->gart_info;
	struct drm_sg_mem *entry = dev->sg;
	int ret = 0;
	int i, j;
	int max_pages, pages;
	u64 *pci_gart, page_base;
	dma_addr_t entry_addr;

	/* okay page table is available - lets rock */

	/* PTEs are 64-bits */
	pci_gart = (u64 *)gart_info->addr;

	max_pages = (gart_info->table_size / sizeof(u64));
	pages = (entry->pages <= max_pages) ? entry->pages : max_pages;

	memset(pci_gart, 0, max_pages * sizeof(u64));

	for (i = 0; i < pages; i++) {
		entry->busaddr[i] = pci_map_single(dev->pdev,
						   page_address(entry->
								pagelist[i]),
						   PAGE_SIZE, PCI_DMA_TODEVICE);
		if (entry->busaddr[i] == 0) {
			DRM_ERROR("unable to map PCIGART pages!\n");
			r600_page_table_cleanup(dev, gart_info);
			ret = -EINVAL;
			goto done;
		}

		entry_addr = entry->busaddr[i];
		for (j = 0; j < (PAGE_SIZE / ATI_PCIGART_PAGE_SIZE); j++) {
			page_base = (u64) entry_addr & ATI_PCIGART_PAGE_MASK;
			page_base |= R600_PTE_VALID | R600_PTE_SYSTEM | R600_PTE_SNOOPED;
			page_base |= R600_PTE_READABLE | R600_PTE_WRITEABLE;

			*pci_gart = page_base;

			if ((i % 128) == 0)
				DRM_DEBUG("page entry %d: 0x%016llx\n", i, page_base);
			pci_gart++;
			entry_addr += ATI_PCIGART_PAGE_SIZE;
		}
	}

done:
	return ret;
}

static void r600_vm_flush_gart_range(struct drm_device *dev)
{
	drm_radeon_private_t *dev_priv = dev->dev_private;
	u32 resp, countdown = 1000;
	RADEON_WRITE(R600_VM_CONTEXT0_INVALIDATION_LOW_ADDR, dev_priv->gart_vm_start >> 12);
	RADEON_WRITE(R600_VM_CONTEXT0_INVALIDATION_HIGH_ADDR, (dev_priv->gart_vm_start + dev_priv->gart_size - 1) >> 12);
	RADEON_WRITE(R600_VM_CONTEXT0_REQUEST_RESPONSE, 2);

	do {
		resp = RADEON_READ(R600_VM_CONTEXT0_REQUEST_RESPONSE);
		countdown--;
		DRM_UDELAY(1);
	} while (((resp & 0xf0) == 0) && countdown );
}

static void r600_vm_init(struct drm_device *dev)
{
	drm_radeon_private_t *dev_priv = dev->dev_private;
	/* initialise the VM to use the page table we constructed up there */
	u32 vm_c0, i;
	u32 mc_rd_a;
	u32 vm_l2_cntl, vm_l2_cntl3;
	/* okay set up the PCIE aperture type thingo */
	RADEON_WRITE(R600_MC_VM_SYSTEM_APERTURE_LOW_ADDR, dev_priv->gart_vm_start >> 12);
	RADEON_WRITE(R600_MC_VM_SYSTEM_APERTURE_HIGH_ADDR, (dev_priv->gart_vm_start + dev_priv->gart_size - 1) >> 12);
	RADEON_WRITE(R600_MC_VM_SYSTEM_APERTURE_DEFAULT_ADDR, 0);

	/* setup MC RD a */
	mc_rd_a = R600_MCD_L1_TLB | R600_MCD_L1_FRAG_PROC | R600_MCD_SYSTEM_ACCESS_MODE_IN_SYS |
		R600_MCD_SYSTEM_APERTURE_UNMAPPED_ACCESS_PASS_THRU | R600_MCD_EFFECTIVE_L1_TLB_SIZE(5) |
		R600_MCD_EFFECTIVE_L1_QUEUE_SIZE(5) | R600_MCD_WAIT_L2_QUERY;

	RADEON_WRITE(R600_MCD_RD_A_CNTL, mc_rd_a);
	RADEON_WRITE(R600_MCD_RD_B_CNTL, mc_rd_a);

	RADEON_WRITE(R600_MCD_WR_A_CNTL, mc_rd_a);
	RADEON_WRITE(R600_MCD_WR_B_CNTL, mc_rd_a);

	RADEON_WRITE(R600_MCD_RD_GFX_CNTL, mc_rd_a);
	RADEON_WRITE(R600_MCD_WR_GFX_CNTL, mc_rd_a);

	RADEON_WRITE(R600_MCD_RD_SYS_CNTL, mc_rd_a);
	RADEON_WRITE(R600_MCD_WR_SYS_CNTL, mc_rd_a);

	RADEON_WRITE(R600_MCD_RD_HDP_CNTL, mc_rd_a | R600_MCD_L1_STRICT_ORDERING);
	RADEON_WRITE(R600_MCD_WR_HDP_CNTL, mc_rd_a /*| R600_MCD_L1_STRICT_ORDERING*/);

	RADEON_WRITE(R600_MCD_RD_PDMA_CNTL, mc_rd_a);
	RADEON_WRITE(R600_MCD_WR_PDMA_CNTL, mc_rd_a);

	RADEON_WRITE(R600_MCD_RD_SEM_CNTL, mc_rd_a | R600_MCD_SEMAPHORE_MODE);
	RADEON_WRITE(R600_MCD_WR_SEM_CNTL, mc_rd_a);

	vm_l2_cntl = R600_VM_L2_CACHE_EN | R600_VM_L2_FRAG_PROC | R600_VM_ENABLE_PTE_CACHE_LRU_W;
	vm_l2_cntl |= R600_VM_L2_CNTL_QUEUE_SIZE(7);
	RADEON_WRITE(R600_VM_L2_CNTL, vm_l2_cntl);

	RADEON_WRITE(R600_VM_L2_CNTL2, 0);
	vm_l2_cntl3 = R600_VM_L2_CNTL3_BANK_SELECT_0(0) |
	              R600_VM_L2_CNTL3_BANK_SELECT_1(1) |
	              R600_VM_L2_CNTL3_CACHE_UPDATE_MODE(2);
	RADEON_WRITE(R600_VM_L2_CNTL3, vm_l2_cntl3);

	vm_c0 = R600_VM_ENABLE_CONTEXT | R600_VM_PAGE_TABLE_DEPTH_FLAT;

	RADEON_WRITE(R600_VM_CONTEXT0_CNTL, vm_c0);

	vm_c0 &= ~R600_VM_ENABLE_CONTEXT;

	/* disable all other contexts */
	for (i = 1; i < 8; i++)
		RADEON_WRITE(R600_VM_CONTEXT0_CNTL + (i * 4), vm_c0);

	RADEON_WRITE(R600_VM_CONTEXT0_PAGE_TABLE_BASE_ADDR, dev_priv->gart_info.bus_addr >> 12);
	RADEON_WRITE(R600_VM_CONTEXT0_PAGE_TABLE_START_ADDR, dev_priv->gart_vm_start >> 12);
	RADEON_WRITE(R600_VM_CONTEXT0_PAGE_TABLE_END_ADDR, (dev_priv->gart_vm_start + dev_priv->gart_size - 1) >> 12);

	r600_vm_flush_gart_range(dev);
}

/* load r600 microcode */
static void r600_cp_load_microcode(drm_radeon_private_t * dev_priv)
{
	int i;

	r600_do_cp_stop(dev_priv);

	RADEON_WRITE(R600_CP_RB_CNTL,
		     (1 << 27) |
		     (15 << 8) |
		     3);


	RADEON_WRITE(R600_GRBM_SOFT_RESET, R600_SOFT_RESET_CP);
	RADEON_READ(R600_GRBM_SOFT_RESET);
	DRM_UDELAY(15000);
	RADEON_WRITE(R600_GRBM_SOFT_RESET, 0);


	RADEON_WRITE(R600_CP_ME_RAM_WADDR, 0);

	if (((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_R600)) {

		DRM_INFO("Loading R600 CP Microcode\n");
		for (i = 0; i < PM4_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     R600_cp_microcode[i][0]);
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     R600_cp_microcode[i][1]);
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     R600_cp_microcode[i][2]);
		}

		RADEON_WRITE(R600_CP_PFP_UCODE_ADDR, 0);

		DRM_INFO("Loading R600 PFP Microcode\n");
		for (i = 0; i < PFP_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_PFP_UCODE_DATA, R600_pfp_microcode[i]);
		}
	} else if (((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV610)) {

		DRM_INFO("Loading RV610 CP Microcode\n");
		for (i = 0; i < PM4_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV610_cp_microcode[i][0]);
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV610_cp_microcode[i][1]);
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV610_cp_microcode[i][2]);
		}

		RADEON_WRITE(R600_CP_PFP_UCODE_ADDR, 0);

		DRM_INFO("Loading RV610 PFP Microcode\n");
		for (i = 0; i < PFP_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_PFP_UCODE_DATA, RV610_pfp_microcode[i]);
		}
	} else if (((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV630)) {

		DRM_INFO("Loading RV630 CP Microcode\n");
		for (i = 0; i < PM4_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV630_cp_microcode[i][0]);
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV630_cp_microcode[i][1]);
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV630_cp_microcode[i][2]);
		}

		RADEON_WRITE(R600_CP_PFP_UCODE_ADDR, 0);

		DRM_INFO("Loading RV630 PFP Microcode\n");
		for (i = 0; i < PFP_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_PFP_UCODE_DATA, RV630_pfp_microcode[i]);
		}
	} else if (((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV620)) {

		DRM_INFO("Loading RV620 CP Microcode\n");
		for (i = 0; i < PM4_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV620_cp_microcode[i][0]);
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV620_cp_microcode[i][1]);
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV620_cp_microcode[i][2]);
		}

		RADEON_WRITE(R600_CP_PFP_UCODE_ADDR, 0);

		DRM_INFO("Loading RV620 PFP Microcode\n");
		for (i = 0; i < PFP_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_PFP_UCODE_DATA, RV620_pfp_microcode[i]);
		}
	} else if (((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV635)) {

		DRM_INFO("Loading RV635 CP Microcode\n");
		for (i = 0; i < PM4_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV635_cp_microcode[i][0]);
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV635_cp_microcode[i][1]);
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV635_cp_microcode[i][2]);
		}

		RADEON_WRITE(R600_CP_PFP_UCODE_ADDR, 0);

		DRM_INFO("Loading RV635 PFP Microcode\n");
		for (i = 0; i < PFP_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_PFP_UCODE_DATA, RV635_pfp_microcode[i]);
		}
	} else if (((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV670)) {

		DRM_INFO("Loading RV670 CP Microcode\n");
		for (i = 0; i < PM4_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV670_cp_microcode[i][0]);
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV670_cp_microcode[i][1]);
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV670_cp_microcode[i][2]);
		}

		RADEON_WRITE(R600_CP_PFP_UCODE_ADDR, 0);

		DRM_INFO("Loading RV670 PFP Microcode\n");
		for (i = 0; i < PFP_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_PFP_UCODE_DATA, RV670_pfp_microcode[i]);
		}
	} else if (((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RS780)) {

		DRM_INFO("Loading RS780 CP Microcode\n");
		for (i = 0; i < PM4_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV670_cp_microcode[i][0]);
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV670_cp_microcode[i][1]);
			RADEON_WRITE(R600_CP_ME_RAM_DATA,
				     RV670_cp_microcode[i][2]);
		}

		RADEON_WRITE(R600_CP_PFP_UCODE_ADDR, 0);

		DRM_INFO("Loading RS780 PFP Microcode\n");
		for (i = 0; i < PFP_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_PFP_UCODE_DATA, RV670_pfp_microcode[i]);
		}
	}
	RADEON_WRITE(R600_CP_PFP_UCODE_ADDR, 0);
	RADEON_WRITE(R600_CP_ME_RAM_WADDR, 0);
	RADEON_WRITE(R600_CP_ME_RAM_RADDR, 0);

}

static void r700_vm_init(struct drm_device *dev)
{
	drm_radeon_private_t *dev_priv = dev->dev_private;
	/* initialise the VM to use the page table we constructed up there */
	u32 vm_c0, i;
	u32 mc_vm_md_l1;
	u32 vm_l2_cntl, vm_l2_cntl3;
	/* okay set up the PCIE aperture type thingo */
	RADEON_WRITE(R700_MC_VM_SYSTEM_APERTURE_LOW_ADDR, dev_priv->gart_vm_start >> 12);
	RADEON_WRITE(R700_MC_VM_SYSTEM_APERTURE_HIGH_ADDR, (dev_priv->gart_vm_start + dev_priv->gart_size - 1) >> 12);
	RADEON_WRITE(R700_MC_VM_SYSTEM_APERTURE_DEFAULT_ADDR, 0);

	mc_vm_md_l1 = R700_ENABLE_L1_TLB |
	    R700_ENABLE_L1_FRAGMENT_PROCESSING |
	    R700_SYSTEM_ACCESS_MODE_IN_SYS |
	    R700_SYSTEM_APERTURE_UNMAPPED_ACCESS_PASS_THRU |
	    R700_EFFECTIVE_L1_TLB_SIZE(5) |
	    R700_EFFECTIVE_L1_QUEUE_SIZE(5);

	RADEON_WRITE(R700_MC_VM_MD_L1_TLB0_CNTL, mc_vm_md_l1);
	RADEON_WRITE(R700_MC_VM_MD_L1_TLB1_CNTL, mc_vm_md_l1);
	RADEON_WRITE(R700_MC_VM_MD_L1_TLB2_CNTL, mc_vm_md_l1);
	RADEON_WRITE(R700_MC_VM_MB_L1_TLB0_CNTL, mc_vm_md_l1);
	RADEON_WRITE(R700_MC_VM_MB_L1_TLB1_CNTL, mc_vm_md_l1);
	RADEON_WRITE(R700_MC_VM_MB_L1_TLB2_CNTL, mc_vm_md_l1);
	RADEON_WRITE(R700_MC_VM_MB_L1_TLB3_CNTL, mc_vm_md_l1);

	vm_l2_cntl = R600_VM_L2_CACHE_EN | R600_VM_L2_FRAG_PROC | R600_VM_ENABLE_PTE_CACHE_LRU_W;
	vm_l2_cntl |= R700_VM_L2_CNTL_QUEUE_SIZE(7);
	RADEON_WRITE(R600_VM_L2_CNTL, vm_l2_cntl);

	RADEON_WRITE(R600_VM_L2_CNTL2, 0);
	vm_l2_cntl3 = R700_VM_L2_CNTL3_BANK_SELECT(0) |
	              R700_VM_L2_CNTL3_CACHE_UPDATE_MODE(2);
	RADEON_WRITE(R600_VM_L2_CNTL3, vm_l2_cntl3);

	vm_c0 = R600_VM_ENABLE_CONTEXT | R600_VM_PAGE_TABLE_DEPTH_FLAT;

	RADEON_WRITE(R600_VM_CONTEXT0_CNTL, vm_c0);

	vm_c0 &= ~R600_VM_ENABLE_CONTEXT;

	/* disable all other contexts */
	for (i = 1; i < 8; i++)
		RADEON_WRITE(R600_VM_CONTEXT0_CNTL + (i * 4), vm_c0);

	RADEON_WRITE(R700_VM_CONTEXT0_PAGE_TABLE_BASE_ADDR, dev_priv->gart_info.bus_addr >> 12);
	RADEON_WRITE(R700_VM_CONTEXT0_PAGE_TABLE_START_ADDR, dev_priv->gart_vm_start >> 12);
	RADEON_WRITE(R700_VM_CONTEXT0_PAGE_TABLE_END_ADDR, (dev_priv->gart_vm_start + dev_priv->gart_size - 1) >> 12);

	r600_vm_flush_gart_range(dev);
}

/* load r600 microcode */
static void r700_cp_load_microcode(drm_radeon_private_t * dev_priv)
{
	int i;

	r600_do_cp_stop(dev_priv);

	RADEON_WRITE(R600_CP_RB_CNTL,
		     R600_RB_NO_UPDATE |
		     (15 << 8) |
		     (3 << 0));

	RADEON_WRITE(R600_GRBM_SOFT_RESET, R600_SOFT_RESET_CP);
	RADEON_READ(R600_GRBM_SOFT_RESET);
	DRM_UDELAY(15000);
	RADEON_WRITE(R600_GRBM_SOFT_RESET, 0);


	if (((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV770)) {

		RADEON_WRITE(R600_CP_PFP_UCODE_ADDR, 0);

		DRM_INFO("Loading RV770 PFP Microcode\n");
		for (i = 0; i < R700_PFP_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_PFP_UCODE_DATA, RV770_pfp_microcode[i]);
		}
		RADEON_WRITE(R600_CP_PFP_UCODE_ADDR, 0);

		RADEON_WRITE(R600_CP_ME_RAM_WADDR, 0);

		DRM_INFO("Loading RV770 CP Microcode\n");
		for (i = 0; i < R700_PM4_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_ME_RAM_DATA, RV770_cp_microcode[i]);
		}
		RADEON_WRITE(R600_CP_ME_RAM_WADDR, 0);

	} else if (((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV730)) {

		RADEON_WRITE(R600_CP_PFP_UCODE_ADDR, 0);

		DRM_INFO("Loading RV730 PFP Microcode\n");
		for (i = 0; i < R700_PFP_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_PFP_UCODE_DATA, RV730_pfp_microcode[i]);
		}
		RADEON_WRITE(R600_CP_PFP_UCODE_ADDR, 0);

		RADEON_WRITE(R600_CP_ME_RAM_WADDR, 0);

		DRM_INFO("Loading RV730 CP Microcode\n");
		for (i = 0; i < R700_PM4_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_ME_RAM_DATA, RV730_cp_microcode[i]);
		}
		RADEON_WRITE(R600_CP_ME_RAM_WADDR, 0);

	} else if (((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV710)) {

		RADEON_WRITE(R600_CP_PFP_UCODE_ADDR, 0);

		DRM_INFO("Loading RV710 PFP Microcode\n");
		for (i = 0; i < R700_PFP_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_PFP_UCODE_DATA, RV710_pfp_microcode[i]);
		}
		RADEON_WRITE(R600_CP_PFP_UCODE_ADDR, 0);

		RADEON_WRITE(R600_CP_ME_RAM_WADDR, 0);

		DRM_INFO("Loading RV710 CP Microcode\n");
		for (i = 0; i < R700_PM4_UCODE_SIZE; i++) {
			RADEON_WRITE(R600_CP_ME_RAM_DATA, RV710_cp_microcode[i]);
		}
		RADEON_WRITE(R600_CP_ME_RAM_WADDR, 0);

	}
	RADEON_WRITE(R600_CP_PFP_UCODE_ADDR, 0);
	RADEON_WRITE(R600_CP_ME_RAM_WADDR, 0);
	RADEON_WRITE(R600_CP_ME_RAM_RADDR, 0);

}

static void r600_test_writeback(drm_radeon_private_t * dev_priv)
{
	u32 tmp;

	/* Writeback doesn't seem to work everywhere, test it here and possibly
	 * enable it if it appears to work
	 */
	DRM_WRITE32(dev_priv->ring_rptr, R600_SCRATCHOFF(1), 0);
	RADEON_WRITE(R600_SCRATCH_REG1, 0xdeadbeef);

	for (tmp = 0; tmp < dev_priv->usec_timeout; tmp++) {
		if (DRM_READ32(dev_priv->ring_rptr, R600_SCRATCHOFF(1)) ==
		    0xdeadbeef)
			break;
		DRM_UDELAY(1);
	}

	if (tmp < dev_priv->usec_timeout) {
		dev_priv->writeback_works = 1;
		DRM_INFO("writeback test succeeded in %d usecs\n", tmp);
	} else {
		dev_priv->writeback_works = 0;

		for (tmp = 0; tmp < 512; tmp+=16 )
			DRM_DEBUG("%d %x %x %x %x\n",  tmp, DRM_READ32(dev_priv->ring_rptr, tmp),
				  DRM_READ32(dev_priv->ring_rptr, tmp + 4),
				  DRM_READ32(dev_priv->ring_rptr, tmp + 8),
				  DRM_READ32(dev_priv->ring_rptr, tmp + 16));

		DRM_INFO("writeback test failed %x %x\n", DRM_READ32(dev_priv->ring_rptr, R600_SCRATCHOFF(1)), RADEON_READ(R600_SCRATCH_REG1));
	}
	if (radeon_no_wb == 1) {
		dev_priv->writeback_works = 0;
		DRM_INFO("writeback forced off\n");
	}

	if (!dev_priv->writeback_works) {
		/* Disable writeback to avoid unnecessary bus master transfers */
		RADEON_WRITE(R600_CP_RB_CNTL, RADEON_READ(R600_CP_RB_CNTL) | RADEON_RB_NO_UPDATE);
		RADEON_WRITE(R600_SCRATCH_UMSK, 0);
	}
}

int r600_engine_reset(struct drm_device * dev)
{
	drm_radeon_private_t *dev_priv = dev->dev_private;
	u32 cp_ptr, cp_me_cntl, cp_rb_cntl;

	DRM_INFO("Resetting GPU\n");

	cp_ptr = RADEON_READ(R600_CP_RB_WPTR);
	cp_me_cntl = RADEON_READ(R600_CP_ME_CNTL);
	RADEON_WRITE(R600_CP_ME_CNTL, R600_CP_ME_HALT);

	RADEON_WRITE(R600_GRBM_SOFT_RESET, 0x7fff);
	RADEON_READ(R600_GRBM_SOFT_RESET);
	DRM_UDELAY(50);
	RADEON_WRITE(R600_GRBM_SOFT_RESET, 0);
	RADEON_READ(R600_GRBM_SOFT_RESET);

	RADEON_WRITE(R600_CP_RB_WPTR_DELAY, 0);
	cp_rb_cntl = RADEON_READ(R600_CP_RB_CNTL);
	RADEON_WRITE(R600_CP_RB_CNTL, R600_RB_RPTR_WR_ENA);

	RADEON_WRITE(R600_CP_RB_RPTR_WR, cp_ptr);
	RADEON_WRITE(R600_CP_RB_WPTR, cp_ptr);
	RADEON_WRITE(R600_CP_RB_CNTL, cp_rb_cntl);
	RADEON_WRITE(R600_CP_ME_CNTL, cp_me_cntl);

	/* Reset the CP ring */
	r600_do_cp_reset(dev_priv);

	/* The CP is no longer running after an engine reset */
	dev_priv->cp_running = 0;

	/* Reset any pending vertex, indirect buffers */
	radeon_freelist_reset(dev);

	return 0;

}

static u32 r600_get_tile_pipe_to_backend_map(u32 num_tile_pipes,
					     u32 num_backends,
					     u32 backend_disable_mask)
{
	u32 backend_map = 0;
	u32 enabled_backends_mask;
	u32 enabled_backends_count;
	u32 cur_pipe;
	u32 swizzle_pipe[R6XX_MAX_PIPES];
	u32 cur_backend;
	u32 i;

	if (num_tile_pipes > R6XX_MAX_PIPES)
		num_tile_pipes = R6XX_MAX_PIPES;
	if (num_tile_pipes < 1)
		num_tile_pipes = 1;
	if (num_backends > R6XX_MAX_BACKENDS)
		num_backends = R6XX_MAX_BACKENDS;
	if (num_backends < 1)
		num_backends = 1;

	enabled_backends_mask = 0;
	enabled_backends_count = 0;
	for (i = 0; i < R6XX_MAX_BACKENDS; ++i) {
		if (((backend_disable_mask >> i) & 1) == 0) {
			enabled_backends_mask |= (1 << i);
			++enabled_backends_count;
		}
		if (enabled_backends_count == num_backends)
			break;
	}

	if (enabled_backends_count == 0) {
		enabled_backends_mask = 1;
		enabled_backends_count = 1;
	}

	if (enabled_backends_count != num_backends)
		num_backends = enabled_backends_count;

	memset((uint8_t *)&swizzle_pipe[0], 0, sizeof(u32) * R6XX_MAX_PIPES);
	switch (num_tile_pipes) {
	case 1:
		swizzle_pipe[0] = 0;
		break;
	case 2:
		swizzle_pipe[0] = 0;
		swizzle_pipe[1] = 1;
		break;
	case 3:
		swizzle_pipe[0] = 0;
		swizzle_pipe[1] = 1;
		swizzle_pipe[2] = 2;
		break;
	case 4:
		swizzle_pipe[0] = 0;
		swizzle_pipe[1] = 1;
		swizzle_pipe[2] = 2;
		swizzle_pipe[3] = 3;
		break;
	case 5:
		swizzle_pipe[0] = 0;
		swizzle_pipe[1] = 1;
		swizzle_pipe[2] = 2;
		swizzle_pipe[3] = 3;
		swizzle_pipe[4] = 4;
		break;
	case 6:
		swizzle_pipe[0] = 0;
		swizzle_pipe[1] = 2;
		swizzle_pipe[2] = 4;
		swizzle_pipe[3] = 5;
		swizzle_pipe[4] = 1;
		swizzle_pipe[5] = 3;
		break;
	case 7:
		swizzle_pipe[0] = 0;
		swizzle_pipe[1] = 2;
		swizzle_pipe[2] = 4;
		swizzle_pipe[3] = 6;
		swizzle_pipe[4] = 1;
		swizzle_pipe[5] = 3;
		swizzle_pipe[6] = 5;
		break;
	case 8:
		swizzle_pipe[0] = 0;
		swizzle_pipe[1] = 2;
		swizzle_pipe[2] = 4;
		swizzle_pipe[3] = 6;
		swizzle_pipe[4] = 1;
		swizzle_pipe[5] = 3;
		swizzle_pipe[6] = 5;
		swizzle_pipe[7] = 7;
		break;
	}

	cur_backend = 0;
	for (cur_pipe=0; cur_pipe<num_tile_pipes; ++cur_pipe) {
		while (((1 << cur_backend) & enabled_backends_mask) == 0)
			cur_backend = (cur_backend + 1) % R6XX_MAX_BACKENDS;

		backend_map |= (u32)(((cur_backend & 3) << (swizzle_pipe[cur_pipe] * 2)));

		cur_backend = (cur_backend + 1) % R6XX_MAX_BACKENDS;
	}

	return backend_map;
}

static int r600_count_pipe_bits (uint32_t val)
{
	int i, ret = 0;
	for (i = 0; i < 32; i++) {
		ret += val & 1;
		val >>= 1;
	}
	return ret;
}

static void r600_gfx_init(struct drm_device * dev,
			  drm_radeon_private_t * dev_priv)
{
	int i, j, num_qd_pipes;
	u32 sx_debug_1;
	u32 tc_cntl;
	u32 arb_pop;
	u32 num_gs_verts_per_thread;
        u32 vgt_gs_per_es;
	u32 gs_prim_buffer_depth = 0;
	u32 sq_ms_fifo_sizes;
	u32 sq_config;
	u32 sq_gpr_resource_mgmt_1 = 0;
	u32 sq_gpr_resource_mgmt_2 = 0;
	u32 sq_thread_resource_mgmt = 0;
	u32 sq_stack_resource_mgmt_1 = 0;
	u32 sq_stack_resource_mgmt_2 = 0;
	u32 hdp_host_path_cntl;
	u32 backend_map;
	u32 gb_tiling_config = 0;
	u32 cc_rb_backend_disable = 0;
	u32 cc_gc_shader_pipe_config = 0;
        u32 ramcfg;

	/* setup chip specs */
        switch (dev_priv->flags & RADEON_FAMILY_MASK) {
        case CHIP_R600:
		dev_priv->r600_max_pipes = 4;
		dev_priv->r600_max_tile_pipes = 8;
		dev_priv->r600_max_simds = 4;
		dev_priv->r600_max_backends = 4;
		dev_priv->r600_max_gprs = 256;
		dev_priv->r600_max_threads = 192;
		dev_priv->r600_max_stack_entries = 256;
		dev_priv->r600_max_hw_contexts = 8;
		dev_priv->r600_max_gs_threads = 16;
		dev_priv->r600_sx_max_export_size = 128;
		dev_priv->r600_sx_max_export_pos_size = 16;
		dev_priv->r600_sx_max_export_smx_size = 128;
		dev_priv->r600_sq_num_cf_insts = 2;
		break;
        case CHIP_RV630:
	case CHIP_RV635:
		dev_priv->r600_max_pipes = 2;
		dev_priv->r600_max_tile_pipes = 2;
		dev_priv->r600_max_simds = 3;
		dev_priv->r600_max_backends = 1;
		dev_priv->r600_max_gprs = 128;
		dev_priv->r600_max_threads = 192;
		dev_priv->r600_max_stack_entries = 128;
		dev_priv->r600_max_hw_contexts = 8;
		dev_priv->r600_max_gs_threads = 4;
		dev_priv->r600_sx_max_export_size = 128;
		dev_priv->r600_sx_max_export_pos_size = 16;
		dev_priv->r600_sx_max_export_smx_size = 128;
		dev_priv->r600_sq_num_cf_insts = 2;
		break;
        case CHIP_RV610:
        case CHIP_RS780:
        case CHIP_RV620:
		dev_priv->r600_max_pipes = 1;
		dev_priv->r600_max_tile_pipes = 1;
		dev_priv->r600_max_simds = 2;
		dev_priv->r600_max_backends = 1;
		dev_priv->r600_max_gprs = 128;
		dev_priv->r600_max_threads = 192;
		dev_priv->r600_max_stack_entries = 128;
		dev_priv->r600_max_hw_contexts = 4;
		dev_priv->r600_max_gs_threads = 4;
		dev_priv->r600_sx_max_export_size = 128;
		dev_priv->r600_sx_max_export_pos_size = 16;
		dev_priv->r600_sx_max_export_smx_size = 128;
		dev_priv->r600_sq_num_cf_insts = 1;
		break;
        case CHIP_RV670:
		dev_priv->r600_max_pipes = 4;
		dev_priv->r600_max_tile_pipes = 4;
		dev_priv->r600_max_simds = 4;
		dev_priv->r600_max_backends = 4;
		dev_priv->r600_max_gprs = 192;
		dev_priv->r600_max_threads = 192;
		dev_priv->r600_max_stack_entries = 256;
		dev_priv->r600_max_hw_contexts = 8;
		dev_priv->r600_max_gs_threads = 16;
		dev_priv->r600_sx_max_export_size = 128;
		dev_priv->r600_sx_max_export_pos_size = 16;
		dev_priv->r600_sx_max_export_smx_size = 128;
		dev_priv->r600_sq_num_cf_insts = 2;
		break;
        default:
		break;
        }

	/* Initialize HDP */
	j = 0;
	for (i = 0; i < 32; i++) {
		RADEON_WRITE((0x2c14 + j), 0x00000000);
		RADEON_WRITE((0x2c18 + j), 0x00000000);
		RADEON_WRITE((0x2c1c + j), 0x00000000);
		RADEON_WRITE((0x2c20 + j), 0x00000000);
		RADEON_WRITE((0x2c24 + j), 0x00000000);
		j += 0x18;
	}

	RADEON_WRITE(R600_GRBM_CNTL, R600_GRBM_READ_TIMEOUT(0xff));

	/* setup tiling, simd, pipe config */
	ramcfg = RADEON_READ(R600_RAMCFG);

	switch (dev_priv->r600_max_tile_pipes) {
	case 1:
		gb_tiling_config |= R600_PIPE_TILING(0);
                break;
	case 2:
		gb_tiling_config |= R600_PIPE_TILING(1);
                break;
	case 4:
		gb_tiling_config |= R600_PIPE_TILING(2);
                break;
	case 8:
		gb_tiling_config |= R600_PIPE_TILING(3);
                break;
	default:
		break;
	}

	gb_tiling_config |= R600_BANK_TILING((ramcfg >> R600_NOOFBANK_SHIFT) & R600_NOOFBANK_MASK);

	gb_tiling_config |= R600_GROUP_SIZE(0);

	if (((ramcfg >> R600_NOOFROWS_SHIFT) & R600_NOOFROWS_MASK) > 3) {
		gb_tiling_config |= R600_ROW_TILING(3);
		gb_tiling_config |= R600_SAMPLE_SPLIT(3);
	} else {
		gb_tiling_config |=
			R600_ROW_TILING(((ramcfg >> R600_NOOFROWS_SHIFT) & R600_NOOFROWS_MASK));
		gb_tiling_config |=
			R600_SAMPLE_SPLIT(((ramcfg >> R600_NOOFROWS_SHIFT) & R600_NOOFROWS_MASK));
	}

	gb_tiling_config |= R600_BANK_SWAPS(1);

	backend_map = r600_get_tile_pipe_to_backend_map(dev_priv->r600_max_tile_pipes,
							dev_priv->r600_max_backends,
							(0xff << dev_priv->r600_max_backends) & 0xff);
	gb_tiling_config |= R600_BACKEND_MAP(backend_map);

	cc_gc_shader_pipe_config =
		R600_INACTIVE_QD_PIPES((R6XX_MAX_PIPES_MASK << dev_priv->r600_max_pipes) & R6XX_MAX_PIPES_MASK);
	cc_gc_shader_pipe_config |=
		R600_INACTIVE_SIMDS((R6XX_MAX_SIMDS_MASK << dev_priv->r600_max_simds) & R6XX_MAX_SIMDS_MASK);

	cc_rb_backend_disable =
		R600_BACKEND_DISABLE((R6XX_MAX_BACKENDS_MASK << dev_priv->r600_max_backends) & R6XX_MAX_BACKENDS_MASK);

	RADEON_WRITE(R600_GB_TILING_CONFIG,      gb_tiling_config);
	RADEON_WRITE(R600_DCP_TILING_CONFIG,    (gb_tiling_config & 0xffff));
	RADEON_WRITE(R600_HDP_TILING_CONFIG,    (gb_tiling_config & 0xffff));

	RADEON_WRITE(R600_CC_RB_BACKEND_DISABLE,      cc_rb_backend_disable);
	RADEON_WRITE(R600_CC_GC_SHADER_PIPE_CONFIG,   cc_gc_shader_pipe_config);
	RADEON_WRITE(R600_GC_USER_SHADER_PIPE_CONFIG, cc_gc_shader_pipe_config);

	num_qd_pipes =
		R6XX_MAX_BACKENDS - r600_count_pipe_bits(cc_gc_shader_pipe_config & R600_INACTIVE_QD_PIPES_MASK);
	RADEON_WRITE(R600_VGT_OUT_DEALLOC_CNTL, (num_qd_pipes * 4) & R600_DEALLOC_DIST_MASK);
	RADEON_WRITE(R600_VGT_VERTEX_REUSE_BLOCK_CNTL, ((num_qd_pipes * 4) - 2) & R600_VTX_REUSE_DEPTH_MASK);

	/* set HW defaults for 3D engine */
	RADEON_WRITE(R600_CP_QUEUE_THRESHOLDS, (R600_ROQ_IB1_START(0x16) |
						R600_ROQ_IB2_START(0x2b)));

	RADEON_WRITE(R600_CP_MEQ_THRESHOLDS, (R600_MEQ_END(0x40) |
					      R600_ROQ_END(0x40)));

	RADEON_WRITE(R600_TA_CNTL_AUX, (R600_DISABLE_CUBE_ANISO |
					R600_SYNC_GRADIENT |
					R600_SYNC_WALKER |
					R600_SYNC_ALIGNER));

	if ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV670)
		RADEON_WRITE(R600_ARB_GDEC_RD_CNTL, 0x00000021);

	sx_debug_1 = RADEON_READ(R600_SX_DEBUG_1);
	sx_debug_1 |= R600_SMX_EVENT_RELEASE;
	if (((dev_priv->flags & RADEON_FAMILY_MASK) > CHIP_R600))
		sx_debug_1 |= R600_ENABLE_NEW_SMX_ADDRESS;
	RADEON_WRITE(R600_SX_DEBUG_1, sx_debug_1);

	if (((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_R600) ||
	    ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV630) ||
	    ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV610) ||
	    ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV620) ||
	    ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RS780))
		RADEON_WRITE(R600_DB_DEBUG, R600_PREZ_MUST_WAIT_FOR_POSTZ_DONE);
	else
		RADEON_WRITE(R600_DB_DEBUG, 0);

        RADEON_WRITE(R600_DB_WATERMARKS, (R600_DEPTH_FREE(4) |
					  R600_DEPTH_FLUSH(16) |
					  R600_DEPTH_PENDING_FREE(4) |
					  R600_DEPTH_CACHELINE_FREE(16)));
        RADEON_WRITE(R600_PA_SC_MULTI_CHIP_CNTL, 0);
        RADEON_WRITE(R600_VGT_NUM_INSTANCES, 0);

	RADEON_WRITE(R600_SPI_CONFIG_CNTL, R600_GPR_WRITE_PRIORITY(0));
	RADEON_WRITE(R600_SPI_CONFIG_CNTL_1, R600_VTX_DONE_DELAY(0));

	sq_ms_fifo_sizes = RADEON_READ(R600_SQ_MS_FIFO_SIZES);
	if (((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV610) ||
	    ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV620) ||
	    ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RS780)) {
		sq_ms_fifo_sizes = (R600_CACHE_FIFO_SIZE(0xa) |
				    R600_FETCH_FIFO_HIWATER(0xa) |
				    R600_DONE_FIFO_HIWATER(0xe0) |
				    R600_ALU_UPDATE_FIFO_HIWATER(0x8));
	} else if (((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_R600) ||
		   ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV630)) {
		sq_ms_fifo_sizes &= ~R600_DONE_FIFO_HIWATER(0xff);
		sq_ms_fifo_sizes |= R600_DONE_FIFO_HIWATER(0x4);
	}
	RADEON_WRITE(R600_SQ_MS_FIFO_SIZES, sq_ms_fifo_sizes);

	/* SQ_CONFIG, SQ_GPR_RESOURCE_MGMT, SQ_THREAD_RESOURCE_MGMT, SQ_STACK_RESOURCE_MGMT
	 * should be adjusted as needed by the 2D/3D drivers.  This just sets default values
	 */
	sq_config = RADEON_READ(R600_SQ_CONFIG);
	sq_config &= ~(R600_PS_PRIO(3) |
		       R600_VS_PRIO(3) |
		       R600_GS_PRIO(3) |
		       R600_ES_PRIO(3));
	sq_config |= (R600_DX9_CONSTS |
		      R600_VC_ENABLE |
		      R600_PS_PRIO(0) |
		      R600_VS_PRIO(1) |
		      R600_GS_PRIO(2) |
		      R600_ES_PRIO(3));

	if ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_R600) {
		sq_gpr_resource_mgmt_1 = (R600_NUM_PS_GPRS(124) |
					  R600_NUM_VS_GPRS(124) |
					  R600_NUM_CLAUSE_TEMP_GPRS(4));
		sq_gpr_resource_mgmt_2 = (R600_NUM_GS_GPRS(0) |
					  R600_NUM_ES_GPRS(0));
		sq_thread_resource_mgmt = (R600_NUM_PS_THREADS(136) |
					   R600_NUM_VS_THREADS(48) |
					   R600_NUM_GS_THREADS(4) |
					   R600_NUM_ES_THREADS(4));
		sq_stack_resource_mgmt_1 = (R600_NUM_PS_STACK_ENTRIES(128) |
					    R600_NUM_VS_STACK_ENTRIES(128));
		sq_stack_resource_mgmt_2 = (R600_NUM_GS_STACK_ENTRIES(0) |
					    R600_NUM_ES_STACK_ENTRIES(0));
	} else if (((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV610) ||
		   ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV620) ||
		   ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RS780)) {
		/* no vertex cache */
		sq_config &= ~R600_VC_ENABLE;

		sq_gpr_resource_mgmt_1 = (R600_NUM_PS_GPRS(44) |
					  R600_NUM_VS_GPRS(44) |
					  R600_NUM_CLAUSE_TEMP_GPRS(2));
		sq_gpr_resource_mgmt_2 = (R600_NUM_GS_GPRS(17) |
					  R600_NUM_ES_GPRS(17));
		sq_thread_resource_mgmt = (R600_NUM_PS_THREADS(79) |
					   R600_NUM_VS_THREADS(78) |
					   R600_NUM_GS_THREADS(4) |
					   R600_NUM_ES_THREADS(31));
		sq_stack_resource_mgmt_1 = (R600_NUM_PS_STACK_ENTRIES(40) |
					    R600_NUM_VS_STACK_ENTRIES(40));
		sq_stack_resource_mgmt_2 = (R600_NUM_GS_STACK_ENTRIES(32) |
					    R600_NUM_ES_STACK_ENTRIES(16));
	} else if (((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV630) ||
		   ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV635)){
		sq_gpr_resource_mgmt_1 = (R600_NUM_PS_GPRS(44) |
					  R600_NUM_VS_GPRS(44) |
					  R600_NUM_CLAUSE_TEMP_GPRS(2));
		sq_gpr_resource_mgmt_2 = (R600_NUM_GS_GPRS(18) |
					  R600_NUM_ES_GPRS(18));
		sq_thread_resource_mgmt = (R600_NUM_PS_THREADS(79) |
					   R600_NUM_VS_THREADS(78) |
					   R600_NUM_GS_THREADS(4) |
					   R600_NUM_ES_THREADS(31));
		sq_stack_resource_mgmt_1 = (R600_NUM_PS_STACK_ENTRIES(40) |
					    R600_NUM_VS_STACK_ENTRIES(40));
		sq_stack_resource_mgmt_2 = (R600_NUM_GS_STACK_ENTRIES(32) |
					    R600_NUM_ES_STACK_ENTRIES(16));
	} else if ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV670) {
		sq_gpr_resource_mgmt_1 = (R600_NUM_PS_GPRS(44) |
					  R600_NUM_VS_GPRS(44) |
					  R600_NUM_CLAUSE_TEMP_GPRS(2));
		sq_gpr_resource_mgmt_2 = (R600_NUM_GS_GPRS(17) |
					  R600_NUM_ES_GPRS(17));
		sq_thread_resource_mgmt = (R600_NUM_PS_THREADS(79) |
					   R600_NUM_VS_THREADS(78) |
					   R600_NUM_GS_THREADS(4) |
					   R600_NUM_ES_THREADS(31));
		sq_stack_resource_mgmt_1 = (R600_NUM_PS_STACK_ENTRIES(64) |
					    R600_NUM_VS_STACK_ENTRIES(64));
		sq_stack_resource_mgmt_2 = (R600_NUM_GS_STACK_ENTRIES(64) |
					    R600_NUM_ES_STACK_ENTRIES(64));
	}

        RADEON_WRITE(R600_SQ_CONFIG, sq_config);
        RADEON_WRITE(R600_SQ_GPR_RESOURCE_MGMT_1,  sq_gpr_resource_mgmt_1);
        RADEON_WRITE(R600_SQ_GPR_RESOURCE_MGMT_2,  sq_gpr_resource_mgmt_2);
        RADEON_WRITE(R600_SQ_THREAD_RESOURCE_MGMT, sq_thread_resource_mgmt);
        RADEON_WRITE(R600_SQ_STACK_RESOURCE_MGMT_1, sq_stack_resource_mgmt_1);
        RADEON_WRITE(R600_SQ_STACK_RESOURCE_MGMT_2, sq_stack_resource_mgmt_2);

	if (((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV610) ||
	    ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV620) ||
	    ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RS780))
		RADEON_WRITE(R600_VGT_CACHE_INVALIDATION, R600_CACHE_INVALIDATION(R600_TC_ONLY));
	else
		RADEON_WRITE(R600_VGT_CACHE_INVALIDATION, R600_CACHE_INVALIDATION(R600_VC_AND_TC));

        RADEON_WRITE(R600_PA_SC_AA_SAMPLE_LOCS_2S, ((0xc << 0) |
						    (0x4 << 4) |
						    (0x4 << 8) |
						    (0xc << 12)));
        RADEON_WRITE(R600_PA_SC_AA_SAMPLE_LOCS_4S, ((0xe << 0) |
						    (0xe << 4) |
						    (0x2 << 8) |
						    (0x2 << 12) |
						    (0xa << 16) |
						    (0x6 << 20) |
						    (0x6 << 24) |
						    (0xa << 28)));
        RADEON_WRITE(R600_PA_SC_AA_SAMPLE_LOCS_8S_WD0, ((0xe << 0) |
							(0xb << 4) |
							(0x4 << 8) |
							(0xc << 12) |
							(0x1 << 16) |
							(0x6 << 20) |
							(0xa << 24) |
							(0xe << 28)));
        RADEON_WRITE(R600_PA_SC_AA_SAMPLE_LOCS_8S_WD1, ((0x6 << 0) |
							(0x1 << 4) |
							(0x0 << 8) |
							(0x0 << 12) |
							(0xb << 16) |
							(0x4 << 20) |
							(0x7 << 24) |
							(0x8 << 28)));


	switch (dev_priv->flags & RADEON_FAMILY_MASK) {
        case CHIP_R600:
        case CHIP_RV630:
	case CHIP_RV635:
		gs_prim_buffer_depth = 0;
		break;
        case CHIP_RV610:
        case CHIP_RS780:
        case CHIP_RV620:
		gs_prim_buffer_depth = 32;
		break;
        case CHIP_RV670:
		gs_prim_buffer_depth = 128;
		break;
        default:
		break;
        }

        num_gs_verts_per_thread = dev_priv->r600_max_pipes * 16;
        vgt_gs_per_es = gs_prim_buffer_depth + num_gs_verts_per_thread;
        /* Max value for this is 256 */
        if (vgt_gs_per_es > 256)
		vgt_gs_per_es = 256;

        RADEON_WRITE(R600_VGT_ES_PER_GS, 128);
        RADEON_WRITE(R600_VGT_GS_PER_ES, vgt_gs_per_es);
        RADEON_WRITE(R600_VGT_GS_PER_VS, 2);
        RADEON_WRITE(R600_VGT_GS_VERTEX_REUSE, 16);

	/* more default values. 2D/3D driver should adjust as needed */
        RADEON_WRITE(R600_PA_SC_LINE_STIPPLE_STATE, 0);
        RADEON_WRITE(R600_VGT_STRMOUT_EN, 0);
        RADEON_WRITE(R600_SX_MISC, 0);
        RADEON_WRITE(R600_PA_SC_MODE_CNTL, 0);
        RADEON_WRITE(R600_PA_SC_AA_CONFIG, 0);
        RADEON_WRITE(R600_PA_SC_LINE_STIPPLE, 0);
        RADEON_WRITE(R600_SPI_INPUT_Z, 0);
        RADEON_WRITE(R600_SPI_PS_IN_CONTROL_0, R600_NUM_INTERP(2));
        RADEON_WRITE(R600_CB_COLOR7_FRAG, 0);

	/* clear render buffer base addresses */
	RADEON_WRITE(R600_CB_COLOR0_BASE, 0);
	RADEON_WRITE(R600_CB_COLOR1_BASE, 0);
	RADEON_WRITE(R600_CB_COLOR2_BASE, 0);
	RADEON_WRITE(R600_CB_COLOR3_BASE, 0);
	RADEON_WRITE(R600_CB_COLOR4_BASE, 0);
	RADEON_WRITE(R600_CB_COLOR5_BASE, 0);
	RADEON_WRITE(R600_CB_COLOR6_BASE, 0);
	RADEON_WRITE(R600_CB_COLOR7_BASE, 0);

        switch (dev_priv->flags & RADEON_FAMILY_MASK) {
        case CHIP_RV610:
        case CHIP_RS780:
        case CHIP_RV620:
		tc_cntl = R600_TC_L2_SIZE(8);
		break;
        case CHIP_RV630:
        case CHIP_RV635:
		tc_cntl = R600_TC_L2_SIZE(4);
		break;
        case CHIP_R600:
		tc_cntl = R600_TC_L2_SIZE(0) | R600_L2_DISABLE_LATE_HIT;
		break;
        default:
		tc_cntl = R600_TC_L2_SIZE(0);
		break;
        }

	RADEON_WRITE(R600_TC_CNTL, tc_cntl);

	hdp_host_path_cntl = RADEON_READ(R600_HDP_HOST_PATH_CNTL);
	RADEON_WRITE(R600_HDP_HOST_PATH_CNTL, hdp_host_path_cntl);

	arb_pop = RADEON_READ(R600_ARB_POP);
	arb_pop |= R600_ENABLE_TC128;
	RADEON_WRITE(R600_ARB_POP, arb_pop);

	RADEON_WRITE(R600_PA_SC_MULTI_CHIP_CNTL, 0);
	RADEON_WRITE(R600_PA_CL_ENHANCE, (R600_CLIP_VTX_REORDER_ENA |
					  R600_NUM_CLIP_SEQ(3)));
	RADEON_WRITE(R600_PA_SC_ENHANCE, R600_FORCE_EOV_MAX_CLK_CNT(4095));

}

static u32 r700_get_tile_pipe_to_backend_map(u32 num_tile_pipes,
					     u32 num_backends,
					     u32 backend_disable_mask)
{
	u32 backend_map = 0;
	u32 enabled_backends_mask;
	u32 enabled_backends_count;
	u32 cur_pipe;
	u32 swizzle_pipe[R7XX_MAX_PIPES];
	u32 cur_backend;
	u32 i;

	if (num_tile_pipes > R7XX_MAX_PIPES)
		num_tile_pipes = R7XX_MAX_PIPES;
	if (num_tile_pipes < 1)
		num_tile_pipes = 1;
	if (num_backends > R7XX_MAX_BACKENDS)
		num_backends = R7XX_MAX_BACKENDS;
	if (num_backends < 1)
		num_backends = 1;

	enabled_backends_mask = 0;
	enabled_backends_count = 0;
	for (i = 0; i < R7XX_MAX_BACKENDS; ++i) {
		if (((backend_disable_mask >> i) & 1) == 0) {
			enabled_backends_mask |= (1 << i);
			++enabled_backends_count;
		}
		if (enabled_backends_count == num_backends)
			break;
	}

	if (enabled_backends_count == 0) {
		enabled_backends_mask = 1;
		enabled_backends_count = 1;
	}

	if (enabled_backends_count != num_backends)
		num_backends = enabled_backends_count;

	memset((uint8_t *)&swizzle_pipe[0], 0, sizeof(u32) * R7XX_MAX_PIPES);
	switch (num_tile_pipes) {
	case 1:
		swizzle_pipe[0] = 0;
		break;
	case 2:
		swizzle_pipe[0] = 0;
		swizzle_pipe[1] = 1;
		break;
	case 3:
		swizzle_pipe[0] = 0;
		swizzle_pipe[1] = 2;
		swizzle_pipe[2] = 1;
		break;
	case 4:
		swizzle_pipe[0] = 0;
		swizzle_pipe[1] = 2;
		swizzle_pipe[2] = 3;
		swizzle_pipe[3] = 1;
		break;
	case 5:
		swizzle_pipe[0] = 0;
		swizzle_pipe[1] = 2;
		swizzle_pipe[2] = 4;
		swizzle_pipe[3] = 1;
		swizzle_pipe[4] = 3;
		break;
	case 6:
		swizzle_pipe[0] = 0;
		swizzle_pipe[1] = 2;
		swizzle_pipe[2] = 4;
		swizzle_pipe[3] = 5;
		swizzle_pipe[4] = 3;
		swizzle_pipe[5] = 1;
		break;
	case 7:
		swizzle_pipe[0] = 0;
		swizzle_pipe[1] = 2;
		swizzle_pipe[2] = 4;
		swizzle_pipe[3] = 6;
		swizzle_pipe[4] = 3;
		swizzle_pipe[5] = 1;
		swizzle_pipe[6] = 5;
		break;
	case 8:
		swizzle_pipe[0] = 0;
		swizzle_pipe[1] = 2;
		swizzle_pipe[2] = 4;
		swizzle_pipe[3] = 6;
		swizzle_pipe[4] = 3;
		swizzle_pipe[5] = 1;
		swizzle_pipe[6] = 7;
		swizzle_pipe[7] = 5;
		break;
	}

	cur_backend = 0;
	for (cur_pipe = 0; cur_pipe < num_tile_pipes; ++cur_pipe) {
		while (((1 << cur_backend) & enabled_backends_mask) == 0)
			cur_backend = (cur_backend + 1) % R7XX_MAX_BACKENDS;

		backend_map |= (u32)(((cur_backend & 3) << (swizzle_pipe[cur_pipe] * 2)));

		cur_backend = (cur_backend + 1) % R7XX_MAX_BACKENDS;
	}

	return backend_map;
}

static void r700_gfx_init(struct drm_device * dev,
			  drm_radeon_private_t * dev_priv)
{
	int i, j, num_qd_pipes;
	u32 sx_debug_1;
	u32 smx_dc_ctl0;
	u32 num_gs_verts_per_thread;
        u32 vgt_gs_per_es;
	u32 gs_prim_buffer_depth = 0;
	u32 sq_ms_fifo_sizes;
	u32 sq_config;
	u32 sq_thread_resource_mgmt;
	u32 hdp_host_path_cntl;
	u32 sq_dyn_gpr_size_simd_ab_0;
	u32 backend_map;
	u32 gb_tiling_config = 0;
	u32 cc_rb_backend_disable = 0;
	u32 cc_gc_shader_pipe_config = 0;
        u32 mc_arb_ramcfg;
	u32 db_debug4;

	/* setup chip specs */
        switch (dev_priv->flags & RADEON_FAMILY_MASK) {
	case CHIP_RV770:
		dev_priv->r600_max_pipes = 4;
		dev_priv->r600_max_tile_pipes = 8;
		dev_priv->r600_max_simds = 10;
		dev_priv->r600_max_backends = 4;
		dev_priv->r600_max_gprs = 256;
		dev_priv->r600_max_threads = 248;
		dev_priv->r600_max_stack_entries = 512;
		dev_priv->r600_max_hw_contexts = 8;
		dev_priv->r600_max_gs_threads = 16 * 2;
		dev_priv->r600_sx_max_export_size = 128;
		dev_priv->r600_sx_max_export_pos_size = 16;
		dev_priv->r600_sx_max_export_smx_size = 112;
		dev_priv->r600_sq_num_cf_insts = 2;

		dev_priv->r700_sx_num_of_sets = 7;
		dev_priv->r700_sc_prim_fifo_size = 0xF9;
		dev_priv->r700_sc_hiz_tile_fifo_size = 0x30;
		dev_priv->r700_sc_earlyz_tile_fifo_fize = 0x130;
		break;
	case CHIP_RV730:
		dev_priv->r600_max_pipes = 2;
		dev_priv->r600_max_tile_pipes = 4;
		dev_priv->r600_max_simds = 8;
		dev_priv->r600_max_backends = 2;
		dev_priv->r600_max_gprs = 128;
		dev_priv->r600_max_threads = 248;
		dev_priv->r600_max_stack_entries = 256;
		dev_priv->r600_max_hw_contexts = 8;
		dev_priv->r600_max_gs_threads = 16 * 2;
		dev_priv->r600_sx_max_export_size = 256;
		dev_priv->r600_sx_max_export_pos_size = 32;
		dev_priv->r600_sx_max_export_smx_size = 224;
		dev_priv->r600_sq_num_cf_insts = 2;

		dev_priv->r700_sx_num_of_sets = 7;
		dev_priv->r700_sc_prim_fifo_size = 0xf9;
		dev_priv->r700_sc_hiz_tile_fifo_size = 0x30;
		dev_priv->r700_sc_earlyz_tile_fifo_fize = 0x130;
		break;
	case CHIP_RV710:
		dev_priv->r600_max_pipes = 2;
		dev_priv->r600_max_tile_pipes = 2;
		dev_priv->r600_max_simds = 2;
		dev_priv->r600_max_backends = 1;
		dev_priv->r600_max_gprs = 256;
		dev_priv->r600_max_threads = 192;
		dev_priv->r600_max_stack_entries = 256;
		dev_priv->r600_max_hw_contexts = 4;
		dev_priv->r600_max_gs_threads = 8 * 2;
		dev_priv->r600_sx_max_export_size = 128;
		dev_priv->r600_sx_max_export_pos_size = 16;
		dev_priv->r600_sx_max_export_smx_size = 112;
		dev_priv->r600_sq_num_cf_insts = 1;

		dev_priv->r700_sx_num_of_sets = 7;
		dev_priv->r700_sc_prim_fifo_size = 0x40;
		dev_priv->r700_sc_hiz_tile_fifo_size = 0x30;
		dev_priv->r700_sc_earlyz_tile_fifo_fize = 0x130;
		break;
        default:
		break;
        }

	/* Initialize HDP */
	j = 0;
	for (i = 0; i < 32; i++) {
		RADEON_WRITE((0x2c14 + j), 0x00000000);
		RADEON_WRITE((0x2c18 + j), 0x00000000);
		RADEON_WRITE((0x2c1c + j), 0x00000000);
		RADEON_WRITE((0x2c20 + j), 0x00000000);
		RADEON_WRITE((0x2c24 + j), 0x00000000);
		j += 0x18;
	}

	RADEON_WRITE(R600_GRBM_CNTL, R600_GRBM_READ_TIMEOUT(0xff));

	/* setup tiling, simd, pipe config */
	mc_arb_ramcfg = RADEON_READ(R700_MC_ARB_RAMCFG);

	switch (dev_priv->r600_max_tile_pipes) {
	case 1:
		gb_tiling_config |= R600_PIPE_TILING(0);
                break;
	case 2:
		gb_tiling_config |= R600_PIPE_TILING(1);
                break;
	case 4:
		gb_tiling_config |= R600_PIPE_TILING(2);
                break;
	case 8:
		gb_tiling_config |= R600_PIPE_TILING(3);
                break;
	default:
		break;
	}

	if ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV770)
		gb_tiling_config |= R600_BANK_TILING(1);
	else
		gb_tiling_config |= R600_BANK_TILING((mc_arb_ramcfg >> R700_NOOFBANK_SHIFT) & R700_NOOFBANK_MASK);

	gb_tiling_config |= R600_GROUP_SIZE(0);

	if (((mc_arb_ramcfg >> R700_NOOFROWS_SHIFT) & R700_NOOFROWS_MASK) > 3) {
		gb_tiling_config |= R600_ROW_TILING(3);
		gb_tiling_config |= R600_SAMPLE_SPLIT(3);
	} else {
		gb_tiling_config |=
			R600_ROW_TILING(((mc_arb_ramcfg >> R700_NOOFROWS_SHIFT) & R700_NOOFROWS_MASK));
		gb_tiling_config |=
			R600_SAMPLE_SPLIT(((mc_arb_ramcfg >> R700_NOOFROWS_SHIFT) & R700_NOOFROWS_MASK));
	}

	gb_tiling_config |= R600_BANK_SWAPS(1);

	backend_map = r700_get_tile_pipe_to_backend_map(dev_priv->r600_max_tile_pipes,
							dev_priv->r600_max_backends,
							(0xff << dev_priv->r600_max_backends) & 0xff);
	gb_tiling_config |= R600_BACKEND_MAP(backend_map);

	cc_gc_shader_pipe_config =
		R600_INACTIVE_QD_PIPES((R7XX_MAX_PIPES_MASK << dev_priv->r600_max_pipes) & R7XX_MAX_PIPES_MASK);
	cc_gc_shader_pipe_config |=
		R600_INACTIVE_SIMDS((R7XX_MAX_SIMDS_MASK << dev_priv->r600_max_simds) & R7XX_MAX_SIMDS_MASK);

	cc_rb_backend_disable =
		R600_BACKEND_DISABLE((R7XX_MAX_BACKENDS_MASK << dev_priv->r600_max_backends) & R7XX_MAX_BACKENDS_MASK);

	RADEON_WRITE(R600_GB_TILING_CONFIG,      gb_tiling_config);
	RADEON_WRITE(R600_DCP_TILING_CONFIG,    (gb_tiling_config & 0xffff));
	RADEON_WRITE(R600_HDP_TILING_CONFIG,    (gb_tiling_config & 0xffff));

	RADEON_WRITE(R600_CC_RB_BACKEND_DISABLE,      cc_rb_backend_disable);
	RADEON_WRITE(R600_CC_GC_SHADER_PIPE_CONFIG,   cc_gc_shader_pipe_config);
	RADEON_WRITE(R600_GC_USER_SHADER_PIPE_CONFIG, cc_gc_shader_pipe_config);

	RADEON_WRITE(R700_CC_SYS_RB_BACKEND_DISABLE, cc_rb_backend_disable);
	RADEON_WRITE(R700_CGTS_SYS_TCC_DISABLE, 0);
	RADEON_WRITE(R700_CGTS_TCC_DISABLE, 0);
	RADEON_WRITE(R700_CGTS_USER_SYS_TCC_DISABLE, 0);
	RADEON_WRITE(R700_CGTS_USER_TCC_DISABLE, 0);

	num_qd_pipes =
		R7XX_MAX_BACKENDS - r600_count_pipe_bits(cc_gc_shader_pipe_config & R600_INACTIVE_QD_PIPES_MASK);
	RADEON_WRITE(R600_VGT_OUT_DEALLOC_CNTL, (num_qd_pipes * 4) & R600_DEALLOC_DIST_MASK);
	RADEON_WRITE(R600_VGT_VERTEX_REUSE_BLOCK_CNTL, ((num_qd_pipes * 4) - 2) & R600_VTX_REUSE_DEPTH_MASK);

	/* set HW defaults for 3D engine */
	RADEON_WRITE(R600_CP_QUEUE_THRESHOLDS, (R600_ROQ_IB1_START(0x16) |
						R600_ROQ_IB2_START(0x2b)));

        RADEON_WRITE(R600_CP_MEQ_THRESHOLDS, R700_STQ_SPLIT(0x30));

	RADEON_WRITE(R600_TA_CNTL_AUX, (R600_DISABLE_CUBE_ANISO |
					R600_SYNC_GRADIENT |
					R600_SYNC_WALKER |
					R600_SYNC_ALIGNER));

	sx_debug_1 = RADEON_READ(R700_SX_DEBUG_1);
	sx_debug_1 |= R700_ENABLE_NEW_SMX_ADDRESS;
	RADEON_WRITE(R700_SX_DEBUG_1, sx_debug_1);

	smx_dc_ctl0 = RADEON_READ(R600_SMX_DC_CTL0);
	smx_dc_ctl0 &= ~R700_CACHE_DEPTH(0x1ff);
	smx_dc_ctl0 |= R700_CACHE_DEPTH((dev_priv->r700_sx_num_of_sets * 64) - 1);
	RADEON_WRITE(R600_SMX_DC_CTL0, smx_dc_ctl0);

	RADEON_WRITE(R700_SMX_EVENT_CTL, (R700_ES_FLUSH_CTL(4) |
					  R700_GS_FLUSH_CTL(4) |
					  R700_ACK_FLUSH_CTL(3) |
					  R700_SYNC_FLUSH_CTL));

	if ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV770)
		RADEON_WRITE(R700_DB_DEBUG3, R700_DB_CLK_OFF_DELAY(0x1f));
	else {
		db_debug4 = RADEON_READ(RV700_DB_DEBUG4);
		db_debug4 |= RV700_DISABLE_TILE_COVERED_FOR_PS_ITER;
		RADEON_WRITE(RV700_DB_DEBUG4, db_debug4);
	}

	RADEON_WRITE(R600_SX_EXPORT_BUFFER_SIZES, (R600_COLOR_BUFFER_SIZE((dev_priv->r600_sx_max_export_size / 4) - 1) |
						   R600_POSITION_BUFFER_SIZE((dev_priv->r600_sx_max_export_pos_size / 4) - 1) |
						   R600_SMX_BUFFER_SIZE((dev_priv->r600_sx_max_export_smx_size / 4) - 1)));

	RADEON_WRITE(R700_PA_SC_FIFO_SIZE_R7XX, (R700_SC_PRIM_FIFO_SIZE(dev_priv->r700_sc_prim_fifo_size) |
						 R700_SC_HIZ_TILE_FIFO_SIZE(dev_priv->r700_sc_hiz_tile_fifo_size) |
						 R700_SC_EARLYZ_TILE_FIFO_SIZE(dev_priv->r700_sc_earlyz_tile_fifo_fize)));

        RADEON_WRITE(R600_PA_SC_MULTI_CHIP_CNTL, 0);

        RADEON_WRITE(R600_VGT_NUM_INSTANCES, 1);

	RADEON_WRITE(R600_SPI_CONFIG_CNTL, R600_GPR_WRITE_PRIORITY(0));

	RADEON_WRITE(R600_SPI_CONFIG_CNTL_1, R600_VTX_DONE_DELAY(4));

        RADEON_WRITE(R600_CP_PERFMON_CNTL, 0);

       	sq_ms_fifo_sizes = (R600_CACHE_FIFO_SIZE(16 * dev_priv->r600_sq_num_cf_insts) |
			    R600_DONE_FIFO_HIWATER(0xe0) |
			    R600_ALU_UPDATE_FIFO_HIWATER(0x8));
	switch (dev_priv->flags & RADEON_FAMILY_MASK) {
	case CHIP_RV770:
		sq_ms_fifo_sizes |= R600_FETCH_FIFO_HIWATER(0x1);
		break;
	case CHIP_RV730:
	case CHIP_RV710:
	default:
		sq_ms_fifo_sizes |= R600_FETCH_FIFO_HIWATER(0x4);
		break;
	}
	RADEON_WRITE(R600_SQ_MS_FIFO_SIZES, sq_ms_fifo_sizes);

	/* SQ_CONFIG, SQ_GPR_RESOURCE_MGMT, SQ_THREAD_RESOURCE_MGMT, SQ_STACK_RESOURCE_MGMT
	 * should be adjusted as needed by the 2D/3D drivers.  This just sets default values
	 */
	sq_config = RADEON_READ(R600_SQ_CONFIG);
	sq_config &= ~(R600_PS_PRIO(3) |
		       R600_VS_PRIO(3) |
		       R600_GS_PRIO(3) |
		       R600_ES_PRIO(3));
	sq_config |= (R600_DX9_CONSTS |
		      R600_VC_ENABLE |
		      R600_EXPORT_SRC_C |
		      R600_PS_PRIO(0) |
		      R600_VS_PRIO(1) |
		      R600_GS_PRIO(2) |
		      R600_ES_PRIO(3));
	if ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV710)
		/* no vertex cache */
		sq_config &= ~R600_VC_ENABLE;

	RADEON_WRITE(R600_SQ_CONFIG, sq_config);

	RADEON_WRITE(R600_SQ_GPR_RESOURCE_MGMT_1,  (R600_NUM_PS_GPRS((dev_priv->r600_max_gprs * 24)/64) |
						    R600_NUM_VS_GPRS((dev_priv->r600_max_gprs * 24)/64) |
						    R600_NUM_CLAUSE_TEMP_GPRS(((dev_priv->r600_max_gprs * 24)/64)/2)));

	RADEON_WRITE(R600_SQ_GPR_RESOURCE_MGMT_2,  (R600_NUM_GS_GPRS((dev_priv->r600_max_gprs * 7)/64) |
						    R600_NUM_ES_GPRS((dev_priv->r600_max_gprs * 7)/64)));

	sq_thread_resource_mgmt = (R600_NUM_PS_THREADS((dev_priv->r600_max_threads * 4)/8) |
				   R600_NUM_VS_THREADS((dev_priv->r600_max_threads * 2)/8) |
				   R600_NUM_ES_THREADS((dev_priv->r600_max_threads * 1)/8));
	if (((dev_priv->r600_max_threads * 1) / 8) > dev_priv->r600_max_gs_threads)
		sq_thread_resource_mgmt |= R600_NUM_GS_THREADS(dev_priv->r600_max_gs_threads);
	else
		sq_thread_resource_mgmt |= R600_NUM_GS_THREADS((dev_priv->r600_max_gs_threads * 1)/8);
	RADEON_WRITE(R600_SQ_THREAD_RESOURCE_MGMT, sq_thread_resource_mgmt);

        RADEON_WRITE(R600_SQ_STACK_RESOURCE_MGMT_1, (R600_NUM_PS_STACK_ENTRIES((dev_priv->r600_max_stack_entries * 1)/4) |
						     R600_NUM_VS_STACK_ENTRIES((dev_priv->r600_max_stack_entries * 1)/4)));

        RADEON_WRITE(R600_SQ_STACK_RESOURCE_MGMT_2, (R600_NUM_GS_STACK_ENTRIES((dev_priv->r600_max_stack_entries * 1)/4) |
						     R600_NUM_ES_STACK_ENTRIES((dev_priv->r600_max_stack_entries * 1)/4)));

	sq_dyn_gpr_size_simd_ab_0 = (R700_SIMDA_RING0((dev_priv->r600_max_gprs * 38)/64) |
				     R700_SIMDA_RING1((dev_priv->r600_max_gprs * 38)/64) |
				     R700_SIMDB_RING0((dev_priv->r600_max_gprs * 38)/64) |
				     R700_SIMDB_RING1((dev_priv->r600_max_gprs * 38)/64));

        RADEON_WRITE(R700_SQ_DYN_GPR_SIZE_SIMD_AB_0, sq_dyn_gpr_size_simd_ab_0);
        RADEON_WRITE(R700_SQ_DYN_GPR_SIZE_SIMD_AB_1, sq_dyn_gpr_size_simd_ab_0);
        RADEON_WRITE(R700_SQ_DYN_GPR_SIZE_SIMD_AB_2, sq_dyn_gpr_size_simd_ab_0);
        RADEON_WRITE(R700_SQ_DYN_GPR_SIZE_SIMD_AB_3, sq_dyn_gpr_size_simd_ab_0);
        RADEON_WRITE(R700_SQ_DYN_GPR_SIZE_SIMD_AB_4, sq_dyn_gpr_size_simd_ab_0);
        RADEON_WRITE(R700_SQ_DYN_GPR_SIZE_SIMD_AB_5, sq_dyn_gpr_size_simd_ab_0);
        RADEON_WRITE(R700_SQ_DYN_GPR_SIZE_SIMD_AB_6, sq_dyn_gpr_size_simd_ab_0);
        RADEON_WRITE(R700_SQ_DYN_GPR_SIZE_SIMD_AB_7, sq_dyn_gpr_size_simd_ab_0);

	RADEON_WRITE(R700_PA_SC_FORCE_EOV_MAX_CNTS, (R700_FORCE_EOV_MAX_CLK_CNT(4095) |
						     R700_FORCE_EOV_MAX_REZ_CNT(255)));

	if ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV710)
		RADEON_WRITE(R600_VGT_CACHE_INVALIDATION, (R600_CACHE_INVALIDATION(R600_TC_ONLY) |
							   R700_AUTO_INVLD_EN(R700_ES_AND_GS_AUTO)));
	else
		RADEON_WRITE(R600_VGT_CACHE_INVALIDATION, (R600_CACHE_INVALIDATION(R600_VC_AND_TC) |
							   R700_AUTO_INVLD_EN(R700_ES_AND_GS_AUTO)));

	switch (dev_priv->flags & RADEON_FAMILY_MASK) {
	case CHIP_RV770:
	case CHIP_RV730:
		gs_prim_buffer_depth = 384;
		break;
	case CHIP_RV710:
		gs_prim_buffer_depth = 128;
		break;
        default:
		break;
        }

        num_gs_verts_per_thread = dev_priv->r600_max_pipes * 16;
        vgt_gs_per_es = gs_prim_buffer_depth + num_gs_verts_per_thread;
        /* Max value for this is 256 */
        if (vgt_gs_per_es > 256)
		vgt_gs_per_es = 256;

        RADEON_WRITE(R600_VGT_ES_PER_GS, 128);
        RADEON_WRITE(R600_VGT_GS_PER_ES, vgt_gs_per_es);
        RADEON_WRITE(R600_VGT_GS_PER_VS, 2);

	/* more default values. 2D/3D driver should adjust as needed */
	RADEON_WRITE(R600_VGT_GS_VERTEX_REUSE, 16);
	RADEON_WRITE(R600_PA_SC_LINE_STIPPLE_STATE, 0);
	RADEON_WRITE(R600_VGT_STRMOUT_EN, 0);
	RADEON_WRITE(R600_SX_MISC, 0);
        RADEON_WRITE(R600_PA_SC_MODE_CNTL, 0);
        RADEON_WRITE(R700_PA_SC_EDGERULE, 0xaaaaaaaa);
        RADEON_WRITE(R600_PA_SC_AA_CONFIG, 0);
        RADEON_WRITE(R600_PA_SC_CLIPRECT_RULE, 0xffff);
        RADEON_WRITE(R600_PA_SC_LINE_STIPPLE, 0);
        RADEON_WRITE(R600_SPI_INPUT_Z, 0);
        RADEON_WRITE(R600_SPI_PS_IN_CONTROL_0, R600_NUM_INTERP(2));
        RADEON_WRITE(R600_CB_COLOR7_FRAG, 0);

	/* clear render buffer base addresses */
	RADEON_WRITE(R600_CB_COLOR0_BASE, 0);
	RADEON_WRITE(R600_CB_COLOR1_BASE, 0);
	RADEON_WRITE(R600_CB_COLOR2_BASE, 0);
	RADEON_WRITE(R600_CB_COLOR3_BASE, 0);
	RADEON_WRITE(R600_CB_COLOR4_BASE, 0);
	RADEON_WRITE(R600_CB_COLOR5_BASE, 0);
	RADEON_WRITE(R600_CB_COLOR6_BASE, 0);
	RADEON_WRITE(R600_CB_COLOR7_BASE, 0);

        RADEON_WRITE(R700_TCP_CNTL, 0);

	hdp_host_path_cntl = RADEON_READ(R600_HDP_HOST_PATH_CNTL);
	RADEON_WRITE(R600_HDP_HOST_PATH_CNTL, hdp_host_path_cntl);

	RADEON_WRITE(R600_PA_SC_MULTI_CHIP_CNTL, 0);

	RADEON_WRITE(R600_PA_CL_ENHANCE, (R600_CLIP_VTX_REORDER_ENA |
					  R600_NUM_CLIP_SEQ(3)));

}

static void r600_cp_init_ring_buffer(struct drm_device * dev,
			      drm_radeon_private_t * dev_priv)
{
	u32 ring_start;
	/*u32 cur_read_ptr;*/
	u32 tmp;

	if (((dev_priv->flags & RADEON_FAMILY_MASK) >= CHIP_RV770))
		r700_gfx_init(dev, dev_priv);
	else
		r600_gfx_init(dev, dev_priv);

	RADEON_WRITE(R600_GRBM_SOFT_RESET, R600_SOFT_RESET_CP);
	RADEON_READ(R600_GRBM_SOFT_RESET);
	DRM_UDELAY(15000);
	RADEON_WRITE(R600_GRBM_SOFT_RESET, 0);


	/* Set ring buffer size */
#ifdef __BIG_ENDIAN
	RADEON_WRITE(R600_CP_RB_CNTL,
		     RADEON_BUF_SWAP_32BIT |
		     RADEON_RB_NO_UPDATE |
		     (dev_priv->ring.rptr_update_l2qw << 8) |
		     dev_priv->ring.size_l2qw);
#else
	RADEON_WRITE(R600_CP_RB_CNTL,
		     RADEON_RB_NO_UPDATE |
		     (dev_priv->ring.rptr_update_l2qw << 8) |
		     dev_priv->ring.size_l2qw);
#endif

	RADEON_WRITE(R600_CP_SEM_WAIT_TIMER, 0x4);

	/* Set the write pointer delay */
	RADEON_WRITE(R600_CP_RB_WPTR_DELAY, 0);

#ifdef __BIG_ENDIAN
	RADEON_WRITE(R600_CP_RB_CNTL,
		     RADEON_BUF_SWAP_32BIT |
		     RADEON_RB_NO_UPDATE |
		     RADEON_RB_RPTR_WR_ENA |
		     (dev_priv->ring.rptr_update_l2qw << 8) |
		     dev_priv->ring.size_l2qw);
#else
	RADEON_WRITE(R600_CP_RB_CNTL,
		     RADEON_RB_NO_UPDATE |
		     RADEON_RB_RPTR_WR_ENA |
		     (dev_priv->ring.rptr_update_l2qw << 8) |
		     dev_priv->ring.size_l2qw);
#endif

	/* Initialize the ring buffer's read and write pointers */
#if 0
	cur_read_ptr = RADEON_READ(R600_CP_RB_RPTR);
	RADEON_WRITE(R600_CP_RB_WPTR, cur_read_ptr);
	SET_RING_HEAD(dev_priv, cur_read_ptr);
	dev_priv->ring.tail = cur_read_ptr;

#endif

	RADEON_WRITE(R600_CP_RB_RPTR_WR, 0);
	RADEON_WRITE(R600_CP_RB_WPTR, 0);
	SET_RING_HEAD(dev_priv, 0);
	dev_priv->ring.tail = 0;

#if __OS_HAS_AGP
	if (dev_priv->flags & RADEON_IS_AGP) {
		// XXX
		RADEON_WRITE(R600_CP_RB_RPTR_ADDR,
			     (dev_priv->ring_rptr->offset
			     - dev->agp->base + dev_priv->gart_vm_start) >> 8);
		RADEON_WRITE(R600_CP_RB_RPTR_ADDR_HI, 0);
	} else
#endif
	{
		struct drm_sg_mem *entry = dev->sg;
		unsigned long tmp_ofs, page_ofs;

		tmp_ofs = dev_priv->ring_rptr->offset -
			  (unsigned long)dev->sg->virtual;
		page_ofs = tmp_ofs >> PAGE_SHIFT;

		RADEON_WRITE(R600_CP_RB_RPTR_ADDR, entry->busaddr[page_ofs] >> 8);
		RADEON_WRITE(R600_CP_RB_RPTR_ADDR_HI, 0);
		DRM_DEBUG("ring rptr: offset=0x%08lx handle=0x%08lx\n",
			  (unsigned long)entry->busaddr[page_ofs],
			  entry->handle + tmp_ofs);
	}

#ifdef __BIG_ENDIAN
	RADEON_WRITE(R600_CP_RB_CNTL,
		     RADEON_BUF_SWAP_32BIT |
		     (dev_priv->ring.rptr_update_l2qw << 8) |
		     dev_priv->ring.size_l2qw);
#else
	RADEON_WRITE(R600_CP_RB_CNTL,
		     (dev_priv->ring.rptr_update_l2qw << 8) |
		     dev_priv->ring.size_l2qw);
#endif

#if __OS_HAS_AGP
	if (dev_priv->flags & RADEON_IS_AGP) {
		// XXX
		radeon_write_agp_base(dev_priv, dev->agp->base);

		// XXX
		radeon_write_agp_location(dev_priv,
			     (((dev_priv->gart_vm_start - 1 +
				dev_priv->gart_size) & 0xffff0000) |
			      (dev_priv->gart_vm_start >> 16)));

		ring_start = (dev_priv->cp_ring->offset
			      - dev->agp->base
			      + dev_priv->gart_vm_start);
	} else
#endif
		ring_start = (dev_priv->cp_ring->offset
			      - (unsigned long)dev->sg->virtual
			      + dev_priv->gart_vm_start);

	RADEON_WRITE(R600_CP_RB_BASE, ring_start >> 8);

	RADEON_WRITE(R600_CP_ME_CNTL, 0xff);

	RADEON_WRITE(R600_CP_DEBUG, (1 << 27) | (1 << 28));

	/* Start with assuming that writeback doesn't work */
	dev_priv->writeback_works = 0;

	/* Initialize the scratch register pointer.  This will cause
	 * the scratch register values to be written out to memory
	 * whenever they are updated.
	 *
	 * We simply put this behind the ring read pointer, this works
	 * with PCI GART as well as (whatever kind of) AGP GART
	 */
	RADEON_WRITE(R600_SCRATCH_ADDR, ((RADEON_READ(R600_CP_RB_RPTR_ADDR) << 8)
					 + R600_SCRATCH_REG_OFFSET) >> 8);

	dev_priv->scratch = ((__volatile__ u32 *)
			     dev_priv->ring_rptr->handle +
			     (R600_SCRATCH_REG_OFFSET / sizeof(u32)));

	RADEON_WRITE(R600_SCRATCH_UMSK, 0x7);

	/* Turn on bus mastering */
	tmp = RADEON_READ(R600_BUS_CNTL) & ~R600_BUS_MASTER_DIS;
	RADEON_WRITE(R600_BUS_CNTL, tmp);

	dev_priv->sarea_priv->last_frame = dev_priv->scratch[0] = 0;
	RADEON_WRITE(R600_LAST_FRAME_REG, dev_priv->sarea_priv->last_frame);

	dev_priv->sarea_priv->last_dispatch = dev_priv->scratch[1] = 0;
	RADEON_WRITE(R600_LAST_DISPATCH_REG,
		     dev_priv->sarea_priv->last_dispatch);

	dev_priv->sarea_priv->last_clear = dev_priv->scratch[2] = 0;
	RADEON_WRITE(R600_LAST_CLEAR_REG, dev_priv->sarea_priv->last_clear);

	r600_do_wait_for_idle(dev_priv);

}

static int r600_do_cleanup_cp(struct drm_device * dev)
{
	drm_radeon_private_t *dev_priv = dev->dev_private;
	DRM_DEBUG("\n");

	/* Make sure interrupts are disabled here because the uninstall ioctl
	 * may not have been called from userspace and after dev_private
	 * is freed, it's too late.
	 */
	if (dev->irq_enabled)
		drm_irq_uninstall(dev);

#if __OS_HAS_AGP
	if (dev_priv->flags & RADEON_IS_AGP) {
		if (dev_priv->cp_ring != NULL) {
			drm_core_ioremapfree(dev_priv->cp_ring, dev);
			dev_priv->cp_ring = NULL;
		}
		if (dev_priv->ring_rptr != NULL) {
			drm_core_ioremapfree(dev_priv->ring_rptr, dev);
			dev_priv->ring_rptr = NULL;
		}
		if (dev->agp_buffer_map != NULL) {
			drm_core_ioremapfree(dev->agp_buffer_map, dev);
			dev->agp_buffer_map = NULL;
		}
	} else
#endif
	{

		if (dev_priv->gart_info.bus_addr)
			r600_page_table_cleanup(dev, &dev_priv->gart_info);

		if (dev_priv->gart_info.gart_table_location == DRM_ATI_GART_FB)
		{
			drm_core_ioremapfree(&dev_priv->gart_info.mapping, dev);
			dev_priv->gart_info.addr = 0;
		}
	}
	/* only clear to the start of flags */
	memset(dev_priv, 0, offsetof(drm_radeon_private_t, flags));

	return 0;
}

int r600_do_init_cp(struct drm_device * dev, drm_radeon_init_t * init)
{
	drm_radeon_private_t *dev_priv = dev->dev_private;

	DRM_DEBUG("\n");

	/* if we require new memory map but we don't have it fail */
	if ((dev_priv->flags & RADEON_NEW_MEMMAP) && !dev_priv->new_memmap) {
		DRM_ERROR("Cannot initialise DRM on this card\nThis card requires a new X.org DDX for 3D\n");
		r600_do_cleanup_cp(dev);
		return -EINVAL;
	}

	if (init->is_pci && (dev_priv->flags & RADEON_IS_AGP))
	{
		DRM_DEBUG("Forcing AGP card to PCI mode\n");
		dev_priv->flags &= ~RADEON_IS_AGP;
		/* The writeback test succeeds, but when writeback is enabled,
		 * the ring buffer read ptr update fails after first 128 bytes.
		 */
		radeon_no_wb = 1;
	}
	else if (!(dev_priv->flags & (RADEON_IS_AGP | RADEON_IS_PCI | RADEON_IS_PCIE))
		 && !init->is_pci)
	{
		DRM_DEBUG("Restoring AGP flag\n");
		dev_priv->flags |= RADEON_IS_AGP;
	}

	dev_priv->usec_timeout = init->usec_timeout;
	if (dev_priv->usec_timeout < 1 ||
	    dev_priv->usec_timeout > RADEON_MAX_USEC_TIMEOUT) {
		DRM_DEBUG("TIMEOUT problem!\n");
		r600_do_cleanup_cp(dev);
		return -EINVAL;
	}

	/* Enable vblank on CRTC1 for older X servers
	 */
	dev_priv->vblank_crtc = DRM_RADEON_VBLANK_CRTC1;

	dev_priv->cp_mode = init->cp_mode;

	/* We don't support anything other than bus-mastering ring mode,
	 * but the ring can be in either AGP or PCI space for the ring
	 * read pointer.
	 */
	if ((init->cp_mode != RADEON_CSQ_PRIBM_INDDIS) &&
	    (init->cp_mode != RADEON_CSQ_PRIBM_INDBM)) {
		DRM_DEBUG("BAD cp_mode (%x)!\n", init->cp_mode);
		r600_do_cleanup_cp(dev);
		return -EINVAL;
	}

	switch (init->fb_bpp) {
	case 16:
		dev_priv->color_fmt = RADEON_COLOR_FORMAT_RGB565;
		break;
	case 32:
	default:
		dev_priv->color_fmt = RADEON_COLOR_FORMAT_ARGB8888;
		break;
	}
	dev_priv->front_offset = init->front_offset;
	dev_priv->front_pitch = init->front_pitch;
	dev_priv->back_offset = init->back_offset;
	dev_priv->back_pitch = init->back_pitch;

	dev_priv->ring_offset = init->ring_offset;
	dev_priv->ring_rptr_offset = init->ring_rptr_offset;
	dev_priv->buffers_offset = init->buffers_offset;
	dev_priv->gart_textures_offset = init->gart_textures_offset;

	dev_priv->sarea = drm_getsarea(dev);
	if (!dev_priv->sarea) {
		DRM_ERROR("could not find sarea!\n");
		r600_do_cleanup_cp(dev);
		return -EINVAL;
	}

	dev_priv->cp_ring = drm_core_findmap(dev, init->ring_offset);
	if (!dev_priv->cp_ring) {
		DRM_ERROR("could not find cp ring region!\n");
		r600_do_cleanup_cp(dev);
		return -EINVAL;
	}
	dev_priv->ring_rptr = drm_core_findmap(dev, init->ring_rptr_offset);
	if (!dev_priv->ring_rptr) {
		DRM_ERROR("could not find ring read pointer!\n");
		r600_do_cleanup_cp(dev);
		return -EINVAL;
	}
	dev->agp_buffer_token = init->buffers_offset;
	dev->agp_buffer_map = drm_core_findmap(dev, init->buffers_offset);
	if (!dev->agp_buffer_map) {
		DRM_ERROR("could not find dma buffer region!\n");
		r600_do_cleanup_cp(dev);
		return -EINVAL;
	}

	if (init->gart_textures_offset) {
		dev_priv->gart_textures =
		    drm_core_findmap(dev, init->gart_textures_offset);
		if (!dev_priv->gart_textures) {
			DRM_ERROR("could not find GART texture region!\n");
			r600_do_cleanup_cp(dev);
			return -EINVAL;
		}
	}

	dev_priv->sarea_priv =
	    (drm_radeon_sarea_t *) ((u8 *) dev_priv->sarea->handle +
				    init->sarea_priv_offset);

#if __OS_HAS_AGP
	// XXX
	if (dev_priv->flags & RADEON_IS_AGP) {
		drm_core_ioremap(dev_priv->cp_ring, dev);
		drm_core_ioremap(dev_priv->ring_rptr, dev);
		drm_core_ioremap(dev->agp_buffer_map, dev);
		if (!dev_priv->cp_ring->handle ||
		    !dev_priv->ring_rptr->handle ||
		    !dev->agp_buffer_map->handle) {
			DRM_ERROR("could not find ioremap agp regions!\n");
			r600_do_cleanup_cp(dev);
			return -EINVAL;
		}
	} else
#endif
	{
		dev_priv->cp_ring->handle = (void *)dev_priv->cp_ring->offset;
		dev_priv->ring_rptr->handle =
		    (void *)dev_priv->ring_rptr->offset;
		dev->agp_buffer_map->handle =
		    (void *)dev->agp_buffer_map->offset;

		DRM_DEBUG("dev_priv->cp_ring->handle %p\n",
			  dev_priv->cp_ring->handle);
		DRM_DEBUG("dev_priv->ring_rptr->handle %p\n",
			  dev_priv->ring_rptr->handle);
		DRM_DEBUG("dev->agp_buffer_map->handle %p\n",
			  dev->agp_buffer_map->handle);
	}

	dev_priv->fb_location = (radeon_read_fb_location(dev_priv) & 0xffff) << 24;
	dev_priv->fb_size =
		(((radeon_read_fb_location(dev_priv) & 0xffff0000u) << 8) + 0x1000000)
		- dev_priv->fb_location;

	dev_priv->front_pitch_offset = (((dev_priv->front_pitch / 64) << 22) |
					((dev_priv->front_offset
					  + dev_priv->fb_location) >> 10));

	dev_priv->back_pitch_offset = (((dev_priv->back_pitch / 64) << 22) |
				       ((dev_priv->back_offset
					 + dev_priv->fb_location) >> 10));

	dev_priv->depth_pitch_offset = (((dev_priv->depth_pitch / 64) << 22) |
					((dev_priv->depth_offset
					  + dev_priv->fb_location) >> 10));

	dev_priv->gart_size = init->gart_size;

	/* New let's set the memory map ... */
	if (dev_priv->new_memmap) {
		u32 base = 0;

		DRM_INFO("Setting GART location based on new memory map\n");

		/* If using AGP, try to locate the AGP aperture at the same
		 * location in the card and on the bus, though we have to
		 * align it down.
		 */
#if __OS_HAS_AGP
		// XXX
		if (dev_priv->flags & RADEON_IS_AGP) {
			base = dev->agp->base;
			/* Check if valid */
			if ((base + dev_priv->gart_size - 1) >= dev_priv->fb_location &&
			    base < (dev_priv->fb_location + dev_priv->fb_size - 1)) {
				DRM_INFO("Can't use AGP base @0x%08lx, won't fit\n",
					 dev->agp->base);
				base = 0;
			}
		}
#endif
		/* If not or if AGP is at 0 (Macs), try to put it elsewhere */
		if (base == 0) {
			base = dev_priv->fb_location + dev_priv->fb_size;
			if (base < dev_priv->fb_location ||
			    ((base + dev_priv->gart_size) & 0xfffffffful) < base)
				base = dev_priv->fb_location
					- dev_priv->gart_size;
		}
		dev_priv->gart_vm_start = base & 0xffc00000u;
		if (dev_priv->gart_vm_start != base)
			DRM_INFO("GART aligned down from 0x%08x to 0x%08x\n",
				 base, dev_priv->gart_vm_start);
	}

#if __OS_HAS_AGP
	// XXX
	if (dev_priv->flags & RADEON_IS_AGP)
		dev_priv->gart_buffers_offset = (dev->agp_buffer_map->offset
						 - dev->agp->base
						 + dev_priv->gart_vm_start);
	else
#endif
		dev_priv->gart_buffers_offset = (dev->agp_buffer_map->offset
						 - (unsigned long)dev->sg->virtual
						 + dev_priv->gart_vm_start);

	DRM_DEBUG("fb 0x%08x size %d\n",
	          (unsigned int) dev_priv->fb_location,
	          (unsigned int) dev_priv->fb_size);
	DRM_DEBUG("dev_priv->gart_size %d\n", dev_priv->gart_size);
	DRM_DEBUG("dev_priv->gart_vm_start 0x%08x\n",
	          (unsigned int) dev_priv->gart_vm_start);
	DRM_DEBUG("dev_priv->gart_buffers_offset 0x%08lx\n",
	          dev_priv->gart_buffers_offset);

	dev_priv->ring.start = (u32 *) dev_priv->cp_ring->handle;
	dev_priv->ring.end = ((u32 *) dev_priv->cp_ring->handle
			      + init->ring_size / sizeof(u32));
	dev_priv->ring.size = init->ring_size;
	dev_priv->ring.size_l2qw = drm_order(init->ring_size / 8);

	dev_priv->ring.rptr_update = /* init->rptr_update */ 4096;
	dev_priv->ring.rptr_update_l2qw = drm_order( /* init->rptr_update */ 4096 / 8);

	dev_priv->ring.fetch_size = /* init->fetch_size */ 32;
	dev_priv->ring.fetch_size_l2ow = drm_order( /* init->fetch_size */ 32 / 16);

	dev_priv->ring.tail_mask = (dev_priv->ring.size / sizeof(u32)) - 1;

	dev_priv->ring.high_mark = RADEON_RING_HIGH_MARK;

#if __OS_HAS_AGP
	if (dev_priv->flags & RADEON_IS_AGP) {
		// XXX turn off pcie gart
	} else
#endif
	{
		dev_priv->gart_info.table_mask = DMA_BIT_MASK(32);
		/* if we have an offset set from userspace */
		if (!dev_priv->pcigart_offset_set) {
			DRM_ERROR("Need gart offset from userspace\n");
			r600_do_cleanup_cp(dev);
			return -EINVAL;
		}

		DRM_DEBUG("Using gart offset 0x%08lx\n", dev_priv->pcigart_offset);

		dev_priv->gart_info.bus_addr =
			dev_priv->pcigart_offset + dev_priv->fb_location;
		dev_priv->gart_info.mapping.offset =
			dev_priv->pcigart_offset + dev_priv->fb_aper_offset;
		dev_priv->gart_info.mapping.size =
			dev_priv->gart_info.table_size;

		drm_core_ioremap_wc(&dev_priv->gart_info.mapping, dev);
		if (!dev_priv->gart_info.mapping.handle) {
			DRM_ERROR("ioremap failed.\n");
			r600_do_cleanup_cp(dev);
			return -EINVAL;
		}

		dev_priv->gart_info.addr =
			dev_priv->gart_info.mapping.handle;

		DRM_DEBUG("Setting phys_pci_gart to %p %08lX\n",
			  dev_priv->gart_info.addr,
			  dev_priv->pcigart_offset);

		r600_page_table_init(dev);

		if (((dev_priv->flags & RADEON_FAMILY_MASK) >= CHIP_RV770))
			r700_vm_init(dev);
		else
			r600_vm_init(dev);
	}

	if (((dev_priv->flags & RADEON_FAMILY_MASK) >= CHIP_RV770))
	    r700_cp_load_microcode(dev_priv);
	else
	    r600_cp_load_microcode(dev_priv);

	r600_cp_init_ring_buffer(dev, dev_priv);

	dev_priv->last_buf = 0;

	r600_engine_reset(dev);
	r600_test_writeback(dev_priv);

	return 0;
}

int r600_do_resume_cp(struct drm_device * dev)
{
	drm_radeon_private_t *dev_priv = dev->dev_private;

	DRM_DEBUG("\n");
	if (((dev_priv->flags & RADEON_FAMILY_MASK) >= CHIP_RV770))
	    r700_cp_load_microcode(dev_priv);
	else
	    r600_cp_load_microcode(dev_priv);
	r600_cp_init_ring_buffer(dev, dev_priv);
	r600_engine_reset(dev);

	return 0;
}

#if 0 /* Currently unused, avoid warnings */
/* Wait for the CP to go idle.
 */
static int r600_do_cp_idle(drm_radeon_private_t * dev_priv)
{
	RING_LOCALS;
	DRM_DEBUG("\n");

	return 0;
}
#endif

/* Start the Command Processor.
 */
void r600_do_cp_start(drm_radeon_private_t * dev_priv)
{
	u32 cp_me;
	RING_LOCALS;
	DRM_DEBUG("\n");

	BEGIN_RING(8);
	OUT_RING(CP_PACKET3(R600_IT_ME_INITIALIZE, 5));
	OUT_RING(0x00000001);
	if (((dev_priv->flags & RADEON_FAMILY_MASK) < CHIP_RV770))
		OUT_RING(0x00000003);
        else
		OUT_RING(0x00000000);
        OUT_RING((dev_priv->r600_max_hw_contexts - 1));
	OUT_RING(R600_ME_INITIALIZE_DEVICE_ID(1));
	OUT_RING(0x00000000);
	OUT_RING(0x00000000);
	OUT_RING(CP_PACKET2());
	ADVANCE_RING();
        COMMIT_RING();

	/* set the mux and reset the halt bit */
	cp_me = 0xff;
	RADEON_WRITE(R600_CP_ME_CNTL, cp_me);

	dev_priv->cp_running = 1;

}

void r600_do_cp_reset(drm_radeon_private_t * dev_priv)
{
	u32 cur_read_ptr;
	DRM_DEBUG("\n");

	cur_read_ptr = RADEON_READ(R600_CP_RB_RPTR);
	RADEON_WRITE(R600_CP_RB_WPTR, cur_read_ptr);
	SET_RING_HEAD(dev_priv, cur_read_ptr);
	dev_priv->ring.tail = cur_read_ptr;
}

void r600_do_cp_stop(drm_radeon_private_t * dev_priv)
{
	uint32_t cp_me;

	DRM_DEBUG("\n");

	cp_me = 0xff | R600_CP_ME_HALT;

	RADEON_WRITE(R600_CP_ME_CNTL, cp_me);

	dev_priv->cp_running = 0;
}



static void r600_cp_discard_buffer(struct drm_device * dev, struct drm_buf * buf)
{
	drm_radeon_private_t *dev_priv = dev->dev_private;
	drm_radeon_buf_priv_t *buf_priv = buf->dev_private;
	RING_LOCALS;

	buf_priv->age = ++dev_priv->sarea_priv->last_dispatch;

	/* Emit the vertex buffer age */
	BEGIN_RING(8);
	R600_DISPATCH_AGE(buf_priv->age);
	OUT_RING(CP_PACKET2());
	OUT_RING(CP_PACKET2());
	OUT_RING(CP_PACKET2());
	OUT_RING(CP_PACKET2());
	OUT_RING(CP_PACKET2());
	OUT_RING(CP_PACKET2());
	ADVANCE_RING();

	buf->pending = 1;
	buf->used = 0;
}

int r600_cp_indirect(struct drm_device *dev, struct drm_buf *buf, drm_radeon_indirect_t *indirect)
{
	drm_radeon_private_t *dev_priv = dev->dev_private;
	int start = indirect->start, end = indirect->end;
	RING_LOCALS;

	RING_SPACE_TEST_WITH_RETURN(dev_priv);
	VB_AGE_TEST_WITH_RETURN(dev_priv);

	buf->used = indirect->end;

	if (indirect->start != indirect->end) {
		unsigned long offset = (dev_priv->gart_buffers_offset
					+ buf->offset + start);
		int dwords = (end - start + 3) / sizeof(u32);

		DRM_DEBUG("dwords:%d\n", dwords);
		DRM_DEBUG("offset 0x%lx\n", offset);


		/* Indirect buffer data must be modulo 8.
		 * pad the data with a Type-2 CP packet.
		 */
		while (dwords & 7) {
			u32 *data = (u32 *)
			    ((char *)dev->agp_buffer_map->handle
			     + buf->offset + start);
			data[dwords++] = RADEON_CP_PACKET2;
     		}

		/* Fire off the indirect buffer */
		BEGIN_RING(8);
		OUT_RING(CP_PACKET3(R600_IT_INDIRECT_BUFFER, 2));
		OUT_RING((offset & 0xfffffffc));
		OUT_RING((upper_32_bits(offset) & 0xff));
		OUT_RING(dwords);
		OUT_RING(CP_PACKET2());
		OUT_RING(CP_PACKET2());
		OUT_RING(CP_PACKET2());
		OUT_RING(CP_PACKET2());
		ADVANCE_RING();
	}
	if (indirect->discard)
		r600_cp_discard_buffer(dev, buf);

	COMMIT_RING();
	return 0;
}