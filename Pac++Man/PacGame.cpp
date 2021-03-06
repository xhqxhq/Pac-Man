#include "PacGame.h"

/****************************************************************************
Function: Reset
Parameter(s): N/A
Output: N/A
Comments: Reset Game States and re-render all objects to screen
****************************************************************************/
void PacGame::Reset()
{
    ghostMultiplier = 1;
    vulnerabilityTimer = 0;

    // Initialize Player object and status
    mPlayer.setMaxValidWidth(mGameMap.getMapEdge());
    mPlayer.Reset();

    // Initialize all AI objects and status
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        mGhosts[i].setGhostColor(GREEN + i);
        mGhosts[i].Reset();
        mGhosts[i].setMaxValidWidth(mGameMap.getMapEdge());
        if (i == 0) {
            mGhosts[i].setTarget(&mPlayer);
            mGhosts[i].initializeGhost();
        }
    }

    COORD Position;
    Position.X = 0;
    Position.Y = 0;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Position);
    // Render all components
    mGameMap.clearRenderQueue();
    mGameMap.renderMap(true);
    mScoreBoard.Render();
    mLivesBoard.Render();
    mCreditsBoard.Render();
    RenderAI();
    mPlayer.Render();
} // END Reset

/****************************************************************************
Function: RestartLevel
Parameter(s): N/A
Output: N/A
Comments: Called when Player is caught by Ghosts or Level Completes
****************************************************************************/
void PacGame::RestartLevel() 
{
    mGameMap.setCharacterAtPosition(' ', (int)mPlayer.getXPosition(), (int)mPlayer.getYPosition());
    Reset();
    restartDelayTimer = GetTickCount();
    mLivesBoard.decLives();

    if (mLivesBoard.getLivesLeft() >= 0) {
        RenderStatusText(READY_TEXT);
        gameState = READY;
    }
    else {
        RenderStatusText(GAMEOVER_TEXT);
        gameState = GAME_OVER;
    }

} // END RestartLevel

/****************************************************************************
Function: TriggerNewLevel
Parameter(s): N/A
Output: N/A
Comments: Called to ready the game for the next level to be loaded.
****************************************************************************/
void PacGame::TriggerNewLevel() 
{
    if (gameState == GAME_OVER) {
        // GAME_OVER game state reset the score/lives
        // and the level
        mLivesBoard.setLivesLeft(MAX_VISIBLE_LIVES);
        mScoreBoard.Reset();
        mGameMap.setCurrentLevel(1);
    }
    mGameMap.loadMap();
    Reset();
    restartDelayTimer = GetTickCount();
} // END TriggerNewLevel

/****************************************************************************
Function: PauseGame
Parameter(s): N/A
Output: N/A
Comments: Moves Game State between PAUSED and RUNNING states to allow for
users to take a break.
****************************************************************************/
void PacGame::PauseGame() 
{
    if (gameState == RUNNING) {
        gameState = PAUSED;
        RenderStatusText(PAUSED_TEXT);
    }
    else if (gameState == PAUSED) {
        gameState = RUNNING;
        lastAISpawnTime = GetTickCount();
        ClearStatusText();
    }
} // END PauseGame

/****************************************************************************
Function: Update
Parameter(s): N/A
Output: N/A
Comments: Game Update on time interval
****************************************************************************/
void PacGame::Update(double timeStep)
{
    if (gameState == RUNNING) {
        if (mGameMap.getTotalDotsRemaining() <= 0) {
            mGameMap.incrementCurrentLevel();
            restartDelayTimer = GetTickCount();
            RenderStatusText(mGameMap.getCurrentLevelString());
            gameState = NEXT_LEVEL;
        }
        else if (!IsGameRunning()) {
            RenderStatusText(GAMEOVER_TEXT);
            restartDelayTimer = GetTickCount();
            gameState = GAME_OVER;
        }
        
        // Move Character
        UpdatePlayerCharacter(timeStep);

        // Move AI
        UpdateAICharacters(timeStep);

        if (vulnerabilityTimer > 0 && GetTickCount() - vulnerabilityTimer > VULNERABILITY_TIME_LIMIT) {
            // Reset all of the AI Characters Vulnerability
            setAllGhostsVulnerable(false);
        }
    }
} // END Update

/****************************************************************************
Function: CanMoveInSpecifiedDirection
Parameter(s): int - Enum value (See @Constants.h) for valid Directions input.
int - X coordinate of Position to check
int - Y coordinate of Position to check
int - Movement speed (default = 1)
Output: bool - True if move is allowed, False if not.
Comments: Universal check for all Entities on the Map for
whether next position is valid move.
****************************************************************************/
bool PacGame::CanMoveInSpecifiedDirection(int direction, int xPos, int yPos, int movementSpeed)
{
    GameMap::RenderQueuePosition posToCheck(xPos, yPos);
    switch (direction)
    {
    case LEFT:
        posToCheck.xPos -= movementSpeed;
        break;
    case RIGHT:
        posToCheck.xPos += movementSpeed;
        break;
    case UP:
        posToCheck.yPos -= movementSpeed;
        break;
    case DOWN:
        posToCheck.yPos += movementSpeed;
        break;
    }
    return mGameMap.checkForEmptySpace(posToCheck);
} // END CanMoveInSpecifiedDirection

