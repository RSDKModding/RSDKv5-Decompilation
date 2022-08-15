using namespace RSDK;

uint64_t connectedAxis = 0;

void RSDK::SKU::InputDevicePaddleboat::UpdateInput()
{
    Paddleboat_Controller_Data *inputState = &this->inputState[this->activeState];
    if (Paddleboat_getControllerData(controllerID, inputState) == PADDLEBOAT_NO_ERROR) {
        this->activeState ^= 1;
        int32 changedButtons = ~this->inputState[activeState].buttonsDown & (this->inputState[activeState].buttonsDown ^ inputState->buttonsDown);

        if (changedButtons)
            this->inactiveTimer[0] = 0;
        else
            ++this->inactiveTimer[0];

        if ((changedButtons & PADDLEBOAT_BUTTON_A) || (changedButtons & PADDLEBOAT_BUTTON_START))
            this->inactiveTimer[1] = 0;
        else
            ++this->inactiveTimer[1];

        this->anyPress = changedButtons || (inputState->leftStick.stickX != this->inputState[activeState].leftStick.stickX)
                         || (inputState->leftStick.stickY != this->inputState[activeState].leftStick.stickY)
                         || (inputState->rightStick.stickX != this->inputState[activeState].rightStick.stickX)
                         || (inputState->rightStick.stickY != this->inputState[activeState].rightStick.stickY);

        this->stateUp       = (inputState->buttonsDown & PADDLEBOAT_BUTTON_DPAD_UP) != 0;
        this->stateDown     = (inputState->buttonsDown & PADDLEBOAT_BUTTON_DPAD_DOWN) != 0;
        this->stateLeft     = (inputState->buttonsDown & PADDLEBOAT_BUTTON_DPAD_LEFT) != 0;
        this->stateRight    = (inputState->buttonsDown & PADDLEBOAT_BUTTON_DPAD_RIGHT) != 0;
        this->stateA        = (inputState->buttonsDown & PADDLEBOAT_BUTTON_A) != 0;
        this->stateB        = (inputState->buttonsDown & PADDLEBOAT_BUTTON_B) != 0;
        this->stateX        = (inputState->buttonsDown & PADDLEBOAT_BUTTON_X) != 0;
        this->stateY        = (inputState->buttonsDown & PADDLEBOAT_BUTTON_Y) != 0;
        this->stateStart    = (inputState->buttonsDown & PADDLEBOAT_BUTTON_START) != 0;
        this->stateSelect   = (inputState->buttonsDown & PADDLEBOAT_BUTTON_SELECT) != 0;
        this->stateBumper_L = (inputState->buttonsDown & PADDLEBOAT_BUTTON_L1) != 0;
        this->stateBumper_R = (inputState->buttonsDown & PADDLEBOAT_BUTTON_R1) != 0;
        this->stateStick_L  = (inputState->buttonsDown & PADDLEBOAT_BUTTON_L3) != 0;
        this->stateStick_R  = (inputState->buttonsDown & PADDLEBOAT_BUTTON_R3) != 0;

        // this is copied directly from xinputs but making their constants relative to 0 to 1 or -1 to 1
        // constants are just scalled to what they are in the original bc neither me or rdc currently knows how it works

        this->hDelta_L = inputState->leftStick.stickX;
        this->vDelta_L = inputState->leftStick.stickY;

        float div      = sqrtf((this->hDelta_L * this->hDelta_L) + (this->vDelta_L * this->vDelta_L));
        this->hDelta_L = this->hDelta_L / div;
        this->vDelta_L = this->vDelta_L / div;
        if (div <= (7864.0 / 32767.0)) {
            this->hDelta_L = 0.0;
            this->vDelta_L = 0.0;
        }
        else {
            this->hDelta_L = this->hDelta_L * ((fminf(1.0, div) - (7864.0 / 32767.0)) / (24903.0 / 32767.0));
            this->vDelta_L = this->vDelta_L * ((fminf(1.0, div) - (7864.0 / 32767.0)) / (24903.0 / 32767.0));
        }

        this->hDelta_R = inputState->rightStick.stickX;
        this->vDelta_R = inputState->rightStick.stickY;

        div            = sqrtf((this->hDelta_R * this->hDelta_R) + (this->vDelta_R * this->vDelta_R));
        this->hDelta_R = this->hDelta_R / div;
        this->vDelta_R = this->vDelta_R / div;
        if (div <= (7864.0 / 32767.0)) {
            this->hDelta_R = 0.0;
            this->vDelta_R = 0.0;
        }
        else {
            this->hDelta_R = this->hDelta_R * ((fminf(1.0, div) - (7864.0 / 32767.0)) / (24903.0 / 32767.0));
            this->vDelta_R = this->vDelta_R * ((fminf(1.0, div) - (7864.0 / 32767.0)) / (24903.0 / 32767.0));
        }

        this->deltaBumper_L = this->stateBumper_L ? 1.0 : 0.0;

        this->deltaTrigger_L = inputState->triggerL2;
        if (this->deltaTrigger_L <= (30.0 / 255.0))
            this->deltaTrigger_L = 0.0;
        else
            this->deltaTrigger_L = (this->deltaTrigger_L - (30.0 / 255.0)) / (225.0 / 255.0);

        this->deltaBumper_R = this->stateBumper_R ? 1.0 : 0.0;

        this->deltaTrigger_R = inputState->triggerR2;
        if (this->deltaTrigger_R <= (30.0 / 255.0))
            this->deltaTrigger_R = 0.0;
        else
            this->deltaTrigger_R = (this->deltaTrigger_R - (30.0 / 255.0)) / (225.0 / 255.0);
    }
    this->ProcessInput(CONT_ANY);
}

