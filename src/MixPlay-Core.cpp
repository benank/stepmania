#include "global.h"
#include "MixPlay-Core.h"

interactive_session MixPlayCore::session;
int MixPlayCore::err;
std::map<std::string, std::string> MixPlayCore::controlsByTransaction;

int MixPlayCore::Initialize()
{
	err = open_session();
	if (err) return err;

	err = set_handlers();
	if (err) return err;

	// Asynchronously connect to the user's interactive channel, using the interactive project specified by the version ID.
	err = interactive_connect(session, MixPlayAuth::GetAuthorization().c_str(), VERSION_ID, SHARE_CODE, false);
	if (err) return err;

	return err;
}

void MixPlayCore::Run()
{
	// Simulate game update loop. All previously registered session callbacks will be called from this thread.
	for (;;)
	{
		// This call processes any waiting messages from the interactive service. If there are no messages this returns immediately.
		err = interactive_run(session, 1);
		if (err) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	close_session();
}

int MixPlayCore::Destroy()
{
	return 0;
}

void MixPlayCore::handle_participants_changed(void * context, interactive_session session, interactive_participant_action action, const interactive_participant * participant)
{
#ifdef MIXER_DEBUG
	std::string participantName;
	int err = get_participant_name(session, participant->id, participantName);
	std::cout << "Participant " << participantName << " joined." << std::endl;
#endif

	std::string group = "default";

	if (GAMESTATE->m_iNumStagesOfThisSong == 0)
	{
#ifdef MIXER_DEBUG
		std::cout << "Num stages == 0, so in song select" << std::endl;
#endif
		group = "Song Select Group";
	}
	else if (GAMESTATE->m_iNumStagesOfThisSong > 0)
	{
#ifdef MIXER_DEBUG
		std::cout << "Num stages > 0, so we are ingame" << std::endl;
#endif
		group = "In-Game Group";
	}

	err = interactive_participant_set_group(session, participant->id, group.c_str());
	if (err)
	{
		std::cerr << "Failed to move " << participantName << " to the " << group << " group." << std::endl;
		return;
	}
	//GAMESTATE->GetCurrentGame()
	//	GAMESTATE->m_Position.m_fMusicSeconds

}

void MixPlayCore::handle_input(void * context, interactive_session session, const interactive_input * input)
{
	// Get the participant's Mixer name to give them attribution.
	std::string participantName;
	int err = get_participant_name(session, input->participantId, participantName);
	if (err)
	{
		std::cerr << "Failed to get participant user name (" << std::to_string(err) << ")" << std::endl;
		return;
	}

	// Now handle the input based on input type.
	if ((input->type == input_type_key || input->type == input_type_click) && input->buttonData.action == interactive_button_action_down)
	{
		/*if (0 != input->transactionIdLength)
		{
			// Capture the transaction on button down to deduct the viewer's sparks before executing any game functionality.
			controlsByTransaction[input->transactionId] = input->control.id;
			std::cout << participantName << " clicked '" << input->control.id << "'. Deducting sparks..." << std::endl;
			err = interactive_capture_transaction(session, input->transactionId);
			if (err)
			{
				std::cerr << "Failed to capture interactive transaction." << std::endl;
				return;
			}
		}
		else if (0 == strcmp("ToJoystickScene", input->control.id))
		{
			std::cout << "Moving " << participantName << " to the Joystick group." << std::endl;
			err = interactive_participant_set_group(session, input->participantId, "JoystickGroup");
			if (err)
			{
				std::cerr << "Failed to move " << participantName << " to the Joystick group." << std::endl;
				return;
			}
		}
		else if (0 == strcmp("ToDefaultScene", input->control.id))
		{
			std::cout << "Moving " << participantName << " to the default group." << std::endl;
			err = interactive_participant_set_group(session, input->participantId, "default");
			if (err)
			{
				std::cerr << "Failed to move " << participantName << " to the default group." << std::endl;
				return;
			}
		}*/
	}
	else if (input_type_custom == input->type && (0 == strcmp(input->control.id, "Song Request Input")))
	{
		// Handle text input.
		rapidjson::Document inputJson;
		if (inputJson.Parse(input->jsonData).HasParseError())
		{
			std::cerr << "Failed to parse input JSON data." << std::endl;
			return;
		}

		std::string textInput = inputJson["input"]["value"].GetString();
		std::cout << participantName << " input '" << textInput << "'." << std::endl;
		// Parse song requests and enqueue them!
	}
}

void MixPlayCore::handle_user(void * context, interactive_session session, const interactive_user * user)
{
	std::cout << "Connecting as: " << user->userName << std::endl;
	std::cout << "Avatar: " << user->avatarUrl << std::endl;
	std::cout << "Experience: " << user->experience << std::endl;
	std::cout << "Level: " << user->level << std::endl;
	std::cout << "Sparks: " << user->sparks << std::endl;
	std::cout << "Broadcasting: " << (user->isBroadcasting ? "true" : "false") << std::endl;
}

void MixPlayCore::handle_transaction_complete(void * context, interactive_session session, const char * transactionId, size_t transactionIdLength, unsigned int errorCode, const char * errorMessage, size_t errorMessageLength)
{
	if (!errorCode)
	{
		// Transaction was captured, now execute the most super awesome interactive functionality!
		std::string controlId = controlsByTransaction[std::string(transactionId, transactionIdLength)];
		if (0 == strcmp("GiveHealth", controlId.c_str()))
		{
			std::cout << "Giving health to the player!" << std::endl;
		}
	}
	else
	{
		std::cerr << errorMessage << "(" << std::to_string(errorCode) << ")" << std::endl;
	}

	controlsByTransaction.erase(transactionId);
}

void MixPlayCore::handle_error(void * context, interactive_session session, int errorCode, const char * errorMessage, size_t errorMessageLength)
{
	std::cerr << "Unexpected Mixer interactive error: " << errorMessage << std::endl;
}

int MixPlayCore::set_handlers()
{
	err = set_error_handler();
	if (err) return err;

	err = set_input_handler();
	if (err) return err;

	err = set_transaction_complete_handler();
	if (err) return err;

	err = set_state_changed_handler();
	if (err) return err;

	err = set_participants_changed_handler();
	if (err) return err;

	return 0;
}

int MixPlayCore::set_participants_changed_handler()
{
	return interactive_set_participants_changed_handler(session, handle_participants_changed);
}

int MixPlayCore::set_error_handler()
{
	return interactive_set_error_handler(session, handle_error);
}

int MixPlayCore::set_input_handler()
{
	return interactive_set_input_handler(session, handle_input);
}

int MixPlayCore::set_transaction_complete_handler()
{
	return interactive_set_transaction_complete_handler(session, handle_transaction_complete);
}

int MixPlayCore::set_state_changed_handler()
{
	return interactive_set_state_changed_handler(session, [](void* context, interactive_session session, interactive_state previousState, interactive_state currentState)
	{
		if (interactive_connecting == previousState && interactive_connected == currentState)
		{
			OnConnected();
		}
	});
}

// Called when this connects to the interactive module of the channel
void MixPlayCore::OnConnected()
{
	// Get the connected user's data.
	int err = interactive_get_user(session, handle_user);
	if (err)
	{
		puts(std::to_string(err).c_str());
		return;
	}

	// Create a group for participants to view the ingame scene.
	err = interactive_create_group(session, "In-Game Group", "In-Game");
	if (err)
	{
		puts(std::to_string(err).c_str());
		return;
	}

	// Create a group for participants to view the ingame scene.
	err = interactive_create_group(session, "Song Select Group", "Song Select");
	if (err)
	{
		puts(std::to_string(err).c_str());
		return;
	}

	// Now notify participants that interactive is ready.
	err = interactive_set_ready(session, true);
	if (err)
	{
		puts(std::to_string(err).c_str());
		return;
	}

}

int MixPlayCore::open_session()
{
	err = interactive_open_session(&session);
	return err;
}

int MixPlayCore::close_session()
{
	interactive_close_session(session);
	return 0;
}

int MixPlayCore::get_participant_name(interactive_session session, const char* participantId, std::string& participantName)
{
	// Get the participant's name.
	size_t participantNameLength = 0;

	// First call with a nullptr to get the required size for the user's name, MIXER_ERROR_BUFFER_SIZE is the expected return value.
	int err = interactive_participant_get_user_name(session, participantId, nullptr, &participantNameLength);
	if (MIXER_ERROR_BUFFER_SIZE != err)
	{
		return err;
	}

	// Resize the string to the correct size and call it again.
	participantName.resize(participantNameLength);
	err = interactive_participant_get_user_name(session, participantId, (char*)participantName.data(), &participantNameLength);
	// STL strings don't need a trailing null character.
	participantName = participantName.erase(participantNameLength - 1);

	return 0;
}
