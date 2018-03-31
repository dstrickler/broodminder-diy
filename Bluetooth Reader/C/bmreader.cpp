/*
Code original created by Gerry Rozema in 2017
https://www.rozehaven.ca/farm/
*/

#include <stdlib.h>
#include <errno.h>
#include <curses.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

/*
============
Taken from the broodminder docs linked online
============
Advertising Packet Makeup for BroodMinder

This Document Version 2.0  - Version 1.xx & 2.xx Firmware
Rich Morris 26 May 2016

------------------------------------------------------------
When you read advertising packets from BLE, you can identify BroodMinder products by looking at the following.

The data will look something like this. - From device b5:30:07

GAP Scan Response Event ------------------------------------------------------------------------------------
ble_evt_gap_scan_response: rssi=-77, packet_type=0, sender=[ 07 30 b5 80 07 00 ], address_type=0, bond=255,
  data=[ 02 01 06 02 0a 03 18 ff 8d 02 2b 15 02 00 02 21 00 d0 62 00 ff 7f 05 80 37 07 30 b5 00 00 00 ]


Values are in decimal unless preceeded with 0x

1) Check for "Manufacturer Specific Data"
	Bytes 6,7 = 0x18, 0xff
2) Check for IF, LLC as the manufacturer
	Bytes 8,9 = 0x8d, 0x02
3) Bytes 10-29 are the data from the BroodMinder as outlined below.
	deviceModelIFllc_1 = 2b (43d = scale)
	DeviceVersionMinor_1 = 15 (21d)
	DeviceVersionMajor_1  = 02 (FW 2.21)
	Battery_1V2 = 2%
	Elapsed_2V2 = 21 (33d)
	Temperature_2V2 = 62d0
	WeightL_2V2 = 7FFF
	WeightR_2V2 = 8005
	Humidity_1V2 = 37
	UUID_3V2 = b5:30:07
*/

//  a structure to hold the data from one unit

typedef struct BroodMinder {
    char addr[18];  //  identify units by mac address
    int counter;    //  the counter last time we saw this one
} BROODMINDER;

//  Global variables are bad, but, this is a quick and dirty program to log data
//  so we will just use a few globals to expedite things

//  we are only tracking a few, adjust this variable if you have more
//  than 10 BroodMinder units in range
#define MAX_UNITS 10
//  We will start with no units detected
int NumUnits=0;
//  Now an array of units that we are monitoring
BroodMinder Unit[MAX_UNITS];


int LogReading(char *addr,int battery,int elapsed,float temp,int humidity,float wl,float wr)
{
     BROODMINDER *unit;

    //  lets see if we have already got this one in our list
    unit=NULL;
    for(int x=0; x<NumUnits; x++) {
        if(strcmp(Unit[x].addr,addr)==0) {
            unit=&Unit[x];
        }
    }
    if(unit==NULL) {
        //  this one is not in our list yet
        //  add it, but dont overflow our array size
        if(NumUnits < MAX_UNITS) {
            strcpy(Unit[NumUnits].addr,addr);
            Unit[NumUnits].counter=0;
            unit=&Unit[NumUnits];
            NumUnits++;
            //printf("Found new unit %s %d\n",addr,NumUnits);
        }
    }
    if(unit != NULL) {
        //  only log a new reading if the elapased counter has incremented
        if(elapsed > unit->counter) {
            char cmd[250];

            //printf("new reading for %s\n",addr);
            unit->counter=elapsed;

            //  We have a new reading, lets spawn the script to log it
            //  by spawning a script at this point, we have a setup that is
            //  independant of how data is stored, the script can be unique
            //  to the local installation, and choose to tuck data into a file,
            //  or a database, or whatever suits your fancy

            //  first create a command line with all the information in parameters
            sprintf(cmd,"BmLogger %s %d %d %4.1f %d %4.1f %4.1f",addr,battery,elapsed,temp,humidity,wl,wr);
            //  Now spawn a child with the data as parameters
            //printf("%s\n",cmd);
            system(cmd);

        }
    }
    return 0;
}

