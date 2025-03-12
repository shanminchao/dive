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

#include <QFrame>
#pragma once

// Forward declaration
class QTableWidget;
class QTreeWidget;
class QTreeWidgetItem;
namespace Dive
{
class DataCore;
}

//--------------------------------------------------------------------------------------------------
class DescriptorView : public QFrame
{
    Q_OBJECT

public:
    DescriptorView(const Dive::DataCore &data_core);

private slots:
    void OnEventSelected(uint64_t node_index);

private:
    const Dive::DataCore &m_data_core;
    QTreeWidget *         m_tex_const_list;
    QTreeWidget *         m_ubo_list;
    QTreeWidget *         m_tex_samp_list;
    uint32_t              m_event_index;
};
