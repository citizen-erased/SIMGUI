#include <assert.h>
#include <stack>

#include <cml/cml.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include "gui.h"
#include "gui_draw.h"

extern sf::RenderWindow window;

static cml::vector2f translation;
static float clip_x, clip_y, clip_w, clip_h;
static std::stack<cml::vector2f> translation_stack;

static sf::Font *font_ptr;
static int font_size;
static cml::vector4f font_color;

static int button_draw_mode;


/*--------------------------------------------------------------------------*
 *
 * GUI GL drawing functions. Does the actual rendering.
 *
 *--------------------------------------------------------------------------*/
void GUI_GL_Translate(float x, float y)
{
    glTranslatef(x, y, 0.0f);
}

void GUI_GL_SetTranslation(float x, float y)
{
    glLoadIdentity();
    glTranslatef(x, y, 0.0f);
}

void GUI_GL_PushTranslation()
{
    glPushMatrix();
}

void GUI_GL_PopTranslation()
{
    glPopMatrix();
}

void GUI_GL_SetClipRect(float x, float y, float w, float h)
{
    glScissor(x, y, w, h);
}

void GUI_GL_Line(float x1, float y1, float x2, float y2, const cml::vector4f &col)
{
    glColor4fv(col.data());
    glBegin(GL_LINES);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
    glEnd();
}

void GUI_GL_Triangle(float x0, float y0, float x1, float y1, float x2, float y2, const cml::vector4f &col)
{
    glColor4fv(col.data());
    glBegin(GL_TRIANGLES);
        glVertex2f(x0, y0);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
    glEnd();
}

void GUI_GL_Rect(float x, float y, float w, float h, const cml::vector4f &col)
{
    glColor4fv(col.data());
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x+w, y);
        glVertex2f(x+w, y+h);
        glVertex2f(x, y+h);
    glEnd();
}

void GUI_GL_RectOutline(float x, float y, float w, float h, float thickness, const cml::vector4f &col)
{
    GUI_GL_Rect(x, y, w, thickness, col);
    GUI_GL_Rect(x, y+h-thickness, w, thickness, col);
    GUI_GL_Rect(x, y, thickness, h, col);
    GUI_GL_Rect(x+w-thickness, y, thickness, h, col);
}

void GUI_GL_RectRaised(float x, float y, float w, float h, float border, const cml::vector4f &shade1, const cml::vector4f &shade2, const cml::vector4f &fill_col)
{
    //top
    GUI_GL_Rect(x, y, w, border, shade1);
    //left
    GUI_GL_Rect(x, y, border, h, shade1);
    //bottom
    GUI_GL_Rect(x+border, y+h-border, w-border, border, shade2);
    //right
    GUI_GL_Rect(x+w-border, y+border, border, h-border, shade2);

    //fill in bottom left triangle
    GUI_GL_Triangle(x, y+h, x+border, y+h, x+border, y+h-border, shade2);

    //fill in top right triangle
    GUI_GL_Triangle(x+w-border, y+border, x+w, y+border, x+w, y, shade2);

    //inside
    GUI_GL_Rect(x+border, y+border, w-border-border, h-border-border, fill_col);
}

void GUI_GL_RectTextured(float x, float y, float w, float h, float tminx, float tminy, float tmaxx, float tmaxy, GLuint tex_id, const cml::vector4f &col)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glColor4fv(col.data());
    glBegin(GL_QUADS);
        glTexCoord2f(tminx, tminy);
        glVertex2f(x, y);
        glTexCoord2f(tmaxx, tminy);
        glVertex2f(x+w, y);
        glTexCoord2f(tmaxx, tmaxy);
        glVertex2f(x+w, y+h);
        glTexCoord2f(tminx, tmaxy);
        glVertex2f(x, y+h);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

/*
 * x and y in screen space.
 */