void RSDK::SKU::InputDevicePaddleboat::ProcessInput(int32 controllerID)
{
    controller[controllerID].keyUp.press |= this->stateUp;
    controller[controllerID].keyDown.press |= this->stateDown;
    controller[controllerID].keyLeft.press |= this->stateLeft;
    controller[controllerID].keyRight.press |= this->stateRight;
    controller[controllerID].keyA.press |= this->stateA;
    controller[controllerID].keyB.press |= this->stateB;
    controller[controllerID].keyX.press |= this->stateX;
    controller[controllerID].keyY.press |= this->stateY;
    controller[controllerID].keyStart.press |= this->stateStart;
    controller[controllerID].keySelect.press |= this->stateSelect;

#if RETRO_REV02
    stickL[controllerID].keyStick.press |= this->stateStick_L;
    stickL[controllerID].hDelta = this->hDelta_L;
    stickL[controllerID].vDelta = this->vDelta_L;
    stickL[controllerID].keyUp.press |= this->vDelta_L > INPUT_DEADZONE;
    stickL[controllerID].keyDown.press |= this->vDelta_L < -INPUT_DEADZONE;
    stickL[controllerID].keyLeft.press |= this->hDelta_L < -INPUT_DEADZONE;
    stickL[controllerID].keyRight.press |= this->hDelta_L > INPUT_DEADZONE;

    stickR[controllerID].keyStick.press |= this->stateStick_R;
    stickR[controllerID].hDelta = this->hDelta_R;
    stickR[controllerID].vDelta = this->vDelta_R;
    stickR[controllerID].keyUp.press |= this->vDelta_R > INPUT_DEADZONE;
    stickR[controllerID].keyDown.press |= this->vDelta_R < -INPUT_DEADZONE;
    stickR[controllerID].keyLeft.press |= this->hDelta_R < -INPUT_DEADZONE;
    stickR[controllerID].keyRight.press |= this->hDelta_R > INPUT_DEADZONE;

    triggerL[controllerID].keyBumper.press |= this->stateBumper_L;
    triggerL[controllerID].bumperDelta  = this->deltaBumper_L;
    triggerL[controllerID].triggerDelta = this->deltaTrigger_L;

    triggerR[controllerID].keyBumper.press |= this->stateBumper_R;
    triggerR[controllerID].bumperDelta  = this->deltaBumper_R;
    triggerR[controllerID].triggerDelta = this->deltaTrigger_R;
#else
    controller[controllerID].keyStickL.press |= this->stateStick_L;
    stickL[controllerID].hDeltaL = this->hDelta_L;
    stickL[controllerID].vDeltaL = this->vDelta_L;

    stickL[controllerID].keyUp.press |= this->vDelta_L > INPUT_DEADZONE;
    stickL[controllerID].keyDown.press |= this->vDelta_L < -INPUT_DEADZONE;
    stickL[controllerID].keyLeft.press |= this->hDelta_L < -INPUT_DEADZONE;
    stickL[controllerID].keyRight.press |= this->hDelta_L > INPUT_DEADZONE;

    controller[controllerID].keyStickR.press |= this->stateStick_R;
    stickL[controllerID].hDeltaL = this->hDelta_R;
    stickL[controllerID].vDeltaL = this->vDelta_R;

    controller[controllerID].keyBumperL.press |= this->stateBumper_L;
    stickL[controllerID].deadzone      = this->deltaBumper_L;
    stickL[controllerID].triggerDeltaL = this->deltaTrigger_L;

    controller[controllerID].keyBumperR.press |= this->stateBumper_R;
    stickL[controllerID].triggerDeltaR = this->deltaTrigger_R;
#endif
}

