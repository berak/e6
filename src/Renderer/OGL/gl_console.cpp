#include "ParamDescription.h"
#include "World.h"
#include "Light.h"
#include "Model.h"
#include "Camera.h"
#include "World.h"
#include "gl_engine.h"
#include "e_keys.h"
#include "gl_console.h"

#include <stdio.h>
#include <stdlib.h>

Console::Console() 
    : nl(6),px(40),py(40),th(13),active(0) 
{ 
    clear();
    col[0]=col[1]=col[2]=1.0f;
    scol[0]=1.0f;scol[1]=scol[2]=0.2f; 
}

int Console::scroll( int n )
{
    if ( lo+n < 0 || lo+n >= MAX_LINES )
        return 0;
    if ( ! line[lo+n][0] )
        return 0;
    lo += n;
    if ( sl>=0 )
        sl += n;
    return n;
}

void Console::reset()
{
    cl=sl=lo=0;
}

void Console::clear()
{
    for ( int i=0; i<MAX_LINES; i++ )
        line[i][0]=0;
    reset();
}

int Console::add( char *s )
{
    if ( cl >= MAX_LINES )
        return 0;
    strcpy( line[cl],s );
    return cl ++ ;
}

void Console::render()
{
    int vp[4];
    engine->getViewPort(vp);
    ((GlEngine*)engine)->drawColor( col[0],col[1],col[2] );
    for ( int i=0,k=lo; i<nl; i++,k++ ) {
        if ( ! line[k][0] )
            continue;
        int x = px ;
        int y = vp[3] - py - i*th;
        if ( sl>=0 && sl==k ) {
            ((GlEngine*)engine)->drawColor( scol[0],scol[1],scol[2] );
        }
        ((GlEngine*)engine)->drawText( x,y, line[k] );			
        if ( sl>=0 && sl==k ) {
            ((GlEngine*)engine)->drawColor( col[0],col[1],col[2] );
        }
    }
}


int KeyEdit::set( char *s ) 
{
    active = 1;
    strcpy( buf , s );
    end = strlen( s ); 
    // fprintf( stderr, "set:'%s' %d\n", s, end );
    return end;
}


int KeyEdit::edit( int k ) 
{ 
    int state = STATE_VAL;
    if ( k>0 && k<127 && end<511 ) {
        switch(k) {
            case 8:
                if ( end < 1 ) break;
                buf[end]=0;
                end --;
                buf[end]=0;
                return STATE_EDIT;
            case 0:
            case 27:
            case '\r':
            case '\n':
            case '\t':
                break;
            default:
                buf[end++]=k;
                state = STATE_EDIT;
                //fprintf( stderr, "a k:%c s:%d e:%d\n", k, state, end );
                return state;
        }
    }
    //fprintf( stderr, "f k:%c s:%d e:%d\n", k, state, end );
    flush();
    return state;
}

KeyValueMenu::KeyValueMenu(RemoteControl *r)
    : rm(r),nsel(0),active(0),state(STATE_MENU) 
{
}


int KeyValueMenu::scroll( int n, int v )
{
    switch(state) {
        case STATE_MENU:
            menu.scroll(n);
            break;
        case STATE_ENT:
            ent.scroll(n);
            break;
        case STATE_RM:
            key.scroll(n);
            val.scroll(n);
            break;
        case STATE_VAL:
            edit( v );
            break;
    }
    return n;
}

void KeyValueMenu::clear()
{
    key.clear();
    val.clear();
    ent.clear();
}

int KeyValueMenu::add( char *k, char *v )
{
    key.add(k);
    val.add(v);
    return key.cl ;
}

void KeyValueMenu::render()
{
    switch(state) {
        case STATE_MENU:
            menu.render();
            break;
        case STATE_ENT:
            ent.render();
            break;
        case STATE_RM:
        case STATE_VAL:
            key.render();
            val.render();
            break;
        case STATE_EDIT:
            ed.render();
            break;
    }
}
bool KeyValueMenu::editAdd( ParamDescription &p, int i, float f )
{
    bool changed = true;

    switch( p.typeID ) {
        case 'x':
        case 'i':
            (*(int*)p.data) += i;
            break;
        case 'b':
            (*(int*)p.data) = (i>0);
            break;
        case 'f':
            (*(float*)p.data) += f;
            break;
        case 'v':
            if ( nsel > 2 ) return false;
            ((float*)p.data)[nsel] += f;
            break;
        case 'c':
            if ( nsel > 3 ) return false;
            ((float*)p.data)[nsel] += f;
            break;
        default:
            return false;
            break;
    }
    return true;
}

