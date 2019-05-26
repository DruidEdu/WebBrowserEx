// Fill out your copyright notice in the Description page of Project Settings.

#include "MyWebBrowser.h"

#include <SWidget.h>
#include <SBox.h>
#include <STextBlock.h>
#include <WebBrowser/Public/SWebBrowser.h>
#include <ConstructorHelpers.h>
#include <TaskGraphInterfaces.h>

#include <ITextInputMethodSystem.h>
#include <SlateApplication.h>
#if WITH_EDITOR

#include "Materials/MaterialInterface.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialFunction.h"
#include "Materials/Material.h"
#include "Factories/MaterialFactoryNew.h"
#include "AssetRegistryModule.h"
#include "PackageHelperFunctions.h"
#endif


#if WITH_EDITOR || PLATFORM_ANDROID
#include <WebBrowserTexture/Public/WebBrowserTexture.h>
#endif




#define LOCTEXT_NAMESPACE "WebBrowser"

/////////////////////////////////////////////////////
// UMyWebBrowser

UMyWebBrowser::UMyWebBrowser(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsVariable = true;

#if WITH_EDITOR || PLATFORM_ANDROID
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinder<UObject>			DefaultTextureMaterial;
		FConstructorStatics()
			: DefaultTextureMaterial(TEXT("/WebBrowserEx/WebTexture_M"))
		{}
	};
	static FConstructorStatics ConstructorStatics;

	// Add a hard reference to UMyWebBrowserTexture, without this the WebBrowserTexture DLL never gets loaded on Windows.
	UWebBrowserTexture::StaticClass();

	DefaultMaterial = (UMaterial*)ConstructorStatics.DefaultTextureMaterial.Object;
#endif
}

void UMyWebBrowser::LoadURL(FString NewURL)
{

	if (WebBrowserWidget.IsValid())
	{		
		
		return WebBrowserWidget->LoadURL(NewURL);
	}
}

void UMyWebBrowser::LoadString(FString Contents, FString DummyURL)
{
	if (WebBrowserWidget.IsValid())
	{
		return WebBrowserWidget->LoadString(Contents, DummyURL);
	}
}

void UMyWebBrowser::ExecuteJavascript(const FString& ScriptText)
{
	if (WebBrowserWidget.IsValid())
	{
		return WebBrowserWidget->ExecuteJavascript(ScriptText);
	}
}

FText UMyWebBrowser::GetTitleText() const
{
	if (WebBrowserWidget.IsValid())
	{
		return WebBrowserWidget->GetTitleText();
	}

	return FText::GetEmpty();
}

FString UMyWebBrowser::GetUrl() const
{
	if (WebBrowserWidget.IsValid())
	{
		return WebBrowserWidget->GetUrl();
	}

	return FString();
}

void UMyWebBrowser::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	WebBrowserWidget.Reset();
}

TSharedRef<SWidget> UMyWebBrowser::RebuildWidget()
{
	if (IsDesignTime())
	{
		return SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Web Browser", "Web Browser"))
			];
	}
	else
	{
		WebBrowserWidget = SNew(SWebBrowser)
			.InitialURL(InitialURL)
			.ShowControls(false)
			.SupportsTransparency(bSupportsTransparency)
			.OnUrlChanged(BIND_UOBJECT_DELEGATE(FOnTextChanged, HandleOnUrlChanged))
			.OnBeforePopup(BIND_UOBJECT_DELEGATE(FOnBeforePopupDelegate, HandleOnBeforePopup));

		if (WebBrowserWidget)
		{
			ITextInputMethodSystem* const TextInputMethodSystem = FSlateApplication::Get().GetTextInputMethodSystem();
			WebBrowserWidget->BindInputMethodSystem(TextInputMethodSystem);
		}
		


		return WebBrowserWidget.ToSharedRef();
	}
}

void UMyWebBrowser::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (WebBrowserWidget.IsValid())
	{

	}
}

void UMyWebBrowser::HandleOnUrlChanged(const FText& InText)
{
	OnUrlChanged.Broadcast(InText);
}

bool UMyWebBrowser::HandleOnBeforePopup(FString URL, FString Frame)
{
	if (OnBeforePopup.IsBound())
	{
		if (IsInGameThread())
		{
			OnBeforePopup.Broadcast(URL, Frame);
		}
		else
		{
			// Retry on the GameThread.
			TWeakObjectPtr<UMyWebBrowser> WeakThis = this;
			FFunctionGraphTask::CreateAndDispatchWhenReady([WeakThis, URL, Frame]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->HandleOnBeforePopup(URL, Frame);
				}
			}, TStatId(), nullptr, ENamedThreads::GameThread);
		}

		return true;
	}

	return false;
}

#if WITH_EDITOR

const FText UMyWebBrowser::GetPaletteCategory()
{
	return LOCTEXT("Experimental", "Experimental");
}

#endif

UMaterial* UMyWebBrowser::GetDefaultMaterial() const
{
	return DefaultMaterial;
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE


