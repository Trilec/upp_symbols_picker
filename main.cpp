// ============================================================================
// Material Symbols Icon Picker (SVG) for U++
//
// This is a GUI tool that lets you:
//   * Browse the Google Material Symbols icon sets (Outlined / Rounded / Sharp)
//   * Filter by category and name
//   * Drag icons into an "Export Bin"
//   * Export the bin contents as:
//         - Code iml format    (IML-style inline header: Upp::Image factories)
//         - Code Bin data      (raw RGBA inline header, no plugins required)
//         - SVG                (raw, minified SVG files)
//         - IML helper         (text manifest for building a .iml from PNGs)
//         - ICO                (multi-size .ico)
//         - PNG / JPG          (tinted raster images)
//
// Copy button behaviour:
//   * If "Code Bin data" is selected, it copies the Bin-data header.
//   * If "Code iml format" or any non-code export is selected, it copies the
//     IML-style header by default.
//   * A small label next to the Copy button shows which mode will be used,
//     e.g. [Copy Iml] or [Copy Bin].
//
// Requirements:
//   * Ultimate++ (U++) with CtrlLib, Draw, Painter, plugin/png, plugin/jpg, plugin/z
//   * The generated icon tables included below (icons_*.h)
//
// ============================================================================
// Changelog
// ============================================================================
//  v0.4
//    * Updated icon category mapping for the new generated headers/namespaces
//      (e.g. activitie, audio_video, map/place, ui_action, etc.).
//    * Reworked icon tinting to preserve alpha and apply a true tint color,
//      fixing color artifacts when choosing non-black tints.
//    * Renamed UI/control naming from "Icon Color" to "Tint Color"
//      (`currentTintColor`, `tintColorPusher`) for consistency.
//
//  v0.3
//    * Added dual code export modes:
//        - "Code iml format"  (IML-style inline header, Upp::Image factories)
//        - "Code Bin data"    (raw RGBA inline header, no compression)
//    * Copy button now reflects and uses the selected code mode.
//    * IML-style header clearly documents required includes and usage.
//    * General code/comment cleanup and small UI refinements.
//
//  v0.2
//    * (Earlier internal evolution of export modes and UI layout.)
//
//  v0.1
//    * Initial Material Symbols browser + basic export functionality.
//
// ============================================================================

#include <CtrlLib/CtrlLib.h>
#include <Painter/Painter.h>
#include <Draw/Draw.h>

#include <plugin/png/png.h>
#include <plugin/jpg/jpg.h>
#include <plugin/z/z.h>

#include <StageCard/StageCard.h>

// Generated icon tables (Material Symbols SVG extractor)
/*
#include "icons_action.h"
#include "icons_alert.h"
#include "icons_av.h"
#include "icons_communication.h"
#include "icons_content.h"
#include "icons_device.h"
#include "icons_editor.h"
#include "icons_file.h"
#include "icons_hardware.h"
#include "icons_home.h"
#include "icons_image.h"
#include "icons_maps.h"
#include "icons_navigation.h"
#include "icons_notification.h"
#include "icons_places.h"
#include "icons_search.h"
#include "icons_social.h"
#include "icons_toggle.h"
*/
#include "icons_action.h"
#include "icons_activitie.h"
#include "icons_alert.h"
#include "icons_android.h"
#include "icons_audio_video.h"
#include "icons_busines.h"
#include "icons_communicate.h"
#include "icons_communication.h"
#include "icons_content.h"
#include "icons_device.h"
#include "icons_editor.h"
#include "icons_file.h"
#include "icons_hardware.h"
#include "icons_home.h"
#include "icons_household.h"
#include "icons_image.h"
#include "icons_map.h"
#include "icons_navigation.h"
#include "icons_notification.h"
#include "icons_place.h"
#include "icons_privacy.h"
#include "icons_search.h"
#include "icons_social.h"
#include "icons_text.h"
#include "icons_toggle.h"
#include "icons_transit.h"
#include "icons_travel.h"
#include "icons_ui_action.h"

using namespace Upp;

// ============================================================================
// Version
// ============================================================================
static const char* APP_TITLE = "Material Symbols Icon Picker (SVG)";
static const char* APP_VER   = "v0.4";

// ============================================================================
// Theming helpers
// ============================================================================
static inline Color MainWindowColor()    { return Color(250, 247, 250); }
static inline Color ExitButtonColor()    { return Color(220, 38, 38); }
static inline Color ProcessButtonColor() { return Color(234, 88, 12); }
static inline Color CopyButtonColor()    { return Color(242, 243, 243); }
static inline Color IconsContentColor()  { return Color(240, 243, 246); }
static inline Color BinContentColor()    { return Color(252, 252, 235); }
static inline Color OutputContentColor() { return Color(31, 41, 55); }
static inline Color OutputTextColor()    { return Color(74, 202, 98); }
static inline Color TitleColor()         { return Color(234, 88, 12); }
static inline Color MainFrameColor()     { return Color(200, 200, 200); }
static inline Color MainGrayColor()      { return Color(92, 92, 92); }

// ============================================================================
// Data model (Material Symbols SVG)
// ============================================================================

enum IconStyle {
    ST_OUTLINED = 0,
    ST_ROUNDED  = 1,
    ST_SHARP    = 2
};

static const char* kStyleLabel[] = {
    "Outlined",
    "Rounded",
    "Sharp"
};

enum ExportKind {
    EX_CODE_IML = 0,       // "Code iml format"  (IML-style header, uses UiMakeIcon RAW)
    EX_CODE_RAW,           // "Code RAW (UiMakeIcon)"
    EX_CODE_RLE,           // "Code RLE (UiMakeIcon)"
    EX_SVG,
    EX_IML,
    EX_ICO,
    EX_PNG,
    EX_JPG
};


struct IconRecord : Moveable<IconRecord> {
    String      category;   // e.g. "toggle"
    IconStyle   style;      // OUTLINED / ROUNDED / SHARP
    String      name;       // icon name
    String      source;     // original path (for info)
    const char* b64z;       // pointer to static Base64(zlib(svg)) blob
};

static int ValueToIntDef(const Value& v, int defv)
{
    return IsNumber(v) ? (int)v : defv;
}

// Build stable ID core: e.g. ACTION_OUTLINED_ACCOUNT_BALANCE_48
static String MakeIconIdCore(const IconRecord& r, int px)
{
    String base;
    base << r.category << "_" << kStyleLabel[(int)r.style] << "_" << r.name
         << "_" << AsString(px);

    String out;
    for(int i = 0; i < base.GetCount(); ++i) {
        int c = (byte)base[i];
        if(IsAlNum(c))
            out.Cat(ToUpper((wchar)c));
        else
            out.Cat('_');
    }
    return out;
}

// ============================================================================
// DragBadgeButton
// ----------------------------------------------------------------------------
// A rounded pill/tile button with:
//
//   * Custom palette / hover / pressed states
//   * Optional preview icon image
//   * A small payload struct for drag-and-drop
//   * Modes:
//       - DRAGABLE (used for category and icon tiles)
//       - DROPPED  (used inside the Selection Bin, click to remove)
//       - NORMAL   (plain button)
//
// Used for:
//   * Category pills
//   * Icon tiles (with previews)
//   * Header buttons (Exit / Copy / Process)
// ============================================================================
class DragBadgeButton : public Button {
public:
    typedef DragBadgeButton CLASSNAME;

    enum Mode   { DRAGABLE, DROPPED, NORMAL };
    enum Layout { ICON_CENTER_TEXT_BOTTOM, ICON_CENTER_TEXT_TOP, TEXT_CENTER, ICON_ONLY_CENTER };
    enum { ST_NORMAL = 0, ST_HOT = 1, ST_PRESSED = 2, ST_DISABLED = 3, ST_COUNT = 4 };

    struct Palette {
        Color face[ST_COUNT], border[ST_COUNT], ink[ST_COUNT];
    };
    struct Payload {
        String flavor;    // "category" or "icon"
        String id;        // unique id (e.g. category key or packed icon id)
        String group;     // category group
        String name;      // short name
        String text;      // display text
        String badge;     // optional badge text
    };

    DragBadgeButton()
    {
        Transparent();
        SetFrame(NullFrame());
        SetMinSize(Size(DPI(60), DPI(24)));
        SetFont(StdFont().Bold().Height(DPI(11)));
        SetBadgeFont(StdFont().Height(DPI(17)));
        SetRadius(DPI(8))
            .SetStroke(1)
            .EnableDashed(false)
            .EnableFill(true);
        SetHoverTintPercent(40);
        SetSelected(false);
        SetSelectionColors(Color(37, 99, 235), SColorHighlightText(), White());
        SetLayout(TEXT_CENTER);
        mode = NORMAL;
        SetBaseColors(Blend(SColorPaper(), SColorFace(), 20), SColorShadow(), SColorText());
    }

    // payload & content
    DragBadgeButton& SetPayload(const Payload& p) { payload = p; Refresh(); return *this; }
    const Payload&   GetPayload() const           { return payload; }

    DragBadgeButton& SetLabel(const String& t)    { Button::SetLabel(t); Refresh(); return *this; }
    DragBadgeButton& SetBadgeText(const String& b){ badgeText = b; Refresh(); return *this; }

    // optional preview image & opaque pointer for app data
    DragBadgeButton& SetIconImage(const Image& img)
    {
        iconImage    = img;
        hasIconImage = !iconImage.IsEmpty();
        Refresh();
        return *this;
    }

    DragBadgeButton& SetUserPtr(const void* p) { userPtr = p; return *this; }
    const void*      GetUserPtr() const        { return userPtr; }

    // layout / fonts
    DragBadgeButton& SetLayout(Layout l)   { layout = l; Refresh(); return *this; }
    DragBadgeButton& SetFont(Font f)       { textFont = f; Refresh(); return *this; }
    DragBadgeButton& SetBadgeFont(Font f)  { badgeFont = f; Refresh(); return *this; }
    DragBadgeButton& SetHoverColor(Color c){ hoverColor = c; return *this; }
    DragBadgeButton& SetTileSize(Size s)   { SetMinSize(s); return *this; }
    DragBadgeButton& SetTileSize(int w, int h)
    { return SetTileSize(Size(w, h)); }

    // modes / selection
    DragBadgeButton& SetMode(Mode m)             { mode = m; Refresh(); return *this; }
    DragBadgeButton& SetSelected(bool on = true) { selected = on; Refresh(); return *this; }
    bool             IsSelected() const          { return selected; }

