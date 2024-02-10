import asyncio
from bleak import BleakScanner, BleakClient, BleakGATTCharacteristic

workout = {
    'name': 'Test',
    'description': 'Test workout',
    'segments': [
        {
            'name': 'Warmup',
            'description': 'Warmup',
            'duration': 20,
            'power': 100
        },
        {
            'name': 'Interval',
            'description': 'Interval',
            'duration': 20,
            'power': 200
        },
        {
            'name': 'Cooldown',
            'description': 'Cooldown',
            'duration': 20,
            'power': 100
        }
    ]
}

def power_callback(sender, data):
    flags = data[0:2]
    power = int.from_bytes(data[2:4], byteorder='little')
    print(f'Power: {power}')

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

    print("Starting Workout")
    print("Workout Name:", workout['name'])
    print("Workout Description:", workout['description'])
    await client.start_notify('00002a63-0000-1000-8000-00805f9b34fb', power_callback)
    await client.write_gatt_char('00002ad9-0000-1000-8000-00805f9b34fb', bytearray([0x00]))
    for segment in workout['segments']:
        print("Segment Name:", segment['name'])
        print("Segment Description:", segment['description'])
        print("Segment Duration:", segment['duration'])
        print("Segment Power:", segment['power'])
        power = segment['power'].to_bytes(2, byteorder='little')
        data = [0x05]
        data.extend(power)
        print(data)
        await client.write_gatt_char('00002ad9-0000-1000-8000-00805f9b34fb', bytearray(data))
        await asyncio.sleep(segment['duration'])
    await client.write_gatt_char('00002ad9-0000-1000-8000-00805f9b34fb', bytearray([0x01]))
    #print(await client.read_gatt_char('00002a00-0000-1000-8000-00805f9b34fb'))
    #print(await client.read_gatt_char('00002ad8-0000-1000-8000-00805f9b34fb'))
    #print(await client.read_gatt_char('00002ad6-0000-1000-8000-00805f9b34fb'))
    #await client.write_gatt_char('00002ad9-0000-1000-8000-00805f9b34fb', bytearray([0x00]))
    #await client.write_gatt_char('00002ad9-0000-1000-8000-00805f9b34fb', bytearray([0x01]))
    #await client.write_gatt_char('00002ad9-0000-1000-8000-00805f9b34fb', bytearray([0x05, 0xc8]))
    #await client.start_notify('00002a63-0000-1000-8000-00805f9b34fb', power_callback)
    #await asyncio.sleep(300.0)
    await client.disconnect()


asyncio.run(main())