
// Fill out your copyright notice in the Description page of Project Settings.


#include "NetworkedGameMode.h"

void ANetworkedGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);


	APlayerCharacter* pawn = Cast<APlayerCharacter>(NewPlayer->GetPawn());
	if (pawn)
	{
		pawn->SetPlayerColor(DefaultPlayerColors[playersConnected]);
		playersConnected++;
	}
	playersList.Add(NewPlayer);
}
