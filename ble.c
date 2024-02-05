// How to get notification values from a HR BLE monitor
// Ensure that you have paired the HR BLE monitor with the computer

#include <stdio.h>
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <bthdef.h>
#include <Bluetoothleapis.h>
#pragma comment(lib, "SetupAPI")
#pragma comment(lib, "BluetoothApis.lib")

typedef struct
{
    HANDLE handle;
    PBYTE name;
    PBYTE hardwareId;
    PBYTE deviceInstanceId;
} BleDevice;

typedef struct
{
    BleDevice *list;
    int numDevices;
} BleDevices;

PBYTE GetDeviceRegistryProperty(HDEVINFO hDI, PSP_DEVINFO_DATA pdd, DWORD property);
PBYTE GetDeviceInstanceId(HDEVINFO hDI, PSP_DEVINFO_DATA pdd);
HANDLE GetBLEHandle(GUID guid, PBYTE instanceId);
BleDevices *enumerateDevices(GUID guid);

int main()
{
    GUID bluetoothGUID = {0x781aee18, 0x7733, 0x4ce4, 0xad, 0xd0, 0x91, 0xf4, 0x1c, 0x67, 0xb5, 0x92};

    BleDevices *devices = enumerateDevices(bluetoothGUID);

    printf("%d device(s)\n", devices->numDevices);
    for (int i = 0; i < devices->numDevices; i++)
    {
        printf("Device name: %s\n", devices->list->name);
        printf("Device instance ID: %s\n", devices->list->deviceInstanceId);
        printf("Hardware ID: %s\n", devices->list->hardwareId);
        printf("Handle : %p\n", devices->list->handle);
    }
}

BleDevices *enumerateDevices(GUID guid)
{
    HDEVINFO hDI = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (hDI == INVALID_HANDLE_VALUE)
    {
        printf("Error: Invalid handle value\n");
        return NULL;
    }
    printf("Valid Device Info Handle\n");

    SP_DEVINFO_DATA dd;
    dd.cbSize = sizeof(SP_DEVINFO_DATA);

    int numDevices;
    for (numDevices = 0; SetupDiEnumDeviceInfo(hDI, numDevices, &dd); numDevices++)
    {
    }
    BleDevice *devices = malloc(numDevices * sizeof(BleDevice));
    for (int i = 0; SetupDiEnumDeviceInfo(hDI, i, &dd); i++)
    {
        PBYTE name = GetDeviceRegistryProperty(hDI, &dd, SPDRP_FRIENDLYNAME);
        PBYTE hardwareId = GetDeviceRegistryProperty(hDI, &dd, SPDRP_HARDWAREID);
        PBYTE instanceId = GetDeviceInstanceId(hDI, &dd);
        HANDLE handle = GetBLEHandle(guid, instanceId);
        BleDevice d = {handle, name, hardwareId, instanceId};
        devices[i] = d;
    }
    BleDevices *b = malloc(sizeof(BleDevices));
    b->list = devices;
    b->numDevices = numDevices;

    return b;
}

PBYTE GetDeviceRegistryProperty(HDEVINFO hDI, PSP_DEVINFO_DATA pdd, DWORD property)
{
    DWORD bufferSize = 0;
    PBYTE text = malloc(bufferSize * sizeof(BYTE));

    SetupDiGetDeviceRegistryProperty(hDI, pdd, property, NULL, text, bufferSize, &bufferSize);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        text = realloc(text, bufferSize * sizeof(BYTE));
        SetupDiGetDeviceRegistryProperty(hDI, pdd, property, NULL, text, bufferSize, &bufferSize);
    }
    printf("%s\n", text);
    return text;
}

PBYTE GetDeviceInstanceId(HDEVINFO hDI, PSP_DEVINFO_DATA pdd)
{
    DWORD bufferSize = 1;
    PBYTE text = malloc(bufferSize * sizeof(BYTE));

    SetupDiGetDeviceInstanceId(hDI, pdd, text, bufferSize, &bufferSize);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        text = realloc(text, bufferSize * sizeof(BYTE));
        SetupDiGetDeviceInstanceId(hDI, pdd, text, bufferSize, &bufferSize);
    }
    return text;
}

HANDLE GetBLEHandle(GUID guid, PBYTE instanceId)
{
    printf("%s\n", instanceId);
    SP_DEVINFO_DATA dd;
    SP_DEVICE_INTERFACE_DATA did;
    HDEVINFO hDI = SetupDiGetClassDevs(&guid, instanceId, NULL, DIGCF_DEVICEINTERFACE);
    if (hDI == INVALID_HANDLE_VALUE)
    {
        printf("Error: Invalid handle value\n");
        return NULL;
    }
    printf("Successfully open device info\n");

    dd.cbSize = sizeof(SP_DEVINFO_DATA);
    did.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    int res = SetupDiEnumDeviceInterfaces(hDI, NULL, &guid, 0, &did);
    printf("res: %d\n", res);

    SP_DEVICE_INTERFACE_DETAIL_DATA didd;
    didd.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    DWORD size = 0;

    res = SetupDiGetDeviceInterfaceDetail(hDI, &did, NULL, 0, &size, NULL);
    if (!res)
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            printf("didd size: %d\n", size);
            res = SetupDiGetDeviceInterfaceDetail(hDI, &did, &didd, size, &size, &dd);
            if (!res)
            {
                return NULL;
            }
            printf("Successfully got interface detail\n");
        }
    }

    printf("%s\n", didd.DevicePath);
    HANDLE handle = CreateFile(didd.DevicePath,
                               GENERIC_WRITE | GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL);
    return handle;
}