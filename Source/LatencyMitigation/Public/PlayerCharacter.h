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
struct FServerMoveAck
{
	GENERATED_BODY();

	UPROPERTY();
	uint32 moveID = 0;

	UPROPERTY();
	FVector playerLocation{};
	
	UPROPERTY();
	float playerRotation = 0.f;

	UPROPERTY();
	float lookAtRotation = 0.f;
};

USTRUCT()
struct FServerFireAck
{
	GENERATED_BODY();
public:
	UPROPERTY();
	bool hitPlayer = false;

	UPROPERTY();
	FVector StartRay{};

	UPROPERTY();
	FVector EndRay{};
};

USTRUCT()
struct FServerDrawDebug
{
	GENERATED_BODY();
public:
	UPROPERTY();
	FVector OtherPlayerLocation {};

	UPROPERTY();
	FColor DrawColor = FColor::Red;
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
	void ToggleDummy();

	void GetNetworkEmulationSettings();

	UFUNCTION(Server, Unreliable)
		void ServerMove(FPlayerMove input);
	
	UFUNCTION(Server, Unreliable)
		void ServerFire();

	UFUNCTION(Client, Unreliable)
	virtual void ClientFireResponse(FServerFireAck ack);

	UFUNCTION(Client, Unreliable)
	virtual void ClientHitResponse();

	UFUNCTION(Client, Unreliable)
	virtual void ClientDebugResponse(FServerDrawDebug debugInfo);

	UFUNCTION(NetMulticast, Unreliable)
		void MulticastReconcileMove(FServerMoveAck ack);

	UFUNCTION()
		void OnRep_PlayerColor();
	
	virtual void ServerMove_Implementation(FPlayerMove input);

	virtual void ServerFire_Implementation();

	virtual void ClientFireResponse_Implementation(FServerFireAck ack);

	virtual void ClientHitResponse_Implementation();

	virtual void ClientDebugResponse_Implementation(FServerDrawDebug debugInfo);

	virtual void MulticastReconcileMove_Implementation(FServerMoveAck ack);

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
		void UpdateWidget_LandedHit();

	UFUNCTION(BlueprintImplementableEvent, Category = "HitWidget")
		void UpdateWidget_GotHit();
	

	UPROPERTY(EditAnywhere, Category = "Movement")
		float MovementSpeed = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Movement")
		float TurnSpeed = 1.0f;


	UPROPERTY(ReplicatedUsing = OnRep_PlayerColor)
		FLinearColor PlayerColor = FLinearColor::Red;


	UPROPERTY(EditAnywhere, Category = "Shooting")
		float ShotRange = 300.0f;

	UPROPERTY(EditAnywhere, Category = "Shooting")
		bool DrawDebug = false;


	UPROPERTY(EditAnywhere, Category = "Dummy Player")
		float TotalDummyMoveTime = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Dummy Player")
		float DummyInputRate = 0.1f;

	UPROPERTY(EditAnywhere)
		UStaticMeshComponent* PlayerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
		UCameraComponent* PlayerCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
		UCapsuleComponent* Collider;
private:
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void ApplyMovement(const FPlayerMove& move);
	void DrawCollider(const FVector& colliderPosition, const FColor& color = FColor::Red);

	bool bMoveOnForwardAxis = false;
	bool bMoveOnRightAxis = false;
	bool bMoveLookAt = false;
	bool bMovePlayerRot = false;
	bool bMovementToSend = false;
	float forwardAxis = 0.f;
	float rightAxis = 0.f;
	float lookAtAxis = 0.f;
	float playerRotAxis = 0.f;

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

	bool dummy = false;
	float dummyMovementTime = 0.f;
	float timeSinceDummyInput = 0.f;
	bool movingRight = true;

	float halfHeight = 0.f;
	float radius = 0.f;
};