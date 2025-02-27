/****************************/
/*    NANOSAUR - MAIN 	*/
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <QD3D.h>
#include <QD3DErrors.h>
#include <QD3DMath.h>
#include "globals.h"
#include "mobjtypes.h"
#include "objects.h"
#include "windows_nano.h"
#include "main.h"
#include "misc.h"
#include "skeletonobj.h"
#include "skeletonanim.h"
#include "camera.h"
#include "player_control.h"
#include "sound2.h"
#include "3dmf.h"
#include "file.h"
#include 	"input.h"
#include 	"terrain.h"
#include 	"title.h"
#include "myguy.h"
#include "enemy.h"
#include "infobar.h"
#include "sprites.h"
#include "3dmath.h"
#include 	"selfrundemo.h"
#include "weapons.h"
#include "mainmenu.h"
#include "timeportal.h"
#include "items.h"
#include "movie.h"
#include "highscores.h"
#include "pickups.h"
#include "qd3d_geometry.h"

#include "GamePatches.h"

extern	Boolean			gAbortDemoFlag,gRestartSavedGame,gGameIsDemoFlag,gSongPlayingFlag;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	float			gFramesPerSecond,gFramesPerSecondFrac,gTimeRemaining,gMyHealth,gFuel;
extern	Byte		gMyCharacterType,gDemoMode;
extern	WindowPtr	gCoverWindow;
extern	TQ3Point3D	gCoord;
extern	long	gMyStartX,gMyStartZ;
extern	unsigned long 	gInfobarUpdateBits,gScore;
extern	ObjNode		*gPlayerObj;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void InitLevel(void);
static void CleanupLevel(void);
static void PlayLevel(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	KILL_DELAY	4


/****************************/
/*    VARIABLES             */
/****************************/


short		gMainAppRezFile;
Boolean		gGameOverFlag,gAbortedFlag;
Boolean		gPlayerGotKilledFlag,gWonGameFlag;

QD3DSetupOutputType		*gGameViewInfoPtr = nil;

PrefsType	gGamePrefs;

FSSpec		gDataSpec;


//======================================================================================
//======================================================================================
//======================================================================================


/*****************/
/* TOOLBOX INIT  */
/*****************/

void ToolBoxInit(void)
{
OSErr		iErr;

	gMainAppRezFile = CurResFile();


			/* MAKE FSSPEC FOR DATA FOLDER */

	iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Skeletons", &gDataSpec);
	if (iErr)
		DoFatalAlert("Cannot locate the Nanosaur Data folder.");



			/* LOAD QD3D & QT */
			
	QD3D_Boot();


			/* INIT SOME OF MY STUFF */
			
	InitTerrainManager();
	InitSkeletonManager();
	InitSoundTools();
	Init3DMFManager();	
		


			/* INIT MORE MY STUFF */
					
	InitObjectManager();
	InitSpriteManager();
	
	
			/* INIT PREFERENCES */
			
	memset(&gGamePrefs, 0, sizeof(gGamePrefs));
	snprintf(gGamePrefs.magic, sizeof(gGamePrefs.magic), "%s", PREFS_MAGIC);
	gGamePrefs.highQualityTextures = true;			// set the defaults
	gGamePrefs.canDoFog = true;
	gGamePrefs.shadows = true;
	gGamePrefs.dust = true;
	gGamePrefs.interpolationStyle = 1;
	gGamePrefs.allowGammaFade = false;
	gGamePrefs.fullscreen = true;
	gGamePrefs.vsync = true;
	gGamePrefs.mainMenuHelp = true;
	gGamePrefs.opaqueWater = false;
				
	LoadPrefs(&gGamePrefs);							// attempt to read from prefs file
	SetFullscreenMode();
					
}


/***************** INIT LEVEL ************************/

static void InitLevel(void)
{
QD3DSetupInputType		viewDef;
TQ3ColorRGB		c1 = { 1.0, 1, 1 };
TQ3ColorRGB		c2 = { 1, .9, .6 };

	PlaySong(0,true);

			/* MAKE VIEW */

	QD3D_NewViewDef(&viewDef, gCoverWindow);
	
#if TWO_MEG_VERSION
	viewDef.view.paneClip.left 		+= 144;  
	viewDef.view.paneClip.right 	+= 32;  
	viewDef.view.paneClip.top		+= 40;  
	viewDef.view.paneClip.bottom 	+= 140;  
	viewDef.lights.ambientBrightness = 0.3;
#else	
	viewDef.view.paneClip.left 		+= 118;  
	viewDef.view.paneClip.right 	+= 12;  
	viewDef.view.paneClip.top		+= 9;  
	viewDef.view.paneClip.bottom 	+= 110;  
	viewDef.lights.ambientBrightness = 0.2;
#endif	
	viewDef.camera.hither 			= HITHER_DISTANCE;
	viewDef.camera.yon 				= YON_DISTANCE;
	viewDef.camera.fov 				= 1.0;
	viewDef.lights.fillColor[0] 	= c1;
	viewDef.lights.fillColor[1] 	= c2;
	viewDef.lights.fillBrightness[0] = 1.2;
	viewDef.lights.fillBrightness[1] = .4;
	
	if (gGamePrefs.softerLighting) {
		viewDef.lights.ambientBrightness = 0.4;
		viewDef.lights.fillBrightness[0] = 0.8;
	}
	
	if (!gGamePrefs.canDoFog)		// if no fog possible, then bg is black
	{
		viewDef.view.clearColor.r = 0;
		viewDef.view.clearColor.g = 0;
		viewDef.view.clearColor.b = 0;
	}
	else							// set fog since I think it'll work
	{
		viewDef.view.clearColor.r = .95;
		viewDef.view.clearColor.g = .95;
		viewDef.view.clearColor.b = .75;
	}
	
	QD3D_SetupWindow(&viewDef, &gGameViewInfoPtr);
	QD3D_DrawScene(gGameViewInfoPtr, nil);


				/* INIT FLAGS */
				
	gAbortDemoFlag = gGameOverFlag = false;
	gPlayerGotKilledFlag = false;
	gWonGameFlag = false;
	gMyHealth = 1.0;

			/* LOAD ART */
			
	LoadLevelArt(LEVEL_NUM_0);			
	
	QD3D_InitParticles();	
	InitWeaponManager();
	InitItemsManager();
	InitMyInventory();	
	InitInfobar();

		
		/* INIT THE PLAYER */
			
	InitMyGuy();
	InitEnemyManager();	
					
			/* INIT CAMERA */
			
	MakeCameraEvent();
	
			/* PREP THE TERRAIN */
			
	PrimeInitialTerrain();
	InitTimePortals();
	
	StartAmbientEffect();
}


/**************** CLEANUP LEVEL **********************/

static void CleanupLevel(void)
{
	StopAllEffectChannels();
	StopDemo();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeTerrain();
	DisposeSpriteGroup(0);
	DeleteAll3DMFGroups();
	QD3D_DisposeWindowSetup(&gGameViewInfoPtr);
}


/**************** PLAY LEVEL ************************/

static void PlayLevel(void)
{
float killDelay = KILL_DELAY;						// time to wait after I'm dead before fading out
float	fps;
FSSpec	spec;

			/* INIT LEVEL */
			
	InitLevel();

	gGameOverFlag = false;

	MakeFadeEvent(true);
	QD3D_CalcFramesPerSecond();
		

		/******************/
		/* MAIN GAME LOOP */
		/******************/
	
	while(true)
	{
		fps = gFramesPerSecondFrac;
		
		ReadKeyboard();


				/* SEE IF DEMO ENDED */				
		
		if (gAbortDemoFlag)
			break;
	
	
				/* MOVE OBJECTS */
				
		CalcPlayerKeyControls();
		MoveObjects();
		QD3D_MoveParticles();

				/* SPECIFIC MAINTENANCE */
				
		UpdateLavaTextureAnimation();
		UpdateWaterTextureAnimation();
		DecAsteroidTimer();
	
	
				/* DRAW OBJECTS & TERRAIN */
					
		DoMyTerrainUpdate();
		UpdateInfobar();			
		RenderBackdropQuad(BACKDROP_FILL);
		QD3D_DrawScene(gGameViewInfoPtr,DrawTerrain);
		QD3D_CalcFramesPerSecond();

				
			/* SEE IF PAUSE GAME */
				
		if (gDemoMode != DEMO_MODE_RECORD)
		{
			if (GetNewKeyState(kKey_Pause))				// see if pause/abort
				DoPaused();
		}
		
			/* CHECK CHEAT KEYS */
			
		if (GetKeyState_Real(KEY_F15) || GetKeyState_Real(KEY_F12))
		{
			if (GetNewKeyState_Real(KEY_F1))				// get health
				GetHealth(1);
			else
			if (GetNewKeyState_Real(KEY_F2))				// get shield
				StartMyShield(gPlayerObj);
			else
			if (GetNewKeyState_Real(KEY_F3))				// get full weaponry
				GetCheatWeapons();
			else
			if (GetNewKeyState_Real(KEY_F4))				// get all eggs
				GetAllEggsCheat();
			else
			if (GetNewKeyState_Real(KEY_F5))				// get fuel
			{
				gFuel = MAX_FUEL_CAPACITY;
				gInfobarUpdateBits |= UPDATE_FUEL;
			}
				
		}

			/* SEE IF GAME ENDED */				
		
		if (gGameOverFlag)
			break;
			
			
			/* SEE IF GOT KILLED */
				
		if (gPlayerGotKilledFlag)				// if got killed, then hang around for a few seconds before resetting player
		{
			killDelay -= fps;					
			if (killDelay < 0.0f)				// see if time to reset player
			{
				ResetPlayer();					// reset player
				killDelay = KILL_DELAY;			// reset kill timer for next death
			}
		}	
	}

			/* CLEANUP */
	CleanupLevel();


			/* PLAY WIN MOVIE */
	
	if (gWonGameFlag)
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":movies:Win.mov", &spec);
		PlayAMovie(&spec);
		GammaFadeOut();
	}
	
			/* PLAY LOSE MOVIE */
	else
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":movies:Lose.mov", &spec);
		PlayAMovie(&spec);
		GammaFadeOut();
	}


			/* DO HIGH SCORES */
		
	if (!gSongPlayingFlag)						// make sure music is going
		PlaySong(1,true);		
		
	ShowHighScoresScreen(gScore);
}



/************************************************************/
/******************** PROGRAM MAIN ENTRY  *******************/
/************************************************************/


void GameMain(void)
{
unsigned long	someLong;

				/**************/
				/* BOOT STUFF */
				/**************/
				
	ToolBoxInit();
 	 		 	 		
 	InitInput();

	GetDateTime ((unsigned long *)(&someLong));		// init random seed
	SetMyRandomSeed(someLong);
#if 0
	HideCursor();
#endif
	
				/* DO CHARITY SCREEN */
			
	ShowCharity();

	while(true)
	{
		DoTitleScreen();
		DoMainMenu();
		PlayLevel();
	}
}