/****************************************************************************
Function: TriggerGhostEaten
Parameter(s): GhostEntity & - Reference to the Ghost that was just caught
by player.
Output: N/A
Comments: Resets the GhostEntity back to Spawn Box position with Respawn Timer.
****************************************************************************/
void PacGame::TriggerGhostEaten(GhostEntity &entity) 
{
    mScoreBoard.addScoreTotal(GHOST_SCORE_AMOUNT*ghostMultiplier++);
    entity.Reset();
    entity.setRespawnTimer(GetTickCount());
    entity.Render();
} // END TriggerGhostEaten

/****************************************************************************
Function: UpdateAICharacters
Parameter(s): N/A
Output: N/A
Comments: Update all Active AI currently on the Map
****************************************************************************/
void PacGame::UpdateAICharacters(double timeStep) 
{
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (!mGhosts[i].isActive()) {
            int respawnTimer = mGhosts[i].getRespawnTimer();
            if (GetTickCount() - lastAISpawnTime > GHOST_SPAWN_TIMER && (!respawnTimer || GetTickCount() - respawnTimer > GHOST_SPAWN_TIMER * 4)) {
                mGameMap.pushRenderQueuePosition(GameMap::RenderQueuePosition((int)mGhosts[i].getXPosition(), (int)mGhosts[i].getYPosition()));
                mGhosts[i].initializeGhost();
                mGhosts[i].setTarget(&mPlayer);
                lastAISpawnTime = GetTickCount();
            }
            continue;
        }
        int xPos = (int)mGhosts[i].getXPosition();
        int yPos = (int)mGhosts[i].getYPosition();

        mGhosts[i].Update(mGameMap.getAvailableDirectionsForPosition(xPos, yPos), timeStep);

        if (mGhosts[i].IsInvalidated()) {
            mGameMap.pushRenderQueuePosition(GameMap::RenderQueuePosition(xPos, yPos));
        }
    } // END For(i<MAX_ENEMIES)

    CheckCollisions();
} // END UpdateAICharacters

/*********************************************************************************
Function: CheckCollisions
Parameter(s): N/A
Output: N/A
Comments: Checks for collisions of Player against the Map and Active Ghosts.
*********************************************************************************/
void PacGame::CheckCollisions() 
{
    char charAtPos = ' ';
    int xPos = (int)mPlayer.getXPosition();
    int yPos = (int)mPlayer.getYPosition();
    charAtPos = mGameMap.getCharacterAtPosition(xPos, yPos);

    if (charAtPos == NORML_PELLET_CHARACTER)
    {
        mGameMap.decrementDotsRemaining();
        mGameMap.setCharacterAtPosition(' ', xPos, yPos);
        mScoreBoard.addPointsForPickup(charAtPos);
    }
    else if (charAtPos == POWER_PELLET_CHARACTER)
    {
        mGameMap.decrementDotsRemaining();
        mGameMap.setCharacterAtPosition(' ', xPos, yPos);
        setAllGhostsVulnerable(true);
        ghostMultiplier = 1;
        vulnerabilityTimer = GetTickCount();
        mScoreBoard.addPointsForPickup(charAtPos);
    }

    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (!mGhosts[i].isActive()) {
            continue;
        }

        if (xPos == (int)mGhosts[i].getXPosition() && yPos == (int)mGhosts[i].getYPosition()) {
            if (mGhosts[i].isVulnerable()) {
                TriggerGhostEaten(mGhosts[i]);
            }
            else {
                RestartLevel();
            }
        }
    }
} // END CheckCollisions

/*********************************************************************************
Function: UpdatePlayerCharacter
Parameter(s): N/A
Output: N/A
Comments: Input Update for Player object.  Command Queue logic used for ensuring
the next possible move in the direction queued will be attempted.
*********************************************************************************/
void PacGame::UpdatePlayerCharacter(double timeStep)
{
    int cacheXPos = (int)mPlayer.getXPosition();
    int cacheYPos = (int)mPlayer.getYPosition();
    mPlayer.Update(mGameMap.getAvailableDirectionsForPosition(cacheXPos, cacheYPos), timeStep);
    int xPos = (int)mPlayer.getXPosition();
    int yPos = (int)mPlayer.getYPosition();

    // Player is invalidated if a Move has occurred
    if (mPlayer.IsInvalidated()) {
        mGameMap.pushRenderQueuePosition(GameMap::RenderQueuePosition(cacheXPos, cacheYPos));
        CheckCollisions();
    }
} // END UpdatePlayerCharacter

void PacGame::UpdatePlayerDirection(int direction)
{
    if (CanMoveInSpecifiedDirection(direction, (int)mPlayer.getXPosition(), (int)mPlayer.getYPosition()))
    {
        mPlayer.setMovementDirection(direction);
    }
} // END UpdatePlayerDirection

