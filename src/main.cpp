#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <map>
#include <algorithm>

#include <cml/cml.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <boost/lexical_cast.hpp>

#include "gui.h"

sf::RenderWindow window;
bool running = true;

std::vector<std::string> list_data;


void doEvents();
void doGUI(int pass);
void doMenubar();

int main()
{
    window.Create(sf::VideoMode(800, 600), "SFML Window", sf::Style::Close);
    window.PreserveOpenGLStates(true);
    window.SetFramerateLimit(60);

    for(int i = 0; i < 515; i++)
        list_data.push_back("item" + boost::lexical_cast<std::string>(i));

    GUI_Init();
    GUI_ScreenBounds(0, 0, 800, 600);

    while(running)
    {
        doEvents();

        glClear(GL_COLOR_BUFFER_BIT);


        doGUI(GUI_PASS_DRAW);

        window.Display();
    }

    return 0;
}

void doEvents()
{
    sf::Event e;
    while(window.GetEvent(e))
    {
        switch(e.Type)
        {
            case sf::Event::KeyPressed:
            {
                if(e.Key.Code == sf::Key::Escape)
                    running = false;
                GUI_KeyPressed(e.Key.Code, e.Key.Control, e.Key.Alt, e.Key.Shift);
                doGUI(GUI_PASS_EVENT);
                doGUI(GUI_PASS_RESPONSE);
                break;
            }
            case sf::Event::KeyReleased:
            {
                break;
            }
            case sf::Event::MouseMoved:
            {
                GUI_MouseMove(e.MouseMove.X, e.MouseMove.Y);
                doGUI(GUI_PASS_EVENT);
                doGUI(GUI_PASS_RESPONSE);
                break;
            }
            case sf::Event::MouseButtonPressed:
            {
                GUI_MouseButton(e.MouseButton.Button, true);
                doGUI(GUI_PASS_EVENT);
                doGUI(GUI_PASS_RESPONSE);
                break;
            }
            case sf::Event::MouseButtonReleased:
            {
                GUI_MouseButton(e.MouseButton.Button, false);
                doGUI(GUI_PASS_EVENT);
                doGUI(GUI_PASS_RESPONSE);
                break;
            }
            case sf::Event::MouseWheelMoved:
            {
                GUI_MouseWheel(e.MouseWheel.Delta > 0.0f ? 1 : -1);
                doGUI(GUI_PASS_EVENT);
                doGUI(GUI_PASS_RESPONSE);
                break;
            }
            case sf::Event::TextEntered:
            {
                GUI_KeyTyped(e.Text.Unicode);
                doGUI(GUI_PASS_EVENT);
                doGUI(GUI_PASS_RESPONSE);
                break;
            }
            case sf::Event::Closed:
                running = false;
                break;
            default:
                break;
        }
    }
}

