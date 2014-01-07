/*
 * plugins.c
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
 *
 * This file is part of Profanity.
 *
 * Profanity is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Profanity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "config/preferences.h"
#include "log.h"
#include "plugins/callbacks.h"
#include "plugins/api.h"
#include "plugins/plugins.h"

#ifdef PROF_HAVE_PYTHON
#include "plugins/python_plugins.h"
#include "plugins/python_api.h"
#endif

#ifdef PROF_HAVE_RUBY
#include "plugins/ruby_plugins.h"
#include "plugins/ruby_api.h"
#endif

#ifdef PROF_HAVE_LUA
#include "plugins/lua_plugins.h"
#include "plugins/lua_api.h"
#endif

#ifdef PROF_HAVE_C
#include "plugins/c_plugins.h"
#include "plugins/c_api.h"

#endif
#include "ui/ui.h"

static GSList* plugins;

void
plugins_init(void)
{
    plugins = NULL;

#ifdef PROF_HAVE_PYTHON
    python_env_init();
#endif
#ifdef PROF_HAVE_RUBY
    ruby_env_init();
#endif
#ifdef PROF_HAVE_LUA
    lua_env_init();
#endif
#ifdef PROF_HAVE_C
    c_env_init();
#endif

    // load plugins
    gchar **plugins_load = prefs_get_plugins();
    if (plugins_load != NULL) {
        int i;
        for (i = 0; i < g_strv_length(plugins_load); i++)
        {
            gboolean loaded = FALSE;
            gchar *filename = plugins_load[i];
#ifdef PROF_HAVE_PYTHON
            if (g_str_has_suffix(filename, ".py")) {
                ProfPlugin *plugin = python_plugin_create(filename);
                if (plugin != NULL) {
                    plugins = g_slist_append(plugins, plugin);
                    loaded = TRUE;
                }
            }
#endif
#ifdef PROF_HAVE_RUBY
            if (g_str_has_suffix(filename, ".rb")) {
                ProfPlugin *plugin = ruby_plugin_create(filename);
                if (plugin != NULL) {
                    plugins = g_slist_append(plugins, plugin);
                    loaded = TRUE;
                }
            }
#endif
#ifdef PROF_HAVE_LUA
            if (g_str_has_suffix(filename, ".lua")) {
                ProfPlugin *plugin = lua_plugin_create(filename);
                if (plugin != NULL) {
                    plugins = g_slist_append(plugins, plugin);
                    loaded = TRUE;
                }
            }
#endif
#ifdef PROF_HAVE_C
            if (g_str_has_suffix(filename, ".so")) {
                ProfPlugin *plugin = c_plugin_create(filename);
                if (plugin != NULL) {
                    plugins = g_slist_append(plugins, plugin);
                    loaded = TRUE;
                }
            }
#endif
            if (loaded == TRUE) {
                log_info("Loaded plugin: %s", filename);
            }
        }

        // initialise plugins
        GSList *curr = plugins;
        while (curr != NULL) {
            ProfPlugin *plugin = curr->data;
            plugin->init_func(plugin, PROF_PACKAGE_VERSION, PROF_PACKAGE_STATUS);
            curr = g_slist_next(curr);
        }
    }

    return;
}

GSList *
plugins_get_list(void)
{
    return plugins;
}

char *
plugins_get_lang_string(ProfPlugin *plugin)
{
    switch (plugin->lang)
    {
        case LANG_PYTHON:
            return "Python";
        case LANG_RUBY:
            return "Ruby";
        case LANG_LUA:
            return "Lua";
        case LANG_C:
            return "C";
        default:
            return "Unknown";
    }
}

void
plugins_win_process_line(char *win, const char * const line)
{
    api_win_process_line(win, line);
}

void
plugins_on_start(void)
{
    GSList *curr = plugins;
    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        plugin->on_start_func(plugin);
        curr = g_slist_next(curr);
    }
}

void
plugins_on_connect(const char * const account_name, const char * const fulljid)
{
    GSList *curr = plugins;
    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        plugin->on_connect_func(plugin, account_name, fulljid);
        curr = g_slist_next(curr);
    }
}

void
plugins_on_disconnect(const char * const account_name, const char * const fulljid)
{
    GSList *curr = plugins;
    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        plugin->on_disconnect_func(plugin, account_name, fulljid);
        curr = g_slist_next(curr);
    }
}

char *
plugins_before_message_displayed(const char * const message)
{
    GSList *curr = plugins;
    char *new_message = NULL;
    char *curr_message = strdup(message);

    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->before_message_displayed_func(plugin, curr_message);
        if (new_message != NULL) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_slist_next(curr);
    }

    return curr_message;
}

char *
plugins_on_message_received(const char * const jid, const char *message)
{
    GSList *curr = plugins;
    char *new_message = NULL;
    char *curr_message = strdup(message);

    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->on_message_received_func(plugin, jid, curr_message);
        if (new_message != NULL) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_slist_next(curr);
    }

    return curr_message;
}

char *
plugins_on_private_message_received(const char * const room, const char * const nick,
    const char *message)
{
    GSList *curr = plugins;
    char *new_message = NULL;
    char *curr_message = strdup(message);

    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->on_private_message_received_func(plugin, room, nick, curr_message);
        if (new_message != NULL) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_slist_next(curr);
    }

    return curr_message;
}

char *
plugins_on_room_message_received(const char * const room, const char * const nick,
    const char *message)
{
    GSList *curr = plugins;
    char *new_message = NULL;
    char *curr_message = strdup(message);

    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->on_room_message_received_func(plugin, room, nick, curr_message);
        if (new_message != NULL) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_slist_next(curr);
    }

    return curr_message;
}

char *
plugins_on_message_send(const char * const jid, const char *message)
{
    GSList *curr = plugins;
    char *new_message = NULL;
    char *curr_message = strdup(message);

    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->on_message_send_func(plugin, jid, curr_message);
        if (new_message != NULL) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_slist_next(curr);
    }

    return curr_message;
}

char *
plugins_on_private_message_send(const char * const room, const char * const nick,
    const char * const message)
{
    GSList *curr = plugins;
    char *new_message = NULL;
    char *curr_message = strdup(message);

    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->on_private_message_send_func(plugin, room, nick, curr_message);
        if (new_message != NULL) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_slist_next(curr);
    }

    return curr_message;
}

char *
plugins_on_room_message_send(const char * const room, const char *message)
{
    GSList *curr = plugins;
    char *new_message = NULL;
    char *curr_message = strdup(message);

    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->on_room_message_send_func(plugin, room, curr_message);
        if (new_message != NULL) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_slist_next(curr);
    }

    return curr_message;
}

void
plugins_on_shutdown(void)
{
    GSList *curr = plugins;
    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        plugin->on_shutdown_func(plugin);
        curr = g_slist_next(curr);
    }
}

void
plugins_shutdown(void)
{
    GSList *curr = plugins;

    while (curr != NULL) {
#ifdef PROF_HAVE_PYTHON
        if (((ProfPlugin *)curr->data)->lang == LANG_PYTHON) {
            python_plugin_destroy(curr->data);
        }
#endif
#ifdef PROF_HAVE_RUBY
        if (((ProfPlugin *)curr->data)->lang == LANG_RUBY) {
            ruby_plugin_destroy(curr->data);
        }
#endif
#ifdef PROF_HAVE_LUA
        if (((ProfPlugin *)curr->data)->lang == LANG_LUA) {
            lua_plugin_destroy(curr->data);
        }
#endif
#ifdef PROF_HAVE_C
        if (((ProfPlugin *)curr->data)->lang == LANG_C) {
            c_plugin_destroy(curr->data);
        }
#endif

        curr = g_slist_next(curr);
    }
#ifdef PROF_HAVE_PYTHON
    python_shutdown();
#endif
#ifdef PROF_HAVE_RUBY
    ruby_shutdown();
#endif
#ifdef PROF_HAVE_LUA
    lua_shutdown();
#endif
#ifdef PROF_HAVE_C
    c_shutdown();
#endif
}

gchar *
plugins_get_dir(void)
{
    gchar *xdg_data = xdg_get_data_home();
    GString *plugins_dir = g_string_new(xdg_data);
    g_string_append(plugins_dir, "/profanity/plugins");
    gchar *result = strdup(plugins_dir->str);
    g_free(xdg_data);
    g_string_free(plugins_dir, TRUE);

    return result;
}