void RSDK::SKU::InputDevicePaddleboat::CloseDevice()
{
    this->active       = false;
    this->isAssigned   = false;
    this->controllerID = PADDLEBOAT_MAX_CONTROLLERS;
    GameActivityPointerAxes_disableAxis(controllerID);
}

RSDK::SKU::InputDevicePaddleboat *RSDK::SKU::InitPaddleboatInputDevice(uint32 id, uint8 controllerID)
{
    if (inputDeviceCount >= INPUTDEVICE_COUNT)
        return NULL;

    if (inputDeviceList[inputDeviceCount] && inputDeviceList[inputDeviceCount]->active)
        return NULL;

    if (inputDeviceList[inputDeviceCount])
        delete inputDeviceList[inputDeviceCount];

    inputDeviceList[inputDeviceCount] = new InputDevicePaddleboat();

    InputDevicePaddleboat *device = (InputDevicePaddleboat *)inputDeviceList[inputDeviceCount];

    device->controllerID = controllerID;
    GameActivityPointerAxes_enableAxis(controllerID);

    uint8 controllerType = DEVICE_XBOX;

    Paddleboat_Controller_Info info;
    Paddleboat_getControllerInfo(controllerID, &info);

    Paddleboat_ControllerButtonLayout layout = (Paddleboat_ControllerButtonLayout)(info.controllerFlags & PADDLEBOAT_CONTROLLER_LAYOUT_MASK);

    if (layout == PADDLEBOAT_CONTROLLER_LAYOUT_SHAPES)
        controllerType = DEVICE_PS4;
    else if (layout == PADDLEBOAT_CONTROLLER_LAYOUT_REVERSE)
        controllerType = DEVICE_SWITCH_PRO;

    if (controllerType == DEVICE_XBOX) {
        // extra fine tuning
        char name[0x100];
        Paddleboat_getControllerName(controllerID, 0x100, name);
        if (strstr(name, "Xbox"))
            controllerType = DEVICE_XBOX;
        else if (strstr(name, "Saturn"))
            controllerType = DEVICE_SATURN;
    }

    device->active      = true;
    device->disabled    = false;
    device->gamepadType = (DEVICE_API_PDBOAT << 16) | (DEVICE_TYPE_CONTROLLER << 8) | (controllerType << 0);
    device->id          = id;

    for (int32 i = 0; i < PLAYER_COUNT; ++i) {
        if (inputSlots[i] == id) {
            inputSlotDevices[i] = device;
            device->isAssigned  = true;
        }
    }

    inputDeviceCount++;
    return device;
}

void RSDK::SKU::PaddleboatStatusCallback(const int32 jid, const Paddleboat_ControllerStatus status, void *data)
{
    (void)data;

    uint32 hash;
    char idBuffer[0x20];
    sprintf_s(idBuffer, (int32)sizeof(idBuffer), "%s%d", "PDBDevice", jid);
    GenerateHashCRC(&hash, idBuffer);

    PrintLog(PRINT_NORMAL, "callback %d %s", status, idBuffer);

    if (status == PADDLEBOAT_CONTROLLER_JUST_CONNECTED)
        SKU::InitPaddleboatInputDevice(hash, jid);
    else
        RemoveInputDevice(InputDeviceFromID(hash));
}

void RSDK::SKU::ProcessPaddleboatInputDevices()
{
    // connect/disconnect controllers
    Paddleboat_update(GetJNISetup()->env);
}

void RSDK::SKU::InitPaddleboatInputAPI()
{
    // Paddleboat_init(GetJNISetup()->env, GetJNISetup()->thiz);

    Paddleboat_setControllerStatusCallback(SKU::PaddleboatStatusCallback, NULL);

    // TODO: connect existing controllers
}