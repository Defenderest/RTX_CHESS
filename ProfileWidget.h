#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProfileWidget.generated.h"

class UEditableTextBox;
class UTextBlock;
class UButton;

/**
 * Виджет для отображения и редактирования профиля игрока.
 */
UCLASS()
class RTX_CHESS_API UProfileWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

public:
    /** Возвращает имя игрока для отображения. */
    UFUNCTION(BlueprintPure, Category = "Player Profile")
    FText GetPlayerName() const;

    /** Возвращает рейтинг Эло игрока. */
    UFUNCTION(BlueprintPure, Category = "Player Profile")
    FText GetEloRating() const;

    /** Возвращает страну игрока. */
    UFUNCTION(BlueprintPure, Category = "Player Profile")
    FText GetCountry() const;

    /** Возвращает статистику игр (побед/поражений/ничьих). */
    UFUNCTION(BlueprintPure, Category = "Player Profile")
    FText GetGameStats() const;

    /** Вызывается при нажатии кнопки "Сохранить". Логика уже привязана в C++. */
    UFUNCTION(BlueprintCallable, Category = "Player Profile")
    void OnSaveClicked();

protected:
    /** Поле для ввода нового имени игрока. Назовите его PlayerNameInput в редакторе. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> PlayerNameInput;

    /** Текстовый блок для отображения рейтинга Эло. Назовите его EloRatingText в редакторе. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> EloRatingText;

    /** Текстовый блок для отображения страны. Назовите его CountryText в редакторе. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> CountryText;

    /** Текстовый блок для отображения статистики игр. Назовите его GameStatsText в редакторе. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> GameStatsText;

    /** Кнопка для сохранения изменений. Назовите ее SaveButton в редакторе. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> SaveButton;

private:
    /** Получает текущий профиль игрока из GameInstance. */
    struct FPlayerProfile GetCurrentProfile() const;
};
