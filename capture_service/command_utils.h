/*
Copyright 2023 Google Inc.

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

#include <string>

namespace Dive
{

struct CommandResult
{
    std::string        m_output;
    std::string        m_err;
    int                m_ret = -1;
    bool               Ok() const { return m_ret == 0; }
    const std::string &Out() const { return m_output; }
    const std::string &Err() const { return m_err; }
};

CommandResult RunCommand(const std::string &command, bool quiet = false);

class AdbSession
{
public:
    AdbSession() = default;
    AdbSession(const std::string &serial) :
        m_serial(serial)
    {
    }

    CommandResult Run(const std::string &command, bool quiet = false) const;

private:
    std::string m_serial;
};
}  // namespace Dive