#include <tcp/Socket.h>
#include <tcp/SocketHandler.h>
#include <tcp/HTTPRequestFactory.h>
#include <list>
#include <tcp/SocketFactory.h>
#include <tcp/SocketListener.h>
template< typename T , typename P >
void operator-=( std::list< T > &out , P i )
{
	out.erase( i );
}
int main( int argc , char *argv[] )
{
	/*SocketHandler s( Socket( "httpbin.org" , 80 ) );
	 s << HTTPRequestFactory::simpleGET( "httpbin.org" , "/ip" );
	 std::cout << s.timeout( 1000 ) << "\n";*/
	SocketFactory factory( SocketListener( 8080 ) );
	std::list< SocketHandler > sockets;
	while( true )
	{
		maybe( sockets += factory.timeout( 100 ); );
		all_over: iterate( p , sockets )
		{
			try
			{
				maybe( std::cout << p->timeout( 100 ) << "\n"; *p << "test"; );
			} catch( std::exception const &e )
			{
				//std::cout << sockets.size() << e.what() << "\n";
				sockets -= p;
				goto all_over;
			}
		}
	}
	return 0;
}
