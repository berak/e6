#include "WndClass.h"
#include "ComDlg.h"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <stdio.h>
#include <assert.h>

namespace w32
{

	static struct _
	{
		_() { InitCommonControls(); }
	} __;
    


    bool Docking::resize(HWND win, const RECT &r ) 
    {
        int x = r.left;
        int y = r.top;
        int h = r.bottom-r.top;
        int w = r.right -r.left;

        if ( IsWindowVisible( top ) ) {
        //if ( IsWindow( top ) ) {
            RECT rd;
            GetClientRect( top, &rd );
            y += rd.bottom;
            h -= rd.bottom;
        } 
            
        if ( IsWindowVisible( bottom ) ) {
        //if ( IsWindow( bottom ) ) {
            RECT rd;
            GetClientRect( bottom, &rd );
            h -= rd.bottom;
        } 

        if ( IsWindowVisible( left ) ) {
        //if ( IsWindow( left ) ) {
            RECT rd;
            GetClientRect( left, &rd );
            x += rd.right;
            w -= rd.right;
        }

        if ( IsWindowVisible( right ) ) {
        //if ( IsWindow( right ) ) {
            RECT rd;
            GetClientRect( right, &rd );
            w -= rd.right;
        }

        MoveWindow( win, x,y,w,h, 1 );
        return 1;
    }




    //
    // #.  #.  ###.   ##.     ##.   ####. #####.  ###.  #####.   #.     ##.  #.  #.
    // #.  #.        #. #.   #. #. #.       #.   #.  #.   #.    #.#.   #. #. #. #.
    // #.  #.   #.   #.  #. #.     #.###.   #.    #.      #.   #.  #. #.     #.#.
    // #.  #.   #.   #.  #. #.##.  #.       #.     ##.    #.   #####. #.     ##.#.
    // #.#.#.   #.   #.  #. #.  #. #.       #.       #.   #.   #.  #. #.  #. #.  #.
    // ##.##.  ###.  ####.   ###.  #####.   #.   ####.    #.   #.  #.  ###.  #.  #.
    //



    WidgetStack::WidgetStack() 
        : _front(0xbaadf00d), _size(0) 
    {  }

    size_t WidgetStack::add( HWND w )
    {
        if ( ! w )   return 0;
        _dwarfs [ _size ] = w;
        _front =  _size;       
        _size ++;
        return _size-1;
    }


    size_t WidgetStack::frontID() 
    {
        return  _front ;   
    }

    HWND WidgetStack::front() 
    {
        return ( (_front <_size) ? _dwarfs[ _front ] : 0  );  
    }

    HWND WidgetStack::raise( size_t i ) 
    {
        if ( i >= _size )   return (0);

        if ( _front != (0xbaadf00d) ) //first time, else there is always one dwarf in front.
        {
            ReleaseCapture();
            ShowWindow( _dwarfs[_front], SW_HIDE );
        }

        _front = i;
        ShowWindow( _dwarfs[_front], SW_SHOW );     
        return _dwarfs[_front];
    }



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Control
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HWND Control::createStatic( HWND prnt, int id, int x, int y, int w, int h ) {
        HWND hwnd = CreateWindowEx(
            WS_EX_STATICEDGE , "static", "", WS_VISIBLE | WS_CHILD,
            x,y,w,h,
            prnt, (HMENU)(id), getHinst(prnt), 0 );
        ShowWindow(hwnd,SW_SHOWNORMAL);
        return hwnd;
    }

    //! simple, automatic tooltip that covers the whole parent window
    HWND Control::createTooltip (HWND prnt, char *txt, int uid) {
        HINSTANCE hinz = getHinst(prnt);
        HWND      hwnd = CreateWindowEx( 
            WS_EX_TOPMOST,  TOOLTIPS_CLASS,   NULL,
            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP ,		
            CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
            prnt, 0, hinz, 0 );
        RECT rect;                  // for client area coordinates

        GetClientRect (prnt, &rect);    
        TOOLINFO ti    = {0};
        ti.cbSize      = sizeof(TOOLINFO);
        ti.uFlags      = TTF_SUBCLASS;
        ti.hwnd        = prnt;
        ti.hinst       = hinz;
        ti.uId         = uid;
        ti.lpszText    = txt;
        ti.rect.left   = rect.left;    // ToolTip will cover the whole window
        ti.rect.top    = rect.top;
        ti.rect.right  = rect.right;
        ti.rect.bottom = rect.bottom;       
        // SetWindowPos(hwnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
        SendMessage(hwnd, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);	
        return hwnd;
    } 


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // TrackBar
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HWND TrackBar::create( HWND prnt, int id, int x, int y, int w, int h ) {
        hwnd = CreateWindowEx(
            0,//WS_EX_TRANSPARENT ,
            "msctls_trackbar32", "",
            TBS_BOTH | TBS_NOTICKS | TBS_ENABLESELRANGE | WS_TABSTOP | WS_VISIBLE | WS_CHILD,
            x,y,w,h,
            prnt, (HMENU)(id), getHinst(prnt), 0 );
        ShowWindow(hwnd,SW_SHOWNORMAL);
        return hwnd;
    }

