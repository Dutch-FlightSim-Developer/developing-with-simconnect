#pragma once
/*
 * Copyright (c) 2025. Bert Laverman
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

#include <simconnect/message_handler.hpp>
#include <simconnect/data/init_position.hpp>


 namespace SimConnect {

class AIHandler : public MessageHandler<AIHandler, SIMCONNECT_RECV_ID_ASSIGNED_OBJECT_ID> {

    // No copies or moves
    AIHandler(const AIHandler&) = delete;
    AIHandler(AIHandler&&) = delete;
    AIHandler& operator=(const AIHandler&) = delete;
    AIHandler& operator=(AIHandler&&) = delete;

public:
    AIHandler() = default;
    ~AIHandler() = default;


    /**
     * Returns the correlation ID from the message. This is specific to the SIMCONNECT_RECV_ASSIGNED_OBJECT_ID message.
     *
     * @param msg The message to get the correlation ID from.
     * @returns The correlation ID from the message.
     */
    unsigned long correlationId(const SIMCONNECT_RECV& msg) const {
        return static_cast<const SIMCONNECT_RECV_ASSIGNED_OBJECT_ID*>(&msg)->dwRequestID;
    }


    void createNonATCAircraft(Connection& connection,
        std::string title, std::string livery, std::string tailNumber,
        Data::InitPosition initPos,
        std::function<void(unsigned long)> objectIdHandler)
    {
        auto requestId = connection.requests().nextRequestID();

        registerHandler(requestId, [objectIdHandler](const SIMCONNECT_RECV& msg) {
            auto& assigned = reinterpret_cast<const SIMCONNECT_RECV_ASSIGNED_OBJECT_ID&>(msg);
            objectIdHandler(assigned.dwObjectID);
        }, true);

        connection.createNonATCAircraft(title, livery, tailNumber, initPos, requestId);
    }
};

} // namespace SimConnect