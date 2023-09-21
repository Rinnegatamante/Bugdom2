/****************************/
/*   	LEVELINTROS.C		*/
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "globals.h"
#include "misc.h"
#include "objects.h"
#include "windows.h"
#include "input.h"
#include "sound2.h"
#include	"file.h"
#include	"ogl_support.h"
#include	"main.h"
#include "3dmath.h"
#include "miscscreens.h"
#include "sobjtypes.h"
#include "sprites.h"
#include "effects.h"
#include "bg3d.h"
#include "mobjtypes.h"
#include "sparkle.h"
#include "player.h"
#include <aglmacro.h>
#include "infobar.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond,gGammaFadePercent;
extern	FSSpec		gDataSpec;
extern	Boolean		gNetGameInProgress,gGameOver;
extern	KeyMap gKeyMap,gNewKeys;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean		gSongPlayingFlag,gResetSong,gDisableAnimSounds;
extern	PrefsType	gGamePrefs;
extern	OGLPoint3D	gCoord;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	SparkleType	gSparkles[];
extern	int			gLevelNum;
extern	AGLContext		gAGLContext;
extern	u_long				gGlobalMaterialFlags;
extern	float				gGlobalTransparency;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawIntroCallback(OGLSetupOutputType *info);
static void SetupIntroScreen(void);
static void FreeIntroScreen(void);
static void CreateIntroSaucers(void);
static void MoveIntroSaucer(ObjNode *topObj);
static void MovePlanet(ObjNode *theNode);
static void MoveStar(ObjNode *theNode);
static void CreateIntroSaucer2(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	SAUCER_SCALE	1.0f

#define	ICESAUCER_SCALE	1.8f


/******************* LEVELINTRO *************************/

enum
{
	INTRO_ObjType_EnemySaucer_Top,
	INTRO_ObjType_IceSaucer,

	INTRO_ObjType_Star,
	INTRO_ObjType_PlanetGlow,

	INTRO_ObjType_Earth,
	INTRO_ObjType_EarthClouds,

	INTRO_ObjType_Blob,
	INTRO_ObjType_BlobClouds,

	INTRO_ObjType_Apoc,
	INTRO_ObjType_ApocClouds,

	INTRO_ObjType_Cloud,
	INTRO_ObjType_CloudClouds,

	INTRO_ObjType_Jungle,
	INTRO_ObjType_JungleClouds,

	INTRO_ObjType_Fire,
	INTRO_ObjType_FireClouds,

	INTRO_ObjType_Saucer,
	INTRO_ObjType_SaucerClouds,

	INTRO_ObjType_X,
	INTRO_ObjType_XClouds
};


enum
{
	LEVELINTRO_SObjType_FarmText,
	LEVELINTRO_SObjType_SlimeText,
	LEVELINTRO_SObjType_ApocalypseText,
	LEVELINTRO_SObjType_CloudText,
	LEVELINTRO_SObjType_JungleText,
	LEVELINTRO_SObjType_FireIceText,
	LEVELINTRO_SObjType_SaucerText,
	LEVELINTRO_SObjType_BrainBossText

};



/*********************/
/*    VARIABLES      */
/*********************/


static float	gIntroTimer,gLevelNameTransparency;


/******************* DO LEVEL INTRO **************************/

void DoLevelIntro(void)
{
float	oldTime,maxTime = 11.0f;

			/**************/
			/* PLAY MUSIC */
			/**************/

	switch(gLevelNum)
	{
		case	LEVEL_NUM_TEST:
				PlaySong(SONG_FARM, true);
				break;

		case	LEVEL_NUM_BLOB:
				PlaySong(SONG_SLIME, true);
				break;

		case	LEVEL_NUM_BLOBBOSS:
				PlaySong(SONG_SLIMEBOSS, true);
				return;										// this level doesn't have an intro, so bail now.

		case	LEVEL_NUM_CLOUD:
				PlaySong(SONG_CLOUD, true);
				break;

		case	LEVEL_NUM_JUNGLE:
				PlaySong(SONG_JUNGLE, true);
				break;

		case	LEVEL_NUM_JUNGLEBOSS:
				PlaySong(SONG_JUNGLEBOSS, true);
				return;										// this level doesn't have an intro, so bail now.
				break;

		case	LEVEL_NUM_FIREICE:
				PlaySong(SONG_FIREICE, true);
				break;

		case	LEVEL_NUM_SAUCER:
				PlaySong(SONG_SAUCER, true);
				break;

		case	LEVEL_NUM_BRAINBOSS:
				PlaySong(SONG_BRAINBOSS, true);
				maxTime = 5.0f;
				break;

		default:
				PlaySong(SONG_APOCALYPSE, true);
	}


	gIntroTimer = 0;
	gLevelNameTransparency = 0;

			/* SETUP */

	SetupIntroScreen();
	MakeFadeEvent(true);

				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	ReadKeyboard();

	while(true)
	{
			/* DRAW STUFF */

		CalcFramesPerSecond();
		ReadKeyboard();
		MoveObjects();
		OGL_DrawScene(gGameViewInfoPtr, DrawIntroCallback);

		if (AreAnyNewKeysPressed())
			break;


				/* CHECK TIMER */

		oldTime = gIntroTimer;
		gIntroTimer += gFramesPerSecondFrac;
		if ((gIntroTimer > maxTime) && (oldTime <= maxTime))
		{
#if ALLOW_FADE
			MakeFadeEvent(false);
#else
			break;
#endif
		}

		if (gGammaFadePercent <= 0.0f)
			break;
	}


			/* CLEANUP */

	GammaFadeOut();
	FreeIntroScreen();
}


/***************** DRAW INTRO CALLBACK *******************/

static void DrawIntroCallback(OGLSetupOutputType *info)
{
AGLContext agl_ctx = gAGLContext;
const short textSprites[] =
{
	LEVELINTRO_SObjType_FarmText,
	LEVELINTRO_SObjType_SlimeText,
	0,
	LEVELINTRO_SObjType_ApocalypseText,
	LEVELINTRO_SObjType_CloudText,
	LEVELINTRO_SObjType_JungleText,
	0,
	LEVELINTRO_SObjType_FireIceText,
	LEVELINTRO_SObjType_SaucerText,
	LEVELINTRO_SObjType_BrainBossText,
};


			/* DRAW OBJECTS */

	DrawObjects(info);
	DrawSparkles(info);


		/************************/
		/* DRAW SPRITE OVERLAYS */
		/************************/

	OGL_PushState();

	if (info->useFog)
		glDisable(GL_FOG);

	SetInfobarSpriteState();


			/* DRAW LEVEL TEXT */

	if (gLevelNum == LEVEL_NUM_BRAINBOSS)							// comes in sooner on Brain Boss level
	{
		if (gIntroTimer > 1.0f)
		{
			gLevelNameTransparency += gFramesPerSecondFrac *.6f;
			if (gLevelNameTransparency > 1.0f)
				gLevelNameTransparency = 1.0f;
		}
	}
	else
	{
		if (gIntroTimer > 3.0f)
		{
			gLevelNameTransparency += gFramesPerSecondFrac *.6f;
			if (gLevelNameTransparency > 1.0f)
				gLevelNameTransparency = 1.0f;
		}
	}

	gGlobalTransparency = gLevelNameTransparency;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);								// make glow
	DrawInfobarSprite(0,440, 300, textSprites[gLevelNum], info);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gGlobalTransparency = 1.0;

			/***********/
			/* CLEANUP */
			/***********/

	OGL_PopState();
	gGlobalMaterialFlags = 0;

}


