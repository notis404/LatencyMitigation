#include "PlayerCharacter.h"
#include "NetworkedGameMode.h"
#include "NetworkedPlayerController.h"

// Sets default values
APlayerCharacter::APlayerCharacter() :
	serverMovesToApply{},
	nonAckedMoves{},
	serverPositionsToSimulate{}
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Setup RootComponent
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	PlayerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlayerMesh"));
	PlayerMesh->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> StaticMeshAsset(TEXT("/Script/Engine.StaticMesh'/Game/StarterContent/Shapes/Shape_NarrowCapsule.Shape_NarrowCapsule'"));
	if (StaticMeshAsset.Succeeded())
	{
		PlayerMesh->SetStaticMesh(StaticMeshAsset.Object);
		PlayerMesh->SetMobility(EComponentMobility::Movable);
	}

	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(RootComponent);
	PlayerCamera->SetRelativeLocation(FVector(0.f, 0.f, 100.f));

	Collider = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collider"));
	Collider->SetupAttachment(RootComponent);
	Collider->SetCapsuleSize(25.0f, 50.0f);
	Collider->SetRelativeLocation(FVector(0.f, 0.f, 50.0f));

	
	SetReplicates(true);
	SetReplicateMovement(false);
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		UpdateWidget_ClientInfo(GetActorLocation());

	}
	
	Collider->GetScaledCapsuleSize(radius, halfHeight);
	GetNetworkEmulationSettings();
	
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (dummy)
	{
		if (dummyMovementTime >= TotalDummyMoveTime)
		{
			movingRight = movingRight ? false : true;
			dummyMovementTime = 0.f;
		}

		if (movingRight)
		{
			MoveRight(1.0f);
		}
		else
		{
			MoveRight(-1.0f);
		}

		dummyMovementTime += DeltaTime;
	}

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{

		UpdateWidget_ClientInfo(GetActorLocation());


		if (bMovementToSend)
		{
			bMovementToSend = false;
			FPlayerMove currentMove{};
			if (bMoveOnForwardAxis)
			{
				currentMove.forwardAxis = forwardAxis;
				bMoveOnForwardAxis = false;
				forwardAxis = 0.f;
			}

			if (bMoveOnRightAxis)
			{
				currentMove.rightAxis = rightAxis;
				bMoveOnRightAxis = false;
				rightAxis = 0.f;
			}

			if (bMoveLookAt)
			{
				currentMove.lookAtRotation = lookAtAxis;
				bMoveLookAt = false;
				lookAtAxis = 0.f;
			}

			if (bMovePlayerRot)
			{
				currentMove.playerRotation = playerRotAxis;
				bMovePlayerRot = false;
				playerRotAxis = 0.f;
			}

			currentMove.moveID = nextMoveId++;
			ServerMove(currentMove);
			ApplyMovement(currentMove);
			nonAckedMoves.push(currentMove);


			UpdateWidget_SentMoves(currentMove.moveID);

		}
	}
	else if (GetLocalRole() == ROLE_Authority)
	{
		serverUpdateCounter += DeltaTime;
		if (serverUpdateCounter >= 0.1f)
		{
			FServerMoveAck ack;
			if (!serverMovesToApply.empty())
			{
				FPlayerMove lastMove;
				while (!serverMovesToApply.empty())
				{
					lastMove = serverMovesToApply.front();
					ApplyMovement(lastMove);
					serverMovesToApply.pop();
				}

				ack.moveID = lastMove.moveID;
				ack.playerLocation = GetActorLocation();
				ack.playerRotation = GetActorRotation().Yaw;
				ack.lookAtRotation = PlayerCamera->GetComponentRotation().Pitch;
			}
			else
			{
				ack.moveID = 0;
				ack.playerLocation = GetActorLocation();
				ack.playerRotation = GetActorRotation().Yaw;
				ack.lookAtRotation = PlayerCamera->GetComponentRotation().Pitch;
			}
			MulticastReconcileMove(ack);
			serverUpdateCounter = 0.f;
		}
	}
	else if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		if (startSimulateMovement)
		{
			if (!serverPositionsToSimulate.empty())
			{
				simulatedUpdateCounter += DeltaTime;
				FVector NewLocation = FMath::Lerp(oldestServerPosition, serverPositionsToSimulate.front(), simulatedUpdateCounter / 0.1f);

				if (simulatedUpdateCounter > 0.1f)
				{
					oldestServerPosition = serverPositionsToSimulate.front();
					serverPositionsToSimulate.pop();
					simulatedUpdateCounter = 0.f;
				}

				SetActorLocation(NewLocation);
			}
		}
	}
	
}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &APlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &APlayerCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &APlayerCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &APlayerCharacter::LookUp);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &APlayerCharacter::Fire);
	PlayerInputComponent->BindAction("Dummy", IE_Pressed, this, &APlayerCharacter::ToggleDummy);
}

