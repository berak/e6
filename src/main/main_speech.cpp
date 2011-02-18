#include "../e6/e6_sys.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_enums.h"
#include "../Speech/Speech.h"
#include "../Net/Net.h"



using e6::uint;

//int tuio(e6::Engine * e)
//{
//	struct Printer : Net::Tuio::Callback
//	{
//		void call( int mode, Net::Tuio::Object &o )
//		{
//			switch(mode)
//			{ 
//				case 1 : 
//					printf( "set obj %2i %2i [%3.3f %3.3f %3.3f] [%3.3f %3.3f %3.3f] [%3.3f %3.3f]\n",o.id,o.fi,o.x,o.y,o.a,o.X,o.Y,o.A,o.m,o.r );
//					break;
//				case 2 : 
//					printf( "set cur %2i [%3.3f %3.3f] [%3.3f %3.3f] [%3.3f]\n",o.id,o.x,o.y,o.X,o.Y,o.m );
//					break;
//			}
//		}
//	} printer;
//
//	Net::Tuio::Listener * tl = (Net::Tuio::Listener*)e->createInterface("Net","Net.Tuio.Listener");
//	tl->start( 3333, printer );
//
//	for( int ever=1; ever; ever )
//		;
//
//	E_RELEASE(tl);
//	return 0;
//}
//

int main(int argc, char **argv)
{
	char * in = argc>1 ? argv[1] : 0;
	uint nal = argc>2 ? atoi(argv[2]) : 0;
	char *topic = argc>3 ? argv[3] : 0;
	bool r = false;
	e6::Engine * e = e6::CreateEngine();	

	//Speech::Voice * sp = (Speech::Voice*) e->createInterface( "Speech", "Speech.Voice" );
	//sp->speak("hello world");
	//E_RELEASE(sp);

	struct SPK : Speech::Recognizer::Listener
	{
		virtual uint listen( const char * txt, uint ruleID, uint alternation ) 
		{
			if ( alternation == (Speech::Recognizer::cbAdaption +9000) )
				printf( "adapt   %s\n", txt );
			else
			if ( alternation == (Speech::Recognizer::cbHypothesis +9000) )
				printf( "hyp.    %s\n", txt );
			else
			if ( alternation == (Speech::Recognizer::cbInterference +9000) )
				printf( "....    %s\n", txt );
			else
			if ( alternation == (Speech::Recognizer::cbFalseReco +9000) )
				printf( "false   %s\n", txt );
			else
			if ( ! ruleID )
				printf( "%4d a%3d %s\n",ruleID, alternation, txt );
			else
				printf( "%4d ok.  %s\n",ruleID, txt );
			if ( (ruleID == 253) && (!strcmp( txt, "exit" ) ) )
			{
				printf("\n - bye - \n" );
				return 0;
			}
			return 1;
		}
		
	} spk;

	Speech::Recognizer * sr = (Speech::Recognizer*) e->createInterface( "Speech", "Speech.Recognizer" );

	if ( in )
		sr->loadGrammar( in );

	if ( nal < 32 )
		sr->enableNumAlternates( nal );

	if ( topic )
		sr->showUI(topic);

	//sr->adapt( "foo" );
	//sr->adapt( "bar" );
	//sr->adapt( "mein kleines schweinchen" );
	sr->enableCallBack( Speech::Recognizer::cbRecognition
					  | Speech::Recognizer::cbInterference 
					  | Speech::Recognizer::cbFalseReco );

	//const char* people[] = {"me","my dog","louisa","mom",0};
	//sr->addDynamicRule( 23, people, false );

	sr->start( &spk, false );
	printf( "..stopped..\n");
	E_RELEASE(sr);


	E_RELEASE(e);	
	return r!=0;
}
