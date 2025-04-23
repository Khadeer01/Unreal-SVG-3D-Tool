#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "SVGElements.h"
#include "XmlFile.h"


class ToolUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(ToolUI) {}
	SLATE_END_ARGS()
	
	void Construct(const FArguments& args);

private:
	// Button handlers.
	FReply OnGenerateButtonClicked();
	FReply OnBrowseButtonClicked();
	FReply OnConvertSVGButtonClicked();
	FReply OnExtractSVGButtonClicked();
	FReply OnParseCustomSVGButtonClicked();

	TSharedPtr<STextBlock> ErrorTextBlock;
	

	void ProcessSVGData(const FString& SVGData);
	void ProcessSVGNode(FXmlNode* Node);

	void Trinangulation(FSVGElements& Elements);
	// UI elements.
	TSharedPtr<class SEditableTextBox> FilePathTextBox;
	TSharedPtr<class SEditableTextBox> ExtractedSVGTextBox;
	TSharedPtr<STextBlock> StatusTextBox;

	// Currently selected SVG file path.
	FString CurrentFilePath;

	//array to store the svg data
	TArray<FSVGElements> ParsedSVGElements;

	float ExtrusionDepth;
	TSharedPtr<SEditableTextBox> ExtrusionDepthTextBox;

	// Slider functions
	FText GetExtrusionDepthText() const;
	void OnExtrusionDepthTextCommitted(const FText& InText, ETextCommit::Type CommitInfo);
	float GetExtrusionDepthSliderValue() const;
	void OnExtrusionDepthSliderChanged(float NewValue);
};
