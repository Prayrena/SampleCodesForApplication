#include "Engine/core/DevConsole.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/Time.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/core/EventSystem.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/core/StringUtils.hpp"
#include "../Math/AABB2.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include <map>

extern DevConsole*	g_theDevConsole;
extern InputSystem* g_theInput;
extern Clock*		g_theGameClock;

// Rgba8 const DevConsole::ERROR = Rgba8::RED; // should not use this way becauseglobal variables (including class static variables) are constructed before main(), and in random order.  
// So if Rgba8::RED happens to be created before DevConsole::INFO_ERROR then it will work; otherwise, it won't.
const Rgba8 DevConsole::INFO_ERROR				= Rgba8(255, 0, 0);
const Rgba8 DevConsole::INFO_WARNING			= Rgba8(205, 96, 16, 255);// light orange
const Rgba8 DevConsole::INFO_MAJOR				= Rgba8(0, 255, 255);// cyan
const Rgba8 DevConsole::INFO_MINOR				= Rgba8(248, 220, 103, 255);// naples yellow

const Rgba8 DevConsole::INPUT_TEXT				= Rgba8(255, 255, 255);
const Rgba8 DevConsole::INPUT_INSERTION_POINT	= Rgba8(255, 255, 255);


DevConsole::DevConsole(DevConsoleConfig const& config)
	:m_config(config)
{
	if (m_config.m_numLinesOnScreen == 0.f)
	{
		m_config.m_numLinesOnScreen = 1.f;	 // make sure the m_numLines won't be 0
	}
}

void DevConsole::Startup()
{
	// for the dev console functions
	SubscribeEventCallbackFunction("ControlInstructions", DevConsole::Command_ControlInstructions);
	SubscribeEventCallbackFunction("CharInput", DevConsole::Event_CharInput);
	SubscribeEventCallbackFunction("KeyPressed", DevConsole::Event_KeyPressed);
	SubscribeEventCallbackFunction("clear", DevConsole::Command_Clear);
	SubscribeEventCallbackFunction("help", DevConsole::Command_Help);
	SubscribeEventCallbackFunction("ChangeTimeScale", DevConsole::Command_ChangeTimeScale);

	// for the debug render system
	SubscribeEventCallbackFunction("DebugRenderToggle", Command_DebugRenderToggle);
	SubscribeEventCallbackFunction("DebugRenderClear", Command_DebugRenderClear);

	// set the timer for insertion point
	m_insertionPointBlinkTimer = new Timer(0.5f);
	m_systemInfoTimer = new Timer(0.15f);
	m_systemInfoTimer->Start();

	// save the default insertion point ratio so for later we could control the scale for making the animation
	m_originalInsertionScale = g_theDevConsole->m_originalInsertionScale = g_theDevConsole->m_config.m_insertionHeightLineHeightRatio;
}

void DevConsole::Shutdown()
{

}

void DevConsole::BeginFrame()
{
	++m_frameNumber;
}

void DevConsole::EndFrame()
{

}

