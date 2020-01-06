// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ScriptEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr< FSlateStyleSet > FScriptEditorStyle::StyleSet = nullptr;

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

// Const icon sizes
static const FVector2D Icon8x8(8.0f, 8.0f);
static const FVector2D Icon9x19(9.0f, 19.0f);
static const FVector2D Icon16x16(16.0f, 16.0f);
static const FVector2D Icon18x18(18.0f, 18.0f);
static const FVector2D Icon20x20(20.0f, 20.0f);
static const FVector2D Icon22x22(22.0f, 22.0f);
static const FVector2D Icon24x24(24.0f, 24.0f);
static const FVector2D Icon28x28(28.0f, 28.0f);
static const FVector2D Icon27x31(27.0f, 31.0f);
static const FVector2D Icon26x26(26.0f, 26.0f);
static const FVector2D Icon32x32(32.0f, 32.0f);
static const FVector2D Icon40x40(40.0f, 40.0f);
static const FVector2D Icon48x48(48.0f, 48.0f);
static const FVector2D Icon75x82(75.0f, 82.0f);
static const FVector2D Icon360x32(360.0f, 32.0f);
static const FVector2D Icon171x39(171.0f, 39.0f);
static const FVector2D Icon170x50(170.0f, 50.0f);
static const FVector2D Icon267x140(170.0f, 50.0f);

