#include "debug.h"

#define GINPUT_COMPILE_VC_VERSION
#include <GInputAPI.h>

// VC

static uintptr thisptr;
static uintptr baseptr;

WRAPPER double CGeneral__GetATanOfXY(float x, float y) { EAXJMP(0x4A55E0); }

WRAPPER void CCam::Process(void) { EAXJMP(0x48351A); }

bool &CPad::m_bMapPadOneToPadTwo = *(bool*)0xA10AE4;

static int toggleDebugCamSwitch;
static int toggleHudSwitch;

// inlined in VC
void
CCam::GetVectorsReadyForRW(void)
{
	CVector right;
	Up = CVector(0.0f, 0.0f, 1.0f);
	Front.Normalise();
	if(Front.x == 0.0f && Front.y == 0.0f){
		Front.x = 0.0001f;
		Front.y = 0.0001f;
	}
	right = CrossProduct(Front, Up);
	right.Normalise();
	Up = CrossProduct(right, Front);
}

enum Controlmode {
	CONTROL_CAMERA,
	CONTROL_PLAYER,
};
int controlMode = CONTROL_CAMERA;
int activatedFromKeyboard;

float gFOV = 70.0f;

void
CCamera::InitialiseCameraForDebugMode(void)
{
	CEntity *e;
	if(e = FindPlayerVehicle())
		this->Cams[2].Source = e->_.GetPosition();
	else if(e = FindPlayerPed())
		this->Cams[2].Source = e->_.GetPosition();
	this->Cams[2].Alpha = 0;
	this->Cams[2].Beta = 0;
	this->Cams[2].Mode = 6;
//	activatedFromKeyboard = keytemp;
//	CPad::m_bMapPadOneToPadTwo = 1;


	Cams[2].Source = Cams[ActiveCam].Source;
	CVector nfront = Cams[ActiveCam].Front;
	float groundDist = sqrt(nfront.x*nfront.x + nfront.y*nfront.y);
	Cams[2].Beta = CGeneral__GetATanOfXY(nfront.x, nfront.y);
	Cams[2].Alpha = CGeneral__GetATanOfXY(groundDist, nfront.z);
	while(Cams[2].Beta >= PI) Cams[2].Beta -= 2.0f*PI;
	while(Cams[2].Beta < -PI) Cams[2].Beta += 2.0f*PI;
	while(Cams[2].Alpha >= PI) Cams[2].Alpha -= 2.0f*PI;
	while(Cams[2].Alpha < -PI) Cams[2].Alpha += 2.0f*PI;


	LoadSavedCams();
}

float sensPos = 1.0f;
float sensRot = 1.0f;
int mode;

