//#include "DatabaseManager.h"
//
//UDatabaseManager::UDatabaseManager()
//{
//    DBConnection = nullptr;
//}
//
//UDatabaseManager::~UDatabaseManager()
//{
//    Disconnect();
//}
//
//bool UDatabaseManager::Connect()
//{
//    if (DBConnection)
//    {
//        UE_LOG(LogTemp, Warning, TEXT("DatabaseManager: Already connected."));
//        return true;
//    }
//
//    FString ConnInfo = FString::Printf(TEXT("host=%s port=%s dbname=%s user=%s password=%s"),
//        *Host, *Port, *DBName, *User, *Password);
//
//    DBConnection = PQconnectdb(TCHAR_TO_UTF8(*ConnInfo));
//
//    if (PQstatus(DBConnection) != CONNECTION_OK)
//    {
//        UE_LOG(LogTemp, Error, TEXT("DatabaseManager: Connection to database failed: %s"), UTF8_TO_TCHAR(PQerrorMessage(DBConnection)));
//        PQfinish(DBConnection);
//        DBConnection = nullptr;
//        return false;
//    }
//
//    UE_LOG(LogTemp, Log, TEXT("DatabaseManager: Successfully connected to PostgreSQL database '%s'."), *DBName);
//    
//    // Создаем таблицы, если их нет
//    InitializeDatabaseSchema();
//
//    return true;
//}
//
//void UDatabaseManager::InitializeDatabaseSchema()
//{
//    if (!DBConnection) return;
//
//    // Таблица для хранения информации об играх
//    const char* CreateGamesTableQuery =
//        "CREATE TABLE IF NOT EXISTS games ("
//        "id BIGSERIAL PRIMARY KEY,"
//        "white_player_name VARCHAR(255) NOT NULL,"
//        "black_player_name VARCHAR(255) NOT NULL,"
//        "result VARCHAR(50) DEFAULT 'in_progress',"
//        "start_time TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP"
//        ");";
//
//    // Таблица для хранения ходов
//    const char* CreateMovesTableQuery =
//        "CREATE TABLE IF NOT EXISTS moves ("
//        "id BIGSERIAL PRIMARY KEY,"
//        "game_id BIGINT NOT NULL REFERENCES games(id) ON DELETE CASCADE,"
//        "move_number INT NOT NULL,"
//        "move_notation VARCHAR(10) NOT NULL,"
//        "fen_after_move TEXT NOT NULL,"
//        "move_time TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP"
//        ");";
//    
//    PGresult* res_games = PQexec(DBConnection, CreateGamesTableQuery);
//    if (PQresultStatus(res_games) != PGRES_COMMAND_OK)
//    {
//        UE_LOG(LogTemp, Error, TEXT("DatabaseManager: Failed to create 'games' table: %s"), UTF8_TO_TCHAR(PQerrorMessage(DBConnection)));
//    }
//    PQclear(res_games);
//
//    PGresult* res_moves = PQexec(DBConnection, CreateMovesTableQuery);
//    if (PQresultStatus(res_moves) != PGRES_COMMAND_OK)
//    {
//        UE_LOG(LogTemp, Error, TEXT("DatabaseManager: Failed to create 'moves' table: %s"), UTF8_TO_TCHAR(PQerrorMessage(DBConnection)));
//    }
//    PQclear(res_moves);
//}
//
//
//void UDatabaseManager::Disconnect()
//{
//    if (DBConnection)
//    {
//        PQfinish(DBConnection);
//        DBConnection = nullptr;
//        UE_LOG(LogTemp, Log, TEXT("DatabaseManager: Disconnected from database."));
//    }
//}
//
//bool UDatabaseManager::SaveNewGame(const FString& WhitePlayerName, const FString& BlackPlayerName, int64& OutGameID)
//{
//    if (!DBConnection)
//    {
//        UE_LOG(LogTemp, Error, TEXT("DatabaseManager::SaveNewGame: Not connected to database."));
//        return false;
//    }
//
//    const char* Query = "INSERT INTO games (white_player_name, black_player_name) VALUES ($1, $2) RETURNING id;";
//    const char* ParamValues[2] = { TCHAR_TO_UTF8(*WhitePlayerName), TCHAR_TO_UTF8(*BlackPlayerName) };
//    
//    PGresult* res = PQexecParams(DBConnection, Query, 2, NULL, ParamValues, NULL, NULL, 0);
//
//    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1)
//    {
//        UE_LOG(LogTemp, Error, TEXT("DatabaseManager::SaveNewGame: Failed to insert new game: %s"), UTF8_TO_TCHAR(PQerrorMessage(DBConnection)));
//        PQclear(res);
//        return false;
//    }
//
//    // Получаем ID созданной игры
//    OutGameID = FCString::Atoi64(UTF8_TO_TCHAR(PQgetvalue(res, 0, 0)));
//    PQclear(res);
//
//    UE_LOG(LogTemp, Log, TEXT("DatabaseManager: Saved new game with ID: %lld"), OutGameID);
//    return true;
//}
//
//bool UDatabaseManager::SaveMove(int64 GameID, int32 MoveNumber, const FString& MoveNotation, const FString& FEN)
//{
//    if (!DBConnection)
//    {
//        UE_LOG(LogTemp, Error, TEXT("DatabaseManager::SaveMove: Not connected to database."));
//        return false;
//    }
//
//    const FString GameIDStr = FString::FromInt(GameID);
//    const FString MoveNumberStr = FString::FromInt(MoveNumber);
//
//    const char* Query = "INSERT INTO moves (game_id, move_number, move_notation, fen_after_move) VALUES ($1, $2, $3, $4);";
//    const char* ParamValues[4] = {
//        TCHAR_TO_UTF8(*GameIDStr),
//        TCHAR_TO_UTF8(*MoveNumberStr),
//        TCHAR_TO_UTF8(*MoveNotation),
//        TCHAR_TO_UTF8(*FEN)
//    };
//
//    PGresult* res = PQexecParams(DBConnection, Query, 4, NULL, ParamValues, NULL, NULL, 0);
//
//    if (PQresultStatus(res) != PGRES_COMMAND_OK)
//    {
//        UE_LOG(LogTemp, Error, TEXT("DatabaseManager::SaveMove: Failed to insert move: %s"), UTF8_TO_TCHAR(PQerrorMessage(DBConnection)));
//        PQclear(res);
//        return false;
//    }
//
//    PQclear(res);
//    UE_LOG(LogTemp, Log, TEXT("DatabaseManager: Saved move #%d for game ID %lld."), MoveNumber, GameID);
//    return true;
//}
//
//bool UDatabaseManager::UpdateGameResult(int64 GameID, EGameResult Result)
//{
//    if (!DBConnection)
//    {
//        UE_LOG(LogTemp, Error, TEXT("DatabaseManager::UpdateGameResult: Not connected to database."));
//        return false;
//    }
//
//    const FString ResultStr = GameResultToString(Result);
//    const FString GameIDStr = FString::FromInt(GameID);
//
//    const char* Query = "UPDATE games SET result = $1 WHERE id = $2;";
//    const char* ParamValues[2] = {
//        TCHAR_TO_UTF8(*ResultStr),
//        TCHAR_TO_UTF8(*GameIDStr)
//    };
//
//    PGresult* res = PQexecParams(DBConnection, Query, 2, NULL, ParamValues, NULL, NULL, 0);
//
//    if (PQresultStatus(res) != PGRES_COMMAND_OK)
//    {
//        UE_LOG(LogTemp, Error, TEXT("DatabaseManager::UpdateGameResult: Failed to update game result: %s"), UTF8_TO_TCHAR(PQerrorMessage(DBConnection)));
//        PQclear(res);
//        return false;
//    }
//
//    PQclear(res);
//    UE_LOG(LogTemp, Log, TEXT("DatabaseManager: Updated result for game ID %lld to '%s'."), GameID, *ResultStr);
//    return true;
//}
//
//FString UDatabaseManager::GameResultToString(EGameResult Result)
//{
//    switch (Result)
//    {
//        case EGameResult::WhiteWins: return TEXT("white_wins");
//        case EGameResult::BlackWins: return TEXT("black_wins");
//        case EGameResult::Draw: return TEXT("draw");
//        case EGameResult::InProgress:
//        default:
//            return TEXT("in_progress");
//    }
//}
