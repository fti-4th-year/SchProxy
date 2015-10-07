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
	out.remove( i );
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
		int i;
		SocketHandler in;
		SocketHandler out;
		bool operator==( Pipe const &p ) const
		{
			return p.i == i;
		}
	};
	Waiter waiter;
	bool working = true;
	waiter.pushSleepy( factory.getSocketListener( ) , [ &waiter , &factory , &working ]()
	{
		try
		{
			SocketHandler in( factory.get( ) );
			Msg socks4_request;
			in >> socks4_request;
			std::cout << "request sent: " + Socks4Manager::getInfo( socks4_request.getString() ) << "\n";
			SocketHandler out = Socks4Manager::getSocket( socks4_request );
			std::cout << "connection created:" << out.getInfo( ) << "\n";
			in << Socks4Manager::getSuccess( );
			waiter.pushSleepy( in.getSocket() , [&waiter , in , out]()
					{
						try
						{
							try
							{
								in.timeout( 100 ) >> out;
								//std::cout << "in woke up" << "\n";
							} catch( TimeoutException const &e )
							{

							}
						} catch( std::exception const &e )
						{
							std::cerr << "in socket:" << e.what( ) << "\n";
							waiter.removeSleepy( in.getSocket() );
							waiter.removeSleepy( out.getSocket() );
						}
					}
			);
			waiter.pushSleepy( out.getSocket() , [&waiter , in , out]()
					{
						try
						{
							try
							{
								out.timeout( 100 ) >> in;
								//std::cout << "out woke up" << "\n";
							} catch( TimeoutException const &e )
							{

							}
						} catch( std::exception const &e )
						{
							std::cerr << "out socket:" << e.what( ) << "\n";
							waiter.removeSleepy( in.getSocket() );
							waiter.removeSleepy( out.getSocket() );
						}
					}
			);
		} catch( std::exception const &e )
		{
			std::cerr << "main catcher:" << e.what() << "\n";
			//working = false;
		}
	} );
	while( working )
	{
		try
		{
			waiter.wait( 100 );
		} catch( std::exception const &e )
		{
			std::cerr << "waiter catcher:" << e.what( ) << "\n";
			working = false;
		}
	}
	return 0;
}
