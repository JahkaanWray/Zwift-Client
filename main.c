#include <windows.h>
#include <stdio.h>
#include <initguid.h>
#include <bluetoothapis.h>
#include <bluetoothleapis.h>
#include <bthledef.h>
#include <devguid.h>
#include <setupapi.h>

void CALLBACK notify(BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID Context)
{
    printf("Event\n");
    switch (EventType)
    {
    case CharacteristicValueChangedEvent:
        printf("CharacteristicValueChangedEvent\n");
        break;
    default:
        printf("Unknown event\n");
        break;
    }
}

int main()
{
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_BLUETOOTHLE_DEVICE_INTERFACE, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if(hDevInfo == INVALID_HANDLE_VALUE)
    {
        printf("Error: Invalid handle value\n");
        return 1;
    }
    printf("Valid Device Info Handle\n");

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    BOOL res = SetupDiEnumDeviceInfo(hDevInfo, 0, &devInfoData);
    if(!res)
    {
        printf("Error: SetupDiEnumDeviceInfo failed\n");
        printf("Error code: %d\n", GetLastError());
        return 1;
    }
    printf("Valid Device Info Data\n");


    DWORD bufferSize = 1;
    PBYTE text = malloc(bufferSize * sizeof(BYTE));

    SetupDiGetDeviceInstanceId(hDevInfo, &devInfoData, text, bufferSize, &bufferSize);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        text = realloc(text, bufferSize * sizeof(BYTE));
        if(text == NULL)
        {
            printf("Error: realloc failed\n");
            return 1;
        }
        SetupDiGetDeviceInstanceId(hDevInfo, &devInfoData, text, bufferSize, &bufferSize);
    }
    printf("Device Instance ID: %s\n", text);

    SP_DEVICE_INTERFACE_DATA devInterfaceData;

    hDevInfo = SetupDiGetClassDevs(&GUID_BLUETOOTHLE_DEVICE_INTERFACE, text, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if(hDevInfo == INVALID_HANDLE_VALUE)
    {
        printf("Error: Invalid handle value\n");
        return 1;
    }
    printf("Valid Device Info Handle\n");

    devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    res = SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_BLUETOOTHLE_DEVICE_INTERFACE, 0, &devInterfaceData);
    if(!res)
    {
        printf("Error: SetupDiEnumDeviceInterfaces failed\n");
        printf("Error code: %d\n", GetLastError());
        return 1;
    }
    printf("Valid Device Interface Data\n");

    SP_DEVICE_INTERFACE_DETAIL_DATA devInterfaceDetailData;
    devInterfaceDetailData.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    DWORD dwSize = 0;

    res = SetupDiGetDeviceInterfaceDetail(hDevInfo, &devInterfaceData, NULL, 0, &dwSize, NULL);
    if(!res)
    {
        printf("Error: SetupDiGetDeviceInterfaceDetail failed\n");
        if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            printf("Error: Insufficient buffer\n");
            printf("Buffer size: %d\n", dwSize);
            WCHAR *buffer = malloc(dwSize * sizeof(BYTE));
            res = SetupDiGetDeviceInterfaceDetail(hDevInfo, &devInterfaceData, &devInterfaceDetailData, dwSize, &dwSize, &devInfoData);
            if(!res)
            {
                printf("Error: SetupDiGetDeviceInterfaceDetail failed\n");
                printf("Error code: %d\n", GetLastError());
                return 1;
            }
        }else {
            printf("Error code: %d\n", GetLastError());
            return 1;
        }
    }

    printf("%s\n", devInterfaceDetailData.DevicePath);
    HANDLE handle = CreateFile(devInterfaceDetailData.DevicePath,
                        GENERIC_WRITE | GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);
    printf("%p\n", handle);
    printf("Hello, World!\n");
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
    for(int i = 0; i < gattServiceCount; i++)
    {
        printf("Service %d: %d\n", i, pServiceBuffer[i].AttributeHandle);
        if(pServiceBuffer[i].ServiceUuid.IsShortUuid){
            printf("Short UUID: %04x\n", pServiceBuffer[i].ServiceUuid.Value.ShortUuid);
        }
        USHORT gattCharacteristicCount;
        BluetoothGATTGetCharacteristics(handle, &pServiceBuffer[i], 0, NULL, &gattCharacteristicCount, BLUETOOTH_GATT_FLAG_NONE);
        printf("Service %d has %d characteristics\n", i, gattCharacteristicCount);
        PBTH_LE_GATT_CHARACTERISTIC pCharacteristicBuffer = (PBTH_LE_GATT_CHARACTERISTIC)malloc(sizeof(BTH_LE_GATT_CHARACTERISTIC) * gattCharacteristicCount);
        if (pCharacteristicBuffer == NULL)
        {
            printf("could not find memory for characteristic buffer\n");
            return 1;
        }
        else
        {
            memset(pCharacteristicBuffer, 0, gattCharacteristicCount * sizeof(BTH_LE_GATT_CHARACTERISTIC));
        }
        hr = BluetoothGATTGetCharacteristics(handle, &pServiceBuffer[i], gattCharacteristicCount, pCharacteristicBuffer, &gattCharacteristicCount, BLUETOOTH_GATT_FLAG_NONE);
        for(int j = 0; j < gattCharacteristicCount; j++)
        {
            printf("Characteristic %d: ", j);
            if(pCharacteristicBuffer[j].CharacteristicUuid.IsShortUuid){
                printf("Short UUID: %04x\n", pCharacteristicBuffer[j].CharacteristicUuid.Value.ShortUuid);
            } else {
                printf("Long UUID: ");
                printf("\n");
            }
            if(pCharacteristicBuffer[j].IsBroadcastable){
                printf("Broadcastable\n");
            }
            if(pCharacteristicBuffer[j].IsReadable){
                printf("Readable\n");
            }
            if(pCharacteristicBuffer[j].IsWritable){
                printf("Writable\n");
            }
            if(pCharacteristicBuffer[j].IsWritableWithoutResponse){
                printf("Writable without response\n");
            }
            if(pCharacteristicBuffer[j].IsSignedWritable){
                printf("Signed writable\n");
            }
            if(pCharacteristicBuffer[j].IsNotifiable){
                printf("Notifiable\n");
            }
            if(pCharacteristicBuffer[j].IsIndicatable){
                printf("Indicatable\n");
            }
            if(pCharacteristicBuffer[j].HasExtendedProperties){
                printf("Has extended properties\n");
            }
            if(pCharacteristicBuffer[j].CharacteristicUuid.Value.ShortUuid != 0x2A00){
                continue;
            }
            printf("Cycling Power Measurement\n");
            USHORT size;
            hr = BluetoothGATTGetCharacteristicValue(handle, &pCharacteristicBuffer[j], 0, NULL, &size, BLUETOOTH_GATT_FLAG_NONE);
            if(hr != HRESULT_FROM_WIN32(ERROR_MORE_DATA))
            {
                printf("Error: BluetoothGATTGetCharacteristicValue failed\n");
                printf("Error code: %08x\n", hr);
                return 1;
            }
            printf("Size: %d\n", size);
            BTH_LE_GATT_CHARACTERISTIC_VALUE *value = (BTH_LE_GATT_CHARACTERISTIC_VALUE *)malloc(size);
            if(value == NULL)
            {
                printf("Error: malloc failed\n");
                return 1;
            }
            hr = BluetoothGATTGetCharacteristicValue(handle, &pCharacteristicBuffer[j], size, value, &size, BLUETOOTH_GATT_FLAG_NONE);
            if(hr != S_OK)
            {
                printf("Error: BluetoothGATTGetCharacteristicValue failed\n");
                printf("Error code: %08x\n", hr);
                return 1;
            }
            printf("Value: ");
            for(int k = 0; k < size; k++)
            {
                printf("%02x ", value->Data[k]);
            }
            printf("\n");
        }

    }
    return 0;
}
