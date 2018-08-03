#include "StdAfx.h"
#include "airctl.h"


/**
* maybe increase it for yor own pupose
*/
#define SIZEOF_DEVICE_NAME 256
/**
*Contructor... set them NULL
*/
airctl::airctl(void)
{
	m_handle =NULL;
	m_devices = NULL;
	m_pBSSIDList = NULL;

}

/**
*Destructor.. free the memory
*/
airctl::~airctl(void)
{
	if(m_handle != NULL)
		CloseHandle( m_handle);

	clearDeviceList();
}	

/**
* maybe increase it for yor own pupose
*/
#define NUMBEROF_BSSIDS 1000  

/**
* Scan for wlans.
* the class will free it every scan
* @return returns the list...
*/
std::vector<PWLAN_BSS_LIST> airctl::scan(void)
{

	std::vector<PWLAN_BSS_LIST> vec;
	HANDLE hClient = NULL;
	DWORD dwMaxClient = 2;      //      
	DWORD dwCurVersion = 0;
	DWORD dwResult = 0;
	DWORD dwRetVal = 0;
	int iRet = 0;

	WCHAR GuidString[39] = { 0 };

	unsigned int i, j, k;

	/* variables used for WlanEnumInterfaces  */

	PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
	PWLAN_INTERFACE_INFO pIfInfo = NULL;

	PWLAN_AVAILABLE_NETWORK_LIST pBssList = NULL;
	PWLAN_AVAILABLE_NETWORK pBssEntry = NULL;

	int iRSSI = 0;

	dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
	if (dwResult == ERROR_SUCCESS)
	{
		dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
		if (dwResult != ERROR_SUCCESS)
		{
			printf("WlanEnumInterfaces failed with error: %u\n", dwResult);
			// You can use FormatMessage here to find out why the function failed  
		}
		else
		{
			for (i = 0; i < (int)pIfList->dwNumberOfItems; i++) {
				pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[i];

				dwResult = WlanGetAvailableNetworkList(hClient,
					&pIfInfo->InterfaceGuid, 0, NULL, &pBssList);

				if (dwResult != ERROR_SUCCESS) {
					printf("WlanGetAvailableNetworkList failed with error: %u\n", dwResult);
					dwRetVal = 1;
					// You can use FormatMessage to find out why the function failed  
				}
				else
				{
					for (j = 0; j < pBssList->dwNumberOfItems; j++)
					{
						pBssEntry = (WLAN_AVAILABLE_NETWORK *)& pBssList->Network[j];

						PWLAN_BSS_LIST ppWlanBssList;

						DWORD dwResult2 = WlanGetNetworkBssList(hClient, &pIfInfo->InterfaceGuid,
							&pBssEntry->dot11Ssid,
							pBssEntry->dot11BssType,
							pBssEntry->bSecurityEnabled,
							NULL,
							&ppWlanBssList);
						
						if (dwResult2 == ERROR_SUCCESS)
						{
							vec.push_back(ppWlanBssList);
							/*
							for (int z = 0; z < ppWlanBssList->dwNumberOfItems; z++)
							{
								WLAN_BSS_ENTRY bssEntry = ppWlanBssList->wlanBssEntries[z];

								//AString bssid = 
								printf("%s\n", bssEntry.dot11Ssid.ucSSID);
								printf("%d\n", bssEntry.usBeaconPeriod);
								printf("%d\n", bssEntry.lRssi);
								printf("%d\n", bssEntry.usCapabilityInformation);
								printf("%d\n", (bssEntry.ulChCenterFrequency - 2407000) / 5000);
								printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
									bssEntry.dot11Bssid[0],
									bssEntry.dot11Bssid[1],
									bssEntry.dot11Bssid[2],
									bssEntry.dot11Bssid[3],
									bssEntry.dot11Bssid[4],
									bssEntry.dot11Bssid[5]);

								//vecBSSIDs.push_back(bssid);
							}
							*/
						}
					}
				}
			}
			return vec;
		}
	}
	else
	{
		printf("WlanOpenHandle failed with error: %u\n", dwResult);
		// You can use FormatMessage here to find out why the function failed  
	}

	if (pBssList != NULL) {
		WlanFreeMemory(pBssList);
		pBssList = NULL;
	}

	if (pIfList != NULL) {
		WlanFreeMemory(pIfList);
		pIfList = NULL;
	}

	return vec;
}