    DragBadgeButton& SetSelectionColors(Color faceC, Color borderC, Color inkC = SColorHighlightText())
    {
        selFace   = faceC;
        selBorder = borderC;
        selInk    = inkC;
        return *this;
    }

    // palette
    DragBadgeButton& SetPalette(const Palette& p) { pal = p; Refresh(); return *this; }
    Palette          GetPalette() const           { return pal; }

    DragBadgeButton& SetBaseColors(Color face, Color border, Color ink,
                                   int hot_pct = 12, int press_pct = 14)
    {
        pal.face[ST_NORMAL]   = face;
        pal.face[ST_HOT]      = Blend(face, White(), hot_pct);
        pal.face[ST_PRESSED]  = Blend(face, Black(), press_pct);
        pal.face[ST_DISABLED] = Blend(SColorFace(), SColorPaper(), 60);

        pal.border[ST_NORMAL]   = border;
        pal.border[ST_HOT]      = Blend(border, SColorHighlight(), 20);
        pal.border[ST_PRESSED]  = Blend(border, Black(), 15);
        pal.border[ST_DISABLED] = Blend(border, SColorDisabled(), 35);

        pal.ink[ST_NORMAL]   = ink;
        pal.ink[ST_HOT]      = ink;
        pal.ink[ST_PRESSED]  = ink;
        pal.ink[ST_DISABLED] = SColorDisabled();

        Refresh();
        return *this;
    }

    // geometry
    DragBadgeButton& SetRadius(int px)           { radius = max(0, px); Refresh(); return *this; }
    DragBadgeButton& SetStroke(int th)           { stroke = max(0, th); Refresh(); return *this; }
    DragBadgeButton& EnableDashed(bool on = true){ dashed = on; Refresh(); return *this; }
    DragBadgeButton& SetDashPattern(const String& d)
    { dash = d; Refresh(); return *this; }
    DragBadgeButton& EnableFill(bool on = true)  { fill = on; Refresh(); return *this; }
    DragBadgeButton& SetHoverTintPercent(int pct){ hoverPct = clamp(pct, 0, 100); return *this; }

    Callback WhenRemove; // used in Selection Bin: clicking tile removes it

    // DnD
    void LeftDrag(Point, dword) override
    {
        if(mode != DRAGABLE)
            return;
        if(IsEmpty(payload.flavor))
            return;

        Image sample = MakeDragSample();
        DoDragAndDrop(InternalClip(*this, payload.flavor), sample, DND_COPY);
    }

	void LeftDown(Point p, dword k) override
	{
	    if(mode == DROPPED && WhenRemove) {
	        WhenRemove();   // this will cause the tile to be rebuilt/removed
	        return;         // DO NOT call Button::LeftDown after this or crash ooooo
	    }
	    Button::LeftDown(p, k);
	}

    void Paint(Draw& w) override
    {
        const int st = StateIndex();
        Color faceC   = pal.face[st];
        Color borderC = pal.border[st];
        Color inkC    = pal.ink[st];

        if(HasMouse() && st == ST_HOT && !selected) {
            Color target = IsNull(hoverColor) ? White() : hoverColor;
            faceC = Blend(faceC, target, hoverPct);
        }
        if(selected) {
            if(!IsNull(selFace))   faceC   = selFace;
            if(!IsNull(selBorder)) borderC = selBorder;
            if(!IsNull(selInk))    inkC    = selInk;
        }

        Size sz = GetSize();
        ImageBuffer ib(sz);
        Fill(~ib, RGBAZero(), ib.GetLength());
        {
            BufferPainter p(ib, MODE_ANTIALIASED);
            const double inset = 0.5;
            const double x = inset;
            const double y = inset;
            const double wdt = sz.cx - 2 * inset;
            const double hgt = sz.cy - 2 * inset;
            const int    r   = min(radius, min(sz.cx, sz.cy) / 2);

            p.Begin();
            p.RoundedRectangle(x, y, wdt, hgt, r);
            if(fill)
                p.Fill(faceC);
            if(stroke > 0) {
                if(dashed)
                    p.Dash(dash, 0.0);
                p.Stroke(stroke, borderC);
            }
            p.End();
        }
        w.DrawImage(0, 0, ib);

        Rect r = Rect(sz).Deflated(DPI(6), DPI(4));
        const String label_txt = AsString(GetLabel());

        if(layout == ICON_CENTER_TEXT_BOTTOM) {
            if(hasIconImage && !IsNull(iconImage)) {
                int  iconMaxH = max(16, r.GetHeight() - DPI(18));
                int  iconMaxW = r.GetWidth() - DPI(6);
                Size isz      = iconImage.GetSize();
                if(isz.cx <= 0 || isz.cy <= 0) {
                    Size ts = GetTextSize(label_txt, textFont);
                    w.DrawText(r.left + (r.GetWidth() - ts.cx) / 2,
                               r.top  + (r.GetHeight() - ts.cy) / 2,
                               label_txt, textFont, inkC);
                } else {
                    double scale = min(iconMaxW / (double)isz.cx,
                                       (iconMaxH - DPI(10)) / (double)isz.cy);
                    int wicon = (int)(isz.cx * scale);
                    int hicon = (int)(isz.cy * scale);
                    int ix    = r.left + (r.GetWidth() - wicon) / 2;
                    int iy    = r.top  + DPI(2);
                    w.DrawImage(ix, iy, wicon, hicon, iconImage);

                    Size ts = GetTextSize(label_txt, textFont);
                    int  ty = r.bottom - ts.cy;
                    w.DrawText(r.left + (r.GetWidth() - ts.cx) / 2,
                               ty,
                               label_txt,
                               textFont,
                               inkC);
                }
            } else {
                Size ts = GetTextSize(label_txt, textFont);
                w.DrawText(r.left + (r.GetWidth() - ts.cx) / 2,
                           r.top  + (r.GetHeight() - ts.cy) / 2,
                           label_txt, textFont, inkC);
            }
        }
        else if(layout == TEXT_CENTER) {
            Size ts = GetTextSize(label_txt, textFont);
            w.DrawText(r.left + (r.GetWidth() - ts.cx) / 2,
                       r.top  + (r.GetHeight() - ts.cy) / 2,
                       label_txt, textFont, inkC);
        }
        else { // ICON_ONLY_CENTER or ICON_CENTER_TEXT_TOP
            Size gsz = GetTextSize(badgeText, badgeFont);
            w.DrawText(r.left + (r.GetWidth() - gsz.cx) / 2,
                       r.top  + (r.GetHeight() - gsz.cy) / 2,
                       badgeText, badgeFont, inkC);
        }
    }

private:
    int StateIndex() const
    {
        if(!IsShowEnabled())         return ST_DISABLED;
        if(IsPush() || HasCapture()) return ST_PRESSED;
        return HasMouse() ? ST_HOT : ST_NORMAL;
    }

    Image MakeDragSample() const
    {
        Size sz = GetSize();
        if(sz.cx <= 0 || sz.cy <= 0)
            return Image();
        ImageBuffer ib(sz);
        Fill(~ib, RGBAZero(), ib.GetLength());
        {
            BufferPainter p(ib);
            const_cast<DragBadgeButton*>(this)->Paint(p);
        }
        return Image(ib);
    }

    Payload    payload;
    String     badgeText;
    Layout     layout   = TEXT_CENTER;
    Palette    pal;
    Font       textFont, badgeFont;
    int        radius   = DPI(8);
    int        stroke   = 1;
    bool       dashed   = false;
    bool       fill     = true;
    String     dash     = "5,5";
    int        hoverPct = 22;
    bool       selected = false;
    Color      selFace  = Null;
    Color      selBorder= Null;
    Color      selInk   = Null;
    Mode       mode     = NORMAL;
    Color      hoverColor = Null;

    Image      iconImage;
    bool       hasIconImage = false;
    const void* userPtr      = nullptr;
};

// ============================================================================
// DropListStyled
// ----------------------------------------------------------------------------
// A styled DropList with rounded background, custom colors and an integrated
// arrow icon. Used for:
//   * Style selection (Outlined / Rounded / Sharp)
//   * Export type selection (Code / SVG / PNG / ...)
//   * Icon size selection
// ============================================================================
struct DropListStyled : DropList {
    int   radius        = DPI(8);
    Color face_normal   = SColorFace();
    Color face_hot      = Blend(Color(36, 99, 235), White(), 15);
    Color face_pressed  = Blend(Color(37, 99, 235), Black(), 100);
    Color face_disabled = Blend(SColorFace(), SColorPaper(), 60);
    Color ink_normal    = SColorShadow();
    Color ink_disabled  = GrayColor(180);
    Color border_color  = SColorShadow();
    int   stroke        = 0;
    bool  dashed        = false;
    String dash         = "5,5";
    bool   fill         = true;
    VectorMap<Value, String> key_to_label;

    DropListStyled()
    {
        Transparent();
        SetFrame(NullFrame());
        SetDropLines(16);
    }

    DropListStyled& SetBgColor(Color base, int hot_pct = 15, int press_pct = 20)
    {
        face_normal   = base;
        face_hot      = Blend(base, White(), hot_pct);
        face_pressed  = Blend(base, Black(), press_pct);
        face_disabled = Blend(SColorFace(), SColorPaper(), 60);
        Refresh();
        return *this;
    }

    DropListStyled& SetLabelColor(Color ink, Color dis = GrayColor(180))
    {
        ink_normal   = ink;
        ink_disabled = dis;
        Refresh();
        return *this;
    }

    DropListStyled& SetBorderColor(Color c) { border_color = c; Refresh(); return *this; }
    DropListStyled& SetRadius(int px)       { radius = max(0, px); Refresh(); return *this; }
    DropListStyled& SetStroke(int th)       { stroke = max(0, th); Refresh(); return *this; }

    DropListStyled& EnableDashed(bool on = true)
    { dashed = on; Refresh(); return *this; }

    DropListStyled& SetDashPattern(const String& d)
    { dash = d; Refresh(); return *this; }

    DropListStyled& EnableFill(bool on = true)
    { fill = on; Refresh(); return *this; }

    DropListStyled& SetTileSize(Size s)
    { SetMinSize(s); return *this; }

    DropListStyled& SetTileSize(int w, int h)
    { return SetTileSize(Size(w, h)); }

    DropListStyled& Add(const Value& key, const Value& label, bool enable = true)
    {
        key_to_label.GetAdd(key) = AsString(label);
        DropList::Add(key, label, enable);
        return *this;
    }

