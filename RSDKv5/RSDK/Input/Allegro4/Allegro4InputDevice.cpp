
using namespace RSDK;

#define NORMALIZE(val, minVal, maxVal) ((float)(val) - (float)(minVal)) / ((float)(maxVal) - (float)(minVal))

/*bool32 getControllerButton(RSDK::SKU::InputDeviceSDL *device, uint8 buttonID)
{
    return false;
}*/

void RSDK::SKU::InputDeviceAllegro4::UpdateInput()
{
/*   int32 buttonMasks;
    int32 prevButtonMasks;
    uint8 stateUp;
    uint8 stateDown;
    uint8 stateLeft;
    uint8 stateRight;
    uint8 stateA;
    uint8 stateB;
    uint8 stateC;
    uint8 stateX;
    uint8 stateY;
    uint8 stateZ;
    uint8 stateStart;
    uint8 stateSelect;
    uint8 stateBumper_L;
    uint8 stateBumper_R;
    uint8 stateStick_L;
    uint8 stateStick_R;
    float bumperDeltaL;
    float bumperDeltaR;
    float triggerDeltaL;
    float triggerDeltaR;
    float hDelta_L;
    float vDelta_L;
    float vDelta_R;
    float hDelta_R;*/
	stateUp = key[__allegro_KEY_UP];
	stateDown = key[__allegro_KEY_DOWN];
	stateLeft = key[__allegro_KEY_LEFT];
	stateRight = key[__allegro_KEY_RIGHT];
	stateA = key[__allegro_KEY_A];
	stateB = key[__allegro_KEY_S];
	stateC = key[__allegro_KEY_D];
	stateX = key[__allegro_KEY_Z];
	stateY = key[__allegro_KEY_X];
	stateZ = key[__allegro_KEY_C];
	
	poll_keyboard();
	
	stateStart = key[__allegro_KEY_ENTER];
	stateSelect = key[__allegro_KEY_SPACE];
	
	inputDeviceList[0]->inactiveTimer[0] = 0;
	inputDeviceList[0]->inactiveTimer[1] = 0;

	
	anyPress = stateUp || stateDown || stateLeft || stateRight || stateA || stateB || stateC || stateX || stateY || stateZ || stateStart || stateSelect;// > 0;
	
	ProcessInput(CONT_ANY);
	ProcessInput(CONT_P1);
}

