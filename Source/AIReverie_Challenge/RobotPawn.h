// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/SceneCaptureComponent2D.h"
#include "RobotPawn.generated.h"

UCLASS()
class AIREVERIE_CHALLENGE_API ARobotPawn : public APawn
{
	GENERATED_BODY()

public:
	ARobotPawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// Moves forward DistanceToMove if no obstructions within MinDistanceToObstruction
	UFUNCTION(BlueprintCallable)
	bool MoveForward();
	// Rotates this Robot by a random number of degrees between +/- RotationMinMax
	UFUNCTION(BlueprintCallable)
	void Rotate();
	// Captures a shot of this robot's view, saves it as a png and saves a text file with a list of all actors in the viewport
	UFUNCTION(BlueprintCallable)
	void CaptureAndRecordViewportData();

	// Called once CaptureAndRecordViewportData() has finished
	UFUNCTION(BlueprintImplementableEvent, Category = "Robot|Capture")
	void OnCapturedData();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot")
	USceneComponent* SceneComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot|Capture")
	USceneCaptureComponent2D* CaptureComponent;

protected:
	// How many units to move each step
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Robot|Movement")
	float DistancePerMove;
	// If an obstruction is detected within this distance, a rotation will be done between +/- RotationMinMax degrees
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Robot|Movement")
	float MinDistanceToObstruction;
	// The name of the collision profile to use during collision checks (line traces)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Robot|Movement")
	FName CollisionProfileToUse;
	// The min {X} and max {Y} degrees to rotate when an obstruction is detected. Rotation will be a random value between +/- [5, 25] degrees
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Robot|Movement")
	FVector2D RotationMinMax;
	// Number of seconds between each move
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Robot|Movement")
	float SecondsBetweenMoves;

private:
	// Track the number of captures made, mainly for file name use
	uint32 CaptureCount;
};