void GUI_GL_TextAligned(float x, float y, float w, float h, int horz, int vert, sf::Font *font, int font_size, const cml::vector4f &font_col, const std::string &str)
{
    sf::String txt(str, *font, font_size);

    sf::FloatRect rect = txt.GetRect();
    float txt_w = rect.GetWidth();
    //float txt_h = rect.GetHeight();
    float txt_h = font_size;
    float txt_x, txt_y;

    if(horz == GUI_ALIGN_LEFT)
        txt_x = x;
    else if(horz == GUI_ALIGN_RIGHT)
        txt_x = x + w - txt_w;
    else
        txt_x = x + w/2.0f - txt_w/2.0f;

    if(vert == GUI_ALIGN_TOP)
        txt_y = y;
    else if(vert == GUI_ALIGN_BOTTOM)
        txt_y = y + h - txt_h;
    else
        txt_y = y + h/2.0f - txt_h/2.0f;

    /* floor coordinates or blurry subpixel rendering happens */
    txt.SetPosition(std::floor(txt_x), std::floor(txt_y));
    txt.SetColor(sf::Color(font_color[0]*255, font_color[1]*255, font_color[2]*255, font_color[3]*255));
    window.Draw(txt);

#if 0
    glPushMatrix();
    glLoadIdentity();
    GUI_GL_RectOutline(txt_x, txt_y, txt_w, txt_h, 1, font_col);
    glPopMatrix();
#endif
}















/*--------------------------------------------------------------------------*
 * 
 * Draw command buffering.
 *
 *--------------------------------------------------------------------------*/
enum
{
    CMD_TRANSLATE,
    CMD_TRANSLATE_SET,
    CMD_TRANSLATE_PUSH,
    CMD_TRANSLATE_POP,
    CMD_CLIP_RECT_SET,

    CMD_LINE,
    CMD_TRIANGLE,
    CMD_RECT,
    CMD_RECT_OUTLINE,
    CMD_RECT_RAISED,
    CMD_RECT_TEXTURED,
    CMD_TEXT,
    CMD_FRAME,
};

void GUI_BufferPrint();

struct BufferEntry
{
    int type, layer;

    float         x0,y0,x1,y1,x2,y2,x3,y3;
    cml::vector4f cols[4];
    int           ints[4];
    float         floats[4];
    GLuint        tex0;
    void          *data0;
    std::string   str;
};

static std::vector<BufferEntry> buffer;
static int draw_layer = 0;


BufferEntry& addBufferEntry(int type)
{
    BufferEntry e;
    e.type = type;
    e.layer = draw_layer;
    buffer.push_back(e);
    return buffer[buffer.size()-1];
}

static bool bufferSortComp(BufferEntry a, BufferEntry b)
{
    return a.layer < b.layer;
}

void GUI_BufferExecute()
{
    //std::cout << "before---------------------------------------------------------\n";
    //GUI_BufferPrint();
    std::stable_sort(buffer.begin(), buffer.end(), bufferSortComp);
    //std::cout << "after----------------------------------------------------------\n";
    //GUI_BufferPrint();

    for(size_t i = 0; i < buffer.size(); i++)
    {
        BufferEntry &e = buffer[i];

        switch(e.type)
        {
            case CMD_TRANSLATE:         GUI_GL_Translate(e.x0, e.y0); break;
            case CMD_TRANSLATE_SET:     GUI_GL_SetTranslation(e.x0, e.y0); break;
            case CMD_TRANSLATE_PUSH:    GUI_GL_PushTranslation(); break;
            case CMD_TRANSLATE_POP:     GUI_GL_PopTranslation(); break;
            case CMD_CLIP_RECT_SET:     GUI_GL_SetClipRect(e.x0, e.y0, e.x1, e.y1); break;

            case CMD_LINE:              GUI_GL_Line(e.x0, e.y0, e.x1, e.y1, e.cols[0]); break;
            case CMD_TRIANGLE:          GUI_GL_Triangle(e.x0, e.y0, e.x1, e.y1, e.x2, e.y2, e.cols[0]); break;
            case CMD_RECT:              GUI_GL_Rect(e.x0, e.y0, e.x1, e.y1, e.cols[0]); break;
            case CMD_RECT_OUTLINE:      GUI_GL_RectOutline(e.x0, e.y0, e.x1, e.y1, e.floats[0], e.cols[0]); break;
            case CMD_RECT_RAISED:       GUI_GL_RectRaised(e.x0, e.y0, e.x1, e.y1, e.floats[0], e.cols[0], e.cols[1], e.cols[2]); break;
            case CMD_RECT_TEXTURED:     GUI_GL_RectTextured(e.x0, e.y0, e.x1, e.y1, e.x2, e.y2, e.x3, e.y3, e.tex0, e.cols[0]); break;
            case CMD_TEXT:              GUI_GL_TextAligned(e.x0, e.y0, e.x1, e.y1, e.ints[0], e.ints[1], (sf::Font*)e.data0, e.ints[2], e.cols[0], e.str); break;
            //case CMD_FRAME:             GUI_GL_Frame(e.x0, e.y0, e.x1, e.y1); break;
            default: break;
        }
    }
}

