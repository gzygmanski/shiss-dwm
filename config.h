/* See LICENSE file for copyright and license details. */
#include "movestack.c"
#include "tcl.c"
/* appearance */
static const unsigned int borderpx  = 2;        /* border pixel of windows */
static const unsigned int gappx     = 0;       /* gap pixel between windows */
static const unsigned int snap      = 5;       /* snap pixel */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 1;        /* 0 means bottom bar */
static const int vertpad            = 0;
static const int sidepad            = 0;
static const int horizpadbar        = 8;        /* horizontal padding for statusbar */
static const int vertpadbar         = 8;        /* vertical padding for statusbar */
static const char *fonts[]          = { "sans:pixelsize=14:antialias=true:autohint=true:style=regular" };
static const char dmenufont[]       = "sans:pixelsize=14:antialias=true:autohint=true:style=regular";
static const char nord1[]           = "#d08770";
static const char nord2[]           = "#3b4252";
static const char nord3[]           = "#434c5e";
static const char nord4[]           = "#4c566a";
static const char nord5[]           = "#e5e9f0";
static const char nord6[]           = "#bf616a";
static const unsigned int baralpha = 0xff;
static const unsigned int borderalpha = OPAQUE;
static const char *colors[][3]      = {
	/*                   fg         bg         border   */
	[SchemeNorm]     = { nord5,     nord2,     nord2},
	[SchemeSel]      = { nord5,     nord3,     nord6},
	[SchemeStatus]   = { nord2,     nord5,     nord1},
	[SchemeTagsSel]  = { nord5,     nord3,     nord1},
	[SchemeTagsNorm] = { nord5,     nord2,     nord1},
	[SchemeInfoSel]  = { nord5,     nord4,     nord1},
	[SchemeInfoNorm] = { nord5,     nord2,     nord1},
};
static const unsigned int alphas[][3]      = {
	/*                   fg      bg        border     */
	[SchemeNorm]     = { OPAQUE, baralpha, borderalpha },
	[SchemeSel]      = { OPAQUE, baralpha, borderalpha },
	[SchemeStatus]   = { OPAQUE, baralpha, borderalpha },
	[SchemeTagsSel]  = { OPAQUE, baralpha, borderalpha },
	[SchemeTagsNorm] = { OPAQUE, baralpha, borderalpha },
	[SchemeInfoSel]  = { OPAQUE, baralpha, borderalpha },
	[SchemeInfoNorm] = { OPAQUE, baralpha, borderalpha },
};
/* tagging */
// japanese
static const char *tags[] = { "一", "二", "三", "四", "五", "六", "七", "八", "九" };
// roman
/* static const char *tags[] = { "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX" }; */
// arabic
/* static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" }; */
static const char *tagsalt[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class      		instance            title           tags mask     isfloating   monitor */
	{ NULL,  	        NULL,               "Friends List", 1,            0,            1 },
	{ NULL,	  	        NULL,               "Steam",        1 << 2,       0,            0 },
	{ "discord",  	        NULL,               NULL,           1,            0,            1 },
	{ "TeamSpeak 3",        NULL,               NULL,           1,            0,            1 },
	/* { "qutebrowser",  	NULL,               NULL,           1 << 1,       0,           -1 }, */
	{ "mpv",  	        "mpv-center",       NULL,           0,            1,           -1 },
};

/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "﩯 t",      tile },    /* first entry is default */
	{ " f",      NULL },    /* no layout function means floating behavior */
	{ "ﱢ m",      monocle },
	{ "恵 c",      tcl},
};

/* key definitions */
#define MODKEY Mod1Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
/* under bar */
/* static const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", nord1, "-nf", nord5, "-sb", nord6, "-sf", nord5, "-x", "0", "-y", "23", "-w", "1920", "-h", "23", NULL }; */
/* on top of bar */
static const char *dmenucmd[] = { "dmenu_run", "-p", "  Run:", "-m", dmenumon, "-fn", dmenufont, "-nb", nord2, "-nf", nord5, "-sb", nord6, "-sf", nord5, "-x", "0", "-y", "0", "-w", "1920", "-h", "27", NULL };
/* static const char *tvwatchcmd[] = { "tvwatch", "  Watch:", dmenumon, dmenufont, nord2, nord5, nord6, nord5, "20", "0", "1880", "25", NULL }; */
static const char *termcmd[]  = { "st", NULL };
static const char *browsercmd[] = { "qutebrowser", NULL };
static const char *ncmpcppcmd[] = { "st", "-e", "ncmpcpp", NULL };
static const char *newsboatcmd[] = { "st", "-e", "newsboat", NULL };
static const char *lfcmd[] = { "st", "-e", "lf", NULL };

