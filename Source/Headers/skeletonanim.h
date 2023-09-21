//
// anim.h
//


		/* ANIM EVENTS */

#define	MAX_ANIM_EVENTS		30

#define	MAX_ANIMEVENT_TYPES	7

enum
{
	ANIM_DIRECTION_FORWARD,
	ANIM_DIRECTION_BACKWARD
};


enum
{
	ANIMEVENT_TYPE_STOP,
	ANIMEVENT_TYPE_LOOP,
	ANIMEVENT_TYPE_ZIGZAG,
	ANIMEVENT_TYPE_GOTOMARKER,
	ANIMEVENT_TYPE_SETMARKER,
	ANIMEVENT_TYPE_PLAYSOUND,
	ANIMEVENT_TYPE_SETFLAG,
	ANIMEVENT_TYPE_CLEARFLAG,
	ANIMEVENT_TYPE_PAUSE
};

		/* ACCELERATION MODES */
enum
{
	ACCEL_MODE_LINEAR,
	ACCEL_MODE_EASEINOUT,
	ACCEL_MODE_EASEIN,
	ACCEL_MODE_EASEOUT
};

		/* SLIDERS */

typedef struct
{
	Rect	bounds;
	long	type;
	long	info1;
	long	info2;
}SliderBox;


enum
{
	SLIDER_TYPE_CURRENTTIME,
	SLIDER_TYPE_KEYFRAME,
	SLIDER_TYPE_ANIMEVENT
};

#define	NUM_ACCELERATION_CURVE_NUBS		23						// THESE MUST MATCH BIO-OREO'S NUMBERS!!!
#define	SPLINE_POINTS_PER_NUB			100
#define CURVE_SIZE						((NUM_ACCELERATION_CURVE_NUBS-3)*SPLINE_POINTS_PER_NUB)





//============================================================


extern	void UpdateSkeletonAnimation(ObjNode *theNode);
extern	void SetSkeletonAnim(SkeletonObjDataType *skeleton, long animNum);
extern	void GetModelCurrentPosition(SkeletonObjDataType *skeleton);
extern	void MorphToSkeletonAnim(SkeletonObjDataType *skeleton, long animNum, float speed);
extern	void CalcAccelerationSplineCurve(void);
void SetSkeletonAnimTime(SkeletonObjDataType *skeleton, float timeRatio);

void BurnSkeleton(ObjNode *theNode, float flameScale);


