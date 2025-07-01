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

#define CHECK_AND_TRACK_STATE_1(stats_enum, state) \
    if (event_state_it->Is##state##Set())          \
        statsList[stats_enum]++;

#define CHECK_AND_TRACK_STATE_2(stats_enum, state1, state2)                     \
    if (event_state_it->Is##state1##Set() && event_state_it->Is##state2##Set()) \
        statsList[stats_enum]++;

#define CHECK_AND_TRACK_STATE_3(stats_enum, state1, state2, state3)               \
    if (event_state_it->Is##state1##Set() && event_state_it->Is##state2##Set() && \
        event_state_it->Is##state3##Set())                                        \
        statsList[stats_enum]++;

#define CHECK_AND_TRACK_STATE_EQUAL(stats_enum, state, state_value) \
    if (event_state_it->Is##state##Set())                           \
        if (event_state_it->state() == state_value)                 \
            statsList[stats_enum]++;

#define CHECK_AND_TRACK_STATE_NOT_EQUAL(stats_enum, state, state_value) \
    if (event_state_it->Is##state##Set())                           \
        if (event_state_it->state() != state_value)                 \
            statsList[stats_enum]++;

#define FUNC_CHOOSER(_f1, _f2, _f3, _f4, ...) _f4
#define MSVC_WORKAROUND(argsWithParentheses) FUNC_CHOOSER argsWithParentheses

// Trailing , is workaround for GCC/CLANG, suppressing false positive error "ISO C99 requires rest
// arguments to be used" MSVC_WORKAROUND is workaround for MSVC for counting # of arguments
// correctly
#define CHECK_AND_TRACK_STATE_N(...) \
    MSVC_WORKAROUND(                 \
    (__VA_ARGS__, CHECK_AND_TRACK_STATE_3, CHECK_AND_TRACK_STATE_2, CHECK_AND_TRACK_STATE_1, ))
#define CHECK_AND_TRACK_STATE(stats_enum, ...) \
    CHECK_AND_TRACK_STATE_N(__VA_ARGS__)(stats_enum, __VA_ARGS__)

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
        kDepthTestEnabled,
        kDepthWriteEnabled,
        kEarlyZ,
        kLateZ,
        kEarlyLRZLateZ,
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
        "Num Draws with Depth Test Enabled",
        "Num Draws with Depth Write Enabled",
        "Num Draws with EarlyZ",
        "Num Draws with LateZ",
        "Num Draws with Early LRZ & LateZ",
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

        if (info.m_render_mode != Dive::RenderModeType::kBinning)
        {
            CHECK_AND_TRACK_STATE(kDepthTestEnabled, DepthTestEnabled);
            CHECK_AND_TRACK_STATE(kDepthWriteEnabled, DepthTestEnabled, DepthWriteEnabled);
            if (event_state_it->DepthTestEnabled())
            {
                CHECK_AND_TRACK_STATE_EQUAL(kEarlyZ, ZTestMode, A6XX_EARLY_Z);
                CHECK_AND_TRACK_STATE_EQUAL(kLateZ, ZTestMode, A6XX_LATE_Z);
                CHECK_AND_TRACK_STATE_EQUAL(kEarlyLRZLateZ, ZTestMode, A6XX_EARLY_LRZ_LATE_Z);
            }
        }
        if ((info.m_render_mode == Dive::RenderModeType::kDirect) ||
            (info.m_render_mode == Dive::RenderModeType::kBinning))
        {
            CHECK_AND_TRACK_STATE(kLrzEnabled, DepthTestEnabled, LRZEnabled);
            CHECK_AND_TRACK_STATE(kLrzWriteEnabled, DepthTestEnabled, DepthWriteEnabled, LRZWrite);
        }

        CHECK_AND_TRACK_STATE_NOT_EQUAL(kCullModeEnabled, CullMode, VK_CULL_MODE_NONE);
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
