//#pragma once
//
//#include "CoreMinimal.h"
//#include "UObject/NoExportTypes.h"
//#include "libpq-fe.h" // PostgreSQL C library
//#include "DatabaseManager.generated.h"
//
//// Enum для результата игры
//UENUM(BlueprintType)
//enum class EGameResult : uint8
//{
//    InProgress,
//    WhiteWins,
//    BlackWins,
//    Draw
//};
//
//UCLASS(Blueprintable)
//class RTX_CHESS_API UDatabaseManager : public UObject
//{
//    GENERATED_BODY()
//
//public:
//    UDatabaseManager();
//    ~UDatabaseManager();
//
//    /**
//     * Подключается к базе данных PostgreSQL, используя настройки, заданные в Blueprint.
//     * @return true, если подключение успешно.
//     */
//    UFUNCTION(BlueprintCallable, Category = "Database")
//    bool Connect();
//
//    /**
//     * Отключается от базы данных.
//     */
//    UFUNCTION(BlueprintCallable, Category = "Database")
//    void Disconnect();
//
//    /**
//     * Сохраняет информацию о новой игре в базу данных.
//     * @param WhitePlayerName Имя белого игрока.
//     * @param BlackPlayerName Имя черного игрока.
//     * @param OutGameID ID созданной игры (возвращаемое значение).
//     * @return true, если игра успешно сохранена.
//     */
//    UFUNCTION(BlueprintCallable, Category = "Database")
//    bool SaveNewGame(const FString& WhitePlayerName, const FString& BlackPlayerName, int64& OutGameID);
//
//    /**
//     * Сохраняет ход в базу данных.
//     * @param GameID ID игры, к которой относится ход.
//     * @param MoveNumber Номер хода.
//     * @param MoveNotation Нотация хода (например, "e2e4").
//     * @param FEN Состояние доски после хода.
//     * @return true, если ход успешно сохранен.
//     */
//    UFUNCTION(BlueprintCallable, Category = "Database")
//    bool SaveMove(int64 GameID, int32 MoveNumber, const FString& MoveNotation, const FString& FEN);
//
//    /**
//     * Обновляет результат завершенной игры.
//     * @param GameID ID игры для обновления.
//     * @param Result Результат игры.
//     * @return true, если результат успешно обновлен.
//     */
//    UFUNCTION(BlueprintCallable, Category = "Database")
//    bool UpdateGameResult(int64 GameID, EGameResult Result);
//
//
//protected:
//    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Database|Connection")
//    FString Host = TEXT("localhost");
//
//    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Database|Connection")
//    FString Port = TEXT("2112");
//
//    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Database|Connection")
//    FString User = TEXT("postgres");
//
//    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Database|Connection")
//    FString Password = TEXT("2112"); // ЗАМЕНИТЕ НА ВАШ ПАРОЛЬ
//
//    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Database|Connection")
//    FString DBName = TEXT("chess_db");
//
//private:
//    PGconn* DBConnection;
//
//    // Вспомогательная функция для создания таблиц, если они не существуют.
//    void InitializeDatabaseSchema();
//
//    FString GameResultToString(EGameResult Result);
//};
