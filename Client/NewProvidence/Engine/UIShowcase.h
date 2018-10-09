#pragma once

#include "Engine/GUIObjectNode.h"
#include "Engine/GUIListBox.h"
#include "Engine/GUIMoveable.h"
#include "Engine/GUILabel.h"
#include "Engine/GUICheckbox.h"
#include "Engine/GUIButton.h"
#include "Engine/GUIEditBox.h"
#include "Engine/TextureAnimation.h"

TextureAnimation* animationTest = nullptr;

class UIShowcaseDialogue : public GUIObjectNode
{
public:
	UIShowcaseDialogue();
	~UIShowcaseDialogue() {}
};

inline UIShowcaseDialogue::UIShowcaseDialogue()
{
	//  Create the label that acts as an explanation of the current showcase UI
	auto introductionLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "This is a basic showcase of the GUI Manager system.", 10, 10, 300, 32);
	AddChild(introductionLabel);

	//  Create the container that holds all of the main objects in the scene
	auto container1 = GUIMoveable::CreateTemplatedMoveable("Standard", 10, 40, 600, 420, 0, 0, 600, 16);
	AddChild(container1);

	//  Create the label that acts as an explanation of the container / moveable
	auto containerExplanationLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "This is a basic container. Click and drag it with the top bar.", 10, 24, 600, 22);
	container1->AddChild(containerExplanationLabel);

	//  Create the label that our checkbox and button will manipulate
	auto callbackLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "This text appears and disappears with a button callback.", 10, 100, 300, 32);
	container1->AddChild(callbackLabel);

	//  Create the label that acts as an explanation of the listbox
	auto listboxLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "This is a basic listbox that you can throw GUI object into.", 10, 150, 300, 32);
	container1->AddChild(listboxLabel);

	//  Create the listbox that shows actions taken in the UI
	auto listbox1 = GUIListBox::CreateTemplatedListBox("Standard", 10, 170, 580, 240, 560, 4, 16, 16, 16, 16, 16, 22, 2);
	listbox1->SetFlowToBottom(true);
	container1->AddChild(listbox1);

	//  Create the checkbox that allows us to disable the button's ability to alter the label
	auto checkbox1 = GUICheckbox::CreateTemplatedCheckbox("Standard", 140, 60, 20, 20);
	checkbox1->SetCheckCallback([=](GUIObjectNode*)
	{
		listbox1->AddItem(GUILabel::CreateLabel(fontManager.GetFont("Arial"), checkbox1->GetChecked() ? "Checkbox CHECKED" : "Checkbox UNCHECKED", 10, 6, 300, 22));

		callbackLabel->SetText("Checkbox callbacks work, and this one locks the text to visible.");
		callbackLabel->SetVisible(checkbox1->GetChecked());
	});
	container1->AddChild(checkbox1);

	//  Create the button that allows us to alter a label's visibility and text
	auto button1 = GUIButton::CreateTemplatedButton("Standard", 10, 50, 120, 40);
	button1->SetColor(0.5f, 0.5f, 1.0f, 1.0f);
	button1->SetFont(fontManager.GetFont("Arial"));
	button1->SetText("Click Me");
	button1->SetLeftClickCallback([=](GUIObjectNode*)
	{
		listbox1->AddItem(GUILabel::CreateLabel(fontManager.GetFont("Arial"), "Button clicked (left)", 10, 6, 300, 22));

		if (checkbox1->GetChecked()) return;
		callbackLabel->SetText("This text appears and disappears with a callback.");
		callbackLabel->SetVisible(!callbackLabel->GetVisible());
	});
	button1->SetMiddleClickCallback([=](GUIObjectNode*)
	{
		listbox1->AddItem(GUILabel::CreateLabel(fontManager.GetFont("Arial"), "Button clicked (middle)", 10, 6, 300, 22));

		if (checkbox1->GetChecked()) return;
		callbackLabel->SetText("Middle click callbacks also exist (and right!).");
		callbackLabel->SetVisible(!callbackLabel->GetVisible());
	});
	button1->SetRightClickCallback([=](GUIObjectNode*)
	{
		listbox1->AddItem(GUILabel::CreateLabel(fontManager.GetFont("Arial"), "Button clicked (right)", 10, 6, 300, 22));

		if (checkbox1->GetChecked()) return;
		callbackLabel->SetText("Right click callbacks also exist (and middle!).");
		callbackLabel->SetVisible(!callbackLabel->GetVisible());
	});
	container1->AddChild(button1);

	//  Create the edit box for submitting strings to the listbox
	auto editbox1 = GUIEditBox::CreateTemplatedEditBox("Standard", 260, 58, 200, 24);
	editbox1->SetObjectName("UIShowcaseEditbox");
	editbox1->SetFont(fontManager.GetFont("Arial"));
	container1->AddChild(editbox1);

	//  Create the button that submits the contents of the edit box to the listbox
	auto button2 = GUIButton::CreateTemplatedButton("Standard", 470, 50, 120, 40);
	button2->SetObjectName("UIShowcaseSubmitButton");
	button2->SetFont(fontManager.GetFont("Arial"));
	button2->SetText("Submit");
	button2->SetLeftClickCallback([=](GUIObjectNode*)
	{
		if (editbox1->GetText().length() == 0) return;
		listbox1->AddItem(GUILabel::CreateLabel(fontManager.GetFont("Arial"), (std::string("Editbox text: ") + editbox1->GetText()).c_str(), 10, 6, 300, 22));
		editbox1->SetText("");
	});
	container1->AddChild(button2);

	//  NOTE: Push TextureAnimation into a zelda-like example showcase
	/*animationTest = TextureAnimation::CreateTextureAnimation("Assets/Sprites/IdleAnim.xml");
	animationTest->AddAnimationCallback("Start", [=](TextureAnimation*)
	{
		static float alpha = 1.0f;
		alpha -= 0.22f;
		if (alpha <= 0.0f) alpha = 1.0f;

		animationTest->SetColor(1.0f, 1.0f, 1.0f, alpha);
	});
	auto animNode = GUIObjectNode::CreateObjectNode(animationTest);
	animNode->SetWidth(30);
	animNode->SetHeight(60);
	container1->AddChild(animNode);*/
}