void APlayerCharacter::GetNetworkEmulationSettings()
{
	APlayerController* PlayerController = GetController<APlayerController>();

	if (PlayerController)
	{
		UNetConnection* NetConnection = PlayerController->GetNetConnection();
		if (NetConnection)
		{
			const FPacketSimulationSettings SimulationSettings = NetConnection->PacketSimulationSettings;
			UE_LOG(LogTemp, Warning, TEXT("Packet Lag: %d"), SimulationSettings.PktLag);
			UE_LOG(LogTemp, Warning, TEXT("PktLagVariance: %d"), SimulationSettings.PktLagVariance);
			UE_LOG(LogTemp, Warning, TEXT("PktLagMax: %d"), SimulationSettings.PktLagMax);
			UE_LOG(LogTemp, Warning, TEXT("PktLagMin: %d"), SimulationSettings.PktLagMin);
			UE_LOG(LogTemp, Warning, TEXT("Packet Loss: %d"), SimulationSettings.PktLoss);
			UE_LOG(LogTemp, Warning, TEXT("PktLossMaxSize: %d"), SimulationSettings.PktLossMaxSize);
			UE_LOG(LogTemp, Warning, TEXT("PktLossMinSize: %d"), SimulationSettings.PktLossMinSize);
			UE_LOG(LogTemp, Warning, TEXT("Packet Incoming Loss: %d"), SimulationSettings.PktIncomingLoss);
			UE_LOG(LogTemp, Warning, TEXT("PktIncomingLagMax: %d"), SimulationSettings.PktIncomingLagMax);
			UE_LOG(LogTemp, Warning, TEXT("PktIncomingLagMin: %d"), SimulationSettings.PktIncomingLagMin);
			UE_LOG(LogTemp, Warning, TEXT("Duplicate Packets: %d"), SimulationSettings.PktDup);
			UE_LOG(LogTemp, Warning, TEXT("PktJitter: %d"), SimulationSettings.PktJitter);
			UE_LOG(LogTemp, Warning, TEXT("PktOrder: %d"), SimulationSettings.PktOrder);
		}
	}
}


void APlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APlayerCharacter, PlayerColor);
}

void APlayerCharacter::ApplyMovement(const FPlayerMove& move)
{
	[[maybe_unused]] auto testPlayerRotationPre = GetActorRotation().Yaw;
	[[maybe_unused]] auto testLookAtRotationPre = PlayerCamera->GetComponentRotation().Pitch;
	AddActorLocalRotation(FRotator{ 0.f, TurnSpeed * move.playerRotation, 0.f });
	PlayerCamera->AddLocalRotation(FRotator{ TurnSpeed * move.lookAtRotation, 0.f, 0.f });
	[[maybe_unused]] auto testPlayerRotationPost = GetActorRotation().Yaw;
	[[maybe_unused]] auto testLookAtRotationPost = PlayerCamera->GetComponentRotation().Pitch;

	FVector NewLocation = GetActorLocation();
	if (move.forwardAxis != 0.f)
	{
		FVector ForwardVector = GetActorForwardVector();
		NewLocation += (ForwardVector * MovementSpeed * move.forwardAxis);
	}

	if (move.rightAxis != 0.f)
	{
		FVector RightVector = GetActorRightVector();
		NewLocation += (RightVector * MovementSpeed * move.rightAxis);
	}
	SetActorLocation(NewLocation);
}

void APlayerCharacter::DrawCollider(const FVector& colliderPosition, const FColor& color)
{
	FVector CapsuleCenterLocal = FVector(0.0f, 0.0f, halfHeight);

	// Convert the local center to world space by adding the component's location
	FVector CapsuleCenterWorld = colliderPosition + CapsuleCenterLocal;

	DrawDebugCapsule(GetWorld(), CapsuleCenterWorld, halfHeight, radius, GetActorRotation().Quaternion(), color, false, 1.0f, 0, 1.0f);
}


void APlayerCharacter::MoveForward(float Axis)
{
	if (Axis != 0.f)
	{
		bMovementToSend = true;
		bMoveOnForwardAxis = true;
		forwardAxis = Axis;
	}
}

void APlayerCharacter::MoveRight(float Axis)
{
	if (Axis != 0.f)
	{
		bMovementToSend = true;
		bMoveOnRightAxis = true;
		rightAxis = Axis;
	}
}

void APlayerCharacter::Turn(float Axis)
{
	if (Axis != 0.f)
	{
		bMovementToSend = true;
		bMovePlayerRot = true;
		playerRotAxis = Axis;
	}
}

void APlayerCharacter::LookUp(float Axis)
{
	if (Axis != 0.f)
	{
		bMovementToSend = true;
		bMoveLookAt = true;
		lookAtAxis = Axis;
	}
}

void APlayerCharacter::Fire()
{
	ServerFire();
	if (DrawDebug)
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), FoundActors);
		for (auto foundActor : FoundActors)
		{
			if (!(foundActor == this))
			{
				DrawCollider(foundActor->GetActorLocation(), FColor::Blue);
			}
		}
	}
}

void APlayerCharacter::ToggleDummy()
{
	dummy = dummy ? false : true;
	dummyMovementTime = TotalDummyMoveTime / 2;
}

