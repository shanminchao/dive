#!/usr/bin/python3 -i
#
# Copyright (c) 2018-2020 Valve Corporation
# Copyright (c) 2018-2020 LunarG, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

from khronos_base_generator import write


class KhronosReplayConsumerBodyGenerator():
    """Base class for generating replay cousumers body code."""

    def endFile(self):
        self.generate_replay_dump_resources_body()

    def generate_replay_dump_resources_body(self):
        """Performs C++ code generation for the replay dump resources."""
        platform_type = self.get_api_prefix()

        for cmd in self.get_all_filtered_cmd_names():

            if self.is_resource_dump_class() and self.is_dump_resources_api_call(cmd) == False:
                continue

            info = self.all_cmd_params[cmd]
            return_type = info[0]
            values = info[2]

            cmddef = '\n'
            if self.is_resource_dump_class():
                cmddef += self.make_dump_resources_func_decl(
                    return_type,
                    '{}ReplayDumpResources::Process_'.format(platform_type) + cmd,
                    values, cmd in self.DUMP_RESOURCES_OVERRIDES
                ) + '\n'
            else:
                cmddef += self.make_consumer_func_decl(
                    return_type,
                    '{}ReplayConsumer::Process_'.format(platform_type) + cmd,
                    values
                ) + '\n'
            cmddef += '{\n'
            cmddef += self.make_consumer_func_body(return_type, cmd, values)
            cmddef += '}'

            write(cmddef, file=self.outFile)