/****************************************************************************
Function: setAllGhostsVulnerable
Parameter(s): bool - Boolean value used to set whether the Ghosts are
vulnerable after Player collides with specific object.
Output: N/A
Comments: Updates all Active Ghost objects on Map vulnerability status.
****************************************************************************/
void PacGame::setAllGhostsVulnerable(bool status) 
{
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        mGhosts[i].setVulnerable(status);
    }
} // END setAllGhostsVulnerable

/****************************************************************************
Function: GatherGamePlayInput
Parameter(s): N/A
Output: N/A
Comments: State machine for the GameState, retrieves input and updates Player
when detected.
****************************************************************************/
void PacGame::GatherGamePlayInput()
{
    switch (gameState)
    {
    case ATTRACT:
    {
        if (!GetAsyncKeyState(VK_ADD))
        {
            creditInserted = false;
        }
        else if (!creditInserted) {
            mCreditsBoard.incCredits();
            creditInserted = true;
        }

        if ((GetAsyncKeyState(VK_NUMPAD1) || GetAsyncKeyState(VK_1)) && mCreditsBoard.getCreditTotal() > 0)
        {
            mCreditsBoard.decCredits();
            ClearStatusText();
            RenderStatusText(READY_TEXT);
            restartDelayTimer = GetTickCount();
            gameState = READY;
        }
        break;
    }
    case RUNNING:
    {
        if (GetAsyncKeyState(VK_LEFT))
        {
            UpdatePlayerDirection(LEFT);
        }
        if (GetAsyncKeyState(VK_RIGHT))
        {
            UpdatePlayerDirection(RIGHT);
        }
        if (GetAsyncKeyState(VK_UP))
        {
            UpdatePlayerDirection(UP);
        }
        if (GetAsyncKeyState(VK_DOWN))
        {
            UpdatePlayerDirection(DOWN);
        }
        if (GetAsyncKeyState(VK_SPACE))
        {
            PauseGame();
        }
        break;
    }
    case READY:
    {
        if (GetTickCount() - restartDelayTimer > 3000) {
            gameState = RUNNING;
            lastAISpawnTime = GetTickCount();
            ClearStatusText();
            vulnerabilityTimer = 0;
        }
    }
    case PAUSED:
        if (GetAsyncKeyState(VK_SPACE))
        {
            PauseGame();
        }
        break;
    case NEXT_LEVEL:
        if (GetTickCount() - restartDelayTimer > 3000) {
            TriggerNewLevel();
            gameState = READY;
            RenderStatusText(READY_TEXT);
        }
        break;
    case GAME_OVER:
    {
        if (GetTickCount() - restartDelayTimer > 3000) {
            ClearStatusText();
            TriggerNewLevel();
            RenderStatusText(PRESS_START_TEXT);
            gameState = ATTRACT;
        }
    }
    default:
        break;
    };
} // END GatherGamePlayInput

/****************************************************************************
Function: Render
Parameter(s): N/A
Output: N/A
Comments: Renders the entire frame and update for all Objects for Game.
****************************************************************************/
void PacGame::Render()
{
    mScoreBoard.Render();
    mLivesBoard.Render();
    mCreditsBoard.Render();
    mGameMap.renderMap();
    mPlayer.Render();
    RenderAI();

    // TODO: Add the RenderEngine to run through all TrackedEntities for Rendering
    //          Score, Lives, Credits, etc.
} // END Render

/****************************************************************************
Function: RenderAI
Parameter(s): N/A
Output: N/A
Comments: Renders the Active Ghost AI on-screen at their present positions.
****************************************************************************/
void PacGame::RenderAI()
{
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        mGhosts[i].Render();
    }
} // END RenderAI

/****************************************************************************
Function: RenderStatusText
Parameter(s): const char * - String to display in the line below the Ghost
Spawn box.
Output: N/A
Comments: Renders string to screen below Ghost Spawn box.
****************************************************************************/
void PacGame::RenderStatusText(const char *stringToDisplay) 
{
    COORD Position;
    Position.X = STATUS_TEXT_OFFSET_MARGIN;
    Position.Y = 16;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Position);
    // Attempt to pad the string display to center the text 
    // under the Ghost Spawn box
    int length = strlen(stringToDisplay);
    length = (15 - length) / 2;
    for (int i = 0; i < length; ++i) {
        cout << ' ';
    }
    cout << stringToDisplay;
} // END RenderStatusText

/****************************************************************************
Function: ClearStatusText
Parameter(s): N/A
Output: N/A
Comments: Clears the Text from the screen for gameplay at the line below
the Ghost Spawn box
****************************************************************************/
void PacGame::ClearStatusText() 
{
    COORD Position;
    Position.X = STATUS_TEXT_OFFSET_MARGIN;
    Position.Y = 16;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Position);
    cout << CLEAR_STATUS_TEXT;
} // END ClearStatusText