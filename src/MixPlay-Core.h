#include "global.h"
#pragma once
#include "MixPlay-Globals.h"
#include "MixPlay-Auth.h"
#include "interactive/internal/rapidjson/Document.h"

#include "GameState.h"

#include <iostream>
#include <string>
#include <map>
#include <chrono>
#include <thread>

class MixPlayCore
{
public:
	static int Initialize();
	static void Run();
	static int Destroy();
	static interactive_session GetSession() { return session; };


private:
	static interactive_session session;
	static int err;
	static std::map<std::string, std::string> controlsByTransaction;

	static void handle_participants_changed(void* context, interactive_session session, interactive_participant_action action, const interactive_participant *participant);
	static void handle_input(void* context, interactive_session session, const interactive_input* input);
	static void handle_user(void* context, interactive_session session, const interactive_user* user);
	static void handle_transaction_complete(void* context, interactive_session session, const char* transactionId, size_t transactionIdLength, unsigned int errorCode, const char* errorMessage, size_t errorMessageLength);
	static void handle_error(void* context, interactive_session session, int errorCode, const char* errorMessage, size_t errorMessageLength);
	static int set_handlers();
	static int set_participants_changed_handler();
	static int set_error_handler();
	static int set_input_handler();
	static int set_transaction_complete_handler();
	static int set_state_changed_handler();
	static void OnConnected();
	static int open_session();
	static int close_session();
	static int get_participant_name(interactive_session session, const char* participantId, std::string& participantName);
};

