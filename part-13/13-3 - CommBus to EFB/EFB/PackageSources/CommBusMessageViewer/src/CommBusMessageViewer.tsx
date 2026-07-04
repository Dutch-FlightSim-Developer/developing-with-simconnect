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

// CommBus Message Viewer - a minimal EFB app demonstrating the CommBus Communication API.
//
// Unlike ClientData (see part-11), CommBus needs no shared memory area and no data
// definition set up in advance: any SimConnect client can broadcast a named event with a
// string payload, and any JS gauge, WASM module, or other SimConnect client that has
// registered a listener for that event name receives it directly. This app just shows
// the last message received on "DutchFlightSim.Tutorial.Hello", the event the 13-3 C++
// sender (see ../../C++/) broadcasts on.
//
// This is the first EFB/JS example in this repository. It is built and tested through
// MSFS Developer Mode's Project Editor, not through this repo's CMake pipeline - see
// README.md in this folder for the setup and build/test steps.
//
// NOTE: The CommBusAircraft sample gauge and the SDK's JS Communication API docs use
// RegisterCommBusListener() (from including CommBus.js), but that throws
// "Can't find variable: RegisterCommBusListener" when run inside an EFB page - it seems
// to only initialize correctly in a VCockpit gauge context. Registering directly against
// the underlying Coherent view listener it wraps, "JS_LISTENER_COMM_BUS", works in the EFB
// instead - see https://devsupport.flightsimulator.com/t/is-commbus-available-in-efb/15285/3.
// RegisterViewListener and ViewListener are already typed by @microsoft/msfs-types, so no
// ambient declarations are needed here.

import { App, AppInstallProps, AppView, AppViewProps, Efb, RequiredProps, TVNode } from "@efb/efb-api";
import { FSComponent, Subject, VNode } from "@microsoft/msfs-sdk";

import "./CommBusMessageViewer.scss";

/** The CommBus event name broadcast by the 13-3 C++ sender. Must match COMMBUS_HELLO_EVENT in shared.hpp. */
const COMMBUS_HELLO_EVENT = "DutchFlightSim.Tutorial.Hello";

/** BASE_URL is defined by build.js and points at this app's coui:// folder once packaged. */
declare const BASE_URL: string;

class CommBusMessageViewerView extends AppView<RequiredProps<AppViewProps, "bus">> {
  private readonly lastMessage = Subject.create<string>("(no message received yet)");

  /**
   * Register the CommBus listener once this view is opened.
   *
   * No subscription needs to exist on the SimConnect side beforehand for calls to reach
   * us here - SimConnect_CallCommBusEvent broadcasts with
   * SIMCONNECT_COMM_BUS_BROADCAST_TO_DEFAULT by default, which already includes JS gauges.
   */
  public onOpen(): void {
    const listener = RegisterViewListener("JS_LISTENER_COMM_BUS");
    listener.on(COMMBUS_HELLO_EVENT, (data: string) => {
      this.lastMessage.set(data);
    });
  }

  public render(): VNode {
    return (
      <div class="commbus-message-viewer">
        <h2>CommBus Message Viewer</h2>
        <p>Listening for CommBus event:</p>
        <code>{COMMBUS_HELLO_EVENT}</code>
        <div class="last-message">{this.lastMessage}</div>
      </div>
    );
  }
}

class CommBusMessageViewer extends App {
  public get name(): string {
    return "CommBus Message Viewer";
  }

  public get icon(): string {
    return `${BASE_URL}/Assets/app-icon.svg`;
  }

  /**
   * Loads this app's compiled stylesheet into the page. Without this, CommBusMessageViewer.css
   * is built into dist/ but never applied, so the view renders with default (black) text on the
   * EFB's dark background - effectively invisible.
   */
  public async install(_props: AppInstallProps): Promise<void> {
    Efb.loadCss(`${BASE_URL}/CommBusMessageViewer.css`);
    return Promise.resolve();
  }

  public render(): TVNode<CommBusMessageViewerView> {
    return <CommBusMessageViewerView bus={this.bus} />;
  }
}

/**
 * App definition to be injected into the EFB.
 */
Efb.use(CommBusMessageViewer);
