/**************************************************************************
 *
 * Copyright 2014 Advanced Micro Devices, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "util/macros.h"
#include "util/u_qsort.h"
#include "util/vl_rbsp.h"
#include "vl/vl_zscan.h"
#include "va_private.h"

struct ref_cmp {
   bool asc;
   int32_t *poc;
};

static int ref_cmp(const uint8_t *a, const uint8_t *b, struct ref_cmp *ctx)
{
   int32_t pa = ctx->poc[*a];
   int32_t pb = ctx->poc[*b];
   if (pa == pb)
      return 0;
   if (pa > pb)
      return ctx->asc ? 1 : -1;
   return ctx->asc ? -1 : 1;
}

static void vlVaSortRefPicSet(vlVaContext *context, uint8_t *refs, uint8_t num_refs, bool asc)
{
   struct ref_cmp ctx = {
      .asc = asc,
      .poc = context->desc.h265.PicOrderCntVal,
   };
   util_qsort_r(refs, num_refs, sizeof(uint8_t), (void*)ref_cmp, &ctx);
}

void vlVaHandlePictureParameterBufferHEVC(vlVaDriver *drv, vlVaContext *context, vlVaBuffer *buf)
{
   VAPictureParameterBufferHEVC *hevc = buf->data;
   unsigned i;

   assert(buf->size >= sizeof(VAPictureParameterBufferHEVC) && buf->num_elements == 1);
   context->desc.h265.pps->sps->chroma_format_idc = hevc->pic_fields.bits.chroma_format_idc;
   context->desc.h265.pps->sps->separate_colour_plane_flag =
      hevc->pic_fields.bits.separate_colour_plane_flag;
   context->desc.h265.pps->sps->no_pic_reordering_flag =
      hevc->pic_fields.bits.NoPicReorderingFlag;
   context->desc.h265.pps->sps->no_bi_pred_flag =
      hevc->pic_fields.bits.NoBiPredFlag;
   context->desc.h265.pps->sps->pic_width_in_luma_samples = hevc->pic_width_in_luma_samples;
   context->desc.h265.pps->sps->pic_height_in_luma_samples = hevc->pic_height_in_luma_samples;
   context->desc.h265.pps->sps->bit_depth_luma_minus8 = hevc->bit_depth_luma_minus8;
   context->desc.h265.pps->sps->bit_depth_chroma_minus8 = hevc->bit_depth_chroma_minus8;
   context->desc.h265.pps->sps->log2_max_pic_order_cnt_lsb_minus4 =
      hevc->log2_max_pic_order_cnt_lsb_minus4;
   context->desc.h265.pps->sps->sps_max_dec_pic_buffering_minus1 =
      hevc->sps_max_dec_pic_buffering_minus1;
   context->desc.h265.pps->sps->log2_min_luma_coding_block_size_minus3 =
      hevc->log2_min_luma_coding_block_size_minus3;
   context->desc.h265.pps->sps->log2_diff_max_min_luma_coding_block_size =
      hevc->log2_diff_max_min_luma_coding_block_size;
   context->desc.h265.pps->sps->log2_min_transform_block_size_minus2 =
      hevc->log2_min_transform_block_size_minus2;
   context->desc.h265.pps->sps->log2_diff_max_min_transform_block_size =
      hevc->log2_diff_max_min_transform_block_size;
   context->desc.h265.pps->sps->max_transform_hierarchy_depth_inter =
      hevc->max_transform_hierarchy_depth_inter;
   context->desc.h265.pps->sps->max_transform_hierarchy_depth_intra =
      hevc->max_transform_hierarchy_depth_intra;
   context->desc.h265.pps->sps->scaling_list_enabled_flag =
      hevc->pic_fields.bits.scaling_list_enabled_flag;
   context->desc.h265.pps->sps->amp_enabled_flag = hevc->pic_fields.bits.amp_enabled_flag;
   context->desc.h265.pps->sps->sample_adaptive_offset_enabled_flag =
      hevc->slice_parsing_fields.bits.sample_adaptive_offset_enabled_flag;
   context->desc.h265.pps->sps->pcm_enabled_flag = hevc->pic_fields.bits.pcm_enabled_flag;
   if (hevc->pic_fields.bits.pcm_enabled_flag == 1) {
      context->desc.h265.pps->sps->pcm_sample_bit_depth_luma_minus1 =
         hevc->pcm_sample_bit_depth_luma_minus1;
      context->desc.h265.pps->sps->pcm_sample_bit_depth_chroma_minus1 =
         hevc->pcm_sample_bit_depth_chroma_minus1;
      context->desc.h265.pps->sps->log2_min_pcm_luma_coding_block_size_minus3 =
         hevc->log2_min_pcm_luma_coding_block_size_minus3;
      context->desc.h265.pps->sps->log2_diff_max_min_pcm_luma_coding_block_size =
         hevc->log2_diff_max_min_pcm_luma_coding_block_size;
      context->desc.h265.pps->sps->pcm_loop_filter_disabled_flag =
         hevc->pic_fields.bits.pcm_loop_filter_disabled_flag;
   }
   context->desc.h265.pps->sps->num_short_term_ref_pic_sets = hevc->num_short_term_ref_pic_sets;
   context->desc.h265.pps->sps->long_term_ref_pics_present_flag =
      hevc->slice_parsing_fields.bits.long_term_ref_pics_present_flag;
   context->desc.h265.pps->sps->num_long_term_ref_pics_sps = hevc->num_long_term_ref_pic_sps;
   context->desc.h265.pps->sps->sps_temporal_mvp_enabled_flag =
      hevc->slice_parsing_fields.bits.sps_temporal_mvp_enabled_flag;
   context->desc.h265.pps->sps->strong_intra_smoothing_enabled_flag =
      hevc->pic_fields.bits.strong_intra_smoothing_enabled_flag;

   context->desc.h265.pps->dependent_slice_segments_enabled_flag =
      hevc->slice_parsing_fields.bits.dependent_slice_segments_enabled_flag;
   context->desc.h265.pps->output_flag_present_flag =
      hevc->slice_parsing_fields.bits.output_flag_present_flag;
   context->desc.h265.pps->num_extra_slice_header_bits = hevc->num_extra_slice_header_bits;
   context->desc.h265.pps->sign_data_hiding_enabled_flag =
      hevc->pic_fields.bits.sign_data_hiding_enabled_flag;
   context->desc.h265.pps->cabac_init_present_flag =
      hevc->slice_parsing_fields.bits.cabac_init_present_flag;
   context->desc.h265.pps->num_ref_idx_l0_default_active_minus1 =
      hevc->num_ref_idx_l0_default_active_minus1;
   context->desc.h265.pps->num_ref_idx_l1_default_active_minus1 =
      hevc->num_ref_idx_l1_default_active_minus1;
   context->desc.h265.pps->init_qp_minus26 = hevc->init_qp_minus26;
   context->desc.h265.pps->constrained_intra_pred_flag =
      hevc->pic_fields.bits.constrained_intra_pred_flag;
   context->desc.h265.pps->transform_skip_enabled_flag =
      hevc->pic_fields.bits.transform_skip_enabled_flag;
   context->desc.h265.pps->cu_qp_delta_enabled_flag =
      hevc->pic_fields.bits.cu_qp_delta_enabled_flag;
   context->desc.h265.pps->diff_cu_qp_delta_depth = hevc->diff_cu_qp_delta_depth;
   context->desc.h265.pps->pps_cb_qp_offset = hevc->pps_cb_qp_offset;
   context->desc.h265.pps->pps_cr_qp_offset = hevc->pps_cr_qp_offset;
   context->desc.h265.pps->pps_slice_chroma_qp_offsets_present_flag =
      hevc->slice_parsing_fields.bits.pps_slice_chroma_qp_offsets_present_flag;
   context->desc.h265.pps->weighted_pred_flag = hevc->pic_fields.bits.weighted_pred_flag;
   context->desc.h265.pps->weighted_bipred_flag = hevc->pic_fields.bits.weighted_bipred_flag;
   context->desc.h265.pps->transquant_bypass_enabled_flag =
      hevc->pic_fields.bits.transquant_bypass_enabled_flag;
   context->desc.h265.pps->tiles_enabled_flag = hevc->pic_fields.bits.tiles_enabled_flag;
   context->desc.h265.pps->entropy_coding_sync_enabled_flag =
      hevc->pic_fields.bits.entropy_coding_sync_enabled_flag;
   if (hevc->pic_fields.bits.tiles_enabled_flag == 1) {
      context->desc.h265.pps->num_tile_columns_minus1 = hevc->num_tile_columns_minus1;
      context->desc.h265.pps->num_tile_rows_minus1 = hevc->num_tile_rows_minus1;
      for (i = 0 ; i < 19 ; i++)
         context->desc.h265.pps->column_width_minus1[i] = hevc->column_width_minus1[i];
      for (i = 0 ; i < 21 ; i++)
         context->desc.h265.pps->row_height_minus1[i] = hevc->row_height_minus1[i];
      context->desc.h265.pps->loop_filter_across_tiles_enabled_flag =
         hevc->pic_fields.bits.loop_filter_across_tiles_enabled_flag;
   }
   context->desc.h265.pps->pps_loop_filter_across_slices_enabled_flag =
      hevc->pic_fields.bits.pps_loop_filter_across_slices_enabled_flag;
   context->desc.h265.pps->deblocking_filter_override_enabled_flag =
      hevc->slice_parsing_fields.bits.deblocking_filter_override_enabled_flag;
   context->desc.h265.pps->pps_deblocking_filter_disabled_flag =
      hevc->slice_parsing_fields.bits.pps_disable_deblocking_filter_flag;
   context->desc.h265.pps->pps_beta_offset_div2 = hevc->pps_beta_offset_div2;
   context->desc.h265.pps->pps_tc_offset_div2 = hevc->pps_tc_offset_div2;
   context->desc.h265.pps->lists_modification_present_flag =
      hevc->slice_parsing_fields.bits.lists_modification_present_flag;
   context->desc.h265.pps->log2_parallel_merge_level_minus2 =
      hevc->log2_parallel_merge_level_minus2;
   context->desc.h265.pps->slice_segment_header_extension_present_flag =
      hevc->slice_parsing_fields.bits.slice_segment_header_extension_present_flag;

   context->desc.h265.IDRPicFlag = hevc->slice_parsing_fields.bits.IdrPicFlag;
   context->desc.h265.RAPPicFlag = hevc->slice_parsing_fields.bits.RapPicFlag;
   context->desc.h265.IntraPicFlag = hevc->slice_parsing_fields.bits.IntraPicFlag;

   context->desc.h265.CurrPicOrderCntVal = hevc->CurrPic.pic_order_cnt;

   for (i = 0 ; i < 8 ; i++) {
      context->desc.h265.RefPicSetStCurrBefore[i] = 0xFF;
      context->desc.h265.RefPicSetStCurrAfter[i] = 0xFF;
      context->desc.h265.RefPicSetLtCurr[i] = 0xFF;
   }
   context->desc.h265.NumPocStCurrBefore = 0;
   context->desc.h265.NumPocStCurrAfter = 0;
   context->desc.h265.NumPocLtCurr = 0;
   unsigned int iBefore = 0;
   unsigned int iAfter = 0;
   unsigned int iCurr = 0;
   for (i = 0 ; i < 15 ; i++) {
      context->desc.h265.PicOrderCntVal[i] = hevc->ReferenceFrames[i].pic_order_cnt;

      vlVaGetReferenceFrame(drv, hevc->ReferenceFrames[i].picture_id, &context->desc.h265.ref[i]);

      if ((hevc->ReferenceFrames[i].flags & VA_PICTURE_HEVC_RPS_ST_CURR_BEFORE) && (iBefore < 8)) {
         context->desc.h265.RefPicSetStCurrBefore[iBefore++] = i;
         context->desc.h265.NumPocStCurrBefore++;
      }
      if ((hevc->ReferenceFrames[i].flags & VA_PICTURE_HEVC_RPS_ST_CURR_AFTER) && (iAfter < 8)) {
         context->desc.h265.RefPicSetStCurrAfter[iAfter++] = i;
         context->desc.h265.NumPocStCurrAfter++;
      }
      if ((hevc->ReferenceFrames[i].flags & VA_PICTURE_HEVC_RPS_LT_CURR) && (iCurr < 8)) {
         context->desc.h265.RefPicSetLtCurr[iCurr++] = i;
         context->desc.h265.NumPocLtCurr++;
      }
      context->desc.h265.IsLongTerm[i] = ((hevc->ReferenceFrames[i].flags & VA_PICTURE_HEVC_LONG_TERM_REFERENCE) != 0) ? 1 : 0;
   }
   vlVaSortRefPicSet(context, context->desc.h265.RefPicSetStCurrBefore, context->desc.h265.NumPocStCurrBefore, false);
   vlVaSortRefPicSet(context, context->desc.h265.RefPicSetStCurrAfter, context->desc.h265.NumPocStCurrAfter, true);
   context->desc.h265.LtCurrDone = context->desc.h265.NumPocLtCurr < 2;

   context->desc.h265.NumShortTermPictureSliceHeaderBits = hevc->st_rps_bits;

   context->desc.h265.slice_parameter.slice_count = 0;
   context->desc.h265.slice_parameter.slice_info_present = false;
   memset(context->desc.h265.slice_parameter.slice_data_flag, 0, sizeof(context->desc.h265.slice_parameter.slice_data_flag));
   memset(context->desc.h265.slice_parameter.slice_data_offset, 0, sizeof(context->desc.h265.slice_parameter.slice_data_offset));
   memset(context->desc.h265.slice_parameter.slice_data_size, 0, sizeof(context->desc.h265.slice_parameter.slice_data_size));
}

void vlVaHandleIQMatrixBufferHEVC(vlVaContext *context, vlVaBuffer *buf)
{
   VAIQMatrixBufferHEVC *h265 = buf->data;
   int i, j;

   assert(buf->size >= sizeof(VAIQMatrixBufferHEVC) && buf->num_elements == 1);

   for (i = 0; i < 6; i++) {
      for (j = 0; j < 16; j++)
         context->desc.h265.pps->sps->ScalingList4x4[i][j] =
                                h265->ScalingList4x4[i][vl_zscan_h265_up_right_diagonal_16[j]];

      for (j = 0; j < 64; j++) {
         context->desc.h265.pps->sps->ScalingList8x8[i][j] =
                                h265->ScalingList8x8[i][vl_zscan_h265_up_right_diagonal[j]];
         context->desc.h265.pps->sps->ScalingList16x16[i][j] =
                                h265->ScalingList16x16[i][vl_zscan_h265_up_right_diagonal[j]];

         if (i < 2)
            context->desc.h265.pps->sps->ScalingList32x32[i][j] =
                                   h265->ScalingList32x32[i][vl_zscan_h265_up_right_diagonal[j]];
      }

      context->desc.h265.pps->sps->ScalingListDCCoeff16x16[i] =
                             h265->ScalingListDC16x16[i];
      if (i < 2)
         context->desc.h265.pps->sps->ScalingListDCCoeff32x32[i] =
                                h265->ScalingListDC32x32[i];
   }
}

void vlVaHandleSliceParameterBufferHEVC(vlVaContext *context, vlVaBuffer *buf)
{
   VASliceParameterBufferHEVC *h265 = buf->data;

   for (uint32_t buffer_idx = 0; buffer_idx < buf->num_elements; buffer_idx++, h265++) {
      uint32_t slice_index = context->desc.h265.slice_parameter.slice_count + buffer_idx;

      const size_t max_pipe_hevc_slices = ARRAY_SIZE(context->desc.h265.slice_parameter.slice_data_offset);
      assert(slice_index < max_pipe_hevc_slices);
      if (slice_index >= max_pipe_hevc_slices) {
         static bool warn_once = true;
         if (warn_once) {
            fprintf(stderr, "Warning: Number of slices (%d) provided exceed driver's max supported (%d), stop handling remaining slices.\n",
               slice_index + 1, (int)max_pipe_hevc_slices);
            warn_once = false;
         }
         return;
      }

      switch(h265->LongSliceFlags.fields.slice_type) {
      /* Depending on slice_type, only update relevant reference */
      case 0: /* HEVC_SLICE_B */
         for (int j = 0 ; j < 15 ; j++)
            context->desc.h265.RefPicList[1][j] = h265->RefPicList[1][j];
         FALLTHROUGH;
      case 1: /* HEVC_SLICE_P */
         for (int j = 0 ; j < 15 ; j++)
            context->desc.h265.RefPicList[0][j] = h265->RefPicList[0][j];
         FALLTHROUGH;
      default:
         break;
      }

      context->desc.h265.slice_parameter.slice_info_present = true;
      context->desc.h265.slice_parameter.slice_data_size[slice_index] = h265->slice_data_size;
      context->desc.h265.slice_parameter.slice_data_offset[slice_index] = h265->slice_data_offset;

      switch (h265->slice_data_flag) {
      case VA_SLICE_DATA_FLAG_ALL:
         context->desc.h265.slice_parameter.slice_data_flag[slice_index] = PIPE_SLICE_BUFFER_PLACEMENT_TYPE_WHOLE;
         break;
      case VA_SLICE_DATA_FLAG_BEGIN:
         context->desc.h265.slice_parameter.slice_data_flag[slice_index] = PIPE_SLICE_BUFFER_PLACEMENT_TYPE_BEGIN;
         break;
      case VA_SLICE_DATA_FLAG_MIDDLE:
         context->desc.h265.slice_parameter.slice_data_flag[slice_index] = PIPE_SLICE_BUFFER_PLACEMENT_TYPE_MIDDLE;
         break;
      case VA_SLICE_DATA_FLAG_END:
         context->desc.h265.slice_parameter.slice_data_flag[slice_index] = PIPE_SLICE_BUFFER_PLACEMENT_TYPE_END;
         break;
      default:
         break;
      }
   }
   context->desc.h265.slice_parameter.slice_count += buf->num_elements;
}