int ProcessBroodMinderAdvert(char *addr,unsigned char *ad, int len)
{
    int model;
    int fwminor;
    int fwmajor;
    int battery;
    int temperature;
    int humidity;
    int weightL;
    int weightR;
    int elapsed;
    float tempc,wl,wr;

    //for(int x=0; x<len; x++) {
    //    printf("%02x ",ad[x]);
    //}
    //printf("\n");

    model=ad[10];
    fwminor=ad[11];
    fwmajor=ad[12];
    battery=ad[14];
    //  if we do this the long tedious way, one step at a time
    //  then things like compiler optimizations and processor endian issues
    //  wont play in the result, and the steps are very clear for folks not
    //  terribly familiar with c programming
    elapsed=ad[16];
    elapsed=elapsed<<8;
    elapsed=elapsed+ad[15];

    temperature=ad[18];
    temperature=temperature<<8;
    temperature=temperature+ad[17];

    humidity=ad[24];

    weightL=ad[21];
    weightL=weightL<<8;
    weightL=weightL+ad[20];

    weightR=ad[23];
    weightR=weightR<<8;
    weightR=weightR+ad[22];

    //  we have the raw weight data, now lets convert it
    //  to a useable number in human understandable units
    weightL=weightL-32767;
    weightR=weightR-32767;
    //  now scale this down into a floating point result
    wl=weightL;
    wl=wl/100;
    wr=weightR;
    wr=wr/100;

    //  now convert temperature to a human understandable number
    tempc=temperature;
    tempc=tempc/65536;
    tempc=tempc*165;
    tempc=tempc-40;

    /*
    printf("%s - ",addr);
    if(model==0x2b) {
            printf("Scale    ");
    } else if(model==0x2a) {
        printf("T/H      ");
     } else {
         printf("dunno %02x ",model);
     }
    printf("Firmware %1d.%02d ",fwmajor,fwminor);
    printf("Battery %3d ",battery);
    printf("Elapsed %6d ",elapsed);
    printf("temperature %4.1f ",tempc);
    printf("humidity %d ",humidity);
    if(model==0x2b) {
        printf("WL %4.1f WR %4.1f",wl,wr);
    }
    printf("\n");
    */
    if(model != 0x2b ) {
        //  it's not a scale, set weights to zero
        wl=0;
        wr=0;
    }

    LogReading(addr,battery,elapsed,tempc,humidity,wl,wr);

    return 0;
}

struct hci_request ble_hci_request(uint16_t ocf, int clen, void * status, void * cparam)
{
	struct hci_request rq;
	memset(&rq, 0, sizeof(rq));
	rq.ogf = OGF_LE_CTL;
	rq.ocf = ocf;
	rq.cparam = cparam;
	rq.clen = clen;
	rq.rparam = status;
	rq.rlen = 1;
	return rq;
}

