//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2013-2015 SuperTuxKart-Team
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License: or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "config/user_config.hpp"
#include "graphics/irr_driver.hpp"
#include "guiengine/engine.hpp"
#include "guiengine/screen_keyboard.hpp"
#include "guiengine/layout_manager.hpp"
#include "guiengine/widget.hpp"
#include "guiengine/widgets/button_widget.hpp"
#include "guiengine/widgets/CGUIEditBox.hpp"
#include "states_screens/state_manager.hpp"
#include "utils/log.hpp"

#ifdef ANDROID
#include "../../../lib/irrlicht/source/Irrlicht/CIrrDeviceAndroid.h"
#endif

#include <algorithm>
#include <string>

using namespace GUIEngine;

typedef std::vector<std::vector<std::string> > KeyboardLayout;
typedef std::vector<std::vector<int> > KeyboardLayoutProportions;
          
KeyboardLayout layout_lower = 
            {{"q", "w", "e", "r", "t", "y", "u", "i", "o", "p"},
             {"Separator", "a", "s", "d", "f", "g", "h", "j", "k", "l", "Separator"},
             {"Shift", "z", "x", "c", "v", "b", "n", "m", "?", "Back"},
             {"123", ",", "Space", ".", "Enter"}};
             
KeyboardLayout layout_upper = 
            {{"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"},
             {"Separator", "A", "S", "D", "F", "G", "H", "J", "K", "L", "Separator"},
             {"Shift", "Z", "X", "C", "V", "B", "N", "M", "!", "Back"},
             {"123", ",", "Space", ".", "Enter"}};
                          
