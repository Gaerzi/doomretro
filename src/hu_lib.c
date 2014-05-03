/*
====================================================================

DOOM RETRO
The classic, refined DOOM source port. For Windows PC.

Copyright (C) 1993-1996 id Software LLC, a ZeniMax Media company.
Copyright (C) 2005-2014 Simon Howard.
Copyright (C) 2013-2014 Brad Harding.

This file is part of DOOM RETRO.

DOOM RETRO is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

DOOM RETRO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with DOOM RETRO. If not, see http://www.gnu.org/licenses/.

====================================================================
*/

#include "hu_lib.h"
#include "i_swap.h"
#include "r_local.h"
#include "v_data.h"
#include "v_video.h"

extern boolean  automapactive;
extern boolean  widescreen;
extern boolean  translucency;

void HUlib_clearTextLine(hu_textline_t *t)
{
    t->len = 0;
    t->l[0] = 0;
    t->needsupdate = true;
}

void HUlib_initTextLine(hu_textline_t *t, int x, int y, patch_t **f, int sc)
{
    t->x = x;
    t->y = y;
    t->f = f;
    t->sc = sc;
    HUlib_clearTextLine(t);
}

boolean HUlib_addCharToTextLine(hu_textline_t *t, char ch)
{
    if (t->len == HU_MAXLINELENGTH)
        return false;
    else
    {
        t->l[t->len++] = ch;
        t->l[t->len] = 0;
        t->needsupdate = 4;
        return true;
    }
}

boolean HUlib_delCharFromTextLine(hu_textline_t *t)
{
    if (!t->len)
        return false;
    else
    {
        t->l[--t->len] = 0;
        t->needsupdate = 4;
        return true;
    }
}

static void HU_drawDot(int x, int y, char src)
{
    byte        *dest = &tempscreen[y * SCREENWIDTH + x];

    if (src == '�')
        *dest = 0;
    else if (src != ' ')
        *dest = src;
}

// [BH] draw an individual character to temporary buffer
void HU_drawChar(int x, int y, int i)
{
    int         w = strlen(smallcharset[i]) / 10;
    int         x1, y1;

    for (y1 = 0; y1 < 10; y1++)
        for (x1 = 0; x1 < w; x1++)
        {
            char        src = smallcharset[i][y1 * w + x1];
            int         i = (x + x1) * 2;
            int         j = (y + y1) * 2;

            HU_drawDot(i, j, src);
            HU_drawDot(++i, j, src);
            HU_drawDot(i, ++j, src);
            HU_drawDot(--i, j, src);
        }
}

static struct
{
    char        char1;
    char        char2;
    int         adjust;
} kern[] = {
    { '.', '1', -1 },
    { '.', '7', -1 },
    { ',', '1', -1 },
    { ',', '7', -1 },
    { ',', 'Y', -1 },
    { 'T', '.', -1 },
    { 'T', ',', -1 },
    { 'Y', '.', -1 },
    { 'Y', ',', -1 }
};

void HUlib_drawTextLine(hu_textline_t *l)
{
    int                 i;
    int                 w = 0;
    int                 tw = 0;
    int                 x, y;
    int                 xx, yy;
    unsigned char       c;
    static char         prev = '\0';

    // draw the new stuff
    x = l->x;
    y = l->y;
    memset(tempscreen, 251, SCREENWIDTH * SCREENHEIGHT);
    for (i = 0; i < l->len; i++)
    {
        c = toupper(l->l[i]);
        if (c == '\n')          // [BH] allow multiple lines
        {
            x = l->x;
            y += 9;
        }
        else if (c != ' ' && c >= l->sc && c <= '_')
        {
            int j = c - '!';
            int k = 0;

            // [BH] have matching curly single and double quotes
            if (!i || (i > 0 && l->l[i - 1] == ' '))
            {
                if (c == '\"')
                    j = 64;
                else if (c == '\'' || c == '\u2019')
                    j = 65;
            }

            if (STCFN034)
            {
                // [BH] display lump from PWAD with shadow
                w = l->f[c - l->sc]->width;
                V_DrawPatchWithShadow(x, l->y, 0, l->f[c - l->sc], false);
            }
            else
            {
                // [BH] apply kerning to certain character pairs
                while (kern[k].char1)
                {
                    if (prev == kern[k].char1 && c == kern[k].char2)
                        x += kern[k].adjust;
                    k++;
                }

                // [BH] draw individual character
                w = strlen(smallcharset[j]) / 10 - 1;
                HU_drawChar(x, y - 1, j);
            }
            x += w;

            prev = c;
        }
        else
        {
            w = (i > 0 && (l->l[i - 1] == '.' || l->l[i - 1] == '!' || l->l[i - 1] == '?') ? 5 : 3);
            x += w;
        }
        tw += w;
    }

    if (STCFN034)
        return;

    // [BH] draw underscores for IDBEHOLD cheat message
    if (idbehold)
    {
        int     x1, y1, x2, y2;

        for (y1 = 0; y1 < 4; y1++)
            for (x1 = 0; x1 < ORIGINALWIDTH; x1++)
            {
                char src = (automapactive && !widescreen ? underscores2[y1 * ORIGINALWIDTH + x1] :
                                                           underscores1[y1 * ORIGINALWIDTH + x1]);

                for (y2 = 0; y2 < SCREENSCALE; y2++)
                    for (x2 = 0; x2 < SCREENSCALE; x2++)
                    {
                        byte *dest = &tempscreen[((8 + y1) * SCREENSCALE + y2) * SCREENWIDTH +
                                                 x1 * SCREENSCALE + x2];

                        if (src == '�')
                            *dest = 0;
                        else if (src != ' ')
                            *dest = src;
                    }
            }
    }

    // [BH] draw entire message from buffer onto screen with translucency
    for (yy = l->y - 1; yy < (y + 10) * SCREENSCALE; yy++)
        for (xx = l->x; xx < (l->x + tw + 1) * SCREENSCALE; xx++)
        {
            int         dot = yy * SCREENWIDTH + xx;
            byte        *source = &tempscreen[dot];
            byte        *dest1 = &screens[0][dot];
            byte        *dest2 = &screens[screensize < 7 && !automapactive][dot];

            if (!*source)
                *dest1 = tinttab50[*dest2];
            else if (*source != 251)
            {
                byte color = *source;

                if (translucency)
                {
                    color = tinttab33[(*dest2 << 8) + color];
                    if (color >= 168 && color <= 175)
                        color -= 144;
                }
                *dest1 = color;
            }
        }
}

