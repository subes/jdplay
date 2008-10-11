/*	Copyright 2007 	Edwin Stang (edwinstang@gmail.com)

    This file is part of JDPlay.

    JDPlay is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    JDPlay is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with JDPlay.  If not, see <http://www.gnu.org/licenses/>.
*/

/****************************************************************************************************************/

/* JDPlay.cpp
 *
 *	JDPlay, a class to the DirectPlay RippleLaunch technology for easy use.
 *
 *	Needs to be linked to: "dplayx.lib dxguid.lib"
 */

/****************************************************************************************************************/

#include "JDPlay.h"

JDPlay* JDPlay::instance;

BOOL FAR PASCAL EnumSessionsCallback(LPCDPSESSIONDESC2 lpThisSD, LPDWORD lpdwTimeOut, DWORD dwFlags, LPVOID lpContext){
	if(lpThisSD){
		JDPlay::getInstance()->updateFoundSessionDescription(lpThisSD);
	}
	return 0;
}

JDPlay::JDPlay(char* playerName, int maxSearchRetries, bool debug){

	this->instance = this;
	this->maxSearchRetries = maxSearchRetries;
	lpDPIsOpen = false;
	isInitialized = false;
	this->debug = debug;

	// clear out memory for info objects
	ZeroMemory(&dpName,sizeof(DPNAME));
	ZeroMemory(&dpSessionDesc, sizeof(DPSESSIONDESC2));
	ZeroMemory(&dpConn, sizeof(DPLCONNECTION));

	// populate player info
	dpName.dwSize = sizeof(DPNAME);
	dpName.dwFlags = 0; // not used, must be zero
	dpName.lpszLongNameA = (char*)malloc(256);
	dpName.lpszShortNameA = (char*)malloc(256);
	updatePlayerName(playerName); //don't set debug to true before this

	if(debug){
		clock_t tClock = clock();
		cout << "[DEBUG@obj@sec:" << (tClock/CLOCKS_PER_SEC) << "] ++ constructed ++" << endl;
		fflush(stdout);
	}
}

JDPlay::~JDPlay(){

	deInitialize();

	if(debug){
		clock_t tClock = clock();
		cout << "[DEBUG@obj@sec:" << (tClock/CLOCKS_PER_SEC) << "] -- destructed --" << endl;
		fflush(stdout);
	}
}

