#include <windows.h>
#include <winrt/windows.devices.bluetooth.h>
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

void print_bluetooth_UUID(BTH_LE_UUID uuid)
{
    if (uuid.IsShortUuid)
    {
        printf("0x%04x", uuid.Value.ShortUuid);
    }
    else
    {
        printf("{");
        printf("%08x-", uuid.Value.LongUuid.Data1);
        printf("%04x-", uuid.Value.LongUuid.Data2);
        printf("%04x", uuid.Value.LongUuid.Data3);
        for (int i = 0; i < 8; i++)
        {
            printf("-%02x", uuid.Value.LongUuid.Data4[i]);
        }
        printf("}");
    }
}

int main()
{
    // Get device information set handle
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_BLUETOOTHLE_DEVICE_INTERFACE, NULL, NULL, DIGCF_DEVICEINTERFACE);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        printf("Error: Invalid handle value\n");
        return 1;
    }
    printf("Valid Device Information Set Handle\n");

    // Get device information data
    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    BOOL res = SetupDiEnumDeviceInfo(hDevInfo, 0, &devInfoData);
    if (!res)
    {
        printf("Error: SetupDiEnumDeviceInfo failed\n");
        printf("Error code: %d\n", GetLastError());
        return 1;
    }
    printf("Valid Device Information Data\n");

    // Get device name
    DWORD bufferSize = 1;
    PBYTE text = malloc(bufferSize * sizeof(BYTE));
    SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL, text, bufferSize, &bufferSize);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        text = realloc(text, bufferSize * sizeof(BYTE));
        if (text == NULL)
        {
            printf("Error: realloc failed\n");
            return 1;
        }
        SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL, text, bufferSize, &bufferSize);
    }
    printf("Device Name: %s\n", text);

    // Get device instance ID
    bufferSize = 1;
    text = malloc(bufferSize * sizeof(BYTE));
    SetupDiGetDeviceInstanceId(hDevInfo, &devInfoData, text, bufferSize, &bufferSize);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        text = realloc(text, bufferSize * sizeof(BYTE));
        if (text == NULL)
        {
            printf("Error: realloc failed\n");
            return 1;
        }
        SetupDiGetDeviceInstanceId(hDevInfo, &devInfoData, text, bufferSize, &bufferSize);
    }
    printf("Device Instance ID: %s\n", text);

    // Get device interface data
    SP_DEVICE_INTERFACE_DATA devInterfaceData;
    hDevInfo = SetupDiGetClassDevs(&GUID_BLUETOOTHLE_DEVICE_INTERFACE, text, NULL, DIGCF_DEVICEINTERFACE);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        printf("Error: Invalid handle value\n");
        return 1;
    }
    printf("Valid Device Interface Handle\n");

    // Get device interface data
    devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    res = SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_BLUETOOTHLE_DEVICE_INTERFACE, 0, &devInterfaceData);
    if (!res)
    {
        printf("Error: SetupDiEnumDeviceInterfaces failed\n");
        printf("Error code: %d\n", GetLastError());
        return 1;
    }
    printf("Valid Device Interface Data\n");

    // Get device interface detail data
    DWORD detailSize = 1;
    PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = malloc(detailSize * sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA));
    res = SetupDiGetDeviceInterfaceDetail(hDevInfo, &devInterfaceData, NULL, 0, &detailSize, NULL);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        detailData = realloc(detailData, detailSize * sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA));
        if (detailData == NULL)
        {
            printf("Error: realloc failed\n");
            return 1;
        }
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        res = SetupDiGetDeviceInterfaceDetail(hDevInfo, &devInterfaceData, detailData, detailSize, &detailSize, NULL);
    }
    printf("Device Interface Detail Data: %s\n", detailData->DevicePath);

    // Open device handle
    HANDLE hDevice = CreateFile(detailData->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        printf("Error: Invalid handle value\n");
        return 1;
    }
    printf("Valid Device Handle: %p\n", hDevice);

    // Get device services
    USHORT numServices = 0;
    res = BluetoothGATTGetServices(hDevice, 0, NULL, &numServices, BLUETOOTH_GATT_FLAG_NONE);
    if (!res)
    {
        printf("Error: BluetoothGATTGetServices failed\n");
        printf("Error code: %d\n", GetLastError());
        return 1;
    }
    printf("Number of Services: %d\n", numServices);

    PBTH_LE_GATT_SERVICE services = malloc(numServices * sizeof(BTH_LE_GATT_SERVICE));
    res = BluetoothGATTGetServices(hDevice, numServices, services, &numServices, BLUETOOTH_GATT_FLAG_NONE);
    if (res != S_OK)
    {
        printf("Error: BluetoothGATTGetServices failed\n");
        printf("Error code: %d\n", GetLastError());
        return 1;
    }
    printf("Successfully retrieved services\n");

    for (int i = 0; i < numServices; i++)
    {
        printf("Service %d: ", i);
        print_bluetooth_UUID(services[i].ServiceUuid);
        printf("\n");

        // Get characteristics
        USHORT numCharacteristics = 0;
        res = BluetoothGATTGetCharacteristics(hDevice, &services[i], 0, NULL, &numCharacteristics, BLUETOOTH_GATT_FLAG_NONE);
        if (!res)
        {
            printf("Error: BluetoothGATTGetCharacteristics failed\n");
            printf("Error code: %d\n", GetLastError());
            return 1;
        }
        printf("Number of Characteristics: %d\n", numCharacteristics);
        PBTH_LE_GATT_CHARACTERISTIC characteristics = malloc(numCharacteristics * sizeof(BTH_LE_GATT_CHARACTERISTIC));
        res = BluetoothGATTGetCharacteristics(hDevice, &services[i], numCharacteristics, characteristics, &numCharacteristics, BLUETOOTH_GATT_FLAG_NONE);
        if (res != S_OK)
        {
            printf("Error: BluetoothGATTGetCharacteristics failed\n");
            printf("Error code: %d\n", GetLastError());
            return 1;
        }
        for (int j = 0; j < numCharacteristics; j++)
        {
            printf("Characteristic %d: ", j);
            print_bluetooth_UUID(characteristics[j].CharacteristicUuid);
            printf("\n");
            if (characteristics[j].IsNotifiable)
            {
                printf("Characteristic is notifiable\n");
            }
            if (characteristics[j].IsReadable)
            {
                printf("Characteristic is readable\n");
            }
            if (characteristics[j].IsWritable)
            {
                printf("Characteristic is writable\n");
            }
            if (characteristics[j].CharacteristicUuid.Value.ShortUuid == 0x2a00)
            {
                printf("Found device name characteristic\n");
                // Read device name
                USHORT valueSize = 0;
                res = BluetoothGATTGetCharacteristicValue(hDevice, &characteristics[j], 0, NULL, &valueSize, BLUETOOTH_GATT_FLAG_NONE);
                printf("Return value: %x\n", res);
                if (!res)
                {
                    printf("Error: BluetoothGATTGetCharacteristicValue failed\n");
                    printf("Error code: %d\n", GetLastError());
                    return 1;
                }
                printf("Value Size: %d\n", valueSize);
                PBTH_LE_GATT_CHARACTERISTIC_VALUE value = (PBTH_LE_GATT_CHARACTERISTIC_VALUE)malloc(valueSize * sizeof(BYTE));
                res = BluetoothGATTGetCharacteristicValue(hDevice, &characteristics[j], valueSize, value, NULL, BLUETOOTH_GATT_FLAG_NONE);
                printf("Return value: %x\n", res);
                if (!res)
                {
                    printf("Error: BluetoothGATTGetCharacteristicValue failed\n");
                    printf("Error code: %d\n", GetLastError());
                    return 1;
                }
                printf("Device Name: %s\n", (char *)value);
            }
            // Register for notifications
        }
    }
    return 0;
}