/********************* SETUP INTRO SCREEN **********************/

static void SetupIntroScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
ObjNode	*planet,*glow,*newObj,*clouds;
int		i;

const OGLColorRGBA glowColor[] =
{
	.7,.7,1,.99,						// earth
	.7,.3,1,.99,						// slime
	.7,.7,1,.99,						// slime boss
	.7,.7,.7,.8,						// apoc
	.7,.2,1,.99,						// cloud
	.9,.7,.6,.99,						// jungle
	.7,.7,1,.99,						// jungle boss
	.7,.7,.1,.99,						// fire & ice
	.5,.7,1,.99,						// saucer
	1,1,.9,.99,							// brain boss
};

static OGLColorRGBA			ambientColor = { .03, .03, .03, 1 };

const Byte	terra[] =
{
	INTRO_ObjType_Earth,				// earth
	INTRO_ObjType_Blob,					// slime
	INTRO_ObjType_Blob,					// slime boss
	INTRO_ObjType_Apoc,					// apoc
	INTRO_ObjType_Cloud,				// cloud
	INTRO_ObjType_Jungle,				// jungle
	INTRO_ObjType_Jungle,				// jungle boss
	INTRO_ObjType_Fire,					// fire & ice
	INTRO_ObjType_Saucer,				// saucer
	INTRO_ObjType_X,					// brain boss
};