bool JDPlay::initialize(char* gameGUID, char* hostIP, bool isHost){

	if(isInitialized){
		deInitialize();
	}

	time_t tStart = NULL;

	if(debug){
		tStart = time(NULL);
		clock_t tClock = clock();
		cout << "[DEBUG@obj@sec:" << (tClock/CLOCKS_PER_SEC) << "] ++ started initialization ++" << endl;
		fflush(stdout);
	}

	HRESULT hr;
	
	// create GUID Object **************************************************************************
	GUID gameID;
	
	USES_CONVERSION;
	LPOLESTR lpoleguid = A2OLE(gameGUID);
	hr = CLSIDFromString(lpoleguid, &gameID);

	if(hr != S_OK){
		if(debug){
			cout << "[ERROR] invalid GUID" << endl;
			fflush(stdout);
		}
		return false;
	}

	if(debug){
		time_t tNow = time(NULL);
		cout << "[DEBUG@func@sec:" << (tNow-tStart) << "] initialized GUID" << endl;
		fflush(stdout);
	}
	
	// create TCP connection ***********************************************************************
	LPDIRECTPLAYLOBBYA old_lpDPLobbyA = NULL;    // old lobby pointer
	DPCOMPOUNDADDRESSELEMENT  address[2];        // to create compound addr
	DWORD     addressSize = 0;					// size of compound address
	LPVOID    lpConnection = NULL;				// pointer to make connection

	// registering COM
	hr = CoInitialize(NULL);
	
	if(hr != S_OK){
		if(debug){
			cout << "[ERROR] failed to initialize COM" << endl;
			fflush(stdout);
		}
		return false;
	}

	if(debug){
		time_t tNow = time(NULL);
		cout << "[DEBUG@func@sec:" << (tNow-tStart) << "] initialized COM" << endl;
		fflush(stdout);
	}

	// creating directplay object
	hr = CoCreateInstance(CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlay3A,(LPVOID*)&lpDP );
	
	if(hr != S_OK){
		if(debug){
			cout << "[ERROR] failed to initialize DirectPlay" << endl;
			fflush(stdout);
		}
		return false;
	}

	CoUninitialize();  // unregister the COM

	if(debug){
		time_t tNow = time(NULL);
		cout << "[DEBUG@func@sec:" << (tNow-tStart) << "] initialized DirectPlay and deinitialized COM" << endl;
		fflush(stdout);
	}

	// creating lobby object
	hr = DirectPlayLobbyCreate(NULL, &old_lpDPLobbyA, NULL, NULL, 0);

	if(hr != S_OK){
		if(debug){
			cout << "[" << getDPERR(hr) <<"] failed to create lobby object" << endl;
			fflush(stdout);
		}
		return false;
	}

	// get new interface of lobby
	hr = old_lpDPLobbyA->QueryInterface(IID_IDirectPlayLobby3A, (LPVOID *)&lpDPLobby);
	
	if(hr != S_OK){
		if(debug){
			cout << "[" << getDPERR(hr) << "] failed to get new lobby interface" << endl;
			fflush(stdout);
		}
		return false;
	}

	// release old interface since we have new one
	hr = old_lpDPLobbyA->Release();

	if(hr != S_OK){
		if(debug){
			cout << "[" << getDPERR(hr) << "] failed to release old lobby interface" << endl;
			fflush(stdout);
		}
		return false;
	}

	if(debug){
		time_t tNow = time(NULL);
		cout << "[DEBUG@func@sec:" << (tNow-tStart) << "] initialized lobby" << endl;
		fflush(stdout);
	}

	// fill in data for address
	address[0].guidDataType = DPAID_ServiceProvider;
	address[0].dwDataSize   = sizeof(GUID);
	address[0].lpData       = (LPVOID)&DPSPGUID_TCPIP;  // TCP ID

	address[1].guidDataType = DPAID_INet;
	address[1].dwDataSize   = strlen(hostIP)+1;
	address[1].lpData       = hostIP;

	// get size to create address
	// this method will return DPERR_BUFFERTOOSMALL, that is not an error
	hr = lpDPLobby->CreateCompoundAddress(address, 2, NULL, &addressSize);

	if(hr != S_OK && hr != DPERR_BUFFERTOOSMALL){
		if(debug){
			cout << "[" << getDPERR(hr) << "] failed to get size for CompoundAddress" << endl;
			fflush(stdout);
		}
		return false;
	}

	lpConnection = GlobalAllocPtr(GHND, addressSize);  // allocating mem

	// now creating the address
	hr = lpDPLobby->CreateCompoundAddress(address, 2, lpConnection, &addressSize);

	if(hr != S_OK){
		if(debug){
			cout << "[" << getDPERR(hr) << "] failed to create CompoundAddress" << endl;
			fflush(stdout);
		}
		return false;
	}

	// initialize the tcp connection
	hr = lpDP->InitializeConnection(lpConnection, 0);

	if(hr != S_OK){
		if(debug){
			cout << "[" << getDPERR(hr) << "] failed to initialize TCP connection" << endl;
			fflush(stdout);
		}
		return false;
	}

	if(debug){
		time_t tNow = time(NULL);
		cout << "[DEBUG@func@sec:" << (tNow-tStart) << "] initialized TCP connection" << endl;
		fflush(stdout);
	}

	// populate session description ****************************************************************** 
	dpSessionDesc.dwSize = sizeof(DPSESSIONDESC2);
	dpSessionDesc.dwFlags = 0;									// optional: DPSESSION_MIGRATEHOST
	dpSessionDesc.guidApplication = gameID;						// Game GUID
	dpSessionDesc.guidInstance = gameID;							// ID for the session instance
	dpSessionDesc.lpszSessionName = NULL;			// ANSI name of the session
	dpSessionDesc.lpszSessionNameA = NULL;			// ANSI name of the session
	dpSessionDesc.dwMaxPlayers = 0;								// Maximum # players allowed in session
	dpSessionDesc.dwCurrentPlayers = 0;							// Current # players in session (read only)
	dpSessionDesc.lpszPasswordA = NULL;							// ANSI password of the session (optional)
	dpSessionDesc.dwReserved1 = 0;								// Reserved for future M$ use.
	dpSessionDesc.dwReserved2 = 0;
	dpSessionDesc.dwUser1 = 0;									// For use by the application
	dpSessionDesc.dwUser2 = 0;
	dpSessionDesc.dwUser3 = 0;
	dpSessionDesc.dwUser4 = 0;

	// populate connection info **********************************************************************
	dpConn.dwSize = sizeof(DPLCONNECTION);
	dpConn.lpSessionDesc = &dpSessionDesc;		// Pointer to session desc to use on connect
	dpConn.lpPlayerName = &dpName;			// Pointer to Player name structure
	dpConn.guidSP = DPSPGUID_TCPIP;			// GUID of the DPlay SP to use
	dpConn.lpAddress = lpConnection;		// Address for service provider
	dpConn.dwAddressSize = addressSize;		// Size of address data
	if(isHost){
		dpConn.dwFlags = DPLCONNECTION_CREATESESSION;
	}else{
		dpConn.dwFlags = DPLCONNECTION_JOINSESSION;
	}

	if(debug){
		time_t tNow = time(NULL);
		cout << "[DEBUG@func@sec:" << (tNow-tStart) << "] configured session info" << endl;
		fflush(stdout);
	}

	// set other vars
	if(isHost){
		sessionFlags = DPOPEN_CREATE;
		playerFlags = DPPLAYER_SERVERPLAYER;
	}else{
		sessionFlags = DPOPEN_JOIN;
		playerFlags = 0;
	}

	if(debug){
		clock_t tClock = clock();
		cout << "[DEBUG@obj@sec:" << (tClock/CLOCKS_PER_SEC) << "] -- finished initialization --" << endl;
		fflush(stdout);
	}

	isInitialized = true;

	return true;
}

