namespace SKU
{

struct InputDevicePaddleboat : InputDevice {
    void UpdateInput();
    void ProcessInput(int32 controllerID);
    void CloseDevice();

    Paddleboat_Controller_Data inputState[2];
    uint8 activeState;
    uint8 controllerID;

    uint8 stateUp;
    uint8 stateDown;
    uint8 stateLeft;
    uint8 stateRight;
    uint8 stateA;
    uint8 stateB;
    uint8 stateX;
    uint8 stateY;
    uint8 stateStart;
    uint8 stateSelect;
    uint8 stateBumper_L;
    uint8 stateBumper_R;
    uint8 stateStick_L;
    uint8 stateStick_R;
    float hDelta_L;
    float vDelta_L;
    float hDelta_R;
    float vDelta_R;
    float deltaBumper_L;
    float deltaTrigger_L;
    float deltaBumper_R;
    float deltaTrigger_R;
};

InputDevicePaddleboat *InitPaddleboatInputDevice(uint32 id, uint8 controllerID);
void InitPaddleboatInputAPI();
void ProcessPaddleboatInputDevices();

void PaddleboatStatusCallback(const int32 jid, const Paddleboat_ControllerStatus status, void *);

} // namespace SKU