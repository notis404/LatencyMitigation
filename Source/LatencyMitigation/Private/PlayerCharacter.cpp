#include "PlayerCharacter.h"

// Sets default values
APlayerCharacter::APlayerCharacter() :
	savedMoves{}
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
	NetworkInfoWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	NetworkInfoWidgetComponent->SetVisibility(true);

	SetReplicates(true);
	SetReplicateMovement(true);
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	NetworkInfoWidgetComponent->GetWidget()->AddToViewport();
	UNetInfoWidget* widget = Cast<UNetInfoWidget>(NetworkInfoWidgetComponent->GetWidget());
	if (widget)
	{
		widget->UpdateClientInfo(GetActorLocation());
	}
	GetNetworkEmulationSettings();
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
			savedMoves.push(currentMove);

			if (widget)
			{
				widget->UpdateSentMoves(currentMove.moveID);
			}
		}
		//// Get the player controller
		//APlayerController* PlayerController = Cast<APlayerController>(GetController());


		//// Get the camera location and rotation
		//FVector CameraLocation;
		//FRotator CameraRotation;
		//PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

		//// Calculate the text location in front of the camera
		//FVector TextLocation = CameraLocation + CameraRotation.Vector() * 300.0f; // Adjust the distance as needed

		//// Convert the location to a string
		//FString LocationString = GetActorLocation().ToString();

		//// Draw the debug string in front of the camera
		//DrawDebugString(GetWorld(), TextLocation, TEXT("Client Position: ") + LocationString, nullptr, FColor::Green, 0.0f, true);
	}
	else if (GetLocalRole() == ROLE_Authority)
	{
		//// Get the player controller
		//APlayerController* PlayerController = Cast<APlayerController>(GetController());


		//// Get the camera location and rotation
		//FVector CameraLocation;
		//FRotator CameraRotation;
		//PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

		//// Calculate the text location in front of the camera
		//FVector TextLocation = CameraLocation + CameraRotation.Vector() * 300.0f; // Adjust the distance as needed

		//// Convert the location to a string
		//FString LocationString = GetActorLocation().ToString();

		//// Draw the debug string in front of the camera
		//DrawDebugString(GetWorld(), TextLocation, TEXT("Server Position: ") + LocationString, nullptr, FColor::Red, 0.0f, true);
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

bool APlayerCharacter::ServerMove_Validate(FPlayerMove input)
{
	return true;
}

void APlayerCharacter::ServerMove_Implementation(FPlayerMove input)
{
	ApplyMovement(input);

	FServerAck ack;
	ack.moveID = input.moveID;
	ack.playerLocation = GetActorLocation();
	ReconcileMove(ack);
}

void APlayerCharacter::ReconcileMove_Implementation(FServerAck ack)
{
	uint32 ackedID = 0;
	while (ack.moveID != ackedID)
	{
		ackedID = savedMoves.front().moveID;
		savedMoves.pop();
	}

	SetActorLocation(ack.playerLocation);
	float cachedLookAt = GetActorRotation().Yaw;

	std::queue<FPlayerMove> tempQueue {savedMoves};	
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
