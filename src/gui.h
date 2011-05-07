#ifndef GUI_H
#define GUI_H

enum
{
    /*
     * An input should be injected and then the hot widget should be found
     * during this pass.
     * 
     * No response to events should be done.
     */
    GUI_PASS_EVENT,

    /*
     * Response to input events should be done. The hot widget set from the
     * last GUI_PASS_EVENT should be used.
     *
     * The hot widget shouldn't be changed but the active widget should be set
     * during this pass.
     */
    GUI_PASS_RESPONSE,

    /*
     * GUI drawing.
     */
    GUI_PASS_DRAW,

    GUI_PASS_NONE,


    GUI_HORIZONTAL,
    GUI_VERTICAL,

    GUI_ACTIVATE_ON_UP,
    GUI_ACTIVATE_ON_DOWN,

    GUI_ALIGN_LEFT,
    GUI_ALIGN_RIGHT,
    GUI_ALIGN_TOP,
    GUI_ALIGN_BOTTOM,
    GUI_ALIGN_CENTER,

    GUI_LEFT,
    GUI_RIGHT,
    GUI_UP,
    GUI_DOWN,

    GUI_BUTTON_DRAW_STRING,
    GUI_BUTTON_DRAW_ARROW,
    GUI_BUTTON_DRAW_TEXTURE,
};

enum
{
    GUI_EVT_SCROLLED  = 0x0001,
    GUI_EVT_CHOICE    = 0x0002,
    GUI_EVT_CONFIRMED = 0x0004,
    GUI_EVT_CANCELLED = 0x0008,
};

#define GUI_MAX_LAYER 1024
#define GUI_DROP_LIST_LAYER 1025

struct PopupNode
{
    std::string name;
    std::vector<PopupNode*> children;
    PopupNode *parent;
    bool active;

    PopupNode(const std::string &name) { this->name = name; active = false; parent = NULL; }
    void addChild(PopupNode *n) { n->parent = this; children.push_back(n); }
};

struct GUISpinnerData
{
    int caret, selection;
    float offset;
    std::string text_str;
};

struct GUIEditBoxData
{
    int caret_pos;
    int selection;
    float offset;
    std::string str;
    std::string *str_ptr;

    GUIEditBoxData();
    GUIEditBoxData(const std::string &s);
    GUIEditBoxData(std::string *s);
};

struct GUIDirContents
{
    std::vector<std::string> names, types, sizes, dirfile;
    std::vector<bool> selected;

    void add(const std::string &name, const std::string type, const std::string size, bool is_file);
    void clear();
    void sort();
};

struct GUIFileChooserData
{
    GUIEditBoxData path_edit_data, file_edit_data;
    GUIDirContents contents;
    cml::vector2i scrolls;

    std::string dir;
    void(*getDirContents)(const std::string &dir, GUIDirContents *data);
    bool(*exists)(const std::string &path);
    bool(*isFile)(const std::string &path);
    bool(*isDir)(const std::string &path);
    std::string(*joinPaths)(const std::string &a, const std::string &b);

    bool is_save;

    GUIFileChooserData();
    std::string getFile();
    std::vector<std::string> getSelectedFiles();
    std::vector<std::string> getSelectedDirs();
};


class GUI_AABB
{
public:
    static GUI_AABB fromPositionSize(float x, float y, float w, float h);
    static GUI_AABB fromMinMax(float x1, float y1, float x2, float y2);

public:
    cml::vector2f   min, max;

                    GUI_AABB();
                    GUI_AABB(float x1, float y1, float x2, float y2);
                    GUI_AABB(cml::vector2f min, cml::vector2f max);

    bool            containsPoint(float x, float y) const;
    bool            intersects(const GUI_AABB &o) const;
    bool            intersects(float x1, float y1, float x2, float y2) const;
    GUI_AABB        intersection(const GUI_AABB &o) const;
    float           getWidth() const;
    float           getHeight() const;
};

struct GUIFont
{
    sf::Font *sf_font;
    int size;
    cml::vector4f color;
    bool valid;
};


/*--------------------------------------------------------------------------*
 * Initialization                                                           *
 *--------------------------------------------------------------------------*/
void GUI_Init();



/*--------------------------------------------------------------------------*
 * Passes                                                                   *
 *--------------------------------------------------------------------------*/
void GUI_BeginPass(int pass);
void GUI_EndPass();



/*--------------------------------------------------------------------------*
 * Input                                                                    *
 *--------------------------------------------------------------------------*/
void GUI_MouseButton(sf::Mouse::Button button, bool down);
void GUI_MouseMove(float x, float y);
void GUI_MouseWheel(int delta);

void GUI_KeyPressed(sf::Key::Code key, bool control, bool alt, bool shift);
void GUI_KeyTyped(int key);



/*--------------------------------------------------------------------------*
 * Layers                                                                   *
 *--------------------------------------------------------------------------*/
void GUI_SetLayer(int new_layer);
void GUI_PushLayer();
void GUI_PopLayer();



/*--------------------------------------------------------------------------*
 * Translation                                                              *
 *--------------------------------------------------------------------------*/
void GUI_Translate(float x, float y);
void GUI_PushTranslation();
void GUI_PopTranslation();



/*--------------------------------------------------------------------------*
 * Grouping                                                                 *
 *--------------------------------------------------------------------------*/
void GUI_BeginGroup(float x, float y, float w, float h);
void GUI_EndGroup();



/*--------------------------------------------------------------------------*
 * Labels                                                                   *
 *--------------------------------------------------------------------------*/
void GUI_Label(const std::string &id, int x, int y, int w, int h, const std::string &str);



/*--------------------------------------------------------------------------*
 * Buttons                                                                  *
 *--------------------------------------------------------------------------*/