void GUI_BufferClear()
{
    buffer.clear();
}

void GUI_BufferPrint()
{
    for(size_t i = 0; i < buffer.size(); i++)
    {
        BufferEntry &e = buffer[i];

        std::cout << e.layer << ": ";

        switch(e.type)
        {
            case CMD_TRANSLATE:         
                std::cout << "TRANSLATE(" << e.x0 << ", " << e.y0 << ")\n";
                break;
            case CMD_TRANSLATE_SET:         
                std::cout << "TRANSLATE_SET(" << e.x0 << ", " << e.y0 << ")\n";
                break;
            case CMD_TRANSLATE_PUSH:    
                std::cout << "TRANSLATE_PUSH\n";
                break;
            case CMD_TRANSLATE_POP:     
                std::cout << "TRANSLATE_POP\n";
                break;
            case CMD_CLIP_RECT_SET:     
                std::cout << "SET_CLIP_RECT\n";
                break;

            case CMD_LINE:              
                std::cout << "LINE\n";
                break;
            case CMD_TRIANGLE:          
                std::cout << "TRIANGLE\n";
                break;
            case CMD_RECT:              
                std::cout << "RECT\n";
                break;
            case CMD_RECT_OUTLINE:      
                std::cout << "RECT_OUTLINE\n";
                break;
            case CMD_RECT_RAISED:       
                std::cout << "RECT_RAISED\n";
                break;
            case CMD_RECT_TEXTURED:     
                std::cout << "RECT_TEXTURED\n";
                break;
            case CMD_TEXT:              
                std::cout << "TEXT(" << e.x0 << ", " << e.y0 << ", " << e.x1 << ", " << e.y1 << ", " << e.ints[0] << ", " << e.ints[1] << ", " << e.str << ")\n";
                break;
            default: break;
        }
    }
}

/*--------------------------------------------------------------------------*
 * 
 * Primitive buffering.
 *
 *--------------------------------------------------------------------------*/

void GUI_DrawLine(float x0, float y0, float x1, float y1, const cml::vector4f &col)
{
    BufferEntry &e = addBufferEntry(CMD_LINE);
    e.x0 = x0; e.y0 = y0;
    e.x1 = x1; e.y1 = y1;
    e.cols[0] = col;
}

void GUI_DrawTriangle(float x0, float y0, float x1, float y1, float x2, float y2, const cml::vector4f &col)
{
    BufferEntry &e = addBufferEntry(CMD_TRIANGLE);
    e.x0 = x0; e.y0 = y0;
    e.x1 = x1; e.y1 = y1;
    e.x2 = x2; e.y2 = y2;
    e.cols[0] = col;
}

void GUI_DrawRect(float x, float y, float w, float h, const cml::vector4f &col)
{
    BufferEntry &e = addBufferEntry(CMD_RECT);
    e.x0 = x; e.y0 = y; e.x1 = w; e.y1 = h;
    e.cols[0] = col;
}