const Byte	cloud[] =
{
	INTRO_ObjType_EarthClouds,			// earth
	INTRO_ObjType_BlobClouds,			// slime
	INTRO_ObjType_BlobClouds,			// slime boss
	INTRO_ObjType_ApocClouds,			// apoc
	INTRO_ObjType_CloudClouds,			// cloud
	INTRO_ObjType_JungleClouds,			// jungle
	INTRO_ObjType_JungleClouds,			// jungle boss
	INTRO_ObjType_FireClouds,			// fire & ice
	INTRO_ObjType_SaucerClouds,			// saucer
	INTRO_ObjType_XClouds,				// brain boss
};



			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.lights.ambientColor 	= ambientColor;
	viewDef.lights.fillDirection[0].x *= -1.0f;

	viewDef.camera.fov 		= 1.4;
	viewDef.camera.hither 	= 400;
	viewDef.camera.yon 		= 15000;

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/


			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:sprites:spheremap.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS, gGameViewInfoPtr);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:sprites:levelIntro.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_LEVELINTRO, gGameViewInfoPtr);
	BlendASprite(SPRITE_GROUP_LEVELINTRO, LEVELINTRO_SObjType_FarmText);

			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:models:LevelIntro.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELINTRO, gGameViewInfoPtr);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELINTRO, INTRO_ObjType_IceSaucer,
								 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sea);


	InitSparkles();
	InitParticleSystem(gGameViewInfoPtr);


			/**************/
			/* MAKE STARS */
			/**************/

	for (i = 0; i < 170; i++)
	{
		OGLMatrix4x4	m;
		OGLPoint3D		p;
		static OGLColorRGBA colors[] =
		{
			1,1,1,1,			// white
			1,.6,.6,1,			// red
			.6,.6,1,1,			// blue
			.7,.7,8,1,			// grey

		};

		OGLMatrix4x4_SetRotateAboutPoint(&m, &viewDef.camera.from, RandomFloat()*PI2,RandomFloat()*PI2,RandomFloat()*PI2);
		p.x = p.y =0;
		p.z = viewDef.camera.yon * .9f;
		OGLPoint3D_Transform(&p,&m,&gNewObjectDefinition.coord);

		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;
		gNewObjectDefinition.type 		= INTRO_ObjType_Star;
		gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES | STATUS_BIT_GLOW | STATUS_BIT_NOTEXTUREWRAP |
										STATUS_BIT_NOZWRITES | STATUS_BIT_DONTCULL | STATUS_BIT_NOLIGHTING | STATUS_BIT_AIMATCAMERA;
		gNewObjectDefinition.slot 		= 200;
		gNewObjectDefinition.moveCall 	= MoveStar;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 	    = 4.0f + RandomFloat() * 3.0f;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->SpecialF[0] = RandomFloat() * PI;

		newObj->ColorFilter = colors[MyRandomLong()&0x3];

	}


			/***************/
			/* MAKE PLANET */
			/***************/

				/* EARTH SPHERE */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;
	gNewObjectDefinition.type 		= terra[gLevelNum];
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= -viewDef.camera.yon * .8f;
	gNewObjectDefinition.flags 		= STATUS_BIT_ROTYZX | STATUS_BIT_DONTCULL | STATUS_BIT_UVTRANSFORM;
	gNewObjectDefinition.slot 		= 50;
	gNewObjectDefinition.moveCall 	= MovePlanet;
	gNewObjectDefinition.rot 		= PI2/3;
	gNewObjectDefinition.scale 	    = 60;
	planet = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* EARTH CLOUDS */

	gNewObjectDefinition.type 		= cloud[gLevelNum];
	gNewObjectDefinition.flags 		= STATUS_BIT_ROTYZX | STATUS_BIT_DONTCULL | STATUS_BIT_UVTRANSFORM |
									STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOZWRITES;
	if (gLevelNum == LEVEL_NUM_APOCALYPSE)
		gNewObjectDefinition.flags |= STATUS_BIT_GLOW;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	clouds = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	clouds->ColorFilter.a = .95;

			/* GLOW */

	gNewObjectDefinition.type 		= INTRO_ObjType_PlanetGlow;
	gNewObjectDefinition.flags 		= STATUS_BIT_ROTYZX | STATUS_BIT_DONTCULL | STATUS_BIT_UVTRANSFORM |
									STATUS_BIT_GLOW | STATUS_BIT_NOZWRITES;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.scale 	    *= 1.3f;
	glow = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	glow->ColorFilter = glowColor[gLevelNum];

	planet->ChainNode = glow;

	glow->ChainNode = clouds;


			/**************/
			/* MAKE SHIPS */
			/**************/

	switch(gLevelNum)
	{
		case	LEVEL_NUM_SAUCER:								// show ice saucer
				CreateIntroSaucer2();
				break;

		case	LEVEL_NUM_BRAINBOSS:							// no ships
				break;

		default:
				CreateIntroSaucers();

	}
}