// use this function to trigger the event system
void DevConsole::Execute(std::string const& consoleCommandText)
{
	// for every text lines, the string before first spaceBar' ' is the command string
	Strings commandTextLine = SplitStringOnDelimiter(consoleCommandText, ' ');
	std::string commandString = commandTextLine[0];

	// then we detect if the string has '=', e.g. "undead m_health = 999"
	Strings keyAndValue = SplitStringOnDelimiter(consoleCommandText, '=');

	int numCutByEqual = (int)keyAndValue.size();
	int numCutBySpace = (int)commandTextLine.size();

	// "undead"
	//if the input string don't have a '='
	if (numCutBySpace == 1 && numCutByEqual == 1)
	{
		FireEvent(commandString);

		PrintExecuteCommandOnScreen(consoleCommandText);
	}			
	// numCutBySpace == 2: "undead m_health = 999"
	// (int)keyAndValue.size() == 4: "undead m_health = 999" has 4 when cut by; "undead m_health=999" has 2
	else if (numCutBySpace == 4 && numCutByEqual == 2)
	{	
		// say we have 'unDead playerHealth = 999'
		std::string keyString = commandTextLine[1];
		std::string valueString = commandTextLine[3];
		// write this key string and value string into the game blackboard
		EventArgs args;
		args.SetValue(keyString, valueString);
		// the strings between first 
		FireEvent(commandString, args);

		PrintExecuteCommandOnScreen(consoleCommandText);
	}
	else if(numCutByEqual > 1)// if there is = in the command line
	{
		m_linesMutex.lock();
		g_theDevConsole->m_lines.push_back(DevConsoleLine(consoleCommandText, DevConsole::INFO_ERROR));
		g_theDevConsole->m_lines.push_back(DevConsoleLine("Syntax Error. Follow the format: EventName key = value", DevConsole::INFO_ERROR));
		m_linesMutex.unlock();
	}
	else
	{
		m_linesMutex.lock();
		g_theDevConsole->m_lines.push_back(DevConsoleLine(consoleCommandText, DevConsole::INFO_ERROR));
		g_theDevConsole->m_lines.push_back(DevConsoleLine("Unknown Command: Use \"help\" for registered commands", DevConsole::INFO_ERROR));
		m_linesMutex.unlock();
	}

	// clear the input text line
	g_theDevConsole->m_inputTexts.clear();
}

void DevConsole::PrintExecuteCommandOnScreen(std::string consoleCommandText)
{
	// if the key is wrong or is the string is '', do not print on interface
	if (consoleCommandText.empty() || g_theDevConsole->m_executeFoundSubscriberButKeyIsWrong)
	{
		g_theDevConsole->m_executeFoundSubscriberButKeyIsWrong = false;
		return;
	}
	// if the event corresponding to the command is found
	if (m_executeFoundSubscriber)
	{
		// print the line on screen if the devConsole execute a command
		// write the fire command into the history
		g_theDevConsole->m_commandHistory.push_back(DevConsoleLine(consoleCommandText, DevConsole::INFO_MAJOR));
		g_theDevConsole->AddLine(consoleCommandText, DevConsole::INFO_MAJOR);
		m_executeFoundSubscriber = false;
	}
	else
	{
		g_theDevConsole->AddLine(consoleCommandText, DevConsole::INFO_ERROR);
		g_theDevConsole->m_lines.push_back(DevConsoleLine("Unknown Command: Use \"help\" for registered commands", DevConsole::INFO_ERROR));
		m_executeFoundSubscriber = false;
	}
}

void DevConsole::AddLine(std::string const& text, Rgba8 const& color)
{
	// write in the DevConsole text line information
	DevConsoleLine newLine(text, color);
	newLine.m_timePrinted = GetCurrentTimeSeconds();
	newLine.m_frameNumberPrinted = m_frameNumber;

	m_linesMutex.lock();
	m_lines.push_back(newLine);
	m_linesMutex.unlock();
	// no need to delete the old lines when exceeding the max num of lines, will remember all history
}

void DevConsole::AddInstruction(std::string const& text, Rgba8 const& color /*= INFO_MAJOR*/)
{
	// write in the DevConsole text line information
	DevConsoleLine* newLine = new DevConsoleLine(text, color);
	newLine->m_timePrinted = GetCurrentTimeSeconds();
	newLine->m_frameNumberPrinted = m_frameNumber;

	m_controlInstructions.push_back(*newLine);
}

void DevConsole::Render(AABB2 const& bounds, Renderer* rendererOverride /*= nullptr*/)
{
	// if the console is not opened, nothing will be shown
	if (!m_isOpen)
	{
		return;
	}

	// if the override is nullptr, we will use the config renderer to render the devConsole
	Renderer* renderer = rendererOverride ? rendererOverride : m_config.m_renderer;
	RenderConsole(bounds, *renderer); 
}

