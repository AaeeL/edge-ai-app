#Startpoint of the python program
import logging
import struct
import time
import asyncio
from bleak import BleakScanner
from bleak import BleakClient
#function imports
from get_ruuvitag_data import get_data
from utils.calc_dew_point import calculate
from utils.calc_slope import calc_slope

#MAC addresses of the Ruuvitag sensors
macs = ['EA:33:0E:59:73:83', 'C5:74:D0:24:B4:77']
async def main():

    logging.basicConfig(filename='data.log', format='%(asctime)s %(message)s', datefmt='%d/%m/%Y %I:%M:%S %p', encoding='utf-8', level=logging.INFO)

    while True:
        readings = get_data(macs)
        try:
            dewpoint = calculate(readings['EA:33:0E:59:73:83']['humidity'], readings['EA:33:0E:59:73:83']['temperature'])
            #TODO: Kulmakerroin väärästä lämpötilasta!!!!! VAIHDA!!!!!!!
            slope = calc_slope(dewpoint, readings['C5:74:D0:24:B4:77']['temperature'])
            print("Here is dewpoint and slope: ")
            print(dewpoint)
            print(slope)

            dewpoint_to_send = bytearray(struct.pack("f", dewpoint))
            slope_to_send = bytearray(struct.pack("f", slope))
            temp_to_send = bytearray(struct.pack("f",readings['C5:74:D0:24:B4:77']['temperature']))

            address = "93:3e:66:b9:26:fb"
            async with BleakClient(address) as client:
                await client.write_gatt_char("19b10001-e8f2-537e-4f6c-d104768a1215",dewpoint_to_send)
                time.sleep(2)
                await client.write_gatt_char("19b10001-e8f2-537e-4f6c-d104768a1214",temp_to_send)
                time.sleep(2)
                await client.write_gatt_char("19b10001-e8f2-537e-4f6c-d104768a1216",slope_to_send)

                time.sleep(5)

                raw_inference = await client.read_gatt_char("19b10001-e8f2-537e-4f6c-d104768a1217")
                inference = struct.unpack('f', raw_inference)
                print(inference[0])

                logging.info("Dewpoint value: %.2f", dewpoint)
                logging.info("Temp value: %.2f", readings['C5:74:D0:24:B4:77']['temperature'])
                logging.info("Slope value: %.2f", slope)
                logging.info("Inference value: %.2f", inference[0])

                print("+++ Communication over, disconnecting +++")
        except:
            logging.info("Error occurred")

        print("Going to sleep for 1 hour")
        logging.info("Going to sleep for 1 hour")
        time.sleep(3600)

asyncio.run(main())
