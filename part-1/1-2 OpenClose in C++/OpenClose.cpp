/*
 * Copyright (c) 2024. Bert Laverman
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

#include <simconnect.hpp>
#include <simconnect/simple_connection.hpp>

#include <iostream>


int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) // NOLINT(bugprone-exception-escape)
{
	try {
		SimConnect::SimpleConnection simConnect("CleanOpenClose");

		if (simConnect.open()) {	// Try this with open(5) or some other undefined section number...
			std::cout << "Connected to Flight Simulator.\n";

			simConnect.close();
			std::cout << "Disconnected from Flight Simulator.\n";
		}
		else {
			std::cerr << "Failed to connect to Flight Simulator!\n";
		}
		return simConnect ? 0 : 1;
	}
	catch (const SimConnect::SimConnectException& ex) {
		std::cerr << "SimConnect exception: " << ex.what() << "\n";
		return 1;
	}
}