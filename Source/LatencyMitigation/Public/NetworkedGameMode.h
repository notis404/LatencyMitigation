// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PlayerCharacter.h"
#include "NetworkedPlayerController.h"
#include "NetworkedGameMode.generated.h"

/**
 * 
 */
UCLASS()
class LATENCYMITIGATION_API ANetworkedGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category = "Player Colors")
    TArray<FLinearColor> DefaultPlayerColors;

    // Override the function to handle starting a new player
    virtual void PostLogin(APlayerController* NewPlayer) override;
    TArray<APlayerController*> playersList {};
private:
    int8 playersConnected = 0;
};