void GUI_DrawRectOutline(float x, float y, float w, float h, float thickness, const cml::vector4f &col)
{
    BufferEntry &e = addBufferEntry(CMD_RECT_OUTLINE);
    e.x0 = x; e.y0 = y; e.x1 = w; e.y1 = h;
    e.cols[0] = col;
    e.floats[0] = thickness;
}

void GUI_DrawRectRaised(float x, float y, float w, float h, float border, const cml::vector4f &shade1, const cml::vector4f &shade2, const cml::vector4f &fill_col)
{
    BufferEntry &e = addBufferEntry(CMD_RECT_RAISED);
    e.x0 = x; e.y0 = y; e.x1 = w; e.y1 = h;
    e.cols[0] = shade1; e.cols[1] = shade2; e.cols[2] = fill_col;
    e.floats[0] = border;
}

void GUI_DrawRectTextured(float x, float y, float w, float h, float tminx, float tminy, float tmaxx, float tmaxy, GLuint tex_id, const cml::vector4f &col)
{
    BufferEntry &e = addBufferEntry(CMD_RECT_TEXTURED);
    e.x0 = x; e.y0 = y; e.x1 = w; e.y1 = h;
    e.x2 = tminx; e.y2 = tminy;
    e.x3 = tmaxx; e.y3 = tmaxy;
    e.cols[0] = col;
    e.tex0 = tex_id;
}

void GUI_DrawText(float x, float y, const std::string &str)
{
    GUI_DrawTextAligned(x, y, 0, 0, GUI_ALIGN_LEFT, GUI_ALIGN_TOP, str);
}

void GUI_DrawTextAligned(float x, float y, float w, float h, int horz, int vert, const std::string &str)
{
    if(font_ptr == NULL)
        return;

    BufferEntry &e = addBufferEntry(CMD_TEXT);
    e.x0 = translation[0] + x; e.y0 = translation[1] + y;
    e.x1 = w; e.y1 = h;
    e.ints[0] = horz; e.ints[1] = vert; e.ints[2] = font_size;
    e.cols[0] = font_color;
    e.str = str;
    e.data0 = font_ptr;
}

void GUI_DrawArrow(float x, float y, float w, float h, int dir, const cml::vector4f &col)
{
    float cx = x + w/2.0f;
    float cy = y + h/2.0f;
    float size = std::min(w, h) / 2.0f;
    float hsize = size / 2.0f;

    switch(dir)
    {
        case GUI_UP:    GUI_DrawTriangle(cx-size, cy+hsize, cx+size, cy+hsize, cx, cy-hsize, col); break;
        case GUI_DOWN:  GUI_DrawTriangle(cx-size, cy-hsize, cx+size, cy-hsize, cx, cy+hsize, col); break;
        case GUI_LEFT:  GUI_DrawTriangle(cx+hsize, cy+size, cx+hsize, cy-size, cx-hsize, cy, col); break;
        case GUI_RIGHT: GUI_DrawTriangle(cx-hsize, cy+size, cx-hsize, cy-size, cx+hsize, cy, col); break;
        default: break;
    }
}























/*--------------------------------------------------------------------------*
 *
 * GUI drawing functions. These functions buffer actual drawing commands for
 * drawing later, so widgets will be drawn in the correct order, and
 * transparency will work.
 *
 *--------------------------------------------------------------------------*/

void GUI_DrawBegin()
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GUI_DrawEnd()
{
    GUI_BufferExecute();
    GUI_BufferClear();

    glPopAttrib();
}

void GUI_DrawTranslate(float x, float y)
{
    BufferEntry &e = addBufferEntry(CMD_TRANSLATE);
    e.x0 = x; e.y0 = y;

    translation[0] += x;
    translation[1] += y;
}

void GUI_DrawSetTranslation(float x, float y)
{
    BufferEntry &e = addBufferEntry(CMD_TRANSLATE_SET);
    e.x0 = x; e.y0 = y;

    translation[0] = x;
    translation[1] = y;
}
void GUI_DrawPushTranslation()
{
    BufferEntry &e = addBufferEntry(CMD_TRANSLATE_PUSH);
    translation_stack.push(translation);
}

