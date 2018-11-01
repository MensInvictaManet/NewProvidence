#pragma once

#include "TextureManager.h"
#include "XMLWrapper.h"
#include "TimeSlice.h"
#include "Color.h"

#include <vector>
#include <map>
#include <functional>

class TextureAnimation
{
public:
	typedef std::function<void(TextureAnimation*)> TextureAnimationCallback;

private:
	struct TextureAnimationKeyframe
	{
	public:
		TextureAnimationKeyframe(float t, int x, int y, int w, int h, float a, int xOff, int yOff, std::string callback = "") :
			m_Time(t),
			m_Length(0.0f),
			m_X(x),
			m_Y(y),
			m_W(w),
			m_H(h),
			m_Alpha(a),
			m_XOff(xOff),
			m_YOff(yOff),
			m_Callback(callback)
		{}

		float m_Time = 0.0f;
		float m_Length = 0.0f;
		int m_X = 0;
		int m_Y = 0;
		int m_W = 0;
		int m_H = 0;
		float m_Alpha = 0.0f;
		int m_XOff = 0;
		int m_YOff = 0;
		std::string m_Callback = "";
	};

	TextureAnimation() :
		m_Texture(nullptr),
		m_StartTime(gameTicksUint),
		m_Length(0),
		m_CurrentFrame(0)
	{}
	~TextureAnimation() {}

	std::vector<TextureAnimationKeyframe> m_KeyframeList;

	std::map<std::string, TextureAnimationCallback> m_CallbackList;

public:
	static TextureAnimation* CreateTextureAnimation(const char* xmlFilename);

	void AddKeyframeData(float t, int x, int y, int w, int h, float a, int xOff, int yOff, std::string callback);
	void ResetAnimation();
	void Update();
	void Render(int x, int y);
	void AddAnimationCallback(std::string callbackTrigger, TextureAnimationCallback callback) { m_CallbackList[callbackTrigger] = callback; }

	TextureAnimationKeyframe* GetLastKeyframe() { return m_KeyframeList.empty() ? nullptr : &m_KeyframeList.back(); }
	const TextureAnimationKeyframe* GetCurrentFrame() const { return &m_KeyframeList[m_CurrentFrame]; }

	void SetColor(float r, float g, float b, float a) { m_Color.colorValues[0] = r; m_Color.colorValues[1] = g; m_Color.colorValues[2] = b; m_Color.colorValues[3] = a; }

	TextureManager::ManagedTexture* m_Texture;
	Uint32 m_StartTime;
	Uint32 m_Length;
	Color m_Color;
	unsigned int m_CurrentFrame;
};

inline TextureAnimation* TextureAnimation::CreateTextureAnimation(const char* xmlFilename)
{
	auto animXML = xmlWrapper.LoadXMLFile(xmlFilename);
	if (animXML == nullptr) return nullptr;

	const RapidXML_Node* baseNode = animXML->first_node();
	if (baseNode == nullptr || strcmp(baseNode->name(), "TextureAnimation") != 0) return nullptr;

	auto animAttribute = baseNode->first_attribute("length");
	auto length = float(atof(animAttribute->value()));
	animAttribute = animAttribute->next_attribute("file");
	auto texture = textureManager.LoadTexture(animAttribute->value());

	if (texture == nullptr) return nullptr;

	auto anim = new TextureAnimation;
	anim->m_Length = SECONDS_TO_TICKS(length);
	anim->m_Texture = texture;

	for (auto iter = baseNode->first_node(); iter != nullptr; iter = iter->next_sibling())
	{
		auto dataAttribute = iter->first_attribute("t");
		auto t = float(atof(dataAttribute->value()));
		dataAttribute = dataAttribute->next_attribute("x");
		auto x = int(atoi(dataAttribute->value()));
		dataAttribute = dataAttribute->next_attribute("y");
		auto y = int(atoi(dataAttribute->value()));
		dataAttribute = dataAttribute->next_attribute("w");
		auto w = int(atoi(dataAttribute->value()));
		dataAttribute = dataAttribute->next_attribute("h");
		auto h = int(atoi(dataAttribute->value()));
		dataAttribute = dataAttribute->next_attribute("a");
		auto a = float(atof(dataAttribute->value()));
		dataAttribute = dataAttribute->next_attribute("xOff");
		auto xOff = int(atoi(dataAttribute->value()));
		dataAttribute = dataAttribute->next_attribute("yOff");
		auto yOff = int(atoi(dataAttribute->value()));
		dataAttribute = dataAttribute->next_attribute("callback");
		std::string callback = (dataAttribute == nullptr) ? "" : dataAttribute->value();

		auto lastKeyframe = anim->GetLastKeyframe();
		if (lastKeyframe != nullptr) lastKeyframe->m_Length = t - lastKeyframe->m_Time;
		anim->AddKeyframeData(t, x, y, w, h, a, xOff, yOff, callback);
	}

	auto lastKeyframe = anim->GetLastKeyframe();
	if (lastKeyframe != nullptr) lastKeyframe->m_Length = anim->m_Length - lastKeyframe->m_Time;

	anim->m_StartTime = gameTicksUint;

	xmlWrapper.RemoveXMLFile(xmlFilename);
	return anim;
}

inline void TextureAnimation::AddKeyframeData(float t, int x, int y, int w, int h, float a, int xOff, int yOff, std::string callback)
{
	m_KeyframeList.push_back(TextureAnimationKeyframe(t, x, y, w, h, a, xOff, yOff, callback));
}

inline void TextureAnimation::ResetAnimation()
{
	m_StartTime = gameTicksUint;
	m_CurrentFrame = -1;
}

inline void TextureAnimation::Update()
{
	if (m_Texture == nullptr) return;

	auto animTimeTotalUint = (gameTicksUint - m_StartTime);
	auto animTimeUint = (animTimeTotalUint % m_Length);
	float animTime = float(TICKS_TO_SECONDS(animTimeUint));
	auto index = 0;
	for (auto iter = m_KeyframeList.begin(); iter != m_KeyframeList.end(); ++iter, ++index)
	{
		if (animTime < (*iter).m_Time) continue;
		if (animTime > (*iter).m_Time + (*iter).m_Length) continue;

		//  if it's a new frame index, we need to check for callbacks and call them if needed
		if (m_CurrentFrame != index)
		{
			//  We always call the "Finished" callback when we go over the time of the animation and are about to restart
			if (index == 0 && (animTimeTotalUint > animTimeUint) && (m_CallbackList.find("Finished") != m_CallbackList.end())) m_CallbackList["Finished"](this);

			//  If any other callback exists for this frame, call it as well
			if ((*iter).m_Callback.compare("") != 0 && (m_CallbackList.find((*iter).m_Callback) != m_CallbackList.end())) m_CallbackList[(*iter).m_Callback](this);

			//  Set the new frame index
			m_CurrentFrame = index;
		}
	}
}

inline void TextureAnimation::Render(int x, int y)
{
	if (m_Texture == nullptr) return;

	glColor4f(m_Color.colorValues[0], m_Color.colorValues[1], m_Color.colorValues[2], m_Color.colorValues[3]);
	m_Texture->RenderTexturePart(x + m_KeyframeList[m_CurrentFrame].m_XOff, y + m_KeyframeList[m_CurrentFrame].m_YOff, m_KeyframeList[m_CurrentFrame].m_X, m_KeyframeList[m_CurrentFrame].m_Y, m_KeyframeList[m_CurrentFrame].m_W, m_KeyframeList[m_CurrentFrame].m_H);
}