#ifndef RESOURCE_H
#define RESOURCE_H

// resource.h: the numbers of the controls of the window and the icon in res/gui.rc.
// Ids from 5000 up are taken by the "empty list" messages of the lists (list id plus 5000).

#define IDI_APP 1

// the main window
#define IDC_NAV 2000
#define IDC_SWITCHUSER 2001
#define IDC_STATUSBAR 2002

// the welcome page: three role buttons, then the name list with Back and Continue
#define IDC_W_COORD 2010
#define IDC_W_DONOR 2011
#define IDC_W_RECIP 2012
#define IDC_W_NAMES 2013
#define IDC_W_CONTINUE 2014
#define IDC_W_BACK 2015

// the donor pages (only the Add donation form has controls of its own)
#define IDC_D_FOOD 2101
#define IDC_D_QTY 2102
#define IDC_D_USABLE 2103
#define IDC_D_ERROR 2104
#define IDC_D_CANCEL 2105
#define IDC_D_SAVE 2106

// the coordinator pages and the match page
#define IDC_C_PROCESS_SEL 2201
#define IDC_C_CLOSE_SEL 2202
#define IDC_C_WITHDRAW 2203
#define IDC_M_QTY 2210
#define IDC_M_ERROR 2211
#define IDC_M_BACK 2212
#define IDC_M_SEND 2214

// the recipient pages
#define IDC_R_ACCEPT 2301
#define IDC_R_DECLINE 2302
#define IDC_R_CAPACITY 2310
#define IDC_R_ACCEPTING 2311
#define IDC_R_PICKUP 2312
#define IDC_R_ERROR 2313
#define IDC_R_SAVE 2315

#endif