void GUI_DrawPopTranslation()
{
    assert(!translation_stack.empty());

    BufferEntry &e = addBufferEntry(CMD_TRANSLATE_POP);
    translation = translation_stack.top();
    translation_stack.pop();
}

void GUI_DrawSetClipRect(float x, float y, float w, float h)
{
    BufferEntry &e = addBufferEntry(CMD_CLIP_RECT_SET);
    e.x0 = x; e.y0 = y; e.x1 = w; e.y1 = h;

    clip_x = x; clip_y = y; clip_w = w; clip_h = h;
}

void GUI_DrawSetButtonMode(int mode)
{
    button_draw_mode = mode;
}

int GUI_DrawGetButtonMode()
{
    return button_draw_mode;
}

void GUI_DrawFont(sf::Font *sf_font, int size, const cml::vector4f &color)
{
    font_ptr = sf_font;
    font_size = size;
    font_color = color;
}

void GUI_DrawSetLayer(int layer)
{
    draw_layer = layer;

    /*
     * When changing layer any modifiable state must be set as the first
     * commands in the layer.  If this isn't done then they could be different
     * when the commands are sorted.
     *
     * Example:
     *
     * setLayer(1)
     * translate(10, 10)
     * setLayer(0)
     * rect(0, 0) //expected to draw at 10, 10
     *
     * would result in the wrong draw order since the list will be rearranged to:
     * setLayer(0)
     * rect(0, 0) //draws at 0, 0
     * setLayer(1)
     * translate(10, 10)
     */

    GUI_DrawSetTranslation(translation[0], translation[1]);
    GUI_DrawSetClipRect(clip_x, clip_y, clip_w, clip_h);
}


void GUI_DrawLabel(float x, float y, float w, float h, const std::string &str)
{
    //GUI_DrawRect(x, y, w, h, cml::vector4f(0.8f, 0.8f, 0.8f, 1.0f));
    GUI_DrawTextAligned(x, y, w, h, GUI_ALIGN_LEFT, GUI_ALIGN_CENTER, str);
}

void GUI_DrawButton(float x, float y, float w, float h, bool hot, bool active, const std::string &str)
{
    cml::vector4f tri1_col(0.9f, 0.9f, 0.9f, 1.0f);
    cml::vector4f tri2_col(95.0f/255.0f, 95.0f/255.0f, 95.0f/255.0f, 1.0f);
    cml::vector4f quad_col(169.0f/255.0f, 169.0f/255.0f, 169.0f/255.0f, 1.0f);
    float border_size = 2.0f;
    float offset = active ? border_size/2.0f : 0.0f;

    if(hot)
        quad_col += cml::vector4f(0.1f, 0.1f, 0.1f, 0.0f);

    if(active)
        std::swap(tri1_col, tri2_col);

    GUI_DrawRectRaised(x, y, w, h, border_size, tri1_col, tri2_col, quad_col);

    switch(button_draw_mode)
    {
        case GUI_BUTTON_DRAW_STRING:
        {
            if(!str.empty())
                GUI_DrawTextAligned(x+offset, y+offset, w, h, GUI_ALIGN_CENTER, GUI_ALIGN_CENTER, str);
            break;
        }
        case GUI_BUTTON_DRAW_ARROW:
        {
            int dir;

            if     (str == "u") dir = GUI_UP;
            else if(str == "d") dir = GUI_DOWN;
            else if(str == "l") dir = GUI_LEFT;
            else                dir = GUI_RIGHT;

            float arrow_border = std::min(w, h) * 0.2f;
            GUI_DrawArrow(x+arrow_border+offset, y+arrow_border+offset, w-arrow_border*2.0f, h-arrow_border*2.0f, dir, cml::vector4f(0,0,0,1));
            break;
        }
        case GUI_BUTTON_DRAW_TEXTURE:
        {
            break;
        }
        default: break;
    }
}

