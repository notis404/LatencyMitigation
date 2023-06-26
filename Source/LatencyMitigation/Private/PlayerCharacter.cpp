#include "PlayerCharacter.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
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
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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


void APlayerCharacter::MoveForward(float Axis)
{
	FVector ForwardVector = GetActorForwardVector();
	FVector NewLocation = GetActorLocation() + (ForwardVector * MovementSpeed * Axis);
	SetActorLocation(NewLocation);
}

void APlayerCharacter::MoveRight(float Axis)
{
	FVector RightVector = GetActorRightVector();
	FVector NewLocation = GetActorLocation() + (RightVector * MovementSpeed * Axis);
	SetActorLocation(NewLocation);
}

void APlayerCharacter::Turn(float Axis)
{
	AddActorLocalRotation(FRotator{0.f, TurnSpeed * Axis, 0.f});
}

void APlayerCharacter::LookUp(float Axis)
{
	/*GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, FString::Printf(TEXT("Axis: %f"), Axis));*/
	PlayerCamera->AddLocalRotation(FRotator{ TurnSpeed * Axis, 0.f, 0.f });
}
