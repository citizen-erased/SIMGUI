#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <map>
#include <algorithm>

#include <boost/lexical_cast.hpp>

#include <cml/cml.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include "gui_draw.h"
#include "gui.h"


GUI_AABB GUI_AABB::fromPositionSize(float x, float y, float w, float h)
{
    return GUI_AABB(x, y, x+w, y+h);
}

GUI_AABB GUI_AABB::fromMinMax(float x1, float y1, float x2, float y2)
{
    return GUI_AABB(x1, y1, x2, y2);
}

GUI_AABB::GUI_AABB()
{
    min.zero();
    max.zero();
}

GUI_AABB::GUI_AABB(float x1, float y1, float x2, float y2)
{
    min[0] = std::min(x1, x2);
    min[1] = std::min(y1, y2);
    max[0] = std::max(x1, x2);
    max[1] = std::max(y1, y2);
}

GUI_AABB::GUI_AABB(cml::vector2f min, cml::vector2f max)
{
    this->min = min;
    this->max = max;
}

bool GUI_AABB::containsPoint(float x, float y) const
{
    return x >= min[0] && x <= max[0] && y >= min[1] && y <= max[1];
}

bool GUI_AABB::intersects(const GUI_AABB &o) const
{
    return intersects(o.min[0], o.min[1], o.max[0], o.max[1]);
}

bool GUI_AABB::intersects(float x1, float y1, float x2, float y2) const
{
    return !(max[0] < x1 || min[0] > x2 || max[1] < y1 || min[1] > y2);
}

GUI_AABB GUI_AABB::intersection(const GUI_AABB &o) const
{
    cml::vector2f minp, maxp;

    minp[0] = cml::clamp(o.min[0], min[0], max[0]);
    maxp[0] = cml::clamp(o.max[0], min[0], max[0]);
    minp[1] = cml::clamp(o.min[1], min[1], max[1]);
    maxp[1] = cml::clamp(o.max[1], min[1], max[1]);

    return GUI_AABB(minp, maxp);
}

float GUI_AABB::getWidth() const
{
    return max[0] - min[0];
}

float GUI_AABB::getHeight() const
{
    return max[1] - min[1];
}


GUIEditBoxData::GUIEditBoxData()
{
    caret_pos = 0;
    selection = 0;
    offset = 0.0f;
    str_ptr = NULL;
}

GUIEditBoxData::GUIEditBoxData(std::string *s)
{
    caret_pos = 0;
    selection = 0;
    offset = 0.0f;
    str_ptr = s;
}

GUIEditBoxData::GUIEditBoxData(const std::string &s)
{
    caret_pos = 0;
    selection = 0;
    offset = 0.0f;
    str = s;
}



void GUIDirContents::add(const std::string &name, const std::string type, const std::string size, bool is_file)
{
    names.push_back(name);
    types.push_back(type);
    sizes.push_back(size);
    dirfile.push_back(is_file ? "f" : "d");
    selected.push_back(false);
}

void GUIDirContents::clear()
{
    names.clear();
    types.clear();
    sizes.clear();
    dirfile.clear();
    selected.clear();
}

void GUIDirContents::sort()
{

}

GUIFileChooserData::GUIFileChooserData()
{
    scrolls.zero();
    getDirContents = NULL;
    exists = NULL;
    isFile = NULL;
    isDir = NULL;
    is_save = false;
}

std::string GUIFileChooserData::getFile()
{
    return file_edit_data.str;
}

std::vector<std::string> GUIFileChooserData::getSelectedFiles()
{
    std::vector<std::string> strs;
    for(size_t i = 0; i < contents.names.size(); i++)
        if(contents.selected[i] && contents.dirfile[i] == "f")
            strs.push_back(contents.names[i]);
    return strs;
}

std::vector<std::string> GUIFileChooserData::getSelectedDirs()
{
    std::vector<std::string> strs;
    for(size_t i = 0; i < contents.names.size(); i++)
        if(contents.selected[i] && contents.dirfile[i] == "d")
            strs.push_back(contents.names[i]);
    return strs;
}



struct GUIMouseState
{
    float x, y;
    float dx, dy;
    float left_down_x, left_down_y;
    float right_down_x, right_down_y;
    int wheel_delta;
    bool left_just_pressed, left_just_released;
    bool right_just_pressed, right_just_released;
    bool left_down;
    bool right_down;
    bool dragged;
};

struct GUIKeyboardState
{
    bool is_key_typed;
    int key_typed;
    sf::Key::Code key_pressed;
    bool alt, control, shift;
};


extern sf::RenderWindow window;

static GUIMouseState mouse;
static GUIKeyboardState keyboard;

static std::string hot_widget;
static std::string active_widget;

static int active_widget_layer;
static int hot_widget_layer;
static bool is_mouse_in;

static int layer;

/*
 * The current pass the GUI is performing. One of GUI_PASS_* enum values
 */
static int pass;


/*
 * Stores all pushed desired clipping rectangles and the current one. The
 * current desired clipping rectangle is always clip_stack.top. clip_rect might
 * be different from the desired clipping rectangle since it gets clipped to
 * the screen_rect.
 */
static std::stack<GUI_AABB> clip_stack;

/*
 * The current clipping rectangle to actually use.
 */
static GUI_AABB clip_rect;

/*
 * The size of the virtual screen the GUI thinks it's on. Shouldn't be set
 * between GUI_BeginPass() and GUI_EndPass() pairs.
 */
static GUI_AABB screen_rect;


static std::stack<cml::vector2f> mouse_pos_stack;
static std::stack<cml::vector2f> w_offset_stack;
static cml::vector2f w_offset;

static std::stack<int> layer_stack;

static bool in_drop_menu = false;
static bool drop_menu_mouse_in = false;

static int button_mode;
static std::stack<int> button_mode_stack;

static float listbox_item_height = 20.0f;

static int event_bits = 0;

static GUIFont font;


void setClipRect(const GUI_AABB &clip);



bool mouseIn(int x, int y, int w, int h)
{
    float mx = mouse.x;
    float my = mouse.y;

    /*
     * mouse must be in the current group's clip rect. clip_rect is in gui
     * screen space so it must be transformed to local space first.
     */
    GUI_AABB local_clip_rect(clip_rect.min - w_offset, clip_rect.max - w_offset);
    if(!local_clip_rect.containsPoint(mx, my))
        return false;

    return mx >= x && mx <= x+w && my >= y && my <= y+h;
}