void KeyValueMenu::edit( int k ) 
{
    static float f_hi = 5.0f;
    static float f_lo = 0.02f;

    static int i_hi = 20;
    static int i_lo = 1;

    if ( ! rm )
        return;
    
    bool changed = true;
    ParamDescription p;
    if ( ! rm->getParam( key.sl , &p ) )
        return ;

    switch( k ) {
        case KEY_UP       :
            changed = editAdd( p, i_lo, f_lo );
            break;
        case KEY_DOWN     : 
            changed = editAdd( p, -i_lo, -f_lo );
            break;
        case KEY_PAGE_UP  :
            changed = editAdd( p, i_hi, f_hi );
            break;
        case KEY_PAGE_DOWN:  
            changed = editAdd( p, -i_hi, -f_hi );
            break;
        /*
            break;
            break;
        case GLUT_KEY_HOME     :  
            break;
        case GLUT_KEY_END      :   
            break;
        case GLUT_KEY_INSERT   :  
            break;
        */
        default:
            changed = false;
            break;
    }
    if ( changed ) {
        strcpy( val.line[key.sl], p.getString() );
        rm->cmd( 'cpar', &p, 1 );
    }
}

void KeyValueMenu::reset() {
    state  = 0;
    ent.reset();
    key.reset();
    val.reset();
}

bool KeyValueMenu::editEnabled() {
    if ( ! rm )
        return 0;
    ParamDescription p;
    if ( ! rm->getParam( key.sl , &p ) )
        return 0;
    return (p.flag & ParamDescription::EDITABLE);
}

void KeyValueMenu::action(int k, int x, int y) {
    int s = state;

    if ( s == STATE_EDIT  ) {
        state = ed.edit(k);
        if ( state == s || !rm ) 
            return;
        //fprintf( stderr, "e :%c s:%d e:%d\n", k, state, ed.active );
        ParamDescription p;
        if ( ! rm->getParam( val.sl, &p ) )
            return;
        p.setString( ed.str() );
        rm->cmd( 'cpar', &p, 1 );
        strcpy( val.line[key.sl], p.getString() );
        return;
    }

    switch( k ) {
        /*
        case 27:
            active = 0;
            menu.reset();
            rm = 0;
            reset();
            break;
        */
        case KEY_UP       : 
            scroll( -1, k );
            break;
        case KEY_DOWN     : 
            scroll( 1, k );
            break;
        case KEY_PAGE_UP  :
            scroll( -menu.nl, k );
            break;
        case KEY_PAGE_DOWN:  
            scroll( menu.nl, k );
            break;

        case KEY_LEFT     :
            switch ( s ) {
                case STATE_MENU:
                    active = 0;
                    menu.reset();
                    rm = 0;
                    reset();
                    break;
                case STATE_ENT:
                    reset();
                    break;
                case STATE_RM:
                    if ( enumRM( menu.sl ) ) {
                        state = STATE_ENT;
                        ent.sl = 0;
                        key.sl = 0;
                        val.sl = -1;
                    } else {
                        reset();
                    }
                    break;
                case STATE_VAL:
                    if ( nsel > 0 ) {
                        nsel --;
                        break;
                    }
                    val.sl = -1;
                    state = STATE_RM;
                    break;
            }
            break;
        case KEY_RIGHT    : 
            switch ( s ) {
                case STATE_MENU:
                    if ( enumRM( menu.sl ) ) {
                        state = STATE_ENT;
                    } else {
                        if ( selectRM( menu.sl, 0 ) ) 
                            state = STATE_RM;
                        else 
                            state = STATE_MENU;
                    }
                    break;
                case STATE_ENT:
                    if ( selectRM( menu.sl, ent.sl ) )
                        state = STATE_RM;
                    val.sl = -1;
                    break;
                case STATE_RM:
                    if ( ! editEnabled() )
                        break;
                    state = STATE_VAL;
                    val.sl = key.sl;
                    break;
                case STATE_VAL:
                    if ( nsel < 3 )
                        nsel ++ ;
                    break;
            }
            break;
        case KEY_HOME     :  
            break;
        case KEY_END      :   
            break;
        case KEY_INSERT   :  
            if ( s == STATE_VAL ) {
                ed.set( val.line[val.sl] );
                state = STATE_EDIT;
            }
            break;
        default:  
            return;
    }
}

int KeyValueMenu::selectRM( int n, int k ) {
    switch(n) {
        default : rm = 0; break;
    }
    if ( ! rm ) 
        return 0;
    updateRM();
    return 1;
}

int KeyValueMenu::enumRM( int n ) {
    return 0;
}

int KeyValueMenu::updateRM() {
    clear();
    if ( ! rm ) return 0;

    int n; 
    ParamDescription p;
    for ( n=0; n<rm->numParams(); n++ ) {
        if ( ! rm->getParam( n, &p ) )
            continue;
        char *st = p.getString();
        add( p.name, ( (st&&st[0]) ? st : " " ) );
    }
    return n;
}
