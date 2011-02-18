#include "../e6/e6_sys.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_enums.h"
#include "../e6/e6_prop.h"
#include "../e6/sys/w32/e6_thread.h"
#include "../Core/Core.h"
#include "../Core/CoreString.h"
#include "../TimeLine/TimeLine.h"
#include "../main/w32/WndClass.h"
#include "../main/w32/ComDlg.h"
#include "../main/SceneTree.h"
#include "../main/ScriptWin.h"
#include "../main/resource.h"
#include "../Script/Script.h"
#include "../Physics/Physics.h"
#include "../Gui/ClientGui.h"
#include "../Net/Net.h"

#include <stdio.h>
#include <vector>


#define DOPROF

using w32::CWindow;
using w32::Menu;
using w32::ToolBar;
using w32::StatBar;
using w32::Splitter;
using w32::PropListBox;
using w32::ImageList;
using w32::WidgetStack;

using e6::float3;
using e6::float4;
using e6::float4x4;

//const 
char *files_scene = 
    "All Files     (*.*)\0*.*\0"
    "333 models    (*.333)\0*.333\0"
    "Txt models    (*.txt)\0*.txt\0"
    "3ds models    (*.3ds)\0*.3ds\0"
    "Xml models    (*.xml)\0*.xml\0"
    "Obj models    (*.obj)\0*.obj\0"
    "Oct models    (*.oct)\0*.oct\0"
    "\0" ;


char *files_image = 
    "All Files (*.*)\0*.*\0"
    "PngImage (*.png)\0*.png\0"
    "BmpImage (*.bmp)\0*.bmp\0"
    "TgaImage (*.tga)\0*.tga\0"
    "\0";



FilterTable _filters[] =
{
	{"TimeLine.Sinus",		"TimeLine",	19001},
	{"TimeLine.Value",		"TimeLine",	19002},
	{"TimeLine.Scaler",		"TimeLine",	19003},
	{"TimeLine.Ipo",		"TimeLine",	19004},
	{"TimeLine.Ipo3",		"TimeLine",	19005},
	{"TimeLine.Ipo4",		"TimeLine",	19006},
	{"Audio.BandFilter",	"Audio",	19010},
	{"Midi.Input",			"Midi",		19011},
	{0,0,0}
};


#ifdef DOPROF

struct Profile
{
	const char * name;
	uint t0, t1, ncalls;
	float tp;

	static std::vector<Profile*> profs;
	
	Profile( const char * n ) 
		: name(n), t0(0), t1(0), ncalls(0), tp(0) 
	{
		profs.push_back(this);
	}
	~Profile()
	{
		printf(__FUNCTION__ "\n");
	}
	void start() 
	{
		t0 = e6::sys::getMicroSeconds();
	}
	void stop() 
	{
		t1 = e6::sys::getMicroSeconds();
		int diff = t1-t0;
		if ( diff < 0 )
		{
			diff = 1;
		}
		tp += (diff) * 0.001;
		ncalls ++;
	}
	float getTime() 
	{
		if ( ! ncalls ) return 0;
		float r = tp / (float)ncalls;
		ncalls = 0;
		tp  = 0;
		return r;
	}
};
std::vector<Profile*> Profile::profs;

#endif // DoProf

struct ProfileList 
	: public PropListBox
{
    bool create( HWND hWnd )
    {
		PropListBox::create( hWnd, GetModuleHandle(0), 0, 0,0,0,0, 0,1 );
		MoveWindow( hwnd, 0,0,0,0, 1 );      //??? why
        addRow( "Function"  );
        addRow( "Time" );
        addRow( "Calls"  );

		PropListBox::clear();
    	return 1;
   	}

	~ProfileList()
	{
		printf(__FUNCTION__ "\n");
	}
	void update()
	{
	#ifdef DOPROF
		PropListBox::clear();
		uint n = Profile::profs.size();
		for ( u_int i=0; i<n; i++ ) 
		{
			char b1[50],b2[50];
			Profile * p = Profile::profs[i];
			sprintf( b2, "%i", p->ncalls );
			sprintf( b1, "%2.4f", p->getTime() );
            const char *v[3] = { p->name, b1, b2 };  
			addProp(v);
    	}
	#endif // DoProf
	}
};


//
//  00.    0000. 0.  0.  00.    0000.  00.
// 0. 0.  0.     00. 0. 0. 0.  0.     0. 0.
// 0.  0. 0.000. 0.0.0. 0.  0. 0.000. 0.  0.
// 0.00.  0.     0. 00. 0.  0. 0.     0.00.
// 0.0.   0.     0.  0. 0.  0. 0.     0.0.
// 0. 0.  00000. 0.  0. 0000.  00000. 0. 0.
//

struct Render : public CWindow
{
	
	LRESULT CALLBACK message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
    {
        switch ( uMsg  ) {
        case WM_CREATE:
            on_create( hWnd );
            return 1;
        case WM_SIZE :
            on_resize( hWnd );
            return 1;
        case WM_LBUTTONDOWN :
            SetFocus( hWnd );
            return 1;
        case WM_SETFOCUS :
            SetFocus( hWnd );
		    // ShowCursor(0);
            return 1;
        //case WM_CHAR:
        //    on_char( hWnd , uMsg, wParam, lParam) ;
        //    return 1;
        //case WM_KEYDOWN:
        //    on_key( hWnd , uMsg, wParam, lParam) ;
        //    return 1;
        }
        return DefWindowProc( hWnd, uMsg, wParam, lParam );
    }

    uint setRenderer( Core::Renderer * r ) 
    {
		renderer = r;
		if ( hwnd )
			return on_create( hwnd );
		return 0;
    }

    uint on_create( HWND hWnd ) 
    {
		return on_resize( hWnd );
    }

    uint on_resize( HWND hWnd ) 
    {
		if ( !hwnd ) return 0;
		if ( !renderer ) return 0;
		RECT r;
		GetClientRect(hWnd, &r);
        uint ok = renderer->init( hWnd, r.right, r.bottom );
		if ( ! ok )
		{
			SendMessage( parent, IDM_RENDER_FAILED,0,0 );
		}
		return ok;
    }

  //  void on_char(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
  //  {
  //      //uint c = LOWORD(wParam);
  //      //switch ( c ) 
  //      //{
  //      //}
  //  }

  //  void on_key(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
  //  {
  //      uint c = LOWORD(wParam);
		//switch ( c ) 
  //      {            
  //          case 27:  SendMessage(parent, IDM_FILE_QUIT,0,0); return;
		//	default:  SendMessage(parent, IDM_RENDER_KEY,wParam,0); return;
  //      }
  //  }
	void create( HWND prnt, Core::Renderer * rend )  
	{ 
		renderer = rend;
        parent = prnt;
		hwnd   = CWindow::create( "render","render", 0,0,400,400, WS_TABSTOP|WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|CS_OWNDC, 0, prnt, GetModuleHandle(0), 0, WS_EX_CLIENTEDGE );
	}

    Render()
		: renderer(0)
		, hwnd(0)
    {}

	Core::Renderer * renderer;	
	HWND hwnd;
    HWND parent;
};




/**

  ##.    ##.     #.    ##.    ####.  ##.   #####.  ###.   ####.  ###.
 #. #.  #. #.   #.#.  #. #.  #.     #. #.    #.          #.     #.  #.
 #.  #. #.  #. #.  #. #.  #. #.###. #.  #.   #.     #.   #.###.  #.
 #.##.  #.##.  #.  #. #.##.  #.     #.##.    #.     #.   #.       ##.
 #.     #.#.   #.  #. #.     #.     #.#.     #.     #.   #.         #.
 #.     #. #.   ###.  #.     #####. #. #.    #.    ###.  #####. ####.

**/



//
// use the built-in listbox, just swap key & value and do label-editing on the values.
//
struct Properties : public PropListBox
{
    e6::IStringProp * prop;

	Properties()
		: prop(0)
	{
	}
		
	~Properties()
	{
		$X();
		E_RELEASE(prop);
	}
		
    bool clear()
	{
		//~ $X();
		PropListBox::clear();
		E_RELEASE(prop);
		return 1;
	}
    bool setup( e6::IStringProp * x )
    {
		E_ASSERT(x);

		clear();
    	prop = x;

		uint n = x->numProps();
		for ( u_int i=0; i<n; i++ ) {
            const char *v[3] = { x->getPropValue(i), x->getPropName(i),x->getPropType(i) };  
			addProp(v);
    	}
    	return 1;
   	}