void JDPlay::updatePlayerName(char* playerName){
	if(strlen(playerName) > 256){
		if(debug){
			cout << "[WARNING] playername is too long, cutting at 255 chars" << endl;
		}
		playerName[256] = '\0';
	}

	strcpy(dpName.lpszShortNameA, playerName);
	strcpy(dpName.lpszLongNameA, playerName);
	
	if(debug){
		cout << "[DEBUG] set playername to \"" << playerName << "\"" << endl;
	}
}

bool JDPlay::launch(bool searchForSession){

	if(!isInitialized){
		if(debug){
			cout << "[WARNING] JDPlay has to be initialized before launching!" << endl;
			fflush(stdout);
		}
		return false;
	}

	time_t tStart = NULL;

	if(debug){
		tStart = time(NULL);
		clock_t tClock = clock();
		cout << "[DEBUG@obj@sec:" << (tClock/CLOCKS_PER_SEC) << "] ++ started launch ++" << endl;
		fflush(stdout);
	}

	HRESULT hr;

	if(searchForSession && sessionFlags == DPOPEN_JOIN ){
		// join/host session ***************************************************************************
		if(lpDP && sessionFlags == DPOPEN_JOIN){
			if(debug){
				time_t tNow = time(NULL);
				cout << "[DEBUG@func@sec:" << (tNow-tStart) << "] searching for a session .";
				fflush(stdout);
			}
			
			for(curRetry = 1; curRetry < maxSearchRetries; curRetry++){
		
				if(debug){
					cout << ".";
					fflush(stdout);
				}

				hr = lpDP->EnumSessions(&dpSessionDesc, 0, EnumSessionsCallback, NULL, 0);
				if(hr != S_OK){
					if(debug){
						cout << endl << "[" << getDPERR(hr) << "] failed to enumerate sessions" << endl;
						fflush(stdout);
					}
					return false;
				}

				if(foundLobby){
					break;
				}
			}

			if(debug){
				if(!foundLobby){
					cout << " FAILURE after " << curRetry << ". try!" << endl;
					fflush(stdout);
				}else{
					cout << " SUCCESS at " << curRetry << ". try!" << endl;
					fflush(stdout);
				}
			}

			if(debug){
				time_t tNow = time(NULL);
				if(foundLobby){
					cout << "[DEBUG@func@sec:" << (tNow-tStart) << "] session found" << endl;
					fflush(stdout);
				}else{
					cout << "[DEBUG@func@sec:" << (tNow-tStart) << "] search failed" << endl;
					fflush(stdout);
				}
			}

			if(!foundLobby){
				return false;
			}

		}

		if(lpDP){

			hr = lpDP->Open(&dpSessionDesc, sessionFlags | DPOPEN_RETURNSTATUS);
			if(hr != S_OK){
				if(debug){
					cout << "[" << getDPERR(hr) << "] failed to open DirectPlay session" << endl;
					fflush(stdout);
				}
				return false;
			}

			lpDPIsOpen = true;

			// create player *******************************************************************************
			hr = lpDP->CreatePlayer(&dPid, &dpName, NULL, NULL, 0, playerFlags);

			if(hr != S_OK){
				if(debug){
					cout << "[" << getDPERR(hr) << "] failed to create local player" << endl;
					fflush(stdout);
				}
				return false;
			}

			if(debug){
				time_t tNow = time(NULL);
				cout << "[DEBUG@func@sec:" << (tNow-tStart) << "] opened session and initialized player" << endl;
				fflush(stdout);
			}
		}
	}else{
		if(debug){
			time_t tNow = time(NULL);
			cout << "[DEBUG@func@sec:" << (tNow-tStart) << "] skipping session search" << endl;
			fflush(stdout);
		}
	}

	// release temporary directplay interface ***************************************************************
	if(lpDP){

		if(lpDPIsOpen){
			hr = lpDP->Close();		//close dplay interface
			if(hr != S_OK){
				if(debug){
					cout << "[" << getDPERR(hr) << "] failed to close DirectPlay interface" << endl;
					fflush(stdout);
				}
				return false;
			}
		}
		
		hr = lpDP->Release();	//release dplay interface
		if(hr != S_OK){
			if(debug){
				cout << "[" << getDPERR(hr) << "] failed to release DirectPlay interface" << endl;
				fflush(stdout);
			}
			return false;
		}
		
		lpDP = NULL;  // set to NULL, safe practice here

		if(debug){
			time_t tNow = time(NULL);
			cout << "[DEBUG@func@sec:" << (tNow-tStart) << "] deinitialized DirectPlay" << endl;
			fflush(stdout);
		}
	}

	// launch game *********************************************************************************
	hr = lpDPLobby->RunApplication( 0, &appID, &dpConn, 0);
	
	if(hr != S_OK){
		if(debug){
			cout << "[" << getDPERR(hr) << "] failed to launch the game, maybe it's not installed properly" << endl;
			fflush(stdout);
		}
		return false;
	}

	if(debug){
		time_t tNow = time(NULL);
		cout << "[DEBUG@func@sec:" << (tNow-tStart) << "] started game, ProcessID = " << appID << endl;
		fflush(stdout);
	}

	// wait until game exits ***********************************************************************
	HANDLE appHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, appID);
	if(appHandle == NULL){
		if(debug){
			cout << "[ERROR] failed to open game process" << endl;
			fflush(stdout);
		}
		return false;
	}

	DWORD exitCode = NULL;
	while(GetExitCodeProcess(appHandle, &exitCode)){   // process is running
		if(exitCode != STILL_ACTIVE){
			break;
		}
		Sleep(1000);		
	}

	if(debug){
		clock_t tClock = clock();
		cout << "[DEBUG@obj@sec:" << (tClock/CLOCKS_PER_SEC) << "] -- game closed, finished launch --" << endl;
		fflush(stdout);
	}

	return true;
}