bool GUI_Button(const std::string &id, int x, int y, int w, int h, const std::string &str);
bool GUI_ToggleButton(const std::string &id, int x, int y, int w, int h, const std::string &str, bool *value);
void GUI_ButtonMode(int mode);
void GUI_PushButtonMode();
void GUI_PopButtonMode();



/*--------------------------------------------------------------------------*
 * Checkboxes                                                               *
 *--------------------------------------------------------------------------*/
bool GUI_Checkbox(const std::string &id, float x, float y, float w, float h, bool *value);
bool GUI_CheckboxLabelled(const std::string id, float x, float y, float w, float h, const std::string &label, bool *value);


/*--------------------------------------------------------------------------*
 * Slider/Scrollbar                                                         *
 *--------------------------------------------------------------------------*/
bool GUI_Slider(const std::string &id, float x, float y, float w, float h, int type, int min, int max, int page_size, int *value);
bool GUI_Scrollbar(const std::string &id, float x, float y, float w, float h, int type, int min, int max, int page_size, int *value);
void GUI_ThumbSize();



/*--------------------------------------------------------------------------*
 * Listboxes                                                                *
 *--------------------------------------------------------------------------*/
bool GUI_Listbox(const std::string &id, float x, float y, float w, float h, const std::vector<std::string> &data, int data_offset, int *choice);
bool GUI_ListboxMulti(const std::string &id, float x, float y, float w, float h, const std::vector<std::string> &data, int data_offset, std::vector<bool> *selected);
bool GUI_ScrolledListbox(const std::string &id, float x, float y, float w, float h, const std::vector<std::string> &data, int *choice, cml::vector2i *scroll);
bool GUI_ScrolledListboxMulti(const std::string &id, float x, float y, float w, float h, const std::vector<std::string> &data, std::vector<bool> *selected, cml::vector2i *scroll);



/*--------------------------------------------------------------------------*
 * Scroll Area                                                              *
 *--------------------------------------------------------------------------*/
void GUI_BeginScrollArea(const std::string &id, float x, float y, float w, float h, float scroll_size, int min_scroll_x, int max_scroll_x, int min_scroll_y, int max_scroll_y, cml::vector2i *scroll);
void GUI_EndScrollArea();



/*--------------------------------------------------------------------------*
 * Window                                                                   *
 *--------------------------------------------------------------------------*/
void GUI_BeginWindow(const std::string &id, float *x, float *y, float w, float h, const std::string &title);
void GUI_EndWindow();



/*--------------------------------------------------------------------------*
 * Dropdown Menu                                                            *
 *--------------------------------------------------------------------------*/
bool GUI_BeginDropMenu(const std::string &id, float x, float y, float w, float h, float item_width, PopupNode *root);
void GUI_EndDropMenu();



/*--------------------------------------------------------------------------*
 * Popup                                                                    *
 *--------------------------------------------------------------------------*/
void GUI_BeginPopupGroup(PopupNode *root);
void GUI_EndPopupGroup();

void GUI_BeginPopupMenu(const std::string &id, float x, float y, float w, float h, float item_h, PopupNode *root);
void GUI_EndPopupMenu();

bool GUI_PopupMenuButton(const std::string &id, const std::string &left_str, const std::string &right_str);
void GUI_PopupSubMenuButton(const std::string &id, PopupNode *node);
void GUI_PopupSeparator(float h);



/*--------------------------------------------------------------------------*
 * Text Editing                                                             *
 *--------------------------------------------------------------------------*/
bool GUI_EditBox(const std::string &id, float x, float y, float w, float h, int *caret_pos, int *selection, float *offset, std::string *str); //FIXME get rid of this one
bool GUI_EditBox(const std::string &id, float x, float y, float w, float h, GUIEditBoxData *data);



/*--------------------------------------------------------------------------*
 * File Dialog                                                              *
 *--------------------------------------------------------------------------*/
bool GUI_FileChooser(const std::string &id, float x, float y, float w, float h, GUIFileChooserData *data);



/*--------------------------------------------------------------------------*
 * Misc                                                                     *
 *--------------------------------------------------------------------------*/
void GUI_ScreenBounds(float x, float y, float w, float h);
void GUI_PushClipRect(const GUI_AABB &clip);
void GUI_PopClipRect();

void GUI_Font(sf::Font *sf_font, int size, const cml::vector4f &color);

void GUI_FrameType();
void GUI_Frame(float x, float y, float w, float h);

void GUI_BeginFrame(float x, float y, float w, float h, float padding_x, float padding_y);
void GUI_EndFrame();

bool GUI_DropList(const std::string &id, float x, float y, float w, float h, const std::vector<std::string> &data, int *choice, bool *open);

bool GUI_Event(int mask);
bool GUI_IsMouseIn();

template<class T> GUISpinnerData GUI_CreateSpinnerData(T value);
template<class T> bool GUI_Spinner(const std::string &id, float x, float y, float w, float h, GUISpinnerData *data, T min, T max, T *value);



/*--------------------------------------------------------------------------*
 * Extending                                                                *
 *--------------------------------------------------------------------------*/
int         GUI_GetPass();
void        GUI_GenericHotActive(const std::string &id, float x, float y, float w, float h);
std::string GUI_HotWidget();
std::string GUI_ActiveWidget();
bool        GUI_MouseLeftJustPressed();
bool        GUI_MouseLeftDown();
bool        GUI_MouseRightDown();
float       GUI_MouseX();
float       GUI_MouseY();
float       GUI_MouseDX();
float       GUI_MouseDY();
int         GUI_MouseWheelDelta();
bool        GUI_MouseDragged();

void        GUI_LocalToWorld(float *x, float *y);

#endif /* GUI_H */