    void Paint(Draw& w) override
    {
        Size sz = GetSize();
        const int st = VisualState();

        Color bg  = (st == CTRL_DISABLED) ? face_disabled
                   : (st == CTRL_PRESSED) ? face_pressed
                   : (st == CTRL_HOT)     ? face_hot
                                          : face_normal;
        Color ink = (st == CTRL_DISABLED) ? ink_disabled : ink_normal;

        ImageBuffer ib(sz);
        ib.SetKind(IMAGE_ALPHA);
        Fill(~ib, RGBAZero(), ib.GetLength());
        {
            BufferPainter p(ib, MODE_ANTIALIASED);
            const double inset = 0.5;
            const double x = inset;
            const double y = inset;
            const double wdt = sz.cx - 2 * inset;
            const double hgt = sz.cy - 2 * inset;
            const int    r   = min(radius, min(sz.cx, sz.cy) / 2);
            p.Begin();
            p.RoundedRectangle(x, y, wdt, hgt, r);
            if(fill)
                p.Fill(bg);
            if(stroke > 0) {
                if(dashed)
                    p.Dash(dash, 0.0);
                p.Stroke(stroke, border_color);
            }
            p.End();
        }
        w.DrawImage(0, 0, ib);

        Value  key  = Get();
        String text = key_to_label.Get(key, AsString(key));
        Font   f    = StdFont().Height(DPI(11));
        Size   ts   = GetTextSize(text, f);

        const int padL = DPI(8);
        const int padR = DPI(8);
        Image arrow = CtrlImg::SortDown();
        const int aw = DPI(12);
        const int ah = DPI(12);
        int ax = sz.cx - padR - aw;
        int ay = (sz.cy - ah) / 2;

        Rect tr = RectC(padL, 0, ax - padL - DPI(4), sz.cy);
        w.Clip(tr);
        w.DrawText(padL, (sz.cy - ts.cy) / 2, text, f, ink);
        w.End();
        w.DrawImage(ax, ay, aw, ah, arrow);
    }

private:
    int VisualState() const
    {
        if(!IsShowEnabled()) return CTRL_DISABLED;
        if(IsPopUp())        return CTRL_PRESSED;
        return HasMouse() ? CTRL_HOT : CTRL_NORMAL;
    }
};

// ============================================================================
// Helpers binding IconRecord -> DragBadgeButton tiles
// ============================================================================

static inline DragBadgeButton& SetupPillButton(DragBadgeButton& b,
                                               const String& category)
{
    DragBadgeButton::Payload p;
    p.flavor = "category"; // DnD flavor
    p.id     = category;   // id == category key
    p.group  = category;   // group used in bin to know which category
    p.name   = category;
    p.text   = category;

    return b.SetLabel(category)
            .SetBadgeText(String())
            .SetLayout(DragBadgeButton::TEXT_CENTER)
            .SetPayload(p);
}

static inline DragBadgeButton& SetupIconTile(DragBadgeButton& b,
                                             const IconRecord& rec,
                                             Size tile,
                                             const Image& preview)
{
    DragBadgeButton::Payload p;
    p.flavor = "icon";
    p.id     = Format("%s|%s|%d", ~rec.category, ~rec.name, (int)rec.style);
    p.group  = rec.category;
    p.name   = rec.name;
    p.text   = rec.name;

    Font labelFont = StdFont().Height(DPI(9));

    return b.SetLabel(rec.name)
            .SetBadgeText(String())
            .SetLayout(DragBadgeButton::ICON_CENTER_TEXT_BOTTOM)
            .SetPayload(p)
            .SetTileSize(tile)
            .SetFont(labelFont)
            .SetIconImage(preview)
            .SetUserPtr(&rec);
}

// ============================================================================
// SVG decode + render helpers (Base64(zlib(svg)) -> Image)
// ============================================================================

static Image TintIcon(const Image& src, Color col)
{
    if(src.IsEmpty())
        return src;

    // If you ever want "no tint", pass Null color.
    if(IsNull(col))
        return src;

    Size        sz = src.GetSize();
    ImageBuffer ib(sz);
    ib.SetKind(src.GetKind());

    for(int y = 0; y < sz.cy; ++y) {
        const RGBA* srow = src[y];
        RGBA*       drow = ib[y];

        for(int x = 0; x < sz.cx; ++x) {
            const RGBA& s = srow[x];
            RGBA&       d = drow[x];

            // Build coverage from source darkness and source alpha.
            // This works both for:
            //  - black-on-transparent SVG rasterization, and
            //  - black-on-white rasterization (white background becomes transparent).
            int lum = (int)((54 * s.r + 183 * s.g + 19 * s.b + 128) >> 8); // ~0.2126/0.7152/0.0722
            int darkness = 255 - lum;
            int a = (s.a * darkness + 127) / 255;

            // U++ draw pipeline expects premultiplied RGBA in Image buffers.
            d.r = (byte)((col.GetR() * a + 127) / 255);
            d.g = (byte)((col.GetG() * a + 127) / 255);
            d.b = (byte)((col.GetB() * a + 127) / 255);
            d.a = a;
        }
    }

    return Image(ib);
}

static String DecodeSvg(const IconRecord& r)
{
    if(!r.b64z)
        return String();
    String packed_b64 = String(r.b64z);
    if(packed_b64.IsEmpty())
        return String();
    String packed = Base64Decode(packed_b64);
    if(packed.IsEmpty())
        return String();
    return ZDecompress(packed);
}

static Image RenderSvgIconToImage(const IconRecord& r, int px)
{
    static VectorMap<String, Image> cache;

    String key = Format("%s|%d|%s|%d", ~r.category, (int)r.style, ~r.name, px);
    int idx    = cache.Find(key);
    if(idx >= 0)
        return cache[idx];

    String svg = DecodeSvg(r);
    if(svg.IsEmpty())
        return Image();

    Size  sz  = Size(px, px);
    Image img = RenderSVGImage(sz, svg);
    cache.Add(key, img);
    return img;
}

// ============================================================================
// Lean ICO writer (multi-size 16,24,32,48,64 BGRA)
// ============================================================================

static inline void Put16le(Stream& s, int v)
{
    s.Put((byte)(v));
    s.Put((byte)(v >> 8));
}

static inline void Put32le(Stream& s, int v)
{
    s.Put((byte)(v));
    s.Put((byte)(v >> 8));
    s.Put((byte)(v >> 16));
    s.Put((byte)(v >> 24));
}

static inline int IcoPayloadSize(int w, int h)
{
    const int header      = 40;                  // BITMAPINFOHEADER
    const int xor_bytes   = w * h * 4;
    const int mask_stride = ((w + 31) / 32) * 4; // 1 bpp, DWORD aligned
    const int and_bytes   = mask_stride * h;
    return header + xor_bytes + and_bytes;
}

// Save a lean .ico with sizes {16,24,32,48,64} as 32-bit BGRA DIB entries
static bool SaveIcon64(const String& path, const Image& base)
{
    static const int SIZES[] = {16, 24, 32, 48, 64};
    const int        N      = (int)(sizeof(SIZES) / sizeof(SIZES[0]));

    Vector<Image> imgs;
    imgs.SetCount(N);
    for(int i = 0; i < N; ++i)
        imgs[i] = Rescale(base, SIZES[i], SIZES[i]);

    FileOut out(path);
    if(!out.IsOpen())
        return false;

    const int dir_bytes = 6 + 16 * N; // ICONDIR + N ICONDIRENTRY
    int       offset    = dir_bytes;
    Vector<int> payload(N);
    Vector<int> offs(N);
    for(int i = 0; i < N; ++i) {
        const int w = SIZES[i];
        const int h = SIZES[i];
        payload[i] = IcoPayloadSize(w, h);
        offs[i]    = offset;
        offset    += payload[i];
    }

    // ICONDIR
    Put16le(out, 0);           // reserved
    Put16le(out, 1);           // type = 1 (icon)
    Put16le(out, N);           // count

    // ICONDIRENTRY
    for(int i = 0; i < N; ++i) {
        const int w = SIZES[i];
        const int h = SIZES[i];
        out.Put((byte)w);
        out.Put((byte)h);
        out.Put((byte)0);      // color count (0 = 256+)
        out.Put((byte)0);      // reserved
        Put16le(out, 1);       // planes
        Put16le(out, 32);      // bit count
        Put32le(out, payload[i]);
        Put32le(out, offs[i]);
    }

    // Image payloads
    for(int i = 0; i < N; ++i) {
        const Image& img = imgs[i];
        const int    w   = img.GetWidth();
        const int    h   = img.GetHeight();

        const int xor_bytes   = w * h * 4;
        const int mask_stride = ((w + 31) / 32) * 4;
        const int and_bytes   = mask_stride * h;

        // BITMAPINFOHEADER
        Put32le(out, 40);                 // header size
        Put32le(out, w);
        Put32le(out, h * 2);              // XOR + AND
        Put16le(out, 1);                  // planes
        Put16le(out, 32);                 // bits
        Put32le(out, 0);                  // compression
        Put32le(out, xor_bytes + and_bytes);
        Put32le(out, 0);                  // x ppm
        Put32le(out, 0);                  // y ppm
        Put32le(out, 0);                  // clr used
        Put32le(out, 0);                  // clr important

        // XOR bitmap (BGRA, bottom-up)
        for(int y = h - 1; y >= 0; --y) {
            const RGBA* row = img[y];
            for(int x = 0; x < w; ++x) {
                const RGBA& p = row[x];
                out.Put(p.b);
                out.Put(p.g);
                out.Put(p.r);
                out.Put(p.a);
            }
        }

        // AND mask: all zeros (completely opaque)
        Buffer<byte> zeroline(mask_stride, 0);
        for(int y = 0; y < h; ++y)
            out.Put(zeroline, mask_stride);
    }

    return out.IsOK();
}

// ============================================================================
// DropBinCard
// ----------------------------------------------------------------------------
// A StageCard that acts as the "Selection Bin":
//   * Accepts drag & drop of:
//       - individual icons ("icon" flavor)
//       - entire categories ("category" flavor)
//   * Tiles show a tinted preview and clicking them removes that entry
//   * Exposes a Vector<IconRecord> of selected icons
// ============================================================================
class DropBinCard : public StageCard {
public:
    typedef DropBinCard CLASSNAME;

    Callback1<const IconRecord&> WhenAdded;
    Callback1<const IconRecord&> WhenRemoved;
    Callback                     WhenListChanged;
    Callback1<const String&>     WhenCategoryDropped;

    Size tileSizes = Size(70, 70);