void DevConsole::RenderConsole(AABB2 const& bounds, Renderer& renderer)
{
	std::vector<Vertex_PCU> backgroundVerts;

	float screenHeigt = bounds.m_maxs.y - bounds.m_mins.y;
	float textLineHeight = screenHeigt / m_config.m_numLinesOnScreen;
	float fontHeight = g_theDevConsole->m_config.m_lineHeightAndTextBoxRatio * textLineHeight;
	float fontWidth = g_theDevConsole->m_config.m_fontAspect * fontHeight;
	Vec2  fontAlignment = Vec2(0.f, 0.5f);

	// set up the display area for the interface and the input line
	// so the input line is always at the bottom
	// shrink the camera bounds a little so it is not tight on the side
	AABB2 shrinkBounds = bounds;
	shrinkBounds.SetUniformScale(0.97f);
	AABB2 interfaceBox = shrinkBounds;
	interfaceBox.m_mins.y += textLineHeight;
	AABB2 inputBox = shrinkBounds;
	inputBox.m_maxs.y = shrinkBounds.m_mins.y + textLineHeight;

	// choose the display area base on different mode
	AABB2 overrideBounds = bounds;
	Rgba8 backgroundColor = Rgba8(0, 0, 0, 120);
	switch (m_mode)
	{
	case DevConsoleMode::OPENFULL:
		break;
	// use top right part of the screen to show FPS and system/game clock
	case DevConsoleMode::SYSTEMSTATISTICS: {
		Vec2 dimensions = bounds.GetDimensions();
		AABB2 topRightCornerBox(Vec2(bounds.m_mins.x + dimensions.x * 0.75f, bounds.m_mins.y + dimensions.y * 0.75f), Vec2(bounds.m_maxs.x, bounds.m_maxs.y));
		overrideBounds = topRightCornerBox;
		backgroundColor = Rgba8(0, 0, 0, 60); }
		break;
	// use lower part of the screen showing the dev console
	case DevConsoleMode::BOTTOMSCREEN: {
		AABB2 bottomBoxScreen(Vec2(bounds.m_mins.x, bounds.m_mins.y), Vec2(bounds.m_maxs.x, bounds.m_maxs.y * 0.25f));
		overrideBounds = bottomBoxScreen; }
		break;
	default:
		break;
	}

	// the background is only needed to be rendered once
	renderer.BindTexture(nullptr);
	renderer.SetBlendMode(BlendMode::ALPHA);
	AddVertsForAABB2D(backgroundVerts, overrideBounds, backgroundColor);
	renderer.DrawVertexArray((int)backgroundVerts.size(), backgroundVerts.data());

	// draw the dev console frame to show the status and frame
	Vec2 BL = overrideBounds.GetPointAtUV(Vec2(0.f, 0.f));
	Vec2 BR = overrideBounds.GetPointAtUV(Vec2(1.f, 0.f));
	Vec2 TL = overrideBounds.GetPointAtUV(Vec2(0.f, 1.f));
	Vec2 TR = overrideBounds.GetPointAtUV(Vec2(1.f, 1.f));
	LineSegment2 leftFrame(BL, TL);
	LineSegment2 rightFrame(BR, TR);
	LineSegment2 upperFrame(TL, TR);
	LineSegment2 bottomFrame(BL, BR);
	float frameThickness = overrideBounds.GetDimensions().y * 0.02f;
	AddVertsForLineSegment2D(backgroundVerts, leftFrame, frameThickness, m_frameColor);
	AddVertsForLineSegment2D(backgroundVerts, rightFrame, frameThickness, m_frameColor);
	AddVertsForLineSegment2D(backgroundVerts, upperFrame, frameThickness, m_frameColor);
	AddVertsForLineSegment2D(backgroundVerts, bottomFrame, frameThickness, m_frameColor);
	renderer.DrawVertexArray((int)backgroundVerts.size(), backgroundVerts.data());

	if (m_mode == DevConsoleMode::OPENFULL || m_mode == DevConsoleMode::BOTTOMSCREEN)
	{
		std::vector<Vertex_PCU> interfaceTextVerts;

		// get the bounds for fist interface text line
		AABB2 currentLineBox(Vec2(interfaceBox.m_mins.x, interfaceBox.m_mins.y), Vec2(interfaceBox.m_maxs.x, interfaceBox.m_mins.y + textLineHeight));
		AABB2 currentLineShadowBox = currentLineBox + Vec2((overrideBounds.GetDimensions().x) * 0.003f, (overrideBounds.GetDimensions().y) * 0.003f);

		// calculate how many lines to draw on screen (performance)
		int linesLeftToDrawOnScreen = (int)(m_config.m_numLinesOnScreen);
		if (m_mode == DevConsoleMode::BOTTOMSCREEN)
		{
			linesLeftToDrawOnScreen = (int)(m_config.m_numLinesOnScreen * 0.25f - 1.f);
		}

		//----------------------------------------------------------------------------------------------------------------------------------------------------
		// draw interface
		m_linesMutex.lock();
		if (!m_lines.empty())
		{
			// draw the line from bottom to up
			for (int lineIndex = ((int)m_lines.size() - 1); lineIndex >= 0; --lineIndex)
			{
				// if the screen is filled with text lines, then stop render older text lines
				if (linesLeftToDrawOnScreen == 0)
				{
					// and also delete the old text lines
					if ((float)m_lines.size() > m_config.m_numLinesOnScreen) 
					{
						m_lines.erase(m_lines.begin(), m_lines.begin() + (int)((float)m_lines.size() - m_config.m_numLinesOnScreen));
					}
					break;
				}

				// print out the console line by order from the latest to oldest in shrink to fit mode
				DevConsoleLine const* textLine = &m_lines[lineIndex];
				m_config.m_font->AddVertsForTextInBox2D(interfaceTextVerts, textLine->m_displayInfo, currentLineShadowBox, fontHeight, Vec2(0.f, 0.5f), Rgba8::BLACK, 0.6f);
				m_config.m_font->AddVertsForTextInBox2D(interfaceTextVerts, textLine->m_displayInfo, currentLineBox, fontHeight, Vec2(0.f, 0.5f), textLine->m_startColor, 0.6f);

				--linesLeftToDrawOnScreen;

				// move the text line box upwards a line each time
				currentLineBox.Translate(Vec2(0.f, textLineHeight));
				currentLineShadowBox.Translate(Vec2(0.f, textLineHeight));
			}

			m_linesMutex.unlock();

			renderer.BindTexture(&m_config.m_font->GetTexture());
			renderer.SetDepthMode(DepthMode::DISABLED);
			renderer.DrawVertexArray((int)interfaceTextVerts.size(), interfaceTextVerts.data());
		}

		//----------------------------------------------------------------------------------------------------------------------------------------------------
		// draw the input text line
		if (!m_inputTexts.empty())
		{
			std::vector<Vertex_PCU> inputTextVerts;
			// shadow effects bounds
			AABB2 inputShadowBox = inputBox + Vec2((bounds.GetDimensions().x) * 0.005f, (bounds.GetDimensions().y) * 0.005f);

			renderer.BindTexture(&m_config.m_font->GetTexture());
			m_config.m_font->AddVertsForTextInBox2D(inputTextVerts, m_inputTexts, inputShadowBox, fontHeight, Vec2(0.f, 0.5f), Rgba8::BLACK, 0.6f);
			m_config.m_font->AddVertsForTextInBox2D(inputTextVerts, m_inputTexts, inputBox, fontHeight, Vec2(0.f, 0.5f), INPUT_TEXT, 0.6f);
			renderer.DrawVertexArray((int)inputTextVerts.size(), inputTextVerts.data());
		}

		//unbind texture
		renderer.BindTexture(nullptr);

		//----------------------------------------------------------------------------------------------------------------------------------------------------
		// draw the insert point
		if (m_insertionPointBlinkTimer->DecrementPeroidIfElapsed())
		{
			m_insertionPointVisible = !m_insertionPointVisible;

			// reset the insertion scale
			// animation reset for the left/right arrow key
			g_theDevConsole->m_config.m_insertionHeightLineHeightRatio = g_theDevConsole->m_originalInsertionScale;
		}
		if (m_insertionPointVisible)
		{
			std::vector<Vertex_PCU> insertionPointVerts;

			AABB2 insertionBox;
			Vec2 insertionCenter = inputBox.m_mins + Vec2(m_insertionPointPosIndex * fontWidth, textLineHeight * 0.5f);
			insertionBox.SetCenter(insertionCenter);
			insertionBox.SetDimensions(Vec2(fontWidth * 0.25f, textLineHeight* g_theDevConsole->m_config.m_insertionHeightLineHeightRatio));

			AddVertsForAABB2D(insertionPointVerts, insertionBox, DevConsole::INPUT_INSERTION_POINT);
			renderer.DrawVertexArray((int)insertionPointVerts.size(), insertionPointVerts.data());
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// show system statistic: fps, system running time, game running time	
	// if only show system statistics, no need to draw input lines
	if (m_mode == DevConsoleMode::SYSTEMSTATISTICS)
	{
		std::vector<Vertex_PCU> systemInfoVerts;

		// get fps string and system update every 0.15 second
		if (m_systemInfoTimer->DecrementPeroidIfElapsed())
		{
			m_fps = Clock::GetSystemClock().GetFrameRatePerSecond();
			m_systemTime = Clock::GetSystemClock().GetTotalSeconds();
			m_GameTime = g_theGameClock->GetTotalSeconds();
		}
		std::string line_fps = "FPS: " + std::to_string(m_fps);
		std::string line_systemTime = "System Time: " + std::to_string(m_systemTime);
		line_systemTime.erase(18);
		std::string line_gameTime = "Game Time: " + std::to_string(m_GameTime);
		line_gameTime.erase(16);

		// first line 
		Vec2 dimensions = shrinkBounds.GetDimensions();
		AABB2 topRightCornerBox(Vec2(shrinkBounds.m_mins.x + dimensions.x * 0.75f, shrinkBounds.m_mins.y + dimensions.y * 0.75f), Vec2(shrinkBounds.m_maxs.x, shrinkBounds.m_maxs.y));
		AABB2 topLineBox(Vec2(topRightCornerBox.m_mins.x, topRightCornerBox.m_maxs.y - textLineHeight), 
			Vec2(topRightCornerBox.m_maxs.x, topRightCornerBox.m_maxs.y));
		m_config.m_font->AddVertsForTextInBox2D(systemInfoVerts, line_fps, topLineBox, fontHeight, Vec2(1.f, 0.5f), m_systemColor, 0.6f);

		// second line
		topLineBox.Translate(Vec2(0.f, textLineHeight * (-1.f)));
		m_config.m_font->AddVertsForTextInBox2D(systemInfoVerts, line_systemTime, topLineBox, fontHeight, Vec2(1.f, 0.5f), m_systemColor, 0.6f);

		// second line
		topLineBox.Translate(Vec2(0.f, textLineHeight* (-1.f)));
		m_config.m_font->AddVertsForTextInBox2D(systemInfoVerts, line_gameTime, topLineBox, fontHeight, Vec2(1.f, 0.5f), m_systemColor, 0.6f);
		renderer.BindTexture(&m_config.m_font->GetTexture());
		renderer.DrawVertexArray((int)systemInfoVerts.size(), systemInfoVerts.data());

		//unbind texture
		renderer.BindTexture(nullptr);
	}
}

DevConsoleMode DevConsole::GetMode() const
{
	return m_mode;
}

void DevConsole::SetMode(DevConsoleMode mode)
{
	m_mode = mode;
}

void DevConsole::ToggleMode()
{
	int modeIndex = (int)m_mode;
	m_mode = DevConsoleMode(modeIndex + 1);
	if ( m_mode == DevConsoleMode::NUM_DEVCONSOLE_MODE)
	{
		m_mode = DevConsoleMode(0);
	}
}

void DevConsole::ToggleOpen()
{
	if (g_theDevConsole->m_isOpen)
	{
		g_theDevConsole->m_isOpen = false;
		m_insertionPointBlinkTimer->Stop();
	}
	else
	{
		g_theDevConsole->m_isOpen = true;
		m_insertionPointBlinkTimer->Start();
	}
}

bool DevConsole::IsOpen()
{
	return g_theDevConsole->m_isOpen;
}

// handle the console open or close, arrow keys messages
// handle 
STATIC bool DevConsole::Event_KeyPressed(EventArgs& args)
{
	// if the devConsole does not exists, do not consume the message
	if (!g_theDevConsole)
	{
		return false;
	}

	// When there is no KeyCode could be read from the args
	int keyCode = args.GetValue("KeyCode", -1);
	unsigned char inputChar = (unsigned char)args.GetValue("KeyCode", -1);
	if (keyCode == -1)
	{
		g_theDevConsole->m_linesMutex.lock();
		g_theDevConsole->m_lines.push_back(DevConsoleLine("Syntax Error. Follow the format: KeyPressed KeyCode = value", DevConsole::INFO_ERROR));
		g_theDevConsole->m_linesMutex.unlock();
		return false;
	}

	// the tilde key control open/close of the console
	if (inputChar == KEYCODE_TILDE)
	{
		g_theDevConsole->ToggleOpen();
		return true;
	}

	// if the console is not open, do not proceed the action
	if (!g_theDevConsole->IsOpen())
	{
		return false;
	}

	// F11 will loop through all the display mode
	if (inputChar == KEYCODE_F11)
	{
		g_theDevConsole->ToggleMode();
		return true;
	}

	// home key will move the insertion point back to the beginning of the line, same to '$'
	if (inputChar == 36)
	{
		g_theDevConsole->m_insertionPointPosIndex = 0;
		return true;
	}

	// home key will move the insertion point back to the beginning of the line, same to '#'
	if (inputChar == 35)
	{
		g_theDevConsole->m_insertionPointPosIndex = (int)g_theDevConsole->m_inputTexts.size();
		return true;
	}

	// see if the player trying to execute the command
	if (inputChar == KEYCODE_ENTER)
	{
		if (!g_theDevConsole->m_inputTexts.empty())
		{
			g_theDevConsole->Execute(g_theDevConsole->m_inputTexts);
			g_theDevConsole->m_insertionPointPosIndex = 0;
		}
		else
		{
			g_theDevConsole->m_isOpen = false;
		}
		return true;
	}

	// if the console is opened and the input text is empty
	// the ESC key pressed will close the console
	if (inputChar == KEYCODE_ESC)
	{
		// if the dev is open, ESC will close the console
		if (g_theDevConsole->IsOpen() && g_theDevConsole->m_inputTexts.empty())
		{
			g_theDevConsole->ToggleOpen();
			return true;
		}
		else if(g_theDevConsole->IsOpen() && !g_theDevConsole->m_inputTexts.empty()) // otherwise the ESC will just clear the input text line
		{
			g_theDevConsole->m_inputTexts.clear();
			g_theDevConsole->m_insertionPointPosIndex = 0;
			return true;
		}
		else // otherwise the ESC will just control the game
		{
			return false;
		}
	}

	// backspace will delete the char on the left of the insertion point
	if (inputChar == KEYCODE_BACKSPACE || inputChar == KEYCODE_DELETE)
	{
		// the insertion point will keep visible while deleting
		g_theDevConsole->m_insertionPointVisible = true;
		g_theDevConsole->m_insertionPointBlinkTimer->Restart();

		if (inputChar == KEYCODE_BACKSPACE)
		{
			if (!g_theDevConsole->m_inputTexts.empty() && g_theDevConsole->m_insertionPointPosIndex != 0)
			{
				g_theDevConsole->m_inputTexts.erase((size_t)g_theDevConsole->m_insertionPointPosIndex - 1, 1);
				g_theDevConsole->m_insertionPointPosIndex -= 1;
			}
			else
			{
				return true;
			}
		}

		// delete will delete the char on the right of the insertion point
		if (inputChar == KEYCODE_DELETE)
		{
			if (!g_theDevConsole->m_inputTexts.empty() && g_theDevConsole->m_insertionPointPosIndex < (int)g_theDevConsole->m_inputTexts.size())
			{
				g_theDevConsole->m_inputTexts.erase( g_theDevConsole->m_insertionPointPosIndex, 1 );
			}
			else
			{
				return true;
			}
		}
	}

	// the arrow key up and down will change the history index
	if (inputChar == KEYCODE_UPARROW || inputChar == KEYCODE_DOWNARROW)
	{
		// the insertion point will keep visible while using keys
		g_theDevConsole->m_insertionPointVisible = true;
		g_theDevConsole->m_insertionPointBlinkTimer->Restart();

		int commandHistoryLength = (int)g_theDevConsole->m_commandHistory.size();
		if (g_theDevConsole->m_commandHistory.empty())
		{
			g_theDevConsole->m_inputTexts.clear(); // press upper or lower arrow key will clear the command line only if there was a command
		}
		if (inputChar == KEYCODE_UPARROW)
		{
			if (g_theDevConsole->m_mode == DevConsoleMode::SYSTEMSTATISTICS)
			{
				return false;
			}

			// if the arrow key never been pressed, up arrow will get the last one
			// if the index is at the 0, up arrow will get the last one too
			if (g_theDevConsole->m_historyIndex == -1 || g_theDevConsole->m_historyIndex == 0)
			{
				g_theDevConsole->m_historyIndex = commandHistoryLength - 1;
			}
			else
			{
				g_theDevConsole->m_historyIndex -= 1;
			}
		}
		if (inputChar == KEYCODE_DOWNARROW)
		{
			if (g_theDevConsole->m_mode == DevConsoleMode::SYSTEMSTATISTICS)
			{
				return false;
			}

			// if the arrow key never been pressed, up arrow will get the last one
			// if the index is at the 0, up arrow will get the last one too
			if (g_theDevConsole->m_historyIndex == -1)
			{
				g_theDevConsole->m_historyIndex = 0;
			}
			else if (g_theDevConsole->m_historyIndex == (commandHistoryLength - 1))
			{
				g_theDevConsole->m_historyIndex = 0;
			}
			else
			{
				g_theDevConsole->m_historyIndex += 1;
			}
		}

		// if the command history is not empty
		// then the up and down arrow key will show the change the input text with the command history
		if (!g_theDevConsole->m_commandHistory.empty())
		{
			g_theDevConsole->m_inputTexts = g_theDevConsole->m_commandHistory[g_theDevConsole->m_historyIndex].m_displayInfo;
			g_theDevConsole->m_insertionPointPosIndex = (int)g_theDevConsole->m_inputTexts.size();
			return true;
		}
	}

	// left and right keys changes the horizontal position of the insertion point
	if (inputChar == KEYCODE_LEFTARROW || inputChar == KEYCODE_RIGHTARROW)
	{
		// the insertion point will keep visible while moving
		g_theDevConsole->m_insertionPointVisible = true;
		g_theDevConsole->m_insertionPointBlinkTimer->Restart();

		if (inputChar == KEYCODE_LEFTARROW)
		{
			if (g_theDevConsole->m_mode == DevConsoleMode::SYSTEMSTATISTICS)
			{
				return false;
			}

			if (g_theDevConsole->m_insertionPointPosIndex > 0)
			{
				g_theDevConsole->m_insertionPointPosIndex -= 1;
				return true;
			}
			else
			{
				g_theDevConsole->m_config.m_insertionHeightLineHeightRatio = 2.f;
				return true;
			}
		}
		if (inputChar == KEYCODE_RIGHTARROW)
		{
			if (g_theDevConsole->m_mode == DevConsoleMode::SYSTEMSTATISTICS)
			{
				return false;
			}

			if (g_theDevConsole->m_insertionPointPosIndex < (int)g_theDevConsole->m_inputTexts.size())
			{
				g_theDevConsole->m_insertionPointPosIndex += 1;
				return true;
			}
			else
			{
				g_theDevConsole->m_config.m_insertionHeightLineHeightRatio = 2.f;
				return true;
			}
		}

	}

	// if the devConsole is opened
	if (g_theDevConsole->m_isOpen)
	{
		// if current mode is showing system statistics, player could control the game
		if (g_theDevConsole->m_mode == DevConsoleMode::SYSTEMSTATISTICS)
		{
			return false;
		}
		// in other modes, player could not control the game
		else
		{
			return true;
		}
	}

	// for other key codes, do not consume the message
	return false;
}

STATIC bool DevConsole::Event_CharInput(EventArgs& args)
{
	// if the devConsole does not exists, do not consume the message
	if (!g_theDevConsole)
	{
		return false;
	}

	// is the console is not open, do nothing
	if (!g_theDevConsole->IsOpen())
	{
		return false;
	}

	// if current mode is showing system statistics, player is not inputing the characters
	if (g_theDevConsole->m_mode == DevConsoleMode::SYSTEMSTATISTICS)
	{
		return false;
	}

	// if the console is open, then the console is going to take the WM_Char message
	// to modify the m_inputText
	unsigned char inputChar = (unsigned char)args.GetValue("TextInput", -1);
	if (inputChar == -1)
	{
		return false;
	}
	// we are not print out ESC key or ` key, which are used to control the console
	// notice we are not using KEYCODE_TILDE because it is not what WM_CHAR output
	else if (inputChar == 27 || inputChar == '`' || inputChar == '~')
	{
		return true;
	}
	else if (inputChar >= 32 && inputChar <= 126) // only take these characters or we do not have corresponding texture for the others character
	{
		// std::string insertChar = std::string(reinterpret_cast<char const*>(&inputChar), 1);
		g_theDevConsole->m_inputTexts.insert(g_theDevConsole->m_insertionPointPosIndex, 1, static_cast<char>(inputChar));
		g_theDevConsole->m_insertionPointPosIndex += 1;
		return true;
	}

	// the enter for character input is doing nothing
	// key pressed is already handled the execute
	if (inputChar == KEYCODE_ENTER)
	{
		return true;
	}

	// handle other char input
	return true;
}

STATIC bool DevConsole::Command_Clear(EventArgs& args)
{
	UNUSED(args);

	// if the devConsole does not exists, do not consume the message
	if (!g_theDevConsole)
	{
		return false;
	}

	g_theDevConsole->m_inputTexts.clear();

	g_theDevConsole->m_linesMutex.lock();
	g_theDevConsole->m_lines.clear();
	g_theDevConsole->m_linesMutex.unlock();

	return true;
}

STATIC bool DevConsole::Command_Help(EventArgs& args)
{
	UNUSED(args);

	// if the devConsole does not exists, do not consume the message
	if (!g_theDevConsole)
	{
		return false;
	}

	// get all registered events from the event system
	Strings registedEventsName = g_theEventSystem->GetAllSubscriptionEventNames();

	// add the events name to the m_lines
	int numEvents = (int)registedEventsName.size();
	for (int lineIndex = (numEvents - 1); lineIndex >= 0; --lineIndex)
	{
		g_theDevConsole->m_linesMutex.lock();
		g_theDevConsole->m_lines.push_back(DevConsoleLine(registedEventsName[lineIndex], DevConsole::INFO_MINOR));
		g_theDevConsole->m_linesMutex.unlock();
	}

	return true;
}

STATIC bool DevConsole::Command_ControlInstructions(EventArgs& args)
{
	UNUSED(args);

	g_theDevConsole->m_lines = g_theDevConsole->m_controlInstructions;
	return true;
}

STATIC bool DevConsole::Command_ChangeTimeScale(EventArgs& args)
{
	float timeScale = args.GetValue("scale", -1.f);
	if (timeScale == -1.f)
	{
		g_theDevConsole->m_linesMutex.lock();
		g_theDevConsole->m_lines.push_back(DevConsoleLine(g_theDevConsole->m_inputTexts, DevConsole::INFO_ERROR));
		std::string result = "Wrong input key. Please follow the syntax: ChangeTimeScale scale = 0.1";
		g_theDevConsole->m_lines.push_back(DevConsoleLine(result, DevConsole::INFO_ERROR));	
		g_theDevConsole->m_linesMutex.unlock();
		g_theDevConsole->m_executeFoundSubscriberButKeyIsWrong = true;
		return false;
	}
	else
	{
		g_theGameClock->SetTimeScale(timeScale);
	}
	return true;
}

