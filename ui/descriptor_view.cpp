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
#include "descriptor_view.h"
#include "dive_core/data_core.h"
#include "dive_core/dive_strings.h"
#include "dive_core/pm4_info.h"

#include <QLabel>
#include <QTableWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

// =================================================================================================
// DescriptorWidgetItem
// =================================================================================================
class DescriptorWidgetItem : public QTreeWidgetItem
{
public:
    DescriptorWidgetItem(uint32_t buffer_index, QTreeWidget *view) :
        QTreeWidgetItem(view),
        m_buffer_index(buffer_index)
    {
    }
    uint32_t GetBufferIndex() const { return m_buffer_index; }

private:
    uint32_t m_buffer_index;
};

// =================================================================================================
// DescriptorView
// =================================================================================================
DescriptorView::DescriptorView(const Dive::DataCore &data_core) :
    m_data_core(data_core),
    m_event_index(UINT32_MAX)
{
    QVBoxLayout *layout = new QVBoxLayout();

    QLabel *texture_label = new QLabel(tr("A6XX_TEX_CONST"));
    m_tex_const_list = new QTreeWidget();
    m_tex_const_list->setColumnCount(18);
    m_tex_const_list->setHeaderLabels(QStringList() << "Bindless"
                                                    << "Stage"
                                                    << "Descriptor Set"
                                                    << "Offset"
                                                    << "Width"
                                                    << "Height"
                                                    << "Depth"
                                                    << "Format"
                                                    << "MipLevels"
                                                    << "Addr"
                                                    << "MinLodClamp"
                                                    << "Swizzle (x,y,z,w)"
                                                    << "Tile Mode"
                                                    << "Flag"
                                                    << "Flag Buffer Addr"
                                                    << "Flag Buffer Pitch"
                                                    << "Flag Buffer Array Pitch"
                                                    << "Flag Buffer LogW"
                                                    << "Flag Buffer LogH");

    QLabel *ubo_label = new QLabel(tr("A6XX_UBO"));
    m_ubo_list = new QTreeWidget();
    m_ubo_list->setColumnCount(6);
    m_ubo_list->setHeaderLabels(QStringList() << "Bindless"
                                              << "Stage"
                                              << "Descriptor Set"
                                              << "Offset"
                                              << "Addr"
                                              << "Size (Dwords)");

    QLabel *sampler_label = new QLabel(tr("A6XX_TEX_SAMP"));
    m_tex_samp_list = new QTreeWidget();
    m_tex_samp_list->setColumnCount(21);
    m_tex_samp_list->setHeaderLabels(QStringList() << "Bindless"
                                                   << "Stage"
                                                   << "Descriptor Set"
                                                   << "Offset"
                                                   << "Min"
                                                   << "Mag"
                                                   << "Aniso"
                                                   << "MipFilter Linear Near"
                                                   << "MipFilter Linear Far"
                                                   << "WrapS"
                                                   << "WrapT"
                                                   << "WrapR"
                                                   << "Lod Bias"
                                                   << "Min-Max LOD"
                                                   << "Clamp Enable"
                                                   << "Compare Func"
                                                   << "Reduction Mode"
                                                   << "Cubemap Seamless Filter Off"
                                                   << "Unnormalized Coords"
                                                   << "Chroma Linear"
                                                   << "Border Color");

    layout->addWidget(texture_label);
    layout->addWidget(m_tex_const_list);
    layout->setStretchFactor(m_tex_const_list, 1);

    layout->addWidget(ubo_label);
    layout->addWidget(m_ubo_list);
    layout->setStretchFactor(m_ubo_list, 1);

    layout->addWidget(sampler_label);
    layout->addWidget(m_tex_samp_list);
    layout->setStretchFactor(m_tex_samp_list, 1);
    setLayout(layout);
}

