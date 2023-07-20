#include "PlayerCharacter.h"

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

	NetworkInfoWidgetComponent = CreateDefaultSubobject<UWidgetComponent>("NetInfoWidget");
	static ConstructorHelpers::FClassFinder<UNetInfoWidget> netInfoWidgetObj(TEXT("/Game/Blueprints/NetworkInfo"));
	if (netInfoWidgetObj.Succeeded())
	{
		NetworkInfoWidgetComponent->SetWidgetClass(netInfoWidgetObj.Class);
	}
	NetworkInfoWidgetComponent->SetupAttachment(RootComponent);
	NetworkInfoWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	NetworkInfoWidgetComponent->SetVisibility(true);

	SetReplicates(true);
	SetReplicateMovement(false);
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		NetworkInfoWidgetComponent->GetWidget()->AddToViewport();
		UNetInfoWidget* widget = Cast<UNetInfoWidget>(NetworkInfoWidgetComponent->GetWidget());
		if (widget)
		{
			widget->UpdateClientInfo(GetActorLocation());
		}
		GetNetworkEmulationSettings();
	}
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		UNetInfoWidget* widget = Cast<UNetInfoWidget>(NetworkInfoWidgetComponent->GetWidget());
		if (widget)
		{
			widget->UpdateClientInfo(GetActorLocation());
		}

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
			currentMove.playerRotation = GetActorRotation().Yaw;
			currentMove.lookAtRotation = PlayerCamera->GetRelativeRotation().Pitch;
			currentMove.moveID = nextMoveId++;
			ServerMove(currentMove);
			ApplyMovement(currentMove);
			nonAckedMoves.push(currentMove);

			if (widget)
			{
				widget->UpdateSentMoves(currentMove.moveID);
			}
		}
	}
	else if (GetLocalRole() == ROLE_Authority)
	{
		serverUpdateCounter += DeltaTime;
		if (serverUpdateCounter >= 0.1f)
		{
			FServerAck ack;
			if (!serverMovesToApply.empty())
			{
				FPlayerMove lastMove;
				float forwardSpeed = 0;
				float rightSpeed = 0;
				[[maybe_unused]] std::size_t t = serverMovesToApply.size();
				
				while (!serverMovesToApply.empty())
				{
					lastMove = serverMovesToApply.front();
					ApplyMovement(lastMove);
					serverMovesToApply.pop();

					if (lastMove.forwardAxis != 0.f)
					{
						forwardSpeed += lastMove.forwardAxis;
					}
					else if (lastMove.rightAxis)
					{
						rightSpeed += lastMove.rightAxis;
					}
				}

				ack.moveID = lastMove.moveID;
				ack.playerLocation = GetActorLocation();
				ack.playerRotation = lastMove.playerRotation;
				ack.playerForwardSpeed = (forwardSpeed * MovementSpeed) / 0.1f;
				ack.playerRightSpeed = (rightSpeed * MovementSpeed) / 0.1f;
			}
			else
			{
				ack.moveID = 0;
				ack.playerLocation = GetActorLocation();
				ack.playerRotation = GetActorRotation().Yaw;
				ack.playerForwardSpeed = 0.f;
				ack.playerRightSpeed = 0.f;
				
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
}

void APlayerCharacter::GetNetworkEmulationSettings()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());

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
	SetActorRotation(FRotator{ 0.f, move.playerRotation, 0.f });

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
	AddActorLocalRotation(FRotator{0.f, TurnSpeed * Axis, 0.f});
}

void APlayerCharacter::LookUp(float Axis)
{
	PlayerCamera->AddLocalRotation(FRotator{ TurnSpeed * Axis, 0.f, 0.f });
}

void APlayerCharacter::OnRep_PlayerColor()
{
	auto oldMaterial = PlayerMesh->GetMaterial(0);
	
	UMaterialInstanceDynamic* newMaterialInstance = UMaterialInstanceDynamic::Create(oldMaterial, nullptr);
	newMaterialInstance->SetVectorParameterValue(FName("Color"), PlayerColor);

	PlayerMesh->SetMaterial(0, newMaterialInstance);
}

bool APlayerCharacter::ServerMove_Validate(FPlayerMove input)
{
	return true;
}

void APlayerCharacter::ServerMove_Implementation(FPlayerMove input)
{
	serverMovesToApply.push(input);
}

void APlayerCharacter::MulticastReconcileMove_Implementation(FServerAck ack)
{
	auto role = GetLocalRole();
	if (role == ROLE_AutonomousProxy)
	{
		if (ack.playerForwardSpeed != 0.f || ack.playerRightSpeed != 0.f)
		{
			uint32 ackedID = 0;
			while (ack.moveID != ackedID)
			{
				ackedID = nonAckedMoves.front().moveID;
				nonAckedMoves.pop();
			}

			SetActorLocation(ack.playerLocation);
			float cachedLookAt = GetActorRotation().Yaw;

			std::queue<FPlayerMove> tempQueue {nonAckedMoves};
			while (!tempQueue.empty())
			{
				ApplyMovement(tempQueue.front());
				tempQueue.pop();
			}
			SetActorRotation(FRotator{ 0.f, cachedLookAt, 0.f });

			UNetInfoWidget* widget = Cast<UNetInfoWidget>(NetworkInfoWidgetComponent->GetWidget());
			if (widget)
			{
				widget->UpdateServerInfo(ack.playerLocation);
				widget->UpdateAckedMoves(ack.moveID);
			}
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
