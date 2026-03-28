#include "pch.h"
#include "hacks_vars.h"
#include "hacks_core.h"

namespace
{
class open_hacks_mainmenu_commands : public mainmenu_commands
{
public:
    enum
    {
        cmd_show_main_menu = 0,
        cmd_show_status_bar,
        cmd_maximize,
        cmd_restore,
        cmd_total
    };

    t_uint32 get_command_count() override
    {
        return OpenHacksCore::Get().HasInitError() ? 0 : cmd_total;
    }

    GUID get_command(t_uint32 p_index) override
    {
        pfc::string8_fast name;
        get_name(p_index, name);
        name += pfc::print_guid(get_parent());
        return hasher_md5::get()->process_single_string(name).asGUID();
    }

    void get_name(t_uint32 p_index, pfc::string_base& p_out) override
    {
        switch (p_index)
        {
        case cmd_show_main_menu:
            p_out = "Show main menu";
            break;

        case cmd_show_status_bar:
            p_out = "Show status bar";
            break;

        case cmd_maximize:
            p_out = "Maximize";
            break;

        case cmd_restore:
            p_out = "Restore";
            break;

        default:
            uBugCheck();
        }
    }

    bool get_description(t_uint32 p_index, pfc::string_base& p_out) override
    {
        return false;
    }

    GUID get_parent() override
    {
        return mainmenu_groups::view;
    }

    void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) override
    {
        switch (p_index)
        {
        case cmd_show_main_menu:
            OpenHacksCore::Get().ToggleMenuBar();
            break;

        case cmd_show_status_bar:
            OpenHacksCore::Get().ToggleStatusBar();
            break;

        case cmd_maximize:
            ShowWindow(core_api::get_main_window(), SW_MAXIMIZE);
            break;

        case cmd_restore:
            ShowWindow(core_api::get_main_window(), SW_RESTORE);
            break;

        default:
            uBugCheck();
        }
    }

    bool get_display(t_uint32 p_index, pfc::string_base& p_text, t_uint32& p_flags) override
    {
        // OPTIONAL method
        bool rv = mainmenu_commands::get_display(p_index, p_text, p_flags);
        if (rv)
        {
            switch (p_index)
            {
            case cmd_show_main_menu:
                p_flags |= flag_defaulthidden;
                p_flags |= (OpenHacksVars::ShowMainMenu ? flag_checked : 0);
                break;

            case cmd_show_status_bar:
                p_flags |= flag_defaulthidden;
                p_flags |= (OpenHacksVars::ShowStatusBar ? flag_checked : 0);
                break;

            case cmd_maximize:
            case cmd_restore:
                {
                    p_flags |= flag_defaulthidden;
                    HWND mainWindow = core_api::get_main_window();
                    WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
                    GetWindowPlacement(mainWindow, &wp);
                    bool isMaximized = (wp.showCmd == SW_SHOWMAXIMIZED);
                    bool isMinimized = (wp.showCmd == SW_SHOWMINIMIZED);
                    // Maximize: disabled when already maximized
                    // Restore: disabled when window is normal (not maximized, not minimized)
                    if (p_index == cmd_maximize && isMaximized)
                        p_flags |= flag_disabled;
                    else if (p_index == cmd_restore && !isMaximized && !isMinimized)
                        p_flags |= flag_disabled;
                }
                break;

            default:
                break;
            }
        }

        return rv;
    }
};

static mainmenu_commands_factory_t<open_hacks_mainmenu_commands> g_open_hacks_mainmenu_commands_factory;
} // namespace