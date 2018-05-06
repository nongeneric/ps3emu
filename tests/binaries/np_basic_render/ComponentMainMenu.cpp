/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 475.001 */
/*   Copyright (C) 2009 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

//****************************************************
//
// Title:       NpBasic simple sample program
// Description: Network initialization, NpBasic, Friendlist,
//              Profile, Custom Menu functionality demonstration.
// Date:        06/2008 
// File:        ComponentMainMenu.cpp
//
//****************************************************

#include <sysutil_common.h>
#include "ComponentMainMenu.h"
#include "np_conf.h"
#include "utility_wrappers.h"

//***********************************************************
// Class: ComponentMainMenu
// Method: ComponentMainMenu()
// Description: default constructor
//***********************************************************
ComponentMainMenu::ComponentMainMenu()
  : ComponentBase(),
	m_start_component_id(COMPONENT_NOP),
	m_skip_start(false)
										 
{
}

//***********************************************************
// Class: ComponentMainMenu
// Method: ~ComponentMainMenu()
// Description: default destructor
//***********************************************************
ComponentMainMenu::~ComponentMainMenu()
{
}

//***********************************************************
// Class: ComponentMainMenu
// Method: init()
// Description: initialize main menu
//		Build the gui for the main menu here
//***********************************************************
bool ComponentMainMenu::init()
{
	if(m_status != STATUS_UNINITIALIZED)
		return true;

	m_sys_cb_id = SysCallback::reg_cb(ComponentMainMenu::sysutilCb, this);

	//----------------------------------------------------------
	// Gui Window set-up
	//----------------------------------------------------------
	m_gui_window.set_pos(0.0f, 0.0f);
	m_gui_window.set_is_noback(true);
	
	m_gui_window.add_label(0.28, 0.3, "A Basic NP Sample", GUI_DEFAULT_LABEL_SIZE, GUI_DEFAULT_LABEL_COLOR);
	{
		std::stringstream ss;
		ss << "(" << NpConf::npCommId()->data << ")";
		m_gui_window.add_label(0.5, 0.4, ss.str(), GUI_DEFAULT_LABEL_SIZE / 2.0, GUI_DEFAULT_LABEL_COLOR);
	}

	std::string enter_button_str = Input::gamepad()->enter_button_symbol() + " Enter";
	//std::string cancel_button_str = Input::gamepad()->cancel_button_symbol() + " Back";
	//m_gui_window.add_label(0.45, 0.85, enter_button_str +" "+ cancel_button_str, GUI_DEFAULT_INFO_SIZE, GUI_DEFAULT_INFO_COLOR);
	m_gui_window.add_label(0.45, 0.85, enter_button_str, GUI_DEFAULT_INFO_SIZE, GUI_DEFAULT_INFO_COLOR);
	
	GuiMenu* main_menu = m_gui_window.add_menu(0, 0);
	
	GuiItemButton* start_button = main_menu->add_button(0.45, 0.55);
	start_button->set_label("Start");
	start_button->set_size(GUI_DEFAULT_BUTTON_SIZE);
	start_button->set_color(GUI_DEFAULT_BUTTON_COLOR);
	start_button->set_tool_tip("Choose this to start using NP!");
	start_button->register_callback(makeFunctor(this, &ComponentMainMenu::start_callback));
	
	GuiManager::register_window(&m_gui_window);
	//----------------------------------------------------------
	// End Gui Window set-up
	//----------------------------------------------------------
	
	m_status = STATUS_STOPPED;
	
	return true;
}

//***********************************************************
// Class: ComponentMainMenu
// Method: shutdown()
// Description: Do clean up at the end of the program
//***********************************************************
bool ComponentMainMenu::shutdown()
{
	m_status = STATUS_UNINITIALIZED;
	return true;
}

//***********************************************************
// Class: ComponentMainMenu
// Method: start()
// Description: start the main menu before running it.
//***********************************************************
bool ComponentMainMenu::start(ComponentID callerId)
{
	resume();
	
	m_caller_ID = callerId;
	
	return true;
}

//***********************************************************
// Class: ComponentMainMenu
// Method: resume()
// Description: Resume component.
//***********************************************************
void ComponentMainMenu::resume()
{
	m_status = STATUS_RUNNING;
	
	m_gui_window.activate();
}

//***********************************************************
// Class: ComponentMainMenu
// Method: stop()
// Description: stop the component when not needed
//***********************************************************
bool ComponentMainMenu::stop()
{
	if(m_status != STATUS_STOPPED)
	{
		m_status = STATUS_STOPPED;
		m_gui_window.window_exit();
	}
	
	return true;
}

//***********************************************************
// Class: ComponentMainMenu
// Method: update_input()
// Description: Update input
//***********************************************************
void ComponentMainMenu::update_input()
{
	if(Input::gamepad()->circle_pressed())
	{
		//Do something...
		//e.g.
		//if(m_input->gamepad()->start_pressed())
		//{
		//	m_gui_window.activate();
		//	m_gui_window.unhide();
		//}
	}
}

//***********************************************************
// Class: ComponentMainMenu
// Method: run()
// Description: Main loop of the component
//***********************************************************
void ComponentMainMenu::run()
{
	if(m_status != STATUS_RUNNING)
		return;

	if (m_skip_start) {
		m_skip_start = false;
		start_callback();
	}
}

//***********************************************************
// Class: ComponentMainMenu
// Method: render()
// Description: Render gui, etc. each cycle
//***********************************************************
void ComponentMainMenu::render()
{
	static int i = 0;
	if (i == 2)
		exit(0);
	i++;
	//Do component specific rendering
	//...
}

//***********************************************************
// Class: ComponentMainMenu
// Method: callback_goto_matching2()
// Description: gui callback
//***********************************************************
void ComponentMainMenu::start_callback()
{
	if(m_start_component_id == COMPONENT_NOP ||
		m_skip_start == true)
		return;
	
	m_request = m_start_component_id;
	
	stop();
}

//***********************************************************
// Class: ComponentMainMenu
// Method: deny_request()
// Description: When the request couldn't be handled resume
//		the main menu.
//***********************************************************
void ComponentMainMenu::deny_request(ComponentStatus requested_component_status)
{
	(void)requested_component_status;
	clear_request();
	resume();
}

void ComponentMainMenu::skip_start(void)
{
	m_skip_start = true;
}

void ComponentMainMenu::sysutilCb(
	uint64_t status,
	uint64_t param,
	void* userdata)
{
	ComponentMainMenu *self = (ComponentMainMenu*)userdata;
	DBG("MainMenu:%s (status=0x%llx,param=0x%llx)\n", __FUNCTION__, status, param);

	if(self->m_status == STATUS_RUNNING) {
		switch (status) {
		case CELL_SYSUTIL_NP_INVITATION_SELECTED:
			DBG("MainMenu:%s CELL_SYSUTIL_NP_INVITATION_SELECTED\n", __FUNCTION__);
			self->start_callback();
			break;
		case CELL_SYSUTIL_SYSTEM_MENU_CLOSE:
		case CELL_SYSUTIL_SYSTEM_MENU_OPEN:
		case CELL_SYSUTIL_REQUEST_EXITGAME:
		case CELL_SYSUTIL_DRAWING_BEGIN:
		case CELL_SYSUTIL_DRAWING_END:
			break;
		default:
			DBG("MainMenu:%s Unknown status(status=0x%llx,param=0x%llx)\n", __FUNCTION__, status, param);
			/* do nothing */
			break;
		}
	} else {
		DBG("(ignored)\n");
	}

	return;
}