void APlayerCharacter::ClientDebugResponse_Implementation(FServerDrawDebug debugInfo)
{
	DrawCollider(debugInfo.OtherPlayerLocation, debugInfo.DrawColor);
}

void APlayerCharacter::OnRep_PlayerColor()
{
	auto oldMaterial = PlayerMesh->GetMaterial(0);
	UMaterialInstanceDynamic* newMaterialInstance = UMaterialInstanceDynamic::Create(oldMaterial, nullptr);
	newMaterialInstance->SetVectorParameterValue(FName("Color"), PlayerColor);
	PlayerMesh->SetMaterial(0, newMaterialInstance);
}

void APlayerCharacter::ServerMove_Implementation(FPlayerMove input)
{
	serverMovesToApply.push(input);
}

void APlayerCharacter::ServerFire_Implementation()
{
	FVector StartVector = PlayerCamera->GetComponentLocation();
	FVector EndVector = StartVector + (PlayerCamera->GetComponentRotation().Vector() * ShotRange);

	FHitResult hitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool hitAnotherPlayer = false;
	AActor* hitActor = nullptr;
	if (GetWorld()->LineTraceSingleByChannel(hitResult, StartVector, EndVector, ECC_Visibility, Params))
	{
		hitActor = hitResult.GetActor();
		if (hitActor)
		{
			APlayerCharacter* hitPlayer = Cast<APlayerCharacter>(hitActor);
			if (hitPlayer)
			{
				hitPlayer->ClientHitResponse();
				hitAnotherPlayer = true;
				
			}
		}
	}

	FServerFireAck ack{};
	ack.hitPlayer = hitAnotherPlayer;
	ack.StartRay = StartVector;
	ack.EndRay = EndVector;
	ClientFireResponse(ack);

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerCharacter::StaticClass(), FoundActors);
	for (auto foundActor : FoundActors)
	{
		if (!(foundActor == this))
		{
			FServerDrawDebug debugInfo{};
			debugInfo.OtherPlayerLocation = foundActor->GetActorLocation();
			debugInfo.DrawColor = hitActor == foundActor ? FColor::Green : FColor::Red;
			ClientDebugResponse(debugInfo);
		}
	}
}

void APlayerCharacter::ClientFireResponse_Implementation(FServerFireAck ack)
{
	if (ack.hitPlayer)
	{
		UpdateWidget_LandedHit();
		if (DrawDebug)
		{
			DrawDebugLine(GetWorld(), ack.StartRay, ack.EndRay, FColor::Green, false, 2.0f, 0, 1.0f);
		}
	}
	else if (DrawDebug)
	{
		DrawDebugLine(GetWorld(), ack.StartRay, ack.EndRay, FColor::Red, false, 2.0f, 0, 1.0f);
	}
}

void APlayerCharacter::ClientHitResponse_Implementation()
{
	UpdateWidget_GotHit();
}

void APlayerCharacter::MulticastReconcileMove_Implementation(FServerMoveAck ack)
{
	auto role = GetLocalRole();
	if (role == ROLE_AutonomousProxy)
	{
		if (ack.moveID != 0)
		{
			uint32 ackedID = 0;
			while (ack.moveID != ackedID)
			{
				ackedID = nonAckedMoves.front().moveID;
				nonAckedMoves.pop();
			}

			[[maybe_unused]] auto testPlayerRotationPre = GetActorRotation().Yaw;
			[[maybe_unused]] auto testLookAtRotationPre = PlayerCamera->GetComponentRotation().Pitch;
			SetActorLocation(ack.playerLocation);
			SetActorRotation(FRotator{ 0.f, ack.playerRotation, 0.f });
			PlayerCamera->SetRelativeRotation(FRotator{ ack.lookAtRotation, 0.f, 0.f });
			[[maybe_unused]] auto testPlayerRotationPost = GetActorRotation().Yaw;
			[[maybe_unused]] auto testLookAtRotationPost = PlayerCamera->GetComponentRotation().Pitch;

			//float cachedLookAt = GetActorRotation().Yaw;

			std::queue<FPlayerMove> tempQueue {nonAckedMoves};
			while (!tempQueue.empty())
			{
				ApplyMovement(tempQueue.front());
				tempQueue.pop();
			}
			//SetActorRotation(FRotator{ 0.f, cachedLookAt, 0.f });


			UpdateWidget_ServerInfo(ack.playerLocation);
			UpdateWidget_AckedMoves(ack.moveID);
		}
	}
	else if (role == ROLE_SimulatedProxy)
	{
		serverPositionsToSimulate.push(ack.playerLocation);
		if (!startSimulateMovement && serverPositionsToSimulate.size() > 2)
		{
			oldestServerPosition = serverPositionsToSimulate.front();
			serverPositionsToSimulate.pop();
			startSimulateMovement = true;
		}
	}

}

void APlayerCharacter::SetPlayerColor(const FLinearColor& newColor)
{
	PlayerColor = newColor;
}

