/****************************************************************************
File: GameMap.cpp
Author: fookenCode
****************************************************************************/
#include "GameMap.h"
#include <iostream>
#include <fstream>
#include <Windows.h>
#include "Constants.h"

GameMap::GameMap() : mapSizeY(0), mapSizeX(0), totalDots(0), currentLevel(1), 
                     backColor(0), foreColor(0), mapStrings(nullptr), unalteredMapStrings(nullptr) {
    renderQueue.clear();
    loadMap();
    initializeMapObject();
}

GameMap::~GameMap() {
    releaseMapAssetMemory();
}

/****************************************************************************
Function: releaseMapAssetMemory
Parameter(s): N/A
Output: N/A
Comments: Frees all memory from Map buffers.
****************************************************************************/
void GameMap::releaseMapAssetMemory() {
    if (mapStrings != nullptr) {
        for (int i = 0; i < mapSizeY; ++i) {
            if (mapStrings[i] != nullptr) {
                delete[] mapStrings[i];
                mapStrings[i] = nullptr;
            }
        }
        delete[] mapStrings;
        mapStrings = nullptr;
    }
    if (unalteredMapStrings != nullptr) {
        for (int i = 0; i < mapSizeY; ++i) {
            if (unalteredMapStrings[i] != nullptr) {
                delete[] unalteredMapStrings[i];
                unalteredMapStrings[i] = nullptr;
            }
        }
        delete[] unalteredMapStrings;
        unalteredMapStrings = nullptr;
    }
} // END releaseMapAssetMemory

/****************************************************************************
Function: allocateMapAssetMemory
Parameter(s): N/A
Output: N/A
Comments: Frees and re-allocates Map buffers to specified dimensions.
****************************************************************************/
bool GameMap::allocateMapAssetMemory() {
    if (mapStrings != nullptr || unalteredMapStrings != nullptr) {
        releaseMapAssetMemory();
    }

    mapStrings = new char*[mapSizeY];
    unalteredMapStrings = new char*[mapSizeY];
    
    for (int i = 0; i < mapSizeY; ++i) {
        mapStrings[i] = new char[mapSizeX+1];
        unalteredMapStrings[i] = new char[mapSizeX+1];

        if (mapStrings[i] == nullptr || unalteredMapStrings == nullptr) {
            return false;
        }

        memset(mapStrings[i], '\0', sizeof(char)*mapSizeX+1);
        memset(unalteredMapStrings[i], '\0', sizeof(char)*mapSizeX+1);
    }

    
    if (mapStrings != nullptr && unalteredMapStrings != nullptr) {
        return true;
    }

    return false;
} // END allocateMapAssetMemory

/****************************************************************************
Function: initializeMapObject
Parameter(s): N/A
Output: N/A
Comments: Used internally to create the Character Map for game board.
****************************************************************************/
void GameMap::initializeMapObject() {
    for (int i = 0; i < mapSizeY; ++i)
    {
        strcpy_s(mapStrings[i], mapSizeX+1, unalteredMapStrings[i]);
    }

    totalDots = mapLoadedTotalDots;
} // END initializeMapObject

/****************************************************************************
Function: isWallCharacter
Parameter(s): int - X Position within Map
              int - Y Position within Map
              int - Enum value (See @Constants.h) for Walls to check for
Output: N/A
Comments: Used to determine if a move is possible on the Map.
****************************************************************************/
bool GameMap::isWallCharacter(int xPos, int yPos, int wallGroupToTest)
{
    char toCompare = mapStrings[yPos][xPos];

    // Wall characters ordered by frequency in the map for optimal tests
    switch (wallGroupToTest)
    {
    case INNER:
        if (toCompare == '�' || toCompare == '�' || toCompare == '�'  ||
            toCompare == '�' || toCompare == '�' || toCompare == '�' )
        {
            return true;
        }
        break;
    case OUTER:
        if (toCompare == '�' || toCompare == '�' || toCompare == '�' ||
            toCompare == '�' || toCompare == '�' || toCompare == '�')
        {
            return true;
        }
        break;
    case BOTH:
        if (toCompare == '�' || toCompare == '�' || toCompare == '�' ||
            toCompare == '�' || toCompare == '�' || toCompare == '�' ||
            toCompare == '�' || toCompare == '�' || toCompare == '�' ||
            toCompare == '�' || toCompare == '�' || toCompare == '�')
        {
            return true;
        }
        break;

    }
    return false;
} // END isWallCharacter

