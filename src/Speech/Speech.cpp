
#include "../e6/e6_impl.h"
#include "../e6/e6_sys.h"
#include "../e6/sys/w32/e6_thread.h"
#include "Speech.h"

#include <sapi.h>

//#include <sphelper.h>


using e6::uint;
using e6::ClassInfo;


#define COM_RELEASE(x) { if ((x)) (x)->Release(); (x) = NULL; }


namespace Speech
{
	struct _ComUser
	{
		_ComUser()	{CoInitialize(0);}
		~_ComUser() {CoUninitialize();}
	} _we_need_a_singleton_per_module;


	inline int w2a( WCHAR *in, char *out )
	{
		out[0]=0;
		return WideCharToMultiByte(CP_ACP, 0, in, -1, out, MAX_PATH, 0, 0); 
	}

	inline int a2w( const char *in, WCHAR *out )
	{
		out[0]=0;
		return MultiByteToWideChar(CP_ACP, 0, in, -1, out, MAX_PATH); 
	}


	struct CVoice
		: public e6::CName< Voice, CVoice >
	{

		ISpVoice * spVoice;

		CVoice()
			: spVoice(0)
		{
			HRESULT hr = CoCreateInstance( CLSID_SpVoice, NULL, CLSCTX_INPROC_SERVER, IID_ISpVoice, (LPVOID *)&(spVoice) ); 
		}

		CVoice(ISpVoice * v)
			: spVoice(v)
		{
		}

		~CVoice()
		{
			COM_RELEASE( spVoice );
		}



		virtual uint speak( const char * txt ) const 
		{
			if ( ! spVoice )
				return 0;

			WCHAR wtxt[800];
			a2w(txt,wtxt);

			ULONG pulstream = 0;
			HRESULT hr = spVoice->Speak( wtxt, 0, &pulstream );

			return hr==S_OK; 
		}
		

		// Supported values range from -10 to 10 
		virtual uint setRate( int s )
		{
			if ( ! spVoice )
				return 0;

			HRESULT hr = spVoice->SetRate( s );

			return hr==S_OK; 
		}

		// Supported values range from 0 to 100 
		virtual uint setVolume( uint s )
		{
			if ( ! spVoice )
				return 0;

			HRESULT hr = spVoice->SetVolume ( s );

			return hr==S_OK; 
		}
	};




	struct CRecognizer
		: e6::CName< Recognizer, CRecognizer >
		, e6::sys::Thread
	{

		ISpRecognizer  * spRecognizer;	
		ISpRecoContext * spContext;	
		ISpRecoGrammar * spGrammar;	
		ISpLexicon     * spLexicon;
		
		Speech::Recognizer::Listener * listener;

		uint numAlternates;

		
		CRecognizer()
			: spRecognizer(0)
			, spContext(0)
			, spGrammar(0)
			, spLexicon(0)
			, listener(0)
			, numAlternates(0)
		{
			HRESULT hr = CoCreateInstance( CLSID_SpSharedRecognizer, NULL, CLSCTX_INPROC_SERVER, IID_ISpRecognizer, (LPVOID *)&(spRecognizer) ); 
			if ( hr != S_OK )
			{
				printf("Error creating recognizer\n");
				return ;
			}

			hr = spRecognizer->CreateRecoContext( &spContext );					
			//HRESULT hr = CoCreateInstance( CLSID_SpSharedRecoContext, NULL, CLSCTX_INPROC_SERVER, IID_ISpRecoContext, (LPVOID *)&(spContext) ); 
			if ( hr != S_OK )
			{
				printf("Error creating reco context\n");
				return ;
			}


			//hr = spContext->SetNotifyCallbackInterface( this, 0, 0 );
			hr = spContext->SetNotifyWin32Event();
			if ( hr != S_OK )
			{
				printf("Error setting reco callback\n");
				return;
			}

			const ULONGLONG ullInterest = SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_HYPOTHESIS);
			hr = spContext->SetInterest(ullInterest, ullInterest);
			if ( hr != S_OK )
			{
				printf("Error setting reco Interest\n");
				return;
			}

			//hr = spContext->SetAudioOptions(SPAO_RETAIN_AUDIO, NULL, NULL);
			//if ( hr != S_OK )
			//{
			//	printf("Error setting Audio Options\n");
			//	return;
			//}

			hr = spContext->CreateGrammar( 0, &spGrammar );
			if ( hr != S_OK )
			{
				printf("Error creating grammar\n");
				return;
			}
			hr = spGrammar->LoadDictation(NULL, SPLO_STATIC);
			if ( hr != S_OK )
			{
				printf("Error loading grammar\n");
				return;
			}
			hr = spGrammar->SetDictationState( SPRS_ACTIVE );
			if ( hr != S_OK )
			{
				printf("Error activating grammar\n");
				return;
			}

			dumpLex();
		}

