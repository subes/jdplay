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

/*	This is a remote controlled program for launching DirectPlay games via RippleLaunch.
 *	It is supposed to give the functionality of the JDPLay_jni.dll with an environment like wine or cedega.
 *
 *  You will need the DirectX SDK April 2007 (latest version with all DirectPlay headers).
 *  Also you will need the Windows Platform SDK for ATL.
 */

/****************************************************************************************************************/

#include "DirectPlay.h"
#include <iostream>
#include <sstream>
#include <conio.h>

using namespace std;

#define MAX_DONE_COUNT 10

// *** Variable declarations ***
JDPlay* jdplay;
bool debug;

int doneCounter; //used to realize if father process dies -> detectable if we get sent "DONE" endlessly
int maxSearchRetries;
bool sessionFound = false;

// *** Method declarations ***
void waitForCommand();
void initialize(char* gameGUID, char* hostIP, bool isHost, int maxPlayers);
void launch(bool doSearch, bool startGame);
void printHelp();

// *** Method implementations ***
int main(int argc, char* argv[]){
	//Read args ************************************************
	debug = false;
	doneCounter = 0;
	char* playerName;
	int searchValidationCount;

	//Go through arguments
	bool playerfound = false;
	bool retriesfound = false;
	bool validatefound = false;

	for(int i = 0; i < argc; i++){
		if(!strcmp(argv[i], "--playerName")){
			if(!playerfound){
				playerfound = true;
				if(argc > i+1){
					playerName = argv[i+1];
					i++;
				}else{
					cout << "ERROR: --playerName was used without a <VALUE>" << endl;
					fflush(stdout);
					printHelp();
					exit(1);
				}
			}else{
				cout << "ERROR: --playerName was given more than once" << endl;
				fflush(stdout);
				printHelp();
				exit(1);
			}
		}else
		if(!strcmp(argv[i], "--searchValidationCount")){
			if(!validatefound){
				if(argc > i+1){
					validatefound = true;
					//Convert char* to int
					istringstream istr(argv[i+1]);
					if(!(istr >> searchValidationCount)){
						cout << "ERROR: the <VALUE> of --searchValidationCount is not a number" << endl;
						fflush(stdout);
					}
					i++;
				}else{
					cout << "ERROR: --searchValidationCount was used without a <VALUE>" << endl;
					fflush(stdout);
					printHelp();
					exit(1);
				}
			}
			else{
				cout << "ERROR: --searchValidationCount was given more than once" << endl;
				fflush(stdout);
				printHelp();
				exit(1);
			}
		}else
		if(!strcmp(argv[i], "--maxSearchRetries")){
			if(!retriesfound){
				if(argc > i+1){
					retriesfound = true;
					//Convert char* to int
					istringstream istr(argv[i+1]);
					if(!(istr >> maxSearchRetries)){
						cout << "ERROR: the <VALUE> of --maxSearchRetries is not a number" << endl;
						fflush(stdout);
					}
					i++;
				}else{
					cout << "ERROR: --maxSearchRetries was used without a <VALUE>" << endl;
					fflush(stdout);
					printHelp();
					exit(1);
				}
			}
			else{
				cout << "ERROR: --maxSearchRetries was given more than once" << endl;
				fflush(stdout);
				printHelp();
				exit(1);
			}
		}else
		if(!strcmp(argv[i], "--help")){
			printHelp();
			exit(0);
		}else
		if(!strcmp(argv[i], "--debug")){
			debug = true;
		}
	}

	//Check for missing arguments
	if(!playerfound){
		cout << "ERROR: --playerName was not given" << endl;
		fflush(stdout);
		printHelp();
		exit(1);
	}
	if(!retriesfound){
		cout << "ERROR: --maxSearchRetries was not given" << endl;
		fflush(stdout);
		printHelp();
		exit(1);
	}
	if(!validatefound){
		cout << "ERROR: --searchValidationCount was not given" << endl;
		fflush(stdout);
		printHelp();
		exit(1);
	}

	//Init JDPlay *********************************************************
	jdplay = new JDPlay(playerName, searchValidationCount, debug);

	while(true){
		waitForCommand();
	}
}