void GUI_DrawCheckbox(float x, float y, float w, float h, bool selected)
{
    GUI_DrawRect(x, y, w, h, cml::vector4f(1.0f, 1.0f, 1.0f, 1.0f));

    if(selected) GUI_DrawRect(x+2, y+2, w-4, h-4, cml::vector4f(1.0f, 0.0f, 0.0f, 1.0f));
}

void GUI_DrawCheckboxLabelled(float x, float y, float w, float h, float cx, float cy, float cw, float ch, bool hot, const std::string &str, bool selected)
{
    if(hot) GUI_DrawRect(x, y, w, h, cml::vector4f(0.3f, 0.3f, 0.3f, 1.0f));
    GUI_DrawCheckbox(cx, cy, cw, ch, selected);
    GUI_DrawText(x + cw + 3, y, str);
}

void GUI_DrawEditBox(float x, float y, float w, float h, bool active, int caret_pos, int selection, float str_offset, const std::string &str)
{
    cml::vector4f bg_active_col  (131.0f/255.0f, 129.0f/255.0f, 131.0f/255.0f, 1.0f);
    cml::vector4f bg_inactive_col(111.0f/255.0f, 109.0f/255.0f, 111.0f/255.0f, 1.0f);
    cml::vector4f highlight_col  ( 49.0f/255.0f,  97.0f/255.0f, 131.0f/255.0f, 1.0f);
    cml::vector4f caret_col      (  0.0f/255.0f,   0.0f/255.0f,   0.0f/255.0f, 1.0f);
    cml::vector4f border_col     ( 64.0f/255.0f,  64.0f/255.0f,  64.0f/255.0f, 1.0f);

    /* background */
    GUI_DrawRect(x, y, w, h, active ? bg_active_col : bg_inactive_col);

    /* font stuff */
    if(font_ptr)
    {
        sf::String txt(str, *font_ptr, font_size);
        sf::Vector2f caret_vec = txt.GetCharacterPos(caret_pos);

        /* highlight selected text before drawing the string */
        if(selection != 0)
        {
            sf::Vector2f selection_vec = txt.GetCharacterPos(caret_pos + selection);
            float minx = std::min(caret_vec.x, selection_vec.x);
            float maxx = std::max(caret_vec.x, selection_vec.x);
            GUI_DrawRect(x+minx+str_offset, y, maxx-minx, h, highlight_col);
        }

        GUI_DrawTextAligned(x+str_offset, y, txt.GetRect().GetWidth(), h, GUI_ALIGN_LEFT, GUI_ALIGN_CENTER, str.c_str());

        /* draw the caret above everything */
        if(active) GUI_DrawRect(x+caret_vec.x+str_offset, y, 1, h, caret_col);
    }

    /* outline */
    GUI_DrawRectOutline(x, y, w, h, 1.0f, border_col);
}

void GUI_DrawListbox(float x, float y, float w, float h, float item_height, const std::vector<std::string> &data, int data_offset, int choice)
{
    cml::vector4f bg_1(131.0f/255.0f, 129.0f/255.0f, 131.0f/255.0f, 1.0f);
    cml::vector4f bg_2(162.0f/255.0f, 165.0f/255.0f, 162.0f/255.0f, 1.0f);
    cml::vector4f choice_col(49.0f/255.0f, 97.0f/255.0f, 131.0f/255.0f, 1.0f);

    float item_x = x;
    float item_y = y;
    int   index = data_offset;

    GUI_DrawRect(x, y, w, h, bg_1);

    while(item_y < y+h && index < data.size())
    {
        if(index == choice)
            GUI_DrawRect(item_x, item_y, w, item_height, choice_col);
        else if(index % 2)
            GUI_DrawRect(item_x, item_y, w, item_height, bg_2);


        GUI_DrawText(item_x, item_y, data[index].c_str());

        index++;
        item_y += item_height;
    }
}

