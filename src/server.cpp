#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include <exception>
#include <string>
#include <list>
#include <sys/select.h>
#include <sys/types.h>
#include <stdio.h>
#include <sstream>
#include <iostream>


typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned short ushort;
class ProxyServer
{
private:
	class ProxyPipe
	{
	private:
		int _in_socket , _out_socket = -1;
		bool _inited = false;
		ushort _port = 0 , _src_port;
		uint _ip = 0 , _src_ip;
		std::string _user_id;
	public:
		ProxyPipe( int in_socket , ushort src_port , uint src_ip ) :
				_src_port( src_port ), _src_ip( src_ip ), _in_socket( in_socket )
		{
		}
		std::string getIP( uint sip ) const
		{
			byte *ip = ( byte* )&sip;
			std::ostringstream ss;
			ss << ( int )ip[ 0 ] << "." << ( int )ip[ 1 ] << "." << ( int )ip[ 2 ] << "." << ( int )ip[ 3 ];
			return ss.str();
		}
		std::string getParam() const
		{
			std::ostringstream ss;

			byte *port = ( byte* )&_port;
			byte *src_port = ( byte* )&_src_port;
			ss << getIP( _src_ip ) << ":" << ( int )src_port[ 0 ] << ( int )src_port[ 1 ] << "->" << getIP( _ip ) << ":" << ( int )port[ 0 ] << ( int )port[ 1 ];
			return ss.str();
		}
		void doWork()
		{
			/*auto update_connection = [this]()
			{
				if( !_inited )
				return;
				/*int error = 1;
				 {
				 socklen_t len = sizeof (error);
				 getsockopt (_in_socket, SOL_SOCKET, SO_ERROR, &error, &len);
				 if( error != 0 )
				 {
				 throw ServerException( "ProxyPipe" , "connection closed" );
				 }
				 }
				 error = 1;
				 if( _out_socket > 0 )
				 {
				 socklen_t len = sizeof (error);
				 getsockopt (_out_socket, SOL_SOCKET, SO_ERROR, &error, &len);
				 }
				 if( error != 0 )
				{
					if( _out_socket > 0 ) close( _out_socket );
					ERR_HNDL( _out_socket = socket( AF_INET , SOCK_STREAM , 0 ) , "ProxyPipe" , "out_socket creation" );
					sockaddr_in server;
					server.sin_family = AF_INET;
					server.sin_addr.s_addr = _ip; //inet_addr( getIP().c_str() );
					server.sin_port = _port;
					//std::cout << "out_socked connecting to " + getParam() + "...\n";
					ERR_HNDL( connect( _out_socket , ( sockaddr * )&server , sizeof( server ) ) , "ProxyPipe" , "out_socket connect" );
					//std::cout << "out_socked created successfully\n";
				}
			};*/

			fd_set readfds;
			byte buf[ 0x100 ];
			select:
			FD_ZERO( &readfds );
			FD_SET( _in_socket , &readfds );
			int max = _in_socket + 1;
			if( _out_socket > 0 )
			{
				FD_SET( _out_socket , &readfds );
				max = std::max( _in_socket , _out_socket ) + 1;
			}
			timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 1e4;
			int rv;
			ERR_HNDL( rv = select( max , &readfds , NULL , NULL , &tv ) , "ProxyPipe" , "select" );
			if( rv > 0 )
			{
				if( _inited )
				{
					if( FD_ISSET( _in_socket , &readfds ) )
					{
						int c;
						ERR_HNDL( c = recv( _in_socket , buf , sizeof( buf ) , 0 ) , "ProxyPipe" , "in_socket recieve" );
						//if( c < 2 ) return;
						//ERR_HNDL( send( _in_socket , buf , c , 0 ) , "ProxyPipe" , "in_socket send" );
						/*if( c == 0 )
						 {
						 throw ServerException( "ProxyPipe" , getParam() + " in_socket closed" );
						 }*/
						int i = 0;
						try_send: if( _out_socket < 0 || send( _out_socket , buf , c , 0 ) < 1 )
						{
							if( i < 4 )
							{
								i++;
								//if( _out_socket > 0 ) close( _out_socket );
								ERR_HNDL( _out_socket = socket( AF_INET , SOCK_STREAM , 0 ) , "ProxyPipe" , "out_socket creation" );
								sockaddr_in server;
								server.sin_family = AF_INET;
								server.sin_addr.s_addr = _ip;
								server.sin_port = _port;
								std::cout << "out_socket connecting...\n";
								ERR_HNDL( connect( _out_socket , ( sockaddr * )&server , sizeof( server ) ) , "ProxyPipe" , "out_socket connect" );
								std::cout << "out_socket created\n";
								usleep( 1e4 );
								goto try_send;
							} else
							{
								std::cout << "out_socket unaccessable\n";
								throw ServerException( "ProxyPipe" , "out_socket unaccessable" );
							}
						}
						//std::cout << "in socket send:" << buf << "\n";
						//goto select;*/
					}
					if( FD_ISSET( _out_socket , &readfds ) )
					{
						int c;
						ERR_HNDL( c = recv( _out_socket , buf , sizeof( buf ) , 0 ) , "ProxyPipe" , "out_socket recieve" );
						if( c < 2 )
						{
							close( _out_socket );
							_out_socket = -1;
							return;
						}
						ERR_HNDL( send( _in_socket , buf , c , 0 ) , "ProxyPipe" , "in_socket send" );
						//std::cout << "out socket send:" << buf << "\n";
						/*if( c == 0 )
						 {
						 close( _out_socket );
						 return;
						 }*/
					}

				} else
				{
					int c;
					ERR_HNDL( c = recv( _in_socket , buf , sizeof( buf ) , 0 ) , "ProxyPipe" , "in_socket recieve inside init" );
					if( c == 0 )
					{
						throw ServerException( "ProxyPipe" , "fail at recieving init info" );
					}
					if( c < 9 || buf[ 0 ] != 0x04 || buf[ 1 ] != 0x01 ) // version command
					{
						throw ServerException( "ProxyPipe" , "wrong info" );
					}
					_port = *( ( ushort* )buf + 1 );

					_ip = 0;
					//for( int i = 0; i < 4; i++ )
					//	_ip |= ( ( uint )buf[ 4 + i ] ) << ( i * 8 );

					_ip = ( ( uint * )( buf + 4 ) )[ 0 ];
					std::cout << _port << " " << ( ( uint * )( buf + 4 ) )[ 0 ] << "\n";
					_user_id = std::string( ( char* )buf + 8 );
					int i = 0;
					buf[ i++ ] = 0;
					buf[ i++ ] = 0x5b;
					buf[ i++ ] = 0;
					buf[ i++ ] = 0;
					buf[ i++ ] = 0;
					buf[ i++ ] = 0;
					buf[ i++ ] = 0;
					buf[ i++ ] = 0;
					ERR_HNDL( send( _in_socket , buf , 8 , 0 ) , "ProxyPipe" , "in_socket  send  inside init" );
					_inited = true;
					std::cout << "connection established " + getParam() + "\n";
				}
			}
		}
		void release()
		{
			close( _out_socket );
			close( _in_socket );
		}
	};
	ushort _port;
	int _listen_socket = -1;
	bool _inited = false;
	std::list< ProxyPipe > _pipes;
public:
	ProxyServer( ushort port ) :
			_port( port )
	{
	}
	void init()
	{
		ERR_HNDL( _listen_socket = socket( AF_INET , SOCK_STREAM , 0 ) , "ProxyServer" , "socket creation" );
		sockaddr_in _server;
		_server.sin_family = AF_INET;
		_server.sin_addr.s_addr = INADDR_ANY;
		_server.sin_port = htons( _port );
		ERR_HNDL( bind( _listen_socket , ( sockaddr * )&_server , sizeof( _server ) ) , "ProxyServer" , "bind" );
		ERR_HNDL( listen( _listen_socket , 3 ) , "ProxyServer" , "set listen" );
		std::cout << "server created at port:" << _port << "\n";
		_inited = true;
		return;
	}
	void run()
	{
		if( !_inited ) return;
		while( true )
		{
			int rv;
			fd_set readfds;
			FD_ZERO( &readfds );
			FD_SET( _listen_socket , &readfds );
			FD_SET( 0 , &readfds );
			timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 1e4;
			ERR_HNDL( rv = select( _listen_socket + 1 , &readfds , NULL , NULL , &tv ) , "ProxyServer" , "select" );
			if( rv > 0 )
			{
				if( FD_ISSET( 0 , &readfds ) )
				{
					std::string msg = "";
					std::cin >> msg;
					if( msg == "exit" ) return;
					else if( msg == "count" )
					{
						std::cout << _pipes.size() << "\n";
					} else
						std::cout << "no command " << msg << "\n";
				}
				if( FD_ISSET( _listen_socket , &readfds ) )
				{
					sockaddr_in _client;
					int c = sizeof(sockaddr_in);
					int client_sock;
					ERR_HNDL( client_sock = accept( _listen_socket , ( sockaddr * )&_client , ( socklen_t* )&c ) , "ProxyServer" , "accept" );
					_pipes.push_back( ProxyPipe( client_sock , _client.sin_port , _client.sin_addr.s_addr ) );
				}

			} else
			{
				start: for( auto p = _pipes.begin(); p != _pipes.end(); p++ )
				{

					try
					{
						p->doWork();
					} catch( ServerException const &e )
					{
						auto msg = e.getMsg();
						std::cout << msg << "\n";
						//if( msg.find( "in_socket" ) != std::string::npos )
						{
							std::cerr << p->getParam() << " has closed\n";
							p->release();
							_pipes.erase( p );
						}
						goto start;
					}
				}
			}
		}
	}
	void release()
	{
		for( auto &p : _pipes )
		{
			p.release();
		}
	}
};
int main( int argc , char *argv[] )
{
	ProxyServer ps( atoi(argv[1]) );
	try
	{
		ps.init();
		ps.run();
	} catch( ServerException const &e )
	{
		std::cout << e.getMsg() << "\n";
	}
	ps.release();
	std::cout << "server exit\n";
	return 0;
}