void waitForCommand(){
	bool waitForDone = false;

	//Flush stdin
    char prev = 'a';
    int sameCharCount = 0;
	while(_kbhit()){
        char cur = _getch();
        if(prev == cur){
            sameCharCount++;
            if(sameCharCount >= MAX_DONE_COUNT*5){
                exit(1);
            }
        }else{
            prev = cur;
        }
	}
	
	//Now ready for command
	cout << "RDY" << endl;
	fflush(stdout);

	//Read the command
	static char in[MAX_PATH]; //should be enough
	gets_s<MAX_PATH>(in);

	string input(in);

	if(!input.compare("DONE") || input.length() < 1){
		doneCounter++;
	}else{
		if(!input.compare("STILLALIVETEST")){
			cout << "ACK" << endl;
			fflush(stdout);
			doneCounter = 0;
			waitForDone = true;
		}else
		if(!input.substr(0,11).compare("INITIALIZE ") && input.find(" gameGUID:") != string::npos && input.find(" hostIP:") != string::npos && input.find(" isHost:") != string::npos && input.find(" maxPlayers:") != string::npos){
			doneCounter = 0;
			waitForDone = true;

			//Initialize
			
			//INITIALIZE gameGUID:<e.g. {BC3A2ACD-FB46-4c6b-8B5C-CD193C9805CF}> hostIP:<e.g. 192.168.0.3> isHost:<true or false> maxPlayers:<true or false>
			string s_gameGUID = input.substr(input.find(" gameGUID:")+10, input.find(" hostIP:")-input.find(" gameGUID:")-10);
			string s_hostIP = input.substr(input.find(" hostIP:")+8, input.find(" isHost:")-input.find(" hostIP:")-8);
			string s_isHost = input.substr(input.find(" isHost:")+8, input.find(" maxPlayers:")-input.find(" isHost:")-8);
			string s_maxPlayers = input.substr(input.find(" maxPlayers:")+12);

			if(s_gameGUID.length() != 38){
				cout << "NAK" << endl;
				if(debug){
					cout << "gameGUID: got \"" << s_gameGUID << "\", but expected something like \"{BC3A2ACD-FB46-4c6b-8B5C-CD193C9805CF}\"" << endl;
				}
				fflush(stdout);
				return;
			}

			if(s_hostIP.length() > 256){
				cout << "NAK" << endl;
				if(debug){
					cout << "hostIP: value has " << s_gameGUID.length() << " chars length, but 255 chars are maximum" << endl;
				}
				fflush(stdout);
				return;
			}

			bool isHost;
			if(!s_isHost.compare("true")){
				isHost = true;
			}else
			if(!s_isHost.compare("false")){
				isHost = false;
			}else{
				cout << "NAK" << endl;
				if(debug){
					cout << "isHost: got \"" << s_isHost << "\", but expected \"true\" or \"false\"" << endl;
				}
				fflush(stdout);
				return;
			}

			char gameGUID[39];
			strcpy_s<39>(gameGUID, s_gameGUID.c_str());

			char hostIP[256]; //could also be a domain name
			strcpy_s<256>(hostIP, s_hostIP.c_str());

			int maxPlayers;
			istringstream isst;
			isst.str(s_maxPlayers);
			isst >> maxPlayers;
			
			cout << "ACK" << endl;
			fflush(stdout);
			initialize(gameGUID, hostIP, isHost, maxPlayers);
		}else
		if(!input.substr(0,7).compare("LAUNCH ") && input.find(" doSearch:") != string::npos && input.find(" startGame:") != string::npos){
			doneCounter = 0;
			waitForDone = true;

			//Launch game
			string s_doSearch = input.substr(input.find(" doSearch:")+10,input.find(" startGame:")-input.find(" doSearch:")-10);
			string s_startGame = input.substr(input.find(" startGame:")+11);

			bool doSearch;
			if(!s_doSearch.compare("true")){
				doSearch = true;
			}else
			if(!s_doSearch.compare("false")){
				doSearch = false;
			}else{
				cout << "NAK" << endl;
				if(debug){
					cout << "doSearch: found \"" << s_doSearch << "\", but expected \"true\" or \"false\"" << endl;
				}
				fflush(stdout);
				return;
			}

			bool startGame;
			if(!s_startGame.compare("true")){
				startGame = true;
			}else
			if(!s_startGame.compare("false")){
				startGame = false;
			}else{
				cout << "NAK" << endl;
				if(debug){
					cout << "startGame: found \"" << s_startGame << "\", but expected \"true\" or \"false\"" << endl;
				}
				fflush(stdout);
				return;
			}

			//Launch game
			cout << "ACK" << endl;
			fflush(stdout);
			launch(doSearch, startGame);
		}else
		if(!input.substr(0,7).compare("UPDATE ")){
			doneCounter = 0;
			waitForDone = true;
			
			//Set new playername
			string s_playerName = input.substr(input.find(" playerName:")+12);
			char playerName[256];

			if(s_playerName.length() > 255){
				cout << "NAK" << endl;
				if(debug){
					cout << "playerName: value has " << s_playerName.length() << " chars length, but 255 chars are maximum" << endl;
				}
				fflush(stdout);
				return;
			}
			
			strcpy_s<256>(playerName, s_playerName.c_str());
			
			cout << "ACK" << endl;
			fflush(stdout);
			jdplay->updatePlayerName(playerName);
		}else{
			cout << "NAK" << endl;
			fflush(stdout);
			doneCounter++;
		}
	}

	if(doneCounter >= MAX_DONE_COUNT){
		exit(1);
	}

	if(waitForDone){
		//Read DONE
		gets_s<MAX_PATH>(in);
		if(strcmp(in, "DONE")){
			cout << "You have to end each command conversation with DONE!" << endl;
			fflush(stdout);
		}
	}
}