void vlVaDecoderHEVCBitstreamHeader(vlVaContext *context, vlVaBuffer *buf)
{
   struct pipe_h265_picture_desc *pic = &context->desc.h265;
   struct pipe_h265_pps *pps = pic->pps;
   struct pipe_h265_sps *sps = pic->pps->sps;

   /* Only need to parse slice header if the LtCurr order is unknown */
   if (pic->LtCurrDone)
      return;

   struct vl_vlc vlc;
   vl_vlc_init(&vlc, 1, (const void * const*)&buf->data, &buf->size);
   vl_vlc_eatbits(&vlc, 1);
   unsigned nal_unit_type = vl_vlc_get_uimsbf(&vlc, 6);
   vl_vlc_eatbits(&vlc, 9);

   struct vl_rbsp rbsp;
   vl_rbsp_init(&rbsp, &vlc, ~0, true);

   unsigned first_slice_segment_in_pic_flag = vl_rbsp_u(&rbsp, 1);

   if (nal_unit_type >= PIPE_H265_NAL_BLA_W_LP && nal_unit_type <= PIPE_H265_NAL_RSV_IRAP_VCL23)
      vl_rbsp_u(&rbsp, 1); /* no_output_of_prior_pics_flag */

   vl_rbsp_ue(&rbsp); /* slice_pic_parameter_set_id */

   if (!first_slice_segment_in_pic_flag) {
      if (pps->dependent_slice_segments_enabled_flag &&
          vl_rbsp_u(&rbsp, 1)) /* dependent_slice_segment_flag */
         return;
      unsigned ctb_size = 1 <<
         (sps->log2_min_luma_coding_block_size_minus3 + 3 +
          sps->log2_diff_max_min_luma_coding_block_size);
      unsigned size_in_ctbs =
         DIV_ROUND_UP(pps->sps->pic_width_in_luma_samples, ctb_size) *
         DIV_ROUND_UP(pps->sps->pic_height_in_luma_samples, ctb_size);
      vl_rbsp_u(&rbsp, util_logbase2(size_in_ctbs - 1) + 1); /* slice_segment_address */
   }

   for (uint8_t i = 0; i < pps->num_extra_slice_header_bits; i++)
      vl_rbsp_u(&rbsp, 1);

   unsigned slice_type = vl_rbsp_ue(&rbsp);

   if (pps->output_flag_present_flag)
      vl_rbsp_u(&rbsp, 1); /* pic_output_flag */

   if (sps->separate_colour_plane_flag)
      vl_rbsp_u(&rbsp, 2); /* colour_plane_id */

   struct {
      uint32_t poc;
      bool msb;
   } ltr_tmp[8];
   uint8_t ltr_idx = 0;

   unsigned max_poc = 1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
   unsigned num_long_term_sps = 0;

   if (nal_unit_type != PIPE_H265_NAL_IDR_W_RADL && nal_unit_type != PIPE_H265_NAL_IDR_N_LP) {
      vl_rbsp_u(&rbsp, sps->log2_max_pic_order_cnt_lsb_minus4 + 4); /* slice_pic_order_cnt_lsb */
      if (!vl_rbsp_u(&rbsp, 1)) { /* short_term_ref_pic_set_sps_flag */
         for (unsigned i = 0; i < pic->NumShortTermPictureSliceHeaderBits; i++)
            vl_rbsp_u(&rbsp, 1);
      } else if (sps->num_short_term_ref_pic_sets > 1) {
          vl_rbsp_u(&rbsp, util_logbase2_ceil(sps->num_short_term_ref_pic_sets)); /* short_term_ref_pic_set_idx */
      }
      if (sps->long_term_ref_pics_present_flag) {
         if (sps->num_long_term_ref_pics_sps > 0)
            num_long_term_sps = vl_rbsp_ue(&rbsp);
         unsigned num_long_term_pics = vl_rbsp_ue(&rbsp);
         unsigned prev_delta = 0;
         for (unsigned i = 0; i < (num_long_term_sps + num_long_term_pics); i++) {
            unsigned poc = 0;
            unsigned used_by_curr_pic_lt_flag = 0;
            if (i < num_long_term_sps) {
               if (sps->num_long_term_ref_pics_sps > 1)
                  vl_rbsp_u(&rbsp, util_logbase2_ceil(sps->num_long_term_ref_pics_sps)); /* lt_idx_sps[i] */
            } else {
               poc = vl_rbsp_u(&rbsp, sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
               used_by_curr_pic_lt_flag = vl_rbsp_u(&rbsp, 1);
            }
            unsigned delta_poc_msb_present_flag = vl_rbsp_u(&rbsp, 1);
            if (delta_poc_msb_present_flag) {
               unsigned delta = vl_rbsp_ue(&rbsp);
               if (i && i != num_long_term_sps)
                  delta += prev_delta;
               poc += pic->CurrPicOrderCntVal - delta * max_poc - (pic->CurrPicOrderCntVal % max_poc);
               prev_delta = delta;
            }
            if (used_by_curr_pic_lt_flag && ltr_idx < 8) {
               ltr_tmp[ltr_idx].poc = poc;
               ltr_tmp[ltr_idx++].msb = delta_poc_msb_present_flag;
            }
         }
         if (sps->sps_temporal_mvp_enabled_flag)
            vl_rbsp_u(&rbsp, 1); /* temporal_mvp_enable */
      }
   }

   /* We don't know how many SPS LTRs are used, but we do know how many LTRs
    * from slice header are used, so derive the SPS LTRs this way. */
   if (pic->NumPocLtCurr >= ltr_idx)
      num_long_term_sps = pic->NumPocLtCurr - ltr_idx;
   else
      num_long_term_sps = 0;

   /* We know the POCs for all LTRs from slice header, fill the LTR ref pic
    * set with them. */
   for (uint8_t i = 0; i < ltr_idx; i++) {
      for (uint8_t j = 0; j < sizeof(pic->PicOrderCntVal); ++j) {
         uint32_t refpoc = pic->PicOrderCntVal[j];
         if ((ltr_tmp[i].msb && ltr_tmp[i].poc == refpoc) ||
             (!ltr_tmp[i].msb && ltr_tmp[i].poc == refpoc % max_poc)) {
            pic->RefPicSetLtCurr[i + num_long_term_sps] = j;
            break;
         }
      }
   }

   /* If there are no SPS LTRs, the complete LtCurr set was built above. */
   if (!num_long_term_sps) {
      pic->LtCurrDone = true;
      return;
   }

   if (sps->sample_adaptive_offset_enabled_flag) {
       vl_rbsp_u(&rbsp, 1); /* slice_sao_luma_flag */
       if (sps->chroma_format_idc)
          vl_rbsp_u(&rbsp, 1); /* slice_sao_chroma_flag */
   }

   unsigned ltr_start = pic->NumPocStCurrBefore + pic->NumPocStCurrAfter;
   unsigned ref_pic_list_modification_flag_l0 = 0, ref_pic_list_modification_flag_l1 = 0;

   if (slice_type == PIPE_H265_SLICE_TYPE_P || slice_type == PIPE_H265_SLICE_TYPE_B) {
      unsigned num_ref_idx_active_override_flag = vl_rbsp_u(&rbsp, 1);
      unsigned num_ref_idx_l0_active_minus1 = 0, num_ref_idx_l1_active_minus1 = 0;
      if (num_ref_idx_active_override_flag) {
         num_ref_idx_l0_active_minus1 = vl_rbsp_ue(&rbsp);
         if (slice_type == PIPE_H265_SLICE_TYPE_B)
            num_ref_idx_l1_active_minus1 = vl_rbsp_ue(&rbsp);
      }
      if (pps->lists_modification_present_flag) {
         unsigned num_bits = util_logbase2_ceil(ltr_start + pic->NumPocLtCurr);
         unsigned num_ref_l0_minus1 = num_ref_idx_active_override_flag ?
            num_ref_idx_l0_active_minus1 : pps->num_ref_idx_l0_default_active_minus1;
         ref_pic_list_modification_flag_l0 = vl_rbsp_u(&rbsp, 1);
         if (ref_pic_list_modification_flag_l0) {
            for (unsigned i = 0; i <= num_ref_l0_minus1; i++) {
               unsigned idx = vl_rbsp_u(&rbsp, num_bits);
               if (idx >= ltr_start && idx < ltr_start + num_long_term_sps)
                  pic->RefPicSetLtCurr[idx - ltr_start] = pic->RefPicList[0][i];
            }
         }
         if (slice_type == PIPE_H265_SLICE_TYPE_B) {
            unsigned num_ref_l1_minus1 = num_ref_idx_active_override_flag ?
               num_ref_idx_l1_active_minus1 : pps->num_ref_idx_l1_default_active_minus1;
            ref_pic_list_modification_flag_l1 = vl_rbsp_u(&rbsp, 1);
            if (ref_pic_list_modification_flag_l1) {
               for (unsigned i = 0; i <= num_ref_l1_minus1; i++) {
                  unsigned idx = vl_rbsp_u(&rbsp, num_bits);
                  if (idx >= ltr_start && idx < ltr_start + num_long_term_sps)
                     pic->RefPicSetLtCurr[idx - ltr_start] = pic->RefPicList[1][i];
               }
            }
         }
      }
      /* If no modification is used, we can use the RefPicList directly. */
      if (!ref_pic_list_modification_flag_l0) {
         for (unsigned i = 0; i < pic->NumPocLtCurr; i++) {
            uint8_t idx = pic->RefPicList[0][i + ltr_start];
            if (i < num_long_term_sps) {
               assert(idx != 0xff);
               pic->RefPicSetLtCurr[i] = idx;
            } else {
               assert(idx == pic->RefPicSetLtCurr[i]);
            }
         }
         pic->LtCurrDone = true;
      } else if (slice_type == PIPE_H265_SLICE_TYPE_B && !ref_pic_list_modification_flag_l1) {
         for (unsigned i = 0; i < pic->NumPocLtCurr; i++) {
            uint8_t idx = pic->RefPicList[1][i + ltr_start];
            if (i < num_long_term_sps) {
               assert(idx != 0xff);
               pic->RefPicSetLtCurr[i] = idx;
            } else {
               assert(idx == pic->RefPicSetLtCurr[i]);
            }
         }
         pic->LtCurrDone = true;
      }
   }
}