    DropBinCard()
    {
        SetTitle("Export Bin (Drag Here)")
            .SetHeaderAlign(StageCard::LEFT)
            .EnableHeaderFill(false)
            .EnableCardFill(false)
            .EnableCardFrame(false)
            .EnableContentFill(true)
            .EnableContentFrame(true)
            .EnableContentDash(true)
            .SetContentCornerRadius(DPI(10))
            .SetContentFrameThickness(2);

        SetStackH();
        SetWrap();
        EnableContentScroll(true);
        EnableContentClampToPane(true);
        SetContentInset(DPI(8), DPI(8), DPI(8), DPI(8));
        SetContentGap(DPI(4), DPI(4));
    }

    DropBinCard& SetTileSize(Size s)
    {
        tileSizes = s;
        Reflow();
        return *this;
    }

    DropBinCard& SetTileSize(int w, int h)
    {
        tileSizes = Size(w, h);
        Reflow();
        return *this;
    }

    DropBinCard& SetIconColor(Color c)
    {
        iconColor = c;
        RebuildTiles();
        return *this;
    }

    const Vector<IconRecord>& GetIcons() const { return items; }

    void AddIconUnique(const IconRecord& s)
    {
        for(const auto& x : items)
            if(x.category == s.category && x.name == s.name && x.style == s.style)
                return;
        items.Add(s);
        if(WhenAdded)
            WhenAdded(s);
        RebuildTiles();
        FireListChanged();
    }

    void RemoveIndex(int i)
    {
        if(i < 0 || i >= items.GetCount())
            return;
        IconRecord removed = items[i];
        items.Remove(i);
        if(WhenRemoved)
            WhenRemoved(removed);
        RebuildTiles();
        FireListChanged();
    }

    void ClearAll()
    {
        items.Clear();
        RebuildTiles();
        FireListChanged();
    }

    void DragAndDrop(Point, PasteClip& d) override
    {
        const bool ok_icon = AcceptInternal<DragBadgeButton>(d, "icon");
        const bool ok_cat  = AcceptInternal<DragBadgeButton>(d, "category");
        if(!(ok_icon || ok_cat))
            return;

        d.SetAction(DND_COPY);
        d.Accept();
        if(!d.IsPaste())
            return;

        const DragBadgeButton& src = GetInternal<DragBadgeButton>(d);

        if(ok_icon) {
            const IconRecord* srcRec = (const IconRecord*)src.GetUserPtr();
            if(!srcRec)
                return;
            AddIconUnique(*srcRec);
        }
        else {
            // category dropped
            const DragBadgeButton::Payload& P = src.GetPayload();
            if(WhenCategoryDropped)
                WhenCategoryDropped(P.group);   // P.group is the category key/string
        }
    }

    void Paint(Draw& w) override
    {
        StageCard::Paint(w);
        if(items.IsEmpty()) {
            Size  sz = GetSize();
            String t = "Drag icons here. Click a tile to remove.";
            Font  f  = StdFont().Italic().Height(DPI(10));
            Size  ts = GetTextSize(t, f);
            w.DrawText((sz.cx - ts.cx) / 2,
                       (sz.cy - ts.cy) / 2,
                       t, f, SColorDisabled());
        }
    }

private:
    Vector<IconRecord>     items;
    Array<DragBadgeButton> tiles;
    Color                  iconColor = Black();

    void FireListChanged()
    {
        if(WhenListChanged)
            WhenListChanged();
    }

    void RebuildTiles()
    {
        for(int i = 0; i < tiles.GetCount(); ++i)
            tiles[i].Remove();
        tiles.Clear();

        for(int i = 0; i < items.GetCount(); ++i) {
            DragBadgeButton& t = tiles.Create();
            int px = min(tileSizes.cx, tileSizes.cy) - DPI(10);
            px = max(px, 16);

            Image base    = RenderSvgIconToImage(items[i], px);
            Image preview = TintIcon(base, iconColor);

            SetupIconTile(t, items[i], tileSizes, preview)
                .SetMode(DragBadgeButton::DROPPED)
                .SetRadius(DPI(5))
                .SetStroke(1)
                .EnableDashed(false)
                .EnableFill(true);

            t.WhenRemove = THISBACK1(RemoveIndex, i);
            AddFixed(t, tileSizes.cx, tileSizes.cy);
        }
        Layout();
        Refresh();
    }

    void Reflow()
    {
        for(int i = 0; i < tiles.GetCount(); ++i)
            tiles[i].SetMinSize(tileSizes);
        Layout();
        Refresh();
    }
};

// ============================================================================
// IconPickerApp
// ----------------------------------------------------------------------------
// Main window:
//   * Header toolbar (style, export kind, size, Copy/Process)
//   * Category strip
//   * Icons view (filtered tiles)
//   * Selection Bin
//   * Output panel (logs / code / manifests)
// ============================================================================
class IconPickerApp : public TopWindow {
public:
    typedef IconPickerApp CLASSNAME;

    // Cards
    StageCard   headerCard;
    StageCard   categoryCard;
    StageCard   itemsCard;
    DropBinCard binCard;
    StageCard   codeCard;

    // Controls (items card header)
    ColorPusher tintColorPusher;      // icon tint
    EditString  iconSearch;           // search/filter
    Label       lblColor;
    Label       lblSearch;
    Label       lblCopyMode;          // shows [Copy Iml] / [Copy Bin]

    // Header controls (toolbar)
    DragBadgeButton btnExit;
    DragBadgeButton btnCopy;
    DragBadgeButton btnProcess;

    // Drop-downs
    DropListStyled  styleDrop;        // filter: Outlined / Rounded / Sharp
    DropListStyled  exportTypeDrop;   // Code / SVG / IML / ICO / PNG / JPG
    DropListStyled  iconSizeDrop;     // icon size selector

    // Layout
    Splitter        mainSplit;

    // Content
    Array<DragBadgeButton> categoryButtons;
    Array<DragBadgeButton> iconTiles;
    DocEdit         codeOutput;

    // State
    Size            tileSizes         = Size(90, 60);
    IconStyle       currentStyle      = ST_OUTLINED;
    String          currentCategory;       // active category (no "All")
    int             page_index        = 0;
    int             page_size         = 240;
    ExportKind      currentExportKind = EX_CODE_IML;

    Color           currentTintColor  = Black(); // default tint (black)
    String          searchFilter;

    String          lastExportDir;     // remember dir across exports
    String          lastManifestPath;  // for IML manifest helper
    String          lastHeaderPath;    // for inline header export

    // Data
    Vector<IconRecord> allIcons;       // all icons across categories/styles

    // Right-aligned layout helper for header
    int colStart = DPI(20);
    int colPad   = DPI(8);
    int colX     = 0;

    int ColPos(int width, bool reset = false)
    {
        if(reset)
            colX = colStart;
        int cur = colX;
        colX += DPI(width) + colPad;
        return cur;
    }

    IconPickerApp()
    {
        Title(String(APP_TITLE) + " " + APP_VER);
        Sizeable().Zoomable();
        SetMinSize(Size(DPI(800), DPI(640)));
        SetRect(Size(DPI(1280), DPI(840)));

        BuildHeader();
        BuildCategoryStrip();
        BuildItemsCard();
        BuildBinCard();
        BuildCodeCard();

        // Compose splitters
        mainSplit.Horz();
        mainSplit << itemsCard << binCard;
        mainSplit.SetPos(8000);   // ~80% itemsCard / ~20% binCard

        Add(headerCard.HSizePos(DPI(10), DPI(10)).TopPos(DPI(10), DPI(85)));
        Add(categoryCard.HSizePos(DPI(8), DPI(8)).TopPos(DPI(115), DPI(110)));
        Add(mainSplit.HSizePos(DPI(8), DPI(8)).VSizePos(DPI(240), DPI(145)));
        Add(codeCard.HSizePos(DPI(8), DPI(8)).BottomPos(DPI(8), DPI(120)));

        // Data & initial UI
        LoadAllIcons();
        RebuildCategoryButtonsFromData();
        RebuildIconGrid();
        UpdateOutputPaneInfo();
        UpdateCopyModeLabel();
    }

    void Paint(Draw& w) override
    {
        w.DrawRect(GetSize(), MainWindowColor());
    }

private:
    // ------------------------------ UI build helpers ------------------------

	void UpdateCopyModeLabel()
	{
	    String t;
	    switch(currentExportKind) {
	    case EX_CODE_IML:
	        t = "[Copy Iml]";
	        break;
	    case EX_CODE_RAW:
	        t = "[Copy RAW]";
	        break;
	    case EX_CODE_RLE:
	        t = "[Copy RLE]";
	        break;
	    default:
	        // For all non-code export kinds, copy the IML-style header by default.
	        t = "[Copy Iml]";
	        break;
	    }
	    lblCopyMode.SetLabel(t);
	    lblCopyMode.Refresh();
	}


