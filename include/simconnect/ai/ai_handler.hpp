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


#include <simconnect/simconnect.hpp>
#include <simconnect/message_handler.hpp>
#include <simconnect/data/init_position.hpp>


namespace SimConnect {

template <class M>
class AIHandler : public MessageHandler<RequestId, AIHandler, M, Messages::assignedObjectId> {

    // No copies or moves
    AIHandler(const AIHandler&) = delete;
    AIHandler(AIHandler&&) = delete;
    AIHandler& operator=(const AIHandler&) = delete;
    AIHandler& operator=(AIHandler&&) = delete;

public:
    AIHandler() = default;
    ~AIHandler() = default;


    /**
     * Returns the correlation ID from the message. This is specific to the Messages::AssignedObjectId message.
     *
     * @param msg The message to get the correlation ID from.
     * @returns The correlation ID from the message.
     */
    unsigned long correlationId(const Messages::MsgBase& msg) const {
        return static_cast<const Messages::AssignedObjectId*>(&msg)->dwRequestID;
    }


    void createNonATCAircraft(Connection& connection,
        std::string title, std::string livery, std::string tailNumber,
        Data::InitPosition initPos,
        std::function<void(unsigned long)> objectIdHandler)
    {
        auto requestId = connection.requests().nextRequestID();

        registerHandler(requestId, [objectIdHandler](const Messages::MsgBase& msg) {
            auto& assigned = static_cast<const Messages::AssignedObjectId&>(msg);
            objectIdHandler(assigned.dwObjectID);
        }, true);

        connection.createNonATCAircraft(title, livery, tailNumber, initPos, requestId);
    }
};

} // namespace SimConnect