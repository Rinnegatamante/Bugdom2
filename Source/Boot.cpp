// BUGDOM 2 ENTRY POINT
// (C) 2023 Iliyas Jorio
// This file is part of Bugdom 2. https://github.com/jorio/bugdom2

#include <SDL.h>
#include "Pomme.h"
#include "PommeInit.h"
#include "PommeFiles.h"

#include <iostream>
#include <cstring>

#ifdef __vita__
#include <unistd.h> 
#include <vitasdk.h>
extern "C" {
int _newlib_heap_size_user = 256 * 1024 * 1024;
void vglInitExtended(int legacy_pool_size, int width, int height, int ram_threshold, SceGxmMultisampleMode msaa);
};
#endif

extern "C"
{
	#include "game.h"

	SDL_Window* gSDLWindow = nullptr;
	FSSpec gDataSpec;
	int gCurrentAntialiasingLevel;

/*
#if _WIN32
	// Tell Windows graphics driver that we prefer running on a dedicated GPU if available
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
#endif
*/
}

static fs::path FindGameData(const char* executablePath)
{
	fs::path dataPath;

	int attemptNum = 0;

#if !(__APPLE__)
	attemptNum++;		// skip macOS special case #0
#endif

	if (!executablePath)
		attemptNum = 2;

tryAgain:
	switch (attemptNum)
	{
		case 0:			// special case for macOS app bundles
			dataPath = executablePath;
			dataPath = dataPath.parent_path().parent_path() / "Resources";
			break;

		case 1:
			dataPath = executablePath;
			dataPath = dataPath.parent_path() / "Data";
			break;

		case 2:
#if defined(__vita__)
			dataPath = "ux0:data/Bugdom2/Data";
#else
			dataPath = "Data";
#endif
			break;

		default:
			throw std::runtime_error("Couldn't find the Data folder.");
	}

	attemptNum++;

	dataPath = dataPath.lexically_normal();

	// Set data spec -- Lets the game know where to find its asset files
	gDataSpec = Pomme::Files::HostPathToFSSpec(dataPath / "Skeletons");

	FSSpec someDataFileSpec;
	OSErr iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons:Grasshopper.bg3d", &someDataFileSpec);
	if (iErr)
	{
		goto tryAgain;
	}

	return dataPath;
}

#ifdef __vita__
void recursive_cpdir(const char *src, const char *dst) {
	SceIoDirent g_dir;
	int fd = sceIoDopen(src);
	char src_path[512], dst_path[512];
	while (sceIoDread(fd, &g_dir) > 0) {
		sprintf(src_path, "%s/%s", src, g_dir.d_name);
		sprintf(dst_path, "%s/%s", dst, g_dir.d_name);
		if (SCE_S_ISDIR(g_dir.d_stat.st_mode)) {
			sceIoMkdir(dst_path, 0777);
			recursive_cpdir(src_path, dst_path);
		} else {
			FILE *f = fopen(src_path, "rb");
			void *buf = malloc(g_dir.d_stat.st_size);
			fread(buf, 1, g_dir.d_stat.st_size, f);
			fclose(f);
			f = fopen(dst_path, "wb");
			fwrite(buf, 1, g_dir.d_stat.st_size, f);
			fclose(f);
			free(buf);
		}
	}
	sceIoDclose(fd);
}
#endif

static void Boot(int argc, char** argv)
{
#ifdef __vita__
	// Install shader cache to avoid a few stutters first time game is played
	FILE *f = fopen("ux0:data/Bugdom2/shaders.bin", "rb");
	if (!f) {
		sceIoMkdir("ux0:data/shader_cache", 0777);
		recursive_cpdir("app0:shaders", "ux0:data/shader_cache");
		f = fopen("ux0:data/Bugdom2/shaders.bin", "wb");
	}
	fclose(f);
#endif	
	
	const char* executablePath = argc > 0 ? argv[0] : NULL;

	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);

	// Start our "machine"
	Pomme::Init();

	// Load game prefs before starting
	LoadPrefs();