    void BuildHeader()
    {
        headerCard
            .SetTitle("Material Symbols Picker (U++)")
            .SetTitleColor(TitleColor())
            .SetTitleFont(StdFont().Bold().Height(DPI(25)))
            .SetSubTitle("Choose style + type + size, drag icons to bin, then Process Selected.")
            .SetSubTitleFont(StdFont().Height(DPI(11)))
            .SetSubTitleColor(MainGrayColor())
            .SetCardColors(White(), MainFrameColor())
            .SetHeaderGap(DPI(6))
            .SetHeaderInset(DPI(10), DPI(10), 0, 0)
            .SetCardCornerRadius(DPI(8))
            .EnableCardFrame(true)
            .EnableCardFill(true)
            .EnableHeaderFill(false)
            .EnableContentFill(false)
            .SetTitleUnderlineVertical(true);

        // Toolbar buttons
        btnExit.SetLabel("Exit")
               .SetTileSize(90, 40)
               .SetBaseColors(ExitButtonColor(), MainFrameColor(), White());
        btnExit.WhenAction = [=] { Close(); };

        btnCopy.SetLabel("Copy")
               .SetTileSize(90, 40)
               .SetBaseColors(CopyButtonColor(), MainFrameColor(), MainGrayColor());
        btnCopy.WhenAction = THISBACK(CopyInlineHeaderSnippet);

        btnProcess.SetLabel("Process")
                  .SetTileSize(90, 40)
                  .SetBaseColors(ProcessButtonColor(), MainFrameColor(), White());
        btnProcess.WhenAction = THISBACK(ProcessSelected);

        lblCopyMode.SetFont(StdFont().Height(DPI(10)));
        lblCopyMode.SetLabel(""); // initial value set after ctor via UpdateCopyModeLabel()

        // Style dropdown (Outlined / Rounded / Sharp)
        styleDrop.Add((int)ST_OUTLINED, "Outlined");
        styleDrop.Add((int)ST_ROUNDED,  "Rounded");
        styleDrop.Add((int)ST_SHARP,    "Sharp");
        styleDrop.SetRadius(DPI(8)).SetStroke(1);
        styleDrop <<= (int)currentStyle;
        styleDrop.WhenAction = [=] {
            currentStyle = (IconStyle)(int)~styleDrop;
            page_index   = 0;
            RebuildIconGrid();
        };

		// Export type dropdown
		exportTypeDrop.Add((int)EX_CODE_IML, "Code iml format");
		exportTypeDrop.Add((int)EX_CODE_RAW, "Code RAW (UiMakeIcon)");
		exportTypeDrop.Add((int)EX_CODE_RLE, "Code RLE (UiMakeIcon)");
		exportTypeDrop.Add((int)EX_SVG,      "SVG");
		exportTypeDrop.Add((int)EX_IML,      "IML");
		exportTypeDrop.Add((int)EX_ICO,      "ICO");
		exportTypeDrop.Add((int)EX_PNG,      "PNG");
		exportTypeDrop.Add((int)EX_JPG,      "JPG");

        exportTypeDrop.SetRadius(DPI(8)).SetStroke(1);
        exportTypeDrop <<= (int)currentExportKind;
        exportTypeDrop.WhenAction = [=] {
            currentExportKind = (ExportKind)(int)~exportTypeDrop;
            UpdateOutputPaneInfo();
            UpdateCopyModeLabel();
        };

        // Size dropdown
        iconSizeDrop.Add(16,  "16");
        iconSizeDrop.Add(20,  "20");
        iconSizeDrop.Add(24,  "24");
        iconSizeDrop.Add(32,  "32");
        iconSizeDrop.Add(48,  "48 [Default]");
        iconSizeDrop.Add(64,  "64");
        iconSizeDrop.Add(96,  "96");
        iconSizeDrop.Add(128, "128");
        iconSizeDrop.Add(192, "192");
        iconSizeDrop.Add(256, "256");
        iconSizeDrop.SetRadius(DPI(8)).SetStroke(1);
        iconSizeDrop <<= 48; // default preview / export size
        iconSizeDrop.WhenAction = [=] { RebuildIconGrid(); };

        // Order (right to left): Exit, Copy, CopyMode, Process, Size, Type, Style
        headerCard.AddHeader(btnExit.RightPos(ColPos(80, true), DPI(80)).TopPos(DPI(27), DPI(30)));
        headerCard.AddHeader(btnCopy.RightPos(ColPos(80),        DPI(80)).TopPos(DPI(27), DPI(30)));
        headerCard.AddHeader(lblCopyMode.RightPos(ColPos(100),   DPI(55)).TopPos(DPI(27), DPI(30)));
        headerCard.AddHeader(btnProcess.RightPos(ColPos(150),    DPI(150)).TopPos(DPI(27), DPI(30)));
        headerCard.AddHeader(iconSizeDrop.RightPos(ColPos(150),  DPI(150)).TopPos(DPI(27), DPI(30)));
        headerCard.AddHeader(exportTypeDrop.RightPos(ColPos(150),DPI(150)).TopPos(DPI(27), DPI(30)));
        headerCard.AddHeader(styleDrop.RightPos(ColPos(80),      DPI(80)).TopPos(DPI(27), DPI(30)));
    }

    void BuildCategoryStrip()
    {
        categoryCard
            .SetTitle("Categories")
            .SetTitleFont(StdFont().Height(DPI(19)))
            .EnableHeaderFill(false)
            .EnableHeaderFrame(false)
            .EnableContentFill(false)
            .EnableContentFrame(false)
            .EnableCardFill(false)
            .EnableCardFrame(true)
            .SetHeaderInset(12, DPI(8), 0, 0)
            .SetTitleColor(MainGrayColor())
            .SetCardColors()
            .SetTitleUnderlineThickness(0)
            .SetCardCornerRadius(DPI(8))
            .SetCardFrameThickness(1)
            .EnableContentClampToPane(true)
            .EnableContentScroll(true)
            .SetStackH()
            .SetWrap()
            .SetContentInset(DPI(10), DPI(0), DPI(10), DPI(4))
            .SetContentGap(DPI(6), DPI(6));
    }

    void BuildItemsCard()
    {
        itemsCard
            .SetTitle("Icons")
            .SetTitleFont(StdFont().Height(DPI(19)))
            .EnableHeaderFill(false)
            .EnableHeaderFrame(false)
            .EnableContentFill(true)
            .EnableContentFrame(true)
            .EnableCardFill(true)
            .EnableCardFrame(true)
            .SetHeaderInset(12, DPI(8), 0, 0)
            .SetTitleColor(MainGrayColor())
            .SetCardColors(White(), MainFrameColor())
            .SetContentColor(SColorFace)
            .SetTitleUnderlineThickness(0)
            .SetCardCornerRadius(DPI(8))
            .SetCardFrameThickness(1)
            .SetContentCornerRadius(DPI(8))
            .SetContentInset(DPI(10), DPI(6), DPI(10), DPI(10))      // outer visual inset
            .SetContentInnerInset(DPI(6), DPI(6), DPI(6), DPI(6))    // inner layout inset
            .SetContentGap(DPI(6), DPI(6))
            .EnableContentScroll(true)
            .EnableContentClampToPane(true)
            .SetStackH()
            .SetWrap();

        // Color pusher (tint)
        tintColorPusher <<= currentTintColor; // default black
        tintColorPusher.Track();              // live updates
        tintColorPusher.WhenAction = [=] {
            currentTintColor = (Color)~tintColorPusher;
            RebuildIconGrid();                      // recolor icon tiles
            binCard.SetIconColor(currentTintColor); // recolor bin tiles
        };

        // Search box (filter by icon name, case-insensitive)
        iconSearch.WhenAction = [=] {
            searchFilter = ~iconSearch;
            RebuildIconGrid();
        };
        iconSearch.WhenEnter = iconSearch.WhenAction;

        // Live update while typing
        iconSearch.WhenAction = [=] {
            searchFilter = ~iconSearch;
            RebuildIconGrid();
        };

        itemsCard.AddHeader(tintColorPusher
                            .RightPos(ColPos(50, true), DPI(50))
                            .TopPos(DPI(8), DPI(22)));
        itemsCard.AddHeader(lblColor
                            .SetLabel("Tint Color")
                            .RightPos(ColPos(90), DPI(60))
                            .TopPos(DPI(8), DPI(22)));
        itemsCard.AddHeader(iconSearch
                            .RightPos(ColPos(140), DPI(160))
                            .TopPos(DPI(8), DPI(22)));
        itemsCard.AddHeader(lblSearch
                            .SetLabel("Filter")
                            .RightPos(ColPos(90), DPI(60))
                            .TopPos(DPI(8), DPI(22)));
    }

    void BuildBinCard()
    {
        binCard
            .SetTitle("Selection Bin (Drop Here)")
            .SetTitleFont(StdFont().Height(DPI(19)))
            .EnableHeaderFill(false)
            .EnableHeaderFrame(false)
            .EnableContentFill(true)
            .EnableContentFrame(true)
            .EnableCardFill(true)
            .EnableCardFrame(true)
            .SetHeaderInset(12, DPI(8), 0, 0)
            .SetTitleColor(MainGrayColor())
            .SetCardColors(White(), MainFrameColor())
            .SetContentColor(SColorFace)
            .SetTitleUnderlineThickness(0)
            .SetCardCornerRadius(DPI(8))
            .SetCardFrameThickness(1)
            .SetContentInset(DPI(10), DPI(6), DPI(10), DPI(10))
            .SetContentGap(DPI(6), DPI(6));

        binCard.SetTileSize(tileSizes);
        binCard.WhenListChanged     = THISBACK(UpdateOutputPaneInfo);
        binCard.WhenCategoryDropped = THISBACK(OnCategoryDroppedToBin);
    }

    void BuildCodeCard()
    {
        codeCard
            .SetTitle("Output Detail")
            .SetTitleFont(StdFont().Height(DPI(19)))
            .EnableHeaderFill(false)
            .EnableHeaderFrame(false)
            .EnableContentFill(true)
            .EnableContentFrame(false)
            .SetContentCornerRadius(DPI(8))
            .EnableCardFill(false)
            .EnableCardFrame(true)
            .SetHeaderInset(12, DPI(8), 0, 0)
            .SetTitleColor(MainGrayColor())
            .SetCardColors()
            .SetTitleUnderlineThickness(0)
            .SetCardCornerRadius(DPI(8))
            .SetCardFrameThickness(1)
            .SetContentInset(DPI(12), DPI(6), DPI(6), DPI(6))
            .SetContentGap(DPI(6), DPI(6))
            .SetContentColor(OutputContentColor(), OutputTextColor())
            .EnableContentScroll(false);

        // Code / log view
        codeOutput.SetReadOnly();
        codeOutput.Transparent();
        codeOutput.SetFrame(NullFrame());
        codeOutput.SetFont(Monospace(10));
        codeOutput.SetColor(DocEdit::INK_NORMAL,   OutputTextColor());
        codeOutput.SetColor(DocEdit::PAPER_NORMAL, Null);
        codeOutput.SetColor(DocEdit::PAPER_READONLY, Null);
        codeOutput.SetColor(DocEdit::PAPER_SELECTED,
                            Blend(OutputContentColor(), SColorHighlight(), 50));

        codeCard.ReplaceExpand(codeOutput);
        codeOutput.HSizePos(0, 0).VSizePos(0, 0);
    }

    // ------------------------------ Data load -------------------------------

    template <class IconT>
    void AppendCategoryIconsImpl(const IconT* table, int count)
    {
        for(int i = 0; i < count; ++i) {
            const IconT& src = table[i];
            IconRecord   r;
            r.category = src.category;
            r.style    = (IconStyle)(int)src.style;
            r.name     = src.name;
            r.source   = src.source;
            r.b64z     = src.b64zIcon;
            allIcons.Add(pick(r));
        }
    }