void printClipRect()
{
    std::cout << "clip_rect: " << clip_rect.min[0] << ", " << clip_rect.min[1] << ", " << clip_rect.max[0] << ". " << clip_rect.max[1] << std::endl;
}

void genericHotActive(const std::string &id, float x, float y, float w, float h)
{
    bool mouse_in = mouseIn(x, y, w, h);

    if(mouse_in && layer >= hot_widget_layer)
    {
        hot_widget = id;
        hot_widget_layer = layer;
        is_mouse_in = true;
    }

    if(hot_widget == id && mouse.left_just_pressed)
        active_widget = id;
}

void hotTest(const std::string &id, float x, float y, float w, float h)
{
    bool mouse_in = mouseIn(x, y, w, h);

    if(mouse_in && layer >= hot_widget_layer)
    {
        hot_widget = id;
        hot_widget_layer = layer;
        is_mouse_in = true;
    }
}




void GUI_Init()
{
    clip_stack = std::stack<GUI_AABB>();
    clip_rect = GUI_AABB(-1000000.0f, -1000000.0f, 1000000.0f, 1000000.0f);
    screen_rect = GUI_AABB(0.0f, 0.0f, 640.0f, 480.0f);

    GUI_PushClipRect(clip_rect);

    mouse.x = mouse.y = mouse.dx = mouse.dy = 0;
    mouse.left_down_x = mouse.left_down_y = mouse.right_down_x = mouse.right_down_y = 0;
    mouse.wheel_delta = 0;
    mouse.left_down = mouse.right_down = false;
    mouse.left_just_pressed = mouse.right_just_pressed = false;
    mouse.left_just_released = mouse.right_just_released = false;
    mouse.dragged = false;

    keyboard.is_key_typed = false;
    keyboard.key_typed = 0;
    keyboard.key_pressed = sf::Key::Count;
    keyboard.control = keyboard.alt = keyboard.shift = false;

    hot_widget = "";
    active_widget = "";
    active_widget_layer = 0;
    hot_widget_layer = 0;
    layer = 0;
    pass = GUI_PASS_DRAW;

    button_mode = GUI_ACTIVATE_ON_UP;
    button_mode_stack = std::stack<int>();

    font.valid = false;

    is_mouse_in = false;

    GUI_DrawSetButtonMode(GUI_BUTTON_DRAW_STRING);
}

void GUI_BeginPass(int p)
{
    pass = p;
    GUI_SetLayer(0);

    if(pass == GUI_PASS_EVENT)
    {
        is_mouse_in = false;
    }
    else if(pass == GUI_PASS_DRAW)
    {
        GUI_DrawBegin();
    }
}

void GUI_EndPass()
{
    if(pass == GUI_PASS_DRAW)
    {
        GUI_DrawEnd();
    }

    pass = GUI_PASS_NONE;
}

void GUI_ScreenBounds(float x, float y, float w, float h)
{
    screen_rect.min.set(x, y);
    screen_rect.max.set(cml::clamp(w, 0.0f, 1000000.0f), cml::clamp(h, 0.0f, 1000000.0f));
    setClipRect(clip_rect);
}

void setClipRect(const GUI_AABB &clip)
{
    clip_rect = clip.intersection(screen_rect);
    //std::cout << "clip_rect: " << clip_rect.min << " " << clip_rect.max << std::endl;

    if(pass == GUI_PASS_DRAW)
    {
        float x = clip_rect.min[0];
        float w = clip_rect.max[0] - clip_rect.min[0];
        float h = clip_rect.max[1] - clip_rect.min[1];
        float y = window.GetHeight() - clip_rect.max[1];
        GUI_DrawSetClipRect(x, y, w, h);
    }
}

void GUI_PushClipRect(const GUI_AABB &clip)
{
    /*
     * store the requested clip rect at the top of the stack and set the
     * clipping rectangle. The last requested clip rect is stored since
     * the actual clipping rectangle needs to be recomputed if the screen
     * size changes.
     */
    clip_stack.push(clip);
    setClipRect(clip);
}

void GUI_PopClipRect()
{
    if(clip_stack.size() > 1)
    {
        clip_stack.pop();
        setClipRect(clip_stack.top());
    }
    else
        std::cout << "clip_stack empty\n";
}

void GUI_Font(sf::Font *sf_font, int size, const cml::vector4f &color)
{
    if(sf_font == NULL)
    {
        font.valid = false;
    }
    else
    {
        font.sf_font = sf_font;
        font.size = size;
        font.color = color;
        font.valid = true;
    }

    GUI_DrawFont(sf_font, size, color);
}

const GUIFont& GUI_GetFont()
{
    return font;
}

void GUI_BeginGroup(float x, float y, float w, float h)
{
    float wx = w_offset[0];
    float wy = w_offset[1];

    GUI_PushClipRect(GUI_AABB::fromPositionSize(wx + x, wy + y, w, h));
    GUI_PushTranslation();
    GUI_Translate(x, y);
}

void GUI_EndGroup()
{
    if(clip_stack.empty())
        return;

    GUI_PopTranslation();
    GUI_PopClipRect();
}

void GUI_Translate(float x, float y)
{
    w_offset[0] += x;
    w_offset[1] += y;

    mouse.x -= x;
    mouse.y -= y;

    if(pass == GUI_PASS_DRAW) GUI_DrawTranslate(x, y);
}

void GUI_PushTranslation()
{
    w_offset_stack.push(w_offset);
    mouse_pos_stack.push(cml::vector2f(mouse.x, mouse.y));
    if(pass == GUI_PASS_DRAW) GUI_DrawPushTranslation();
}

void GUI_PopTranslation()
{
    w_offset = w_offset_stack.top();
    w_offset_stack.pop();

    cml::vector2f mpos = mouse_pos_stack.top();
    mouse_pos_stack.pop();

    mouse.x = mpos[0];
    mouse.y = mpos[1];

    if(pass == GUI_PASS_DRAW) GUI_DrawPopTranslation();
}


void GUI_ButtonMode(int mode)
{
    button_mode = mode;
}

void GUI_PushButtonMode()
{
    button_mode_stack.push(button_mode);
}

void GUI_PopButtonMode()
{
    GUI_ButtonMode(button_mode_stack.top());
    button_mode_stack.pop();
}