void JDPlay::updateFoundSessionDescription(LPCDPSESSIONDESC2 lpFoundSD){
	//so that dplay also joins sessions created ingame
	dpSessionDesc.dwSize = lpFoundSD->dwSize;
	dpSessionDesc.dwFlags = lpFoundSD->dwFlags;
	dpSessionDesc.guidInstance = lpFoundSD->guidInstance;
	dpSessionDesc.guidApplication = lpFoundSD->guidApplication;
	dpSessionDesc.dwMaxPlayers = lpFoundSD->dwMaxPlayers;
	dpSessionDesc.dwCurrentPlayers = lpFoundSD->dwCurrentPlayers;
	dpSessionDesc.dwReserved1 = lpFoundSD->dwReserved1;
	dpSessionDesc.dwReserved2 = lpFoundSD->dwReserved2;
	dpSessionDesc.dwUser1 = lpFoundSD->dwUser1;
	dpSessionDesc.dwUser2 = lpFoundSD->dwUser2;
	dpSessionDesc.dwUser3 = lpFoundSD->dwUser3;
	dpSessionDesc.dwUser4 = lpFoundSD->dwUser4;
	dpSessionDesc.lpszSessionName = lpFoundSD->lpszSessionName;
	dpSessionDesc.lpszSessionNameA = lpFoundSD->lpszSessionNameA;
	dpSessionDesc.lpszPassword = lpFoundSD->lpszPassword;
	dpSessionDesc.lpszPasswordA = lpFoundSD->lpszPasswordA;

	foundLobby = true;
}

