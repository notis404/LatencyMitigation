// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NetworkedPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "NetInfoWidget.h"
#include "Engine/NetConnection.h"
#include "UMG/Public/UMG.h"
#include <queue>
#include "PlayerCharacter.generated.h"
USTRUCT()
struct FPlayerMove
{
	GENERATED_BODY()

	UPROPERTY();
	uint32 moveID = 0;

	UPROPERTY();
	float forwardAxis = 0.f;
	
	UPROPERTY();
	float rightAxis = 0.f;
	
	UPROPERTY();
	float playerRotation = 0.f;
	
	UPROPERTY();
	float lookAtRotation = 0.f;
};

USTRUCT()
struct FServerAck
{
	GENERATED_BODY();

	UPROPERTY();
	uint32 moveID = 0;

	UPROPERTY();
	FVector playerLocation{};
	
	UPROPERTY();
	float playerRotation = 0.f;

	UPROPERTY();
	float playerForwardSpeed = 0.f;
	
	UPROPERTY();
	float playerRightSpeed = 0.f;
};

UCLASS()
class APlayerCharacter : public APawn
{
	GENERATED_BODY()
public:
	// Sets default values for this pawn's properties
	APlayerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Player Actions
	void MoveForward(float Axis);
	void MoveRight(float Axis);
	void Turn(float Axis);
	void LookUp(float Axis);
	void Fire();

	void GetNetworkEmulationSettings();

	UFUNCTION(Server, WithValidation, Unreliable)
		void ServerMove(FPlayerMove input);

	UFUNCTION(NetMulticast, Unreliable)
		void MulticastReconcileMove(FServerAck ack);

	UFUNCTION()
		void OnRep_PlayerColor();
	
	virtual bool ServerMove_Validate(FPlayerMove input);
	virtual void ServerMove_Implementation(FPlayerMove input);

	virtual void MulticastReconcileMove_Implementation(FServerAck ack);

	void SetPlayerColor(const FLinearColor& newColor);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "NetInfo")
		void UpdateWidget_ClientInfo(const FVector& clientPosition);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "NetInfo")
		void UpdateWidget_SentMoves(int64 numSent);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "NetInfo")
		void UpdateWidget_AckedMoves(int64 numAcked);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "NetInfo")
		void UpdateWidget_ServerInfo(const FVector& serverPosition);

	UFUNCTION(BlueprintImplementableEvent, Category = "HitWidget")
		void UpdateWidget_Hit();
	

	UPROPERTY(EditAnywhere, Category = "Movement")
		float MovementSpeed = 5.0f;
	UPROPERTY(EditAnywhere, Category = "Movement")
		float TurnSpeed = 1.0f;
	UPROPERTY(ReplicatedUsing = OnRep_PlayerColor)
		FLinearColor PlayerColor = FLinearColor::Red;

	UPROPERTY(EditAnywhere, Category = "Shooting")
		float ShotRange = 5.0f;
	UPROPERTY(EditAnywhere, Category = "Shooting")
		bool DrawDebug = false;

	UPROPERTY(EditAnywhere)
		UStaticMeshComponent* PlayerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
		UCameraComponent* PlayerCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
		UCapsuleComponent* Collider;

	UPROPERTY(EditAnywhere)
		UWidgetComponent* NetworkInfoWidgetComponent;
private:
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void ApplyMovement(const FPlayerMove& move);
	bool bMoveOnForwardAxis = false;
	bool bMoveOnRightAxis = false;
	bool bMovementToSend = false;
	float forwardAxis = 0.f;
	float rightAxis = 0.f;

	FVector oldestServerPosition{};
	std::queue<FVector> serverPositionsToSimulate;
	float simulatedRotation = 0.f;
	float simulatedForwardSpeed = 0.f;
	float simulatedRightSpeed = 0.f;
	float simulatedUpdateCounter = 0.f;
	bool startSimulateMovement = false;

	uint32 nextMoveId = 1;
	
	std::queue<FPlayerMove> serverMovesToApply;
	std::queue<FPlayerMove> nonAckedMoves;

	float serverUpdateCounter = 0.f;
};