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
        data = await client.read_gatt_char("00002acc-0000-1000-8000-00805f9b34fb")
        print("Data: {0}".format(data))
        char_uuid = "00002a63-0000-1000-8000-00805f9b34fb"
        
        await client.write_gatt_char("00002ad9-0000-1000-8000-00805f9b34fb", bytearray([0x00]))
        await client.write_gatt_char("00002ad9-0000-1000-8000-00805f9b34fb", bytearray([0x01]))
        await client.write_gatt_char("00002ad9-0000-1000-8000-00805f9b34fb", bytearray([0x05, 0x64]))
        def powcallback(sender: BleakGATTCharacteristic, data: bytearray):
            flags = data[0:2]
            power = data[2:4]
            #convert to int
            power = int.from_bytes(power, byteorder='little', signed=False)
            print("Received power: {0}".format(power))
        
        def callback(sender: BleakGATTCharacteristic, data: bytearray):
            print("Received: {0}".format(data))

        #await client.start_notify(char_uuid, powcallback)
        await client.start_notify("00002a5b-0000-1000-8000-00805f9b34fb", callback)

    
        await asyncio.sleep(500.0)
    
asyncio.run(main())