JDPlay* JDPlay::getInstance(){
	return instance;
}

void JDPlay::deInitialize(){
	time_t tStart = NULL;

	if(debug){
		tStart = time(NULL);
		clock_t tClock = clock();
		cout << "[DEBUG@obj@sec:" << (tClock/CLOCKS_PER_SEC) << "] ++ started deinitialization ++" << endl;
		fflush(stdout);
	}

	HRESULT hr;

	if(lpDP){

		if(lpDPIsOpen){
			hr = lpDP->Close();		//close dplay interface
			if(hr != S_OK){
				if(debug){
					cout << "[" << getDPERR(hr) << "] failed to close DirectPlay interface" << endl;
					fflush(stdout);
				}
			}
		}
		
		hr = lpDP->Release();	//release dplay interface
		if(hr != S_OK){
			if(debug){
				cout << "[" << getDPERR(hr) << "] failed to release DirectPlay interface" << endl;
				fflush(stdout);
			}
		}
		
		lpDP = NULL;  // set to NULL, safe practice here

		if(debug){
			time_t tNow = time(NULL);
			cout << "[DEBUG@func@sec:" << (tNow-tStart) << "] deinitialized DirectPlay" << endl;
			fflush(stdout);
		}
		
	}

	if(lpDPLobby){
		
		hr = lpDPLobby->Release(); //release lobby
		if(hr != S_OK){
			if(debug){
				cout << "[" << getDPERR(hr) << "] failed to release lobby interface" << endl;
				fflush(stdout);
			}
		}
		lpDPLobby = NULL;

		if(debug){
			time_t tNow = time(NULL);
			cout << "[DEBUG@func@sec:" << (tNow-tStart) << "] deinitialized lobby" << endl;
			fflush(stdout);
		}
	}

	lpDPIsOpen = false;

	if(debug){
		clock_t tClock = clock();
		cout << "[DEBUG@obj@sec:" << (tClock/CLOCKS_PER_SEC) << "] -- finished deinitialization --" << endl;
		fflush(stdout);
	}

	isInitialized = false;
}