retryVideo:
	// Initialize SDL video subsystem
	if (0 != SDL_Init(SDL_INIT_VIDEO))
	{
		throw std::runtime_error("Couldn't initialize SDL video subsystem.");
	}

	// Create window
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	gCurrentAntialiasingLevel = gGamePrefs.antialiasingLevel;
	if (gCurrentAntialiasingLevel != 0)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 1 << gCurrentAntialiasingLevel);
	}

	// Determine display
	int display = gGamePrefs.monitorNum;
	if (display >= SDL_GetNumVideoDisplays())
	{
		display = 0;
	}

	// Determine initial window size
	int initialWidth = 640;
	int initialHeight = 480;
	GetDefaultWindowSize(display, &initialWidth, &initialHeight);

	gSDLWindow = SDL_CreateWindow(
			PROJECT_FULL_NAME " (" PROJECT_VERSION ")",
			SDL_WINDOWPOS_UNDEFINED_DISPLAY(display),
			SDL_WINDOWPOS_UNDEFINED_DISPLAY(display),
			initialWidth,
			initialHeight,
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

	if (!gSDLWindow)
	{
		if (gCurrentAntialiasingLevel != 0)
		{
			printf("Couldn't create SDL window with the requested MSAA level. Retrying without MSAA...\n");

			// retry without MSAA
			gGamePrefs.antialiasingLevel = 0;
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
			goto retryVideo;
		}
		else
		{
			throw std::runtime_error("Couldn't create SDL window.");
		}
	}

	// Find path to game data folder
	fs::path dataPath = FindGameData(executablePath);

	// Init joystick subsystem
	{
		SDL_Init(SDL_INIT_GAMECONTROLLER);
		auto gamecontrollerdbPath8 = (dataPath / "System" / "gamecontrollerdb.txt").u8string();
		if (-1 == SDL_GameControllerAddMappingsFromFile((const char*)gamecontrollerdbPath8.c_str()))
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, PROJECT_FULL_NAME, "Couldn't load gamecontrollerdb.txt!", gSDLWindow);
		}
	}

	// Set fullscreen mode from prefs
	SetFullscreenMode(true);
}

static void Shutdown()
{
	SetMacLinearMouse(false);

	Pomme::Shutdown();

	if (gSDLWindow)
	{
		SDL_DestroyWindow(gSDLWindow);
		gSDLWindow = NULL;
	}

	SDL_Quit();
}

int main(int argc, char** argv)
{
	int				returnCode				= 0;
	std::string		finalErrorMessage		= "";
	bool			showFinalErrorMessage	= false;

#ifdef __vita__
	sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});
	SceCommonDialogConfigParam cmnDlgCfgParam;
	sceCommonDialogConfigParamInit(&cmnDlgCfgParam);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, (int *)&cmnDlgCfgParam.language);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, (int *)&cmnDlgCfgParam.enterButtonAssign);
	sceCommonDialogSetConfigParam(&cmnDlgCfgParam);
	
	SDL_setenv("VITA_DISABLE_TOUCH_BACK", "1", 1);
	scePowerSetArmClockFrequency(444);
	scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);
	scePowerSetGpuXbarClockFrequency(166);
	vglInitExtended(2 * 1024 * 1024, 960, 544, 2 * 1024 * 1024, SCE_GXM_MULTISAMPLE_4X);
	chdir("ux0:data");
#endif

	try
	{
		Boot(argc, argv);
		GameMain();
	}
	catch (Pomme::QuitRequest&)
	{
		// no-op, the game may throw this exception to shut us down cleanly
	}
#if !(_DEBUG)
	// In release builds, catch anything that might be thrown by GameMain
	// so we can show an error dialog to the user.
	catch (std::exception& ex)		// Last-resort catch
	{
		returnCode = 1;
		finalErrorMessage = ex.what();
		showFinalErrorMessage = true;
	}
	catch (...)						// Last-resort catch
	{
		returnCode = 1;
		finalErrorMessage = "unknown";
		showFinalErrorMessage = true;
	}
#endif

	Shutdown();

	if (showFinalErrorMessage)
	{
		std::cerr << "Uncaught exception: " << finalErrorMessage << "\n";
		SDL_ShowSimpleMessageBox(0, PROJECT_FULL_NAME, finalErrorMessage.c_str(), nullptr);
	}

	return returnCode;
}