KeyboardLayout layout_digits = 
            {{"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
             {"Separator", "@", "#", "$", "%", "^", "&", "*", "(", ")", "Separator"},
             {"Shift", "-", "+", ":", ";", "\"", "\'", "/", "?", "Back"},
             {"Text", ",", "Space", ".", "Enter"}};

KeyboardLayout layout_digits2 = 
            {{"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
             {"Separator", "@", "[", "]", "{", "}", "~", "`", "\\", "|", "Separator"},
             {"Shift", "_", "=", ":", ";", "<", ">", "/", "!", "Back"},
             {"Text", ",", "Space", ".", "Enter"}};

KeyboardLayoutProportions layout_proportions = 
            {{2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
             {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
             {2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
             {3, 2, 10, 2, 3}};

ScreenKeyboard* ScreenKeyboard::m_screen_keyboard = NULL;

// ----------------------------------------------------------------------------
/** The screen keyboard constructor
 *  \param percent_width A relative value in range of 0.0 to 1.0 that 
 *         determines width of the screen that will be used by the keyboard.
 *  \param percent_height A relative value in range of 0.0 to 1.0 that 
 *         determines height of the screen that will be used by the keyboard.
 *  \param edit_box The edit box that is assigned to the keyboard.
 */
ScreenKeyboard::ScreenKeyboard(float percent_width, float percent_height, 
                               CGUIEditBox* edit_box)
{
    if (m_screen_keyboard != NULL)
    {
        delete m_screen_keyboard;
        Log::warn("GUIEngine", "Showing a screen keyboard while the previous "
                  "one is still open. Destroying the previous keyboard.");
    }
    
    m_screen_keyboard = this;
    m_buttons_type    = BUTTONS_NONE;
    m_percent_width   = std::min(std::max(percent_width, 0.0f), 1.0f);
    m_percent_height  = std::min(std::max(percent_height, 0.0f), 1.0f);
    m_irrlicht_window = NULL;
    m_edit_box        = edit_box;
    m_back_button     = NULL;
    m_repeat_time     = 0;
    m_back_button_pressed = false;
    m_schedule_close = false;
    
    init();
}   // ScreenKeyboard

// ----------------------------------------------------------------------------
/** The screen keyboard destructor
 */
ScreenKeyboard::~ScreenKeyboard()
{
    m_screen_keyboard = NULL;
    
    GUIEngine::getGUIEnv()->removeFocus(m_irrlicht_window);
    m_irrlicht_window->remove();

    input_manager->setMode(m_previous_mode);

    elementsWereDeleted();
}   // ~ScreenKeyboard

// ----------------------------------------------------------------------------
/** Internal function that makes whole screen keyboard initialization
 */
void ScreenKeyboard::init()
{
    const core::dimension2d<u32>& frame_size = irr_driver->getFrameSize();

    int margin = 0;
    int w = int(frame_size.Width * m_percent_width);
    int h = int(frame_size.Height * m_percent_height);
    int x = frame_size.Width/2 - w/2;
    int y = frame_size.Height - h - margin;

    if (m_edit_box != NULL)
    {
        core::rect<s32> pos = m_edit_box->getAbsolutePosition();
        
        if (pos.LowerRightCorner.Y + 5 > y)
        {
            y = margin;
        }
    }
    
    m_area = core::rect<s32>(x, y, x + w, y + h);

    m_irrlicht_window = GUIEngine::getGUIEnv()->addWindow(m_area, true);
    m_irrlicht_window->setDrawTitlebar(false);
    m_irrlicht_window->getCloseButton()->setVisible(false);
    m_irrlicht_window->setDraggable(UserConfigParams::m_artist_debug_mode);

    m_previous_mode=input_manager->getMode();
    input_manager->setMode(InputManager::MENU);
    
    createButtons();
    assignButtons(BUTTONS_LOWER);
    
    Widget* button_widget = getWidget<ButtonWidget>("Back");
    assert(button_widget != NULL);
    m_back_button = button_widget->getIrrlichtElement<IGUIButton>();
}   // init

// ----------------------------------------------------------------------------
/** Creates all button widgets
 */
void ScreenKeyboard::createButtons()
{
    int rows_num = layout_proportions.size();
    int pos_y = 3;

    const int margin = 2;
    int height = (m_area.getHeight() - 2 * pos_y) / rows_num - margin;
    char height_str[100];
    sprintf(height_str, "%i", height);

    for (int i = 0; i < rows_num; i++)
    {
        float pos_x = 3;
        int total_width = m_area.getWidth() - 2 * (int)pos_x;

        char tmp[100];
        sprintf(tmp, "%i", pos_y + (height + margin) * i);
        std::string pos_y_str = tmp;
        
        int total_proportions = 0;
        
        for (int value : layout_proportions[i])
        {
            total_proportions += value;
        }
        
        int cols_num = layout_proportions[i].size();
        
        for (int j = 0; j < cols_num; j++)
        {
            float width = (float)total_width * layout_proportions[i][j] 
                                             / total_proportions - margin;
            
            char width_str[100];
            sprintf(width_str, "%i", (int)roundf(width));
            
            char tmp[100];
            sprintf(tmp, "%i", (int)roundf(pos_x));
            std::string pos_x_str = tmp;
            
            ButtonWidget* button = new ButtonWidget();
            button->setParent(m_irrlicht_window);
            button->m_properties[PROP_WIDTH] = width_str;
            button->m_properties[PROP_HEIGHT] = height_str;
            button->m_properties[PROP_X] = pos_x_str;
            button->m_properties[PROP_Y] = pos_y_str;
            m_widgets.push_back(button);
            m_buttons.push_back(button);
            
            pos_x += width + margin;
        }
    }

    LayoutManager::calculateLayout(m_widgets, this);
    addWidgetsRecursively(m_widgets);
    
    assert(m_buttons.size() > 0);
    m_buttons[0]->setFocusForPlayer(PLAYER_ID_GAME_MASTER);
}   // createButtons

// ----------------------------------------------------------------------------

std::wstring ScreenKeyboard::getKeyName(std::string key_id)
{
    std::wstring key_name;
    
    if (key_id == "Enter")
    {
        key_name = L"\u23CE";
    }
    else if (key_id == "Shift")
    {
        key_name = L"\u21E7";
    }
    else if (key_id == "Back")
    {
        key_name = L"\u232B";
    }
    else if (key_id == "Space")
    {
        key_name = L"\u2423";
    }
    else
    {
        std::wstring tmp(key_id.begin(), key_id.end());
        key_name = tmp;
    }
    
    return key_name;
}

// ----------------------------------------------------------------------------
/** A function that allows to select one of the available buttons layout
 *  \param buttons_type One of the available buttons type
 */
void ScreenKeyboard::assignButtons(ButtonsType buttons_type)
{
    int rows_num = layout_proportions.size();
    
    m_buttons_type = buttons_type;
    
    KeyboardLayout* keys = NULL;
    
    switch (buttons_type)
    {
    case BUTTONS_LOWER:
        keys = &layout_lower;
        break;
    case BUTTONS_UPPER:
        keys = &layout_upper;
        break;
    case BUTTONS_DIGITS:
        keys = &layout_digits;
        break;
    case BUTTONS_DIGITS2:
        keys = &layout_digits2;
        break;
    default:
        keys = NULL;
        break;
    };
    
    unsigned int current_button_id = 0;
    
    for (int i = 0; i < rows_num; i++)
    {
        int cols_num = layout_proportions[i].size();

        for (int j = 0; j < cols_num; j++)
        {
            std::string key = keys != NULL ? (*keys)[i][j] : "?";
            std::wstring key_name = getKeyName(key);

            assert(current_button_id < m_buttons.size());
            ButtonWidget* button = m_buttons[current_button_id];
            
            if (key == "Separator")
            {
                button->setVisible(false);
            }
            else
            {
                button->setVisible(true);
                button->setText(key_name.c_str());
                button->m_properties[PROP_ID] = key;
            }
            
            current_button_id++;
        }
    }
}   // assignButtons

// ----------------------------------------------------------------------------

void ScreenKeyboard::onUpdate(float dt)
{
    if (m_back_button->isPressed() || m_back_button_pressed)
    {
        const unsigned int repeat_rate = 40;
        const unsigned int repeat_delay = 400;
        
        SEvent event;
        event.KeyInput.Key = IRR_KEY_BACK;
        event.KeyInput.Char = 0;
        event.EventType = EET_KEY_INPUT_EVENT;
        event.KeyInput.PressedDown = true;
        event.KeyInput.Control = false;
        event.KeyInput.Shift = false;
        
        if (m_repeat_time == 0)
        {
            m_edit_box->OnEvent(event);
        }

        while (m_repeat_time > repeat_delay + repeat_rate)
        {
            m_edit_box->OnEvent(event);
            m_repeat_time -= repeat_rate;
        }
        
        m_repeat_time += (unsigned int)(dt * 1000);
    }
    
    if (!m_back_button->isPressed())  
    {
        m_back_button_pressed = false;
        m_repeat_time = 0;
    }
}

// ----------------------------------------------------------------------------
/** A function that handles irrlicht events
 *  \param event Irrlicht event
 *  \return Block event if true
 */
bool ScreenKeyboard::onEvent(const SEvent &event)
{
    if (event.EventType == EET_MOUSE_INPUT_EVENT)
    {
        core::position2d<s32> point(event.MouseInput.X, event.MouseInput.Y);
        
        if (m_edit_box->isPointInside(point))
        {
            m_edit_box->OnEvent(event);
            return true;
        }
        else
        {
            bool is_point_inside = m_irrlicht_window->isPointInside(point);
            
            if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN)
            {
                if (!is_point_inside)
                {
                    m_schedule_close = true;
                    return true;
                }
            }
            else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP)
            {
                if (!is_point_inside && m_schedule_close)
                {
                    dismiss();
                    return true;
                }
                else
                {
                    m_schedule_close = false;
                    return false;
                }
            }
        }
    }
    
    return false;
}

// ----------------------------------------------------------------------------
/** A function that handles buttons events
 *  \param eventSource Button ID
 *  \return Block event if edit box is assigned
 */
EventPropagation ScreenKeyboard::processEvent(const std::string& eventSource)
{
    if (m_edit_box == NULL)
        return EVENT_LET;
        
    SEvent event;
    bool send_event = false;
    bool close_keyboard = false;
        
    if (eventSource == "Shift")
    {
        switch (m_buttons_type)
        {
        case BUTTONS_UPPER:
            assignButtons(BUTTONS_LOWER);
            break;
        case BUTTONS_LOWER:
            assignButtons(BUTTONS_UPPER);
            break;
        case BUTTONS_DIGITS:
            assignButtons(BUTTONS_DIGITS2);
            break;
        case BUTTONS_DIGITS2:
            assignButtons(BUTTONS_DIGITS);
            break;
        default:
            break;
        }
    }
    else if (eventSource == "123")
    {
        assignButtons(BUTTONS_DIGITS);
    }
    else if (eventSource == "Text")
    {
        assignButtons(BUTTONS_LOWER);
    }
    else if (eventSource == "Enter")
    {
        event.KeyInput.Key = IRR_KEY_RETURN;
        event.KeyInput.Char = 0;
        send_event = true;
        close_keyboard = true;
    }
    else if (eventSource == "Back")
    {
        send_event = false;
        m_back_button_pressed = true;
    }
    else if (eventSource == "Space")
    {
        event.KeyInput.Key = IRR_KEY_UNKNOWN;
        event.KeyInput.Char = ' ';
        send_event = true;
    }
    else if (eventSource.size() > 0)
    {
        event.KeyInput.Key = IRR_KEY_UNKNOWN;
        event.KeyInput.Char = eventSource.at(0);
        send_event = true;
    }

    if (send_event)
    {
        event.EventType = EET_KEY_INPUT_EVENT;
        event.KeyInput.PressedDown = true;
        event.KeyInput.Control = false;
        event.KeyInput.Shift = false;

        m_edit_box->OnEvent(event);
    }
    
    if (close_keyboard)
    {
        dismiss();
    }

    return EVENT_BLOCK;
}   // processEvent

// ----------------------------------------------------------------------------
/** A function that closes the keyboard
 */
void ScreenKeyboard::dismiss()
{
    delete m_screen_keyboard;
    m_screen_keyboard = NULL;
}   // dismiss

// ----------------------------------------------------------------------------
/** A function that handles escape pressed event
 */
bool ScreenKeyboard::onEscapePressed()
{
    dismiss();
    return true;
}   // onEscapePressed

// ----------------------------------------------------------------------------
/** A function that determines if (native) screen keyboard should be activated
 */
bool ScreenKeyboard::shouldUseScreenKeyboard()
{
    bool always_use_screen_keyboard =
        UserConfigParams::m_screen_keyboard == 2;

    // Enable if no hardware keyboard
    if (UserConfigParams::m_screen_keyboard == 1)
    {
        if (irr_driver->getDevice()->hasHardwareKeyboard())
            return false;
        return true;
    }

    return always_use_screen_keyboard;
}
