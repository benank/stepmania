#pragma once
#include "interactivity.h"
#include "http_client.h"
#include "websocket.h"
#include "rapidjson\document.h"
#include "rapidjson\pointer.h"
#include "interactive_types.h"
#include "interactive_event.h"
#include <map>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>

namespace mixer_internal
{

struct interactive_session_internal
{
	interactive_session_internal();

	// Configuration
	bool isReady;

	// State
	interactive_state state;
	std::string authorization;
	std::string versionId;
	std::string shareCode;
	bool shutdownRequested;
	void* callerContext;
	std::atomic<uint32_t> packetId;
	int sequenceId;
	long long serverTimeOffsetMs;
	bool serverTimeOffsetCalculated;

	// Server time offset
	std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds> getTimeSent;
	unsigned int getTimeRequestId;

	// Cached data
	std::shared_mutex scenesMutex;
	rapidjson::Document scenesRoot;
	bool scenesCached;
	scenes_by_id scenes;
	scenes_by_group scenesByGroup;
	bool groupsCached;
	controls_by_id controls;
	participants_by_id participants;

	// Event handlers
	on_input onInput;
	on_error onError;
	on_state_changed onStateChanged;
	on_participants_changed onParticipantsChanged;
	on_control_changed onControlChanged;
	on_transaction_complete onTransactionComplete;
	on_unhandled_method onUnhandledMethod;

	// Transactions that have been completed.
	std::map<std::string, interactive_error> completedTransactions;

	// Http
	std::unique_ptr<http_client> http;
	std::mutex httpMutex;

	// Websocket
	std::mutex websocketMutex;
	std::unique_ptr<websocket> ws;
	bool wsOpen;
	// Websocket handlers
	void handle_ws_open(const websocket& socket, const std::string& message);
	void handle_ws_message(const websocket& socket, const std::string& message);
	void handle_ws_close(const websocket& socket, unsigned short code, const std::string& message);

	// Outgoing data
	void run_outgoing_thread();
	std::thread outgoingThread;
	std::mutex outgoingMutex;
	std::condition_variable outgoingCV;
	std::queue<std::shared_ptr<interactive_event_internal>> outgoingEvents;
	void enqueue_outgoing_event(std::shared_ptr<interactive_event_internal>&& ev);

	// Incoming data
	void run_incoming_thread();
	std::thread incomingThread;
	std::mutex incomingMutex;
	interactive_event_queue incomingEvents;
	reply_handlers_by_id replyHandlersById;
	std::map<unsigned int, http_response_handler> httpResponseHandlers;
	void enqueue_incoming_event(std::shared_ptr<interactive_event_internal>&& ev);

	// Method handlers
	method_handlers_by_method methodHandlers;
};

typedef std::function<void(rapidjson::Document::AllocatorType& allocator, rapidjson::Value& value)> on_get_params;

// Common helper functions
int queue_method(interactive_session_internal& session, const std::string& method, on_get_params getParams, method_handler onReply, const bool handleImmediately = false);
int bootstrap(interactive_session_internal& session);

int cache_groups(interactive_session_internal& session);
int cache_scenes(interactive_session_internal& session);
int update_cached_control(interactive_session_internal& session, interactive_control& control, rapidjson::Value& controlJson);
int update_control_pointers(interactive_session_internal& session, const char* sceneId = nullptr);
void parse_participant(rapidjson::Value& participantJson, interactive_participant& participant);
void parse_control(rapidjson::Value& controlJson, interactive_control& control);

// Common reply handler that checks a reply for errors and calls the session's error handler if it exists.
int check_reply_errors(interactive_session_internal& session, rapidjson::Document& reply);

}

// Interactive RPC protocol strings.
#define RPC_ETAG                       "etag"
#define RPC_ID                         "id"
#define RPC_DISCARD                    "discard"
#define RPC_RESULT                     "result"
#define RPC_TYPE                       "type"
#define RPC_REPLY                      "reply"
#define RPC_METHOD                     "method"
#define RPC_PARAMS                     "params"
#define RPC_METADATA                   "meta"
#define RPC_ERROR                      "error"
#define RPC_ERROR_CODE                 "code"
#define RPC_ERROR_MESSAGE              "message"
#define RPC_ERROR_PATH                 "path"
#define RPC_DISABLED                   "disabled"
#define RPC_SEQUENCE				   "seq"

// RPC methods and replies
#define RPC_METHOD_HELLO               "hello"
#define RPC_METHOD_READY               "ready"   // equivalent to "start_interactive" or "goInteractive"
#define RPC_METHOD_ON_READY_CHANGED    "onReady" // called by server to both game and participant clients
#define RPC_METHOD_ON_GROUP_CREATE     "onGroupCreate"
#define RPC_METHOD_ON_GROUP_UPDATE     "onGroupUpdate"
#define RPC_METHOD_ON_CONTROL_CREATE   "onControlCreate"
#define RPC_METHOD_ON_CONTROL_UPDATE   "onControlUpdate" 
#define RPC_METHOD_ON_CONTROL_DELETE   "onControlDelete"
#define RPC_METHOD_GET_TIME            "getTime"
#define RPC_TIME                       "time"
#define RPC_PARAM_IS_READY             "isReady"
#define RPC_METHOD_SET_THROTTLE		   "setBandwidthThrottle"
#define RPC_PARAM_CAPACITY			   "capacity"
#define RPC_PARAM_DRAIN_RATE		   "drainRate"
#define RPC_METHOD_GIVE_INPUT          "giveInput"

