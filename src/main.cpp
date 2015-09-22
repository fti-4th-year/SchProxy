#include <tcp/Socket.h>
#include <tcp/SocketHandler.h>
#include <tcp/HTTPRequestFactory.h>
int main( int argc , char *argv[] )
{
	SocketHandler s( Socket( "httpbin.org" , 80 ) );
	s << HTTPRequestFactory::simpleGET( "httpbin.org" , "/ip" );
	std::cout << s.timeout( 1000 ) << "\n";
	return 0;
}