void
CCam::Process_Kalvin(float*, float, float, float)
{
	float frontspeed = 0.0;
	float rightspeed = 0.0;
	float upspeed = 0.0;
	CPad *pad = CPad::GetPad(1);

	if(JUSTDOWN(SELECT))
		mode = !mode;
	if(mode == 0){
		Process_Debug(nil, 0, 0, 0);
		return;
	}

	if(CTRLJUSTDOWN('C')){
		if(controlMode == CONTROL_CAMERA) controlMode = CONTROL_PLAYER;
		else if(controlMode == CONTROL_PLAYER) controlMode = CONTROL_CAMERA;
	}

	if(KEYJUSTDOWN('Z') && controlMode == CONTROL_CAMERA)
		SaveCam(this);
	if(KEYJUSTDOWN('X') && controlMode == CONTROL_CAMERA)
		DeleteSavedCams();
	if(KEYJUSTDOWN('Q') && controlMode == CONTROL_CAMERA)
		PrevSavedCam(this);
	if(KEYJUSTDOWN('E') && controlMode == CONTROL_CAMERA)
		NextSavedCam(this);

	RwCameraSetNearClipPlane(Scene.camera, 0.1f);
	this->FOV = gFOV;

	this->Alpha += DEG2RAD(pad->NewState.LEFTSTICKY/64.0f) * sensRot;
	this->Beta  -= DEG2RAD(pad->NewState.LEFTSTICKX/64.0f) * sensRot;
	if(KEYDOWN((RsKeyCodes)'I') && controlMode == CONTROL_CAMERA)
		this->Alpha += DEG2RAD(sensRot);
	if(KEYDOWN((RsKeyCodes)'K') && controlMode == CONTROL_CAMERA)
		this->Alpha -= DEG2RAD(sensRot);
	if(KEYDOWN((RsKeyCodes)'J') && controlMode == CONTROL_CAMERA)
		this->Beta += DEG2RAD(sensRot);
	if(KEYDOWN((RsKeyCodes)'L') && controlMode == CONTROL_CAMERA)
		this->Beta -= DEG2RAD(sensRot);

	if(this->Alpha > DEG2RAD(89.5f)) this->Alpha = DEG2RAD(89.5f);
	if(this->Alpha < DEG2RAD(-89.5f)) this->Alpha = DEG2RAD(-89.5f);

	CVector vec;
	vec.x = this->Source.x + cos(this->Beta) * cos(this->Alpha) * 3.0f;
	vec.y = this->Source.y + sin(this->Beta) * cos(this->Alpha) * 3.0f;
	vec.z = this->Source.z + sin(this->Alpha) * 3.0f;

	frontspeed -= pad->NewState.RIGHTSTICKY/128.0f * sensPos;
	rightspeed += pad->NewState.RIGHTSTICKX/128.0f * sensPos;

	if(KEYDOWN((RsKeyCodes)'W') && controlMode == CONTROL_CAMERA)
		frontspeed += sensPos;
	if(KEYDOWN((RsKeyCodes)'S') && controlMode == CONTROL_CAMERA)
		frontspeed -= sensPos;
	if(KEYDOWN((RsKeyCodes)'A') && controlMode == CONTROL_CAMERA)
		rightspeed -= sensPos;
	if(KEYDOWN((RsKeyCodes)'D') && controlMode == CONTROL_CAMERA)
		rightspeed += sensPos;
	if(pad->NewState.SQUARE)
		upspeed += sensPos;
	if(pad->NewState.CROSS)
		upspeed -= sensPos;

	this->Front = vec - this->Source;
	this->Front.Normalise();
	this->Source = this->Source + this->Front*frontspeed;

	CVector up = { 0.0f, 0.0f, 1.0f };
	CVector right;
	right = CrossProduct(Front, up);
	up = CrossProduct(right, Front);
	Source = Source + up*upspeed + right*rightspeed;

	if(this->Source.z < -450.0f)
		this->Source.z = -450.0f;

	// stay inside sectors
	while(this->Source.x * 0.02f + 48.0f > 75.0f)
		this->Source.x -= 1.0f;
	while(this->Source.x * 0.02f + 48.0f < 5.0f)
		this->Source.x += 1.0f;
	while(this->Source.y * 0.02f + 40.0f > 75.0f)
		this->Source.y -= 1.0f;
	while(this->Source.y * 0.02f + 40.0f < 5.0f)
		this->Source.y += 1.0f;

/*
	CPad *pad = CPad::GetPad(1);
	if(JUSTDOWN(RIGHTSHOULDER2) ||
	   KEYJUSTDOWN(rsEXTENTER) && controlMode == CONTROL_CAMERA){
		if(FindPlayerVehicle()){
			CEntity *e = FindPlayerVehicle();
			CEntityVtbl *vmt = (CEntityVtbl*)e->vtable;
			vmt->Teleport(e, Source);
		}else if(FindPlayerPed()){
			CEntity *e = FindPlayerPed();
			e->_.SetPosition(this->Source);
		}
	}
*/

//	if(controlMode == CONTROL_CAMERA && activatedFromKeyboard)
//		CPad::GetPad(0)->DisablePlayerControls = 1;

	GetVectorsReadyForRW();
}

