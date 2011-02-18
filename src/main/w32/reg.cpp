#include "reg.h"
#include <stdio.h>


bool setProfile( HKEY root, char *keyName, char *entry, char *format, ...)
{
	HKEY  hKey;
	DWORD dw;
    bool  ok = false;
    char  value[0x200] = {0};

    va_list args;
    va_start( args, format );
    vsprintf( value, format, args );
    va_end( args );

    if ( RegCreateKeyEx( root, keyName, 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dw) == ERROR_SUCCESS) {
        int l=strlen(value);
        value[l] = 0;
        if ( RegSetValueEx( hKey, entry, 0, REG_SZ, (unsigned char *)value, l+1) == ERROR_SUCCESS ) 
			ok = true;
        RegCloseKey(hKey);
    }
    return ok;
}


bool getProfile( HKEY root, char *keyName, char *entry, char *result, unsigned long res_len )
{
    bool ok = false;
	HKEY hKey;
	DWORD dwType;
	result[0]=0;
    if ( RegOpenKeyEx( root, keyName, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
		if ( RegQueryValueEx( hKey, entry, 0, &dwType, (unsigned char *)result, &res_len) == ERROR_SUCCESS && dwType == REG_SZ) 
        {
		  result[res_len] = 0;
		  ok = true;
		}
		RegCloseKey(hKey);
	}
    return ok;
}


bool str_on( const char *s )
{
    if ( s == 0 )             return false;
    if ( *s == 0 )            return false;
    if ( *s == '0' )          return false;
    if ( *s == '1' )          return true;
    if ( !strcmp(s,"false") ) return false;
    if ( !strcmp(s,"true") )  return true;
    if ( !strcmp(s,"off") )   return false;
    if ( !strcmp(s,"on") )    return true;
    if ( !strcmp(s,"no") )    return false;
    if ( !strcmp(s,"yes") )   return true;
    if ( !strcmp(s,"si") )    return true;
    if ( !strcmp(s,"maybe") ) return false; //(!)
    return true;
}




/*
bool enumProfileKey( HKEY root, char *keyName, int off, bool (*fun)(char*,char*,void*), void*user )
{
    bool ok = false;
	HKEY hKey;
    if ( RegOpenKeyEx( root, keyName, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
        DWORD id = off;
        DWORD kl = 200, vl = 200;
        DWORD t  = REG_SZ;
        char  k[200],   v[200];
        while ( RegEnumValue( hKey, id, k,&kl, 0,&t, (unsigned char*)v,&vl ) == ERROR_SUCCESS ) {
	        ok = fun(k,v,user);
            id ++;
		}
		RegCloseKey(hKey);
	}
    return ok;
}
bool saveProfileKey( HKEY root, char *keyName, char*file )
{
    bool ok = false;
	HKEY hKey;
    if ( RegOpenKeyEx( root, keyName, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
        ok = ( RegSaveKey( hKey, file, 0 ) == ERROR_SUCCESS );
        RegCloseKey(hKey);
	}
    return ok;
}
*/