		virtual ~CRecognizer()
		{
			COM_RELEASE( spGrammar );
			COM_RELEASE( spContext );
			COM_RELEASE( spRecognizer );
		}


		virtual uint start( Listener * l, bool newThread ) 
		{
			this->listener = l;

			if ( newThread )
			{
				Thread::start();
			}
			else
			{
				run();
			}
			return 1;
		}

		virtual uint enableCallBack( uint cbMask ) 
		{
			ULONGLONG interest = SPEI_UNDEFINED;
			if ( cbMask & cbRecognition )
				interest |= SPFEI(SPEI_RECOGNITION);
			if ( cbMask & cbHypothesis )
				interest |= SPFEI(SPEI_HYPOTHESIS);
			if ( cbMask & cbAdaption )
				interest |= SPFEI(SPEI_ADAPTATION);
			if ( cbMask & cbFalseReco )
				interest |= SPFEI(SPEI_FALSE_RECOGNITION);
			if ( cbMask & cbInterference )
				interest |= SPFEI(SPEI_INTERFERENCE);
			HRESULT hr = spContext->SetInterest( interest, interest );
			return (hr==S_OK);
		}

		virtual uint enableNumAlternates( uint nAltern ) 
		{
			 numAlternates = min(32,nAltern);
			 return numAlternates;
		}

		virtual uint showUI( const char * topic, const char *fileName=0 ) 
		{
			HRESULT hr = E_FAIL;
			ULONG cch = 0;
			WCHAR wb[8012];
			if ( ! fileName )
				fileName = "train.txt";
			if ( e6::sys::fileExists( fileName ) )
			{
				const char * buf = e6::sys::loadFile( fileName );
				if ( buf ) 
				{
					cch=strlen(buf);
					a2w(buf,wb);
				}
			}
			WCHAR wtopic[64];
			a2w(topic,wtopic);
			hr = spRecognizer->DisplayUI(GetDesktopWindow(), NULL, wtopic, (void *)wb, cch * sizeof(WCHAR));			
			return (hr==S_OK);
		}

		bool handlePhrase(ISpPhrase * reco, uint alt)
		{
			if ( ! reco ) 
				return 0;

			bool ok = 0;
			uint id = ( alt==0 ? getRule( reco ) : 0 );

			WCHAR *wtxt = 0;
			HRESULT hr = reco->GetText(SP_GETWHOLEPHRASE,SP_GETWHOLEPHRASE,0,&wtxt,0);

			if ( (hr == S_OK) && listener )
			{
				char txt[800];
				w2a(wtxt,txt);
				ok = this->listener->listen( txt, id, alt );
				if ( ! ok ) 
				{
					printf ( __FUNCTION__ " failed on (%s %d %d) .\n", txt, id, alt );
				}
			}
			CoTaskMemFree( wtxt );
			return ok;
		}
		
		//! wait a minute for input.
		virtual void run()
		{
			HRESULT hr=E_FAIL;
			bool ok = 0;
			while( S_OK == spContext->WaitForNotifyEvent(60000) )
			{
				SPEVENT ev = {0};
				spContext->GetEvents(1, &ev, NULL);
	 
				if ( ev.eEventId == SPEI_INTERFERENCE )
				{
					ok = listener->listen( "<interference>", 0, 9000+cbInterference );
					if ( ! ok ) 
					{
						printf ( __FUNCTION__ " failed on interference.\n" );
						return;
					}
				}
				if ( ev.eEventId == SPEI_FALSE_RECOGNITION )
				{
					ok = listener->listen( "<false_recognition>", 0, 9000+cbFalseReco );
					if ( ! ok ) 
					{
						printf ( __FUNCTION__ " failed on false_recognition.\n" );
						return;
					}
				}
				if ( ev.eEventId == SPEI_ADAPTATION )
				{
					ISpRecoResult * res = (ISpRecoResult*)(ev.lParam);
					ok = handlePhrase( res, 9000 + cbAdaption );
					if ( ! ok ) 
					{
						printf ( __FUNCTION__ " failed on adaption.\n" );
						return;
					}
				}
				if ( ev.eEventId == SPEI_HYPOTHESIS )
				{
					ISpRecoResult * res = (ISpRecoResult*)(ev.lParam);
					ok = handlePhrase( res, 9000 + cbHypothesis );
					if ( ! ok ) 
					{
						printf ( __FUNCTION__ " failed on Hypothesis.\n" );
						return;
					}
				}
				if ( ev.eEventId == SPEI_RECOGNITION )
				{
					ISpRecoResult * res = (ISpRecoResult*)(ev.lParam);
					if ( res )
					{
						ok = handlePhrase( res, 0 );
						if ( ! ok ) 
						{
							//printf ( __FUNCTION__ " failed on phrase.\n" );
							return;
						}

						if ( numAlternates )
						{
							ISpPhraseAlt  *pPhrases[32];
							ULONG count = 0;
							ULONG maxAlternates = min(numAlternates,32);
							hr = res->GetAlternates( 0, SPPR_ALL_ELEMENTS ,maxAlternates ,pPhrases,&count);
							if ( (hr==S_OK) && (count>1) )
							{
								for ( uint i=1; i<count; i++ )
								{
									ok = handlePhrase( pPhrases[i], i );
									if ( ! ok ) 
									{
										break;
									}
								}
							}
						}
					}
				}
			}
		}