//--------------------------------------------------------------------------------------------------
void DescriptorView::OnEventSelected(uint64_t node_index)
{
    m_tex_const_list->clear();
    m_ubo_list->clear();
    m_tex_samp_list->clear();
    if (node_index == UINT64_MAX)
        return;

    const Dive::CommandHierarchy &command_hierarchy = m_data_core.GetCommandHierarchy();
    Dive::NodeType                node_type = command_hierarchy.GetNodeType(node_index);
    if (node_type != Dive::NodeType::kDrawDispatchBlitNode)
        return;

    uint32_t event_index = command_hierarchy.GetEventNodeId(node_index);

    const Dive::CaptureMetadata &metadata = m_data_core.GetCaptureMetadata();

    const uint32_t str_buffer_size = 256;
    char           str_buffer[str_buffer_size];

    // Add the descriptor(s) to the list
    const Dive::EventInfo &event_info = metadata.m_event_info[event_index];
    for (uint64_t i = 0; i < event_info.m_tex_const_references.size(); ++i)
    {
        const Dive::DescriptorReference &ref = event_info.m_tex_const_references[i];
        const A6XX_TEX_CONST            &descriptor = metadata.m_tex_const[ref.m_descriptor_index];
        DescriptorWidgetItem            *treeItem = new DescriptorWidgetItem(ref.m_descriptor_index,
                                                                  m_tex_const_list);

        int column = 0;

        // Bindless
        snprintf(str_buffer, str_buffer_size, "%d", ref.m_bindless);
        treeItem->setText(column++, tr(str_buffer));

        // Stage
        const char *stage_str = "Unknown";
        assert((uint32_t)ref.m_stage <= (uint32_t)Dive::ShaderStage::kShaderStageCount);
        if (ref.m_stage != Dive::ShaderStage::kShaderStageCount)
            stage_str = kShaderStageStrings[(uint32_t)ref.m_stage];
        snprintf(str_buffer, str_buffer_size, "%s", stage_str);
        treeItem->setText(column++, tr(str_buffer));

        // Descriptor Set
        snprintf(str_buffer, str_buffer_size, "%d", ref.m_descriptor_set);
        treeItem->setText(column++, tr(str_buffer));

        // Offset
        snprintf(str_buffer, str_buffer_size, "%d", ref.m_descriptor_set_offset);
        treeItem->setText(column++, tr(str_buffer));

        // Width
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields1.WIDTH);
        treeItem->setText(column++, tr(str_buffer));

        // Height
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields1.HEIGHT);
        treeItem->setText(column++, tr(str_buffer));

        // Depth
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields5.DEPTH);
        treeItem->setText(column++, tr(str_buffer));

        // Format
        uint32_t    enum_handle = GetEnumHandle("a6xx_format");
        const char *enum_str = GetEnumString(enum_handle, descriptor.bitfields0.FMT);
        treeItem->setText(column++, tr(enum_str));

        // MipLevels
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields0.MIPLVLS);
        treeItem->setText(column++, tr(str_buffer));

        // Addr
        uint64_t addr = ((uint64_t)descriptor.bitfields5.BASE_HI << 32) |
                        descriptor.bitfields4.BASE_LO;
        snprintf(str_buffer, str_buffer_size, "0x%llx", (unsigned long long)addr);
        treeItem->setText(column++, tr(str_buffer));

        // MinLodClamp
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields6.MIN_LOD_CLAMP);
        treeItem->setText(column++, tr(str_buffer));

        // Swizzle
        const char *swizzle_str[] = { "x", "y", "z", "w", "0", "1", "", "" };  // 3-bits
        snprintf(str_buffer,
                 str_buffer_size,
                 "%s, %s, %s, %s",
                 swizzle_str[descriptor.bitfields0.SWIZ_X],
                 swizzle_str[descriptor.bitfields0.SWIZ_Y],
                 swizzle_str[descriptor.bitfields0.SWIZ_Z],
                 swizzle_str[descriptor.bitfields0.SWIZ_W]);
        treeItem->setText(column++, tr(str_buffer));

        // Tile
        enum_handle = GetEnumHandle("a6xx_tile_mode");
        enum_str = GetEnumString(enum_handle, descriptor.bitfields0.TILE_MODE);
        treeItem->setText(column++, tr(enum_str));

        // Flag
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields3.FLAG);
        treeItem->setText(column++, tr(str_buffer));

        // Flag Buffer Addr
        addr = ((uint64_t)descriptor.bitfields8.FLAG_HI << 32) | descriptor.bitfields7.FLAG_LO;
        snprintf(str_buffer, str_buffer_size, "0x%llx", (unsigned long long)addr);
        treeItem->setText(column++, tr(str_buffer));

        // Flag Buffer Pitch
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields9.FLAG_BUFFER_ARRAY_PITCH);
        treeItem->setText(column++, tr(str_buffer));

        // Flag Buffer Array Pitch
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields9.FLAG_BUFFER_ARRAY_PITCH);
        treeItem->setText(column++, tr(str_buffer));

        // Flag Buffer LogW
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields10.FLAG_BUFFER_LOGW);
        treeItem->setText(column++, tr(str_buffer));

        // Flag Buffer LogH
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields10.FLAG_BUFFER_LOGH);
        treeItem->setText(column++, tr(str_buffer));
    }

    for (uint64_t i = 0; i < event_info.m_ubo_references.size(); ++i)
    {
        const Dive::DescriptorReference &ref = event_info.m_ubo_references[i];
        const A6XX_UBO                  &descriptor = metadata.m_ubos[ref.m_descriptor_index];
        DescriptorWidgetItem            *treeItem = new DescriptorWidgetItem(ref.m_descriptor_index,
                                                                  m_ubo_list);

        int column = 0;

        // Bindless
        snprintf(str_buffer, str_buffer_size, "%d", ref.m_bindless);
        treeItem->setText(column++, tr(str_buffer));

        // Stage
        const char *stage_str = "Unknown";
        assert((uint32_t)ref.m_stage <= (uint32_t)Dive::ShaderStage::kShaderStageCount);
        if (ref.m_stage != Dive::ShaderStage::kShaderStageCount)
            stage_str = kShaderStageStrings[(uint32_t)ref.m_stage];
        snprintf(str_buffer, str_buffer_size, "%s", stage_str);
        treeItem->setText(column++, tr(str_buffer));

        // Descriptor Set
        snprintf(str_buffer, str_buffer_size, "%d", ref.m_descriptor_set);
        treeItem->setText(column++, tr(str_buffer));

        // Offset
        snprintf(str_buffer, str_buffer_size, "%d", ref.m_descriptor_set_offset);
        treeItem->setText(column++, tr(str_buffer));

        // Addr
        uint64_t addr = ((uint64_t)descriptor.bitfields1.BASE_HI << 32) |
                        descriptor.bitfields0.BASE_LO;
        snprintf(str_buffer, str_buffer_size, "0x%llx", (unsigned long long)addr);
        treeItem->setText(column++, tr(str_buffer));

        // Size
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields1.SIZE);
        treeItem->setText(column++, tr(str_buffer));
    }

    for (uint64_t i = 0; i < event_info.m_tex_samp_references.size(); ++i)
    {
        const Dive::DescriptorReference &ref = event_info.m_tex_samp_references[i];
        const A6XX_TEX_SAMP             &descriptor = metadata.m_tex_samp[ref.m_descriptor_index];
        DescriptorWidgetItem            *treeItem = new DescriptorWidgetItem(ref.m_descriptor_index,
                                                                  m_tex_samp_list);
        int column = 0;

        // Bindless
        snprintf(str_buffer, str_buffer_size, "%d", ref.m_bindless);
        treeItem->setText(column++, tr(str_buffer));

        // Stage
        const char *stage_str = "Unknown";
        assert((uint32_t)ref.m_stage <= (uint32_t)Dive::ShaderStage::kShaderStageCount);
        if (ref.m_stage != Dive::ShaderStage::kShaderStageCount)
            stage_str = kShaderStageStrings[(uint32_t)ref.m_stage];
        snprintf(str_buffer, str_buffer_size, "%s", stage_str);
        treeItem->setText(column++, tr(str_buffer));

        // Descriptor Set
        snprintf(str_buffer, str_buffer_size, "%d", ref.m_descriptor_set);
        treeItem->setText(column++, tr(str_buffer));

        // Offset
        snprintf(str_buffer, str_buffer_size, "%d", ref.m_descriptor_set_offset);
        treeItem->setText(column++, tr(str_buffer));

        // Min
        uint32_t    enum_handle = GetEnumHandle("a6xx_tex_filter");
        const char *enum_str = GetEnumString(enum_handle, descriptor.bitfields0.XY_MIN);
        treeItem->setText(column++, tr(enum_str));

        // Min
        enum_str = GetEnumString(enum_handle, descriptor.bitfields0.XY_MAG);
        treeItem->setText(column++, tr(enum_str));

        // Aniso
        enum_handle = GetEnumHandle("a6xx_tex_aniso");
        enum_str = GetEnumString(enum_handle, descriptor.bitfields0.ANISO);
        treeItem->setText(column++, tr(enum_str));

        // MipFilter Linear Near
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields0.MIPFILTER_LINEAR_NEAR);
        treeItem->setText(column++, tr(str_buffer));

        // MipFilter Linear Far
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields1.MIPFILTER_LINEAR_FAR);
        treeItem->setText(column++, tr(str_buffer));

        // WrapS
        enum_handle = GetEnumHandle("a6xx_tex_clamp");
        enum_str = GetEnumString(enum_handle, descriptor.bitfields0.WRAP_S);
        treeItem->setText(column++, tr(enum_str));

        // WrapT
        enum_str = GetEnumString(enum_handle, descriptor.bitfields0.WRAP_T);
        treeItem->setText(column++, tr(enum_str));

        // WrapR
        enum_str = GetEnumString(enum_handle, descriptor.bitfields0.WRAP_R);
        treeItem->setText(column++, tr(enum_str));

        // Lod Bias
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields0.LOD_BIAS);
        treeItem->setText(column++, tr(str_buffer));

        // Min-Max LOD
        snprintf(str_buffer,
                 str_buffer_size,
                 "%d - %d",
                 descriptor.bitfields1.MIN_LOD,
                 descriptor.bitfields1.MAX_LOD);
        treeItem->setText(column++, tr(str_buffer));

        // Clamp Enable
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields1.CLAMPENABLE);
        treeItem->setText(column++, tr(str_buffer));

        // Compare Func
        enum_handle = GetEnumHandle("adreno_compare_func");
        enum_str = GetEnumString(enum_handle, descriptor.bitfields1.COMPARE_FUNC);
        treeItem->setText(column++, tr(enum_str));

        // Reduction Mode
        enum_handle = GetEnumHandle("a6xx_reduction_mode");
        enum_str = GetEnumString(enum_handle, descriptor.bitfields2.REDUCTION_MODE);
        treeItem->setText(column++, tr(enum_str));

        // Cubemap Seamless Filter Off
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields1.CUBEMAPSEAMLESSFILTOFF);
        treeItem->setText(column++, tr(str_buffer));

        // Unnormalized Coords
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields1.UNNORM_COORDS);
        treeItem->setText(column++, tr(str_buffer));

        // Chroma Linear
        snprintf(str_buffer, str_buffer_size, "%d", descriptor.bitfields2.CHROMA_LINEAR);
        treeItem->setText(column++, tr(str_buffer));

        // Border Color
        snprintf(str_buffer, str_buffer_size, "0x%x", descriptor.bitfields2.BCOLOR);
        treeItem->setText(column++, tr(str_buffer));
    }

    // Resize columns to fit
    for (int column = 0; column < m_tex_const_list->columnCount(); ++column)
        m_tex_const_list->resizeColumnToContents(column);
    for (int column = 0; column < m_ubo_list->columnCount(); ++column)
        m_ubo_list->resizeColumnToContents(column);
    for (int column = 0; column < m_tex_samp_list->columnCount(); ++column)
        m_tex_samp_list->resizeColumnToContents(column);

    m_event_index = event_index;
}