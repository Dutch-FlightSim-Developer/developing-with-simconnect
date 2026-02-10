#pragma once
/*
 * Copyright (c) 2026. Bert Laverman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <map>
#include <span>
#include <vector>
#include <string>
#include <string_view>

namespace SimConnect::Util {


class Args {
    std::map<std::string, std::string_view> args_;
    std::vector<std::string_view> positionalArgs_;

public:
    Args(std::span<const char*> args) {
        int fixedArg{ 0 };

        args_["Arg" + std::to_string(fixedArg++)] = args[0];
        positionalArgs_.emplace_back(args[0]);

        for (size_t i = 1; i < args.size(); ++i) {
            const std::string_view arg = args[i];
            if (arg.starts_with("--")) {
                auto eqPos = arg.find('=');
                if (eqPos != std::string::npos) {
                    const std::string key(arg.data() + 2, eqPos - 2);

                    args_.emplace(key, std::string_view(arg.data() + eqPos + 1, arg.size() - eqPos - 1));
                } else {
                    const std::string key(arg.data() + 2, arg.size() - 2);
                    args_[key] = "";// No value provided
                }
            } else {
                args_["Arg" + std::to_string(fixedArg++)] = arg;
                positionalArgs_.emplace_back(arg);
            }
        }
    }
    Args(const Args&) = default;
    Args(Args&&) = default;
    Args& operator=(const Args&) = default;
    Args& operator=(Args&&) = default;
    ~Args() = default;

    /**
     * Check if the argument with the given key exists.
     * 
     * @param key The argument key to check.
     * @return true if the argument exists, false otherwise.
     */
    bool has(const std::string key) const {
        return args_.contains(key);
    }


    /**
     * Get the argument value for the given key.
     * 
     * @param key The argument key to get the value for.
     * @return The argument value if it exists, or an empty string view otherwise.
     */
    std::string_view operator[](const std::string key) const {
        auto it = args_.find(key);
        if (it != args_.end()) {
            return it->second;
        }
        static const std::string_view empty{};
        return empty;
    }


    /**
     * Return the number of positional arguments, excluding the programname itself.
     * 
     * @return The number of positional arguments.
     */
    size_t positionalCount() const {
        return positionalArgs_.size() - 1;
    }

    /**
     * Check if the positional argument with the given index exists.
     * 
     * @param index The positional argument index to check.
     * @return true if the positional argument exists, false otherwise.
     */
    bool has(size_t index) const {
        return index < positionalArgs_.size();
    }


    /**
     * Get the positional argument value for the given index.
     * 
     * @param index The positional argument index to get the value for.
     * @return The positional argument value if it exists, or an empty string view otherwise.
     */
    std::string_view operator[](size_t index) const {
        if (index < positionalArgs_.size()) {
            return positionalArgs_[index];
        }
        static const std::string_view empty{};
        return empty;
    }


    /**
     * Get the program name (the first positional argument).
     * 
     * @return The program name if it exists, or an empty string view otherwise.
     */
    std::string_view programName() const {
        return positionalArgs_.empty() ? std::string_view{} : positionalArgs_[0];
    }
};

} // namespace SimConnect::Util