    void LoadAllIcons()
    {
        allIcons.Clear();

        AppendCategoryIconsImpl(action::kIcons,        action::kIconCount);
        AppendCategoryIconsImpl(activitie::kIcons,     activitie::kIconCount);
        AppendCategoryIconsImpl(alert::kIcons,         alert::kIconCount);
        AppendCategoryIconsImpl(android::kIcons,       android::kIconCount);
        AppendCategoryIconsImpl(audio_video::kIcons,   audio_video::kIconCount);
        AppendCategoryIconsImpl(busines::kIcons,       busines::kIconCount);
        AppendCategoryIconsImpl(communicate::kIcons,   communicate::kIconCount);
        AppendCategoryIconsImpl(communication::kIcons, communication::kIconCount);
        AppendCategoryIconsImpl(content::kIcons,       content::kIconCount);
        AppendCategoryIconsImpl(device::kIcons,        device::kIconCount);
        AppendCategoryIconsImpl(editor::kIcons,        editor::kIconCount);
        AppendCategoryIconsImpl(file::kIcons,          file::kIconCount);
        AppendCategoryIconsImpl(hardware::kIcons,      hardware::kIconCount);
        AppendCategoryIconsImpl(home::kIcons,          home::kIconCount);
        AppendCategoryIconsImpl(household::kIcons,     household::kIconCount);
        AppendCategoryIconsImpl(image::kIcons,         image::kIconCount);
        AppendCategoryIconsImpl(map::kIcons,           map::kIconCount);
        AppendCategoryIconsImpl(navigation::kIcons,    navigation::kIconCount);
        AppendCategoryIconsImpl(notification::kIcons,  notification::kIconCount);
        AppendCategoryIconsImpl(place::kIcons,         place::kIconCount);
        AppendCategoryIconsImpl(privacy::kIcons,       privacy::kIconCount);
        AppendCategoryIconsImpl(search::kIcons,        search::kIconCount);
        AppendCategoryIconsImpl(social::kIcons,        social::kIconCount);
        AppendCategoryIconsImpl(text::kIcons,          text::kIconCount);
        AppendCategoryIconsImpl(toggle::kIcons,        toggle::kIconCount);
        AppendCategoryIconsImpl(transit::kIcons,       transit::kIconCount);
        AppendCategoryIconsImpl(travel::kIcons,        travel::kIconCount);
        AppendCategoryIconsImpl(ui_action::kIcons,     ui_action::kIconCount);
    }

    // ------------------------------ Categories ------------------------------

    void RebuildCategoryButtonsFromData()
    {
        for(int i = 0; i < categoryButtons.GetCount(); ++i)
            categoryButtons[i].Remove();
        categoryButtons.Clear();

        Index<String> uniq;
        for(const auto& ic : allIcons)
            uniq.FindAdd(ic.category);

        int btnW = DPI(85);
        int btnH = DPI(24);

        for(int i = 0; i < uniq.GetCount(); ++i) {
            String          cat = uniq[i];
            DragBadgeButton& b  = categoryButtons.Add();
            SetupPillButton(b, cat)
                .SetMode(DragBadgeButton::DRAGABLE)
                .SetTileSize(Size(btnW, btnH))
                .SetBaseColors(CopyButtonColor(), MainFrameColor(), MainGrayColor())
                .SetSelectionColors(ProcessButtonColor(), SColorHighlightText(), White())
                .SetFont(StdFont().Bold().Height(DPI(11)))
                .SetRadius(DPI(10))
                .SetStroke(1);
            b.WhenAction = [=] { OnCategoryClicked(cat); };
            categoryCard.AddFixed(b, btnW, btnH);
        }

        if(currentCategory.IsEmpty() && uniq.GetCount() > 0)
            currentCategory = uniq[0];

        for(int i = 0; i < categoryButtons.GetCount(); ++i) {
            String lab = AsString(categoryButtons[i].GetLabel());
            categoryButtons[i].SetSelected(lab == currentCategory);
        }
        categoryCard.Layout();
    }

    void OnCategoryClicked(const String& cat)
    {
        currentCategory = cat;
        page_index      = 0;
        for(int i = 0; i < categoryButtons.GetCount(); ++i) {
            String lab = AsString(categoryButtons[i].GetLabel());
            categoryButtons[i].SetSelected(lab == cat);
        }
        RebuildIconGrid();
    }

    void OnCategoryDroppedToBin(const String& cat)
    {
        for(int i = 0; i < allIcons.GetCount(); ++i) {
            const IconRecord& r = allIcons[i];
            if(r.category == cat && r.style == currentStyle)
                binCard.AddIconUnique(r);
        }
        UpdateOutputPaneInfo();
    }

    bool PassesCategory(const IconRecord& r) const
    {
        if(currentCategory.IsEmpty())
            return true;
        return r.category == currentCategory;
    }

    bool PassesFilter(const IconRecord& r) const
    {
        // Style
        if(r.style != currentStyle)
            return false;

        // Category
        if(!currentCategory.IsEmpty() && r.category != currentCategory)
            return false;

        // Search text (by icon name)
        if(searchFilter.IsEmpty())
            return true;

        String n = ToLower(r.name);
        String f = ToLower(searchFilter);
        return n.Find(f) >= 0;
    }

    void UpdatePageSizeFromIcon()
    {
        // Simple heuristic; could be made dynamic if needed.
        int viewW = 1000;
        int viewH = 600;
        int cols  = max(1, viewW / tileSizes.cx);
        int rows  = max(2, viewH / tileSizes.cy);
        page_size = clamp(cols * rows, 40, 400);
    }

    void RebuildIconGrid()
    {
        // Remove old tiles from the card
        for(int i = 0; i < iconTiles.GetCount(); ++i)
            iconTiles[i].Remove();
        iconTiles.Clear();

        UpdatePageSizeFromIcon();
        const int px = ValueToIntDef(iconSizeDrop.Get(), 48);

        // Build filtered index list
        Vector<int> idx;
        idx.Reserve(allIcons.GetCount());
        for(int i = 0; i < allIcons.GetCount(); ++i) {
            if(PassesFilter(allIcons[i]))
                idx.Add(i);
        }

        const int total = idx.GetCount();
        const int pages = max(1, (total + page_size - 1) / page_size);
        page_index      = clamp(page_index, 0, pages - 1);
        const int start = page_index * page_size;
        const int end   = min(start + page_size, total);

        Size tile = tileSizes;

        for(int k = start; k < end; ++k) {
            const IconRecord& rec     = allIcons[idx[k]];
            DragBadgeButton&  tileBtn = iconTiles.Create();

            Image base    = RenderSvgIconToImage(rec, px);
            Image preview = TintIcon(base, currentTintColor);

            SetupIconTile(tileBtn, rec, tile, preview)
                .SetMode(DragBadgeButton::DRAGABLE);

            itemsCard.AddFixed(tileBtn, tile.cx, tile.cy);
        }

        itemsCard.Layout();
        UpdateOutputPaneInfo();
    }

    // ------------------------------ Output pane -----------------------------

    void UpdateOutputPaneInfo()
    {
        int filtered = 0;
        for(const auto& ic : allIcons)
            if(PassesFilter(ic))
                ++filtered;

        const Vector<IconRecord>& sel = binCard.GetIcons();
        int px = ValueToIntDef(iconSizeDrop.Get(), 48);

		String ek;
		switch(currentExportKind) {
		case EX_CODE_IML: ek = "Code iml format";         break;
		case EX_CODE_RAW: ek = "Code RAW (UiMakeIcon)";   break;
		case EX_CODE_RLE: ek = "Code RLE (UiMakeIcon)";   break;
		case EX_SVG:      ek = "SVG";                     break;
		case EX_IML:      ek = "IML";                     break;
		case EX_ICO:      ek = "ICO";                     break;
		case EX_PNG:      ek = "PNG";                     break;
		case EX_JPG:      ek = "JPG";                     break;
		}

        String out;
        out << " " << APP_TITLE << " " << APP_VER << "\n";
        out << " Style: " << kStyleLabel[(int)currentStyle]
            << "   Category: " << (currentCategory.IsEmpty()
                                   ? String("<none>")
                                   : currentCategory) << "\n";
        out << " Filter: \"" << searchFilter << "\"\n";
        out << " Total icons loaded: " << allIcons.GetCount()
            << "   Matching filter: " << filtered << "\n";
        out << " Bin count: " << sel.GetCount()
            << "   Export type: " << ek
            << "   Size: " << px << " px\n";

        codeOutput.Set(out.ToWString());
    }

    // ------------------------------ Clipboard helpers -----------------------

	void CopyInlineHeaderSnippet()
	{
	    const Vector<IconRecord>& sel = binCard.GetIcons();
	    if(sel.IsEmpty()) {
	        PromptOK("Bin is empty.");
	        return;
	    }
	
	    String snippet;
	
	    switch(currentExportKind) {
	    case EX_CODE_RAW:
	        // RAW header for UiMakeIcon
	        snippet = MakeInlineUiIconHeader(false, false);
	        break;
	    case EX_CODE_RLE:
	        // RLE header for UiMakeIcon
	        snippet = MakeInlineUiIconHeader(false, true);
	        break;
	    default:
	        // Default for EX_CODE_IML and all non-code export kinds:
	        // copy the IML-style header.
	        snippet = MakeInlineImageHeaderIML(false);
	        break;
	    }
	
	    codeOutput.Set(snippet.ToWString());
	    WriteClipboardText(snippet);
	}

    // ------------------------------ Export entry point ----------------------

	void ProcessSelected()
	{
	    switch(currentExportKind) {
	    case EX_CODE_IML: ExportInlineHeaderIML();  break;
	    case EX_CODE_RAW: ExportInlineHeaderRaw();  break;
	    case EX_CODE_RLE: ExportInlineHeaderRle();  break;
	    case EX_SVG:      ExportSVG();              break;
	    case EX_IML:      ExportIML();              break;
	    case EX_ICO:      ExportICO();              break;
	    case EX_PNG:      ExportPNG();              break;
	    case EX_JPG:      ExportJPG();              break;
	    }
	}


    // ------------------------------ PNG export ------------------------------

