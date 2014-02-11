/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Last modified by Sean Hunt, 2014-02-11 */
/* Copyright (c) Daniel Thaler, 2011.                             */
/* NetHack may be freely redistributed.  See license for details. */

#include "nhcurses.h"


static WINDOW *invwin, *objwin;
static struct nh_objlist flooritems, inventory;

static nh_bool
curses_list_items_core(struct nh_objlist *objlist, nh_bool invent, nh_bool draw)
{
    struct nh_objlist *list;

    if (invent) {
        list = &inventory;
    } else {
        list = &flooritems;
    }

    dealloc_objmenulist(list);

    if (objlist->icount) {
        list->items = malloc(objlist->icount * sizeof (struct nh_objitem));
        list->size = objlist->icount;
        list->icount = objlist->icount;
        memcpy(list->items, objlist->items,
               objlist->icount * sizeof (struct nh_objitem));
    }

    dealloc_objmenulist(objlist);

    if (draw)
        draw_sidebar();

    return ui_flags.draw_sidebar;
}


nh_bool
curses_list_items(struct nh_objlist *objlist, nh_bool invent)
{
    return curses_list_items_core(objlist, invent, TRUE);
}

nh_bool
curses_list_items_nonblocking(struct nh_objlist *objlist, nh_bool invent)
{
    return curses_list_items_core(objlist, invent, FALSE);
}


static int
count_omitted_items(struct nh_objlist *list, int pos)
{
    int omitted = 0;

    for (; pos < list->icount; pos++)
        if (list->items[pos].role == MI_NORMAL)
            omitted++;
    return omitted;
}


void
draw_sidebar(void)
{
    int flheight = 0, invheight = 0, invwh, flwh;

    if (!ui_flags.draw_sidebar)
        return;

    int sbwidth = getmaxx(sidebar);

    /* re-create the subwindows every time; they only exist for use by
       draw_objlist */
    if (objwin)
        delwin(objwin);
    if (invwin)
        delwin(invwin);
    objwin = invwin = NULL;

    werase(sidebar);
    /* Inventory and floor item list sizing: Each list "owns" half the sidebar,
       but will expand into unused space in the other half. If unused space
       remains, the inventory list is expanded to push the floor item list to
       the bottom of the screen. */
    if (flooritems.icount && inventory.icount) {
        invheight = min(inventory.icount + 1, ui_flags.viewheight / 2);
        flheight = min(flooritems.icount + 1, (ui_flags.viewheight - 1) / 2);
        /* if there is need and available space, expand the floor item list up
           */
        if (flooritems.icount + 1 > flheight &&
            invheight < (ui_flags.viewheight + 1) / 2)
            flheight =
                min(flooritems.icount + 1, ui_flags.viewheight - invheight - 1);
        /* assign all unused space to the inventory list whether it is needed
           or not */
        invheight = ui_flags.viewheight - flheight - 1;
        mvwhline(sidebar, invheight, 0, ACS_HLINE, sbwidth);
    } else if (flooritems.icount)
        flheight = ui_flags.viewheight;
    else
        invheight = ui_flags.viewheight;

    if (invheight) {
        wattron(sidebar, A_UNDERLINE);
        mvwaddstr(sidebar, 0, 0, "Inventory:");
        wattroff(sidebar, A_UNDERLINE);

        invwh = invheight - 1;
        if (invwh < inventory.icount) {
            invwh--;
            mvwprintw(sidebar, invheight - 1, 0, "(%d more omitted)",
                      count_omitted_items(&inventory, invwh));
        }
        invwin = derwin(sidebar, invwh, sbwidth, 1, 0);
        draw_objlist(invwin, &inventory, NULL, PICK_NONE);
    }

    if (flheight) {
        wattron(sidebar, A_UNDERLINE);
        mvwaddstr(sidebar, invheight ? invheight + 1 : 0, 0,
                  "Things that are here:");
        wattroff(sidebar, A_UNDERLINE);

        flwh = flheight - 1;
        if (flwh < flooritems.icount) {
            flwh--;
            mvwprintw(sidebar, ui_flags.viewheight - 1, 0, "(%d more omitted)",
                      count_omitted_items(&flooritems, flwh));
        }

        objwin =
            derwin(sidebar, flwh, sbwidth, invheight ? invheight + 2 : 1, 0);
        draw_objlist(objwin, &flooritems, NULL, PICK_NONE);
    }

    wnoutrefresh(sidebar);
}


void
cleanup_sidebar(nh_bool dealloc)
{
    if (dealloc) {
        dealloc_objmenulist(&flooritems);
        dealloc_objmenulist(&inventory);
    }
    delwin(objwin);
    delwin(invwin);
    objwin = invwin = NULL;
}
