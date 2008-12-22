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

/* JDPlay.h
 *
 *	JDPlay, a class to the DirectPlay RippleLaunch technology for easy use.
 *
 *	Needs to be linked to: "dplayx.lib dxguid.lib"
 */

/****************************************************************************************************************/

//includes
#include <iostream>
#include <fstream>
#include <windows.h>
#include <windowsx.h>	//GlobalAllocPtr
#include <dplay.h>
#include <dplobby.h>
#include <comdef.h>
#include <atlbase.h>	//STRING TO GUID
#include <atlstr.h>
#include <time.h>

using namespace std;

BOOL FAR PASCAL EnumSessionsCallback(LPCDPSESSIONDESC2 lpThisSD, LPDWORD lpdwTimeOut, DWORD dwFlags, LPVOID lpContext);

class JDPlay{
	private:
		static JDPlay*			instance;		// needed for callback function to access a method

		bool					debug;
		int						curRetry;
		int						maxSearchRetries;
		bool					foundLobby;
		bool					isInitialized;
		bool					lpDPIsOpen;

		LPDIRECTPLAY3A			lpDP;			// directplay interface pointer
		LPDIRECTPLAYLOBBY3A		lpDPLobby;		// lobby interface pointer
		
		DPNAME					dpName;			// player description
		DPSESSIONDESC2			dpSessionDesc;	// session description
		DPLCONNECTION			dpConn;			// connection description

		DPID					dPid;			// player ID (currently unused)
		DWORD					appID;			// game process ID
		DWORD					sessionFlags;	// either Host or Join Session
		DWORD					playerFlags;	// either Host or not

	public:
		JDPlay(char* playerName, int maxSearchRetries, bool debug);
		~JDPlay();

		void updatePlayerName(char* playerName);
		bool initialize(char* gameGUID, char* hostIP, bool isHost);
		bool launch(bool searchForSession);
		
		void updateFoundSessionDescription(LPCDPSESSIONDESC2 lpFoundSD); //has to be public for the callback function
		static JDPlay* getInstance(); //makes the object available to the callback function
	private:
		void deInitialize();
		char* getDPERR(HRESULT hr);
};
