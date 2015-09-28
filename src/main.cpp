#include <tcp/Socket.h>
#include <tcp/SocketHandler.h>
#include <tcp/HTTPRequestFactory.h>
#include <list>
#include <tcp/SocketFactory.h>
#include <tcp/SocketListener.h>
#include <sys/signal.h>
#include <tcp/Socks4Manager.h>
template< typename T , typename P >
void operator-=( std::list< T > &out , P i )
{
	out.erase( i );
}
template< typename T >
void operator+=( std::list< T > &out , T const &i )
{
	out.push_back( i );
}
void sigpipeHndl( int sign )
{

}
int main( int argc , char *argv[] )
{
	signal( SIGPIPE , sigpipeHndl );
	SocketFactory factory( SocketListener( atoi( argv[ 1 ] ) ) );
	struct Pipe
	{
		SocketHandler in;
		SocketHandler out;
	};
	std::list< Pipe > pipes;
	while( true )
	{
		try
		{
			SocketHandler in( factory.timeout( 10 ).get() );
			Msg socks4_request;
			in >> socks4_request;
			//std::cout << "request sent: " + Socks4Manager::getInfo( socks4_request.getString() ) << "\n";
			SocketHandler out = Socks4Manager::getSocket( socks4_request );
			std::cout << "connection created:" << out.getInfo() << "\n";
			in << Socks4Manager::getSuccess();
			pipes += Pipe { in , out };
		} catch( TimeoutException const &e )
		{
		} catch( TCPException const &e )
		{
			std::cout << e.getMsg() << "\n";
		}
		all_over: iterate( p , pipes )
		{
			try
			{
				try
				{
					p->in.timeout( 500 ) >> p->out;
				} catch( TimeoutException const &e )
				{
				}
				try
				{
					p->out.timeout( 500 ) >> p->in;
				} catch( TimeoutException const &e )
				{
				}
			} catch( SocketCloseException const &e )
			{
				std::cout << e.getMsg() << "\n";
				pipes -= p;
				goto all_over;
			} catch( TCPException const &e )
			{
				std::cout << e.getMsg() << "\n";
				pipes -= p;
				goto all_over;
			}
		}
	}
	return 0;
}
