#include "Core/ChessSaveGame.h"

UChessSaveGame::UChessSaveGame()
{
	SaveSlotName = TEXT("ChessPlayerProfile");
	UserIndex = 0;
	// Default values for PlayerProfile are set in its own constructor
}
