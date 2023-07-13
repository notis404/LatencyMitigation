
// Fill out your copyright notice in the Description page of Project Settings.


#include "NetworkedGameMode.h"

void ANetworkedGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	APlayerCharacter* playerPawn = Cast<APlayerCharacter>(NewPlayer->GetPawn());

	if (playerPawn)
	{
		playerPawn->SetPlayerColor(DefaultPlayerColors[playersConnected++]);
	}
}
