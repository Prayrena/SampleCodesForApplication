#pragma once
#include "Engine/core/NamedStrings.hpp"
#include "Engine/core/StringUtils.hpp"
#include <string>
#include <vector>
#include <Mutex>

// "callback" in this system literally means the function ptr that is prepared to be trigger in the future
// "event" means the std::map which bounds the string and function
typedef NamedStrings EventArgs;
typedef bool (*EventCallbackFuncPtr)(EventArgs& eventArgs); // call back funcs could take the args modified in the func // todo: why I must have const for EventArgs const& eventArgs?
typedef std::vector<EventCallbackFuncPtr> SubscriptionList;

struct EventSubscription
{
	EventSubscription(EventCallbackFuncPtr functionPtr)
		:m_functionPtr(functionPtr)
	{
	}

	EventCallbackFuncPtr m_functionPtr = nullptr;
};

struct EventSystemConfig
{

};

class EventSystem
{
friend class DevConsole;

public:
	EventSystem (EventSystemConfig const& config );
	EventSystem() {};
	~EventSystem() {};
	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFuncPtr callbackFuncPtr);
	void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFuncPtr callbackFuncPtr);
	void UnscribeFromAllEvents(EventCallbackFuncPtr callbackFunc);
	void FireEvent(std::string const& eventName, EventArgs& args);
	void FireEvent(std::string const& eventName); // used when the event argument is set up within this function or it does not need any argument

	Strings GetAllSubscriptionEventNames();

protected:
	EventSystemConfig							m_config;

	std::mutex									m_subscriptionlistsByEventNamesMutex;
	std:: map<std::string, SubscriptionList>	m_subscriptionlistsByEventNames;
};

//----------------------------------------------------------------------------------------------------------------------------------------------------
// standalone global-namespace helper functions: these forward to "the" event system, which should exist for every game
void SubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFuncPtr callbackFunc);
void UnsubscribeEventCallbackFunction(std::string const& eventName, EventCallbackFuncPtr callbackFunc);
void UnscribeFromAllEvents(EventCallbackFuncPtr callbackFunc);
void FireEvent(std::string const& eventName, EventArgs& args);
void FireEvent(std::string const& eventName);