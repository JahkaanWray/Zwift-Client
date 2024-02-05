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
#define TO_SEARCH_DEVICE_UUID "{0000180D-0000-1000-8000-00805F9B34FB}" // we use UUID for an HR BLE device

void somethingHappened(BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID Context);
HANDLE getBLEHandle(GUID guid);
HANDLE openBleInterfaceHandle(GUID interfaceUUID, DWORD dwDesiredAccess);
void printGUID(GUID guid);
GUID mapServiceUUID(const PBTH_LE_UUID serviceUUID);

int main()
{

    GUID bluetoothGUID = {0x781aee18, 0x7733, 0x4ce4, 0xad, 0xd0, 0x91, 0xf4, 0x1c, 0x67, 0xb5, 0x92};

    HANDLE handle = getBLEHandle(bluetoothGUID);
    printf("Handle: %p\n", handle);

    USHORT gattServiceCount;

    HRESULT hr = BluetoothGATTGetServices(handle, 0, NULL, &gattServiceCount, BLUETOOTH_GATT_FLAG_NONE);
    if (hr != HRESULT_FROM_WIN32(ERROR_MORE_DATA))
    {
        printf("unknown error: %lud\n", hr);
        return 1;
    }

    PBTH_LE_GATT_SERVICE pServiceBuffer = (PBTH_LE_GATT_SERVICE)malloc(sizeof(BTH_LE_GATT_SERVICE) * gattServiceCount);

    if (pServiceBuffer == NULL)
    {
        printf("could not find memory for service buffer\n");
        return 1;
    }
    else
    {
        memset(pServiceBuffer, 0, gattServiceCount * sizeof(BTH_LE_GATT_SERVICE));
    }

    hr = BluetoothGATTGetServices(handle, gattServiceCount, pServiceBuffer, &gattServiceCount, BLUETOOTH_GATT_FLAG_NONE);
    if (hr != S_OK)
    {
        printf("Error getting gatt service\n");
        return 1;
    }

    printf("There are %d services\n", gattServiceCount);
    printf("Size of GUID: %d\n", sizeof(GUID));
    printf("Size of long: %d\n", sizeof(long));
    for (int i = 0; i < gattServiceCount; i++)
    {
        BTH_LE_GATT_SERVICE s = pServiceBuffer[i];
        BTH_LE_UUID uuid = s.ServiceUuid;
        printf("Service %d uuid: ", i);
        if (uuid.IsShortUuid)
        {
            printf("0x%04x\n", uuid.Value.ShortUuid);
        }
        else
        {
            GUID guid = uuid.Value.LongUuid;
            printf("%08x %04x %04x ", guid.Data1, guid.Data2, guid.Data3);
            for (int j = 0; j < sizeof(guid.Data4) / sizeof(guid.Data4[0]); j++)
            {
                printf("%02x ", guid.Data4[j]);
            }
            printf("\n");
        }

        USHORT characteristicBufferCount;
        hr = BluetoothGATTGetCharacteristics(handle, &s, 0, NULL, &characteristicBufferCount, BLUETOOTH_GATT_FLAG_NONE);
        if (hr != HRESULT_FROM_WIN32(ERROR_MORE_DATA))
        {
            return 1;
        }
        PBTH_LE_GATT_CHARACTERISTIC pCharacteristicBuffer = malloc(characteristicBufferCount * sizeof(BTH_LE_GATT_CHARACTERISTIC));

        if (pCharacteristicBuffer == NULL)
        {
            printf("Could not find memory for characteristic buffer\n");
        }
        else
        {
            memset(pCharacteristicBuffer, 0, characteristicBufferCount * sizeof(BTH_LE_GATT_CHARACTERISTIC));
        }
        hr = BluetoothGATTGetCharacteristics(handle, &s, characteristicBufferCount, pCharacteristicBuffer, &characteristicBufferCount, BLUETOOTH_GATT_FLAG_NONE);
        if (hr != S_OK)
        {
            return 1;
        }
        for (int j = 0; j < characteristicBufferCount; j++)
        {
            BTH_LE_GATT_CHARACTERISTIC c = pCharacteristicBuffer[j];
            BTH_LE_UUID uuid = c.CharacteristicUuid;
            if (uuid.Value.ShortUuid != 0x2a5b)
            {
                continue;
            }
            printf("Characteristic %d uuid: ", j);
            GUID guid = {uuid.Value.ShortUuid, 0x0000, 0x1000, {0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB}};
            ;
            if (uuid.IsShortUuid)
            {
                printf("0x%04x\n", uuid.Value.ShortUuid);
            }
            else
            {
                guid = uuid.Value.LongUuid;
                printf("%08x %04x %04x ", guid.Data1, guid.Data2, guid.Data3);
                for (int k = 0; k < sizeof(guid.Data4) / sizeof(guid.Data4[0]); k++)
                {
                    printf("%02x ", guid.Data4[k]);
                }
                printf("\n");
            }

            if (!c.IsNotifiable)
            {
                printf("This characteristic is not notifiable\n");
                continue;
            }
            printf("This characteristic is notifiable\n");

            // Determine characteristic value buffer size;
            handle = openBleInterfaceHandle(mapServiceUUID(&uuid), GENERIC_READ | GENERIC_WRITE);
            printf("Handle: %p\n", handle);

            BLUETOOTH_GATT_EVENT_HANDLE EventHandle;
            BLUETOOTH_GATT_VALUE_CHANGED_EVENT_REGISTRATION EventParameterIn;
            BTH_LE_GATT_EVENT_TYPE EventType = CharacteristicValueChangedEvent;
            EventParameterIn.Characteristics[0] = c;
            EventParameterIn.NumCharacteristics = 1;
            hr = BluetoothGATTRegisterEvent(handle,
                                            EventType,
                                            &EventParameterIn,
                                            somethingHappened,
                                            NULL,
                                            &EventHandle,
                                            BLUETOOTH_GATT_FLAG_NONE);

            if (hr == ERROR_ACCESS_DENIED)
            {
                printf("Access Denied\n");
            }
            else if (hr == ERROR_INVALID_PARAMETER)
            {
                printf("Invalid Parameter\n");
            }
            if (hr != S_OK)
            {
                printf("Error registering event\n");
                printf("%d\n", GetLastError());
                printf("%d\n", hr);
                continue;
            }
            printf("Successfully registered event\n");
        }
        printf("%d\n", s.AttributeHandle);
    }
}

