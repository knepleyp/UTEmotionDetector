// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EmotionDetector.h"


IMPLEMENT_MODULE(FEmotionDetector, EmotionDetector)

DEFINE_LOG_CATEGORY_STATIC(LogUTEmotionDetector, Log, All);

const int NUM_EMOTIONS = 10;
const int NUM_PRIMARY_EMOTIONS = 7;
const float MinimumEmotionIntensity = 0.4f;

static WCHAR *EmotionLabels[] = 
{
	L"ANGER",
	L"CONTEMPT",
	L"DISGUST",
	L"FEAR",
	L"JOY",
	L"SADNESS",
	L"SURPRISE"
};
static WCHAR *SentimentLabels[] = 
{
	L"NEGATIVE",
	L"POSITIVE",
	L"NEUTRAL"
};

AEmotionDetector::AEmotionDetector(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

FEmotionDetector::FEmotionDetector()
	: Handler(0)
{
	MainEmotionIndex = -1;
	MainSentimentIndex = -1;
}

FString FEmotionDetector::GetEmotion()
{
	if (MainEmotionIndex == -1)
	{
		return TEXT("UNDEFINED");
	}

	return EmotionLabels[MainEmotionIndex];
}

void FEmotionDetector::InitEmotionCapture()
{
	pPXCSession = PXCSession::CreateInstance();
	pSenseManager = pPXCSession->CreateSenseManager();

	CaptureMgr = pSenseManager->QueryCaptureManager();
	pxcCHAR* Device = NULL;

	PXCSession::ImplDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.group = PXCSession::IMPL_GROUP_SENSOR;
	desc.subgroup = PXCSession::IMPL_SUBGROUP_VIDEO_CAPTURE;

	for (int i = 0;; i++)
	{
		PXCSession::ImplDesc desc1;
		if (pPXCSession->QueryImpl(&desc, i, &desc1) == PXC_STATUS_NO_ERROR)
		{
			PXCCapture *capture;
			if (pPXCSession->CreateImpl<PXCCapture>(&desc1, &capture) == PXC_STATUS_NO_ERROR)
			{
				for (int j = 0;; j++)
				{
					PXCCapture::DeviceInfo dinfo;
					if (capture->QueryDeviceInfo(j, &dinfo) == PXC_STATUS_NO_ERROR)
					{
						Device = dinfo.name;
						break;
					}
				}
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	if (Device != nullptr)
	{
		CaptureMgr->FilterByDeviceInfo(Device, 0, 0);

		pSenseManager->EnableEmotion();
		pSenseManager->Init(&Handler);
	}
}

void FEmotionDetector::ShutdownEmotionCapture()
{
	pSenseManager->Close();
	pSenseManager->Release();

}

void FEmotionDetector::StartupModule()
{
	FWorldDelegates::FWorldInitializationEvent::FDelegate OnWorldCreatedDelegate = FWorldDelegates::FWorldInitializationEvent::FDelegate::CreateRaw(this, &FEmotionDetector::OnWorldCreated);
	FDelegateHandle OnWorldCreatedDelegateHandle = FWorldDelegates::OnPostWorldInitialization.Add(OnWorldCreatedDelegate);
	
	InitEmotionCapture();
}

void FEmotionDetector::ShutdownModule()
{
	FWorldDelegates::FWorldEvent::FDelegate OnWorldDestroyedDelegate = FWorldDelegates::FWorldEvent::FDelegate::CreateRaw(this, &FEmotionDetector::OnWorldDestroyed);
	FDelegateHandle OnWorldDestroyedDelegateHandle = FWorldDelegates::OnPreWorldFinishDestroy.Add(OnWorldDestroyedDelegate);
	
	ShutdownEmotionCapture();
}

void FEmotionDetector::OnWorldCreated(UWorld* World, const UWorld::InitializationValues IVS)
{
	if (IsRunningCommandlet() || IsRunningDedicatedServer())
	{
		return;
	}
}

void FEmotionDetector::OnWorldDestroyed(UWorld* World)
{
	if (IsRunningCommandlet() || IsRunningDedicatedServer())
	{
		return;
	}
}

bool FEmotionDetector::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	return false;
}

void FEmotionDetector::Tick(float DeltaTime)
{
	if (GIsEditor)
	{
		return;
	}

	MainEmotionIndex = -1;
	MainSentimentIndex = -1;

	pxcStatus FrameStatus = pSenseManager->AcquireFrame(false, 0);
	if (FrameStatus == PXC_STATUS_DEVICE_FAILED)
	{
		UE_LOG(LogUTEmotionDetector, Verbose, TEXT("Device failed, restarting"));
		ShutdownEmotionCapture();
		InitEmotionCapture();
		return;
	}

	if (FrameStatus != PXC_STATUS_NO_ERROR)
	{
		return;
	}

	pxcF32 MaxScoreIntensity = 0;
	pxcI32 maxscoreE = -3;
	PXCEmotion *emotionDet = pSenseManager->QueryEmotion();
	if (emotionDet != NULL)
	{
		const PXCCapture::Sample *sample = pSenseManager->QueryEmotionSample();
		int32 numFaces = emotionDet->QueryNumFaces();
		PXCEmotion::EmotionData arrData[NUM_EMOTIONS];
		for (int FaceIndex = 0; FaceIndex < numFaces; FaceIndex++)
		{
			emotionDet->QueryAllEmotionData(FaceIndex, &arrData[0]);
			for (int j = 0; j < NUM_PRIMARY_EMOTIONS; j++)
			{
				if (arrData[j].evidence < maxscoreE)  continue;
				if (arrData[j].intensity < MaxScoreIntensity) continue;
				maxscoreE = arrData[j].evidence;
				MaxScoreIntensity = arrData[j].intensity;
				MainEmotionIndex = j;
			}

			if (MaxScoreIntensity < MinimumEmotionIntensity)
			{
				MainEmotionIndex = -1;
				continue;
			}

			maxscoreE = -3; 
			MaxScoreIntensity = 0;
			for (int k = 0; k < (NUM_EMOTIONS - NUM_PRIMARY_EMOTIONS); k++)
			{
				if (arrData[NUM_PRIMARY_EMOTIONS + k].evidence < maxscoreE) continue;
				if (arrData[NUM_PRIMARY_EMOTIONS + k].intensity < MaxScoreIntensity) continue;
				maxscoreE = arrData[NUM_PRIMARY_EMOTIONS + k].evidence;
				MaxScoreIntensity = arrData[NUM_PRIMARY_EMOTIONS + k].intensity;
				MainSentimentIndex = k;
			}
		}
	}
	pSenseManager->ReleaseFrame();

	if (MainEmotionIndex != -1)
	{
		UE_LOG(LogUTEmotionDetector, Verbose, TEXT("%s"), EmotionLabels[MainEmotionIndex]);
	}

	if (MainSentimentIndex != -1)
	{
		UE_LOG(LogUTEmotionDetector, Verbose, TEXT("%s"), SentimentLabels[MainSentimentIndex]);
	}
}