		uint getRule(ISpPhrase *pPhrase)
		{
			uint id = 0;
			SPPHRASE *pElements=0;
			// Get the phrase elements, one of which is the rule id we specified in
			// the grammar.  Switch on it to figure out which command was recognized.
			if (SUCCEEDED(pPhrase->GetPhrase(&pElements)))
			{   
				id = pElements->Rule.ulId;
				// Free the pElements memory which was allocated for us
				::CoTaskMemFree(pElements);
			}
			return id;
		}



		virtual void adapt(const char * aData)
		{
			HRESULT hr = S_OK;

			do { // once.

				// set interest in the adaptation event
				hr = spContext->SetInterest(SPFEI(SPEI_ADAPTATION), SPFEI(SPEI_ADAPTATION));
				if ( hr != S_OK )
				{
					printf(__FUNCTION__ " SetInterest ADAPTATION failed.\n");
					break;
				}

				ULONG l = strlen(aData);
				WCHAR wData[512];
				a2w(aData,wData);

				// send each chunk of data the engine
				hr = spContext->SetAdaptationData(wData, l);
				if ( hr != S_OK )
				{
					printf(__FUNCTION__ " SetAdaptationData failed(%s).\n",aData);
					break;
				}

				// wait for the engine to ask for more data
				hr = spContext->WaitForNotifyEvent(10000);
				if ( hr != S_OK )
				{
					printf(__FUNCTION__ " WaitForNotifyEvent failed.\n");
					break;
				}
			} while(0); // once.

			hr = spContext->SetInterest(SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION));
			if ( hr != S_OK )
			{
				printf(__FUNCTION__ " SetInterest RECOGNITION failed.\n");
			}
			printf(__FUNCTION__ "[%s] ok.\n",aData);
		}

		void dumpLex()
		{
			SPWORDLIST spwordlist;
			memset(&spwordlist, 0, sizeof(spwordlist));
			DWORD dwCookie = 0;
			DWORD dwGeneration = 0;
		    
			HRESULT hr = CoCreateInstance( CLSID_SpLexicon, NULL, CLSCTX_INPROC_SERVER, IID_ISpLexicon, (LPVOID *)&(spLexicon) ); 
			if ( hr != S_OK )
			{
				printf("Error creating lex.\n");
				return;
			}
			printf("lex %x.\n", spLexicon);
			int i = 0;
			while (SUCCEEDED(hr = spLexicon->GetWords(0x3, &dwGeneration, &dwCookie, &spwordlist)))
			{
				
				printf("lex(%d) %x %x %x %x %x.\n",i, dwGeneration, dwCookie, spwordlist.ulSize, spwordlist.pvBuffer, spwordlist.pFirstWord);
				for (SPWORD *pword = spwordlist.pFirstWord;
					pword != NULL;
					pword = pword->pNextWord)
				{
					char b[200];
					w2a( pword->pszWord, b );
					printf( "%s\n",b );
					for (SPWORDPRONUNCIATION *pwordpron = pword->pFirstWordPronunciation;
						pwordpron != NULL;
						pwordpron = pwordpron->pNextWordPronunciation)
					{
						printf( "	%i [", pwordpron->ePartOfSpeech );
						WCHAR * wc = (WCHAR*)(pwordpron->szPronunciation);
						while( *wc )
						{
							printf( " %02x", *wc );
							wc++;
						}
						printf( " ]\n" );
					}
				}
		 
				if (hr == S_OK)
					   break;  // nothing more to retrieve
			}
			COM_RELEASE(spLexicon);
			//free all the buffers
			CoTaskMemFree(spwordlist.pvBuffer);
		}      

		virtual uint loadGrammar( const char * fileName )
		{
			WCHAR wname[300];
			a2w( fileName, wname );
			HRESULT hr = spGrammar->LoadCmdFromFile( wname, SPLO_DYNAMIC );
			if ( hr != S_OK )
			{
				printf("Error loading grammar[%s]\n", fileName);
				return 0;
			}
	        hr = spGrammar->SetRuleState( NULL, NULL, SPRS_ACTIVE );
			if ( hr != S_OK )
			{
				printf("Error activating grammar rulestate[%s]\n", fileName);
				return 0;
			}
			return (hr == S_OK);
		}

