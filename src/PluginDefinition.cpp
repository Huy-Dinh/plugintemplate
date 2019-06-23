//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include "menuCmdID.h"
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>

using namespace std;

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE /*hModule*/)
{
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{

    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );
    setCommand(0, TEXT("Convert to NMEA"), hello, NULL, false);
    setCommand(1, TEXT("Hello (with dialog)"), helloDlg, NULL, false);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

#define CONFIG_FILE					"plugins\\eBikeParser\\parser_config"
#define ENDING_CHARACTER			(0xfa)

size_t positionOfNextSentence(std::string& inputString, size_t lastPosition, vector<std::string> & inputSentenceVector)
{
	size_t beginPosition = lastPosition + 1;
	size_t nextSentencePosition = std::string::npos;

	for (unsigned int i = 0; i < inputSentenceVector.size(); ++i)
	{
		size_t currentFirstPosition = inputString.find(inputSentenceVector[i], beginPosition);
		if (currentFirstPosition == std::string::npos)
		{
			inputSentenceVector.erase(inputSentenceVector.begin() + i);
			--i;
			continue;
		}
		else
		{
			nextSentencePosition = currentFirstPosition;
			break;
		}
	}
	for (unsigned int i = 1; i < inputSentenceVector.size(); ++i)
	{
		size_t currentPosition = inputString.find(inputSentenceVector[i], beginPosition);
		// If a message header is no longer found, remove it from the vector 
		// to prevent it from being searched again, could improve performance
		if (currentPosition == std::string::npos)
		{
			inputSentenceVector.erase(inputSentenceVector.begin() + i);
			// Since the last header is removed from the vector, the next index should be the same
			// therefore it is decreased here before being incremented by the for loop at the next iteration
			--i;
			continue;
		}
		// This is only reached if currentPosition != npos
		// We can do the comparison here to find out which header is closer
		if (currentPosition < nextSentencePosition)
		{
			nextSentencePosition = currentPosition;
		}
	}
	return nextSentencePosition;
}

bool isValidMsgChar(unsigned char inputChar)
{
	if ((inputChar == '.') || (inputChar == ','))
	{
		return true;
	}
	else if (((inputChar >= 'a') && (inputChar <= 'z'))
			|| ((inputChar >= '0') && (inputChar <= '9'))
			|| ((inputChar >= 'A') && (inputChar <= 'Z')))
	{
		return true;
	}
	return false;
}

size_t getSentenceSize(std::string& inputString, size_t startPosition)
{
	for (size_t i = startPosition; i < inputString.length(); ++i)
	{
		if (!isValidMsgChar((unsigned char) (inputString.c_str()[i])))
		{
			return i - startPosition;
		}
	}
	return inputString.length() - startPosition;
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//
void hello()
{
	char *lastDocCharArray = NULL;
	int length;
	vector<std::string> vectorOfNMEASentences;
	ifstream configFileStream;
	configFileStream.open(CONFIG_FILE);
	if (!configFileStream.is_open())
	{
		::MessageBox(NULL, TEXT("The parser could not open the config file"), TEXT("ERROR"), MB_OK);
		return;
	}
	std::string currentNMEASentence;
	while (std::getline(configFileStream, currentNMEASentence))
	{
		currentNMEASentence.erase(std::remove(currentNMEASentence.begin(), currentNMEASentence.end(), '\r'), currentNMEASentence.end());
		currentNMEASentence.erase(std::remove(currentNMEASentence.begin(), currentNMEASentence.end(), '\n'), currentNMEASentence.end());
		if (currentNMEASentence.length() > 3)
			vectorOfNMEASentences.push_back(currentNMEASentence);
	}
	// Get the current scintilla
	int which = -1;
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
	if (which == -1)
		return;
	HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

	// Get the contents from the source file in to a string
	length = (int) ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0);
	lastDocCharArray = new char[length + 1];
	::SendMessage(curScintilla, SCI_GETTEXT, length, (LPARAM)lastDocCharArray);

	// Clean the charArray of any 0 in the middle
	for (int i = 0; i < length; ++i)
	{
		if (lastDocCharArray[i] == 0)
		{
			lastDocCharArray[i] = 1;
		}
	}

	std::string lastDocString(lastDocCharArray);
	delete[] lastDocCharArray;

	// Converted NMEA string
	std::string nmeaString = "";
	// Open a new document
	::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);
	// Get the current scintilla
	which = -1;
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
	if (which == -1)
		return;
	curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

	size_t startingPosition = positionOfNextSentence(lastDocString, 0, vectorOfNMEASentences);
	while (startingPosition != std::string::npos)
	{
		size_t lengthOfSentence = getSentenceSize(lastDocString, startingPosition);
		nmeaString += "$" + lastDocString.substr(startingPosition, lengthOfSentence);
		nmeaString += "\n";
		startingPosition = positionOfNextSentence(lastDocString, startingPosition, vectorOfNMEASentences);
	}
	::SendMessage(curScintilla, SCI_SETTEXT, 0, (LPARAM)nmeaString.c_str());
}

void helloDlg()
{
    ::MessageBox(NULL, TEXT("Hello, Notepad++!"), TEXT("Notepad++ Plugin Template"), MB_OK);
}