void GUI_DrawListboxMulti(float x, float y, float w, float h, float item_height, const std::vector<std::string> &data, int data_offset, const std::vector<bool> &selected)
{
    cml::vector4f bg_1(131.0f/255.0f, 129.0f/255.0f, 131.0f/255.0f, 1.0f);
    cml::vector4f bg_2(162.0f/255.0f, 165.0f/255.0f, 162.0f/255.0f, 1.0f);
    cml::vector4f choice_col(49.0f/255.0f, 97.0f/255.0f, 131.0f/255.0f, 1.0f);

    float item_x = x;
    float item_y = y;
    int   index = data_offset;

    GUI_DrawRect(x, y, w, h, bg_1);

    while(item_y < y+h && index < data.size())
    {
        if(selected[index])
            GUI_DrawRect(item_x, item_y, w, item_height, choice_col);
        else if(index % 2)
            GUI_DrawRect(item_x, item_y, w, item_height, bg_2);

        GUI_DrawText(item_x, item_y, data[index].c_str());

        index++;
        item_y += item_height;
    }
}

void GUI_DrawDropListHeader(float x, float y, float w, float h, bool open, const std::string &text)
{
    cml::vector4f bg_col(131.0f/255.0f, 129.0f/255.0f, 131.0f/255.0f, 1.0f);

    GUI_DrawRect(x, y, w, h, bg_col);
    GUI_DrawTextAligned(x, y, w-32, h, GUI_ALIGN_LEFT, GUI_ALIGN_CENTER, text);
    GUI_DrawButton(x+w-32, y, 32, h, false, open, "");
}

void GUI_DrawSlider(float x, float y, float w, float h, float thumb_x, float thumb_y, float thumb_w, float thumb_h)
{
    cml::vector4f bg_col(0.3f, 0.3f, 0.3f, 1.0f);
    cml::vector4f thumb_col(95.0f/255.0f, 95.0f/255.0f, 95.0f/255.0f, 1.0f);
    cml::vector4f tri1_col(0.9f, 0.9f, 0.9f, 1.0f);
    cml::vector4f tri2_col(95.0f/255.0f, 95.0f/255.0f, 95.0f/255.0f, 1.0f);
    cml::vector4f quad_col(169.0f/255.0f, 169.0f/255.0f, 169.0f/255.0f, 1.0f);

    GUI_DrawRect(x, y, w, h, bg_col);
    //GUI_DrawRect(thumb_x, thumb_y, thumb_w, thumb_h, thumb_col);
    GUI_DrawRectRaised(thumb_x, thumb_y, thumb_w, thumb_h, 1.0f, tri1_col, tri2_col, quad_col);
}




void GUI_DrawDropMenuHeaderItem(float x, float y, float w, float h, bool hot, bool active, const std::string &str)
{
    cml::vector4f bg_col    (156.0f/255.0f, 153.0f/255.0f, 156.0f/255.0f, 1.0f);
    cml::vector4f hot_col   (176.0f/255.0f, 173.0f/255.0f, 176.0f/255.0f, 1.0f);
    cml::vector4f active_col( 49.0f/255.0f,  97.0f/255.0f, 131.0f/255.0f, 1.0f);

    if(active)
        GUI_DrawRect(x, y, w, h, active_col);
    else if(hot)
        GUI_DrawRect(x, y, w, h, hot_col);
    else
        GUI_DrawRect(x, y, w, h, bg_col);

    GUI_DrawTextAligned(x, y, w, h, GUI_ALIGN_CENTER, GUI_ALIGN_CENTER, str);
}

void GUI_DrawPopup(float x, float y, float w, float h)
{
    cml::vector4f bg_col    (156.0f/255.0f, 153.0f/255.0f, 156.0f/255.0f, 1.0f);
    cml::vector4f border_col( 64.0f/255.0f,  64.0f/255.0f,  64.0f/255.0f, 1.0f);

    GUI_DrawRect(x, y, w, h, bg_col);
    GUI_DrawRectOutline(x, y, w, h, 1.0f, border_col);
}