#define RPC_GET_PARTICIPANTS           "getAllParticipants"
#define RPC_GET_PARTICIPANTS_BLOCK_SIZE 100 // returns up to 100 participants
#define RPC_METHOD_ON_PARTICIPANT_JOIN "onParticipantJoin"
#define RPC_METHOD_ON_PARTICIPANT_LEAVE "onParticipantLeave"
#define RPC_METHOD_ON_PARTICIPANT_UPDATE "onParticipantUpdate"
#define RPC_METHOD_PARTICIPANTS_ACTIVE "getActiveParticipants"
#define RPC_METHOD_PARTICIPANTS_UPDATE "updateParticipants"
#define RPC_PARAM_PARTICIPANTS         "participants"
#define RPC_PARAM_PARTICIPANT          "participant"
#define RPC_PARAM_PARTICIPANTS_ACTIVE_THRESHOLD "threshold" // unix milliseconds timestamp

#define RPC_METHOD_GET_SCENES          "getScenes"
#define RPC_METHOD_UPDATE_SCENES       "updateScenes"
#define RPC_PARAM_SCENES               "scenes"
#define RPC_METHOD_GET_GROUPS          "getGroups"
#define RPC_METHOD_UPDATE_GROUPS       "updateGroups" // updating the sceneID associated with the group is how you set the current scene
#define RPC_PARAM_GROUPS               "groups"
#define RPC_METHOD_CREATE_CONTROLS     "createControls"
#define RPC_METHOD_UPDATE_CONTROLS     "updateControls"
#define RPC_PARAM_CONTROLS             "controls"
#define RPC_METHOD_UPDATE_PARTICIPANTS "updateParticipants"

#define RPC_METHOD_ON_INPUT            "giveInput"
#define RPC_PARAM_INPUT                "input"
#define RPC_PARAM_CONTROL              "control"
#define RPC_PARAM_INPUT_EVENT          "event"
#define RPC_INPUT_EVENT_MOUSE_DOWN     "mousedown"
#define RPC_INPUT_EVENT_MOUSE_UP       "mouseup"
#define RPC_INPUT_EVENT_KEY_DOWN	   "keydown"
#define RPC_INPUT_EVENT_KEY_UP         "keyup"
#define RPC_INPUT_EVENT_MOVE           "move"
#define RPC_INPUT_EVENT_MOVE_X         "x"
#define RPC_INPUT_EVENT_MOVE_Y         "y"
#define RPC_PARAM_TRANSACTION          "transaction"
#define RPC_PARAM_TRANSACTION_ID       "transactionID"
#define RPC_SPARK_COST                 "cost"
#define RPC_METHOD_CAPTURE             "capture"

// Groups
#define RPC_METHOD_CREATE_GROUPS       "createGroups"
#define RPC_METHOD_UPDATE_GROUPS       "updateGroups"
#define RPC_GROUP_ID                   "groupID"
#define RPC_GROUP_DEFAULT              "default"

// Scenes
#define RPC_SCENE_ID                   "sceneID"
#define RPC_SCENE_DEFAULT              "default"
#define RPC_SCENE_CONTROLS             "controls"

// Controls
#define RPC_CONTROL_ID                 "controlID"
#define RPC_TRANSACTION_ID             "transactionID"
#define RPC_CONTROL_KIND               "kind"
#define RPC_CONTROL_BUTTON             "button"
#define RPC_CONTROL_BUTTON_TYPE        "button" // repeat, but for clarity - this corresponds to the javascript mouse event button types (0-4)
#define RPC_CONTROL_BUTTON_TEXT        "text"
#define RPC_CONTROL_BUTTON_PROGRESS    "progress"
#define RPC_CONTROL_BUTTON_COOLDOWN    "cooldown"
#define RPC_CONTROL_JOYSTICK           "joystick"
#define RPC_CONTROL_EVENT              "event"
#define RPC_CONTROL_POSITION		   "position"
#define RPC_CONTROL_POSITION_SIZE	   "size"
#define RPC_CONTROL_POSITION_WIDTH	   "width"
#define RPC_CONTROL_POSITION_HEIGHT	   "height"
#define RPC_CONTROL_POSITION_X		   "x"
#define RPC_CONTROL_POSITION_Y		   "y"
#define RPC_JOYSTICK_MOVE              "move"
#define RPC_JOYSTICK_X                 "x"
#define RPC_JOYSTICK_Y                 "y"
#define RPC_VALUE                      "value"

// Participants
#define RPC_SESSION_ID                 "sessionID"
#define RPC_USER_ID                    "userID"
#define RPC_PARTICIPANT_ID             "participantID"
#define RPC_XUID                       "xuid"
#define RPC_USERNAME                   "username"
#define RPC_LEVEL                      "level"
#define RPC_PART_LAST_INPUT            "lastInputAt"
#define RPC_PART_CONNECTED             "connectedAt"
#define RPC_GROUP_ID                   "groupID"
#define RPC_GROUP_DEFAULT              "default"