void somethingHappened(BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID Context)
{

    printf("Notification received\n");
    PBLUETOOTH_GATT_VALUE_CHANGED_EVENT ValueChangedEventParameters = EventOutParameter;

    HRESULT hr;
    if (ValueChangedEventParameters->CharacteristicValue->DataSize == 0)
    {
        hr = E_FAIL;
        printf("datasize 0\n");
    }
    else
    {
        printf("HR ");

        printf("%d\n", ValueChangedEventParameters->CharacteristicValue->DataSize);
    }
}

HANDLE getBLEHandle(GUID guid)
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

    BOOL res = SetupDiEnumDeviceInfo(hDI, 0, &dd);
    if (!res)
    {
        printf("Failed to enumerate device info\n");
        return NULL;
    }
    printf("Successfully enumerated device info\n");

    DWORD bufferSize = 1;
    PBYTE text = malloc(bufferSize * sizeof(BYTE));

    SetupDiGetDeviceInstanceId(hDI, &dd, text, bufferSize, &bufferSize);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        text = realloc(text, bufferSize * sizeof(BYTE));
        SetupDiGetDeviceInstanceId(hDI, &dd, text, bufferSize, &bufferSize);
    }
    printf("%s\n", text);
    SP_DEVICE_INTERFACE_DATA did;
    HANDLE handle = NULL;

    hDI = SetupDiGetClassDevs(&guid, text, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (hDI == INVALID_HANDLE_VALUE)
    {
        printf("Unable to open device information\n");
        printf("Error code: %d\n", GetLastError());
        return NULL;
    }
    printf("Successfully open device information\n");

    did.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    dd.cbSize = sizeof(SP_DEVINFO_DATA);

    res = SetupDiEnumDeviceInterfaces(hDI, NULL, &guid, 0, &did);
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
    for(int i = 0; i < size; i++)
    {
        printf("%c", didd.DevicePath[i]);
    }
    printf("\n");
    handle = CreateFile(didd.DevicePath,
                        GENERIC_WRITE | GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);
    return handle;
}

void printGUID(GUID guid)
{
    printf("%08x %04x %04x ", guid.Data1, guid.Data2, guid.Data3);
    for (int i = 0; i < 8; i++)
    {
        printf("%02x ", guid.Data4[i]);
    }
    printf("\n");
}

GUID mapServiceUUID(const PBTH_LE_UUID serviceUUID)
{
    if (serviceUUID->IsShortUuid)
    {
        GUID guid = {serviceUUID->Value.ShortUuid, 0x0000, 0x1000, {0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB}};
        return guid;
    }
    else
    {
        return serviceUUID->Value.LongUuid;
    }
}

HANDLE openBleInterfaceHandle(GUID interfaceUUID, DWORD dwDesiredAccess)
{
    HDEVINFO hDI;
    SP_DEVICE_INTERFACE_DATA did;
    SP_DEVINFO_DATA dd;
    GUID BluetoothInterfaceGUID = interfaceUUID;
    HANDLE handle = NULL;

    if ((hDI = SetupDiGetClassDevs(&BluetoothInterfaceGUID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT)) == INVALID_HANDLE_VALUE)
    {
        return NULL;
    }

    did.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    dd.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(hDI, NULL, &BluetoothInterfaceGUID, i, &did); i++)
    {
        SP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData;

        DeviceInterfaceDetailData.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        DWORD size = 0;

        if (!SetupDiGetDeviceInterfaceDetail(hDI, &did, NULL, 0, &size, NULL))
        {
            int err = GetLastError();

            if (err == ERROR_NO_MORE_ITEMS)
                break;

            PSP_DEVICE_INTERFACE_DETAIL_DATA pInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)GlobalAlloc(GPTR, size);

            if (pInterfaceDetailData != NULL)
            {
                pInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

                if (!SetupDiGetDeviceInterfaceDetail(hDI, &did, pInterfaceDetailData, size, &size, &dd))
                    break;

                handle = CreateFile(
                    pInterfaceDetailData->DevicePath,
                    dwDesiredAccess,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

                GlobalFree(pInterfaceDetailData);

                if (handle == INVALID_HANDLE_VALUE)
                {
                    return NULL;
                }
            }
            else
            {
                return NULL;
            }
        }
    }

    SetupDiDestroyDeviceInfoList(hDI);
    return handle;
}