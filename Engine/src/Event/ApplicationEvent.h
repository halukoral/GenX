#pragma once
#include "Event.h"

class WindowResizeEvent : public Event
{
public:
	WindowResizeEvent(const uint32_t width, const uint32_t height) : m_Width(width), m_Height(height) {}

	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }

	std::string ToString() const override
	{
		std::stringstream ss;
		ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
		return ss.str();
	}

	EVENT_CLASS_TYPE(WindowResize)
	EVENT_CLASS_CATEGORY(EventCategory::Application)
	
private:
	uint32_t m_Width;
	uint32_t m_Height;
};

class WindowCloseEvent : public Event
{
public:
	WindowCloseEvent() = default;

	EVENT_CLASS_TYPE(WindowClose)
	EVENT_CLASS_CATEGORY(EventCategory::Application)
};

class AppTickEvent : public Event
{
public:
	AppTickEvent() = default;

	EVENT_CLASS_TYPE(AppTick)
	EVENT_CLASS_CATEGORY(EventCategory::Application)
};

class AppUpdateEvent : public Event
{
public:
	AppUpdateEvent() = default;

	EVENT_CLASS_TYPE(AppUpdate)
	EVENT_CLASS_CATEGORY(EventCategory::Application)
};

class AppRenderEvent : public Event
{
public:
	AppRenderEvent() = default;

	EVENT_CLASS_TYPE(AppRender)
	EVENT_CLASS_CATEGORY(EventCategory::Application)
};