/*
 * Called when an event has been given. Resets per-event variables; things that
 * need to be computed during the EVENT pass (e.g. id of the hot widget and
 * variables that only last for the duration of one event (e.g. button just
 * pressed, mouse dx dy).
 */
void setEventState()
{
    hot_widget = "";
    hot_widget_layer = 0;

    GUIMouseState &m = mouse;
    m.wheel_delta = 0;
    m.left_just_pressed = m.right_just_pressed = false;
    m.left_just_released = m.right_just_released = false;
    m.dx = m.dy = 0.0f;
    mouse.dragged = false;

    keyboard.is_key_typed = false;
    keyboard.key_typed = 0;
    keyboard.key_pressed = sf::Key::Count;
}

void GUI_MouseButton(sf::Mouse::Button button, bool down)
{
    setEventState();

    GUIMouseState &m = mouse;

    m.left_just_pressed = m.right_just_pressed = false;
    m.left_just_released = m.right_just_released = false;

    if(button == sf::Mouse::Left)
    {
        if(down && !m.left_down)
        {
            m.left_down = true;
            m.left_just_pressed = true;
            m.left_down_x = mouse.x;
            m.left_down_y = mouse.y;
        }
        else if(!down && m.left_down)
        {
            m.left_down = false;
            m.left_just_released = true;
        }
    }
    else if(button == sf::Mouse::Right)
    {
        if(down && !m.right_down)
        {
            m.right_down = true;
            m.right_just_pressed = true;
            m.right_down_x = mouse.x;
            m.right_down_y = mouse.y;
        }
        else if(!down && m.right_down)
        {
            m.right_down = false;
            m.left_just_released = true;
        }
    }
}

void GUI_MouseMove(float x, float y)
{
    setEventState();

    mouse.dx = x - mouse.x;
    mouse.dy = y - mouse.y;
    mouse.x = x;
    mouse.y = y;
    hot_widget = "";

    if(mouse.left_down || mouse.right_down)
        mouse.dragged = true;
}

void GUI_MouseWheel(int delta)
{
    setEventState();

    mouse.wheel_delta = delta;
}

void GUI_KeyPressed(sf::Key::Code key, bool control, bool alt, bool shift)
{
    setEventState();

    keyboard.key_pressed = key;
    keyboard.alt = alt;
    keyboard.control = control;
    keyboard.shift = shift;
}

void GUI_KeyTyped(int key)
{
    setEventState();

    keyboard.is_key_typed = true;
    keyboard.key_typed = key;
}



void GUI_SetLayer(int new_layer)
{
    layer = new_layer;
    if(pass == GUI_PASS_DRAW) GUI_DrawSetLayer(new_layer);
}

void GUI_PushLayer()
{
    layer_stack.push(layer);
}

void GUI_PopLayer()
{
    GUI_SetLayer(layer_stack.top());
    layer_stack.pop();
}

void GUI_Label(const std::string &id, int x, int y, int w, int h, const std::string &str)
{
    if(pass == GUI_PASS_DRAW)
    {
        GUI_DrawLabel(x, y, w, h, str);
    }
}

bool GUI_Button(const std::string &id, int x, int y, int w, int h, const std::string &str)
{
    bool event = false;

    if(pass == GUI_PASS_EVENT)
    {
        genericHotActive(id, x, y, w, h);
    }
    else if(pass == GUI_PASS_RESPONSE)
    {
        if(button_mode == GUI_ACTIVATE_ON_UP && mouse.left_just_released)
        {
            if(active_widget == id && hot_widget == id)
            {
                event = true;
                active_widget = "";
            }
            else if(active_widget == id)
            {
                active_widget = "";
            }
        }
        else if(button_mode == GUI_ACTIVATE_ON_DOWN && mouse.left_just_pressed)
        {
            if(hot_widget == id)
            {
                event = true;
                active_widget = "";
            }
        }
    }
    else if(pass == GUI_PASS_DRAW)
    {
        GUI_DrawButton(x, y, w, h, hot_widget == id, active_widget == id, str);
    }


    return event;
}

bool GUI_ToggleButton(const std::string &id, int x, int y, int w, int h, const std::string &str, bool *value)
{
    if(pass == GUI_PASS_DRAW)
    {
        GUI_DrawButton(x, y, w, h, hot_widget == id, *value, str);
        return false;
    }

    bool evt = GUI_Button(id, x, y, w, h, str);

    if(evt)
        *value = !*value;

    return evt;
}

bool GUI_Checkbox(const std::string &id, float x, float y, float w, float h, bool *value)
{
    bool evt = false;

    if(pass != GUI_PASS_DRAW)
        evt = GUI_ToggleButton(id, x, y, w, h, "", value);
    else
        GUI_DrawCheckbox(x, y, w, h, *value);

    return evt;
}

bool GUI_CheckboxLabelled(const std::string id, float x, float y, float w, float h, const std::string &label, bool *value)
{
    bool evt = false;

    if(pass != GUI_PASS_DRAW)
        evt = GUI_ToggleButton(id, x, y, w, h, "", value);
    else
        GUI_DrawCheckboxLabelled(x, y, w, h, x, y, h, h, id == hot_widget, label, *value);

    return evt;
}

bool GUI_Slider(const std::string &id, float x, float y, float w, float h, int type, int min, int max, int page_size, int *value)
{
    bool evt = false;

    if(min > max) std::swap(min, max);

    float range = max - min;
    page_size = cml::clamp(page_size, 1, int(range));
    float move_range = max - min - page_size;
    float thumb_scale = float(page_size) / range;

    float thumb_x, thumb_y, thumb_w, thumb_h;

    if(type == GUI_HORIZONTAL)
    {
        thumb_w = w * thumb_scale;
        thumb_x = x + w * (float(*value) / range);
        thumb_h = h;
        thumb_y = y;
    }
    else
    {
        thumb_h = h * thumb_scale;
        thumb_y = y + h * (float(*value) / range);
        thumb_x = x;
        thumb_w = w;
    }


    if(pass == GUI_PASS_DRAW)
    {
        GUI_DrawSlider(x, y, w, h, thumb_x, thumb_y, thumb_w, thumb_h); 
    }

    if(pass == GUI_PASS_EVENT)
    {
        genericHotActive(id, x, y, w, h);
    }
    else if(pass == GUI_PASS_RESPONSE)
    {
        if(active_widget == id && mouse.left_down && mouse.dragged)
        {
            float rel;

            if(type == GUI_HORIZONTAL)
                rel = cml::clamp((mouse.x-x - thumb_w/2.0f) / (w - thumb_w), 0.0f, 1.0f);
            else
                rel = cml::clamp((mouse.y-y - thumb_h/2.0f) / (h - thumb_h), 0.0f, 1.0f);

            int new_value = cml::clamp(int(move_range * rel), min, max-page_size);

            if(new_value != *value)
            {
                *value = new_value;
                evt = true;
            }
        }
    }

    return evt;
}

