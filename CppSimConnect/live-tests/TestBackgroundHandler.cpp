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
 * See the License for the specific language governing permissions and limitations under the License.
 */

#include "pch.h"

#include <simconnect/util/console_logger.hpp>
#include <simconnect/background_simconnect_manager.hpp>


using namespace SimConnect;
using namespace std::chrono_literals;

TEST(TestBackgroundHandler, StartStop) {
	BackgroundSimConnectManager handler("TestBackgroundHandler");

	// Start the background handler
	handler.start();
	handler.connect();
	std::this_thread::sleep_for(500ms); // Let it run for a bit
	EXPECT_EQ(handler.getState(), State::Connected) << "Background handler should be connected after start()";

	// Stop the background handler
	handler.stop();
	std::this_thread::sleep_for(500ms); // Give it some time to stop
	EXPECT_EQ(handler.getState(), State::Stopped) << "Background handler should be stopped after stop()";
}