    void ExportPNG()
    {
        const Vector<IconRecord>& sel = binCard.GetIcons();
        if(sel.IsEmpty()) {
            PromptOK("Bin is empty.");
            return;
        }

        FileSel fs;
        fs.Type("Folder", ".*");
        fs.ActiveDir(IsEmpty(lastExportDir)
                     ? GetFileFolder(GetExeDirFile("png_export"))
                     : lastExportDir);
        if(!fs.ExecuteSelectDir())
            return;

        int px    = ValueToIntDef(iconSizeDrop.Get(), 48);
        int total = sel.GetCount();

        Progress pi("Exporting PNG...", total);
        String   log;
        log << "// PNG export (" << px << " px)\n";
        log << "// Folder: " << ~fs << "\n\n";

        for(int i = 0; i < total; ++i) {
            if(pi.Canceled())
                break;
            const IconRecord& r = sel[i];

            Image base = RenderSvgIconToImage(r, px);
            Image img  = TintIcon(base, currentTintColor);
            if(img.IsEmpty())
                continue;

            String dir  = AppendFileName(~fs, kStyleLabel[(int)r.style]);
            RealizeDirectory(dir);
            String file = AppendFileName(dir, Format("%s-%d.png", ~r.name, px));
            if(!PNGEncoder().SaveFile(file, img)) {
                PromptOK(Format("Failed to save: %s", file));
                break;
            }
            log << "//   " << kStyleLabel[(int)r.style]
                << " | " << r.category << " / " << r.name
                << " -> " << file << "\n";
            pi.Step();
        }

        lastExportDir = ~fs;
        codeOutput.Set(log.ToWString());
        PromptOK("PNG export finished.");
    }

    // ------------------------------ JPG export ------------------------------

    void ExportJPG()
    {
        const Vector<IconRecord>& sel = binCard.GetIcons();
        if(sel.IsEmpty()) {
            PromptOK("Bin is empty.");
            return;
        }

        FileSel fs;
        fs.Type("Folder", ".*");
        fs.ActiveDir(IsEmpty(lastExportDir)
                     ? GetFileFolder(GetExeDirFile("jpg_export"))
                     : lastExportDir);
        if(!fs.ExecuteSelectDir())
            return;

        int px    = ValueToIntDef(iconSizeDrop.Get(), 48);
        int total = sel.GetCount();

        Progress pi("Exporting JPG...", total);
        String   log;
        log << "// JPG export (" << px << " px)\n";
        log << "// Folder: " << ~fs << "\n\n";

        for(int i = 0; i < total; ++i) {
            if(pi.Canceled())
                break;
            const IconRecord& r = sel[i];

            Image base = RenderSvgIconToImage(r, px);
            Image img  = TintIcon(base, currentTintColor);
            if(img.IsEmpty())
                continue;

            String dir  = AppendFileName(~fs, kStyleLabel[(int)r.style]);
            RealizeDirectory(dir);
            String file = AppendFileName(dir, Format("%s-%d.jpg", ~r.name, px));
            if(!JPGEncoder().SaveFile(file, img)) {
                PromptOK(Format("Failed to save: %s", file));
                break;
            }
            log << "//   " << kStyleLabel[(int)r.style]
                << " | " << r.category << " / " << r.name
                << " -> " << file << "\n";
            pi.Step();
        }

        lastExportDir = ~fs;
        codeOutput.Set(log.ToWString());
        PromptOK("JPG export finished.");
    }

    // ------------------------------ ICO export ------------------------------

    void ExportICO()
    {
        const Vector<IconRecord>& sel = binCard.GetIcons();
        if(sel.IsEmpty()) {
            PromptOK("Bin is empty.");
            return;
        }

        FileSel fs;
        fs.Type("Folder", ".*");
        fs.ActiveDir(IsEmpty(lastExportDir)
                     ? GetFileFolder(GetExeDirFile("ico_export"))
                     : lastExportDir);
        if(!fs.ExecuteSelectDir())
            return;

        int total = sel.GetCount();

        Progress pi("Exporting ICO...", total);
        String   log;
        log << "// ICO export (sizes: 16,24,32,48,64)\n";
        log << "// Folder: " << ~fs << "\n\n";

        for(int i = 0; i < total; ++i) {
            if(pi.Canceled())
                break;
            const IconRecord& r = sel[i];

            // Render a reasonably high-res base (we downscale inside SaveIcon64)
            Image base = TintIcon(RenderSvgIconToImage(r, 256), currentTintColor);
            if(base.IsEmpty())
                continue;

            String file = AppendFileName(~fs,
                                         Format("%s_%s.ico", ~r.name, kStyleLabel[(int)r.style]));
            if(!SaveIcon64(file, base)) {
                PromptOK(Format("Failed to save ICO: %s", file));
                break;
            }
            log << "//   " << kStyleLabel[(int)r.style]
                << " | " << r.category << " / " << r.name
                << " -> " << file << "\n";
            pi.Step();
        }

        lastExportDir = ~fs;
        codeOutput.Set(log.ToWString());
        PromptOK("ICO export finished.");
    }

    // ------------------------------ IML manifest helper ---------------------

    void ExportIML()
    {
        const Vector<IconRecord>& sel = binCard.GetIcons();
        if(sel.IsEmpty()) {
            PromptOK("Bin is empty.");
            return;
        }

        int px    = ValueToIntDef(iconSizeDrop.Get(), 48);
        int total = sel.GetCount();

        FileSel fs;
        fs.Type("Text manifest", "*.txt");
        String baseDir =
            IsEmpty(lastManifestPath)
                ? (IsEmpty(lastExportDir)
                       ? GetFileFolder(GetExeDirFile("iml_manifest.txt"))
                       : lastExportDir)
                : GetFileFolder(lastManifestPath);
        fs.ActiveDir(baseDir);
        fs <<= AppendFileName(baseDir, "iml_manifest.txt");
        if(!fs.ExecuteSaveAs("Save IML helper manifest as"))
            return;

        String outpath = ~fs;
        lastManifestPath = outpath;
        lastExportDir    = GetFileFolder(outpath);

        String manifest;
        String snippet;

        manifest << "# IML manifest helper (Material Symbols)\n";
        manifest << "# Suggested IML IDs -> PNG filenames at " << px << " px\n\n";

        snippet  << "// ---- IML PNG glue (helper) ----\n";
        snippet  << "// For each entry:\n";
        snippet  << "//   1) Export PNGs with PNG type into a folder.\n";
        snippet  << "//   2) Import into your .iml via TheIDE Image Editor.\n\n";

        for(int i = 0; i < total; ++i) {
            const IconRecord& r = sel[i];
            String id = Format("Icon_%s_%s_%s_%d",
                               ToUpper(r.category).Begin(),
                               ToUpper(kStyleLabel[(int)r.style]).Begin(),
                               ToUpper(r.name).Begin(),
                               px);
            String file = Format("%s-%d.png", ~r.name, px);

            manifest << id << " -> " << file << "\n";
            snippet  << "// " << id << " -> " << file
                     << "   (" << r.category << ", " << kStyleLabel[(int)r.style] << ")\n";
        }

        FileOut fo(outpath);
        if(fo.IsOpen()) {
            fo << manifest;
            fo.Close();
        }

        String log;
        log << snippet << "\n";
        log << "// Manifest written to:\n//   " << outpath << "\n";
        codeOutput.Set(log.ToWString());

        PromptOK(Format("IML helper manifest saved to:\n%s", outpath));
    }

    // ------------------------------ SVG export ------------------------------