bool GUI_Scrollbar(const std::string &id, float x, float y, float w, float h, int type, int min, int max, int page_size, int *value)
{
    int old_value = *value;
    int button_mode = GUI_DrawGetButtonMode();
    GUI_DrawSetButtonMode(GUI_BUTTON_DRAW_ARROW);

    GUI_PushButtonMode();
    GUI_ButtonMode(GUI_ACTIVATE_ON_DOWN);
    GUI_BeginGroup(x, y, w, h);
        if(type == GUI_HORIZONTAL)
        {
            float b_size = h;
            GUI_Slider(id + "/_s", b_size, 0.0f, w-b_size-b_size, h, type, min, max, page_size, value);
            
            if(GUI_Button(id + "/_dec", 0.0f, 0.0f, w, b_size, "l"))
                *value = cml::clamp(*value - 1, min, max-page_size);
            if(GUI_Button(id + "/_inc", w-b_size, 0.0f, w, b_size, "r"))
                *value = cml::clamp(*value + 1, min, max-page_size);
        }
        else
        {
            float b_size = w;
            GUI_Slider(id + "/_s", 0.0f, b_size, w, h-b_size-b_size, type, min, max, page_size, value);
            
            if(GUI_Button(id + "/_dec", 0.0f, 0.0f, w, b_size, "u"))
                *value = cml::clamp(*value - 1, min, max-page_size);
            if(GUI_Button(id + "/_inc", 0.0f, h-b_size, w, b_size, "d"))
                *value = cml::clamp(*value + 1, min, max-page_size);
        }
    GUI_EndGroup();
    GUI_PopButtonMode();
    GUI_DrawSetButtonMode(button_mode);

    return old_value != *value;
}

void GUI_BeginFrame(float x, float y, float w, float h, float padding_x, float padding_y)
{
    if(pass == GUI_PASS_DRAW)
        GUI_DrawFrame(x, y, w, h, padding_x, padding_y);

    GUI_BeginGroup(x+padding_x, y+padding_y, w-padding_x*2.0f, h-padding_y*2.0f);
}

void GUI_EndFrame()
{
    GUI_EndGroup();
}

void GUI_Frame(float x, float y, float w, float h)
{
    if(pass == GUI_PASS_DRAW)
    {
        //FIXME get rid of this
        window.Draw(sf::Shape::Line(x,   y,   x+w, y,   2.0f, sf::Color::White));
        window.Draw(sf::Shape::Line(x,   y+h, x+w, y+h, 2.0f, sf::Color::White));
        window.Draw(sf::Shape::Line(x,   y,   x,   y+h, 2.0f, sf::Color::White));
        window.Draw(sf::Shape::Line(x+w, y,   x+w, y+h, 2.0f, sf::Color::White));
    }
}

bool GUI_Listbox(const std::string &id, float x, float y, float w, float h, const std::vector<std::string> &data, int data_offset, int *choice)
{
    bool event = false;

    if(pass == GUI_PASS_EVENT)
    {
        genericHotActive(id, x, y, w, h);
    }
    else if(pass == GUI_PASS_RESPONSE)
    {
        if(hot_widget == id && mouse.left_just_pressed)
        {
            int item = (mouse.y - y) / listbox_item_height;
            item = cml::clamp(item + data_offset, 0, (int)data.size()-1);

            if(0 <= item && item < data.size() && item != *choice)
            {
                *choice = item;
                event = true;
            }
        }
    }
    else if(pass == GUI_PASS_DRAW)
    {
        GUI_BeginGroup(x, y, w, h);
        GUI_DrawListbox(0.0f, 0.0f, w, h, listbox_item_height, data, data_offset, *choice);
        GUI_EndGroup();
    }

    return event;
}

bool GUI_ScrolledListbox(const std::string &id, float x, float y, float w, float h, const std::vector<std::string> &data, int *choice, cml::vector2i *scroll)
{
    event_bits = 0;

    int items_on_screen = h/listbox_item_height;

    GUI_BeginGroup(x, y, w, h);
        if(GUI_Scrollbar(id+"/_0", w-16, 0, 16, h, GUI_VERTICAL, 0, data.size(), items_on_screen, &((*scroll)[1])))
            event_bits |= GUI_EVT_SCROLLED;

        if(GUI_Listbox(id+"/_1", 0, 0, w-16, h, data, (*scroll)[1], choice))
            event_bits |= GUI_EVT_CHOICE;
    GUI_EndGroup();

    if(pass == GUI_PASS_RESPONSE)
    {
        if(hot_widget == id+"/_1")
        {
            if(mouse.wheel_delta > 0)
                (*scroll)[1] = cml::clamp((*scroll)[1] - 1, 0, std::max(0, (int)data.size()-items_on_screen));
            else if(mouse.wheel_delta < 0)
                (*scroll)[1] = cml::clamp((*scroll)[1] + 1, 0, std::max(0, (int)data.size()-items_on_screen));

            if(mouse.wheel_delta != 0)
                event_bits |= GUI_EVT_SCROLLED;
        }
    }

    return event_bits != 0;
}