char* JDPlay::getDPERR(HRESULT hr){
	if(DP_OK) return "DP_OK";
	if(DPERR_ALREADYINITIALIZED) return "DPERR_ALREADYINITIALIZED";
	if(DPERR_ACCESSDENIED) return "DPERR_ACCESSDENIED";
	if(DPERR_ACTIVEPLAYERS) return "DPERR_ACTIVEPLAYERS";
	if(DPERR_BUFFERTOOSMALL) return "DPERR_BUFFERTOOSMALL";
	if(DPERR_CANTADDPLAYER) return "DPERR_CANTADDPLAYER";
	if(DPERR_CANTCREATEGROUP) return "DPERR_CANTCREATEGROUP";
	if(DPERR_CANTCREATEPLAYER) return "DPERR_CANTCREATEPLAYER";
	if(DPERR_CANTCREATESESSION) return "DPERR_CANTCREATESESSION";
	if(DPERR_CAPSNOTAVAILABLEYET) return "DPERR_CAPSNOTAVAILABLEYET";
	if(DPERR_EXCEPTION) return "DPERR_CAPSNOTAVAILABLEYET";
	if(DPERR_GENERIC) return "DPERR_GENERIC";
	if(DPERR_INVALIDFLAGS) return "DPERR_INVALIDFLAGS";
	if(DPERR_INVALIDOBJECT) return "DPERR_INVALIDOBJECT";
	if(DPERR_INVALIDPARAMS) return "DPERR_INVALIDPARAMS";
	if(DPERR_INVALIDPLAYER) return "DPERR_INVALIDPLAYER";
	if(DPERR_INVALIDGROUP) return "DPERR_INVALIDGROUP";
	if(DPERR_NOCAPS) return "DPERR_NOCAPS";
	if(DPERR_NOCONNECTION) return "DPERR_NOCONNECTION";
	if(DPERR_NOMEMORY) return "DPERR_NOMEMORY";
	if(DPERR_OUTOFMEMORY) return "DPERR_OUTOFMEMORY";
	if(DPERR_NOMESSAGES) return "DPERR_NOMESSAGES";
	if(DPERR_NONAMESERVERFOUND) return "DPERR_NONAMESERVERFOUND";
	if(DPERR_NOPLAYERS) return "DPERR_NOPLAYERS";
	if(DPERR_NOSESSIONS) return "DPERR_NOSESSIONS";
	if(DPERR_PENDING) return "DPERR_PENDING";
	if(DPERR_SENDTOOBIG) return "DPERR_SENDTOOBIG";
	if(DPERR_TIMEOUT) return "DPERR_TIMEOUT";
	if(DPERR_UNAVAILABLE) return "DPERR_UNAVAILABLE";
	if(DPERR_UNSUPPORTED) return "DPERR_UNSUPPORTED";
	if(DPERR_BUSY) return "DPERR_BUSY";
	if(DPERR_USERCANCEL) return "DPERR_USERCANCEL";
	if(DPERR_NOINTERFACE) return "DPERR_NOINTERFACE";
	if(DPERR_CANNOTCREATESERVER) return "DPERR_CANNOTCREATESERVER";
	if(DPERR_PLAYERLOST) return "DPERR_PLAYERLOST";
	if(DPERR_SESSIONLOST) return "DPERR_SESSIONLOST";
	if(DPERR_UNINITIALIZED) return "DPERR_UNINITIALIZED";
	if(DPERR_NONEWPLAYERS) return "DPERR_NONEWPLAYERS";
	if(DPERR_INVALIDPASSWORD) return "DPERR_INVALIDPASSWORD";
	if(DPERR_CONNECTING) return "DPERR_CONNECTING";
	if(DPERR_CONNECTIONLOST) return "DPERR_CONNECTIONLOST";
	if(DPERR_UNKNOWNMESSAGE) return "DPERR_UNKNOWNMESSAGE";
	if(DPERR_CANCELFAILED) return "DPERR_CANCELFAILED";
	if(DPERR_INVALIDPRIORITY) return "DPERR_INVALIDPRIORITY";
	if(DPERR_NOTHANDLED) return "DPERR_NOTHANDLED";
	if(DPERR_CANCELLED) return "DPERR_CANCELLED";
	if(DPERR_ABORTED) return "DPERR_ABORTED";
	if(DPERR_BUFFERTOOLARGE) return "DPERR_BUFFERTOOLARGE";
	if(DPERR_CANTCREATEPROCESS) return "DPERR_CANTCREATEPROCESS";
	if(DPERR_APPNOTSTARTED) return "DPERR_APPNOTSTARTED";
	if(DPERR_INVALIDINTERFACE) return "DPERR_INVALIDINTERFACE";
	if(DPERR_NOSERVICEPROVIDER) return "DPERR_NOSERVICEPROVIDER";
	if(DPERR_UNKNOWNAPPLICATION) return "DPERR_UNKNOWNAPPLICATION";
	if(DPERR_NOTLOBBIED) return "DPERR_NOTLOBBIED";
	if(DPERR_SERVICEPROVIDERLOADED) return "DPERR_SERVICEPROVIDERLOADED";
	if(DPERR_ALREADYREGISTERED) return "DPERR_ALREADYREGISTERED";
	if(DPERR_NOTREGISTERED) return "DPERR_NOTREGISTERED";
	if(DPERR_AUTHENTICATIONFAILED) return "DPERR_AUTHENTICATIONFAILED";
	if(DPERR_CANTLOADSSPI) return "DPERR_CANTLOADSSPI";
	if(DPERR_ENCRYPTIONFAILED) return "DPERR_ENCRYPTIONFAILED";
	if(DPERR_SIGNFAILED) return "DPERR_SIGNFAILED";
	if(DPERR_CANTLOADSECURITYPACKAGE) return "DPERR_CANTLOADSECURITYPACKAGE";
	if(DPERR_ENCRYPTIONNOTSUPPORTED) return "DPERR_ENCRYPTIONNOTSUPPORTED";
	if(DPERR_CANTLOADCAPI) return "DPERR_CANTLOADCAPI";
	if(DPERR_NOTLOGGEDIN) return "DPERR_NOTLOGGEDIN";
	if(DPERR_LOGONDENIED) return "DPERR_LOGONDENIED";

	return "ERROR";
}
