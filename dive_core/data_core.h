
/*
 Copyright 2019 Google LLC

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

#pragma once
#include <algorithm>
#include <unordered_map>
#include <vector>
#include "capture_data.h"
#include "capture_event_info.h"
#include "command_hierarchy.h"
#include "event_state.h"
#include "progress_tracker.h"

namespace Dive
{

//--------------------------------------------------------------------------------------------------
// Metadata that describes information in the capture
struct CaptureMetadata
{
    // Information about the command buffers, represented in a tree hierarchy
    CommandHierarchy m_command_hierarchy;

    // Information about each shader in the capture
    DiveVector<Disassembly> m_shaders;

    // Descriptors in the capture
    DiveVector<A6XX_UBO>       m_ubos;       // aka vSharp
    DiveVector<A6XX_TEX_CONST> m_tex_const;  // aka tSharp
    DiveVector<A6XX_TEX_SAMP>  m_tex_samp;   // aka sSharp

    // Information about each event in the capture
    DiveVector<EventInfo> m_event_info;

    // Register state tracking for each event
    // This is separated from EventInfo to take advantage of code-gen
    EventStateInfo m_event_state;

    // Information about the submits in this capture
    uint64_t m_num_pm4_packets;
};

//--------------------------------------------------------------------------------------------------
// Main container for the capture data as well as associated metadata
class DataCore
{
protected:
    ProgressTracker *m_progress_tracker;

public:
    DataCore(ILog *log_ptr);

    DataCore(ProgressTracker *progress_tracker, ILog *log_ptr);

    // Load the capture file
    CaptureData::LoadResult LoadCaptureData(const char *file_name);

    // Parse the capture to generate info that describes the capture
    bool ParseCaptureData();

    // Create meta data from the captured data
    bool CreateMetaData();

    // Get the capture data (includes access to raw command buffers and memory blocks)
    const CaptureData &GetCaptureData() const;
    CaptureData       &GetMutableCaptureData();

    // Get the command-hierarchy, which is a tree view interpretation of the command buffer
    const CommandHierarchy &GetCommandHierarchy() const;

    // Get metadata describing the capture (info obtained by parsing the capture)
    const CaptureMetadata &GetCaptureMetadata() const;

private:
    // Create command hierarchy from the captured data
    bool CreateCommandHierarchy();
    // The relatively raw captured data (memory & submit blocks)
    CaptureData m_capture_data;

    // Metadata for the capture data in m_capture_data
    CaptureMetadata m_capture_metadata;

    // Handle to the logging system for error/warning output
    ILog *m_log_ptr;
};

//--------------------------------------------------------------------------------------------------
// Handles creation of much of the metadata "info" from the capture
class CaptureMetadataCreator : public IEmulateCallbacks
{
public:
    CaptureMetadataCreator(CaptureMetadata &capture_metadata, EmulateStateTracker &state_tracker);
    ~CaptureMetadataCreator();

    virtual void OnSubmitStart(uint32_t submit_index, const SubmitInfo &submit_info) override;
    virtual void OnSubmitEnd(uint32_t submit_index, const SubmitInfo &submit_info) override;

    const EmulateStateTracker &GetStateTracker() const { return m_state_tracker; }

    // Callbacks
    virtual bool OnIbStart(uint32_t                  submit_index,
                           uint32_t                  ib_index,
                           const IndirectBufferInfo &ib_info,
                           IbType                    type) override;

    virtual bool OnIbEnd(uint32_t                  submit_index,
                         uint32_t                  ib_index,
                         const IndirectBufferInfo &ib_info) override;

    virtual bool OnPacket(const IMemoryManager &mem_manager,
                          uint32_t              submit_index,
                          uint32_t              ib_index,
                          uint64_t              va_addr,
                          Pm4Header             header) override;

private:
    bool HandleDescriptors(const IMemoryManager &mem_manager,
                           uint32_t              submit_index,
                           uint64_t              load_state6_addr);
    bool HandleShaders(const IMemoryManager &mem_manager,
                       uint32_t              submit_index,
                       uint32_t              opcode,
                       EventInfo            *event_info);
    void FillEventStateInfo(EventStateInfo::Iterator event_state_it);
    void FillInputAssemblyState(EventStateInfo::Iterator event_state_it);
    void FillTessellationState(EventStateInfo::Iterator event_state_it);
    void FillViewportState(EventStateInfo::Iterator event_state_it);
    void FillRasterizerState(EventStateInfo::Iterator event_state_it);
    void FillMultisamplingState(EventStateInfo::Iterator event_state_it);
    void FillDepthState(EventStateInfo::Iterator event_state_it);
    void FillColorBlendState(EventStateInfo::Iterator event_state_it);
    void FillHardwareSpecificStates(EventStateInfo::Iterator event_state_it);

    // Map from Sharp dwords to index (in m_capture_metadata arrays)
    struct Sharp
    {
        Sharp(uint32_t *dwords, uint32_t num_dwords, uint32_t type)
        {
            assert(num_dwords <= kMaxSharpDwords);
            memcpy(m_dwords, dwords, num_dwords * sizeof(uint32_t));
            m_num_dwords = num_dwords;
            m_type = type;
        }
        bool operator==(const Sharp &rhs) const
        {
            if (rhs.m_num_dwords != m_num_dwords)
                return false;
            return memcmp(m_dwords, rhs.m_dwords, m_num_dwords) == 0;
        }
        static const uint32_t kMaxSharpDwords = 17;
        uint32_t              m_type;
        uint32_t              m_num_dwords;
        uint32_t              m_dwords[kMaxSharpDwords];
        static_assert(kMaxSharpDwords * sizeof(uint32_t) >= sizeof(A6XX_TEX_SAMP));
        static_assert(kMaxSharpDwords * sizeof(uint32_t) >= sizeof(A6XX_TEX_CONST));
        static_assert(kMaxSharpDwords * sizeof(uint32_t) >= sizeof(A6XX_UBO));
        friend struct SharpHasher;
    };
    struct SharpHasher
    {
        std::size_t operator()(const Sharp &k) const
        {
            static_assert(sizeof(std::size_t) == sizeof(uint64_t));

            // A simple naive hash function, not really based on anything
            size_t hash = 0;
            for (uint32_t i = 0; i < k.m_num_dwords / 2; i += 2)
            {
                size_t new_dword = ((size_t)k.m_dwords[i] << 32) | k.m_dwords[i + 1];
                hash ^= new_dword + 0xc1b727fecbf779c9 + (hash << 4) + (hash >> 4);
            }
            if (k.m_num_dwords % 2 != 0)
            {
                size_t new_dword = k.m_dwords[k.m_num_dwords - 1];
                hash ^= new_dword + 0xc1b727fecbf779c9 + (hash << 4) + (hash >> 4);
            }
            return hash;
        }
    };

    // Map from descriptor to descriptor index (in m_capture_metadata)
    std::unordered_map<Sharp, uint32_t, SharpHasher> m_sharp_map;

    // Map from shader address to shader index (in m_capture_metadata.m_shaders)
    std::unordered_map<uint64_t, uint32_t> m_shader_addrs;

    // Cache all descriptor indices created for the current event
    DiveVector<DescriptorReference> m_ubo_indices;        // aka vSharp
    DiveVector<DescriptorReference> m_tex_const_indices;  // aka tSharp
    DiveVector<DescriptorReference> m_tex_samp_indices;   // aka sSharp

    CaptureMetadata     &m_capture_metadata;
    EmulateStateTracker &m_state_tracker;
    RenderModeType       m_current_render_mode = RenderModeType::kUnknown;
};

}  // namespace Dive