bool GUI_ListboxMulti(const std::string &id, float x, float y, float w, float h, const std::vector<std::string> &data, int data_offset, std::vector<bool> *selected)
{
    bool event = false;

    if(pass == GUI_PASS_EVENT)
    {
        genericHotActive(id, x, y, w, h);
    }
    else if(pass == GUI_PASS_RESPONSE)
    {
        if(hot_widget == id && mouse.left_just_pressed && !data.empty())
        {
            int item = (mouse.y - y) / listbox_item_height;
            item = cml::clamp(item + data_offset, 0, (int)data.size()-1);

            if(keyboard.control)
            {
                (*selected)[item] = !(*selected)[item];
            }
            else if(keyboard.shift)
            {
                //TODO proper shift selection. will probably need to store the last selected item
#if 0
                for(size_t i = item; i < selected->size(); i++)
                {
                    if(!(*selected)[i])
                        (*selected)[i] = true;
                    else
                        break;
                }

                if(item > 0) // size_t will wrap otherwise
                {
                    for(size_t i = item-1; i >= 0; i--)
                    {
                        if(!(*selected)[i])
                            (*selected)[i] = true;
                        else
                            break;
                    }
                }
#endif
            }
            else
            {
                for(size_t i = 0; i < selected->size(); i++)
                    (*selected)[i] = false;
                (*selected)[item] = true;
            }

            event = true;
        }
        else if(keyboard.control && keyboard.key_pressed == sf::Key::A && hot_widget == id)
        {
            for(size_t i = 0; i < selected->size(); i++)
                (*selected)[i] = true;
            event = true;
        }
    }
    else if(pass == GUI_PASS_DRAW)
    {
        GUI_BeginGroup(x, y, w, h);
        GUI_DrawListboxMulti(0.0f, 0.0f, w, h, listbox_item_height, data, data_offset, *selected);
        GUI_EndGroup();
    }

    return event;
}

bool GUI_ScrolledListboxMulti(const std::string &id, float x, float y, float w, float h, const std::vector<std::string> &data, std::vector<bool> *selected, cml::vector2i *scroll)
{
    event_bits = 0;

    int items_on_screen = h/listbox_item_height;

    GUI_BeginGroup(x, y, w, h);
        if(GUI_Scrollbar(id+"/_0", w-16, 0, 16, h, GUI_VERTICAL, 0, data.size(), items_on_screen, &((*scroll)[1])))
            event_bits |= GUI_EVT_SCROLLED;

        if(GUI_ListboxMulti(id+"/_1", 0, 0, w-16, h, data, (*scroll)[1], selected))
            event_bits |= GUI_EVT_CHOICE;
    GUI_EndGroup();

    if(pass == GUI_PASS_RESPONSE)
    {
        if(hot_widget == id+"/_1")
        {
            if(mouse.wheel_delta > 0)
                (*scroll)[1] = cml::clamp((*scroll)[1] - 1, 0, std::max(0, (int)data.size()-items_on_screen));
            else if(mouse.wheel_delta < 0)
                (*scroll)[1] = cml::clamp((*scroll)[1] + 1, 0, std::max(0, (int)data.size()-items_on_screen));

            if(mouse.wheel_delta != 0)
                event_bits |= GUI_EVT_SCROLLED;
        }
    }

    return event_bits != 0;
}

void GUI_BeginScrollArea(const std::string &id, float x, float y, float w, float h, float scroll_size, int min_scroll_x, int max_scroll_x, int min_scroll_y, int max_scroll_y, cml::vector2i *scroll)
{
    GUI_Slider(id+"/_0", x+w-scroll_size, y, scroll_size, h-scroll_size, GUI_HORIZONTAL, min_scroll_x, max_scroll_x, w, &((*scroll)[0]));
    GUI_Slider(id+"/_1", x, y+h-scroll_size, w-scroll_size, scroll_size, GUI_HORIZONTAL, min_scroll_y, max_scroll_y, h, &((*scroll)[1]));

    GUI_BeginGroup(x, y, w-scroll_size, h-scroll_size);
    GUI_PushTranslation();
    GUI_Translate(0, -(*scroll)[1]);
}

void GUI_EndScrollArea()
{
    GUI_PopTranslation();
    GUI_EndGroup();
}





static float popup_y_coord;
static float popup_item_width;
static float popup_item_height;
static bool mouse_in_popup_group;
static PopupNode *popup_group_root = NULL;
static PopupNode *popup_root = NULL;

void deactivateChildrenRecursive(PopupNode *node)
{
    for(unsigned int i = 0; i < node->children.size(); i++)
    {
        node->children[i]->active = false;
        deactivateChildrenRecursive(node->children[i]);
    }
}

void deactivateTree(PopupNode *node)
{
    node->active = false;
    deactivateChildrenRecursive(node);
}

void deactivateRootChildren(PopupNode *node)
{
    PopupNode *root = node;

    while(root->parent != NULL)
        root = root->parent;

    deactivateChildrenRecursive(root); 
}

void activateToRoot(PopupNode *node)
{
    PopupNode *current_node = node;

    while(current_node != NULL)
    {
        current_node->active = true;
        current_node = current_node->parent;
    }
}

void GUI_BeginPopupGroup(PopupNode *root)
{
    popup_group_root = root;
    mouse_in_popup_group = false;

    GUI_PushLayer();
    GUI_SetLayer(GUI_MAX_LAYER + 1);
}

void GUI_EndPopupGroup()
{
    if(pass == GUI_PASS_RESPONSE)
    {
        bool click_outside_group = popup_group_root != NULL && !mouse_in_popup_group && mouse.left_just_pressed;
        bool mouse_in_menu = in_drop_menu && drop_menu_mouse_in;

        if(click_outside_group && !mouse_in_menu)
        {
            deactivateTree(popup_group_root);
        }
    }

    popup_group_root = NULL;
    GUI_PopLayer();
}

void GUI_BeginPopupMenu(const std::string &id, float x, float y, float w, float h, float item_h, PopupNode *root)
{
    popup_root = root;
    popup_y_coord = 0.0f;
    popup_item_width = w;
    popup_item_height = item_h;
    GUI_BeginGroup(x, y, w, h);

    if(pass == GUI_PASS_EVENT)
    {
        genericHotActive(id, 0.0f, 0.0f, w, h);
    }
    else if(pass == GUI_PASS_RESPONSE)
    {
        if(!mouse_in_popup_group)
            mouse_in_popup_group = mouseIn(0.0f, 0.0f, w, h);
    }
    else if(pass == GUI_PASS_DRAW)
    {
        GUI_DrawPopup(0.0f, 0.0f, w, h);
    }
}

void GUI_EndPopupMenu()
{
    GUI_EndGroup();
    popup_root = NULL;
}