void GUI_DrawPopupButton(float x, float y, float w, float h, bool hot, bool active, const std::string &left_str, const std::string &right_str)
{
    if(hot) GUI_DrawRect(x, y, w, h, cml::vector4f(0.3f, 0.3f, 0.3f, 1.0f));

    GUI_DrawTextAligned(x+5.0f, y, w, h, GUI_ALIGN_LEFT, GUI_ALIGN_CENTER, left_str);
    GUI_DrawTextAligned(x + w/2.0f, y, w/2.0f, h, GUI_ALIGN_LEFT, GUI_ALIGN_CENTER, right_str);
}

void GUI_DrawPopupSubMenuButton(float x, float y, float w, float h, bool hot, bool active, const std::string &str)
{
    if(active) GUI_DrawRect(x, y, w, h, cml::vector4f(0.3f, 0.3f, 0.3f, 1.0f));

    GUI_DrawText(x, y, str);

    float arrow_size   = std::min(w, h);
    float arrow_border = arrow_size * 0.2f;
    GUI_DrawArrow(x+w-arrow_size, y+arrow_border, arrow_size-arrow_border*2.0f, arrow_size-arrow_border*2.0f, GUI_RIGHT, cml::vector4f(0.0f, 0.0f, 0.0f, 1.0f));
}

void GUI_DrawSeparator(float x, float y, float w, float h)
{
    GUI_DrawLine(x + 2, y + h/2.0f, x + w - 2, y + h/2.0f, cml::vector4f(0.3f, 0.3f, 0.3f, 1.0f));
}

void GUI_DrawFrame(float x, float y, float w, float h, float padding_x, float padding_y)
{
    cml::vector4f bg_col    (156.0f/255.0f, 153.0f/255.0f, 156.0f/255.0f, 1.0f);
    cml::vector4f border_col( 64.0f/255.0f,  64.0f/255.0f,  64.0f/255.0f, 1.0f);
    GUI_DrawRect(x, y, w, h, bg_col);
    GUI_DrawRectOutline(x, y, w, h, 1.0f, border_col);
}

void GUI_DrawWindow(float x, float y, float w, float h, const std::string &title)
{
    cml::vector4f light(222.0f/255.0f, 173.0f/255.0f, 198.0f/255.0f, 1.0f);
    cml::vector4f medium(181.0f/255.0f, 74.0f/255.0f, 123.0f/255.0f, 1.0f);
    cml::vector4f dark(82.0f/255.0f, 33.0f/255.0f, 57.0f/255.0f, 1.0f);
    cml::vector4f background(222.0f/255.0f, 222.0f/255.0f, 231.0f/255.0f, 1.0f);

    GUI_DrawRect(x, y, w, h, background);

    //GUI_DrawRect(x+25, y+2, w-25-25, 2, medium);
    //GUI_DrawRect(x+25, y+4, w-25-25, 1, dark);
    //GUI_DrawRect(x+w-25, y+1, 1, 3, dark);

    GUI_DrawRect(x, y, w, 2, light);
    GUI_DrawRect(x, y+2, w, 2, medium);
    GUI_DrawRect(x, y+4, w, 1, dark);
    GUI_DrawRect(x, y+5, w, 1, light);
    GUI_DrawRect(x, y+6, w, 17, medium);
    GUI_DrawRect(x, y+23, w, 1, dark);

    GUI_DrawRect(x, y, 2, h, light);
    GUI_DrawRect(x+2, y+2, 2, h-2-2, medium);
    GUI_DrawRect(x+4, y+4, 1, h-2-4, dark);

    GUI_DrawRect(x+w-2, y, 2, h, light);
    GUI_DrawRect(x+w-4, y+2, 2, h-2-2, medium);
    GUI_DrawRect(x+w-4, y+4, 1, h-2-4, dark);

    GUI_DrawRect(x+2, y+h-2, w-2, 2, dark);
    GUI_DrawRect(x+2, y+h-4, w-4, 2, medium);
    GUI_DrawRect(x+4, y+h-5, w-5, 1, light);

    GUI_DrawTextAligned(x, y, w, 25, GUI_ALIGN_CENTER, GUI_ALIGN_CENTER, title);
}