void FScriptEditorStyle::Initialize()
{
	// Only register once
	if( StyleSet.IsValid() )
	{
		return;
	}

	StyleSet = MakeShareable(new FSlateStyleSet("ScriptEditorStyle") );

	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin("UScriptEditor")->GetBaseDir() / TEXT("Resources"));

	// Icons
	{
 		StyleSet->Set("ScriptEditor.OpenPluginWindow", new IMAGE_BRUSH("UI/Common/ButtonIcon_40x", Icon16x16));
		StyleSet->Set("ScriptEditor.TabIcon", new IMAGE_BRUSH("UI/Common/ButtonIcon_40x", Icon40x40));
	
		StyleSet->Set("ScriptEditor.Reload", new IMAGE_BRUSH("UI/Common/icon_reload_40x", Icon40x40));
		StyleSet->Set("ScriptEditor.ReloadAll", new IMAGE_BRUSH("UI/Common/icon_reload_40x", Icon40x40));

		StyleSet->Set("ScriptEditor.Save", new IMAGE_BRUSH("UI/Common/Save_40x", Icon40x40));
		StyleSet->Set("ScriptEditor.Save.Small", new IMAGE_BRUSH("UI/Common/Save_40x", Icon16x16));
		StyleSet->Set("ScriptEditor.SaveAll", new IMAGE_BRUSH("UI/Common/SaveAll_40x", Icon40x40));
		StyleSet->Set("ScriptEditor.SaveAll.Small", new IMAGE_BRUSH("UI/Common/SaveAll_40x", Icon16x16));

		StyleSet->Set("ScriptEditor.Backward", new IMAGE_BRUSH("UI/Common/icon_nav_back_32x", Icon32x32));
		StyleSet->Set("ScriptEditor.Forward", new IMAGE_BRUSH("UI/Common/icon_nav_fwd_32x", Icon32x32));

		StyleSet->Set("ScriptEditor.DebugContinue", new IMAGE_BRUSH("UI/Debug/icon_debug_continue", Icon40x40));
		StyleSet->Set("ScriptEditor.DebugStepover", new IMAGE_BRUSH("UI/Debug/icon_debug_step_over", Icon40x40));
		StyleSet->Set("ScriptEditor.DebugStepin", new IMAGE_BRUSH("UI/Debug/icon_debug_step_into", Icon40x40));
		StyleSet->Set("ScriptEditor.DebugStepout", new IMAGE_BRUSH("UI/Debug/icon_debug_step_out", Icon40x40));
		StyleSet->Set("ScriptEditor.DebugAbort", new IMAGE_BRUSH("UI/Debug/icon_debug_step_out", Icon40x40));		//Abort

		//Tab Icons
		StyleSet->Set("ScriptEditor.Project", new IMAGE_BRUSH("UI/Widgets/icon_tab_members_16x", Icon16x16));
		StyleSet->Set("ScriptEditor.APIBroswer", new IMAGE_BRUSH("UI/Widgets/icon_tab_editor_16x", Icon16x16));
		StyleSet->Set("ScriptEditor.Debugger", new IMAGE_BRUSH("UI/Widgets/icon_tab_breakpoints_16x", Icon16x16));
		StyleSet->Set("ScriptEditor.Log", new IMAGE_BRUSH("UI/Widgets/icon_tab_log_16x", Icon16x16));
		StyleSet->Set("ScriptEditor.VarWatcher", new IMAGE_BRUSH("UI/Widgets/icon_tab_preview_16x", Icon16x16));
	}

	const FSlateFontInfo Consolas10  = DEFAULT_FONT("Mono", 11);

	const FTextBlockStyle NormalText = FTextBlockStyle()
		.SetFont(Consolas10)
		.SetColorAndOpacity(FLinearColor::White)
		.SetShadowOffset(FVector2D::ZeroVector)
		.SetShadowColorAndOpacity(FLinearColor::Black)
		.SetHighlightColor(FLinearColor(0.02f, 0.3f, 0.0f))
		.SetHighlightShape(BOX_BRUSH("UI/Widgets/TextBlockHighlightShape", FMargin(3.f / 8.f)));

	// Text editor
	{
		StyleSet->Set("TextEditor.NormalText", NormalText);

		StyleSet->Set("SyntaxHighlight.CPP.Normal", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xffdfd706))));// yellow
		StyleSet->Set("SyntaxHighlight.CPP.Operator", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xffcfcfcf)))); // light grey
		StyleSet->Set("SyntaxHighlight.CPP.Keyword", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xff006ab4)))); // blue
		StyleSet->Set("SyntaxHighlight.CPP.String", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xff9e4a1e)))); // pinkish
		StyleSet->Set("SyntaxHighlight.CPP.Number", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xff6db3a8)))); // cyan
		StyleSet->Set("SyntaxHighlight.CPP.Comment", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xff57a64a)))); // green
		StyleSet->Set("SyntaxHighlight.CPP.PreProcessorKeyword", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xffcfcfcf)))); // light grey

		StyleSet->Set("SyntaxHighlight.LUA.Normal", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xffdfd706))));// yellow
		StyleSet->Set("SyntaxHighlight.LUA.Operator", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xffcfcfcf)))); // light grey
		StyleSet->Set("SyntaxHighlight.LUA.Keyword", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xff006ab4)))); // blue
		StyleSet->Set("SyntaxHighlight.LUA.String", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xff9e4a1e)))); // pinkish
		StyleSet->Set("SyntaxHighlight.LUA.Number", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xff6db3a8)))); // cyan
		StyleSet->Set("SyntaxHighlight.LUA.Comment", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xff57a64a)))); // green
		StyleSet->Set("SyntaxHighlight.LUA.PreProcessorKeyword", FTextBlockStyle(NormalText).SetColorAndOpacity(FLinearColor(FColor(0xffcfcfcf)))); // light grey

		StyleSet->Set("TextEditor.Border", new BOX_BRUSH("UI/Widgets/TextEditorBorder", FMargin(4.0f/16.0f), FLinearColor(0.02f,0.02f,0.02f,1)));

		const FEditableTextBoxStyle EditableTextBoxStyle = FEditableTextBoxStyle()
			.SetBackgroundImageNormal( FSlateNoResource() )
			.SetBackgroundImageHovered( FSlateNoResource() )
			.SetBackgroundImageFocused( FSlateNoResource() )
			.SetBackgroundImageReadOnly( FSlateNoResource() );
		
		StyleSet->Set("TextEditor.EditableTextBox", EditableTextBoxStyle);

		StyleSet->Set("Breakpoint.On", new IMAGE_BRUSH("UI/Debug/icon_breakpoint_on", Icon16x16));
		StyleSet->Set("Breakpoint.Off", new IMAGE_BRUSH("UI/Debug/icon_breakpoint_off", Icon16x16));
		StyleSet->Set("Breakpoint.Hit", new IMAGE_BRUSH("UI/Debug/icon_breakpoint_hit", Icon16x16));
		StyleSet->Set("Breakpoint.Null", new IMAGE_BRUSH("UI/Debug/icon_breakpoint_null", Icon16x16));
		StyleSet->Set("Breakpoint.DisableAll", new IMAGE_BRUSH("UI/Debug/icon_breakpoint_disable_all", Icon16x16));
		StyleSet->Set("Breakpoint.Enable", new IMAGE_BRUSH("UI/Debug/icon_breakpoint_enable", Icon16x16));
		StyleSet->Set("Breakpoint.Remove", new IMAGE_BRUSH("UI/Debug/icon_breakpoint_remove_all", Icon16x16));

		StyleSet->Set("Callsatck.Next", new IMAGE_BRUSH("UI/Debug/icon_callsatck_next_statement", Icon16x16));
		StyleSet->Set("Callsatck.Selected", new IMAGE_BRUSH("UI/Debug/icon_callstack_selected", Icon16x16));
	}

	// Project editor
	{
		StyleSet->Set("ProjectEditor.Border", new BOX_BRUSH("UI/Widgets/TextEditorBorder", FMargin(4.0f/16.0f), FLinearColor(0.048f,0.048f,0.048f,1)));

		StyleSet->Set("ProjectEditor.Icon.Project", new IMAGE_BRUSH("UI/Widgets/FolderClosed", Icon18x18, FLinearColor(0.25f,0.25f,0.25f,1)));
		StyleSet->Set("ProjectEditor.Icon.Folder", new IMAGE_BRUSH("UI/Widgets/FolderClosed", Icon18x18, FLinearColor(0.25f,0.25f,0.25f,1)));
		StyleSet->Set("ProjectEditor.Icon.File", new IMAGE_BRUSH("UI/Widgets/GenericFile_40x", Icon18x18));
		StyleSet->Set("ProjectEditor.Icon.GenericFile", new IMAGE_BRUSH("UI/Widgets/GenericFile_40x", Icon18x18));
		StyleSet->Set("ProjectEditor.Icon.lua", new IMAGE_BRUSH("UI/Widgets/icon_lua_40x", Icon18x18));
	}
	// Asset file
	{
		StyleSet->Set(FName(TEXT("ClassThumbnail.LuaScript")), new IMAGE_BRUSH("UI/Widgets/icon_lua_40x", Icon40x40));
		StyleSet->Set(FName(TEXT("ClassIcon.LuaScript")), new IMAGE_BRUSH("UI/Widgets/icon_lua_40x", Icon18x18));
	}

	FSlateStyleRegistry::RegisterSlateStyle( *StyleSet.Get() );
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef DEFAULT_FONT

void FScriptEditorStyle::Shutdown()
{
	if( StyleSet.IsValid() )
	{
		FSlateStyleRegistry::UnRegisterSlateStyle( *StyleSet.Get() );
		ensure( StyleSet.IsUnique() );
		StyleSet.Reset();
	}
}

void FScriptEditorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FScriptEditorStyle::Get()
{
	return *( StyleSet.Get() );
}

FName FScriptEditorStyle::GetStyleSetName()
{
	return StyleSet->GetStyleSetName();
}