bool GUI_PopupMenuButton(const std::string &id, const std::string &left_str, const std::string &right_str)
{
    float x = 0.0f;
    float y = popup_y_coord;
    float w = popup_item_width;
    float h = popup_item_height;
    bool evt = false;

    popup_y_coord += popup_item_height;

    if(pass != GUI_PASS_DRAW)
        evt = GUI_Button(id, x, y, w, h, "");

    if(pass == GUI_PASS_RESPONSE)
    {
        if(evt)
        {
            deactivateTree(popup_group_root);
        }
        else if(id == hot_widget)
        {
            deactivateChildrenRecursive(popup_root);
        }
    }
    else if(pass == GUI_PASS_DRAW)
    {
        GUI_DrawPopupButton(x, y, w, h, hot_widget == id, active_widget == id, left_str, right_str);
    }

    return evt;
}

void GUI_PopupSubMenuButton(const std::string &id, PopupNode *node)
{
    float x = 0.0f;
    float y = popup_y_coord;
    float w = popup_item_width;
    float h = popup_item_height;

    popup_y_coord += popup_item_height;

    if(pass == GUI_PASS_EVENT)
    {
        hotTest(id, x, y, w, h);
    }                                           
    else if(pass == GUI_PASS_RESPONSE)
    {
        if(hot_widget == id)
        {
            deactivateRootChildren(popup_group_root);
            activateToRoot(node);
        }
    }
    else if(pass == GUI_PASS_DRAW)
    {
        GUI_DrawPopupSubMenuButton(x, y, w, h, hot_widget == id, node->active, node->name.c_str());
    }
}

void GUI_PopupSeparator(float h)
{
    if(pass == GUI_PASS_DRAW)
    {
        GUI_DrawSeparator(0.0f, popup_y_coord, popup_item_width, h);
    }

    popup_y_coord += h;
}









void deactivateAllMenuItems(PopupNode *root)
{
    for(unsigned int i = 0; i < root->children.size(); i++)
        root->children[i]->active = false;
}

void setActiveMenuItem(PopupNode *root, unsigned int index)
{
    deactivateAllMenuItems(root);
    root->children[index]->active = true;
}

bool GUI_BeginDropMenu(const std::string &id, float x, float y, float w, float h, float item_width, PopupNode *root)
{
    in_drop_menu = true;

    GUI_PushTranslation();
    GUI_Translate(x, y);

    bool evt = false;
    bool menu_is_open = false;

    for(unsigned int i = 0; i < root->children.size(); i++)
    {
        std::string iid = id + "/_" + boost::lexical_cast<std::string>(i);
        float ix = x + item_width*i;
        float iy = y;
        float iw = item_width;
        float ih = h;

        if(pass != GUI_PASS_DRAW)
        {
            if(GUI_ToggleButton(iid, ix, iy, iw, ih, "", &root->children[i]->active))
            {
                evt = true;

                if(root->children[i]->active)
                    setActiveMenuItem(root, i);
                else
                    deactivateAllMenuItems(root);
            }
        }
        else
        {
            GUI_DrawDropMenuHeaderItem(ix, iy, iw, ih, hot_widget == iid, root->children[i]->active, root->children[i]->name);
        }
    }


    if(pass == GUI_PASS_EVENT)
    {
        drop_menu_mouse_in = mouseIn(0.0f, 0.0f, w, h);
    }
    else if(pass == GUI_PASS_RESPONSE)
    {
        for(unsigned int i = 0; i < root->children.size(); i++)
            if(root->children[i]->active)
                menu_is_open = true;

        if(menu_is_open)
        {
            for(unsigned int i = 0; i < root->children.size(); i++)
                if(hot_widget == id + "/_" + boost::lexical_cast<std::string>(i) && !root->children[i]->active)
                {
                    setActiveMenuItem(root, i);
                    evt = true;
                }
        }
    }
    else if(pass == GUI_PASS_DRAW)
    {
        //FIXME get rid of this
        //window.Draw(sf::Shape::Rectangle(w_offset[0], w_offset[1], w_offset[0]+w, w_offset[1]+h, sf::Color::White));

        for(unsigned int i = 0; i < root->children.size(); i++)
        {
            //GUI_DrawText(w_offset[0]+item_spacing*i, w_offset[1], root->children[i]->name.c_str());
        }
    }

    return evt;
}

void GUI_EndDropMenu()
{
    GUI_PopTranslation();

    in_drop_menu = false;
}
















static bool doEditBoxResponse(int &caret_pos, int &selection, std::string *str);
static void eraseSelection(int &caret_pos, int &selection, std::string *str);

bool GUI_EditBox(const std::string &id, float x, float y, float w, float h, GUIEditBoxData *data)
{
    return GUI_EditBox(id, x, y, w, h, &data->caret_pos, &data->selection, &data->offset, data->str_ptr != NULL ? data->str_ptr : &data->str);
}

bool GUI_EditBox(const std::string &id, float x, float y, float w, float h, int *caret_pos, int *selection, float *offset, std::string *str)
{
    float padding = 5.0f;
    bool event = false;
    event_bits = 0;

    *caret_pos = cml::clamp(*caret_pos, 0, (int)str->size());
    *selection = cml::clamp(*selection, -*caret_pos, (int)str->size() - *caret_pos);
    if(*offset < padding) *offset = padding;

    if(pass == GUI_PASS_EVENT)
    {
        genericHotActive(id, x, y, w, h);
    }
    else if(pass == GUI_PASS_RESPONSE)
    {
        if(active_widget == id)
            event = doEditBoxResponse(*caret_pos, *selection, str);

        if(font.valid)
        {
            sf::String txt(*str, *font.sf_font, font.size);
            sf::Vector2f caret_vec = txt.GetCharacterPos(*caret_pos);

            float caret_world = caret_vec.x + *offset;

            if(caret_world > w - 2.0f*padding)
                *offset = -caret_vec.x + w - padding;
            else if(caret_world < padding)
                *offset = -caret_vec.x + padding;

            if(txt.GetRect().GetWidth() < w - 2.0f*padding)
                *offset = padding;
        }
    }
    else if(pass == GUI_PASS_DRAW)
    {
        //FIXME the drawing code should set the clip rect to clip the text
        GUI_PushClipRect(GUI_AABB::fromPositionSize(w_offset[0]+x, w_offset[1]+y, w, h));
        GUI_DrawEditBox(x, y, w, h, active_widget == id, *caret_pos, *selection, *offset, *str);
        GUI_PopClipRect();
    }

    //std::cout << caret_pos << ", " << selection << ", " << str->size() << "\n";

    return event;
}