/* mpv-pip commands */
static const char *mpvpipup[]   = { "mpv-pip", "--disable-watcher", "--up", NULL };
static const char *mpvpipdown[] = { "mpv-pip", "--disable-watcher", "--down", NULL };
static const char *mpvpipleft[] = { "mpv-pip", "--disable-watcher", "--left", NULL };
static const char *mpvpipcenter[] = { "mpv-pip", "--center", NULL };
static const char *mpvpiptoggle[] = { "mpv-pip", "--toggle", NULL };
static const char *mpvpipforward[] = { "mpv-pip", "--seek-forward", NULL };
static const char *mpvpipbackward[] = { "mpv-pip", "--seek-backward", NULL };
static const char *mpvpipwatcher[] = { "mpv-pip", "--toggle-watcher", NULL };
static const char *mpvpipclose[] = { "mpv-pip", "--close", NULL };

/* mpc commands */
static const char *mpdtoggle[]	= { "mpc", "toggle", NULL };
static const char *mpdstop[]	= { "mpc", "stop", NULL };
static const char *mpdnext[]	= { "mpc", "next", NULL };
static const char *mpdprev[]	= { "mpc", "prev", NULL };
static const char *mpdvolp[]	= { "volpm", "-s", "mpd", "-i", "2", NULL };
static const char *mpdvolm[]	= { "volpm", "-s", "mpd", "-d", "2", NULL };
static const char *mpdseekp[]	= { "mpc", "seek", "+2%", NULL };
static const char *mpdseekm[]	= { "mpc", "seek", "-2%", NULL };
static const char *mpdinfo[]    = { "mpdinfo", NULL };
static const char *mpdrate0[]   = { "mpd-rate", "0", NULL };
static const char *mpdrate1[]   = { "mpd-rate", "1", NULL };
static const char *mpdrate2[]   = { "mpd-rate", "2", NULL };
static const char *mpdrate3[]   = { "mpd-rate", "3", NULL };
static const char *mpdrate4[]   = { "mpd-rate", "4", NULL };
static const char *mpdrate5[]   = { "mpd-rate", "5", NULL };
static const char *mpdfav0[]    = { "mpd-fav", "0", NULL };
static const char *mpdfav1[]    = { "mpd-fav", "1", NULL };
static const char *mpdfav2[]    = { "mpd-fav", "2", NULL };


/* volume commands */
static const char *volp[]	= { "volpm", "-s", "default", "-i", "2", NULL };
static const char *volm[]	= { "volpm", "-s", "default", "-d", "2", NULL };