void
CCam::Process_Debug(float*, float, float, float)
{
	static float speed = 0.0f;
	static float panspeedX = 0.0f;
	static float panspeedY = 0.0f;

	if(CTRLJUSTDOWN('C')){
		if(controlMode == CONTROL_CAMERA) controlMode = CONTROL_PLAYER;
		else if(controlMode == CONTROL_PLAYER) controlMode = CONTROL_CAMERA;
	}

	if(KEYJUSTDOWN('Z') && controlMode == CONTROL_CAMERA)
		SaveCam(this);
	if(KEYJUSTDOWN('X') && controlMode == CONTROL_CAMERA)
		DeleteSavedCams();
	if(KEYJUSTDOWN('Q') && controlMode == CONTROL_CAMERA)
		PrevSavedCam(this);
	if(KEYJUSTDOWN('E') && controlMode == CONTROL_CAMERA)
		NextSavedCam(this);

	RwCameraSetNearClipPlane(Scene.camera, 0.9f);
	this->FOV = gFOV;
	this->Alpha += DEG2RAD(CPad::GetPad(1)->NewState.LEFTSTICKY*0.02f); // magic
	this->Beta  -= CPad::GetPad(1)->NewState.LEFTSTICKX * 0.02617994f * 0.052631579f; // magic
	if(controlMode == CONTROL_CAMERA && CPad::NewMouseControllerState.lmb){
		this->Alpha += DEG2RAD(CPad::NewMouseControllerState.Y/2.0f);
		this->Beta -= DEG2RAD(CPad::NewMouseControllerState.X/2.0f);
	}

	if(this->Alpha > DEG2RAD(89.5f)) this->Alpha = DEG2RAD(89.5f);
	if(this->Alpha < DEG2RAD(-89.5f)) this->Alpha = DEG2RAD(-89.5f);

	CVector vec;
	vec.x = this->Source.x + cos(this->Beta) * cos(this->Alpha) * 3.0f;
	vec.y = this->Source.y + sin(this->Beta) * cos(this->Alpha) * 3.0f;
	vec.z = this->Source.z + sin(this->Alpha) * 3.0f;

	if(CPad::GetPad(1)->NewState.SQUARE ||
	   KEYDOWN((RsKeyCodes)'W') && controlMode == CONTROL_CAMERA)
		speed += 0.1f;
	else if(CPad::GetPad(1)->NewState.CROSS ||
	        KEYDOWN((RsKeyCodes)'S') && controlMode == CONTROL_CAMERA)
		speed -= 0.1f;
	else
		speed = 0.0f;

	if(speed > 70.0f) speed = 70.0f;
	if(speed < -70.0f) speed = -70.0f;

	if((KEYDOWN((RsKeyCodes)rsRIGHT) || KEYDOWN((RsKeyCodes)'D')) && controlMode == CONTROL_CAMERA)
		panspeedX += 0.1f;
	else if((KEYDOWN((RsKeyCodes)rsLEFT) || KEYDOWN((RsKeyCodes)'A')) && controlMode == CONTROL_CAMERA)
		panspeedX -= 0.1f;
	else
		panspeedX = 0.0f;
	if(panspeedX > 70.0f) panspeedX = 70.0f;
	if(panspeedX < -70.0f) panspeedX = -70.0f;

	if(KEYDOWN((RsKeyCodes)rsUP) && controlMode == CONTROL_CAMERA)
		panspeedY += 0.1f;
	else if(KEYDOWN((RsKeyCodes)rsDOWN) && controlMode == CONTROL_CAMERA)
		panspeedY -= 0.1f;
	else
		panspeedY = 0.0f;
	if(panspeedY > 70.0f) panspeedY = 70.0f;
	if(panspeedY < -70.0f) panspeedY = -70.0f;

	this->Front = vec - this->Source;
	this->Front.Normalise();
	this->Source = this->Source + this->Front*speed;

	CVector up = { 0.0f, 0.0f, 1.0f };
	CVector right;
	right = CrossProduct(Front, up);
	up = CrossProduct(right, Front);
	Source = Source + up*panspeedY + right*panspeedX;

	if(this->Source.z < -450.0f)
		this->Source.z = -450.0f;

	// stay inside sectors
	while(this->Source.x * 0.02f + 48.0f > 75.0f)
		this->Source.x -= 1.0f;
	while(this->Source.x * 0.02f + 48.0f < 5.0f)
		this->Source.x += 1.0f;
	while(this->Source.y * 0.02f + 40.0f > 75.0f)
		this->Source.y -= 1.0f;
	while(this->Source.y * 0.02f + 40.0f < 5.0f)
		this->Source.y += 1.0f;

	CPad *pad = CPad::GetPad(1);
	if(JUSTDOWN(RIGHTSHOULDER2) ||
	   KEYJUSTDOWN(rsEXTENTER) && controlMode == CONTROL_CAMERA){
		if(FindPlayerVehicle()){
			CEntity *e = FindPlayerVehicle();
			CEntityVtbl *vmt = (CEntityVtbl*)e->vtable;
			vmt->Teleport(e, Source);
		}else if(FindPlayerPed()){
			CEntity *e = FindPlayerPed();
			e->_.SetPosition(this->Source);
		}
	}

	if(controlMode == CONTROL_CAMERA && activatedFromKeyboard)
		CPad::GetPad(0)->DisablePlayerControls = 1;

	GetVectorsReadyForRW();
}

