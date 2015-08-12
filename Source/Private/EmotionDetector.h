// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Core.h"
#include "UnrealTournament.h"

#include "pxcsession.h"
#include "pxcsensemanager.h"
#include "pxcemotion.h"

#include "EmotionDetector.generated.h"

// Can help to have a UObject around if we want one
UCLASS(Blueprintable, Meta = (ChildCanTick))
class AEmotionDetector : public AActor
{
	GENERATED_UCLASS_BODY()

};

class SenseHandler : public PXCSenseManager::Handler
{
public:

	SenseHandler(int pidx) : m_pidx(pidx)
	{
	}

	virtual pxcStatus PXCAPI OnModuleQueryProfile(pxcUID, PXCBase*, pxcI32 pidx)
	{
		return pidx == m_pidx ? PXC_STATUS_NO_ERROR : PXC_STATUS_PARAM_UNSUPPORTED;
	}

protected:
	pxcI32 m_pidx;
};

struct FEmotionDetector : FTickableGameObject, FSelfRegisteringExec, IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	FEmotionDetector();
	virtual void Tick(float DeltaTime);
	virtual bool IsTickable() const { return true; }
	virtual bool IsTickableInEditor() const { return true; }
	virtual bool IsTickableWhenPaused() const { return true; }

	// Put a real stat id here
	virtual TStatId GetStatId() const
	{
		return TStatId();
	}

	/** FSelfRegisteringExec implementation */
	virtual bool Exec(UWorld* Inworld, const TCHAR* Cmd, FOutputDevice& Ar) override;

	void OnWorldCreated(UWorld* World, const UWorld::InitializationValues IVS);
	void OnWorldDestroyed(UWorld* World);

	SenseHandler Handler;
	PXCSession* pPXCSession;
	PXCSenseManager* pSenseManager;
	PXCCaptureManager* CaptureMgr;
	int32 MainEmotionIndex;
	int32 MainSentimentIndex;

	void InitEmotionCapture();
	void ShutdownEmotionCapture();

	FString GetEmotion();
};