    long TrackBar::setPos(long p) {
        return PostMessage( hwnd, TBM_SETPOS, TRUE, p );
    }
    long TrackBar::getPos() {
        return SendMessage( hwnd, TBM_GETPOS,0,0 );
    }
    void TrackBar::setBounds( long m, long M ) {
        PostMessage( hwnd, TBM_SETRANGEMAX, TRUE, M );
        PostMessage( hwnd, TBM_SETRANGEMIN, TRUE, m );
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // StatBar
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HWND StatBar::create( HWND prnt,int id,int np, int *ap ) {
        hwnd = CreateStatusWindow( WS_CHILD|WS_VISIBLE|WS_BORDER, 0, prnt, id );
        if ( hwnd && ap )
            SendMessage(hwnd, SB_SETPARTS, np, (LPARAM)ap);
        return hwnd;
    }
    int StatBar::setPart( char *sz, int p, int flag ) {
        return SendMessage(hwnd, SB_SETTEXT, (p|flag), (LPARAM)sz);
    }
    int StatBar::setPartTip( char *sz, int p ) {
    #if (_WIN32_IE >= 0x0300)
        return SendMessage(hwnd, SB_SETTIPTEXT, p, (LPARAM)sz);
    #else
        assert(!"no part-tip support");
        return 0;
    #endif
    }
    int StatBar::setPartIcon( HICON ico, int p ) {
    #if (_WIN32_IE >= 0x0300)
        return SendMessage(hwnd, SB_SETICON, p, (LPARAM)ico);
    #else
        assert(!"no icon support");
        return 0;
    #endif
    }
    //! reflect parent size change:
    int StatBar::updateSize( bool scaleParts ) {
        RECT rs = {0};
        SendMessage(hwnd,WM_SIZE,0,0);
        GetClientRect(hwnd,&rs);
        if ( scaleParts ) {
            int ap[20] ={0};
            int n = SendMessage(hwnd, SB_GETPARTS, 20, (LPARAM)ap);
            if ( n ) {
                float f = (float)rs.right/ap[n-1]; 
                for (int i=0; i<n; i++ )
                    ap[i] = (int)((float)ap[i]*f);
                SendMessage(hwnd, SB_SETPARTS, n, (LPARAM)ap);
            } 
        }
        return rs.bottom;
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // ProgressBar
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HWND ProgressBar::create( HWND prnt, int x, int y, int w, int h, bool smooth ) {
        hwnd = CreateWindowEx(
                0, PROGRESS_CLASS, 0,
                WS_CHILD | WS_VISIBLE | (smooth?PBS_SMOOTH:0),
                x,y,w,h, 
                prnt, 0, getHinst(prnt), 0);         
        return hwnd;
    }
    void ProgressBar::setup(int hi,int lo, int step ) {
        SendMessage(hwnd, PBM_SETRANGE32, (WPARAM)lo, (LPARAM)hi ); 
        SendMessage(hwnd, PBM_SETSTEP, (WPARAM) step, 0); 
    }
    int ProgressBar::setPos(int p) {
        return SendMessage(hwnd, PBM_SETPOS, (WPARAM)p, 0);
    }
    int ProgressBar::getPos() {
        return SendMessage(hwnd, PBM_GETPOS, 0, 0);
    }
    int ProgressBar::step() {
        return SendMessage(hwnd, PBM_STEPIT, 0, 0);
    }



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // ComboBox
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HWND ComboBox::create( HWND prnt, int id, int x,int y,int w,int h ) {
        hwnd = CreateWindowEx( 0,"combobox","",
                    CBS_AUTOHSCROLL
                  | CBS_DROPDOWN 
                  //| CBS_SORT
                  | WS_VISIBLE
                  | WS_VSCROLL
                  | WS_CHILD
                  | WS_TABSTOP,
                  x,y,w,h,
                  prnt,(HMENU)id,getHinst(prnt),0);
        return hwnd;
    }
    int  ComboBox::clear()
    { return SendMessage( hwnd, CB_RESETCONTENT, 0, 0 ); }

    int  ComboBox::count()
    { return SendMessage( hwnd, CB_GETCOUNT, 0, 0 ); }

    int  ComboBox::add( char *s )
    { return SendMessage( hwnd, CB_INSERTSTRING, (unsigned int)(-1), (LPARAM)s ); }

    int  ComboBox::get( char *s, int i )
    { return SendMessage( hwnd, CB_GETLBTEXT, i, (LPARAM)s ); }

    int  ComboBox::del( int i )
    { return SendMessage( hwnd, CB_DELETESTRING, i, 0 ); }

    int  ComboBox::select( int i )
    { return SendMessage( hwnd, CB_SETCURSEL, i, 0 ); }

    int  ComboBox::getSelected()
    { return SendMessage( hwnd, CB_GETCURSEL, 0, 0 ); }

    int ComboBox::find(char *s) {
        char k[264];
        int  n = count();
        while ( n -- ) {
            k[0]=0;
            get(k,n);
            if ( ! strcmp(k,s) )
                break;
        }
        return n;
    }


    /*
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // ScrollBar
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ScrollBar::ScrollBar(HWND b, HWND e, float s, float v) 
    : bar(b),edit(e),scale(s) 
    {
        if (b&&e)
            setControl(v);
    }

    void ScrollBar::create(
                           HWND hDlg,
                           int id_b, int xb, int yb, int wb, int hb,
                           int id_e, int xe, int ye, int we, int he ) 
    {
        bar = CreateWindowEx(
            WS_EX_TRANSPARENT ,
            "msctls_trackbar32", "",
            TBS_BOTH | TBS_NOTICKS | TBS_ENABLESELRANGE | WS_TABSTOP | WS_VISIBLE | WS_CHILD,
            xb,yb,wb,hb,
            hDlg, (HMENU)(id_b),
            getHinst(hDlg),
            0 );
        edit = CreateWindowEx(
            WS_EX_CLIENTEDGE ,
            "edit", "",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD,
            xe,ye,we,he,
            hDlg, (HMENU)(id_e),
            getHinst(hDlg),
            0 );
        UpdateWindow( hDlg );
    }

    float ScrollBar::getValue()
    {
        TCHAR sz[512];
        Edit_GetText(edit, sz, 512);
        return (float)atof(sz);
    }

    float ScrollBar::update()
    {
        float f =  (float)SendMessage( bar, TBM_GETPOS,0,0 ) / scale;
        updateEdit(f);
        return f;
    }

    void ScrollBar::setControl(float f)
    {
        updateEdit(f);
        PostMessage( bar, TBM_SETPOS, TRUE, (long)(f * scale) );
    }

    void ScrollBar::updateEdit(float f)
    {
        static char sz[60] = {0};
        sprintf( sz, "%2.2f", f );
        SetWindowText( edit, sz );
    }

    void ScrollBar::setBounds( long m, long M )
    {
        PostMessage( bar, TBM_SETRANGEMAX, TRUE, (long)((float)M * scale) );
        PostMessage( bar, TBM_SETRANGEMIN, TRUE, (long)((float)m * scale) );
    }
    */

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Menu
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    Menu::Menu() 
    {
        menu=CreateMenu();  
    }
    void Menu::add( char *s, int id, int flag ) {
        AppendMenu( menu, (flag|MF_STRING), id, s );
    }
    void Menu::setEnabled( int id, bool on ) {
        EnableMenuItem( menu, id, (on?MF_ENABLED:MF_DISABLED) );
    }
    bool Menu::getEnabled( int id ) {
        return ( ! (GetMenuState(menu,id,0) & MF_DISABLED) );
    }
    void Menu::addSep() {
        AppendMenu( menu, MF_SEPARATOR, 0, 0 );
    }
    bool Menu::getChecked( int id ) {
        return ( GetMenuState(menu,id,0) & MF_CHECKED ) != 0;
    }
    void Menu::setChecked(int id, bool on) {
        CheckMenuItem( menu, id, (on?MF_CHECKED:MF_UNCHECKED) ); 
    }
	void Menu::insertItem( int itemId, const char * name )
	{
		static unsigned short counter = 0;

		MENUITEMINFO mii;
		ZeroMemory( &mii, sizeof(MENUITEMINFO) );

		HMENU hPopMenu = CreatePopupMenu();
		
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING | MIIM_SUBMENU;
        mii.fType = MFT_STRING;
        mii.cch = strlen( name );
        mii.dwTypeData = const_cast<char *>( name ); // :-(
		mii.wID = itemId; //counter++;
		mii.hSubMenu = hPopMenu;

		if ( InsertMenuItem( menu, 0, FALSE, & mii ) == TRUE )
		{
		}
	}
	void Menu::insertSubItem( int itemId, int subItemId, const char * name )
	{
		MENUITEMINFO mii;
		ZeroMemory( &mii, sizeof(MENUITEMINFO) );
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_SUBMENU;
		GetMenuItemInfo( menu, itemId, FALSE, & mii );
		HMENU hSubMenu = mii.hSubMenu;

		ZeroMemory( &mii, sizeof(MENUITEMINFO) );
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING | MFT_STRING;
        mii.wID = subItemId; 
        mii.cch = strlen( name );
        mii.dwTypeData = const_cast<char *>( name ); // :-(
        //    mii.fType |= MFT_SEPARATOR;
		
		if ( InsertMenuItem( hSubMenu, 0, FALSE, & mii ) == TRUE )
		{
		}
	}
    void Menu::attach(HWND prnt) 
    {
        SetMenu( prnt, menu );
        DrawMenuBar( prnt );
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // PopupMenu
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    PopupMenu::PopupMenu() {
        menu = CreatePopupMenu();
    }
    PopupMenu::~PopupMenu() {
        DestroyMenu( menu );
    }
    int PopupMenu::show( HWND parent, int x, int y ) {
        if ( x==0 && y==0 ) {
            POINT p; 
            GetCursorPos(&p);
            x=p.x; y=p.y;
        }
        return TrackPopupMenu( menu, TPM_LEFTALIGN/*|TPM_RETURNCMD*/ , x,y, 0, parent, 0 );
    }




    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // PropListBox
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    HWND PropListBox::create( HWND parent, HINSTANCE hinst, HMENU id, int x, int y, int w, int h, bool doEdit,int sorting ) {
        hwnd = CreateWindowEx( 
            WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | LVS_REPORT 
            | (doEdit?LVS_EDITLABELS:0)
            | (sorting>0?LVS_SORTASCENDING:(sorting<0?LVS_SORTDESCENDING:0)),
            x,y,w,h,  parent, id, hinst, 0 );    
        n_rows = n_items = 0;
        return hwnd;
    }

    int PropListBox::addRow( const char *name, int w ) {
        LVCOLUMN lvC = {0};	         // list view column structure
        lvC.mask     = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvC.fmt      = LVCFMT_LEFT;  // left-align column
        lvC.cx       = w;            // width of 1.st column in pixels
        lvC.pszText  = (CHAR*)name;
        int p = ListView_InsertColumn(hwnd, n_rows, &lvC);
        n_rows ++;
        return p;
    }

    int PropListBox::addProp( const char **sz ) {
        LVITEM     lvI = {0};				
        lvI.mask       = LVIF_TEXT | LVIF_PARAM;
        lvI.iItem      = 0xffff;
        lvI.pszText    = (CHAR*)sz[0]; 
        lvI.cchTextMax = strlen(sz[0])+1;      
        lvI.lParam     = (LPARAM)sz[0]; ///PPP what was this for ?? A:sorting!
        //lvI.lParam     = ListView_GetItemCount( hwnd );
        
        int p = ListView_InsertItem( hwnd, &lvI );
        for ( int i=1; i<n_rows; i++ )
            ListView_SetItemText( hwnd, p, i, (CHAR*)sz[i] );    
        n_items ++;
        return p;
    }

    void PropListBox::setPropVal( int p, int q, const char *val ) {
        if ( p >= n_items ) return;
        ListView_SetItemText( hwnd, p, q, (CHAR*)val );    
    }
    void PropListBox::getPropVal( int p, int q, char *val, int l ) {
        if ( p >= n_items ) return;
        ListView_GetItemText( hwnd, p, q, (CHAR*)val, l );    
    }
    void PropListBox::getPropName( int p, char *val, int l ) {
        if ( p >= n_items ) return;
        ListView_GetItemText( hwnd, p, 0, (CHAR*)val, l );    
    }

    void PropListBox::clear() {
        ListView_DeleteAllItems( hwnd );
        n_items=0;
    }

    void PropListBox::sort( int col, int up ) {
        // nested class stuff allows to declare a func inside another &)
        struct cmp {
            struct args {
                HWND hwnd;
                int  up;
                int  col;
            };
            static int CALLBACK func( LPARAM lp1, LPARAM lp2, LPARAM luser ) {
                cmp::args *arg = (cmp::args*)luser;
                /*
                char l[200];
                char r[200];
                ListView_GetItemText( arg->hwnd, lp1, arg->col, l, 200 );    
                ListView_GetItemText( arg->hwnd, lp2, arg->col, r, 200 );    
                */
                char *l = (char*)lp1;
                char *r = (char*)lp2;
                return arg->up * _stricmp(l,r);
            }  
        };

        cmp::args arg = { hwnd, (up?1:-1), col };
        ListView_SortItems( hwnd, cmp::func, (LPARAM)&arg );
    }




    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // TreeView
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    HTREEITEM TreeView::newEntry( HTREEITEM parent, HTREEITEM after, char *str, LPARAM data, int img ) 
	{       
		TV_INSERTSTRUCT ins  = {0};
        ins.hParent          = parent;
        ins.hInsertAfter     = after;
        ins.item.pszText     = str;
		ins.item.cchTextMax  = str ? strlen(str)+1 : 0;
        ins.item.lParam      = data;
		ins.item.mask        = ( str ? TVIF_TEXT : 0 ) | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE ;
        ins.item.iImage      = img;
        ins.item.iSelectedImage = img;
        cur.hItem = TreeView_InsertItem( hwnd, &ins );
        return ( cur.hItem );
    }

    HWND TreeView::create( HWND parent, HINSTANCE hinst, HMENU id, int x, int y, int w, int h, bool doIcon, bool doEdit ) {
        hwnd = CreateWindowEx( 
            WS_EX_CLIENTEDGE,
            "SysTreeView32",
            "",
              WS_CHILD 
            | WS_VISIBLE
            | WS_BORDER 
            | WS_TABSTOP
            | ( doEdit ? TVS_EDITLABELS : 0 )
            | TVS_HASBUTTONS 
            | TVS_HASLINES, 
            x,y,w,h,  parent, id, hinst, 0 );    
        if ( ! hwnd ) return 0;
        
        if ( doIcon ) {    
            HINSTANCE  hLib = LoadLibrary( "shell32.dll" );
            HIMAGELIST hImages = ImageList_Create(GetSystemMetrics(SM_CXSMICON)-2,GetSystemMetrics(SM_CYSMICON)-2,ILC_COLOR8|ILC_MASK,1,1);
            for ( int i=11; i<64; i++ ) {
                HICON hIcon = (HICON)LoadImage( hLib, MAKEINTRESOURCE(i), IMAGE_ICON, 0,0, LR_SHARED);
                if ( !  hIcon )
                    continue;
                ImageList_AddIcon(hImages,hIcon); 
                DestroyIcon(hIcon); 
            }
            FreeLibrary(hLib);
            TreeView_SetImageList(hwnd,hImages,TVSIL_NORMAL);
        }
        memset(&cur, 0, sizeof(TVITEM));
        return hwnd;
    }

    void TreeView::setImageList( HIMAGELIST  hImages ) {
        TreeView_SetImageList( hwnd, hImages, TVSIL_NORMAL );  
    }

    void TreeView::clearNode( HTREEITEM &node ) {
        TreeView_DeleteItem(hwnd, node);
        //~ HTREEITEM startItem = (
              //~ node
            //~ ? TreeView_GetNextSibling(hwnd, node) 
            //~ : TreeView_GetChild(hwnd, node) );
        
        //~ while ( startItem ) {
            //~ // cache, destroy later:
            //~ HTREEITEM it = TreeView_GetNextSibling(hwnd, startItem);
            //~ TreeView_DeleteItem(hwnd, startItem);
            //~ startItem = it;
        //~ }
    }

    void TreeView::clearAll() {
        TreeView_DeleteAllItems( hwnd );
    }

    TVITEM TreeView::getClickedItem() {
        TVITEM  it    = {0};
        DWORD	dwPos = GetMessagePos();
        TV_HITTESTINFO	htti  = {0};
        htti.pt.x = LOWORD(dwPos);
        htti.pt.y = HIWORD(dwPos);
        
        ScreenToClient(hwnd, &htti.pt);
        
        if ( ! (it.hItem = TreeView_HitTest(hwnd, &htti)))
            return it;
        
        TreeView_SelectItem(hwnd, it.hItem);
       
        char buf[0xff] = {0};
        it.mask   =  TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
        it.lParam = -1;
        it.pszText     = buf;
        it.cchTextMax  = 0xfe;
        TreeView_GetItem( hwnd, &it ); 

		printf( "click : %i %s\n", it.lParam, it.pszText );
        memcpy(&cur,&it,sizeof(TVITEM));
       
        return it;
    }


    /** recursive traversal of all nodes    <br>
     *  if node==0 traverse from root node
    **/
    void TreeView::recurseNode( HTREEITEM node, bool(*fn)(TV_ITEM*,void*), void *usr )
    {
        if ( !node ) 
            node = getRoot();

        char buf[0xff] = {0};
        TV_ITEM   it   = {0};
        it.mask        = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN;
        it.hItem       = node;
        it.pszText     = buf;
        it.cchTextMax  = 0xfe;
        
        TreeView_GetItem( hwnd, &it );
       
        fn( &it, usr );
        
        HTREEITEM next = 0;
        next = TreeView_GetChild(hwnd, node);
        if ( next ) 
            recurseNode( next, fn, usr );

        next = TreeView_GetNextSibling(hwnd, node); 
        if ( next )
            recurseNode( next, fn, usr );
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // ImageList
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ImageList::ImageList() 
    : strip(0)
    {}

    ImageList::~ImageList() { /*release();*/ }

    void ImageList::release() 
    {
        if ( strip )
            ImageList_Destroy( strip );
        strip=0;
    }

    BOOL ImageList::draw(HWND hwnd, int idx, int cx, int cy) 
    { 
        HDC hdc = GetDC(hwnd);        
        if ( hdc == NULL ) 
            return FALSE; 
        BOOL ok = ImageList_Draw(strip, idx, hdc, cx, cy, ILD_NORMAL);
        ReleaseDC(hwnd, hdc); 
        return ok;
    } 

    int ImageList::add( char *file ) //resource id
    {
        HBITMAP bmp = (HBITMAP)LoadImage(
            0,
            file,
            IMAGE_BITMAP ,
            0,0,
            LR_LOADFROMFILE|LR_CREATEDIBSECTION ); 
        
        int n = ImageList_Add(strip, bmp, 0); 
        DeleteObject(bmp);  // ImageList_Add did a copy
        return n; 
    } 

    int ImageList::add( int resid ) //resource id
    {
        HBITMAP bmp = (HBITMAP)LoadImage(
            0,
            MAKEINTRESOURCE(resid),
            IMAGE_BITMAP ,
            0,0,
            LR_LOADFROMFILE|LR_CREATEDIBSECTION ); 
        
        int n = ImageList_Add(strip, bmp, 0); 
        DeleteObject(bmp);  // ImageList_Add did a copy
        return n; 
    } 

    HIMAGELIST ImageList::create( int w, int h ) 
    {   
        strip  = ImageList_Create(w, h, ILC_COLOR , 0, 0); 
        return strip; 
    } 

    HIMAGELIST ImageList::create( char *lib ) 
    {   
        HINSTANCE hLib = LoadLibrary( lib );
        strip = ImageList_Create(GetSystemMetrics(SM_CXSMICON)-2,GetSystemMetrics(SM_CYSMICON)-2,ILC_COLOR8|ILC_MASK,1,1);
        for ( int i=0; i<0xff; i++ ) {
            HICON hIcon = (HICON)LoadImage( hLib, MAKEINTRESOURCE(i), IMAGE_ICON, 0,0, LR_SHARED);
            if ( ! hIcon )
                continue;
            ImageList_AddIcon(strip,hIcon); 
            DestroyIcon(hIcon); 
        }
        FreeLibrary(hLib);
        return strip;
    }
    HICON ImageList::getIcon( int idx, int flag ) {
        return ImageList_GetIcon( strip,idx,flag );
    }



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // ToolBar
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



    HWND ToolBar::create( HWND hwndParent, int id, char *image, int w, int h, bool hosted, bool flat ) 
    {        
        // Create a toolbar. 
        hwnd = CreateWindowEx(
            0,//TBSTYLE_EX_MIXEDBUTTONS, (ie_ver >= 4.7 only)
            TOOLBARCLASSNAME,
            (LPSTR) NULL, 
              WS_CHILD 
            | WS_VISIBLE
            | TBSTYLE_TOOLTIPS 
            | TBSTYLE_ALTDRAG
    #if (_WIN32_IE >= 0x0300)
            | TBSTYLE_AUTOSIZE  
            | TBSTYLE_TRANSPARENT 
            | TBSTYLE_LIST  
            | (flat ? TBSTYLE_FLAT : 0 )
    #endif
            | CCS_ADJUSTABLE
            | (hosted? CCS_NOPARENTALIGN|CCS_NORESIZE : 0), // when used in Rebar, etc.
            0, 0, 0, 0,
            hwndParent, 
            (HMENU) id,
            getHinst(hwndParent),
            NULL  ); 
        
        // required for backward compatibility : 
        SendMessage(hwnd, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0); 

        ImageList img;
            if ( image ) {
            img.create( w, h );
            img.add( image );
        } 
        // send null if no image or dummy will indent text !
    #if (_WIN32_IE >= 0x0300)
        SendMessage( hwnd, TB_SETIMAGELIST, 0, (LPARAM)img.strip );
    #else // load std bitmaps:
        TBADDBITMAP tbab;
        tbab.hInst = getHinst(hwndParent);//HINST_COMMCTRL;
        tbab.nID = (UINT)MAKEINTRESOURCE(image);//IDB_VIEW_SMALL_COLOR;
        SendMessage(hwnd, TB_ADDBITMAP, 3, (LPARAM)&tbab);
    #endif
        return hwnd;
    }
    HWND ToolBar::create( HWND hwndParent, int id, int bitmap, int w, int h, bool hosted ) 
    {        
        // Create a toolbar. 
        hwnd = CreateWindowEx(
            0,//TBSTYLE_EX_MIXEDBUTTONS, (ie_ver >= 4.7 only)
            TOOLBARCLASSNAME,
            (LPSTR) NULL, 
              WS_CHILD 
            | WS_VISIBLE
            | TBSTYLE_TOOLTIPS 
            | TBSTYLE_ALTDRAG
            | CCS_ADJUSTABLE
            | (hosted? CCS_NOPARENTALIGN|CCS_NORESIZE : 0), // when used in Rebar, etc.
            0, 0, 0, 0,
            hwndParent, 
            (HMENU) id,
            getHinst(hwndParent),
            NULL  ); 
        
        // required for backward compatibility : 
        SendMessage(hwnd, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0); 

        TBADDBITMAP tbab;
        tbab.hInst = getHinst(hwndParent);
        tbab.nID   = bitmap;
        SendMessage(hwnd, TB_ADDBITMAP, 30, (LPARAM)&tbab);
        return hwnd;
    }

    //! sz==0: no text.
    //! cmd==0: separator,img=width.
    void ToolBar::add( char *sz, int cmd, int img, int style ) 
    {	 
        int isz = sz ? SendMessage( hwnd, TB_ADDSTRING, 0, (LPARAM) sz ) : -1; 
        TBBUTTON tbb  = {0}; 
        tbb.iBitmap   = img; 
        tbb.idCommand = cmd; 
        tbb.fsState   = TBSTATE_ENABLED;
        tbb.fsStyle   = (cmd?style:TBSTYLE_SEP); //TBSTYLE_BUTTON ; 
        tbb.dwData    = 0; 
        tbb.iString   = isz;         
        SendMessage(hwnd, TB_ADDBUTTONS, (WPARAM) 1, (LPARAM)&tbb); 
    }

    void ToolBar::setImageList( HIMAGELIST  hImages ) 
    {
    #if (_WIN32_IE >= 0x0300)
        SendMessage( hwnd, TB_SETIMAGELIST, 0, (LPARAM)hImages );  
    #else
        assert(!"no imagelist support");
    #endif
    }

    void ToolBar::setToolImage(int t,int i) 
    {
    #if (_WIN32_IE >= 0x0300)
        TBBUTTONINFO tbbi;
        tbbi.cbSize = sizeof(TBBUTTONINFO);
        tbbi.dwMask = TBIF_IMAGE ;
        tbbi.iImage = i;
        SendMessage( hwnd, TB_SETBUTTONINFO, (WPARAM)t, (LPARAM)&tbbi );  
    #else
        assert(!"no toolimage support");
    #endif
    }

    //! needed if not hosted:
    void ToolBar::update( int x, int y )
    {
        SetWindowPos( hwnd, 0, x,y,0,0 ,SWP_NOSIZE );
        SendMessage( hwnd, TB_AUTOSIZE, 0, 0 ); 
        ShowWindow( hwnd, SW_SHOW ); 
    } 

    int ToolBar::numButtons()
    {
        return SendMessage(hwnd, TB_BUTTONCOUNT, 0, 0); 
    }





    #if (_WIN32_IE >= 0x0300)
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // ReBar
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    HWND Rebar::create( HWND hwndParent ) {
        // gnarrfff*@& !
        // InitCommonControls does NOT init rebars!
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC  = ICC_COOL_CLASSES | ICC_BAR_CLASSES;
        InitCommonControlsEx(&icex);

        hwnd = CreateWindowEx(
            //WS_EX_TOOLWINDOW|
            0,//WS_EX_STATICEDGE,
            REBARCLASSNAME,
            NULL,
            WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN
            | CCS_NODIVIDER
            | RBS_VARHEIGHT 
    #if (_WIN32_IE >= 0x0300)
            | RBS_AUTOSIZE ,
    #endif
            //| RBS_BANDBORDERS
            // | RBS_DBLCLKTOGGLE,
            0,0,0,0, 
            hwndParent,
            NULL,
            getHinst(hwndParent),
            NULL);

        if(!hwnd)
            return NULL;

        REBARINFO rbi;
        rbi.cbSize = sizeof(REBARINFO);  // Required 
        rbi.fMask  = 0;
        rbi.himl   = (HIMAGELIST)NULL;
        if(!SendMessage(hwnd, RB_SETBARINFO, 0, (LPARAM)&rbi))
            return NULL;

        return hwnd;
    }   


    void Rebar::addBand( char *txt, HWND child, int w, int h, bool nl, bool hid ) {
        REBARBANDINFO rbBand = {0};
        rbBand.cbSize     = sizeof(REBARBANDINFO);  // Required
        rbBand.fMask      = RBBIM_SIZE
                          | RBBIM_STYLE
                          | RBBIM_ID
                          | RBBIM_CHILDSIZE
                          | ( txt   ? RBBIM_TEXT  : 0 )
                          | ( child ? RBBIM_CHILD : 0 ) ; 
        rbBand.fStyle     = RBBS_CHILDEDGE
    #if (_WIN32_IE >= 0x0300)
                          | RBBS_GRIPPERALWAYS 
    #endif
                          | (hid?RBBS_HIDDEN:0)
                          | (nl ?RBBS_BREAK :0);//| RBBS_FIXEDBMP;
        rbBand.hbmBack    = 0;//LoadBitmap(g_hinst, MAKEINTRESOURCE(IDB_BACKGRND));   
        rbBand.lpText     = txt;
        rbBand.hwndChild  = child;
        rbBand.cyMinChild = h;
        rbBand.cxMinChild = w; //current=min
        rbBand.cx         = w;
        rbBand.wID        = count();

        SendMessage(hwnd, RB_INSERTBAND, (WPARAM)(-1), (LPARAM)&rbBand);
    }


    int Rebar::height() 
    { return SendMessage( hwnd, RB_GETBARHEIGHT, 0, 0 ); }


    int Rebar::count() 
    { return SendMessage( hwnd, RB_GETBANDCOUNT, 0, 0 ); }


    int Rebar::showBand(int id, bool on) 
    {   // we want the index, but get the actual position:
        int idx = SendMessage( hwnd, RB_IDTOINDEX, id, 0 ); 
        return SendMessage( hwnd, RB_SHOWBAND,idx,on);
    }


    #endif




    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Splitter
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void Splitter::setY( int top, int bot )
    {   y_top = top; y_bot = bot;   }


    int Splitter::calcPos( int x, int right, int b ) 
    {
        if (x<b) return b;
        if (x>right-b) return right-b;
        return x;
    }


    //! paint splitter
    void Splitter::blit(RECT &rc)
    {   PatBlt(hdc, (pos-w/2), rc.top + y_top, w, rc.bottom - y_bot, DSTINVERT);  }


    //! called on WM_LBUTTONDOWN
    LRESULT Splitter::startDrag(HWND hwnd, int xPos)
    {
        if ( xPos>pos+2*w || xPos<pos-2*w )
            return 0;

        SetCapture(hwnd);
        //HCURSOR cur = LoadCursor(getHinst(hwnd),MAKEINTRESOURCE(IDC_SIZEWE));
        //SetCursor(cur);

        RECT rcSplit;
        GetClientRect(hwnd, &rcSplit);

        // Get a DC (also used as a flag indicating we have capture)
        if (hdc)
            ReleaseDC(hwnd, hdc);
        hdc = GetDC(hwnd);

        pos = calcPos(xPos,rcSplit.right,30);
        // Draw splitter bar in initial position
        blit(rcSplit);
        return 0;
    }

    //! called on WM_MOUSE_MOVE
    LRESULT Splitter::drag( HWND hwnd,int xPos)
    {
        if (!hdc) 
            return 0;

        RECT rcSplit;
        GetClientRect(hwnd, &rcSplit);

        // Erase previous bar
        blit(rcSplit);
        // Calculate new position
        pos = calcPos(xPos,rcSplit.right,30);
        // Draw bar in new position
        blit(rcSplit);
        return 0;
    }

    //! called on WM_LBUTTONUP
    LRESULT Splitter::endDrag(HWND hwnd, int xPos)
    {
        if (!hdc) 
            return 0;

        RECT rcSplit;
        GetClientRect(hwnd, &rcSplit);

        // Erase previous bar
        blit(rcSplit);
        // Calculate new position
        pos = calcPos(xPos,rcSplit.right,30);

        // Clean up
        ReleaseCapture();
        ReleaseDC(hwnd, hdc);
        hdc = NULL;

        //HCURSOR cur = LoadCursor(getHinst(hwnd),MAKEINTRESOURCE(IDC_ARROW));
        //SetCursor(cur);
        SendMessage(hwnd,WM_SIZE,0,0);
        return 0;
    }



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // TrayIcon
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    /** 
     * trayicon is registered with id 
     * and sends messages (WM_USER + id) to parent
     * (where msg is in lParam)
    **/
    int TrayIcon::create( HWND prnt, int id, HICON ico, char *tip ) 
    {
        memset( &tnd, 0, sizeof(NOTIFYICONDATA) );
        tnd.cbSize		= sizeof(NOTIFYICONDATA);
        tnd.uID         = (UINT)id;
        tnd.hWnd		= prnt;
        tnd.uFlags      = NIF_MESSAGE;
        tnd.uCallbackMessage = WM_USER + id;
        if ( tip ) {
            strcpy( tnd.szTip, tip );
            tnd.uFlags |= NIF_TIP;
        }
        if ( ico ) {
            tnd.hIcon   = ico;
            tnd.uFlags |= NIF_ICON;
        }
        return Shell_NotifyIcon(NIM_ADD, &tnd);
    }

    int TrayIcon::destroy()
    {
        tnd.uFlags = 0;
        return Shell_NotifyIcon(NIM_DELETE, &tnd);
    }

    int TrayIcon::modifyTip( char *tip ) 
    {
        tnd.uFlags  = NIF_TIP;
        strcpy( tnd.szTip, tip );
        return Shell_NotifyIcon(NIM_MODIFY, &tnd);
    }

    int TrayIcon::modifyIcon( HICON ico ) 
    {
        tnd.uFlags  = NIF_ICON;
        tnd.hIcon   = ico;
        return Shell_NotifyIcon(NIM_MODIFY, &tnd);
    }


} // w32