static void __declspec(naked)
switchDebugHook(void)
{
	_asm{
		// standard code for most cases:
		lea	eax, [esp+40h]
		mov	ecx, ebp
		push	[esp+2ch]
		push	dword ptr [ebp+0D4h]
		push	[esp+38h]
		push	eax
		call	CCam::Process_Kalvin
//		call	CCam::Process_Debug
		push	0x483E10
		retn
	}
}

static void
toggleCam(void)
{
	CCamera *cam = (CCamera*)thisptr;
	assert(cam == (CCamera*)0x7E4688);
	CPad *pad = CPad::GetPad(1);
	int keydown = CTRLJUSTDOWN('B') || toggleDebugCamSwitch;
	if(JUSTDOWN(CIRCLE) || keydown){
		toggleDebugCamSwitch = 0;
		cam->WorldViewerBeingUsed = !cam->WorldViewerBeingUsed;
		if(cam->WorldViewerBeingUsed){
			activatedFromKeyboard = keydown;
			cam->InitialiseCameraForDebugMode();
		}
	}
}

static void __declspec(naked)
toggleCamHook(void)
{
	_asm{
		mov	thisptr, ebx
		call	toggleCam

		push	ds:0x68ADC8
		push	0x46C723
		retn
	}
}

static void
processCam(void)
{
	CCamera *cam = (CCamera*)thisptr;
	if(cam->WorldViewerBeingUsed)
		cam->Cams[2].Process();
}

static void __declspec(naked)
processCamHook(void)
{
	_asm{
		pusha
		mov	thisptr, ebx
		sub	esp,108
		fsave	[esp]
		call	processCam
		frstor	[esp]
		add	esp,108
		popa

		movzx	eax, [ebx+0x76]	// active Cam
		imul	eax, 1CCh
		push	0x46C8DF
		retn
	}
}

static void
copyVectors(void)
{
	CCamera *cam = (CCamera*)thisptr;

	CVector *source = (CVector*)(baseptr - 0x174);
	CVector *front = (CVector*)(baseptr - 0x168);
	CVector *up = (CVector*)(baseptr - 0x15C);
	float *fov = (float*)(baseptr - 0x138);

	*source = cam->Cams[2].Source;
	*front = cam->Cams[2].Front;
	*up = cam->Cams[2].Up;
	*fov = cam->Cams[2].FOV;

	*(bool*)0x7038D1 = 0;
}

static void __declspec(naked)
copyVectorsHook(void)
{
	_asm{
		mov	al,[ebx + 0x75]	// WorldViewerBeingUsed
		test	al,al
		jz	nodebug

		mov	thisptr, ebx
		lea	eax, [esp+0x1d0]
		mov	baseptr, eax
		call	copyVectors
		push	0x46D73A
		retn

	nodebug:
		movzx	eax, [ebx+0x76]	// active Cam
		imul	eax, 1CCh
		lea	edi, [eax+ebx+2FCh]
		push	0x46C918
		retn
	}
}

static RwFrame*
copyToRw(void)
{
	CCamera *cam = (CCamera*)thisptr;
	RwCamera *rwcam = cam->m_pRwCamera;
	RwFrame *frm = RwCameraGetFrame(rwcam);
	RwMatrix *mat = RwFrameGetMatrix(frm);

	CVector right;
	right = CrossProduct(cam->Cams[2].Up, cam->Cams[2].Front);
	cam->matrix.matrix.right = *(RwV3d*)&right;
	cam->matrix.matrix.up = *(RwV3d*)&cam->Cams[2].Front;
	cam->matrix.matrix.at = *(RwV3d*)&cam->Cams[2].Up;
	cam->matrix.matrix.pos = *(RwV3d*)&cam->Cams[2].Source;
	CDraw::SetFOV(cam->Cams[2].FOV);
	cam->m_vecGameCamPos = cam->Cams[2].Source;

	mat->pos = cam->matrix.matrix.pos;
	mat->at = cam->matrix.matrix.up;
	mat->up = cam->matrix.matrix.at;
	mat->right = cam->matrix.matrix.right;
	RwMatrixUpdate(mat);
	return frm;
}

