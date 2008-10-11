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

/* Main.java
 *  
 * 	This file demonstrates the usage of the JDPlay_jni.dll.
 *
 */

/****************************************************************************************************************/


package jdplay_jni_example;

import jdplay.JDPlay;

public class Main {

    public static void main(String[] args) {
        //this loads the JDPlay_jni.dll which has been designed for windows
		//on linux we have to use JDPlay_rmt.exe started via wine or cedega
        System.loadLibrary("lib/JDPlay_jni");
        
        //initializing JDPlay with the parameters: playerName, maxSearchRetries, debug
        JDPlay jdplay = new JDPlay("subes", 5, true);
        
        //playername can be changed anytime
        jdplay.updatePlayerName("subes2");
        
        //initializing a game with parameters: gameGUID, hostIP, isHost
        boolean initialized = jdplay.initialize("{BC3A2ACD-FB46-4c6b-8B5C-CD193C9805CF}", "127.0.0.1", true); //takes some seconds
        
        if(initialized){
            //game is ready for launching
        }else{
            //some error occured, see debug output -- maybe dplay dlls are not setup properly
        }
        
        if(initialized){
            //launch game with parameter: doSearch
            boolean launched = jdplay.launch(false); //blocks while game is running

            if(launched){
                //game has been launched and did exit properly
            }else{
                //some error occured, see debug output -- maybe game not installed
            }
            
            //game can be relaunched with same settings anytime by calling launch() again
            //you don't have to create a new JDPlay object for launching a different game, just call initialize() with different arguments again
        }
    }

}