// sorta called by HU_Erase and just better darn get things straight
void HUlib_eraseTextLine(hu_textline_t *l)
{
    int         lh;
    int         y;
    int         yoffset;

    // Only erases when NOT in automap and the screen is reduced,
    // and the text must either need updating or refreshing
    // (because of a recent change back from the automap)

    if (!automapactive && viewwindowx && l->needsupdate)
    {
        lh = (SHORT(l->f[0]->height) + 4) * SCREENSCALE;
        for (y = l->y, yoffset = y * SCREENWIDTH; y < l->y + lh; y++, yoffset += SCREENWIDTH)
        {
            if (y < viewwindowy || y >= viewwindowy + viewheight)
                R_VideoErase(yoffset, SCREENWIDTH);                             // erase entire line
            else
            {
                R_VideoErase(yoffset, viewwindowx);                             // erase left border
                R_VideoErase(yoffset + viewwindowx + viewwidth, viewwindowx);   // erase right border
            }
        }
    }

    if (l->needsupdate)
        l->needsupdate--;
}

void HUlib_initSText(hu_stext_t *s, int x, int y, int h, patch_t **font, int startchar, boolean *on)
{
    int         i;

    s->h = h;
    s->on = on;
    s->laston = true;
    s->cl = 0;
    for (i = 0; i < h; i++)
        HUlib_initTextLine(&s->l[i], x, y - i * (SHORT(font[0]->height) + 1), font, startchar);
}

void HUlib_addLineToSText(hu_stext_t *s)
{
    int         i;

    // add a clear line
    if (++s->cl == s->h)
        s->cl = 0;
    HUlib_clearTextLine(&s->l[s->cl]);

    // everything needs updating
    for (i = 0; i < s->h; i++)
        s->l[i].needsupdate = 4;
}

void HUlib_addMessageToSText(hu_stext_t *s, char *prefix, char *msg)
{
    HUlib_addLineToSText(s);
    if (prefix)
        while (*prefix)
            HUlib_addCharToTextLine(&s->l[s->cl], *(prefix++));

    while (*msg)
        HUlib_addCharToTextLine(&s->l[s->cl], *(msg++));
}

void HUlib_drawSText(hu_stext_t *s)
{
    int                 i, idx;
    hu_textline_t       *l;

    if (!*s->on)
        return; // if not on, don't draw

    // draw everything
    for (i = 0; i < s->h; i++)
    {
        idx = s->cl - i;
        if (idx < 0)
            idx += s->h;        // handle queue of lines

        l = &s->l[idx];

        // need a decision made here on whether to skip the draw
        HUlib_drawTextLine(l); // no cursor, please
    }
}

void HUlib_eraseSText(hu_stext_t *s)
{
    int         i;

    for (i = 0; i < s->h; i++)
    {
        if (s->laston && !*s->on)
            s->l[i].needsupdate = 4;
        HUlib_eraseTextLine(&s->l[i]);
    }
    s->laston = *s->on;
}

void HUlib_initIText(hu_itext_t *it, int x, int y, patch_t **font, int startchar, boolean *on)
{
    it->lm = 0;                 // default left margin is start of text
    it->on = on;
    it->laston = true;
    HUlib_initTextLine(&it->l, x, y, font, startchar);
}

// The following deletion routines adhere to the left margin restriction
void HUlib_delCharFromIText(hu_itext_t *it)
{
    if (it->l.len != it->lm)
        HUlib_delCharFromTextLine(&it->l);
}

void HUlib_eraseLineFromIText(hu_itext_t *it)
{
    while (it->lm != it->l.len)
        HUlib_delCharFromTextLine(&it->l);
}

// Resets left margin as well
void HUlib_resetIText(hu_itext_t *it)
{
    it->lm = 0;
    HUlib_clearTextLine(&it->l);
}

void HUlib_addPrefixToIText(hu_itext_t *it, char *str)
{
    while (*str)
        HUlib_addCharToTextLine(&it->l, *(str++));
    it->lm = it->l.len;
}

// wrapper function for handling general keyed input.
// returns true if it ate the key
boolean HUlib_keyInIText(hu_itext_t *it, unsigned char ch)
{
    ch = toupper(ch);

    if (ch >= ' ' && ch <= '_')
        HUlib_addCharToTextLine(&it->l, (char)ch);
    else if (ch == KEY_BACKSPACE)
        HUlib_delCharFromIText(it);
    else if (ch != KEY_ENTER)
        return false; // did not eat key

    return true; // ate the key
}

void HUlib_drawIText(hu_itext_t *it)
{
    hu_textline_t       *l = &it->l;

    if (!*it->on)
        return;
    HUlib_drawTextLine(l);
}

void HUlib_eraseIText(hu_itext_t *it)
{
    if (it->laston && !*it->on)
        it->l.needsupdate = 4;
    HUlib_eraseTextLine(&it->l);
    it->laston = *it->on;
}