void RSDK::SKU::InputDeviceAllegro4::ProcessInput(int32 controllerID)
{
   controller[controllerID].keyUp.press = this->stateUp;
    controller[controllerID].keyDown.press = this->stateDown;
    controller[controllerID].keyLeft.press = this->stateLeft;
    controller[controllerID].keyRight.press = this->stateRight;
    controller[controllerID].keyA.press = this->stateA;
    controller[controllerID].keyB.press = this->stateB;
    controller[controllerID].keyC.press = this->stateC;
    controller[controllerID].keyX.press = this->stateX;
    controller[controllerID].keyY.press = this->stateY;
    controller[controllerID].keyZ.press = this->stateZ;
    controller[controllerID].keyStart.press = this->stateStart;
    controller[controllerID].keySelect.press = this->stateSelect;

#if RETRO_REV02
    stickL[controllerID].keyStick.press |= this->stateStick_L;
    stickL[controllerID].hDelta = this->hDelta_L;
    stickL[controllerID].vDelta = this->vDelta_L;
    stickL[controllerID].keyUp.press |= this->vDelta_L > INPUT_DEADZONE;
    stickL[controllerID].keyDown.press |= this->vDelta_L < -INPUT_DEADZONE;
    stickL[controllerID].keyLeft.press |= this->hDelta_L < -INPUT_DEADZONE;
    stickL[controllerID].keyRight.press |= this->hDelta_L > INPUT_DEADZONE;

    stickR[controllerID].keyStick.press |= this->stateStick_R;
    stickR[controllerID].hDelta = this->vDelta_R;
    stickR[controllerID].vDelta = this->hDelta_R;
    stickR[controllerID].keyUp.press |= this->vDelta_R > INPUT_DEADZONE;
    stickR[controllerID].keyDown.press |= this->vDelta_R < -INPUT_DEADZONE;
    stickR[controllerID].keyLeft.press |= this->hDelta_R < -INPUT_DEADZONE;
    stickR[controllerID].keyRight.press |= this->hDelta_R > INPUT_DEADZONE;

    triggerL[controllerID].keyBumper.press |= this->stateBumper_L;
    triggerL[controllerID].keyTrigger.press |= this->triggerDeltaL > INPUT_DEADZONE;
    triggerL[controllerID].bumperDelta  = this->bumperDeltaL;
    triggerL[controllerID].triggerDelta = this->triggerDeltaL;

    triggerR[controllerID].keyBumper.press |= this->stateBumper_R;
    triggerR[controllerID].keyTrigger.press |= this->triggerDeltaR > INPUT_DEADZONE;
    triggerR[controllerID].bumperDelta  = this->bumperDeltaR;
    triggerR[controllerID].triggerDelta = this->triggerDeltaR;
#else
    controller[controllerID].keyStickL.press |= this->stateStick_L;
    stickL[controllerID].hDeltaL = this->hDelta_L;
    stickL[controllerID].vDeltaL = this->vDelta_L;
    stickL[controllerID].keyUp.press |= this->vDelta_L > INPUT_DEADZONE;
    stickL[controllerID].keyDown.press |= this->vDelta_L < -INPUT_DEADZONE;
    stickL[controllerID].keyLeft.press |= this->hDelta_L < -INPUT_DEADZONE;
    stickL[controllerID].keyRight.press |= this->hDelta_L > INPUT_DEADZONE;

    controller[controllerID].keyStickR.press |= this->stateStick_R;
    stickL[controllerID].hDeltaR = this->vDelta_R;
    stickL[controllerID].vDeltaR = this->hDelta_R;

    controller[controllerID].keyBumperL.press |= this->stateBumper_L;
    controller[controllerID].keyTriggerL.press |= this->triggerDeltaL > INPUT_DEADZONE;
    stickL[controllerID].triggerDeltaL = this->triggerDeltaL;

    controller[controllerID].keyBumperR.press |= this->stateBumper_R;
    controller[controllerID].keyTriggerR.press |= this->triggerDeltaR > INPUT_DEADZONE;
    stickL[controllerID].triggerDeltaR = this->triggerDeltaR;
#endif

	inputDeviceList[0]->inactiveTimer[0] = 0;
	inputDeviceList[0]->inactiveTimer[1] = 0;
	
}

void RSDK::SKU::InputDeviceAllegro4::CloseDevice()
{

}

void RSDK::SKU::InitAllegro4InputAPI()
{
	printf("initAllegro4\n");
	
//	install_keyboard();

	
	if (inputDeviceCount >= INPUTDEVICE_COUNT)
		return;

	if (inputDeviceList[inputDeviceCount] && inputDeviceList[inputDeviceCount]->active)
		return;

	if (inputDeviceList[inputDeviceCount])
		delete inputDeviceList[inputDeviceCount];
	
	InputDeviceAllegro4 *idev = new InputDeviceAllegro4();
	
	idev->id = DEVICE_SATURN;
	idev->gamepadType = DEVICE_TYPE_CONTROLLER;
	idev->active = true;
	idev->isAssigned = true;
	idev->disabled = false;	
	idev->inactiveTimer[0] = 0;
	idev->inactiveTimer[1] = 0;
	
	inputDeviceList[inputDeviceCount++] = idev;
	
	AssignInputSlotToDevice(CONT_P1, DEVICE_SATURN);

	
	
	printf("inputDeviceCount = %d\n", inputDeviceCount);
}

void RSDK::SKU::ReleaseAllegro4InputAPI()
{
	
}