//		virtual uint addDynamicRule( const char * ruleName, const char ** item )
		virtual uint addDynamicRule( uint ruleId, const char ** item, bool doClear )
		{
			HRESULT hr = S_OK;
			SPGRAMMARSTATE gs = SPGS_DISABLED;
			hr = spGrammar->GetGrammarState(&gs);

			do { // once.

				// deactivate the grammar to prevent premature recognitions to an "under-construction" grammar
				hr = spGrammar->SetGrammarState(SPGS_DISABLED);
				if ( hr != S_OK )
				{
					printf("Error disabling grammar\n");
					break;
				}

				SPSTATEHANDLE hRule;
				hr = spGrammar->GetRule(0, ruleId, SPRAF_Dynamic, FALSE, &hRule);
				//WCHAR wname[200];
				//a2w(ruleName,wname);
				//hr = spGrammar->GetRule(wname, 0, SPRAF_Dynamic, FALSE, &hRule);
				if ( hr != S_OK )
				{
					printf("Error get grammar rule[%d].\n",ruleId);
					break;
				}

				if ( doClear )
				{
					// clear content:
					hr = spGrammar->ClearRule(hRule);
					if ( hr != S_OK )
					{
						printf("Error clear grammar rule.\n");
						break;
					}
				}
				while ( *item )
				{
					WCHAR wc[512];
					a2w((*item), wc);
					hr = spGrammar->AddWordTransition(hRule, NULL, wc, NULL, SPWT_LEXICAL, 1, NULL);
					if ( hr != S_OK )
					{
						printf("Error AddWordTransition[%s].\n", (*item) );
						break;
					}
					item ++;
				}
				// commit the grammar changes, which updates the grammar inside SAPI,
				//    and notifies the SR Engine about the rule change (i.e. "ADDRESS_BOOK"
				hr = spGrammar->Commit(NULL);
				if ( hr != S_OK )
				{
					printf("Error Commit grammar Transitions.\n");
					break;
				}
				hr = spGrammar->SetRuleIdState(ruleId,SPRS_ACTIVE);
				if ( hr != S_OK )
				{
					printf("Error enabling rule[%d].\n", ruleId);
					break;
				}
			} while(0); // once.

			//LPSTREAM strm=0;
			//hr = ::CreateStreamOnHGlobal(NULL, true, &strm);
			//// save the current grammar to the global stream
			//hr = spGrammar->SaveCmd(strm, NULL);
			//strm->Commit(0);
			//LARGE_INTEGER no;
			//no.QuadPart = NULL;
			//strm->Seek(no,0,NULL);
			//char wb[8024];
			//ULONG nb=0;
			//strm->Read(wb,8024,&nb);
			//wb[nb]=0;
			////char b[8024];
			////w2a(wb,b);
			////printf("(%d)%s\n",nb,b);
			//printf("(%d)\n",nb);
			//for ( int i=0; i<nb; i++ )
			//{
			//	printf("%c ",wb[i]);
			//	if ( i%16==0 )
			//		printf("\n");
			//}
			//printf("\n");

			hr = spGrammar->SetGrammarState(gs);
			if ( hr != S_OK )
			{
				printf("Error enabling grammar.\n");
			}
			return (hr == S_OK);
		}

		virtual Voice * getVoice() 
		{
			ISpVoice * v = 0;
			spContext->GetVoice(&v);
			return new CVoice( v );
		}
		virtual uint emulateSpech( const char * txt ) 
		{
			HRESULT hr = E_FAIL;
			WCHAR wb[1024];
			a2w(txt,wb);
			ISpPhrase * phrase = 0;
			// .. i have to build the phrase ..
			hr = spRecognizer->EmulateRecognition( phrase );

			return (hr == S_OK);
		}

	}; // CRecognizer



} // Speech



extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Speech.Voice",		 "Speech",	Speech::CVoice::createInterface,		 Speech::CVoice::classRef	},
		{	"Speech.Recognizer", "Speech",	Speech::CRecognizer::createInterface, Speech::CRecognizer::classRef	},
		{	0, 0, 0	}//,
		//0
	};

	*ptr = (ClassInfo *)_ci;

	return 2; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("Speech 00.000.0023 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}




/***
**
*
[houston, we have a problem]

to hear act teasingly and on
Tinley and on
gays in the Amazon
dues and we have a problem
teasingly have on
this and we have a problem
began on
gays and we have a problem
is and we have a problem
dues and we have in common
to have on
gays in the hands on
gays and we have on
Tuesday-
Tuesday-
dozen we have in
tears and we have in
gays and we have a problem
teasingly have the home
teasingly have a problem
and we have a problem
is and we have a problem
is and we have a problem

*
**
***/