static void eraseSelection(int &caret_pos, int &selection, std::string *str)
{
    // work out for selection > 0
    int lower = caret_pos;
    int upper = caret_pos + selection;

    if(lower > upper) std::swap(lower, upper);

    str->erase(lower, upper-lower);
    selection = 0;
    caret_pos = lower;
}

bool doEditBoxResponse(int &caret_pos, int &selection, std::string *str)
{
    bool event = false;

    if(keyboard.is_key_typed)
    {
        if(' ' <= keyboard.key_typed && keyboard.key_typed <= '~')
        {
            std::cout << keyboard.key_typed << std::endl;

            if(selection != 0) eraseSelection(caret_pos, selection, str);

            str->insert(caret_pos, 1, (char)keyboard.key_typed);
            caret_pos++;
            event = true;
        }
    }

    switch(keyboard.key_pressed)
    {
        case sf::Key::Back:
        {
            if(!str->empty())
            {
                if(selection == 0)
                {
                    if(caret_pos > 0 && caret_pos <= str->size())
                    {
                        str->erase(caret_pos-1, 1);
                        caret_pos--;
                    }
                }
                else
                    eraseSelection(caret_pos, selection, str);

                return true;
            }
            break;
        }
        case sf::Key::Delete:
        {
            if(!str->empty())
            {
                if(selection == 0)
                {
                    if(caret_pos >= 0 && caret_pos < str->size())
                        str->erase(caret_pos, 1);
                }
                else
                    eraseSelection(caret_pos, selection, str);

                return true;
            }
            break;
        }
        case sf::Key::Return:
            event_bits |= GUI_EVT_CONFIRMED;
            return true;
            break;
        case sf::Key::Left:
        {
            int prev_caret_pos = caret_pos;
            caret_pos = std::max(caret_pos-1, 0);
            if(keyboard.shift && caret_pos != prev_caret_pos)
                selection++;
            else if(!keyboard.shift)
                selection = 0;
            break;
        }
        case sf::Key::Right:
        {
            int prev_caret_pos = caret_pos;
            caret_pos = std::min(caret_pos+1, (int)str->size());
            if(keyboard.shift && caret_pos != prev_caret_pos)
                selection--;
            else if(!keyboard.shift)
                selection = 0;
            break;
        }
        case sf::Key::A:
        {
            if(keyboard.control)
            {
                caret_pos = str->size();
                selection = -caret_pos;
            }
        }
    }

    return event;
}











bool GUI_DropList(const std::string &id, float x, float y, float w, float h, const std::vector<std::string> &data, int *choice, bool *open)
{
    bool evt = false;
    float frame_padding = 1;
    float lb_w = w - frame_padding*2.0f;
    float lb_h = 100 - frame_padding*2.0f; //TODO list heigth


    if(pass != GUI_PASS_DRAW)
        GUI_ToggleButton(id, x, y, w, h, "", open);

    if(*open)
    {
        //FIXME take in as a parameter
        static cml::vector2i lb_scrolls(0, 0);

        GUI_PushLayer();
        GUI_SetLayer(GUI_DROP_LIST_LAYER);

        GUI_BeginFrame(x, y+h, w, 100, frame_padding, frame_padding);
        if(GUI_ScrolledListbox(id + "/_list", 0.0f, 0.0f, lb_w, lb_h, data, choice, &lb_scrolls))
            if(GUI_Event(GUI_EVT_CHOICE))
            {
                *open = false;
                evt = true;
            }
        GUI_EndFrame();

        GUI_PopLayer();
    }

    if(pass == GUI_PASS_RESPONSE)
    {
        if(*open && mouse.left_just_pressed && !mouseIn(x, y, lb_w, lb_h))
            *open = false;
    }
    else if(pass == GUI_PASS_DRAW)
    {
        GUI_DrawDropListHeader(x, y, w, h, *open, (0 <= *choice && *choice < data.size()) ? data[*choice] : "");
    }

    return evt;
}


bool GUI_Event(int mask)
{
    return event_bits & mask;
}

bool GUI_IsMouseIn()
{
    return is_mouse_in;
}

















void GUI_BeginWindow(const std::string &id, float *x, float *y, float w, float h, const std::string &title)
{
    float top_border    = 24.0f;
    float bottom_border = 5.0f;
    float left_border   = 5.0f;
    float right_border  = 5.0f;

    if(pass == GUI_PASS_EVENT)
    {
        genericHotActive(id, *x, *y, w, top_border);
    }
    else if(pass == GUI_PASS_RESPONSE)
    {
        if(active_widget == id)
        {
            if(mouse.dragged)
            {
                *x += mouse.dx;
                *y += mouse.dy;
            }
            else if(!mouse.left_down)
                active_widget = "";
        }
    }
    else if(pass == GUI_PASS_DRAW)
    {
        GUI_DrawWindow(*x, *y, w, h, title);
    }

    GUI_BeginGroup(*x+left_border, *y+top_border, w-left_border-right_border, h-top_border-bottom_border);
}

void GUI_EndWindow()
{
    GUI_EndGroup();
}








template<class T> GUISpinnerData GUI_CreateSpinnerData(T value)
{
    GUISpinnerData d;
    d.caret = 0;
    d.selection = 0;
    d.offset = 0.0f;

    try
    {
        d.text_str = boost::lexical_cast<std::string>(value);
    }
    catch(boost::bad_lexical_cast &err)
    {
        d.text_str = "";
    }

    return d;
}

template<class T> bool GUI_Spinner(const std::string &id, float x, float y, float w, float h, GUISpinnerData *data, T min, T max, T *value)
{
    bool evt = false;

    if(GUI_EditBox(id + "/_0", x, y, w-16, h, &data->caret, &data->selection, &data->offset, &data->text_str) && GUI_Event(GUI_EVT_CONFIRMED))
    {
        try
        {
            std::cout << data->text_str << "\n";
            *value = boost::lexical_cast<T>(data->text_str);
            evt = true;
        }
        catch(boost::bad_lexical_cast &err)
        {
            data->text_str = boost::lexical_cast<std::string>(*value);
            std::cout << err.what() << "\n";
        }
    }

    int button_draw_mode = GUI_DrawGetButtonMode();
    GUI_DrawSetButtonMode(GUI_BUTTON_DRAW_ARROW);
    GUI_PushButtonMode();
    GUI_ButtonMode(GUI_ACTIVATE_ON_DOWN);
    if(GUI_Button(id + "/_inc", x+w-16, y, 16, h/2, "u"))
    {
        (*value)++;
        evt = true;
    }

    if(GUI_Button(id + "/_dec", x+w-16, y+h/2, 16, h/2, "d"))
    {
        (*value)--;
        evt = true;
    }
    GUI_PopButtonMode();
    GUI_DrawSetButtonMode(button_draw_mode);


    if(evt)
    {
        *value = cml::clamp(*value, min, max);
        data->text_str = boost::lexical_cast<std::string>(*value);
    }

    return evt;
}

