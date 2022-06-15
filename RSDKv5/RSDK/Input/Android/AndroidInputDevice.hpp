namespace SKU
{

struct InputDeviceAndroid : InputDevice {
    void UpdateInput();
    void ProcessInput(int32 controllerID);
    void CloseDevice();
};

InputDeviceAndroid *InitAndroidInputDevice(uint32 id, uint8 controllerID);
void InitAndroidInputAPI();

} // namespace SKU