	bool notify(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
	{	
		if ( ! prop )
		{
			//~ printf( __FUNCTION__ " called wo. prop !\n" );
			return 0;
		}
		
    	LV_DISPINFO *pLvdi = (LV_DISPINFO *)lParam;
		switch( pLvdi->hdr.code ) {

        case LVN_BEGINLABELEDIT:
            {
                // Get the handle to the edit box.
                HWND hWndEdit = (HWND)SendMessage(hWnd, LVM_GETEDITCONTROL, 0, 0);
                // Limit the amount of text that can be entered.
                SendMessage(hWndEdit, EM_SETLIMITTEXT, (WPARAM)64, 0);
            }
            break;

        case LVN_ENDLABELEDIT:
            {
				// Save the new label information
				int n = pLvdi->item.iItem;
				if ( n==-1 )
				{
					printf( __FUNCTION__ " called wo. item !\n" );
					return 0;
				}
				if (pLvdi->item.pszText == NULL) 
				{
					printf( __FUNCTION__ " called wo. text !\n" );
					return 0;
				}
				setPropVal(n,0,(char*)pLvdi->item.pszText);
				prop->setPropValue(n, pLvdi->item.pszText);
				SendMessage( hWnd, IDM_VIEW_TREE_UPDATE, 0,0 );
				break;
            }
		case LVN_COLUMNCLICK:
            // don't sort, since the props are ref'd by item-number !
			break;

//    	case LVN_GETDISPINFO:
//			switch (pLvdi->item.iSubItem)
//				case 0:     // address
//					pLvdi->item.pszText = pHouse->szAddress;
//			break;
		default:
			break;
	    }
        return 0L;
	}
};






struct InterfaceList 
	: public PropListBox
	, public e6::InterfaceCallback
{
	uint call( const char * interfaceName, const char * moduleName, uint count )
	{
        const char *v[3] = { interfaceName, moduleName, e6::toString(count) };  
		addProp(v);
		return 1;
	}

    bool setup( e6::Engine * x )
    {
		E_ASSERT(x);

		PropListBox::clear();
		x->findInterfaces( "*", *this );
    	return 1;
   	}

	bool notify(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
	{	
        return 1L;
	}
};











struct Dia : public CWindow , Net::Connection
{
	e6::Engine * engine;
	Core::Renderer * renderer;	
	Core::World * world;	
	Core::Camera * cam;
	Core::Node * cutNode;
	Core::Texture * cutTex;
	Core::Shader * cutShader;
	TimeLine::TimeLine * timeLine;
	Net::TcpServer * netServer;	
	Script::Interpreter * speek;
	Physics::Mover * mover;	
	Core::Node * moving;	
	ClientGui::Panel * panel;	
	char cutPin[100];
	TimeLine::Filter * cutFilter;
	
	Menu menu;
    ToolBar tool;
    StatBar st;
    Splitter split;
	Render render;
    SceneTree tree;
	ScriptWin scriptWin;
	WidgetStack merlin;
    Properties props;
    InterfaceList ilist;
    ProfileList prof;
	
	bool barVisible;
	bool statVisible;
	bool widgetsVisible;

	float fps;

	bool running;
	uint frameNo;
	float timeNow;
	float timeBegin;
	float timeEnd;
	float timeStep;
	uint  timeSleep;
	bool doRealTime;
	bool doRender;
	
	bool doRunScript;
	bool doRecord;
	bool doSaveFrame;
	float4 ambient;

	char defRender[30];
	char curScene[400];
	
	Dia()
		: barVisible(1)
		, statVisible(0)
		, widgetsVisible(1)
		, engine(0)
		, world(0)
		, cam(0)
		, fps(0)
		, running(0)
		, renderer(0)
		, panel(0)
		, frameNo(0)
		, timeNow(0)
		, timeBegin(0)
		, timeEnd(1000)
		, timeStep(0.04f)
		, timeSleep(2)
		, doRender(1)
		, doRealTime(1)
		, doRecord(0)
		, doRunScript(0)
		, doSaveFrame(0)
		, timeLine(0)
		, mover(0)
		, moving(0)
		, cutNode(0)
		, cutTex(0)
		, cutShader(0)
		, cutFilter(0)
		, speek(0)
		, ambient(0.2f,0.2f,0.2f,1)
	{
		strcpy( defRender, "RendererDx9" );
	}

	~Dia()
	{
		$X();
		E_RELEASE(mover);	
		E_RELEASE(moving);	
		E_RELEASE(speek);	
		E_RELEASE(panel);	
		E_RELEASE(timeLine);	
		E_RELEASE(cutNode);	
		E_RELEASE(cutTex);	
		E_RELEASE(cutShader);	
		E_RELEASE(cutFilter);
		E_RELEASE(props.prop);	
		E_RELEASE(world);	
		E_RELEASE(cam);	
		E_RELEASE(renderer);	
		E_RELEASE(engine);	
	}
	
	void init( int argc, char **argv )
	{
		char scriptEngine[100] = "ScriptSquirrel";

		engine = e6::CreateEngine();	
		world  = (Core::World *)engine->createInterface( "Core", "Core.World" );	
		if ( ! world ) exit(1);

		for ( int i=1; i<argc; i++ )
		{
			if ( ( argv[i][0]=='-' ) || ( argv[i][0]=='/' ) )
			{
				if ( argv[i][1]=='r' )
				{
					strcpy( defRender, argv[++i] );
				}
				if ( argv[i][1]=='s' )
				{
					strcpy( scriptEngine, argv[++i] );
				}
			}
			//else
			//{
			//	world->load( engine, argv[i] );
			//}
		}

		renderer  = (Core::Renderer *)engine->createInterface( defRender, "Core.Renderer" );	

		setDefCam();
		insertTexViewer();

		timeLine = (TimeLine::TimeLine *)engine->createInterface( "TimeLine", "TimeLine.TimeLine" );

		//netServer = (Net::TcpServer*)engine->createInterface( "Net", "Net.TcpServer" );
		//netServer->start( 9999, *this );

		createScriptEngine( scriptEngine );

		//panel = (ClientGui::Panel *)engine->createInterface( "ClientGui", "ClientGui.Panel" );	
		
		for ( int i=1; i<argc; i++ )
		{
			if ( ( argv[i][0]=='-' ) || ( argv[i][0]=='/' ) )
			{
				if ( argv[i][1]=='r' )
				{
					++i;
				}
				if ( argv[i][1]=='s' )
				{
					++i;
				}
			}
			else
			{
				load( argv[i] );
			}
		}
	}

    //struct Connection 
	virtual uint handle( Net::Socket & socket ) // !!! const
	{
		if ( ! speek )	return 0;
		char buf[1024];
		socket.read( buf, 1024 );
		
		//E6_CRITICAL
		bool ok = speek->exec(buf, "tcp");
		
		if ( ok )
			socket.write( "ok.", 4);
		else
			socket.write( "error.", 7);
		return ok;
	}

	void setDefCam()
	{
		E_RELEASE( cam ) ;

		cam = (Core::Camera *)engine->createInterface( "Core", "Core.Camera" );
		cam->setName( "DefCam" );

		Core::Node * root = world->getRoot();
		cam->setPos( e6::float3(0,0,40) );
		root->link( cam );
		root->synchronize();
		E_RELEASE( root );

		if ( ! mover )
			mover = (Physics::Mover *)engine->createInterface( "Physics", "Physics.Mover" );
		moving = cam;
		E_ADDREF( moving );	
	}
	
	LRESULT CALLBACK message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
    {
        switch ( uMsg  ) {
        case WM_CREATE:
            on_create(hWnd);
            return 1;
        case WM_SIZE :
            on_resize( hWnd );
            return 1;
        case WM_DROPFILES:
            onDropFile(hWnd, wParam);
            return 1;
        case WM_LBUTTONUP :
            split.endDrag(hWnd,LOWORD(lParam));
            return 1;
        case WM_RBUTTONDOWN :
        case WM_LBUTTONDOWN :
            split.startDrag(hWnd,LOWORD(lParam));
            return 1;
        case WM_MOUSEMOVE :
            split.drag(hWnd,LOWORD(lParam));
            return 1;
        case WM_KEYDOWN:
            on_key( hWnd , uMsg, wParam, lParam) ;
            return 1;
        case WM_COMMAND:
			on_command( hWnd, uMsg, wParam, lParam );
            return 1;
	    case WM_NOTIFY:
    		return on_notify( hWnd, uMsg, wParam, lParam) ;
	  //  case IDM_RENDER_KEY:
			//printf( "$%i\n",LOWORD(wParam));
   // 		move_key( LOWORD(wParam) ) ;
			//return 1;
	    case IDM_RENDER_FAILED:
			//~ printf( "$%i\n",LOWORD(wParam));
    		doRender = 0;
			render.setRenderer(0);
			return 1;
        case WM_DESTROY:
			on_destroy();
            return 0;
        }
        return DefWindowProc( hWnd, uMsg, wParam, lParam );
    }

	
	void on_key(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
	{
		uint c = LOWORD(wParam);
		switch ( c ) 
		{            
			case 27 : 
			{
				on_destroy() ;           
				return ;
			}
			case VK_SPACE:
			{
				running = ! running;
				if ( mover )
				{
					mover->reset();
				}
				return ;
			}
		}
		printf( "!%i\n",c);
//		if ( mover ) move_key(c);
	}

	
	void move_key( UINT c ) 
	{
		if ( ! mover ) return;

        static float step_v = 0.7f;
        static float step_r = e6::Pi / 40;
       
		switch ( c ) 
        {            
            case 35:  mover->accel_x( step_v );  return;
            case 36:  mover->accel_x( -step_v );  return;
            case VK_PRIOR:   mover->accel_y( step_v  );  return;
            case VK_NEXT:    mover->accel_y( -step_v );  return;

			case 40:  mover->accel_z( step_v  );  return;
            case 38:  mover->accel_z( -step_v );  return;

            case 39:  mover->rot_y( -step_r  );   return;
            case 37:  mover->rot_y( step_r );   return;

            case 190:  mover->rot_x( step_r  );   return;
            case 189:  mover->rot_x( -step_r );   return;
			default : printf("$$ %i\n", c );
        }
	}

	void on_destroy() 
    {
		running = 0;
		doRender = 0;
		saveIni( "e6.ini" );		
        PostQuitMessage(0);
	}

	bool on_notify_tool(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
	{	
        //NMTTDISPINFO *pd = (NMTTDISPINFO *)lParam;
        static char *tt[] = {
            "View Scene Tree",
            "Edit Renderer",
            "Edit Application",
            "View Script Window",
            "Start Timer",
            "Stop Timer",
            "Pause Timer",
            "Single Step",
            "View Interfaces",
            "View ClientGui",
            "View Profiling",
            0            
        };
        TOOLTIPTEXT *pd = (TOOLTIPTEXT *)lParam;
    	if ( (int)pd->hdr.code  == TTN_NEEDTEXT ) {
           	switch( pd->hdr.idFrom ) {
           		case IDM_VIEW_SCENE:   pd->lpszText = tt[0];       break;
           		case IDM_VIEW_PROPS_RENDER:   pd->lpszText = tt[1];       break;
           		case IDM_VIEW_PROPS_APP:   pd->lpszText = tt[2];       break;
           		case IDM_VIEW_SCRIPT:  pd->lpszText = tt[3];       break;
           		case IDM_VIEW_INTERFACES:  pd->lpszText = tt[8];       break;
           		case IDM_VIEW_CLIENTGUI:  pd->lpszText = tt[9];       break;
           		case IDM_VIEW_PROFILE:  pd->lpszText = tt[10];       break;
           		case IDM_RUN_PLAY:     pd->lpszText = tt[4];       break;
           		case IDM_RUN_STOP:     pd->lpszText = tt[5];       break;
            //FIXME it's a mystery!
           	//	case IDM_RUN_PAUSE:    pd->lpszText = tt[6];       break;
           		case IDM_RUN_STEP:     pd->lpszText = tt[7];       break;
           	}
		}
		return true;
	}

	bool on_notify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
	{	
		switch( wParam ) 
        {	
			case ID_TREEVIEW :
				return tree.notify( hWnd, uMsg, wParam, lParam) ;
			case ID_PROPS :
				return props.notify( hWnd, uMsg, wParam, lParam) ;
			//case ID_TOOLBAR :
            default:
				return on_notify_tool( hWnd, uMsg, wParam, lParam) ;
		}
        return 0;
    }

    void on_command(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
    {
		// printf( __FUNCTION__ " %x\n", wParam );
		switch( wParam )
		{
			case IDM_FILE_QUIT : 
			{
                on_destroy() ;           
				break;
			}
			case IDM_FILE_OPEN_SCENE : 
			{
				props.clear();
				SetCurrentDirectory( engine->getPath() );
				char *fn = w32::fileBox( 0, "" );
                if ( ! fn || fn[0]==0 )
				{
					break;
				}
                if ( load(fn) )
				{
					updateTree();;   
					setStatus( "ok.", 1 );			
				}
				else
				{
					setStatus( "failed.", 1 );			
				}
				setStatus( fn, 0 );

				break;
			}
			case IDM_FILE_SAVE:
			{
				const char * fn = w32::fileBox(1, "my.e6" );
				if ( fn && fn[0] )
				{
					world->save(engine, fn);
				}
				setStatus( fn, 0 );
				setStatus( "saved ok.", 1 );			
				break;
			}
			case IDM_FILE_NEW_SCENE:
			{
				E_RELEASE( moving );
				//~ moving = 0;
				world->clear();
				timeLine->clear();
				setDefCam();
				insertTexViewer();
                updateTree();       
				//if ( timeLine )
				//{
				//	timeLine->clear();
				//}
				if ( speek )
				{
					speek->exec( "cleanup();", "new scene" );
				}
				break;
			}
			case IDM_FILE_RELOAD_SCENE:
			{
				E_RELEASE( moving );
				world->clear();
				timeLine->clear();
				setDefCam();
				insertTexViewer();
                updateTree();       
				if ( speek )
				{
					speek->exec( "cleanup();", "new scene" );
				}
				props.clear();
	            if ( load(curScene) )
				{
					updateTree();;   
					setStatus( "ok.", 1 );			
				}
				else
				{
					setStatus( "failed.", 1 );			
				}
				setStatus( curScene, 0 );

				break;
			}
			case IDM_FILE_RENDERER : 
			{
				props.clear();
                loadRenderer(0);           
				break;
			}
			case IDM_FILE_SCRIPTENGINE : 
			{
				props.clear();
                loadScriptEngine(0);           
				break;
			}
			case IDM_RUN_PLAY : 
			{
				running = true;
				break;
			}
			case IDM_RUN_PAUSE : 
			{
				running = false;
				if ( mover )
				{
					mover->reset();
				}
				break;
			}
			case IDM_RUN_STOP : 
			{
				timeNow = 0;
				frameNo = 0;
				running = false;
				if ( mover )
				{
					mover->reset();
				}
				timeUpdate();
				break;
			}
			case IDM_RUN_STEP : 
			{
				timeNow += timeStep;
				frameNo ++;
				timeUpdate();
				on_idle();
				break;
			}
			case IDM_SCRIPT_LOAD : 
			{
				if ( ! speek ) break;
				SetCurrentDirectory( engine->getPath() );
				const char * s = w32::fileBox( 0, "res\\e6.ini" );
				loadScript(s);
				break;
			}
			case IDM_SCRIPT_SAVE : 
			{
				char b[8024];
				GetWindowText( scriptWin.code, b, 8024 );
				char * fn = w32::fileBox( 1 );
				if ( fn )
				{
					FILE * f = fopen(fn,"wb");
					if ( f )
					{
						fwrite( b, strlen(b), 1, f );
						fclose(f);
					}
				}
				break;
			}
			case IDM_SCRIPT_RUN : 
			{
				if ( ! speek ) break;
				char b[8024];
				GetWindowText( scriptWin.code, b, 8024 );
				printf( "script.exec(%s)\n",b);
				doRunScript |= speek->exec(b, "konsole");
				break;
			}
			case IDM_SCRIPT_CLEAR : 
			{
				SetWindowText( scriptWin.out, "" );
				break;
			}
			case IDM_SCRIPT_VERSION:
			{
				if ( ! speek ) break;
				SetWindowText( scriptWin.out, speek->getName() );
				break;
			}

			case IDM_VIEW_IMAGE:
			{
				TVITEM it = tree.getClickedItem();
				char * name = it.pszText;
				printf( "VIEW %s\n", name );
				this->viewTex( name, 1 );
				break;
			}
			case IDM_VIEW_TOOLBAR : 
			{
                barVisible = ! (GetMenuState(GetMenu(hWnd),IDM_VIEW_TOOLBAR,0) & MF_CHECKED ) ;
                ShowWindow(tool.hwnd,barVisible?SW_SHOW:SW_HIDE); 
                CheckMenuItem( GetMenu(hWnd), IDM_VIEW_TOOLBAR, ((barVisible)<<3)); 
                SendMessage( hWnd, WM_SIZE, 0,0 );
				break;
			}
            case IDM_VIEW_STATBAR : 
            {
                statVisible = ! (GetMenuState(GetMenu(hWnd),IDM_VIEW_STATBAR,0) & MF_CHECKED ) ;
                ShowWindow(st.hwnd,statVisible?SW_SHOW:SW_HIDE); 
                CheckMenuItem( GetMenu(hWnd), IDM_VIEW_STATBAR, (statVisible<<3)); 
                SendMessage( hWnd, WM_SIZE, 0,0 );
                break;
            }
            case IDM_VIEW_WIDGETS  : 
            {
                widgetsVisible = ! (GetMenuState(GetMenu(hWnd),IDM_VIEW_WIDGETS,0) & MF_CHECKED ) ;
                ShowWindow(merlin.front(),widgetsVisible?SW_SHOW:SW_HIDE); 
                CheckMenuItem( GetMenu(hWnd), IDM_VIEW_WIDGETS, (widgetsVisible<<3)); 
                SendMessage( hWnd, WM_SIZE, 0,0 );
                break;
            }
			case IDM_VIEW_SCENE:
			case IDM_VIEW_TREE_UPDATE :
			{
				if ( merlin.frontID() == 0 )
					updateTree();
				merlin.raise(0);
                SendMessage( hWnd, WM_SIZE, 0,0 );
				break;
			}
			case IDM_VIEW_SCRIPT:
			{
				props.clear();
				merlin.raise(1);
                SendMessage( hWnd, WM_SIZE, 0,0 );
				break;
			}
			case IDM_VIEW_CLIENTGUI:
			{
				props.clear();
				RECT r;
				GetClientRect( hWnd, &r );
				if ( panel ) 
				{
					panel->update();
					merlin.raise(4);
					SendMessage( hWnd, WM_SIZE, 0,0 );
				}
				break;
			}

			case IDM_FILE_ALL_MODULES :
			{
                engine->loadAllModules();           
				props.clear();
				ilist.setup( engine );
				merlin.raise(3);
                SendMessage( hWnd, WM_SIZE, 0,0 );
				break;
			}
			case IDM_FILE_CLEAR_MODULES :
			{
                engine->cleanupModules();           
				props.clear();
				ilist.setup( engine );
				merlin.raise(3);
                SendMessage( hWnd, WM_SIZE, 0,0 );
				break;
			}
			case IDM_VIEW_INTERFACES:
			{
				props.clear();
				ilist.setup( engine );
				merlin.raise(3);
                SendMessage( hWnd, WM_SIZE, 0,0 );
				break;
			}
			case IDM_VIEW_PROFILE:
			{
				props.clear();
				prof.update();
				merlin.raise( this->panel ? 5 : 4);
                SendMessage( hWnd, WM_SIZE, 0,0 );
				break;
			}
			case IDM_VIEW_PROPS_RENDER:
			case IDPOP_RENDERER_EDIT:
			{
				if ( ! renderer ) break;
				Core::RenderString* str = (Core::RenderString*)engine->createInterface("Core","Core.RenderString");
				if ( str->init( renderer ) )
				{
					props.setup( str );
					merlin.raise(2);
					SendMessage( hWnd, WM_SIZE, 0,0 );
				}
				break;
			}
			case IDM_VIEW_PROPS_APP:
			{
				props.setup( new AppString(this) );
				merlin.raise(2);
                SendMessage( hWnd, WM_SIZE, 0,0 );
				break;
			}
			case IDPOP_EDIT:
			{
        	    TVITEM it = tree.popItem;
				Core::Node * node = (Core::Node*)it.lParam;
				switch (it.iImage )
				{
					default:
					case EICO_FREENODE:
					{
						Core::NodeString* str = (Core::NodeString*)engine->createInterface("Core","Core.NodeString");
						if ( str->init( node ) )
							props.setup( str );
						break;
					}
					case EICO_CAMERA:
					{
						Core::CameraString* str = (Core::CameraString*)engine->createInterface("Core","Core.CameraString");
						if ( str->init( (Core::Camera*)node ) )
							props.setup( str );
						break;
					}
					case EICO_LIGHT:
					{
						Core::LightString* str = (Core::LightString*)engine->createInterface("Core","Core.LightString");
						if ( str->init( (Core::Light*)node ) )
							props.setup( str );
						break;
					}
					case EICO_MESH:
					{
						Core::MeshString* str = (Core::MeshString*)engine->createInterface("Core","Core.MeshString");
						if ( str->init( (Core::Mesh*)node ) )
							props.setup( str );
						break;
					}			
				}
				merlin.raise(2);
                SendMessage( hWnd, WM_SIZE, 0,0 );
				break;
			}
			case IDPOP_EDIT_VS:
			{
        	    TVITEM it = tree.popItem;
				Core::Node * node = (Core::Node*)it.lParam;
				Core::MeshVSString* str = (Core::MeshVSString*)engine->createInterface("Core","Core.MeshVSString");
				if ( str->init( (Core::Mesh*)node ) )
				{
					props.setup( str );
					merlin.raise(2);
					SendMessage( hWnd, WM_SIZE, 0,0 );
				}
				break;
			}
			case IDPOP_EDIT_PS:
			{
        	    TVITEM it = tree.popItem;
				Core::Node * node = (Core::Node*)it.lParam;
				Core::MeshPSString* str = (Core::MeshPSString*)engine->createInterface("Core","Core.MeshPSString");
				if ( str->init( (Core::Mesh*)node ) )
				{
					props.setup( str );
					merlin.raise(2);
					SendMessage( hWnd, WM_SIZE, 0,0 );
				}
				break;
			}
			case IDPOP_EDIT_RS:
			{
        	    TVITEM it = tree.popItem;
				Core::Node * node = (Core::Node*)it.lParam;
				Core::MeshRSString* str = (Core::MeshRSString*)engine->createInterface("Core","Core.MeshRSString");
				if ( str->init( (Core::Mesh*)node ) )
				{
					props.setup( str );
					merlin.raise(2);
					SendMessage( hWnd, WM_SIZE, 0,0 );
				}
				break;
			}
			case IDPOP_MOVE_NODE:
			{
        	    TVITEM it = tree.popItem;
				printf( "mov : %i %s\n", it.lParam, it.pszText );
				E_RELEASE( moving );
				moving = (Core::Node*)it.lParam;
				E_ASSERT( moving );
				E_ADDREF( moving );
				break;
			}
			case IDPOP_REMOVE:
			{
        	    TVITEM it = tree.popItem;
				printf( "rem : %i %s\n", it.lParam, it.pszText );
				Core::Node * node = (Core::Node*)it.lParam;
				E_ASSERT(node);
				Core::Node * prnt = node->getParent();
				if ( prnt )
				{
					prnt->unlink(node);
					// E_RELEASE( node ); ///PPP???
					//tree.clearNode(prnt);
					updateTree();
					E_RELEASE( prnt );
				}
				else
				{
					e6::sys::alert( "Don't do This !", "You are trying to remove the root node." );
				}
				break;
			}
			case IDPOP_CUT:
			{
        	    TVITEM it = tree.popItem;
				printf( "cut : %i %s\n", it.lParam, it.pszText );
				E_RELEASE( cutNode );
				cutNode = (Core::Node*)it.lParam;
				E_ASSERT( cutNode );
				E_ADDREF( cutNode );
				Core::Node * prnt = cutNode->getParent();
				if ( prnt )
				{
					prnt->unlink(cutNode);
					E_RELEASE( prnt );
				}
				tree.mask |= CCM_NODE; 
				//updateTree();
				break;
			}
			case IDPOP_PASTE:
			{
				if ( ! cutNode ) return;
        	    TVITEM it = tree.popItem;
				printf( "paste : %i %s\n", it.lParam, it.pszText );
				Core::Node * pNode = (Core::Node*)it.lParam;
				E_ASSERT( pNode );
				pNode->link(cutNode);
				E_RELEASE( cutNode );
				tree.mask &= ~CCM_NODE; 
				//updateTree();
				break;
			}
			case IDPOP_CLONE:
			{
        	    TVITEM it = tree.popItem;
				Core::Node * node = (Core::Node*)it.lParam;
				E_ASSERT( node );
				Core::Node * newItem = node->copy();
				E_RELEASE( newItem );
				//updateTree();
				break;
			}
			case IDPOP_COPY_TEX:
			{
        	    TVITEM it = tree.popItem;
				E_RELEASE( cutTex );
				cutTex = (Core::Texture*)it.lParam;
				E_ADDREF( cutTex );
				tree.mask |= CCM_TEX; 
				break;
			}
			case IDPOP_COPY_SHADER:
			{
        	    TVITEM it = tree.popItem;
				E_RELEASE( cutShader );
				cutShader = (Core::Shader*)it.lParam;
				E_ADDREF( cutShader );
				tree.mask |= CCM_SHA; 
				break;
			}
			case IDPOP_PASTE_TEX:
			{
				if ( ! cutTex ) break;
        	    TVITEM it = tree.popItem;
				Core::Mesh * m = (Core::Mesh *)it.lParam;
				m->setTexture( 0, cutTex );
				tree.mask &= ~CCM_TEX; 
				//updateTree();
				break;
			}
			case IDPOP_PASTE_VSHADER:
			{
				if ( ! cutShader ) break;
        	    TVITEM it = tree.popItem;
				Core::Mesh * m = (Core::Mesh *)it.lParam;
				m->setVertexShader( cutShader );
				tree.mask &= ~CCM_SHA; 
				//updateTree();
				break;
			}
			case IDPOP_PASTE_PSHADER:
			{
				if ( ! cutShader ) break;
        	    TVITEM it = tree.popItem;
				Core::Mesh * m = (Core::Mesh *)it.lParam;
				m->setPixelShader( cutShader );
				tree.mask &= ~CCM_SHA; 
				//updateTree();
				break;
			}
			case IDPOP_CREATE_NODE:
			{
				create_node("Core.FreeNode");
				break;
			}
			case IDPOP_CREATE_LIGHT:
			{
				create_node("Core.Light");
				break;
			}
			case IDPOP_CREATE_CAMERA:
			{
				create_node("Core.Camera");
				break;
			}
			case IDPOP_CREATE_RENDERTARGET:
			{
				Core::RenderToTexture * rt = (Core::RenderToTexture *)engine->createInterface( "Core", "Core.RenderToTexture" );
				if ( rt )
				{
					Core::Texture * t = (Core::Texture *) engine->createInterface( "Core", "Core.Texture" );
					if ( ! t )
					{
						E_RELEASE(rt);
						return;
					}
					t->alloc( e6::PF_X8B8G8R8, 512, 512, 1, e6::TU_RENDERTARGET );
					rt->setRenderTarget(t);
					world->textures().add( t->getName(), t );
					E_RELEASE(t);

					TVITEM it = tree.popItem;
					Core::Node * node = (Core::Node*)it.lParam;
					E_ASSERT( node );
					node->link( rt );

					E_RELEASE( rt );
					updateTree();
				}
				break;
			}
			case IDPOP_SELECT_CAM:
			{
				TVITEM it = tree.popItem;
				Core::Camera* c = (Core::Camera*)it.lParam;
				if(c)
				{
					E_ADDREF(c);
					E_RELEASE(cam);
					cam = c;
				}
				break;
			}
			case IDPOP_CREATE_FILTER:
			{
				TVITEM it = tree.popItem;
				FilterTable* f = (FilterTable*)it.lParam;			
				if ( f )
				{
					TimeLine::Filter* filter = (TimeLine::Filter*)engine->createInterface( f->module, f->name );
					if ( filter )
					{
						uint r = timeLine->add( filter );
						E_RELEASE( filter );			
						//if ( r ) updateTree();
					}
				}
				break;
			}
			case IDPOP_REMOVE_FILTER:
			{
				TVITEM it = tree.popItem;
				TimeLine::Filter* filter = (TimeLine::Filter*)it.lParam;
				if ( filter )
				{
					uint r = timeLine->remove( filter->getName() );
					//if ( r ) updateTree();
				}
				break;
			}
			case IDPOP_PIN_DISCONNECT:
			{
				TVITEM it = tree.popItem;
				TimeLine::Filter* filter = (TimeLine::Filter*)it.lParam;
				if ( filter )
				{
					uint r = timeLine->disconnect( filter->getName(), it.pszText );
					//if ( r ) updateTree();
				}
				break;
			}
			case IDPOP_PIN_COPY:
			{
				TVITEM it = tree.popItem;
				printf( "pcopy : %i %s\n", it.lParam, it.pszText );
				strcpy(cutPin,it.pszText);
				E_RELEASE( cutFilter );
				cutFilter = (TimeLine::Filter*)it.lParam;
				E_ADDREF( cutFilter );
				printf( "Filter %s.%s \n", cutFilter->getName(), cutPin ) ;
				break;
			}
			case IDPOP_PIN_PASTE:
			{
				if ( ! cutFilter ) break;
				TVITEM it = tree.popItem;
				printf( "ppaste : %i %s\n", it.lParam, it.pszText );
				TimeLine::Filter* filter = (TimeLine::Filter*)it.lParam;
				if ( ! filter ) break;
				if ( cutFilter == filter )
				{
					e6::sys::alert( "Dont do this!", "You are trying to shortcut this filter !" );
					break;
				}
				printf( "Connecting %s.%s to %s.%s\n", filter->getName(), it.pszText, cutFilter->getName(), cutPin ) ;
				uint r = timeLine->connect( filter->getName(), it.pszText, cutFilter->getName(), cutPin ) ;
				E_RELEASE( cutFilter );
				//if ( r ) updateTree();
				break;
			}
		}
	}

	void create_node( const char * str )
	{
		TVITEM it = tree.popItem;
		Core::Node * node = (Core::Node*)it.lParam;
		Core::Node * newItem = (Core::Node *)engine->createInterface( "Core", str );
		if ( newItem )
		{
			node->link( newItem );
			E_RELEASE( newItem );
			updateTree();
		}
	}
	
    void on_create( HWND hWnd ) 
	{       
        DragAcceptFiles(hWnd, TRUE); 

		HINSTANCE hinst = GetModuleHandle(0);
		char ico[200];
		sprintf( ico, "%s\\bin\\ui\\e6.ico", engine->getPath() );
		uint err = SetClassLong( hWnd, GCL_HICON, (LONG)LoadIcon( hinst,ico )  ); 

		menu.insertItem( IDM_MENU_FILE, "&File" );
		menu.insertSubItem( IDM_MENU_FILE, IDM_FILE_OPEN_SCENE, "&Open Scene" );
		menu.insertSubItem( IDM_MENU_FILE, IDM_FILE_SAVE, "&Save Scene" );
		menu.insertSubItem( IDM_MENU_FILE, IDM_FILE_NEW_SCENE, "&New Scene" );
		menu.insertSubItem( IDM_MENU_FILE, IDM_FILE_RELOAD_SCENE, "Reloa&d Scene" );
		menu.insertSubItem( IDM_MENU_FILE, IDM_FILE_RENDERER, "Change &Renderer" );
		menu.insertSubItem( IDM_MENU_FILE, IDM_FILE_SCRIPTENGINE, "Change Script&Engine" );
		menu.insertSubItem( IDM_MENU_FILE, IDM_FILE_ALL_MODULES, "Load &All Modules" );
		menu.insertSubItem( IDM_MENU_FILE, IDM_FILE_CLEAR_MODULES, "&Clear Unused Modules" );
		menu.insertSubItem( IDM_MENU_FILE, IDM_FILE_QUIT, "&Quit" );
		menu.insertItem( IDM_MENU_VIEW, "&View" );
		menu.insertSubItem( IDM_MENU_VIEW, IDM_VIEW_WIDGETS, "&Widgets" );
		menu.insertSubItem( IDM_MENU_VIEW, IDM_VIEW_TOOLBAR, "&Toolbar" );
		menu.insertSubItem( IDM_MENU_VIEW, IDM_VIEW_STATBAR, "&Statusbar" );
		menu.insertItem( IDM_MENU_WIDGETS, "&Widgets" );
		menu.insertSubItem( IDM_MENU_WIDGETS, IDM_VIEW_SCENE, "&Tree" );
		menu.insertSubItem( IDM_MENU_WIDGETS, IDM_VIEW_PROPS_RENDER, "&Renderer" );
		menu.insertSubItem( IDM_MENU_WIDGETS, IDM_VIEW_PROPS_APP, "&Application" );
		menu.insertSubItem( IDM_MENU_WIDGETS, IDM_VIEW_SCRIPT, "&Script" );
		menu.insertSubItem( IDM_MENU_WIDGETS, IDM_VIEW_INTERFACES, "&Interfaces" );
		menu.insertSubItem( IDM_MENU_WIDGETS, IDM_VIEW_CLIENTGUI, "&Client Gui" );
		menu.insertSubItem( IDM_MENU_WIDGETS, IDM_VIEW_PROFILE, "&Profiling" );

		menu.insertItem( IDM_MENU_RUN, "&Run" );
		menu.insertSubItem( IDM_MENU_RUN, IDM_RUN_PLAY, "&Play" );
		menu.insertSubItem( IDM_MENU_RUN, IDM_RUN_STOP, "&Stop" );
		menu.insertSubItem( IDM_MENU_RUN, IDM_RUN_PAUSE, "P&ause" );
		menu.insertSubItem( IDM_MENU_RUN, IDM_RUN_STEP, "St&ep" );
		menu.attach( hWnd );

		char tb_ico[200];
		sprintf(tb_ico, "%s\\bin\\ui\\tool0.bmp", engine->getPath() );
		tool.create( hWnd, ID_TOOLBAR, tb_ico, 16,16 );
        tool.add( 0, IDM_VIEW_SCENE, 9,   TBSTYLE_BUTTON );
        tool.add( 0, IDM_VIEW_SCRIPT, 10, TBSTYLE_BUTTON );
        tool.add( 0, IDM_VIEW_PROPS_RENDER, 11,  TBSTYLE_BUTTON );
        tool.add( 0, IDM_VIEW_PROPS_APP, 12,  TBSTYLE_BUTTON );
        tool.add( 0, IDM_VIEW_INTERFACES, 15,  TBSTYLE_BUTTON );
        tool.add( 0, IDM_VIEW_CLIENTGUI, 14,  TBSTYLE_BUTTON );
        tool.add( 0, IDM_VIEW_PROFILE, 13,  TBSTYLE_BUTTON );
        tool.add( 0, 0, 0, 0 );
        tool.add( 0, IDM_RUN_PLAY, 0, TBSTYLE_BUTTON );
        tool.add( 0, IDM_RUN_STOP, 1, TBSTYLE_BUTTON );
        tool.add( 0, IDM_RUN_PAUSE, 2, TBSTYLE_BUTTON );
        tool.add( 0, IDM_RUN_STEP, 11, TBSTYLE_BUTTON );

        CheckMenuItem( GetMenu(hWnd), IDM_VIEW_TOOLBAR, (barVisible<<3)); 
		ShowWindow(tool.hwnd,barVisible?SW_SHOW:SW_HIDE); 
        if ( barVisible )
            tool.update(0,0);
		
        // statusbar righttoleft
        int ap[] = {270,350,420};
        st.create( hWnd, ID_STATBAR, 3, ap );
        CheckMenuItem( GetMenu(hWnd), IDM_VIEW_STATBAR, (statVisible<<3)); 
        ShowWindow(st.hwnd,statVisible?SW_SHOW:SW_HIDE); 
        if ( statVisible )
            st.updateSize(true);

		char scr_ico[200];
		sprintf(scr_ico, "%s\\bin\\ui\\tool0.bmp", engine->getPath() );	
		scriptWin.create( hWnd, scr_ico );
		MoveWindow( scriptWin.hwnd, 0,0,0,0, 1 );     //??? why 

        // props -> widgetstack
		props.create( hWnd, hinst, (HMENU)ID_PROPS, 0,0,0,0, 1,0 );
		MoveWindow( props.hwnd, 0,0,0,0, 1 );      //??? why
        props.addRow( "Value"  );
        props.addRow( "Name" );
        props.addRow( "Type"  );

		ilist.create( hWnd, hinst, (HMENU)ID_INTERFACES, 0,0,0,0, 0,1 );
		MoveWindow( ilist.hwnd, 0,0,0,0, 1 );      //??? why
        ilist.addRow( "Interface"  );
        ilist.addRow( "Module" );
        ilist.addRow( "Count"  );
		ilist.setup( engine );

		prof.create( hWnd );
		
		tree.create( hWnd, hinst, (HMENU)ID_TREEVIEW, 0,0,0,1 );
        ImageList img;
        img.create( 16,16 );
		char tr_ico[200];
		sprintf(tr_ico, "%s\\bin\\ui\\tree0.bmp", engine->getPath() );
		img.add( tr_ico );
        tree.setImageList( img.strip );


        render.create( hWnd, renderer );
//		engine->findInterfaces( "TimeLine.Filter", &tree );

		//~ printf("starting panel ...\n");
		if ( panel )
		{
			panel->init( hWnd );
		}
		//~ panel->addRollout( "Foo" );
		//~ panel->addRollout( "Baz" );
		//~ panel->addRollout( "Baaar" );
		//~ panel->addControl( "Foo","Edit", "name" );
		//~ panel->addControl( "Foo","Edit", "value" );
		//~ panel->addControl( "Foo","Edit", "dawn" );
		//~ panel->addControl( "Baz", "Slider", "delay" );
		//~ panel->addControl( "Baaar", "Slider", "delay" );
		//~ panel->addControl( "Baaar", "Edit", "tool" );
		//~ panel->expand("Foo", 1);
		//~ panel->expand("Baz", 1);
		//~ panel->expand("Baaar", 1);
		//		panel->update( hWnd, 20,20,80,300 );
		//~ printf("panel ...\n");
		
		
        CheckMenuItem( GetMenu(hWnd), IDM_VIEW_WIDGETS, (widgetsVisible<<3)); 
		merlin.add( tree.hwnd  );
		merlin.add( scriptWin.hwnd  );
		merlin.add( props.hwnd  );
		merlin.add( ilist.hwnd  );
		if ( panel )
		{
			merlin.add( (HWND)(panel->getWindow()) );
		}
		merlin.add( prof.hwnd  );
		merlin.raise(0);
		split.pos = 169;

		timeUpdate();
		updateTree();
    }

    void on_resize( HWND hWnd ) 
	{
        RECT rc;
        RECT rts = {0};
        RECT rtb = {0};
        GetClientRect(hWnd, &rc);
        if ( barVisible ) 
        {
            GetWindowRect(tool.hwnd, &rtb);
            tool.update(0,0);
        }
        if ( statVisible ) 
        {
            GetWindowRect(st.hwnd, &rts);
            st.updateSize(true);
        }
		int sh = rts.bottom - rts.top;
		int th = rtb.bottom - rtb.top;
		int h  = rc.bottom  - rc.top - th - sh;
		int w  = rc.right   - rc.left;
		if ( widgetsVisible )
		{
			int x  = split.pos;
			split.setY(th,0);
			MoveWindow( merlin.front(), 0, th, x-2, h, 1 );
			MoveWindow( render.hwnd, x+2, th, w-x-2, h, 1 );      
		}
		else
		{
			MoveWindow( render.hwnd, 0, th, w, h, 1 );      
		}
		//HINSTANCE hinst = GetModuleHandle(0);
		//uint err = SetClassLong( hWnd, GCL_HICON, (LONG)LoadIcon( hinst,"..\\bin\\ui\\e6.ico" )  ); 
    }

    void onDropFile( HWND hWnd, WPARAM wParam ) 
    {
        char  f[300];
        HDROP drop = (HDROP)wParam;      
        DragQueryFile(drop, 0, f, sizeof(f)); 
		bool ok = load(f);
		if ( ok )
		{
			updateTree();
		}
		else
		{
			st.setPart( (ok?"ok.":"failed."), 1 );
		}
		st.setPart( f, 0 );
        DragFinish(drop);
    }

	virtual void timeUpdate()
	{
		char b[64];
		sprintf(b, "%2.1f %2.2f", fps, timeNow );
		setStatus( b, 2 );
		setStatus( e6::toString(this->cam->getPos()), 1 );
	}

	void pollKeys()
	{
		if ( ! mover ) return;
		if ( render.hwnd != GetFocus() ) return;
		if ( GetAsyncKeyState( 35 ) ) move_key(35);
		if ( GetAsyncKeyState( 36 ) ) move_key(36);
		if ( GetAsyncKeyState( VK_PRIOR ) ) move_key(VK_PRIOR);
		if ( GetAsyncKeyState( VK_NEXT  ) ) move_key(VK_NEXT);
		if ( GetAsyncKeyState( 40 ) ) move_key(40);
		if ( GetAsyncKeyState( 38 ) ) move_key(38);
		if ( GetAsyncKeyState( 39 ) ) move_key(39);
		if ( GetAsyncKeyState( 37 ) ) move_key(37);
		if ( GetAsyncKeyState( 190 ) ) move_key(190);
		if ( GetAsyncKeyState( 189 ) ) move_key(189);
	}


	virtual uint on_idle()
	{
		if ( running )
		{
			pollKeys();

			float dt = timeStep;
			if ( doRealTime )
			{
				static uint t0=0;
				uint t = e6::sys::getMicroSeconds();
				uint diff = t - t0;
				if ( diff < 0 )
				{
					dt = 0.001f;
				}
				else
				if ( diff < 2000 ) 
				{
					dt = 0.001f * float(diff); 
				}
				t0 = t;
			}
			timeNow += dt;
			frameNo ++;

			static uint tshow=0;
			if ( ++tshow > 30 )
			{
				static float t0=0;			
				fps = 1.0f / ((timeNow - t0) / 30.0f);
				t0 = timeNow;
				tshow = 0;
				timeUpdate();
			}
		
			if ( mover && moving )
			{
				if ( moving == cam )
					mover->moveLocal( moving, dt );
				else
					mover->move( moving, dt );
			}
			
			if ( timeLine )
			{
				#ifdef DOPROF
				static Profile p("Timeline::update");
				p.start();
				#endif
				timeLine->update( timeNow );
				#ifdef DOPROF
				p.stop();
				#endif
			}

			if ( doRunScript && speek )
			{
			#ifdef DOPROF
				static Profile p("Script::exec");
				p.start();
			#endif
				char code[60];
				sprintf( code, "run(%2.3f);", timeNow );
				doRunScript = speek->exec( code, "e6" );
			#ifdef DOPROF
				p.stop();
			#endif
			}
		}

		if ( doRender && renderer )
		{
			#ifdef DOPROF
			static Profile p("Renderer::render");
			p.start();
			#endif
			renderScene();
			#ifdef DOPROF
			p.stop();
			#endif
		}
		if ( running && doSaveFrame && renderer )
		{
			#ifdef DOPROF
			static Profile p("Renderer::save");
			p.start();
			#endif
			if ( (this->timeNow >= this->timeBegin) && (this->timeNow <= this->timeEnd) )
			{
				char fn[200];
				sprintf( fn, "%s/res/frame/frame_%04i.bmp", engine->getPath(), this->frameNo );
				uint ok = renderer->captureFrontBuffer( fn );
				printf( "capture : '%s' %i\n", fn, ok );
			}
			#ifdef DOPROF
			p.stop();
			#endif
		}

		if ( timeSleep )
		{
			Sleep( timeSleep );
		}

		return 1;
	}

	void updateTree()
	{
		props.clear();
		tree.update( world );
		tree.update( timeLine );
		if ( renderer )
			tree.update( renderer );
	}

	void setStatus( const char* s, uint i=0 )
	{
		st.setPart( (char*)s,i );
	}

	bool viewTex( const char * name, bool visible )
	{
		//Core::Node * root = this->world->getRoot();
		//Core::Node * tv = root->findRecursive( "lissi" );
		//if ( tv )
		//{
		//	if ( ! visible )
		//	{
		//		tv->setVisibility( 0 );
		//	}
		//	else
		//	{
		//		tv->setVisibility( 1 );
		//		Core::Node * node = root->findRecursive( "lissi" );
		//		if ( node )
		//		{
		//			Core::Mesh * mesh = (Core::Mesh *)node->cast("Core.Mesh");
		//			mesh->setTexture( 0, world->textures().get(name) );
		//		}
		//		E_RELEASE( node );
		//	}
		//}

		//E_RELEASE( tv );
		//E_RELEASE( root );
		return 1;
	}

	bool insertTexViewer()
	{
		//Core::Node * root = this->world->getRoot();


		//Core::FreeNode * tv = (Core::FreeNode *)engine->createInterface("Core","Core.FreeNode");
		//tv->setName("TexViewer");
		//tv->setVisibility(0);

		//root->link( tv );

		//this->world->load( this->engine, "plate.e6", tv );
		//Core::Node * lissi = root->findRecursive( "lissi" );
		//root->unlink( lissi );
		//tv->link( lissi );
		//E_RELEASE( lissi );
		//

		//E_RELEASE( tv );
		//E_RELEASE( root );
		return true;
	}


	bool loadScript( const char* s )
	{
		if ( ! s ) return false;
		if ( ! speek ) return false;
		SetCurrentDirectory( engine->getPath() );
		const char * buf = e6::sys::loadFile( s, false );
	//	SetWindowText( scriptWin.out, buf );
		bool ok = speek->exec(buf, s);
		doRunScript |= ok;
		return ok;
	}

	bool load( char* s )
	{
		//if ( ! s )
		//{
		//	SetCurrentDirectory( engine->getResPath() );
		//	s = w32::fileBox( 0, "" );
		//}
		if ( ! s || !s[0] )
			return false;

		strcpy( curScene, s );
		if ( speek )
		{
			char path[200], ext[10];
			engine->chopExt( s, path, ext );
			st.setPart( s, 0 );
			if ( ! strcmp( ext, speek->getFileExtension() ) )
			{
				return loadScript(s);
			} 
		}
		
		return world->load( engine, s );
	}

	bool loadScriptEngine( const char * s )
	{
		char name[200], path[200], ext[10];
		if ( ! s )
		{
			s = w32::fileBox( 0, "..\\bin\\Script", "Script    (Script*.dll)\0Script*.dll\0" );
		}
		if ( ! s || !s[0] )
			return 0;

		engine->chopPath( s, path, name );
		engine->chopExt( name, path, ext );
		return createScriptEngine( path );
	}

	
	bool createScriptEngine( const char * name )
	{
//		char binPath[300];
		char * message = "Loaded";
		bool ok = true;
		//sprintf( binPath, "%s%cbin", engine->getPath(), e6::sys::fileSeparator() ); 
		//e6::sys::setCurrentDir( binPath );
		////SetCurrentDirectory( engine->getStartPath() );
		E_RELEASE( speek );

		speek  = (Script::Interpreter *)engine->createInterface( name, "Script.Interpreter" );	
		if ( ! speek )
		{
			message = "Could not load interpreter ";
			ok = false;
		}

		if ( ok )
		{
			ok = speek->setup( engine );
			if ( ok  )
			{
				speek->setErrlog( scriptWin );
				speek->setOutlog( scriptWin );
			}
			else
			{
				message= "Could not start interpreter ";
				E_RELEASE(speek);	
			}
		}



		setStatus( message, 0 );
		setStatus( name, 1 );
		return ok;
	}

	void loadRenderer( char* s )
	{
		char name[200], path[200], ext[10];
		//SetCurrentDirectory( engine->getPath() );
		if ( ! s )
		{
			s = w32::fileBox( 0, "bin\\Renderer", "Renderer    (Renderer*.dll)\0Renderer*.dll\0" );
			SetCurrentDirectory( engine->getPath() );
		}
		if ( ! s || !s[0] )
			return;

		engine->chopPath( s, path, name );
		engine->chopExt( name, path, ext );
		Core::Renderer * r2d2  = (Core::Renderer *)engine->createInterface( path, "Core.Renderer" );	
		if ( r2d2 )
		{		
			if ( render.setRenderer( r2d2 ) )
			{		
				E_RELEASE( renderer );
				renderer = r2d2;
				doRender = 1;
				st.setPart( s, 0 );
				st.setPart( "ok.", 1 );
				renderScene();
				updateTree();
			}
			else
			{
				render.setRenderer( 0 );
				E_RELEASE( r2d2 );
				st.setPart( s,0 );
				st.setPart( "load failed !",1 );
			}
		}
	}

	void saveIni( const char * path )
	{
		SetCurrentDirectory( engine->getPath() );
		FILE * f = fopen( path, "wb" );
		if ( f )
		{
			saveProp( f, "Application", new AppString(this) );
			if ( renderer )
			{
				Core::RenderString* prop = (Core::RenderString*)engine->createInterface("Core","Core.RenderString");
				prop->init(renderer);
				saveProp( f, "Renderer", prop );
			}

			fclose(f);
		}
	}

	void saveProp( FILE * f, char *s, e6::IStringProp * prop )
	{
		fprintf( f, "\n# %s\r\n", s );
		uint n=prop->numProps();
		for ( uint i=0; i<n; i++ )
		{
			fprintf( f, "%s\t%s\r\n", prop->getPropName(i), prop->getPropValue(i) );
		}
		E_RELEASE(prop);
	}
	
	void renderToTex( Core::RenderToTexture * rt )
	{
		static Profile _p("Main::renderToTexture");
		_p.start();

		Core::Texture* tex = rt->getRenderTarget();
		if ( tex )
		{
			renderer->end3D();
			renderer->setRenderTarget( tex );
			renderer->begin3D();
		}
		E_RELEASE( tex );
		
		uint nl = 0;
		renderFirstCam( rt );
		renderLight( rt, nl );

		for ( uint i=0; i<rt->numChildren(); i++ )
		{
			Core::Node * n = rt->getChild(i);
			renderNode( n );
			E_RELEASE(n);
		}

		if ( rt )
		{
			// reset rendertarget to backbuffer:
			renderer->end3D();
			renderer->setRenderTarget( 0 );
			renderer->begin3D();
			renderer->setCamera( cam );
		}
		_p.stop();
	}

	bool renderFirstCam( Core::Node * node )
	{
		Core::Camera * cc = (Core::Camera*)node->cast("Core.Camera");
		if ( cc && cc->getVisibility() )
		{
			renderer->setCamera( cc );
			E_RELEASE( cc );
			return 1;
		}
		for ( uint i=0; i<node->numChildren(); i++ )
		{
			Core::Node * n = node->getChild(i);
			bool ok = renderFirstCam( n );
			E_RELEASE(n);
			if ( ok ) break;
		}
		E_RELEASE( cc );
		return 0;
	}

	void renderNode( Core::Node * node )
	{
		bool rendered = 0;
		//Core::Camera * cc = 0;
		Core::Mesh * mesh = 0;
		//if ( ! rendered )
		//{
		//	// try cam
		//	cc = (Core::Camera*)node->cast("Core.Camera");
		//	if ( cc && cc->getVisibility() && strcmp(cc->getName(), "DefCam" ) )
		//	{
		//		rendered = renderer->setCamera( cc );
		//	}
		//}
		if ( ! node->getVisibility() )
		{
			return;
		}
		if ( ! rendered )
		{
			// try mesh
			mesh = (Core::Mesh*)node->cast("Core.Mesh");
			if ( mesh && mesh->getVisibility() )
			{
				rendered = renderer->setMesh( mesh );
			}
			E_RELEASE( mesh );
		}
		if ( ! rendered )
		{
			// try RenderToTexture
			Core::RenderToTexture * rt = (Core::RenderToTexture*)node->cast("Core.RenderToTexture");
			if ( rt )
			{
				if ( rt->getVisibility() )
				{
					// render children into rt:
					renderToTex( rt );
				}
				E_RELEASE( rt );
				return;
			}
		}

		for ( uint i=0; i<node->numChildren(); i++ )
		{
			Core::Node * n = node->getChild(i);
			renderNode( n );
			E_RELEASE(n);
		}

//		E_RELEASE( cc );
//		E_RELEASE( mesh );
	}


	void renderLight( Core::Node * node, uint & ln )
	{
		Core::Light * light = (Core::Light*)node->cast("Core.Light");
		if ( light && light->getVisibility() )
		{
			renderer->setLight( ln++, light );
		}
		for ( uint i=0; i<node->numChildren(); i++ )
		{
			Core::Node * n = node->getChild(i);
			renderLight( n, ln );
			E_RELEASE(n);
		}
		E_RELEASE( light );
	}

	void renderScene()
	{
		if ( ! renderer ) return ;

		Core::Node * root  = world->getRoot();	
		root->synchronize();

		float c[4] = { timeNow, sinf(timeNow), cosf(timeNow), timeStep };
		renderer->setVertexShaderConstant( e6::VS_TIME, c );
		renderer->setVertexShaderConstant( e6::VS_LIGHT_COLOR_AMBIENT, (float*)&(ambient) );

		renderer->clear();
		renderer->begin3D();
		renderer->setCamera( cam );

		uint nLights=0;
		renderLight(root,nLights);

		renderNode(root);

		renderer->end3D();
		renderer->swapBuffers();
		E_RELEASE(root);	
	}

	void mainloop()
	{
		MSG msg={0}; 
		while( msg.message != WM_QUIT )
		{
			if( PeekMessage(  &msg, 0, 0, 0, PM_REMOVE ) )
			{
			#ifdef DOPROF
				static Profile p("Main::message");
				p.start();
			#endif
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			#ifdef DOPROF
				p.stop();
			#endif
			}
			else
			{
			#ifdef DOPROF
				static Profile p("Main::idle");
				p.start();
			#endif
				on_idle();
			#ifdef DOPROF
				p.stop();
			#endif
			}
		}
		//~ printf( __FUNCTION__ " finished\n");
	}

	struct AppString : e6::Class< e6::IStringProp, AppString >
	{
		Dia * dia;
		AppString( Dia * d )
			: dia(d)
		{
			E_ASSERT(dia);
		}
		virtual uint numProps()
		{
			return 15;
		}
		virtual const char * getPropName(uint i)
		{
			static char * _n[] = {"running","frame","time","timeDelta","timeSleep","timeBegin","timeEnd","realTime","record","saveFrame","scriptFrame","ambient","render","width","height"};
			return _n[i];
		}
		POINT size(HWND hwnd) 
		{
			RECT r;
			GetClientRect(hwnd,&r);
			POINT p = {(r.right-r.left),(r.bottom-r.top) };
			return p;
		}
		virtual const char * getPropValue(uint i)
		{
			switch(i)
			{
				case 0: return e6::toString( (uint)dia->running );
				case 1: return e6::toString( dia->frameNo );
				case 2: return e6::toString( dia->timeNow );
				case 3: return e6::toString( dia->timeStep );
				case 4: return e6::toString( dia->timeSleep );
				case 5: return e6::toString( dia->timeBegin );
				case 6: return e6::toString( dia->timeEnd );
				case 7: return e6::toString( (uint)dia->doRealTime );
				case 8: return e6::toString( (uint)dia->doRecord );
				case 9: return e6::toString( (uint)dia->doSaveFrame );
				case 10: return e6::toString( (uint)dia->doRunScript );
				case 11: return e6::toString( dia->ambient );
				case 12: return e6::toString( (uint)dia->doRender );
				case 13: return e6::toString( (uint)size(dia->render.hwnd).x );
				case 14: return e6::toString( (uint)size(dia->render.hwnd).y );
			}
			return 0;
		}
		virtual const char * getPropType(uint i)
		{
			static char * _n[] = {"i","i","f","f","i","f","f","i","i","i","i","f4","i","i","i"};
			return _n[i];
		}
		virtual bool setPropValue(uint i, const char *s)
		{
			switch(i)
			{
				case 0: dia->running = e6::stringToInt(s); return 1;
				case 1: dia->frameNo = e6::stringToInt(s); return 1;
				case 2: dia->timeNow = e6::stringToFloat(s); return 1;
				case 3: dia->timeStep = e6::stringToFloat(s); return 1;
				case 4: dia->timeSleep = e6::stringToInt(s); return 1;
				case 5: dia->timeBegin = e6::stringToInt(s); return 1;
				case 6: dia->timeEnd = e6::stringToInt(s); return 1;
				case 7: dia->doRealTime = e6::stringToInt(s); return 1;
				case 8: dia->doRecord = e6::stringToInt(s); return 1;
				case 9: dia->doSaveFrame = e6::stringToInt(s); return 1;
				case 10: dia->doRunScript  = e6::stringToInt(s); return 1;
				case 11: dia->ambient = e6::stringToFloat4(s); return 1;
				case 12: dia->doRender = e6::stringToInt(s); return 1;
			}
			return 0;
		}
	};
};


//#define NO_KONSOLE

#ifdef NO_KONSOLE
	#define argc __argc
	#define argv __argv
	int WINAPI WinMain(HINSTANCE hinst, HINSTANCE previnst, LPSTR cmdline, int cmdshow) 
#else
	int main(int argc, char **argv)
#endif
{
	printf("!\n%s\n", argv[0]);

	if ( argc > 1 && (!strcmp(argv[1],"-h")||!strcmp(argv[1],"/?")) )
	{
		printf( "use: %s [options] [filelist]\n" );
		printf( "options:\n" );
		printf( "	-r [dllname]  specify renderer\n" );
		printf( "	-s [dllname]  specify scriptengine\n" );
		return 0;
	}

    w32::CLimitSingleInstance s("wow_i_like_it !");
	if ( s.IsAnotherInstanceRunning() ) {
        w32::alert("e6","one instance at a time !");
        return 0;
    }
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	InitCommonControls();
	
	Dia win;
	win.init( argc, argv ); 
    win.create( "e6","e6", 60,60, 600,400, WS_OVERLAPPEDWINDOW, 0, 0, GetModuleHandle(0) );
	win.mainloop() ;
	
	printf("!\n");
	return 0;
}
