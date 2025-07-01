/*
 Copyright 2024 Google LLC

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/
#include <iostream>
#include <optional>
#include <vector>
#include <sstream>

#include "dive_core/data_core.h"
#include "pm4_info.h"

// Fix depth modes shouldn't count binning passes
// Fix memset of event_state
// Fix drawindex in command_hierarch

void GatherStats(const Dive::CaptureMetadata &meta_data, std::ostream &ostream)
{
    // Number of draw calls in BINNING, DIRECT, and TILED
    //  With LRZ enabled, and LRZ Write enabled
    //  With depth test enabled
    //  With depth write enabled
    //  With EarlyZ, LateZ enabled
    //  With CullMode enabled
    // Number of CLEARs  (???)
    // List of different unique viewports used
    // Min, Max, Total number of indices rendered
    // Number of tiles + size of tiles + location of tiles
    // Number of unique shaders, per stage (BINNING vs TILING)
    //  Min, Max, and total instruction counts
    //  Min, Max, and total GPR count
    // Number of CPEventWrites
    //  RESOLVE, FLUSH_COLOR, FLUSH_DEPTH, INVALIDATE_COLOR, INVALIDATE_DEPTH
    // Number of CpWaitForIdle()
    // Number of prefetches
    // Number of loads/stores to GMEM
    enum Stats
    {
        kBinningDraws,
        kDirectDraws,
        kTiledDraws,
        kDispatches,
        kResolves,
        kSyncs,
        kEarlyZ,
        kLateZ,
        kEarlyLRZLateZ,
        kDepthTestEnabled,
        kDepthWriteEnabled,
        kLrzEnabled,
        kLrzWriteEnabled,
        kCullModeEnabled,
        kNumStats
    };
    const char *statsDesc[kNumStats] = {
        "Num Draws (BINNING)",
        "Num Draws (DIRECT)",
        "Num Draws (TILED)",
        "Num Dispatches",
        "Num Resolves",
        "Num Syncs",
        "Num Draws with EarlyZ",
        "Num Draws with LateZ",
        "Num Draws with Early LRZ & LateZ",
        "Num Draws with Depth Test Enabled",
        "Num Draws with Depth Write Enabled",
        "Num Draws with LRZ Enabled",
        "Num Draws with LRZ Write Enabled",
        "Num Draws with culling enabled",
    };
#ifndef NDEBUG
    // Ensure all elements of statsDesc are initialized
    // Indirect way to ensure the right number of initializers are provided above
    for (uint32_t i = 0; i < kNumStats; ++i)
    {
        assert(statsDesc[i] != nullptr);
    }
#endif

    uint32_t statsList[kNumStats] = {};

    // This is just to align the strings so that they are easier to read
    // auto AppendSpace = [](std::string &str, uint32_t desired_len) {
    //     if (str.length() < desired_len)
    //     {
    //         str.append(desired_len - str.length(), ' ');
    //     }
    // };

    size_t                      event_count = meta_data.m_event_info.size();
    const Dive::EventStateInfo &event_state = meta_data.m_event_state;

    for (size_t i = 0; i < event_count; ++i)
    {
        const Dive::EventInfo &info = meta_data.m_event_info[i];

        if (info.m_type == Dive::EventInfo::EventType::kDraw)
        {
            if (info.m_render_mode == Dive::RenderModeType::kBinning)
                statsList[kBinningDraws]++;
            else if (info.m_render_mode == Dive::RenderModeType::kDirect)
                statsList[kDirectDraws]++;
            else if (info.m_render_mode == Dive::RenderModeType::kTiled)
                statsList[kTiledDraws]++;
        }
        else if (info.m_type == Dive::EventInfo::EventType::kDispatch)
            statsList[kDispatches]++;
        else if (info.m_type == Dive::EventInfo::EventType::kResolve)
            statsList[kResolves]++;
        else if (info.m_type == Dive::EventInfo::EventType::kSync)
            statsList[kSyncs]++;

        const uint32_t event_id = static_cast<uint32_t>(i);
        auto           event_state_it = event_state.find(static_cast<Dive::EventStateId>(event_id));

        bool lrz_render_modes = (info.m_render_mode == Dive::RenderModeType::kDirect) ||
                                (info.m_render_mode == Dive::RenderModeType::kBinning);

        // Note: Not necessary to call Is*Set() for boolean states, since the internal
        // tracking variable is initialized to false
        if (event_state_it->DepthTestEnabled())
        {
            statsList[kDepthTestEnabled]++;

            TODO: Use macros, one for boolean state and one for non-boolean states
            So that the Is*Set() is called automatically
            if (lrz_render_modes && event_state_it->IsLRZEnabledSet() &&
                event_state_it->LRZEnabled())
                statsList[kLrzEnabled]++;
            if (event_state_it->DepthWriteEnabled())
            {
                statsList[kDepthWriteEnabled]++;
                if (event_state_it->LRZWrite())
                    statsList[kLrzWriteEnabled]++;
            }

            if (event_state_it->IsZTestModeSet())
            {
                a6xx_ztest_mode mode = event_state_it->ZTestMode();
                if (mode == A6XX_EARLY_Z)
                    statsList[kEarlyZ]++;
                else if (mode == A6XX_LATE_Z)
                    statsList[kLateZ]++;
                else if (mode == A6XX_EARLY_LRZ_LATE_Z)
                    statsList[kEarlyLRZLateZ]++;
            }
        }

        if (event_state_it->IsCullModeSet())
        {
            VkCullModeFlags cull_flags = event_state_it->CullMode();
            if ((cull_flags & VK_CULL_MODE_FRONT_BIT) != VK_CULL_MODE_NONE)
                statsList[kCullModeEnabled]++;
        }
        /*
        const uint32_t event_id = static_cast<uint32_t>(i);
        auto event_state_it = event_state.find(static_cast<Dive::EventStateId>(event_id));

        // Direct & binning draw states
        if ((info.m_type == Dive::EventInfo::EventType::kDraw) &&
            ((info.m_render_mode == Dive::RenderModeType::kDirect) ||
             (info.m_render_mode == Dive::RenderModeType::kBinning)))
        {
            const uint32_t event_id = static_cast<uint32_t>(i);
            auto event_state_it = event_state.find(static_cast<Dive::EventStateId>(event_id));

            const uint32_t desired_draw_string_len = 64;
            std::string    draw_string = info.m_str;
            AppendSpace(draw_string, desired_draw_string_len);
            ostream << draw_string + "\t";

            const bool  is_depth_test_enabled = event_state_it->DepthTestEnabled();
            const bool  is_depth_write_enabled = event_state_it->DepthWriteEnabled();
            VkCompareOp zfunc = event_state_it->DepthCompareOp();

            ostream << "DepthTest:" +
                          std::string(is_depth_test_enabled ? "Enabled\t" : "Disabled\t");
            ostream << "DepthWrite:" +
                          std::string(is_depth_write_enabled ? "Enabled\t" : "Disabled\t");

            std::string zfunc_str = "Invalid";
            switch (zfunc)
            {
            case VK_COMPARE_OP_NEVER:
                zfunc_str = "Never";
                break;
            case VK_COMPARE_OP_LESS:
                zfunc_str = "Less";
                break;
            case VK_COMPARE_OP_EQUAL:
                zfunc_str = "Equal";
                break;
            case VK_COMPARE_OP_LESS_OR_EQUAL:
                zfunc_str = "Less or Equal";
                break;
            case VK_COMPARE_OP_GREATER:
                zfunc_str = "Greater";
                break;
            case VK_COMPARE_OP_NOT_EQUAL:
                zfunc_str = "Not Equal";
                break;
            case VK_COMPARE_OP_GREATER_OR_EQUAL:
                zfunc_str = "Greater or Equal";
                break;
            case VK_COMPARE_OP_ALWAYS:
                zfunc_str = "Always";
                break;
            default:
                DIVE_ASSERT(false);
                break;
            }
            const uint32_t desired_zfunc_str_len = 16;
            AppendSpace(zfunc_str, desired_zfunc_str_len);
            ostream << "DepthFunc:" + zfunc_str + "\t";
            if (is_depth_test_enabled)
            {
                const bool lrz_enabled = event_state_it->LRZEnabled();
                if (!lrz_enabled)
                {
                    ostream << "LRZ:Disabled\t";
                    // if depth func is Always or Never, we don't really care about LRZ
                    if (is_depth_test_enabled &&
                                    ((zfunc != VK_COMPARE_OP_NEVER) && (zfunc !=
        VK_COMPARE_OP_ALWAYS)))
                    {
                        ostream << "[WARNING!] LRZ is disabled with performance penalties!";
                    }
                }
                else
                {
                    ostream << "LRZ:Enabled\t";
                }
            }
            else
            {
                ostream << "LRZ:Disabled\t";
            }
            ostream << "\n";
        }
        */
    }

    for (uint32_t i = 0; i < kNumStats; ++i)
    {
        ostream << statsDesc[i] << ": " << statsList[i] << std::endl;
    }
}

