import asyncio
from bleak import BleakScanner, BleakClient, BleakGATTCharacteristic

async def main():
    bleakScanner = BleakScanner()
    await bleakScanner.start()
    await asyncio.sleep(5.0)
    await bleakScanner.stop()
    devices = bleakScanner.discovered_devices_and_advertisement_data
    bikeAddress = None
    for key, (dev, ad) in devices.items():
        if '00001826-0000-1000-8000-00805f9b34fb' in ad.service_data:
            print(dev)
            print(ad.service_data)
            bikeAddress = key
            break
    if bikeAddress is None:
        print('No bike found')
        return
    client = BleakClient(bikeAddress)
    await client.connect()
    print('Connected')
    services = client.services
    print('Services')
    for service in services:
        print(service)
        for characteristic in service.characteristics:
            print("\t", characteristic)
            for prop in characteristic.properties:
                print("\t\t", prop)
    print(await client.read_gatt_char('00002a00-0000-1000-8000-00805f9b34fb'))
    print(await client.read_gatt_char('00002ad8-0000-1000-8000-00805f9b34fb'))
    print(await client.read_gatt_char('00002ad6-0000-1000-8000-00805f9b34fb'))
    await client.disconnect()


asyncio.run(main())