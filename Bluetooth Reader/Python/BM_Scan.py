__author__ = "Jesse Ross-Jones"
__license__ = "Public Domain"
__version__ = "1.0"

##Tested with Python 2.7
##Using bluepy, scan for bluetooth devices for 15 seconds.
##Search list of found devices and for devices matching broodminder manufacture data
##Decode and print the advertising data

from bluepy.btle import Scanner, DefaultDelegate

def byte(str, byteNum):
    return str[byteNum*2]+str[byteNum*2+1]
    #https://stackoverflow.com/questions/5649407/hexadecimal-string-to-byte-array-in-python

def checkBM(data):
    check = False
    byteCheck = 0
    BMIFLLC = str("8d02")
    #print(byte(data,byteCheck))
    if (BMIFLLC == byte(data,byteCheck) + byte(data,byteCheck+1)):
        #print "confirmed Broodminder"
        check = True
    return check



def extractData(data):

    offset = 8 #There are 8 bits less than described in Broodminder documentation

    byteNumAdvdeviceModelIFllc_1 = 10 - offset
    byteNumAdvDeviceVersionMinor_1 = 11 - offset
    byteNumAdvDeviceVersionMajor_1 = 12 - offset
    byteNumAdvBattery_1V2 = 14 - offset
    byteNumAdvElapsed_2 = 15 - offset
    byteNumAdvTemperature_2 = 17 - offset
    byteNumAdvHumidity_1 = 24 - offset
    byteNumAdvElapsed_2V2 = 15 - offset
    byteNumAdvTemperature_2V2 = 17 - offset
    byteNumAdvWeightL_2V2 = 20 - offset
    byteNumAdvWeightR_2V2 = 22 - offset
    byteNumAdvHumidity_1V2 = 24 - offset
    byteNumAdvUUID_3V2 = 25 - offset

    # Version 2 advertising
    #batteryPercent = e.data[byteNumAdvBattery_1V2]
    batteryPercent = int(byte(data , byteNumAdvBattery_1V2) , 16)
    #Elapsed = e.data[byteNumAdvElapsed_2V2] + (e.data[byteNumAdvElapsed_2V2 + 1] << 8)
    #temperatureDegreesC = e.data[byteNumAdvTemperature_2V2] + (e.data[byteNumAdvTemperature_2V2 + 1] << 8)
    temperatureDegreesC = int(byte(data,byteNumAdvTemperature_2V2+1) + byte(data,byteNumAdvTemperature_2V2),16)
    temperatureDegreesC = (float(temperatureDegreesC) / pow(2, 16) * 165 - 40) #* 9 / 5 + 32
    #humidityPercent = e.data[byteNumAdvHumidity_1V2]
    #weightL = e.data[byteNumAdvWeightL_2V2+1] * 256 + e.data[byteNumAdvWeightL_2V2 + 0] - 32767
    weightL = int(byte(data,byteNumAdvWeightL_2V2+1) + byte(data,byteNumAdvWeightL_2V2 + 0),16) - 32767
    weightScaledL = float(weightL) / 100
    #weightR = e.data[byteNumAdvWeightR_2V2 + 1] * 256 + e.data[byteNumAdvWeightR_2V2 + 0] - 32767
    weightR = int(byte(data,byteNumAdvWeightR_2V2+1) + byte(data,byteNumAdvWeightR_2V2 + 0),16) - 32767
    weightScaledR = float(weightR) / 100
    weightScaledTotal = weightScaledL + weightScaledR

    #print "weight = %s , temp = %s, bat = %s" % (weightScaledTotal, temperatureDegreesC, batteryPercent)
    print("Weight = {}, Temperature = {}, Battery = {}".format(weightScaledTotal, temperatureDegreesC, batteryPercent))
    
    
class ScanDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)

    def handleDiscovery(self, dev, isNewDev, isNewData):
        if isNewDev:
            #print "Discovered device", dev.addr
            print("Discovered device {}".format(dev.addr))
        elif isNewData:
            #print "Received new data from", dev.addr
            print("Received new data from {}".format(dev.addr))

scanner = Scanner().withDelegate(ScanDelegate())
devices = scanner.scan(15.0)


for dev in devices:
    if(checkBM(dev.getValueText(255))):
        #print "BroodMinder Found!"
        #print "Device %s (%s), RSSI=%d dB" % (dev.addr, dev.addrType, dev.rssi)
        print("Device {} ({}), RSSI={} dB".format(dev.addr,dev.addrType,dev.rssi))
        for (adtype, desc, value) in dev.getScanData():
            #print "  %s = %s" % (desc, value)
            print ("{} = {}".format(desc,value))
        extractData(dev.getValueText(255))
