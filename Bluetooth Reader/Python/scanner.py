#
# Original code from Duane Kaufman in 2017-2018
# Uses BlueGiga client for BLE connectivity from Silicon Labs
# https://www.silabs.com/products/development-tools/software/bluegiga-bluetooth-smart-software-stack
#
# Has never been tested on a RaspberryPi, but might work.
#

from bgapi.module import BlueGigaClient
import struct

#CLIENT_SERIAL = "COM4"
CLIENT_SERIAL = "COM3"

# 00 b5:18:83 13:00:18 FW=09.10 B=059% #=0258 T=74F H=50 W=1.985 L=2.309(10523) R=-0.324(-9410)

def test_simple_scan(ble_client):
    """
    Client scans in observation mode.
    """
    ble_client.reset_ble_state()
    responses = ble_client.scan_all(timeout=6)
    return responses


if __name__ == "__main__":
    ble_client = BlueGigaClient(port=CLIENT_SERIAL, baud=115200, timeout=0.1)
    ResponseArray = test_simple_scan(ble_client)
    for Response in ResponseArray:
        print("\r\nResponse length:" + str(len(Response.data)))
        counter = 0
        for ch in Response.data:
            # print(counter, str(ord(ch)))
            print(counter, str((ch)))
            counter += 1
        if len(Response.data) > 30:
            print("\r\nScale advertisement:")
            print("IF Model#: {0}".format(struct.unpack_from("b", Response.data, offset=10)[0]))
            print("FW Version: {0}.{1}".format(struct.unpack_from("b", Response.data, offset=12)[0], struct.unpack_from("b", Response.data, offset=11)[0]))
            print("ID#: {0}".format(struct.unpack_from("b", Response.data, offset=13)[0]))
            print("Battery level: {0}".format(struct.unpack_from("b", Response.data, offset=14)[0]))
            print("Elapsed: {0}".format(struct.unpack_from("h", Response.data, offset=15)[0]))
            TemperatureRawInt = struct.unpack_from("h", Response.data, offset=17)[0]
            # From Datasheet SHT3x-DIS
            TemperatureF = TemperatureRawInt * 315 / (2**16-1) - 49
            print("Temperature(F): {0}".format(TemperatureF))
            print("Humidity: {0}".format(struct.unpack_from("b", Response.data, offset=24)[0]))
            if Response.data[25] == 255:
                PrePendByteStr = '\xFF'
            else:
                PrePendByteStr = '\x00'
            WeightLInt = struct.unpack_from(">i", PrePendByteStr + str(Response.data[25:]), offset=0)[0]
            print("WeightL: {0}".format(WeightLInt))
            if Response.data[28] == 255:
                PrePendByteStr = '\xFF'
            else:
                PrePendByteStr = '\x00'
            WeightRInt = struct.unpack_from(">i", PrePendByteStr + str(Response.data[28:]), offset=0)[0]
            print("WeightR: {0}".format(WeightRInt))



