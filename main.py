import asyncio
from bleak import BleakScanner, BleakClient, BleakGATTCharacteristic

async def main():
    #devices = await BleakScanner.discover()
    #for d in devices:
    #    print(d)
    async with BleakClient("ED:65:75:D9:CF:6C") as client:
        print("Connected: {0}".format(client.is_connected))
        for s in client.services:
            print(s)
        data = await client.read_gatt_char("00002a65-0000-1000-8000-00805f9b34fb")
        print("Data: {0}".format(data))
        char_uuid = "00002a63-0000-1000-8000-00805f9b34fb"
        def callback(sender: BleakGATTCharacteristic, data: bytearray):
            flags = data[0:2]
            power = data[2:4]
            #convert to int
            power = int.from_bytes(power, byteorder='little', signed=False)
            print("Received power: {0}".format(power))

        await client.start_notify(char_uuid, callback)
    
        await asyncio.sleep(500.0)
    
asyncio.run(main())