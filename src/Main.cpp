#include "Pomme.h"
#include "PommeInit.h"
#include "PommeFiles.h"
#include "PommeGraphics.h"
#include "GamePatches.h"
#include "version.h"

#include <Quesa.h>
#include <SDL.h>

#include <iostream>
#include <cstring>

#if __APPLE__
#include <libproc.h>
#include <unistd.h>
#endif

extern "C"
{
	// bare minimum from Windows.c to satisfy externs in game code
	WindowPtr gCoverWindow = nullptr;
	UInt32* gCoverWindowPixPtr = nullptr;

	// Lets the game know where to find its asset files
	extern FSSpec gDataSpec;

	extern SDL_Window* gSDLWindow;

	extern int PRO_MODE;

	// Tell Windows graphics driver that we prefer running on a dedicated GPU if available
#if _WIN32
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
#endif

	void GameMain(void);
}

static void FindGameData()
{
	fs::path dataPath;

#if __APPLE__
	char pathbuf[PROC_PIDPATHINFO_MAXSIZE];

	pid_t pid = getpid();
	int ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));
	if (ret <= 0)
	{
		throw std::runtime_error(std::string(__func__) + ": proc_pidpath failed: " + std::string(strerror(errno)));
	}

	dataPath = pathbuf;
	dataPath = dataPath.parent_path().parent_path() / "Resources";
#else
	dataPath = "Data";
#endif

	dataPath = dataPath.lexically_normal();

	// Set data spec
	gDataSpec = Pomme::Files::HostPathToFSSpec(dataPath / "Skeletons");

	// Use application resource file
	auto applicationSpec = Pomme::Files::HostPathToFSSpec(dataPath / "Application");
	short resFileRefNum = FSpOpenResFile(&applicationSpec, fsRdPerm);
	UseResFile(resFileRefNum);
}

static bool AskProMode()
{
	const SDL_MessageBoxButtonData buttons[] =
	{
		{ /* .flags, .buttonid, .text */        0, 0, (const char*)u8"Extreme!" },
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, (const char*)u8"Normal" },
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, (const char*)u8"Quit" },
	};

	const SDL_MessageBoxData messageboxdata =
	{
		SDL_MESSAGEBOX_INFORMATION,		// .flags
		gSDLWindow,						// .window
		(const char*)u8"Select Nanosaur difficulty",	// .title
		(const char*)u8"Which version of Nanosaur would you like to play?", // .message
		SDL_arraysize(buttons),			// .numbuttons
		buttons,						// .buttons
		nullptr							// .colorScheme
	};

	int buttonid;
	if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0)
	{
		throw Pomme::QuitRequest();
	}

	switch (buttonid) {
		case 2: // quit
			throw Pomme::QuitRequest();
		case 0: // extreme
			return true;
		default: // normal
			return false;
	}
}

static const char* GetWindowTitle()
{
	static char windowTitle[256];
	snprintf(windowTitle, sizeof(windowTitle), "Nanosaur%s %s", PRO_MODE ? " Extreme" : "", PROJECT_VERSION);
	return windowTitle;
}

int CommonMain(int argc, const char** argv)
{
	Pomme::InitParams initParams =
	{
		.windowName = "Nanosaur",
		.windowWidth = 640,
		.windowHeight = 480,
		.msaaSamples = 0
	};

	// Start our "machine"
	Pomme::Init(initParams);

	// Set up globals that the game expects
	gCoverWindow = Pomme::Graphics::GetScreenPort();
	gCoverWindowPixPtr = (UInt32*) GetPixBaseAddr(GetGWorldPixMap(gCoverWindow));

	// Clear window
	ExclusiveOpenGLMode_Begin();
	ClearBackdrop(0xFFA5A5A5);
	RenderBackdropQuad(BACKDROP_FILL);
	ExclusiveOpenGLMode_End();

	FindGameData();
#if !(__APPLE__)
	Pomme::Graphics::SetWindowIconFromIcl8Resource(128);
#endif

	// Initialize Quesa
	auto qd3dStatus = Q3Initialize();
	if (qd3dStatus != kQ3Success)
	{
		throw std::runtime_error("Couldn't initialize Quesa.");
	}

	// Start the game
	try
	{
		SetProModeSettings(AskProMode());
		SDL_SetWindowTitle(gSDLWindow, GetWindowTitle());
		GameMain();
	}
	catch (Pomme::QuitRequest&)
	{
		// no-op, the game may throw this exception to shut us down cleanly
	}

	// Clean up
	Pomme::Shutdown();

	return 0;
}

int main(int argc, char** argv)
{
	std::string uncaught;

	try
	{
		return CommonMain(argc, const_cast<const char**>(argv));
	}
	catch (std::exception& ex)
	{
		uncaught = ex.what();
	}
	catch (...)
	{
		uncaught = "unknown";
	}

	std::cerr << "Uncaught exception: " << uncaught << "\n";
	SDL_ShowSimpleMessageBox(0, "Uncaught Exception", uncaught.c_str(), nullptr);
	return 1;
}