static Key keys[] = {
	/* modifier                     key        function        argument */
	{ MODKEY,                       XK_p,      spawn,          {.v = dmenucmd } },
	/* { MODKEY,                       XK_w,      spawn,          {.v = tvwatchcmd } }, */
	{ MODKEY,	                XK_Return, spawn,          {.v = termcmd } },
	{ MODKEY,	                XK_w,	   spawn,          {.v = browsercmd} },
	{ MODKEY|ShiftMask,             XK_m, 	   spawn,          {.v = ncmpcppcmd} },
	{ MODKEY|ShiftMask,             XK_r, 	   spawn,          {.v = newsboatcmd} },
	{ MODKEY|ShiftMask,             XK_f, 	   spawn,          {.v = lfcmd} },
	{ MODKEY,                       XK_b,      togglebar,      {0} },
	{ MODKEY|ShiftMask,             XK_j,      movestack,      {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_k,      movestack,      {.i = -1 } },
        { MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
	{ MODKEY,                       XK_i,      incnmaster,     {.i = +1 } },
	{ MODKEY,                       XK_d,      incnmaster,     {.i = -1 } },
	{ MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
	{ MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
	{ MODKEY|ShiftMask,             XK_h,      setcfact,       {.f = +0.25} },
	{ MODKEY|ShiftMask,             XK_l,      setcfact,       {.f = -0.25} },
	{ MODKEY|ShiftMask,             XK_o,      setcfact,       {.f =  0.00} },
	{ MODKEY|ShiftMask,             XK_Return, zoom,           {0} },
	{ MODKEY,                       XK_Tab,    view,           {0} },
	{ MODKEY,	                XK_q,      killclient,     {0} },
	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[0]} },
	{ MODKEY,                       XK_f,      setlayout,      {.v = &layouts[1]} },
	{ MODKEY,                       XK_m,      setlayout,      {.v = &layouts[2]} },
        { MODKEY,                       XK_c,      setlayout,      {.v = &layouts[3]} },
        { MODKEY|ControlMask,           XK_m,      fullscreen,     {0} },
	{ MODKEY,                       XK_space,  setlayout,      {0} },
	{ MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
	{ MODKEY,                       XK_0,      view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,      tag,            {.ui = ~0 } },
	{ MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_period, focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },
	{ MODKEY,                       XK_n,      togglealttag,   {0} },
	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
	TAGKEYS(                        XK_7,                      6)
	TAGKEYS(                        XK_8,                      7)
	TAGKEYS(                        XK_9,                      8)
	{ MODKEY|ShiftMask,             XK_q,      quit,           {0} },

        /* mpv pip control */
	{ MODKEY,			XK_Prior,       		spawn,	{.v = mpvpipup } },
	{ MODKEY,			XK_Next,         		spawn,	{.v = mpvpipdown } },
	{ MODKEY,			XK_Home,         		spawn,	{.v = mpvpipleft } },
	{ MODKEY,			XK_End,         		spawn,	{.v = mpvpipcenter } },
	{ MODKEY,       		XK_Insert,          		spawn,	{.v = mpvpiptoggle } },
	{ MODKEY|ShiftMask,   		XK_Home,       		spawn,	{.v = mpvpipforward } },
	{ MODKEY|ShiftMask,    		XK_End,        		spawn,	{.v = mpvpipbackward } },
	{ MODKEY,			XK_backslash,         		spawn,	{.v = mpvpipwatcher } },
	{ MODKEY,			XK_Delete,         		spawn,	{.v = mpvpipclose } },

	/* mediakeys mpd controls */
	{ 0,				XF86XK_AudioPlay,		spawn,	{.v = mpdtoggle } },
	{ 0,				XF86XK_AudioStop,		spawn,	{.v = mpdstop } },
	{ 0,				XF86XK_AudioNext,		spawn,	{.v = mpdnext } },
	{ 0,				XF86XK_AudioPrev,		spawn,	{.v = mpdprev } },
	{ 0,				XF86XK_AudioRaiseVolume,	spawn,	{.v = mpdvolp } },
	{ 0,				XF86XK_AudioLowerVolume,	spawn,	{.v = mpdvolm } },
	{ MODKEY,			XK_bracketright,		spawn,	{.v = mpdseekp } },
	{ MODKEY,			XK_bracketleft,			spawn,	{.v = mpdseekm } },
	{ MODKEY,			XK_slash,			spawn,	{.v = mpdinfo } },

	/* volume controls */
	{ MODKEY,		        XK_equal,			spawn,	{.v = volp } },
	{ MODKEY,		        XK_minus,			spawn,	{.v = volm } },
        { MODKEY,                       XK_F1,                          spawn,  {.v = mpdrate1 } },
        { MODKEY,                       XK_F2,                          spawn,  {.v = mpdrate2 } },
        { MODKEY,                       XK_F3,                          spawn,  {.v = mpdrate3 } },
        { MODKEY,                       XK_F4,                          spawn,  {.v = mpdrate4 } },
        { MODKEY,                       XK_F5,                          spawn,  {.v = mpdrate5 } },
        { MODKEY,                       XK_F6,                          spawn,  {.v = mpdfav0 } },
        { MODKEY,                       XK_F7,                          spawn,  {.v = mpdfav1 } },
        { MODKEY,                       XK_F9,                          spawn,  {.v = mpdrate0 } },
        { MODKEY,                       XK_F10,                         spawn,  {.v = mpdfav2 } },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         MODKEY|ShiftMask,      Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY|ShiftMask,      Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY|ShiftMask,      Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY|ShiftMask,      Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY|ShiftMask,      Button3,        toggletag,      {0} },
};