int main()
{
	int ret, status;

	//  On a pi where we have just restarted
	//  the bluetooth is in the down state and cant read our gadgets
	//  and sometimes other things have left it in an indeterminate state
	//  so start with a down / up cycle and these issues get fixed
	system("hciconfig hci0 down");
	sleep(1);
	system("hciconfig hci0 up");

	// Get HCI device.

	const int device = hci_open_dev(hci_get_route(NULL));
	if ( device < 0 ) {
		perror("Failed to open HCI device.");
		return 1;
	}

	// Set BLE scan parameters.

	le_set_scan_parameters_cp scan_params_cp;
	memset(&scan_params_cp, 0, sizeof(scan_params_cp));
	scan_params_cp.type 			= 0x00;
	scan_params_cp.interval 		= htobs(0x0010);
	scan_params_cp.window 			= htobs(0x0010);
	scan_params_cp.own_bdaddr_type 	= 0x00; // Public Device Address (default).
	scan_params_cp.filter 			= 0x00; // Accept all.

	struct hci_request scan_params_rq = ble_hci_request(OCF_LE_SET_SCAN_PARAMETERS, LE_SET_SCAN_PARAMETERS_CP_SIZE, &status, &scan_params_cp);

	ret = hci_send_req(device, &scan_params_rq, 1000);
	if ( ret < 0 ) {
		hci_close_dev(device);
		perror("Failed to set scan parameters data.");
		return 2;
	}

	// Set BLE events report mask.
	le_set_event_mask_cp event_mask_cp;
	memset(&event_mask_cp, 0, sizeof(le_set_event_mask_cp));
	int i = 0;
	for ( i = 0 ; i < 8 ; i++ ) event_mask_cp.mask[i] = 0xFF;

	struct hci_request set_mask_rq = ble_hci_request(OCF_LE_SET_EVENT_MASK, LE_SET_EVENT_MASK_CP_SIZE, &status, &event_mask_cp);
	ret = hci_send_req(device, &set_mask_rq, 1000);
	if ( ret < 0 ) {
		hci_close_dev(device);
		perror("Failed to set event mask.");
		return 3;
	}

	// Enable scanning.

	le_set_scan_enable_cp scan_cp;
	memset(&scan_cp, 0, sizeof(scan_cp));
	scan_cp.enable 		= 0x01;	// Enable flag.
	scan_cp.filter_dup 	= 0x00; // Filtering disabled.

	struct hci_request enable_adv_rq = ble_hci_request(OCF_LE_SET_SCAN_ENABLE, LE_SET_SCAN_ENABLE_CP_SIZE, &status, &scan_cp);

	ret = hci_send_req(device, &enable_adv_rq, 1000);
	if ( ret < 0 ) {
		hci_close_dev(device);
		perror("Failed to enable scan.");
		return 4;
	}

	// Get Results.

	struct hci_filter nf;
	hci_filter_clear(&nf);
	hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
	hci_filter_set_event(EVT_LE_META_EVENT, &nf);
	if ( setsockopt(device, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0 ) {
		hci_close_dev(device);
		perror("Could not set socket options\n");
		return 5;
	}

	//printf("Scanning....\n");

	uint8_t buf[HCI_MAX_EVENT_SIZE];
	evt_le_meta_event * meta_event;
	le_advertising_info * info;
	int len;

	while ( 1 ) {
		len = read(device, buf, sizeof(buf));
		if ( len >= HCI_EVENT_HDR_SIZE ) {
			meta_event = (evt_le_meta_event*)(buf+HCI_EVENT_HDR_SIZE+1);
			if ( meta_event->subevent == EVT_LE_ADVERTISING_REPORT ) {
				uint8_t reports_count = meta_event->data[0];
				void * offset = meta_event->data + 1;
				//printf("%d reports\n",reports_count);
				while ( reports_count-- ) {
					info = (le_advertising_info *)offset;

					if((info->data[6]==0x18)&&(info->data[7]==0xff)) { // check for manufacturer specific data
                        if((info->data[8]==0x8d)&&(info->data[9]==0x02)) {  //  did this come from a broodminder device ?
                            //  this is a broodminder gadget telling us something
                            char addr[18];
                            ba2str(&(info->bdaddr), addr);
                            ProcessBroodMinderAdvert(addr,info->data,info->length);
                        }
					}
				}
			}
		}
	}

	// Disable scanning.

	memset(&scan_cp, 0, sizeof(scan_cp));
	scan_cp.enable = 0x00;	// Disable flag.

	struct hci_request disable_adv_rq = ble_hci_request(OCF_LE_SET_SCAN_ENABLE, LE_SET_SCAN_ENABLE_CP_SIZE, &status, &scan_cp);
	ret = hci_send_req(device, &disable_adv_rq, 1000);
	if ( ret < 0 ) {
		hci_close_dev(device);
		perror("Failed to disable scan.");
		return 0;
	}

	hci_close_dev(device);

	return 0;
}
