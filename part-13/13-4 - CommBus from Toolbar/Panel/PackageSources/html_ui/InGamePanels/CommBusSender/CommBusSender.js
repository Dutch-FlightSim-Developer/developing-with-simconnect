// CommBus Sender - a minimal in-game panel demonstrating *sending* a CommBus event from a
// plain toolbar panel (as opposed to 13-3's EFB app, which only *receives*).
//
// Clicking "Send" packages the message text and checkbox state into a JSON payload and
// broadcasts it via CommBusListener::callSimConnect() on the
// "DutchFlightSim.Tutorial.HelloJson" event. Any SimConnect client that has called
// SimConnect_SubscribeToCommBusEvent for that name receives it - see
// Connection::subscribeToCommBusEvent() in include/simconnect/connection.hpp. The C++
// listener for this event lives in ../../C++/ (built separately, not included here).
//
// API reference (SDK docs, 6_Programming_APIs/JavaScript/Communication_API):
//   function RegisterCommBusListener(callback?: any): CommBusListener;
//   CommBusListener::callSimConnect(name: string, jsonBuf: string): Promise<any>;
//
// RegisterCommBusListener() is not a built-in global - it's defined by CommBus.js, loaded via
// Include.addScript() per the SDK docs. That call is asynchronous with no documented
// completion callback or reliable ordering guarantee relative to this file's own load, so
// registerWhenReady() below polls for RegisterCommBusListener to actually exist rather than
// assuming any particular load order (see this example's README for why).
Include.addScript("/JS/Services/CommBus.js");

const COMMBUS_HELLOJSON_EVENT = "DutchFlightSim.Tutorial.HelloJson";

let commBusListener = null;

function setStatus(text) {
	const status = document.getElementById("dfs-status");
	if (status) {
		status.textContent = text;
	}
}

function onSendClicked() {
	if (!commBusListener) {
		setStatus("CommBus listener not ready yet - try again in a moment.");
		return;
	}

	const messageInput = document.getElementById("dfs-message");
	const flagInput = document.getElementById("dfs-flag");

	const payload = {
		message: messageInput.value,
		flag: flagInput.checked,
	};

	commBusListener.callSimConnect(COMMBUS_HELLOJSON_EVENT, JSON.stringify(payload));
	setStatus(`Sent: ${JSON.stringify(payload)}`);
}

const REGISTER_POLL_INTERVAL_MS = 100;
const REGISTER_POLL_MAX_ATTEMPTS = 50; // 5 seconds

function registerWhenReady(attempt = 0) {
	if (typeof RegisterCommBusListener === "function") {
		commBusListener = RegisterCommBusListener(() => {
			setStatus("Ready.");
		});
		return;
	}
	if (attempt >= REGISTER_POLL_MAX_ATTEMPTS) {
		setStatus("Failed to load CommBus.js - RegisterCommBusListener never became available.");
		return;
	}
	setTimeout(() => registerWhenReady(attempt + 1), REGISTER_POLL_INTERVAL_MS);
}

class IngamePanelCommBusSender extends TemplateElement {
	constructor() {
		super(...arguments);
	}

	connectedCallback() {
		super.connectedCallback();

		registerWhenReady();
	}
}

window.customElements.define("ingamepanel-commbus-sender", IngamePanelCommBusSender);
checkAutoload();
