/****************************/
/*        FADE              */
/* (c)2022 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include	"game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveFadePane(ObjNode *theNode);
static void DrawFadePane(ObjNode *theNode);

#define FaderFrameCounter	Special[0]


/****************************/
/*    CONSTANTS             */
/****************************/

#define	FADEPANE_SLOT	(SLOT_OF_DUMB + 4000)

enum
{
	kFaderMode_FadeOut,
	kFaderMode_FadeIn,
	kFaderMode_Done,
};

/**********************/
/*     VARIABLES      */
/**********************/

float			gGammaFadeFrac = 1.0f;



/***************** FREEZE-FRAME FADE OUT ********************/

void OGL_FadeOutScene(void (*drawCall)(void), void (*moveCall)(void))
{
#if SKIPFLUFF
	gGammaFadeFrac = 0;
	return;
#endif

#if 0
	if (gDebugMode)
	{
		gGammaFadeFrac = 0;
		return;
	}
#endif

	ObjNode* fader = MakeFadeEvent(false, 3.0f);

	long pFaderFrameCount = fader->FaderFrameCounter;

	while (fader->Mode != kFaderMode_Done)
	{
		CalcFramesPerSecond();
		DoSDLMaintenance();

		if (moveCall)
		{
			moveCall();
		}

		// Force fader object to move even if MoveObjects was skipped
		if (fader->FaderFrameCounter == pFaderFrameCount)	// fader wasn't moved by moveCall
		{
			MoveFadePane(fader);
			pFaderFrameCount = fader->FaderFrameCounter;
		}

		OGL_DrawScene(drawCall);
	}

	// Draw one more blank frame
	gGammaFadeFrac = 0;
	CalcFramesPerSecond();
	DoSDLMaintenance();
	OGL_DrawScene(drawCall);

#if FADE_SOUND
	if (gGameView->fadeSound)
	{
		FadeSound(0);
		KillSong();
		StopAllEffectChannels();
		FadeSound(1);		// restore sound volume for new playback
	}
#endif
}


/******************** MAKE FADE EVENT *********************/
//
// INPUT:	fadeIn = true if want fade IN, otherwise fade OUT.
//

ObjNode* MakeFadeEvent(Boolean fadeIn, float fadeSpeed)
{
	ObjNode	*newObj = NULL;

#if SKIPFLUFF
	if (fadeIn)
		gGammaFadeFrac = 1;
	else
		gGammaFadeFrac = 0;
	return NULL;
#endif

	/* SCAN FOR OLD FADE EVENTS STILL IN LIST */

	for (ObjNode *node = gFirstNodePtr; node != NULL; node = node->NextNode)
	{
		if (node->MoveCall == MoveFadePane)
		{
			newObj = node;
			break;
		}
	}



	if (newObj != NULL)
	{
		/* RECYCLE OLD FADE EVENT */

		newObj->StatusBits = STATUS_BIT_DONTCULL;    // reset status bits in case NOMOVE was set
	}
	else
	{
		/* MAKE NEW FADE EVENT */

		NewObjectDefinitionType def =
		{
			.genre = CUSTOM_GENRE,
			.flags = STATUS_BIT_DONTCULL,
			.slot = FADEPANE_SLOT,
			.moveCall = MoveFadePane,
			.drawCall = DrawFadePane,
			.scale = 1,
		};

		newObj = MakeNewObject(&def);
	}


	gGammaFadeFrac = fadeIn? 0: 1;

	newObj->Mode = fadeIn ? kFaderMode_FadeIn : kFaderMode_FadeOut;
	newObj->FaderFrameCounter = 0;
	newObj->Speed = fadeSpeed;

//	SendNodeToOverlayPane(newObj);

	return newObj;
}


/***************** MOVE FADE EVENT ********************/

static void MoveFadePane(ObjNode *theNode)
{
	float	fps = gFramesPerSecondFrac;
	float	speed = theNode->Speed * fps;

	/* SEE IF FADE IN */

	if (theNode->Mode == kFaderMode_FadeIn)
	{
		gGammaFadeFrac += speed;
		if (gGammaFadeFrac >= 1.0f)				// see if @ 100%
		{
			gGammaFadeFrac = 1;
			theNode->Mode = kFaderMode_Done;
			DeleteObject(theNode);				// nuke it if fading in
		}
	}

		/* FADE OUT */

	else if (theNode->Mode == kFaderMode_FadeOut)
	{
		gGammaFadeFrac -= speed;
		if (gGammaFadeFrac <= 0.0f)				// see if @ 0%
		{
			gGammaFadeFrac = 0;
			theNode->Mode = kFaderMode_Done;
			theNode->StatusBits |= STATUS_BIT_NOMOVE;	// DON'T nuke the fader pane if fading out -- but don't run this again
		}
	}

#if FADE_SOUND
	if (gGameView->fadeSound)
	{
		FadeSound(gGammaFadeFrac);
	}
#endif
}


/***************** DRAW FADE PANE ********************/

static void DrawFadePane(ObjNode* theNode)
{
	(void) theNode;

	OGL_PushState();
	SetInfobarSpriteState();	// (0, 1);

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	SetColor4f(0, 0, 0, 1.0f - gGammaFadeFrac);

	glBegin(GL_QUADS);
	glVertex3f(g2DLogicalRect.right,	g2DLogicalRect.top, 0);
	glVertex3f(g2DLogicalRect.left,		g2DLogicalRect.top, 0);
	glVertex3f(g2DLogicalRect.left,		g2DLogicalRect.bottom, 0);
	glVertex3f(g2DLogicalRect.right,	g2DLogicalRect.bottom, 0);
	glEnd();

	OGL_PopState();
}