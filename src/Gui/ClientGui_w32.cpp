
#include "../e6/e6.h"
#include "../e6/e6_impl.h"
#include "../Gui/ClientGui.h"
#include "../main/w32/WndClass.h"
#include "../main/w32/ComDlg.h"

#include <map>

using e6::uint;



namespace ClientGui
{
    enum Units
    {
        CTL_HEIGHT_ROLLOUT  = 20,
        CTL_HEIGHT_ONE_UNIT = 18,
        CTL_SLIDER_RANGE    = 1000,
        CTL_ROLLOUT_ID      = 0x0800,
        CTL_BUTTON_ID       = 0x0900,
        CTL_MAX,
    };


    struct Item
    {
        HWND hwnd;      
        HWND val;      
        char name[100];
        char type[100];

        Item(const char *t,const char *n) 
            : hwnd(0),val(0) 
        { 
            strcpy( name, n );
            strcpy( type, t ); 
        }
		~Item()
		{
			if ( hwnd ) DestroyWindow( hwnd );	hwnd=0;
			if ( val )  DestroyWindow( val );	val=0;
		}

        uint setValue( const char * value ) 
        {
            if ( type[1] == 'l' ) // slider
            {
                int p=0;
                sscanf( value, "%i", &p );
                SendMessage( val, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) p );
            }
            else
            {
                SetWindowText( val, value );
            }
            return 1;
        }
        const char * getValue() const 
        {
            static char buf[200];
            buf[0]=0;
            if ( type[1] == 'l' ) // slider
            {
                sprintf( buf, "%d", SendMessage( val, TBM_GETPOS, 0, 0 ) );
            }
            else
            {
                GetWindowText( val, buf,200 );
            }
            return buf;
        }
        void show ( uint vis )
        {
            ShowWindow(hwnd,vis);
            //~ UpdateWindow(hwnd);
            ShowWindow(val,vis);
            //~ UpdateWindow(val);
        }
        uint update( HWND parent, uint x,uint y, uint w, uint h )
        {
            if ( ! hwnd )
            {
                HFONT hFont = (HFONT) GetStockObject( ANSI_VAR_FONT ); 
                hwnd = CreateWindowEx(
                    WS_EX_STATICEDGE , "static", name, WS_VISIBLE | WS_CHILD | WS_TABSTOP ,
                    x,y,60,h,
                    parent, (HMENU)(0x13f00), GetModuleHandle(0), 0 );
//                printf( "CREATED ITEM %s:  %x\n", name, hwnd );
                if ( ! stricmp( type, "edit" ) )
                {
                    val = CreateWindowEx(
                        WS_EX_STATICEDGE , "edit", "", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
                        x+60,y,w-60,h,
                        parent, (HMENU)(0x13f00), GetModuleHandle(0), 0 );
//                    printf( "CREATED ITEMV %s:  %x\n", type, val );
                }
                else
                if ( ! stricmp( type, "slider" ) )
                {
                    val = CreateWindowEx(
                        WS_EX_STATICEDGE , "msctls_trackbar32", name, TBS_BOTH | TBS_NOTICKS | TBS_ENABLESELRANGE | WS_VISIBLE | WS_CHILD | WS_TABSTOP ,
                        x+60,y,w-60,h,
                        parent, (HMENU)(0x13f00), GetModuleHandle(0), 0 );
//                    printf( "CREATED ITEMV %s:  %x\n", type, val );
                    SendMessage( val, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG(0, CTL_SLIDER_RANGE) );
                }
                SendMessage( hwnd, WM_SETFONT, (WPARAM) hFont, 1 );
                SendMessage( val, WM_SETFONT, (WPARAM) hFont, 1 );
            }
            MoveWindow( hwnd, x,y,60,h,1 );
            MoveWindow( val, x+60,y,w-60,h,1 );
//            printf( __FUNCTION__ " \t:  %x\t%5i%5i%5i%5i\t%s\n", hwnd,x,y,w,h,type );
            return 0;
        }
    };

    struct Rollout
    {
        typedef std::map< const char*, Item*, e6::StrLess > ItemMap;
        typedef ItemMap::iterator IIter;
        typedef ItemMap::const_iterator IIterC;

        ItemMap items;
        HWND hwnd;      
        HWND check;      
        char name[200];
        bool expanded;
        uint id;
        
        Rollout( const char * n ) 
            : hwnd(0)
            , check(0)
            , expanded(0)
            , id(0)
        { 
            strcpy( name, n ); 
        }

        ~Rollout() 
        {
            clear();

			if ( hwnd )   DestroyWindow( hwnd );	hwnd=0;
			if ( check )  DestroyWindow( check );	check=0;
        }

        uint clear()
        {
            for ( IIter it = items.begin(); it != items.end(); ++it )
            {
                E_DELETE( it->second );               
            }
            items.clear();           
            return 1;
        }

        uint expand( bool big )        
        {
            expanded = big;
            return big;
        }

        uint addItem( const char *type, const char *name )
        {
            items[ name ] = new Item(type,name);
            return items.size();
        }
        uint setValue( const char *name, const char * value ) 
        {
            IIter it = items.find(name);
            if ( it == items.end())
                return 0; 
            return it->second->setValue(value);
        }
        const char * getValue(  const char *name ) const 
        {
            IIterC it = items.find(name);
            if ( it == items.end())
                return 0; 
            return it->second->getValue();
        }

        uint init( HWND parent, uint id )
        {
            this->id = id;
            hwnd = CreateWindowEx(
                WS_EX_STATICEDGE , "static", "", WS_VISIBLE | WS_CHILD ,
                1,2,3,4,
                parent, (HMENU)(0x13f00), GetModuleHandle(0), 0 );
            HFONT hFont = (HFONT) GetStockObject( ANSI_VAR_FONT ); 
            check = CreateWindowEx(
                0 , "button", name, BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD,
                1,2,3,4,
                parent, (HMENU)(id), GetModuleHandle(0), 0 );
            SendMessage( check, WM_SETFONT, (WPARAM) hFont, 1 );
            ShowWindow(hwnd,SW_SHOWNORMAL);
//            printf( "CREATED ROLLOUT %s:  %x\n", name, hwnd );
            return 1;
        }

        uint update( HWND parent, uint x, uint y, uint w )
        {
            SendMessage( check, BM_SETCHECK, (expanded?BST_CHECKED:BST_UNCHECKED) , 0 );
//            printf( __FUNCTION__ " :  %x\t\t%5i%5i%5i%5i\n", hwnd,x,y,w,height() );
            MoveWindow(check, x,y+2,w,16, 1);
            MoveWindow(hwnd, x,y+CTL_HEIGHT_ROLLOUT,w,height()-CTL_HEIGHT_ROLLOUT, 1);
            uint _y = y + CTL_HEIGHT_ROLLOUT;
            for ( IIter it = items.begin(); it != items.end(); ++it )
            {
                Item *item = it->second;
                if ( _y >= y+height() ) 
                {
                    item->show( SW_HIDE );
                }
                else
                {
                    item->update( parent, x, _y, w, CTL_HEIGHT_ONE_UNIT );
                    item->show( SW_SHOWNORMAL );
                }
                _y += CTL_HEIGHT_ONE_UNIT;                
            }
            UpdateWindow(hwnd);
			return 1;
        }

        uint height() 
        {           
            uint h = CTL_HEIGHT_ROLLOUT ;
            if ( expanded )
                 h += items.size() * (CTL_HEIGHT_ONE_UNIT);
            return h;
        }
    };


    struct CPanel
        : e6::CName< Panel, CPanel >
        , w32::CWindow
    {
        typedef std::map< const char*, Rollout*, e6::StrLess > RolloutMap;
        typedef RolloutMap::iterator RMIter;
        typedef RolloutMap::const_iterator RMIterC;
        
        RolloutMap rolls;
        HWND hwnd;
        
        CPanel() : hwnd(0) {}
        ~CPanel() { clear(); }

        virtual uint clear()
        {
            for ( RMIter it = rolls.begin(); it != rolls.end(); ++it )
            {
                E_DELETE( it->second );               
            }
            rolls.clear();           
            return 1;
        }

        virtual uint addRollout( const char *name ) 
        {
            Rollout * rol = new Rollout( name );
            rol->init( hwnd, CTL_ROLLOUT_ID + rolls.size() );           
            rolls[ name ] = rol;
            return rolls.size();
        }

        virtual uint expand( const char *rollout, bool big )        
        {
            RMIter it = rolls.find(rollout);
            if ( it == rolls.end())
                return 0; 
            return it->second->expand(big);
        }

        virtual uint addControl( const char *rollout, const char *type, const char *name )
        {
            RMIter it = rolls.find(rollout);
            if ( it == rolls.end())
                return 0; 
            return it->second->addItem(type,name);
        }
        virtual uint setValue( const char *rollout, const char *name, const char * value ) 
        {
            RMIter it = rolls.find(rollout);
            if ( it == rolls.end())
                return 0; 
            return it->second->setValue(name,value);
        }
        virtual const char * getValue( const char *rollout, const char *name ) const 
        {
            RMIterC it = rolls.find(rollout);
            if ( it == rolls.end())
                return 0; 
            return it->second->getValue(name);
        }

        virtual uint init( void * parent )
        {
            RECT r;
            GetClientRect( (HWND)parent, &r );
            hwnd   = CWindow::create( "panel","panel", 0,0,50,50, WS_BORDER | WS_TABSTOP|WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|CS_OWNDC, 0, (HWND)parent, GetModuleHandle(0), 0, WS_EX_STATICEDGE );
//            printf( "CREATED PANEL:  %x\n", hwnd );
            return ( hwnd != 0 );
        };
        
        virtual uint update()
        {
            if ( ! hwnd) return 0;

            RECT r;
//            printf( __FUNCTION__ " :  %x\n", hwnd );
            GetClientRect( hwnd, &r );
            InvalidateRect( hwnd, &r, 0 );
            uint x=r.left;
            uint y=r.top;
            uint w=r.right-r.left;
            uint h=r.bottom-r.top;
            
//            printf( __FUNCTION__ " :  %x\t\t%5i%5i%5i%5i\n", hwnd,x,y,w,h );

            uint _y = y + 2;
            for ( RMIter it = rolls.begin(); it != rolls.end(); ++it )
            {
                if ( _y > h ) break;
                it->second->update( hwnd, x+2, _y, w );
                _y += it->second->height();
            }
            //~ MoveWindow(hwnd, x,y,w,h, 1);
            //~ UpdateWindow(hwnd);
			return (hwnd!=0);
        }

        virtual void * getWindow() 
        {
            return hwnd;
        }

        LRESULT CALLBACK message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
        {
            switch ( uMsg  ) 
            {
                case WM_COMMAND:
                {
//                    printf( "cmd : (%x %x)(%x %x)\n", LOWORD(wParam), HIWORD(wParam),LOWORD(lParam), HIWORD(lParam) );
                    for ( RMIter it = rolls.begin(); it != rolls.end(); ++it )
                    {
                        if ( it->second->id == wParam )
                        {
                            HWND chk = it->second->check;
                            uint state = SendMessage( chk, BM_GETCHECK, 0, 0 );
                            it->second->expanded = state;
//                            printf( "cmd %s: %x %i\n", it->first, wParam, state );
                            update();
                            return 1;
                        }
                    }
                    break;
                }
                case WM_CREATE:
                    return on_create( hWnd );
                //case WM_PAINT :
                case WM_SIZE :
                    return on_resize( hWnd );
                case WM_DESTROY:
                    DestroyWindow(hwnd);
                    hwnd=0;
                    break;
            }
            return DefWindowProc( hWnd, uMsg, wParam, lParam );
        }


        uint on_create( HWND hWnd ) 
        {
//            printf( __FUNCTION__ " parent : %x\n", hWnd );
            //~ RECT r;
            //~ GetClientRect( hWnd, &r );
            //~ return update( hWnd, r.left, r.top, r.right, r.bottom );
            return 0;
        }

        uint on_resize( HWND hWnd ) 
        {
            return update();
        }

    };
};



using e6::ClassInfo;

extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"ClientGui.Panel",	 	"ClientGui",	ClientGui::CPanel::createSingleton,  ClientGui::CPanel::classRef	},
		{	0, 0, 0	}//,
		//0
	};

	*ptr = (ClassInfo *)_ci;

	return 1; // classses
}


#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ( "ClientGui 00.00.1 (" __DATE__ ")" );
	mv->e6Version =	e6::e6_version;
	return 1; 
}
