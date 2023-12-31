// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NetInfoWidget.generated.h"

/**
 * 
 */
UCLASS()
class LATENCYMITIGATION_API UNetInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "UpdateInfo")
	void UpdateClientInfo(const FVector& clientPosition);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "UpdateInfo")
		void UpdateServerInfo(const FVector& serverPosition);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "UpdateInfo")
		void UpdateAckedMoves(int64 numAcked);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "UpdateInfo")
		void UpdateSentMoves(int64 numSent);
};