void initialize(char* gameGUID, char* hostIP, bool isHost, int maxPlayers){
	bool ret = jdplay->initialize(gameGUID, hostIP, isHost, maxPlayers);
	sessionFound = false;
	if(!ret){
		cout << "ERR" << endl;
		fflush(stdout);
	}else{
		cout << "FIN" << endl;
		fflush(stdout);
	}
}

void launch(bool doSearch, bool startGame){

	if(doSearch && !jdplay->isHost() && !sessionFound){
		for(int i = 1; i <= maxSearchRetries && !sessionFound; i++){
			
			cout << "SEARCHTRY " << i << "/" << maxSearchRetries << endl;
			fflush(stdout);
			
			sessionFound = jdplay->searchOnce();
		}

		if(!sessionFound){
			cout << "NOTFOUND" << endl;
			fflush(stdout);
			return;
		}else{
			cout << "FOUND" << endl;
			fflush(stdout);
		}
	}
	
	int ret = jdplay->launch(startGame);
	if(!ret){
		cout << "ERR" << endl;
		fflush(stdout);
	}else{
		cout << "FIN" << endl;
		fflush(stdout);
	}
}

void printHelp(){
	cout << endl << "JDPlay usage:" << endl
	     << "    jdplay.exe --playerName <NAME> --maxSearchRetries <NUMBER>  --searchValidationCount <NUMBER> [--debug]" << endl
		 << endl
		 << "    --help                   print this help and exit" << endl
		 << "    --playerName             the name of the player" << endl
		 << "    --maxSearchRetries       how often to retry when " << endl
		 << "                               searching for a session" << endl
		 << "    --searchValidationCount  how often to validate a found session" << endl
		 << "    --debug                  print debug messages" << endl
		 << endl
		 << "This is a remote controlled program for launching DirectPlay games via RippleLaunch. "
		 << endl
		 << "Explanation of remote control:" << endl
		 << "  # JDPlay is started and waits for the first command" << endl
		 << "    OUT: RDY" << endl
		 << "  # remote app wants to initialize a game" << endl
		 << "    IN:  INITIALIZE gameGUID:{BC3A2ACD-FB46-4c6b-8B5C-CD193C9805CF} hostIP:192.168.2.101 isHost:false maxPlayers:2" << endl
		 << "    IN:  DONE" << endl
		 << "  # JDPlay understood the command and launches" << endl
		 << "    OUT: ACK" << endl
		 << "  # the initalization process takes some seconds" << endl
		 << "    OUT: FIN" << endl
		 << "    OUT: RDY" << endl
		 << "  # now, any other command can follow, you can even initialize again for a different game" << endl
		 << endl
		 << "  # a game is initialized properly, so lets launch it" << endl
		 << "    OUT: RDY" << endl
		 << "  # remote app wants to launch the game" << endl
		 << "    IN:  LAUNCH doSearch:true startGame:true" << endl
		 << "    IN:  DONE" << endl
		 << "  # JDPlay understood the command, searches for a session and launches" << endl
		 << "    OUT: ACK" << endl
		 << "    OUT: FOUND # NOTFOUND if no Session was found, aborts launch" << endl
		 << "  # game has been closed" << endl
		 << "    OUT: FIN" << endl
		 << "    OUT: RDY" << endl
		 << "  # now, the game could be relaunched by sending LAUNCH again" << endl
		 << endl
		 << "  # there's also the possibility to change the playername after a RDY" << endl
		 << "    OUT: RDY" << endl
		 << "    IN:  UPDATE playerName:subes" << endl
		 << "    IN:  DONE" << endl
		 << "    OUT: ACK" << endl
		 << "    OUT: RDY" << endl
		 << endl
		 << "  # use this to detect if the exe is still running" << endl
		 << "    OUT: RDY" << endl
		 << "    IN:  STILLALIVETEST" << endl
		 << "    IN:  DONE" << endl
		 << "    IN:  ACK" << endl
		 << "    IN:  RDY" << endl
		 << endl
		 << "  # write DONE after each command you give, this is used as a workaround to shutdown, when JDPlay looses your process" << endl
		 << endl
		 << "  # this happens when gibberish is read" << endl
		 << "    OUT: RDY" << endl
		 << "    IN:  jsdakfsdfkh" << endl
		 << "    IN:  NAK" << endl
		 << "    OUT: RDY" << endl
		 << endl
		 << "  # this indicates an error and may be printed instead of FIN" << endl
		 << "    OUT: ERR" << endl
		 << endl
		 << "Parameters are always mandatory, though when a game is initialized as host, the doSearch parameters value to LAUNCH gets ignored."
		 << "Whitespace between parameters and the order of the parameters has to be correct. Here is a detailed list of available commands:" << endl
		 << endl
		 << "  INITIALIZE gameGUID:<e.g. {BC3A2ACD-FB46-4c6b-8B5C-CD193C9805CF}> hostIP:<e.g. 192.168.0.3> isHost:<true or false> maxPlayers:<int> #maxPlayers:0 means unlimited" << endl
		 << "  LAUNCH doSearch:<bool> startGame:<bool> #if startGame is false, launching the game will be skipped" << endl
		 << "  UPDATE playerName:<NAME>" << endl
		 << endl
		 << "Every command ends with and is recognized after an endline (\\n). Commands have to be written in UPPERCASE. "
		 << "Before a RDY is written, stdin gets flushed, so theres no possibility that old text is read. JDPlay may print stuff that can be ignored while remote controlling."
		 << "To launch a different game or to connect to a different host, JDPlay has to be restarted with different arguments. "
		 << endl;
	fflush(stdout);
}