void doGUI(int pass)
{
    static bool b1 = false;
    static int s1 = 0;
    static int lb_choice = 0;
    static cml::vector2i lb_scrolls(0, 0);
    static cml::vector2i area_scrolls(0, 0);
    static std::string edit0_str("0123456789012345678901234567890123456789");

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, window.GetWidth(), window.GetHeight(), 0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    GUI_BeginPass(pass);

    GUI_Label("", 800-256, 0, 256, 24, "Sprite Chooser:");
    GUI_BeginGroup(window.GetWidth() - 256, 24, 256, window.GetHeight());

    if(GUI_ScrolledListbox("tile/tilemaps", 0, 0, 256, 128, list_data, &lb_choice, &lb_scrolls))
        std::cout << "Listbox 1 event. value=" << lb_choice << "\n";

    if(GUI_Button("tile/new", 0, 128, 64, 24, ""))
        std::cout << "button 2 event\n";

    if(GUI_Button("tile/delete", 65, 128, 64, 24, ""))
        std::cout << "button 3 event\n";

    if(GUI_ToggleButton("tile/toggle", 130, 128, 64, 24, "", &b1))
        std::cout << "button 4 event\n";

    GUI_Label("", 0, 128+24, 256, 24, "Sprite Chooser:");

    GUI_EndGroup();


    if(GUI_Button("random/button0", 150, 128, 64, 24, ""))
        std::cout << "button 3 event\n";

    static int edit_caret = 0;
    static int edit_selection = 0;
    static float edit_offset = 0;
    GUI_EditBox("random/edit0", 200, 128, 100, 24, &edit_caret, &edit_selection, &edit_offset, &edit0_str);

    GUI_CheckboxLabelled("checkbox1", 200, 200, 100, 16, "checkbox", &b1);

    static bool drop_open = false;
    GUI_DropList("droplist1", 200, 300, 100, 16, list_data, &lb_choice, &drop_open);
    GUI_Button("tile/delete2", 200, 316, 200, 24, "");
    GUI_Button("tile/delete3", 200, 356, 64, 24, "");

    static float win1x = 200.0f;
    static float win1y = 400.0f;
    GUI_BeginWindow("window1", &win1x, &win1y, 200, 200, "Test Window");
        GUI_Button("window1/button1", 0, 0, 20, 20, "");
        static int edit0_caret = 0;
        static int edit0_selection = 0;
        static float edit0_offset = 0;
        GUI_EditBox("window1/edit0", 0, 20, 100, 24, &edit0_caret, &edit0_selection, &edit0_offset, &edit0_str);

        static float spinf0 = 1.0f;
        static GUISpinnerData spin0_data = GUI_CreateSpinnerData(spinf0);
        GUI_Spinner("window1/spin0", 0, 50, 100, 24, &spin0_data, -100.0f, 100.0f, &spinf0);

        static int spini1 = 1;
        static GUISpinnerData spin1_data = GUI_CreateSpinnerData(spini1);
        GUI_Spinner("window1/spin1", 0, 50+25, 100, 24, &spin1_data, -10, 10, &spini1);
    GUI_EndWindow();

    doMenubar();

    GUI_EndPass();
}

void doMenubar()
{
    static PopupNode root("");

    static PopupNode file_root("File");
    static PopupNode sub1("Sub1");
    static PopupNode sub2("Sub2");

    static PopupNode edit_root("Edit");

    static bool nodes_made = false;
    if(!nodes_made)
    {
        root.addChild(&file_root);
        root.addChild(&edit_root);
        file_root.addChild(&sub1);
        file_root.addChild(&sub2);
        nodes_made = true;
    }

    GUI_BeginDropMenu("menubar", 0.0f, 0.0f, window.GetWidth(), 16.0f, 64.0f, &root);
        GUI_BeginPopupGroup(&root);

        if(file_root.active)
        {
            GUI_BeginPopupMenu("menubar/file", 0, 16, 100, 200, 16, &file_root);
                if(GUI_PopupMenuButton("menubar/file/b1", "left1", "right1"))
                    running = false;
                GUI_PopupMenuButton("menubar/file/b2", "left2", "right2");
                GUI_PopupSeparator(4);
                GUI_PopupMenuButton("menubar/file/b3", "left3", "right3");
                GUI_PopupMenuButton("menubar/file/b4", "left4", "right4");
                GUI_PopupSubMenuButton("menubar/file/activate_sub1", &sub1);
                GUI_PopupSubMenuButton("menubar/file/activate_sub2", &sub2);
            GUI_EndPopupMenu();
        }

        if(sub1.active)
        {
            GUI_BeginPopupMenu("menubar/file/sub1", 100, 16+16+16+16+16+4, 100, 200, 16, &sub1);
                GUI_PopupMenuButton("menubar/file/sub1/b1", "left1", "right2");
                GUI_PopupMenuButton("menubar/file/sub1/b2", "left2", "right2");
            GUI_EndPopupMenu();
        }

        if(sub2.active)
        {
            GUI_BeginPopupMenu("menubar/file/sub2", 100, 16+16+16+16+16+16+4, 100, 200, 16, &sub2);
                GUI_PopupMenuButton("menubar/file/sub2/b1", "left1", "right1");
            GUI_EndPopupMenu();
        }

        if(edit_root.active)
        {
            GUI_BeginPopupMenu("menubar/edit", 64, 16, 100, 200, 16, &file_root);
                GUI_PopupMenuButton("menubar/edit/b1", "copy", "Ctrl+c");
                GUI_PopupMenuButton("menubar/edit/b2", "pasta", "");
            GUI_EndPopupMenu();
        }
        GUI_EndPopupGroup();
    GUI_EndDropMenu();
}