/********************** CREATE INTRO SAUCERS ***********************/

static void CreateIntroSaucers(void)
{
ObjNode	*top;
int		n,r,c;
float	x,z;

	n = 0;
	z = 1000;
	for (r = 0; r < 4; r++)
	{
		x = 130.0f + r / 2.0f * -900.0f;

		for (c = 0; c < (r+1); c++)
		{
			gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;
			gNewObjectDefinition.type 		= INTRO_ObjType_EnemySaucer_Top;
			gNewObjectDefinition.coord.x 	= x + RandomFloat2() * 100.0f;
			gNewObjectDefinition.coord.y 	= -2000;
			gNewObjectDefinition.coord.z 	= z + RandomFloat2() * 100.0f;
			gNewObjectDefinition.flags 		= 0;
			gNewObjectDefinition.slot 		= 100;
			gNewObjectDefinition.moveCall 	= MoveIntroSaucer;
			gNewObjectDefinition.rot 		= RandomFloat()*PI2;
			gNewObjectDefinition.scale 		= SAUCER_SCALE;
			top = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			top->SpecialF[0] = RandomFloat2() * PI2;

			top->Kind = n++;

			x += 900.0f;
		}

		z += 900.0f;
	}
}


/********************** CREATE INTRO SAUCER 2 ***********************/

static void CreateIntroSaucer2(void)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;
	gNewObjectDefinition.type 		= INTRO_ObjType_IceSaucer;
	gNewObjectDefinition.coord.x 	= 130;
	gNewObjectDefinition.coord.y 	= -2000;
	gNewObjectDefinition.coord.z 	= 1000;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveIntroSaucer;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= ICESAUCER_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
}




/********************** FREE INTRO ART **********************/

static void FreeIntroScreen(void)
{
	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	DeleteAllObjects();
	DisposeParticleSystem();
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
}

#pragma mark -


/************ MOVE INTRO SAUCER ************************/

static void MoveIntroSaucer(ObjNode *topObj)
{
float	fps = gFramesPerSecondFrac;
float	r;

	GetObjectInfo(topObj);

			/* MAKE WOBBLE */

	topObj->SpecialF[0] += fps * 3.0f;
	r = sin(topObj->SpecialF[0]) * .1f;
	topObj->Rot.z = r;

			/* MOVE IT */

	gCoord.z -= 800.0f * fps;

			/* MAKE IT SPIN */

	topObj->Rot.y -= fps * 3.0f;


	UpdateObject(topObj);


			/* UPDATE CAMERA */

	if (topObj->Kind == 0)
	{
		OGL_UpdateCameraFromTo(gGameViewInfoPtr, &gGameViewInfoPtr->cameraPlacement.cameraLocation, &gCoord);

	}


			/* UPDATE AUDIO */

	if (topObj->EffectChannel != -1)
	{
		Update3DSoundChannel(EFFECT_SAUCER, &topObj->EffectChannel, &topObj->Coord);
	}
	else
		topObj->EffectChannel = PlayEffect_Parms3D(EFFECT_SAUCER,  &topObj->Coord, NORMAL_CHANNEL_RATE + (MyRandomLong()&0x1ff), .2);
}




/********************* MOVE PLANET ******************************/

static void MovePlanet(ObjNode *theNode)
{
ObjNode	*glow = theNode->ChainNode;
ObjNode	*clouds = glow->ChainNode;

	theNode->TextureTransformU -= gFramesPerSecondFrac * .01f;

	if (clouds)
		clouds->TextureTransformU -= gFramesPerSecondFrac * .015f;

}


/************************ MOVE STAR *************************/

static void MoveStar(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->SpecialF[0] += fps * 2.0f;

	theNode->ColorFilter.a = .3f + (sin(theNode->SpecialF[0]) + 1.0f) * .5f;

}