    void ExportSVG()
    {
        const Vector<IconRecord>& sel = binCard.GetIcons();
        if(sel.IsEmpty()) {
            PromptOK("Bin is empty.");
            return;
        }

        FileSel fs;
        fs.Type("Folder", ".*");
        fs.ActiveDir(IsEmpty(lastExportDir)
                     ? GetFileFolder(GetExeDirFile("svg_export"))
                     : lastExportDir);
        if(!fs.ExecuteSelectDir())
            return;

        int total = sel.GetCount();

        Progress pi("Exporting SVG...", total);
        String   log;
        log << "// SVG export (raw minified SVG)\n";
        log << "// Folder: " << ~fs << "\n\n";

        for(int i = 0; i < total; ++i) {
            if(pi.Canceled())
                break;
            const IconRecord& r = sel[i];
            String svg = DecodeSvg(r);
            if(svg.IsEmpty())
                continue;

            String dir  = AppendFileName(~fs, kStyleLabel[(int)r.style]);
            RealizeDirectory(dir);
            String file = AppendFileName(dir, Format("%s.svg", ~r.name));
            if(!SaveFile(file, svg)) {
                PromptOK(Format("Failed to save: %s", file));
                return;
            }
            log << "//   " << kStyleLabel[(int)r.style]
                << " | " << r.category << " / " << r.name
                << " -> " << file << "\n";
            pi.Step();
        }

        lastExportDir = ~fs;
        codeOutput.Set(log.ToWString());
        PromptOK("SVG export finished.");
    }

// ------------------------------ Code header generation ------------------
//
// New formats for UiMakeIcon:
//
//   Data layout in each static array:
//
//       [0]   = low  byte of (w_raw)
//       [1]   = high byte of (w_raw)
//       [2]   = low  byte of (h)
//       [3]   = high byte of (h)
//
//       where:
//           w_raw = width, with bit 15 used as "RLE flag"
//                   bit 15 = 0 -> RAW  RGBA payload
//                   bit 15 = 1 -> RLE  payload (run-length encoded)
//
//   RAW payload:
//       - Row-major RGBA: r,g,b,a per pixel
//
//   RLE payload:
//       - Sequence of runs:
//             uint16  run_length   (1..65535), little-endian
//             byte    r
//             byte    g
//             byte    b
//             byte    a
//
//       - Decoding stops after width*height pixels have been written.
//
//   The shared factory:
//
//       inline Upp::Image UiMakeIcon(const unsigned char* data);
//
//   lives in UiDraw.h and auto-detects RAW vs RLE from the width flag.
// ------------------------------------------------------------------------

// Core generator for UiMakeIcon-style headers.
//   with_guard == true : wraps output in a header guard + includes
//   use_rle    == true : RLE-encode payload, otherwise RAW RGBA
String MakeInlineUiIconHeader(bool with_guard, bool use_rle)
{
    const Vector<IconRecord>& sel = binCard.GetIcons();
    int total = sel.GetCount();
    if(total <= 0)
        return String("// No icons in export bin.\n");

    int px = ValueToIntDef(iconSizeDrop.Get(), 48);

    String out;
    out << "// Inline U++ icons exported from Symbols Icon Picker (" << APP_VER << ")\n";
    out << "// Format: UiMakeIcon " << (use_rle ? "RLE" : "RAW") << " data blocks.\n";
    out << "//   - First 4 bytes: width/height header as described below.\n";
    out << "//   - Payload: " << (use_rle ? "RLE runs (uint16 length + RGBA)\n" : "raw RGBA (r,g,b,a per pixel)\n");
    out << "//   - Factory: inline Upp::Image ICON_*(void) calls UiMakeIcon(DATA_*).\n\n";

    if(with_guard) {
        const char* guard = use_rle
                                ? "MATERIAL_SYMBOLS_INLINE_UIICON_RLE_H"
                                : "MATERIAL_SYMBOLS_INLINE_UIICON_RAW_H";
        out << "#ifndef " << guard << "\n";
        out << "#define " << guard << "\n\n";
        out << "#include <CtrlLib/CtrlLib.h>\n";
        out << "#include <Ui/UiDraw.h>\n\n";
    }
    else {
        out << "// NOTE: Ensure the including file sees UiMakeIcon declaration, e.g.:\n";
        out << "//   #include <Ui/UiDraw.h>\n\n";
    }

    for(int i = 0; i < total; ++i) {
        const IconRecord& r = sel[i];

        Image base = RenderSvgIconToImage(r, px);
        Image img  = TintIcon(base, currentTintColor);
        if(img.IsEmpty())
            continue;

        Size sz = img.GetSize();
        int  w  = sz.cx;
        int  h  = sz.cy;

        // Width must fit in 15 bits for the flag scheme.
        unsigned int w_raw = (unsigned int)w;
        if(w_raw > 0x7FFFu)
            w_raw = 0x7FFFu;

        if(use_rle)
            w_raw |= 0x8000u; // set RLE flag

        unsigned int hh = (unsigned int)h;

        String idcore  = MakeIconIdCore(r, px);   // e.g. ACTION_OUTLINED_VISIBILITY_48
        String dataSym = "DATA_" + idcore;
        String funcSym = "ICON_" + idcore;

        const char* typeTitle = kStyleLabel[(int)r.style];

        // Per-icon comment
        out << "// Category: " << r.category
            << " | Type: " << typeTitle
            << " | Icon: " << r.name
            << " | Size: " << w << "x" << h
            << " | Format: " << (use_rle ? "UiMakeIcon RLE" : "UiMakeIcon RAW")
            << "\n";

        out << "static const unsigned char " << dataSym << "[] = {\n    ";

        int   col   = 0;
        bool  first = true;

        auto EmitByte = [&](unsigned int b) {
            if(!first) {
                out << ", ";
            }
            first = false;
            out << "0x" << FormatIntHex((int)(b & 0xFFu), 2);
            ++col;
            if(col >= 16) {
                out << "\n    ";
                col = 0;
            }
        };

        // Header: w_raw (little-endian), h (little-endian)
        EmitByte(w_raw & 0xFFu);
        EmitByte((w_raw >> 8) & 0xFFu);
        EmitByte(hh & 0xFFu);
        EmitByte((hh >> 8) & 0xFFu);

        // Payload
        if(use_rle) {
            // RLE over RGBA, row-major
            int total_px = w * h;
            if(total_px > 0) {
                int idx_px   = 0;
                int cur_x    = 0;
                int cur_y    = 0;
                RGBA prev    = img[0][0];
                unsigned int run = 1;

                for(idx_px = 1; idx_px < total_px; ++idx_px) {
                    ++cur_x;
                    if(cur_x >= w) {
                        cur_x = 0;
                        ++cur_y;
                    }
                    const RGBA& p = img[cur_y][cur_x];
                    if(p.r == prev.r && p.g == prev.g && p.b == prev.b && p.a == prev.a && run < 65535u) {
                        ++run;
                    }
                    else {
                        // Emit previous run
                        EmitByte(run & 0xFFu);
                        EmitByte((run >> 8) & 0xFFu);
                        EmitByte(prev.r);
                        EmitByte(prev.g);
                        EmitByte(prev.b);
                        EmitByte(prev.a);

                        prev = p;
                        run  = 1;
                    }
                }

                // Emit last run
                EmitByte(run & 0xFFu);
                EmitByte((run >> 8) & 0xFFu);
                EmitByte(prev.r);
                EmitByte(prev.g);
                EmitByte(prev.b);
                EmitByte(prev.a);
            }
        }
        else {
            // RAW RGBA payload
            for(int y = 0; y < h; ++y) {
                const RGBA* row = img[y];
                for(int x = 0; x < w; ++x) {
                    const RGBA& p = row[x];
                    EmitByte(p.r);
                    EmitByte(p.g);
                    EmitByte(p.b);
                    EmitByte(p.a);
                }
            }
        }

        out << "\n};\n\n";

        // Factory, shared UiMakeIcon
        out << "inline Upp::Image " << funcSym << "()\n";
        out << "{\n";
        out << "    return Upp::UiMakeIcon(" << dataSym << ");\n";
        out << "}\n\n";
    }

    if(with_guard) {
        const char* guard = use_rle
                                ? "MATERIAL_SYMBOLS_INLINE_UIICON_RLE_H"
                                : "MATERIAL_SYMBOLS_INLINE_UIICON_RAW_H";
        out << "#endif // " << guard << "\n";
    }

    return out;
}

// IML-style inline header (wrapper around the RAW UiMakeIcon header).
// This keeps the older "Code iml format" behaviour, but internally uses
// UiMakeIcon RAW data blocks instead of hand-written decode loops.
String MakeInlineImageHeaderIML(bool with_guard)
{
    String body = MakeInlineUiIconHeader(false, false); // RAW UiMakeIcon

    String out;
    out << "// U++ IML-style inline icons exported from Symbols Icon Picker ("
        << APP_VER << ")\n";
    out << "// This header is intended to be drop-in compatible with typical U++ usage:\n";
    out << "//   - You get Upp::Image factories for each icon (ICON_... functions).\n";
    out << "//   - You can use them anywhere you would use an IML icon.\n";
    out << "//   - No external PNG/IML files are required; everything is in this header.\n";
    out << "//\n";
    out << "// Internally this uses UiMakeIcon RAW blocks with a small 4-byte header\n";
    out << "// and either raw RGBA or (optionally) RLE payloads.\n";
    out << "//\n";
    out << "// To use this header, make sure the including module has:\n";
    out << "//   #include <CtrlLib/CtrlLib.h>\n";
    out << "//   #include <Ui/UiDraw.h>   // for UiMakeIcon\n";
    out << "//   using namespace Upp;\n";
    out << "//\n";

    if(with_guard) {
        out << "#ifndef ICON_PICKER_IML_STYLE_ICONS_H\n";
        out << "#define ICON_PICKER_IML_STYLE_ICONS_H\n\n";
        out << "#include <CtrlLib/CtrlLib.h>\n";
        out << "#include <Ui/UiDraw.h>\n\n";
    }

    out << body;

    if(with_guard) {
        out << "\n#endif // ICON_PICKER_IML_STYLE_ICONS_H\n";
    }

    return out;
}

// ------------------------------ Code export: IML-style header -----------

void ExportInlineHeaderIML()
{
    const Vector<IconRecord>& sel = binCard.GetIcons();
    if(sel.IsEmpty()) {
        PromptOK("Bin is empty.");
        return;
    }

    FileSel fs;
    fs.Type("C++ header", "*.h");

    String baseDir =
        IsEmpty(lastHeaderPath)
            ? (IsEmpty(lastExportDir)
                   ? GetFileFolder(GetExeDirFile("MaterialSymbolsInlineIML.h"))
                   : lastExportDir)
            : GetFileFolder(lastHeaderPath);

    fs.ActiveDir(baseDir);
    fs <<= AppendFileName(baseDir, "MaterialSymbolsInlineIML.h");

    if(!fs.ExecuteSaveAs("Save IML-style inline UiMakeIcon header as"))
        return;

    String header = MakeInlineImageHeaderIML(true);

    FileOut out(~fs);
    if(!out.IsOpen()) {
        PromptOK("Failed to open header file for writing.");
        return;
    }
    out << header;
    out.Close();

    lastHeaderPath = ~fs;
    lastExportDir  = GetFileFolder(lastHeaderPath);

    codeOutput.Set(header.ToWString());
    PromptOK(Format("IML-style inline UiMakeIcon header saved to:\n%s", lastHeaderPath));
}

// ------------------------------ Code export: RAW UiMakeIcon header ------

void ExportInlineHeaderRaw()
{
    const Vector<IconRecord>& sel = binCard.GetIcons();
    if(sel.IsEmpty()) {
        PromptOK("Bin is empty.");
        return;
    }

    FileSel fs;
    fs.Type("C++ header", "*.h");

    String baseDir =
        IsEmpty(lastHeaderPath)
            ? (IsEmpty(lastExportDir)
                   ? GetFileFolder(GetExeDirFile("MaterialSymbolsInlineRaw.h"))
                   : lastExportDir)
            : GetFileFolder(lastHeaderPath);

    fs.ActiveDir(baseDir);
    fs <<= AppendFileName(baseDir, "MaterialSymbolsInlineRaw.h");

    if(!fs.ExecuteSaveAs("Save RAW UiMakeIcon header as"))
        return;

    String header = MakeInlineUiIconHeader(true, false); // RAW

    FileOut out(~fs);
    if(!out.IsOpen()) {
        PromptOK("Failed to open header file for writing.");
        return;
    }
    out << header;
    out.Close();

    lastHeaderPath = ~fs;
    lastExportDir  = GetFileFolder(lastHeaderPath);

    codeOutput.Set(header.ToWString());
    PromptOK(Format("RAW UiMakeIcon header saved to:\n%s", lastHeaderPath));
}

// ------------------------------ Code export: RLE UiMakeIcon header ------

void ExportInlineHeaderRle()
{
    const Vector<IconRecord>& sel = binCard.GetIcons();
    if(sel.IsEmpty()) {
        PromptOK("Bin is empty.");
        return;
    }

    FileSel fs;
    fs.Type("C++ header", "*.h");

    String baseDir =
        IsEmpty(lastHeaderPath)
            ? (IsEmpty(lastExportDir)
                   ? GetFileFolder(GetExeDirFile("MaterialSymbolsInlineRle.h"))
                   : lastExportDir)
            : GetFileFolder(lastHeaderPath);

    fs.ActiveDir(baseDir);
    fs <<= AppendFileName(baseDir, "MaterialSymbolsInlineRle.h");

    if(!fs.ExecuteSaveAs("Save RLE UiMakeIcon header as"))
        return;

    String header = MakeInlineUiIconHeader(true, true); // RLE

    FileOut out(~fs);
    if(!out.IsOpen()) {
        PromptOK("Failed to open header file for writing.");
        return;
    }
    out << header;
    out.Close();

    lastHeaderPath = ~fs;
    lastExportDir  = GetFileFolder(lastHeaderPath);

    codeOutput.Set(header.ToWString());
    PromptOK(Format("RLE UiMakeIcon header saved to:\n%s", lastHeaderPath));
}
};

// ============================================================================
// App entry
// ============================================================================
GUI_APP_MAIN
{
    IconPickerApp().Run();
}
