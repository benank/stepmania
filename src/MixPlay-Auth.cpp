#include "MixPlay-Auth.h"

std::string Auth::authorization;
std::string Auth::token;

int Auth::Authorize()
{
#ifdef MIXER_DEBUG
	std::cout << "Authorizing...\n";
#endif

	LoadToken();

	int err = 0;

	if (token.empty())
	{
		err = ReAuthenticate();
		if (err) return err;
	}
	else
	{
		bool isStale;
		err = interactive_auth_is_token_stale(token.c_str(), &isStale);
		if (err) return err;

		if (isStale)
		{
			char newToken[1024];
			size_t newTokenLength = _countof(newToken);
			err = interactive_auth_refresh_token(CLIENT_ID, nullptr, token.c_str(), newToken, &newTokenLength);
			if (err) return err;
			token = std::string(newToken, newTokenLength);
		}
	}

	// Extract the authorization header from the refresh token.
	char authBuffer[1024];
	size_t authBufferLength = _countof(authBuffer);
	err = interactive_auth_parse_refresh_token(token.c_str(), authBuffer, &authBufferLength);
	if (err) return err;

	// Set the authorization out parameter.
	Auth::authorization = std::string(authBuffer, authBufferLength);

#ifdef MIXER_DEBUG
	std::cout << "Successfully authorized!\n";
#endif
	return 0;
}

int Auth::ReAuthenticate()
{
	int err = 0;
	char shortCode[7];
	size_t shortCodeLength = _countof(shortCode);
	char shortCodeHandle[1024];
	size_t shortCodeHandleLength = _countof(shortCodeHandle);

	// Get an OAuth short code from the user. For more information about OAuth see: https://oauth.net/2/
	err = interactive_auth_get_short_code(CLIENT_ID, nullptr, shortCode, &shortCodeLength, shortCodeHandle, &shortCodeHandleLength);
	if (err) return err;

	// Pop the browser for the user to approve access.
	std::string authUrl = std::string("https://www.mixer.com/go?code=") + shortCode;
	ShellExecuteA(0, 0, authUrl.c_str(), nullptr, nullptr, SW_SHOW);

	// Wait for OAuth token response.
	char refreshTokenBuffer[1024];
	size_t refreshTokenLength = _countof(refreshTokenBuffer);
	err = interactive_auth_wait_short_code(CLIENT_ID, nullptr, shortCodeHandle, refreshTokenBuffer, &refreshTokenLength);
	if (err)
	{
		if (MIXER_ERROR_TIMED_OUT == err)
		{
			std::cout << "Authorization timed out, user did not approve access within the time limit." << std::endl;
		}
		else if (MIXER_ERROR_AUTH_DENIED == err)
		{
			std::cout << "User denied access." << std::endl;
		}

		return err;
	}

	token = std::string(refreshTokenBuffer, refreshTokenLength);
	SaveToken(token);

	return 0;
}

void Auth::SaveToken(std::string token)
{
#ifdef MIXER_DEBUG
	std::cout << "Saving auth token...\n";
#endif

	std::ofstream file;
	file.open("token.dat", std::ofstream::out | std::ofstream::trunc | std::ofstream::in);
	file << token;
	file.close();

#ifdef MIXER_DEBUG
	std::cout << "Saved auth token!\n";
#endif
}

void Auth::LoadToken()
{
#ifdef MIXER_DEBUG
	std::cout << "Loading auth token...\n";
#endif

	std::ifstream t("token.dat");
	std::stringstream buffer;
	buffer << t.rdbuf();
	token = buffer.str();

#ifdef MIXER_DEBUG
	std::cout << "Loaded auth token!\n";
#endif
}