/****************************************************************************
Function: getAvailableDirectionsForPosition
Parameter(s): int - X position to check in Map.
              int - Y position to check in Map.
Output: Unsigned - Bits representing the available directions.
Comments: Tests all 4 directions from the given position against the
MapStrings and returns a bitmask of the valid directions.
****************************************************************************/
unsigned GameMap::getAvailableDirectionsForPosition(int xPos, int yPos) {
    unsigned returnValue = 0;
    // Left
    if (checkForEmptySpace(xPos - 1, yPos)) {
        returnValue |= LEFT_BIT;
    }
    // Right
    if (checkForEmptySpace(xPos + 1, yPos)) {
        returnValue |= RIGHT_BIT;
    }
    // Down
    if (checkForEmptySpace(xPos, yPos + 1)) {
        returnValue |= DOWN_BIT;
    }
    // Up
    if (checkForEmptySpace(xPos, yPos - 1)) {
        returnValue |= UP_BIT;
    }

    return returnValue;
} // END getAvailableDirectionsForPosition

/****************************************************************************
Function: checkForEmptySpace
Parameter(s): int - X position to check in Map.
int - Y position to check in Map.
Output: Bool - Whether the space is a Dot, PowerDot, or Space.
Comments: Tests position in the MapStrings for non-wall character.
****************************************************************************/
bool GameMap::checkForEmptySpace(int xPos, int yPos) {
    if (xPos >= 0 && xPos <= mapSizeX && yPos >= 0 && yPos < mapSizeY) {
        char toTest = mapStrings[yPos][xPos];
        if (toTest == NORML_PELLET_CHARACTER || toTest == ' ' || toTest == POWER_PELLET_CHARACTER) {
            return true;
        }
    }
    return false;
} // END checkForEmptySpace

/****************************************************************************
Function: checkForEmptySpace (overload)
Parameter(s): RenderQueuePosition & - Reference to struct that holds X/Y
                                      positions.
Output: Bool - Whether the space is a Dot, PowerDot, or Space.
Comments: Tests position in the MapStrings for non-wall character.
****************************************************************************/
bool GameMap::checkForEmptySpace(RenderQueuePosition &posToCheck) {
    if (posToCheck.xPos >= 0 && posToCheck.xPos <= mapSizeX && posToCheck.yPos >= 0 && posToCheck.yPos < mapSizeY) {
        char toTest = mapStrings[posToCheck.yPos][posToCheck.xPos];
        if (toTest == NORML_PELLET_CHARACTER || toTest == ' ' || toTest == POWER_PELLET_CHARACTER) {
            return true;
        }
    }
    return false;
} // END checkForEmptySpace (overload)

/****************************************************************************
Function: renderMap
Parameter(s): N/A
Output: N/A
Comments: Loops over entire Map by each character position to render the 
           Map to the screen.  TODO: Support multiple levels with varying
           colors.
****************************************************************************/
void GameMap::renderMap(bool forceFullRender) {
    using namespace std;
    
    if (forceFullRender) {
        for (int i = 0; i < mapSizeY; ++i)
        {
            for (int j = 0; j < mapSizeX; ++j)
            {
                if (j == 0)
                {
                    cout << "\033[0m";
                    for (int spaces = 0; spaces < SCREEN_OFFSET_MARGIN; ++spaces) {
                        cout << ' ';
                    }
                }

                if (mapStrings[i][j] == POWER_PELLET_CHARACTER || mapStrings[i][j] == NORML_PELLET_CHARACTER || mapStrings[i][j] == ' ')
                {
                    cout << "\033[0m" << mapStrings[i][j];
                }
                else {
                    cout << "\033[" << backColor << ";" << foreColor << ";1m";
                    if (mapStrings[i][j] == MAP_FILLER_CHARACTER) {
                        cout << ' ';
                    }
                    else {
                        cout << mapStrings[i][j];
                    }
                }
            }
            cout << "\033[0m" << endl;
        }
    }
    else {
        RenderQueuePosition toRender;
        COORD Position;
        char charToPrint;
        while (!renderQueue.empty()) {
            // Loop over all of the positions to render to screen
            toRender = renderQueue.back();
            Position.X = toRender.xPos + SCREEN_OFFSET_MARGIN;
            Position.Y = toRender.yPos;
            charToPrint = mapStrings[toRender.yPos][toRender.xPos];
            SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Position);
            if (charToPrint == ' '
                || charToPrint == NORML_PELLET_CHARACTER 
                || charToPrint == POWER_PELLET_CHARACTER )
            {
                cout << "\033[0m" << charToPrint;
            }
            else {
                cout << "\033[" << backColor << ';' << foreColor << ";1m" << charToPrint;
            }

            renderQueue.pop_back();
        }
    }
} // END renderMap