BOOL airctl::get_device_info(   int Index,
                        char *key_name,
                        char *device_info,
                        char *device_description)
{
        HKEY hkey ;
        DWORD size ;
        DWORD type ;
        BOOL retval ;

        retval = FALSE ;

      memset( device_info, 0, SIZEOF_DEVICE_NAME) ;

		if( RegOpenKeyExA(       HKEY_LOCAL_MACHINE,
                                key_name,
                                0,
                                KEY_READ,
                                &hkey) == ERROR_SUCCESS)
        {
                type = REG_SZ ;
                size = SIZEOF_DEVICE_NAME ;

                if( RegQueryValueExA(    hkey,
                                        "ServiceName",
                                        NULL,
                                        &type,
                                        ( BYTE *) device_info,
                                        &size) == ERROR_SUCCESS)
                {
                        type = REG_SZ ;
                        size = SIZEOF_DEVICE_NAME ;

                        if( RegQueryValueExA(    hkey,
                                                "Description",
                                                NULL,
                                                &type,
                                                ( BYTE *) device_description,
                                                &size) == ERROR_SUCCESS)
                        {
                                retval = TRUE ;
                        }
                }

                RegCloseKey( hkey) ;
        }

        return retval ;
}
/**
* Fills his own list with devices
* @return returns true if there are some devices
*/
BOOL airctl::list_devices( void)
{
		
        char key_name[ SIZEOF_DEVICE_NAME] ;
        char full_name[ SIZEOF_DEVICE_NAME] ;
        char device_info[ SIZEOF_DEVICE_NAME] ;
        char device_description[ SIZEOF_DEVICE_NAME] ;

		this->clearDeviceList ();

        FILETIME file_time ;

        HKEY hkey ;
        int index ;
        DWORD size ;

        index = 0 ;

		if( RegOpenKeyExA(       HKEY_LOCAL_MACHINE,
                                "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards",
                                0,
                                KEY_READ,
                                &hkey) == ERROR_SUCCESS)
        {
                size = SIZEOF_DEVICE_NAME ;

                while(  RegEnumKeyExA(   hkey,
                                        index,
                                        key_name,
                                        &size,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &file_time) == ERROR_SUCCESS)
                {
                        sprintf(        full_name,
                                        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards\\%s",
                                        key_name) ;

                        get_device_info(        index,
                                                full_name,
                                                device_info,
                                                device_description) ;

						this->AddDevice(device_description,device_info);
                        printf( "Index= %d\nName= %s\nDesc=%s\nKey=%s\n\n",
                                index + 1,
                                device_info,
                                device_description,
                                full_name) ;
                        index++ ;

                        size = SIZEOF_DEVICE_NAME ;
                }

                RegCloseKey( hkey) ;
        }

        if( index == 0)
        {
               return false;// printf( "No devices found\n\n") ;
        }

        return TRUE ;
}


/**
*Opens the Device and saves the handle
*/
bool airctl::open( char *device_name)
{
	//printf("12131313131");
        char device_file[ SIZEOF_DEVICE_NAME] ;

        sprintf( device_file, "\\\\.\\%s", device_name) ;

        m_handle = CreateFileA(   device_file,
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL) ;

		if( m_handle == INVALID_HANDLE_VALUE)
        {
			//printf("Error: Device not available\n\n");
			return false;// printf( "Error: Device not available\n\n") ;
        }
        else
        {
               // ... open
			//printf("Open Device:%s\n\n", device_name);
			return true;//printf( "Open Device:%s\n\n",device_name) ;

        }

    //    CloseHandle( hdevice) ;
}


// adds one device to the list
void airctl::AddDevice(char * desc, char * name)
{	
	deviceInfo *tmp= this->m_devices;
	deviceInfo *xtmp;


	xtmp = new deviceInfo;
		xtmp->description = new char[strlen(desc)+1];
		xtmp->name = new char[strlen(name)+1];
		strcpy(xtmp->description,desc);
		strcpy(xtmp->name,name);
		xtmp->next = NULL;


	if(tmp != NULL)
	{
		while (tmp->next != NULL)
		{
			tmp = tmp->next;
		}
		tmp->next= xtmp;
	}
	else
	{
		m_devices = xtmp;
	}

	

}

// clears the intern list
void airctl::clearDeviceList(void)
{
	if(m_devices == NULL)
	{
		return;
	}

	deviceInfo *tmp;
	while (m_devices != NULL)
	{
		tmp =m_devices->next;
		delete[] m_devices->name;
		delete[] m_devices->description ;
		delete m_devices;
		m_devices =tmp;
	}

}

// Frees the list of wlans
void airctl::freeScanList(void)
{
	if(m_pBSSIDList !=NULL){
		::VirtualFree(m_pBSSIDList,sizeof( NDIS_802_11_BSSID_LIST) * NUMBEROF_BSSIDS,0);
		m_pBSSIDList =NULL;
	}
}