template GUISpinnerData GUI_CreateSpinnerData<char>  (char   value);
template GUISpinnerData GUI_CreateSpinnerData<short> (short  value);
template GUISpinnerData GUI_CreateSpinnerData<int>   (int    value);
template GUISpinnerData GUI_CreateSpinnerData<long>  (long   value);
template GUISpinnerData GUI_CreateSpinnerData<float> (float  value);
template GUISpinnerData GUI_CreateSpinnerData<double>(double value);

template bool GUI_Spinner<char>  (const std::string &id, float x, float y, float w, float h, GUISpinnerData *data, char   min, char   max, char   *value);
template bool GUI_Spinner<short> (const std::string &id, float x, float y, float w, float h, GUISpinnerData *data, short  min, short  max, short  *value);
template bool GUI_Spinner<int>   (const std::string &id, float x, float y, float w, float h, GUISpinnerData *data, int    min, int    max, int    *value);
template bool GUI_Spinner<long>  (const std::string &id, float x, float y, float w, float h, GUISpinnerData *data, long   min, long   max, long   *value);
template bool GUI_Spinner<float> (const std::string &id, float x, float y, float w, float h, GUISpinnerData *data, float  min, float  max, float  *value);
template bool GUI_Spinner<double>(const std::string &id, float x, float y, float w, float h, GUISpinnerData *data, double min, double max, double *value);





















int GUI_GetPass()               { return pass; }
std::string GUI_HotWidget()     { return hot_widget; }
std::string GUI_ActiveWidget()  { return active_widget; }
bool GUI_MouseLeftJustPressed() { return mouse.left_just_pressed; }
bool GUI_MouseLeftDown()        { return mouse.left_down; }
bool GUI_MouseRightDown()       { return mouse.right_down; }
float GUI_MouseX()              { return mouse.x; }
float GUI_MouseY()              { return mouse.y; }
float GUI_MouseDX()             { return mouse.dx; }
float GUI_MouseDY()             { return mouse.dy; }
bool GUI_MouseDragged()         { return mouse.dragged; }
int GUI_MouseWheelDelta()       { return mouse.wheel_delta; }
void GUI_GenericHotActive(const std::string &id, float x, float y, float w, float h)     { genericHotActive(id, x, y, w, h); }

void GUI_LocalToWorld(float *x, float *y)
{
    *x = *x + w_offset[0];
    *y = *y + w_offset[1];
}



















bool GUI_FileChooser(const std::string &id, float x, float y, float w, float h, GUIFileChooserData *data)
{
    GUIDirContents &c = data->contents;
    int offset = data->scrolls[1];

    float pb_w = w, pb_h = 25;
    float b_w = 64, b_h = 24;
    float lw = w/4, lh = h-pb_h-b_h;

    float ok_x = x+w-b_w*2, ok_y = y+h-25;
    float no_x = x+w-b_w*1, no_y = y+h-25;

#if 0
    if(GUI_EditBox(id+"/_edit", x, y, pb_w, pb_h, &data->path_edit_data) && GUI_Event(GUI_EVT_CONFIRMED))
    {
        std::string &str = data->path_edit_data.str;
        if(!data->exists(str))
        {
            c.clear();
        }
        else if(data->isDir(str))
        {
            c.clear();
            c.dir = str;
            data->getDirContents(c.str, &c);
            c.sort();
        }
        else if(data->isFile(str))
        {
        }
    }
#endif

    if(pass == GUI_PASS_DRAW)
    {
        float dirfiles_w = 20.0f;
        float sizes_w = 40.0f;
        float types_w = 40.0f;
        float names_w = w - dirfiles_w - sizes_w - types_w;

        GUI_ListboxMulti(id+"/_l3", x, y+20, dirfiles_w, lh, c.dirfile, offset, &c.selected);
        GUI_ListboxMulti(id+"/_l0", x+dirfiles_w, y+20, names_w, lh, c.names, offset, &c.selected);
        GUI_ListboxMulti(id+"/_l2", x+dirfiles_w+names_w, y+20, sizes_w, lh, c.sizes, offset, &c.selected);
        GUI_ScrolledListboxMulti(id+"/_l1", x+dirfiles_w+names_w+sizes_w, y+20, types_w, lh, c.types, &c.selected, &data->scrolls);
    }
    else
    {
        if(GUI_ScrolledListboxMulti(id+"/_l3", x, y+20, w, lh, c.dirfile, &c.selected, &data->scrolls) && GUI_Event(GUI_EVT_CHOICE))
        {
            int num_selected = 0;
            size_t index = 0;

            for(size_t i = 0; i < c.selected.size(); i++)
            {
                if(c.selected[i])
                {
                    num_selected++;
                    index = i;
                }
            }

            if(num_selected == 1)
            {
                printf("joining\n");
                std::string str = data->joinPaths(data->dir, c.names[index]);

                if(data->isDir(str) && data->getDirContents)
                {
                    printf("getting contents\n");
                    data->dir = str;
                    c.clear();
                    data->getDirContents(data->dir, &c);
                    c.sort();
                }
                else if(data->isFile(str))
                {
                    data->file_edit_data.str = c.names[index];
                    printf("is file\n");
                }
            }
        }
    }

    event_bits = 0;

    if(data->is_save && GUI_EditBox(id+"/_edit", x, ok_y, w, b_h, &data->file_edit_data) && GUI_Event(GUI_EVT_CONFIRMED))
        event_bits |= GUI_EVT_CONFIRMED;

    if(GUI_Button(id+"/_ok", ok_x, ok_y, b_w, b_h, data->is_save ? "Save" : "Open"))
        event_bits |= GUI_EVT_CONFIRMED;

    if(GUI_Button(id+"/_no", no_x, no_y, b_w, b_h, "Cancel"))
        event_bits |= GUI_EVT_CANCELLED;

    return event_bits != 0;
}

