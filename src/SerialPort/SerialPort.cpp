
#include "../e6/e6_impl.h"
#include "SerialPort.h"

#ifdef WIN32
 #include <windows.h>
 #include <stdio.h>
#endif // WIN32


using e6::uint;
using e6::ClassInfo;


namespace SerialPort
{

#ifdef WIN32

	struct CSerialDevice
		: public e6::CName< SerialPort::Device, CSerialDevice >
	{
		HANDLE hComm;
		DCB dcb_alt;
		COMMTIMEOUTS timeouts_alt;

		CSerialDevice() 
		{
			this->unique("SerialPort");
			this->hComm = INVALID_HANDLE_VALUE;
		}
		~CSerialDevice() 
		{
			this->close();
		}


		virtual bool open (int nComPortNr, bool readOnly)
		{
			if (INVALID_HANDLE_VALUE != hComm)  // we're open?
				return (TRUE);

			char szPort[15];
			wsprintf (szPort, "\\\\.\\COM%d", nComPortNr);

			DWORD flag = GENERIC_READ;
			if ( ! readOnly )
				flag |= GENERIC_WRITE;

			this->hComm = CreateFile (szPort, // COM-Port цffnen
				flag, // 
				FILE_SHARE_READ,	// (non) exclusive
				0,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL); // non OVERLAPPED, exclusive

			if (this->hComm == INVALID_HANDLE_VALUE) 
			{
				MessageBox (NULL, "could not open comport!\nCreateFile()", NULL, NULL);
				return (FALSE);
			}

			//if ( ! this->setTimeout( 1000 ) )
			//{
			//	return false;
			//}

			//if ( ! this->setMode( nBaud, nBits, nParity, nStopp ) )
			//{
			//	return false;
			//}
			return true;
		}

		virtual bool close()
		{
			BOOL bResult;

			if(INVALID_HANDLE_VALUE == hComm)
				return (TRUE);

			SetCommTimeouts(hComm, &timeouts_alt);
			SetCommState(hComm, &dcb_alt);

			bResult = CloseHandle(hComm);
			hComm   = INVALID_HANDLE_VALUE;

			return(bResult);
		}


		virtual bool isOpen()
		{
			if (INVALID_HANDLE_VALUE != this->hComm)
				return (TRUE);
			else
				return (FALSE);
		}


		virtual bool setMode(int nBaud, int nBits, int nParity, int nStopp)
		{
			if (INVALID_HANDLE_VALUE == this->hComm)
			{
				SetLastError(ERROR_INVALID_HANDLE);
				return (FALSE);
			}

			DCB dcb;
			ZeroMemory (&dcb, sizeof(dcb));

			if (!GetCommState (hComm, &dcb))
			{
				close ();
				MessageBox (NULL, "could not open comport!\nGetCommState()",
					NULL, NULL);
				return (FALSE);
			}

			dcb_alt = dcb; 
			dcb.DCBlength         = sizeof(DCB);
			dcb.fBinary           = TRUE; // always TRUE
			dcb.fParity           = TRUE;
			dcb.fOutxCtsFlow      = FALSE;
			dcb.fOutxDsrFlow      = FALSE;
			dcb.fDtrControl       = DTR_CONTROL_ENABLE;
			dcb.fDsrSensitivity   = FALSE;
			dcb.fTXContinueOnXoff = TRUE;
			dcb.fOutX             = FALSE;
			dcb.fInX              = FALSE;
			dcb.fErrorChar        = FALSE;
			dcb.fNull             = FALSE;
			dcb.fRtsControl       = RTS_CONTROL_ENABLE;
			dcb.fAbortOnError     = FALSE;
			dcb.wReserved         = 0; // always true!


			dcb.BaudRate = nBaud;
			dcb.ByteSize = (BYTE)nBits;
			dcb.Parity   = (BYTE)nParity;
			dcb.StopBits = (BYTE)nStopp;
			dcb.fParity = (dcb.Parity != NOPARITY);

			if (!SetCommState (hComm, &dcb))
			{
				// try to revert:
				if (!SetCommState (hComm, &dcb_alt))
				{
					close ();
					MessageBox (NULL, "could not set COM-Port Parameter\nSetCommState()",
						NULL, NULL);
					return (FALSE);
				}
				return false;
			}
			return true;
		}


		virtual bool setTimeout (int iTotalReadTimeout)
		{
			if (INVALID_HANDLE_VALUE == hComm)
				return (TRUE);

			if (!GetCommTimeouts(this->hComm, &timeouts_alt)) 
			{
				this->close ();
				MessageBox (NULL, "could not open comport!\nGetCommTimeouts()",
					NULL, NULL);
				return (FALSE);
			}

			COMMTIMEOUTS timeouts;
			timeouts.ReadIntervalTimeout = MAXDWORD ;
			timeouts.ReadTotalTimeoutMultiplier = MAXDWORD ;
			timeouts.ReadTotalTimeoutConstant = (DWORD) iTotalReadTimeout;
			timeouts.WriteTotalTimeoutMultiplier = 1000;
			timeouts.WriteTotalTimeoutConstant = 1000;

			if (!SetCommTimeouts(hComm, &timeouts))
				return (FALSE);

			return(TRUE);
		}


		virtual bool writeCommByte (unsigned char ucByte)
		{
			if (INVALID_HANDLE_VALUE == hComm)
			{
				SetLastError(ERROR_INVALID_HANDLE);
				return (FALSE);
			}

			if (!TransmitCommChar(hComm, ucByte))
				return (FALSE);

			return (TRUE);
		}


		virtual int sendData (const char *buffer, int iBytesToWrite)
		{
			if (INVALID_HANDLE_VALUE == hComm)
			{
				SetLastError(ERROR_INVALID_HANDLE);
				return (0);
			}

			DWORD dwBytesWritten = 0;
			WriteFile(hComm, buffer, iBytesToWrite, &dwBytesWritten, NULL);

			return ((int) dwBytesWritten);
		}


		virtual int readData (char *buffer, int iMaxCount)
		{
			if (INVALID_HANDLE_VALUE == hComm)
			{
				SetLastError(ERROR_INVALID_HANDLE);
				return (0);
			}

			DWORD dwRead = 0;
			char chRead;
			int i = 0;

			while (ReadFile(hComm, &chRead, 1, &dwRead, NULL))
			{
				if (dwRead != 1)
					break;

				buffer[i++] = chRead;

				if (i == iMaxCount)
					break;
			}

			return (i);
		}

	}; // CSerialDevice 

#else // !WIN32

#endif // WIN32
};

extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"SerialPort.Device",	 "SerialPort",	SerialPort::CSerialDevice::createSingleton, SerialPort::CSerialDevice::classRef	},
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
	mv->modVersion = ("SerialPort 00.000.0023 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}
