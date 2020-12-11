// Fill out your copyright notice in the Description page of Project Settings.


#include "RobotPawn.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "EngineUtils.h"
#include "HAL/PlatformFilemanager.h"

DECLARE_LOG_CATEGORY_CLASS(LogRobot, Verbose, Display);

ARobotPawn::ARobotPawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, "SceneComponent");
	RootComponent = SceneComponent;

	CaptureComponent = ObjectInitializer.CreateDefaultSubobject<USceneCaptureComponent2D>(this, TEXT("CaptureComponent"));
	CaptureComponent->SetupAttachment(RootComponent);

	DistancePerMove = 10.f;
	MinDistanceToObstruction = 15.f;
	RotationMinMax = { 5.f, 25.f };
	SecondsBetweenMoves = 1.f;

	CaptureCount = 1;
}

bool ARobotPawn::MoveForward()
{
	auto World = GetWorld();
	if (!World)
	{
		UE_LOG(LogRobot, Error, TEXT("World is null, cancelling MoveForward..."));
		return true; // Should never get here. But if we do, do not trigger a rotation.
	}

	if (World->LineTraceTestByProfile(GetActorLocation(), GetActorLocation() + GetActorForwardVector() * MinDistanceToObstruction, CollisionProfileToUse))
	{
		UE_LOG(LogRobot, Verbose, TEXT("MoveForward detected obstruction..."));
		return false; // Obstruction detected, move unsuccessful
	}

	AddActorWorldOffset(GetActorForwardVector() * DistancePerMove);
	UE_LOG(LogRobot, Verbose, TEXT("MoveForward succeeded..."));

	return true;
}

void ARobotPawn::Rotate()
{
	FRotator deltaRotation(ForceInitToZero);
	deltaRotation.Yaw = FMath::RandRange(RotationMinMax.X, RotationMinMax.Y);
	if (FMath::RandBool())
		deltaRotation.Yaw *= -1.f;

	AddActorWorldRotation(deltaRotation);

	UE_LOG(LogRobot, Verbose, TEXT("Rotated by %.2f degrees"), deltaRotation.Yaw);
}

void ARobotPawn::CaptureAndRecordViewportData()
{	
	auto World = GetWorld();
	if (!World)
	{
		UE_LOG(LogRobot, Error, TEXT("World is null, cancelling CaptureAndRecordViewportData..."));
		return; // Not Likely...
	}
	if (!CaptureComponent->TextureTarget)
	{
		UE_LOG(LogRobot, Error, TEXT("Render Target has not been set on CaptureComponent, cancelling CaptureAndRecordViewportData..."));
		return;
	}
	if (CaptureComponent->TextureTarget->RenderTargetFormat == RTF_RGBA16f)
	{
		UE_LOG(LogRobot, Error, TEXT("Render Target Format must not be set to RGBA16F, cancelling CaptureAndRecordViewportData..."));
		return;
	}
	// NOTE: May want to add a warning for using HDR SceneCapture types...

	const auto CurrentCaptureNum = CaptureCount; // Copy for the async bits

	// Make sure we get a capture ready asap in case we move before recording
	CaptureComponent->CaptureScene();

	Async(EAsyncExecution::TaskGraphMainThread, [=] 
	{
		const FString SaveFolder = FPaths::ProjectSavedDir() / TEXT("Data"); // Can be a property, or better yet a setting in Project Settings...
		// Must be done on the game thread due to UTextureRenderTarget::GameThread_GetRenderTargetResource() call in FImageUtils::ExportRenderTarget2DAsPNG()...
		UKismetRenderingLibrary::ExportRenderTarget(this, CaptureComponent->TextureTarget, SaveFolder, FString::Printf(TEXT("image_%d.png"), CurrentCaptureNum)); // NOTE: RenderTarget used must be set to RGBA8 for a PNG

		const FString ActorsTextFilePath = FString::Printf(TEXT("%s/image_%d_actors.txt"), *SaveFolder, CurrentCaptureNum);
		// We'll be appending each line so we need to delete the old one first
		FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*ActorsTextFilePath);

		for (TActorIterator<AActor> it(World); it; ++it)
		{
			AActor* Actor = *it;
			if (!Actor || Actor == this) // Skip ourself
				continue;

			constexpr float LAST_RENDERED_TOLERENCE = .016f;
			
			// NOTE: We could rely solely on AActor::WasRecentlyRendered() if we can guerantee that the camera will be setup identically to the capture component AND that we only care about visible/rendered Actors. 
			//		 Also, this may not yield accurate results when ejected from the Pawn in PIE
			// Otherwise, a frustum check should be performed. But it would have to be done without utilizing the PlayerController's ViewportClient as we would no longer be able to guarantee that the capture component and the player's view match up
			
			// Assuming WasRecentlyRendered() is sufficient in our current use-case
			if (!Actor->WasRecentlyRendered(LAST_RENDERED_TOLERENCE)) 
				continue; // Not in view

			if (!FFileHelper::SaveStringToFile(Actor->GetName() + TEXT("\n"),
				*ActorsTextFilePath,
				FFileHelper::EEncodingOptions::AutoDetect,
				&IFileManager::Get(),
				FILEWRITE_Append))
			{
				UE_LOG(LogRobot, Error, TEXT("CaptureAndRecordData failed to save %s..."), *ActorsTextFilePath);
			}
		}

		// Notify BP on the main thread
		OnCapturedData();
	});

	CaptureCount++;
}