int main(int argc, char **argv)
{
    Pm4InfoInit();

    // Handle args
    if ((argc != 2) && (argc != 3))
    {
        std::cout << "You need to call: lrz_validator <input_file_name.rd> "
                     "<output_details_file_name.txt>(optional)";
        return 0;
    }
    char *input_file_name = argv[1];

    std::string output_file_name = "";
    if (argc == 3)
    {
        output_file_name = argv[2];
    }

    // Load capture
    Dive::LogCompound               log_compound;
    std::unique_ptr<Dive::DataCore> data_core = std::make_unique<Dive::DataCore>(&log_compound);
    Dive::CaptureData::LoadResult   load_res = data_core->LoadCaptureData(input_file_name);
    if (load_res != Dive::CaptureData::LoadResult::kSuccess)
    {
        std::cout << "Loading capture \"" << input_file_name << "\" failed!";
        return 0;
    }
    std::cout << "Capture file \"" << input_file_name << "\" is loaded!\n";

    // Create meta data
    if (!data_core->CreateMetaData())
    {
        std::cout << "Failed to create meta data!";
        return 0;
    }
    std::cout << "Gathering Stats...\n";

    std::ostream *ostream = &std::cout;
    std::ofstream ofstream;
    if (!output_file_name.empty())
    {
        std::cout << "Output detailed validation result to \"" << output_file_name << "\""
                  << std::endl;
        ofstream.open(output_file_name);
        ostream = &ofstream;
    }

    // Gather Stats...
    const Dive::CaptureMetadata &meta_data = data_core->GetCaptureMetadata();
    GatherStats(meta_data, *ostream);

    return 1;
}
