// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/layer_tree_settings.h"

#include "components/viz/common/resources/platform_color.h"
#include "third_party/khronos/GLES2/gl2.h"

#if defined(USE_NEVA_APPRUNTIME)
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "cc/base/switches_neva.h"
#endif

namespace cc {

LayerTreeSettings::LayerTreeSettings()
    : default_tile_size(gfx::Size(256, 256)),
      max_untiled_layer_size(gfx::Size(512, 512)),
      minimum_occlusion_tracking_size(gfx::Size(160, 160)),
      memory_policy(64 * 1024 * 1024,
                    gpu::MemoryAllocation::CUTOFF_ALLOW_EVERYTHING,
                    ManagedMemoryPolicy::kDefaultNumResourcesLimit) {
#if defined(USE_NEVA_APPRUNTIME)
  base::CommandLine& cmd_line = *base::CommandLine::ForCurrentProcess();
  if (cmd_line.HasSwitch(cc::switches::kDecodedImageWorkingSetBudgetMB)) {
    size_t budget_bytes_mb;
    if (base::StringToSizeT(cmd_line.GetSwitchValueASCII(
                                cc::switches::kDecodedImageWorkingSetBudgetMB),
                            &budget_bytes_mb))
      decoded_image_working_set_budget_bytes = budget_bytes_mb * 1024 * 1024;
  }
#endif
}

LayerTreeSettings::LayerTreeSettings(const LayerTreeSettings& other) = default;
LayerTreeSettings::~LayerTreeSettings() = default;

SchedulerSettings LayerTreeSettings::ToSchedulerSettings() const {
  SchedulerSettings scheduler_settings;
  scheduler_settings.main_frame_before_activation_enabled =
      main_frame_before_activation_enabled;
  scheduler_settings.timeout_and_draw_when_animation_checkerboards =
      timeout_and_draw_when_animation_checkerboards;
  scheduler_settings.using_synchronous_renderer_compositor =
      using_synchronous_renderer_compositor;
  scheduler_settings.enable_latency_recovery = enable_latency_recovery;
  scheduler_settings.background_frame_interval =
      base::TimeDelta::FromSecondsD(1.0 / background_animation_rate);
  scheduler_settings.wait_for_all_pipeline_stages_before_draw =
      wait_for_all_pipeline_stages_before_draw;
  scheduler_settings.enable_surface_synchronization =
      enable_surface_synchronization;
  return scheduler_settings;
}

TileManagerSettings LayerTreeSettings::ToTileManagerSettings() const {
  TileManagerSettings tile_manager_settings;
  tile_manager_settings.use_partial_raster = use_partial_raster;
  tile_manager_settings.enable_checker_imaging = enable_checker_imaging;
  tile_manager_settings.min_image_bytes_to_checker = min_image_bytes_to_checker;
  return tile_manager_settings;
}

}  // namespace cc
