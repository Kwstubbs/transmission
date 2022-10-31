// This file copyright (C) Transmission authors and contributors.
// It may be used under the MIT (SPDX: MIT) license.
// License text can be found in the licenses/ folder.

#include <glibmm.h>
#include <glibmm/i18n.h>

#include <libtransmission/transmission.h>

#include <libtransmission/utils.h>
#include <libtransmission/variant.h>

#include "Prefs.h"
#include "Utils.h"

using namespace std::literals;

namespace
{

std::string gl_confdir;

} // namespace

void gtr_pref_init(std::string_view config_dir)
{
    gl_confdir = config_dir;
}

/***
****
****  Preferences
****
***/

namespace
{

std::string get_default_download_dir()
{
    auto dir = Glib::get_user_special_dir(TR_GLIB_USER_DIRECTORY(DOWNLOAD));

    if (dir.empty())
    {
        dir = Glib::get_user_special_dir(TR_GLIB_USER_DIRECTORY(DESKTOP));
    }

    if (dir.empty())
    {
        dir = tr_getDefaultDownloadDir();
    }

    return dir;
}

tr_variant* getPrefs()
{
    static tr_variant settings;
    static bool loaded = false;

    if (!loaded)
    {
        tr_variantInitDict(&settings, 1);
        tr_variantDictAddStr(&settings, TR_KEY_download_dir, get_default_download_dir());
        tr_sessionLoadSettings(&settings, gl_confdir.c_str(), nullptr);
        loaded = true;
    }

    return &settings;
}

} // namespace

/***
****
***/

tr_variant* gtr_pref_get_all()
{
    return getPrefs();
}

int64_t gtr_pref_int_get(tr_quark const key)
{
    int64_t i;

    return tr_variantDictFindInt(getPrefs(), key, &i) ? i : 0;
}

void gtr_pref_int_set(tr_quark const key, int64_t value)
{
    tr_variantDictAddInt(getPrefs(), key, value);
}

double gtr_pref_double_get(tr_quark const key)
{
    double d;

    return tr_variantDictFindReal(getPrefs(), key, &d) ? d : 0.0;
}

void gtr_pref_double_set(tr_quark const key, double value)
{
    tr_variantDictAddReal(getPrefs(), key, value);
}

/***
****
***/

bool gtr_pref_flag_get(tr_quark const key)
{
    bool boolVal;

    return tr_variantDictFindBool(getPrefs(), key, &boolVal) ? boolVal : false;
}

void gtr_pref_flag_set(tr_quark const key, bool value)
{
    tr_variantDictAddBool(getPrefs(), key, value);
}

/***
****
***/

std::string gtr_pref_string_get(tr_quark const key)
{
    auto sv = std::string_view{};
    (void)tr_variantDictFindStrView(getPrefs(), key, &sv);
    return std::string{ sv };
}

void gtr_pref_string_set(tr_quark const key, std::string_view value)
{
    tr_variantDictAddStr(getPrefs(), key, value);
}

/***
****
***/

void gtr_pref_save(tr_session* session)
{
    tr_sessionSaveSettings(session, gl_confdir.c_str(), getPrefs());
}