static void __declspec(naked)
copyToRWHook(void)
{
	_asm{
		mov	al,[ebx + 0x75]	// WorldViewerBeingUsed
		test	al,al
		jz	nodebug

		pop	ecx	// clean up argument
		mov	thisptr, ebx
		sub	esp,108
		fsave	[esp]
		call	copyToRw
		frstor	[esp]
		add	esp,108
		mov	edi,eax
		push	0x46DD42
		retn

	nodebug:
		mov     eax, [ebx+808h]
		push	0x46DCD3
		retn
	}
}

static bool
toggleHUD(void)
{
	CPad *pad = CPad::GetPad(1);
	bool ret = JUSTDOWN(START) || (CTRLJUSTDOWN('H') && TheCamera.WorldViewerBeingUsed) || toggleHudSwitch;
	toggleHudSwitch = 0;
	return ret;
}

// Not strictly cam but whatever
void __declspec(naked)
togglehudhook(void)
{
	_asm{
		call	toggleHUD
		mov	dl,al
		push	0x55736C
		retn
	}
}

// TEMP TEMP TEMP
IGInputPad *ginput;
bool padswitch;
void
switchPad(void)
{
	ginput->SendEvent(GINPUT_EVENT_FORCE_MAP_PAD_ONE_TO_PAD_TWO, (void*)padswitch);
}

void
patchDebugCam(void)
{
	InjectHook(0x46C71D, toggleCamHook, PATCH_JUMP);
	InjectHook(0x46C8D5, processCamHook, PATCH_JUMP);
	InjectHook(0x46C911, copyVectorsHook, PATCH_JUMP);
	InjectHook(0x46DCCD, copyToRWHook, PATCH_JUMP);
	*(uintptr*)0x68b274 = (uintptr)switchDebugHook;

	InjectHook(0x55734B, togglehudhook, PATCH_JUMP);
	
	if(DebugMenuLoad()){
		DebugMenuEntry *e;
		static const char *controlStr[] = { "Camera", "Player" };
		DebugMenuAddCmd("Debug", "Toggle Debug Camera", [](){ toggleDebugCamSwitch = 1; });
		e = DebugMenuAddVar("Debug", "Debug Camera Control", &controlMode, nil, 1, CONTROL_CAMERA, CONTROL_PLAYER, controlStr);
		DebugMenuEntrySetWrap(e, true);
		DebugMenuAddVar("Debug", "Debug Camera FOV", &gFOV, nil, 1.0f, 5.0f, 180.0f);
		DebugMenuAddCmd("Debug", "Save Camera Position", [](){ SaveCam(&TheCamera.Cams[2]); });
		DebugMenuAddCmd("Debug", "Cycle Next", [](){ NextSavedCam(&TheCamera.Cams[2]); });
		DebugMenuAddCmd("Debug", "Cycle Prev", [](){ PrevSavedCam(&TheCamera.Cams[2]); });
		DebugMenuAddCmd("Debug", "Delete Camera Positions", DeleteSavedCams);

		DebugMenuAddCmd("Debug", "Toggle HUD", [](){ toggleHudSwitch = 1; });

//		DebugMenuAddVarBool8("Debug", "Map Pad 1 -> Pad 2", (int8_t*)&CPad::m_bMapPadOneToPadTwo, nil);
// TEMP TEMP TEMP
		if(GInput_Load(&ginput))
			DebugMenuAddVarBool8("Debug", "Map Pad 1 -> Pad 2", (int8_t*)&padswitch, switchPad);

		DebugMenuAddFloat32("Debug", "KALVINCAM sensitivity translate", &sensPos, nil, 0.1f, 0.0f, 100.0f);
		DebugMenuAddFloat32("Debug", "KALVINCAM sensitivity rotate", &sensRot, nil, 0.1f, 0.0f, 100.0f);

	}
}
