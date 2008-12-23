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

/* JDPlay_rmt.cpp
 *
 *	This is a remote controlled program for launching DirectPlay games via RippleLaunch.
 *	It is supposed to give the functionality of the JDPLay_jni.dll with an environment like wine or cedega.
 *
 */

/****************************************************************************************************************/

#include "..\\JDPlay\\JDPlay.h"
#include <string>
#include <sstream>
#include <conio.h>

using namespace std;

#define MAX_DONECOUNTER 10

// *** Variable declarations ***
JDPlay* jdplay;
bool debug;

//used to realize if father process dies -> detectable if we get sent "DONE" endlessly
int doneCounter;

// *** Method declarations ***
void waitForCommand();
void initialize(char* gameGUID, char* hostIP, bool isHost);
void launch(bool doSearch);
void printHelp();

// *** Method implementations ***
int main(int argc, char* argv[]){
	//Read args ************************************************
	debug = false;
	doneCounter = 0;
	char* playerName;
	int maxSearchRetries;

	//Go through arguments
	bool playerfound = false;
	bool retriesfound = false;

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

	//Init JDPlay *********************************************************
	jdplay = new JDPlay(playerName, maxSearchRetries, debug);

	while(true){
		waitForCommand();
	}
}

void waitForCommand(){
	//Flush stdin
	while(_kbhit()){
		_getch();
	}
	
	//Now ready for command
	cout << "RDY" << endl;
	fflush(stdout);

	//Read the command
	static char in[MAX_PATH]; //should be enough
	gets(in);

	string input(in);

	if(!input.compare("DONE")){
		if(doneCounter == MAX_DONECOUNTER){
			exit(1);
		}
		doneCounter++;
	}else{
		doneCounter = 0;
		if(!input.substr(0,11).compare("INITIALIZE ") && input.find(" gameGUID:") != string::npos && input.find(" hostIP:") != string::npos && input.find(" isHost:") != string::npos){
			//Initialize
			
			//INITIALIZE gameGUID:<e.g. {BC3A2ACD-FB46-4c6b-8B5C-CD193C9805CF}> hostIP:<e.g. 192.168.0.3> isHost:<true or false> 
			string s_gameGUID = input.substr(input.find(" gameGUID:")+10, input.find(" hostIP:")-input.find(" gameGUID:")-10);
			string s_hostIP = input.substr(input.find(" hostIP:")+8, input.find(" isHost:")-input.find(" hostIP:")-8);
			string s_isHost = input.substr(input.find(" isHost:")+8);

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
			char hostIP[256]; //could also be a domain name
			strcpy(gameGUID, s_gameGUID.c_str());
			
			strcpy(hostIP, s_hostIP.c_str());
			
			cout << "ACK" << endl;
			fflush(stdout);
			initialize(gameGUID, hostIP, isHost);
		}else
		if(!input.substr(0,7).compare("LAUNCH ") && input.find(" doSearch:") != string::npos){
			//Launch game
			string s_doSearch = input.substr(input.find(" doSearch:")+10);

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

			cout << "ACK" << endl;
			fflush(stdout);
			launch(doSearch);
		}else
		if(!input.substr(0,7).compare("UPDATE ")){
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
			
			strcpy(playerName, s_playerName.c_str());
			
			cout << "ACK" << endl;
			fflush(stdout);
			jdplay->updatePlayerName(playerName);
		}else{
			//Unknown command
			cout << "NAK" << endl;
			fflush(stdout);
		}
	}

	//Read DONE
	gets(in);
	if(strcmp(in, "DONE")){
		cout << "You have to end each command conversation with DONE!" << endl;
		fflush(stdout);
	}
}

void initialize(char* gameGUID, char* hostIP, bool isHost){
	bool ret = jdplay->initialize(gameGUID, hostIP, isHost);
	if(!ret){
		cout << "ERR" << endl;
		fflush(stdout);
	}else{
		cout << "FIN" << endl;
		fflush(stdout);
	}
}

void launch(bool doSearch){
	bool ret = jdplay->launch(doSearch);
	if(!ret){
		cout << "ERR" << endl;
		fflush(stdout);
	}else{
		cout << "FIN" << endl;
		fflush(stdout);
	}
}

void printHelp(){
	cout << endl << "JDPlay_rmt usage:" << endl
	     << "    JDPlay_rmt.exe --playerName <NAME> --maxSearchRetries <NUMBER> [--debug]" << endl
		 << endl
		 << "    --help          print this help and exit" << endl
		 << "    --playerName        the name of the player" << endl
		 << "    --maxSearchRetries  how often to retry when " << endl
		 << "                            searching for a session" << endl
		 << "    --debug         print debug messages" << endl
		 << endl
		 << "This is a remote controlled program for launching DirectPlay games via RippleLaunch. "
		 << endl
		 << "Explanation of remote control:" << endl
		 << "  # JDPlay is started and waits for the first command" << endl
		 << "    OUT: RDY" << endl
		 << "  # remote app wants to initialize a game" << endl
		 << "    IN:  INITIALIZE gameGUID:{BC3A2ACD-FB46-4c6b-8B5C-CD193C9805CF} hostIP:192.168.0.3 isHost:false" << endl
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
		 << "  # remote app wants to launch the game with searching for a session" << endl
		 << "    IN:  LAUNCH doSearch:true" << endl
		 << "    IN:  DONE" << endl
		 << "  # JDPlay understood the command and launches" << endl
		 << "    OUT: ACK" << endl
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
		 << "  # write DONE after each command you give, this is used as a workaround to shutdown, when JDPlay_rmt looses your process" << endl
		 << endl
		 << "  # this happens when gibberish is read" << endl
		 << "    OUT: RDY" << endl
		 << "    IN:  jsdakfsdfkh" << endl
		 << "    OUT: NAK" << endl
		 << "    OUT: RDY" << endl
		 << endl
		 << "  # this indicates an error and may be printed instead of FIN" << endl
		 << "    OUT: ERR" << endl
		 << endl
		 << "Parameters are always mandatory, though when a game is initialized as host, the doSearch parameters value to LAUNCH gets ignored."
		 << "Whitespace between parameters and the order of the parameters has to be correct. Here is a detailed list of available commands:" << endl
		 << endl
		 << "  INITIALIZE gameGUID:<e.g. {BC3A2ACD-FB46-4c6b-8B5C-CD193C9805CF}> hostIP:<e.g. 192.168.0.3> isHost:<true or false>" << endl
		 << "  LAUNCH doSearch:<true or false>" << endl
		 << "  UPDATE playerName:<NAME>" << endl
		 << endl
		 << "Every command ends with and is recognized after an endline (\\n). Commands have to be written in UPPERCASE. "
		 << "Before a RDY is written, stdin gets flushed, so theres no possibility that old text is read. JDPlay may print stuff that can be ignored while remote controlling."
		 << "To launch a different game or to connect to a different host, JDPlay has to be restarted with different arguments. "
		 << endl;
	fflush(stdout);
}