/****************************************************************************
Function: loadMap
Parameter(s): N/A
Output: N/A
Comments: Loads the current level from file, allocates buffers, and
          initializes the UNALTERED buffer for use with game.
****************************************************************************/
bool GameMap::loadMap() {
    using namespace std;

    if (currentLevel > 0) {
        char filename[256];
        sprintf_s(filename, LEVEL_FILENAME_TEMPLATE, currentLevel);
        ifstream mapFileInput;
        mapFileInput.open(filename, fstream::in);
        if (mapFileInput.is_open()) {
            int tempX, tempY;

            mapFileInput >> tempX;
            mapFileInput >> tempY;
            
            // Grab colors from file
            mapFileInput >> foreColor;
            mapFileInput >> backColor;

            // If the dimensions have not changed, don't do expensive memory allocations
            if (tempX != mapSizeX || tempY != mapSizeY) {
                if (mapStrings != nullptr || unalteredMapStrings != nullptr) {
                    releaseMapAssetMemory();
                }

                // Update the dimensions of the map
                mapSizeX = tempX;
                mapSizeY = tempY;

                allocateMapAssetMemory();
            }
            
            /*
                [0][0] [0][1] [0][2] ...
                [1][0] [1][1] [1][2] ...
            */
            
            int count = 0;
            int unicodeChar;
            mapLoadedTotalDots = 0;
            while (!mapFileInput.eof()) {
                
                mapFileInput >> hex >>  unicodeChar;
                unalteredMapStrings[count/mapSizeX][count%mapSizeX] = (char)unicodeChar;
                
                // Check for pellet character to increment internal total field tracking this data.
                if ((char)unicodeChar == POWER_PELLET_CHARACTER || (char)unicodeChar == NORML_PELLET_CHARACTER) {
                    mapLoadedTotalDots++;
                }
                
                count++;
                
                if (count > mapSizeX*mapSizeY) {
                    break;
                }
            }

            for (int i = 0; i < mapSizeY; ++i) {
                unalteredMapStrings[i][mapSizeX] = '\0';
            }
            mapFileInput.close();

            initializeMapObject();
            return true;
        }
        else if (mapLoadedTotalDots > 0) {
            // Re-use the currently loaded map, 
            // if there is no map for the current level number
            initializeMapObject();
        }
    }

    return false;
} // END loadMap

/****************************************************************************
Function: pushRenderQueuePosition
Parameter(s): RenderQueuePosition - Entity position data for location to
                                    render.
Output: N/A
Comments: Adds the RenderQueuePosition to the Vector to limit the tiles of 
          the map that need rendering every frame.
****************************************************************************/
void GameMap::pushRenderQueuePosition(RenderQueuePosition newPos) {
    renderQueue.push_back(newPos);
} // END pushRenderQueuePosition

/****************************************************************************
Function: clearRenderQueue
Parameter(s): N/A
Output: N/A
Comments: Empties the entire GameMap position queue to avoid re-rendering
spaces from reset/new level.
****************************************************************************/
void GameMap::clearRenderQueue() {
    renderQueue.clear();
} // END clearRenderQueue

/****************************************************************************
Function: setCharacterAtPosition
Parameter(s): char - Character to enter at position specified
              int - X Position within Map
              int - Y Position within Map
Output: N/A
Comments: Temporarily used to access the Map to edit the current tile for
          movement and interaction on the map.
****************************************************************************/
void GameMap::setCharacterAtPosition(char toEnter, int xPos, int yPos) {
    if (xPos < 0 || xPos > mapSizeX || yPos < 0 || yPos > mapSizeY) {
        return;
    }

    mapStrings[yPos][xPos] = toEnter;
} // END setCharacterAtPosition

/****************************************************************************
Function: getCharacterAtPosition
Parameter(s): int - X Position within Map
              int - Y Position within Map
Output: char - Character that exists at specified Position on Map
Comments: Temporarily used to access the Map to retrieve character
          information.
****************************************************************************/
char GameMap::getCharacterAtPosition(int xPos, int yPos) {
    if (xPos < 0 || xPos > mapSizeX || yPos < 0 || yPos > mapSizeY) {
        return ' ';
    }
    
    return mapStrings[yPos][xPos];
} // END getCharacterAtPosition

/****************************************************************************
Function: getCurrentLevelString
Parameter(s): N/A
Output: const char * - String representing the "Level <currLevel>" text.
Comments: Used to display in the Status Text field of the map the current
level details.
****************************************************************************/
const char *GameMap::getCurrentLevelString() {
    memset(levelStatusString, 0, sizeof(char)*MAX_LEVEL_STRING_LENGTH);
    if (currentLevel > 0) {
        sprintf_s(levelStatusString, LEVEL_TEMPLATE_TEXT, currentLevel);
    }
    return levelStatusString;
} // END getCurrentLevelString