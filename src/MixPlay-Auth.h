#pragma once

#include <string>
#include <iostream>
#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include <sstream>
#include "interactive/interactivity.h"

// OAuth and Stepmania Interactions MixPlay Game Client
#define CLIENT_ID "3f2b957b95cc2741bf90e7b7db8a28ba5cf07a3c06b404d3"
#define VERSION_ID "433588"
#define SHARE_CODE "e2wksr20"

class Auth
{
public:
	static int Authorize();
	static std::string GetAuthorization() { return authorization; };

private:
	static std::string authorization;
	static std::string token;

	static int ReAuthenticate();
	static void SaveToken(std::string token);
	static void LoadToken();
};

