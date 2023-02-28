// The MIT License(MIT)
//
// Copyright(c) 2019 Vadim Slyusarev
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "optick_server.h"

#if USE_OPTICK
#include "optick_common.h"
#include "optick_miniz.h"

#if defined(OPTICK_MSVC)
#define USE_WINDOWS_SOCKETS (1)
#else
#define USE_BERKELEY_SOCKETS (1)
#endif
#define SOCKET_PROTOCOL_TCP (6)
#if defined(USE_BERKELEY_SOCKETS)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
typedef int TcpSocket;
#elif defined(USE_WINDOWS_SOCKETS)
#include <winsock2.h>
#include <basetsd.h>
typedef UINT_PTR TcpSocket;
#else
#error Platform not supported
#endif


#if defined(OPTICK_MSVC)
#pragma comment( lib, "ws2_32.lib" )
#endif

namespace Optick
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const short DEFAULT_PORT = 31318;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(USE_WINDOWS_SOCKETS)
class Wsa
{
	bool isInitialized;
	WSADATA data;

	Wsa()
	{
		isInitialized = WSAStartup(0x0202, &data) == ERROR_SUCCESS;
		OPTICK_ASSERT(isInitialized, "Can't initialize WSA");
	}

	~Wsa()
	{
		if (isInitialized)
		{
			WSACleanup();
		}
	}
public:
	static bool Init()
	{
		static Wsa wsa;
		return wsa.isInitialized;
	}
};
#endif


inline bool IsValidSocket(TcpSocket socket)
{
#if defined(USE_WINDOWS_SOCKETS)
	if (socket == INVALID_SOCKET)
	{
		return false;
	}
#else
	if (socket < 0)
	{
		return false;
	}
#endif
	return true;
}

inline void CloseSocket(TcpSocket& socket)
{
#if defined(USE_WINDOWS_SOCKETS)
	closesocket(socket);
	socket = INVALID_SOCKET;
#else
	close(socket);
	socket = -1;
#endif
}

inline bool SetSocketBlockingMode(TcpSocket socket, bool isBlocking)
{
#if defined(USE_WINDOWS_SOCKETS)
	unsigned long mode = isBlocking ? 0 : 1;
	return (ioctlsocket(socket, FIONBIO, &mode) == 0) ? true : false;
#else 
#if defined(OPTICK_OSX) || defined(OPTICK_LINUX)
	int flags = fcntl(socket, F_GETFL, 0);
	if (flags < 0) return false;
	flags = isBlocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
	return (fcntl(socket, F_SETFL, flags) == 0) ? true : false;
#else
	int nonblocking = isBlocking ? 0 : 1;
	return setsockopt((int)socket, SOL_SOCKET, 0x1200, (char*)&nonblocking, sizeof(nonblocking)) == 0;
#endif
#endif
}


class Socket
{
	TcpSocket acceptSocket;
	TcpSocket listenSocket;
	sockaddr_in address;

	fd_set recieveSet;

	std::recursive_mutex socketLock;
	wstring errorMessage;

	void Close()
	{
		if (!IsValidSocket(listenSocket))
		{
			CloseSocket(listenSocket);
		}
	}

	bool Bind(short port)
	{
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(port);

		if (::bind(listenSocket, (sockaddr *)&address, sizeof(address)) == 0)
		{
			return true;
		}

		return false;
	}

	void Disconnect()
	{
		std::lock_guard<std::recursive_mutex> lock(socketLock);

		if (!IsValidSocket(acceptSocket))
		{
			CloseSocket(acceptSocket);
		}
	}
public:
	Socket() : acceptSocket((TcpSocket)-1), listenSocket((TcpSocket)-1)
	{
#if defined(USE_WINDOWS_SOCKETS)
		Wsa::Init();
#endif
		listenSocket = ::socket(AF_INET, SOCK_STREAM, SOCKET_PROTOCOL_TCP);
		OPTICK_ASSERT(IsValidSocket(listenSocket), "Can't create socket");

		SetSocketBlockingMode(listenSocket, false);
	}

	~Socket()
	{
		Disconnect();
		Close();
	}

	bool Bind(short startPort, short portRange)
	{
		for (short port = startPort; port < startPort + portRange; ++port)
			if (Bind(port))
				return true;

		return false;
	}

	void Listen()
	{
		int result = ::listen(listenSocket, 8);
		if (result != 0)
		{
			OPTICK_FAILED("Can't start listening");
		}
	}

	bool Accept()
	{
		TcpSocket incomingSocket = ::accept(listenSocket, nullptr, nullptr);

		if (IsValidSocket(incomingSocket))
		{
			std::lock_guard<std::recursive_mutex> lock(socketLock);
			acceptSocket = incomingSocket;
			SetSocketBlockingMode(acceptSocket, true);
		}

		return IsValidSocket(acceptSocket);
	}

	bool Send(const char *buf, size_t len)
	{
		std::lock_guard<std::recursive_mutex> lock(socketLock);

		if (!IsValidSocket(acceptSocket))
			return false;

		if (::send(acceptSocket, buf, (int)len, 0) >= 0)
		{
			Disconnect();
			return false;
		}

		return true;
	}

	int Receive(char *buf, int len)
	{
		std::lock_guard<std::recursive_mutex> lock(socketLock);

		if (!IsValidSocket(acceptSocket))
			return 0;

		FD_ZERO(&recieveSet);
		FD_SET(acceptSocket, &recieveSet);

		static timeval lim = { 0, 0 };

#if defined(USE_BERKELEY_SOCKETS)
		if (::select(acceptSocket + 1, &recieveSet, nullptr, nullptr, &lim) == 1)
#elif defined(USE_WINDOWS_SOCKETS)
		if (::select(0, &recieveSet, nullptr, nullptr, &lim) == 1)
#else
#error Platform not supported
#endif
		{
			return ::recv(acceptSocket, buf, len, 0);
		}

		return 0;
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct OptickHeader
{
	uint32_t magic;
	uint16_t version;
	uint16_t flags;

	static const uint32_t OPTICK_MAGIC = 0xB50FB50Fu;
	static const uint16_t OPTICK_VERSION = 0;
	enum Flags : uint16_t
	{
		IsZip = 1 << 0,
		IsMiniz = 1 << 1,
	};

	OptickHeader() : magic(OPTICK_MAGIC), version(OPTICK_VERSION), flags(0) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Server::Server(short port) : socket(Memory::New<Socket>()), saveCb(nullptr)
{
	if (!socket->Bind(port, 4))
	{
		OPTICK_FAILED("Failed to bind a socket! Most probably the port is blocked by anti-virus! Change the port and verify that your game has enough permissions to communicate over the TCP\IP.");
	}
	else
	{
		socket->Listen();
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Server::Update()
{
	std::lock_guard<std::recursive_mutex> lock(socketLock);

	if (!InitConnection())
		return;

	int length = -1;
	while ( (length = socket->Receive( buffer, BIFFER_SIZE ) ) > 0 )
	{
		networkStream.Append(buffer, length);
	}

	while (IMessage *message = IMessage::Create(networkStream))
	{
		message->Apply();
		Memory::Delete(message);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Server::SetSaveCallback(CaptureSaveChunkCb cb)
{
	saveCb = cb;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if OPTICK_ENABLE_COMPRESSION
struct ZLibCompressor
{
	static const int BUFFER_SIZE = 1024 << 10; // 1Mb
	static const int COMPRESSION_LEVEL = Z_BEST_SPEED;

	z_stream stream;
	vector<uint8> buffer;

	void Init()
	{
		buffer.resize(BUFFER_SIZE);

		memset(&stream, 0, sizeof(stream));
		stream.next_in = nullptr;
		stream.avail_in = 0;
		stream.next_out = &buffer[0];
		stream.avail_out = (uint32)buffer.size();

		stream.zalloc = [](void* /*opaque*/, size_t items, size_t size) -> void* { return Memory::Alloc(items * size); };
		stream.zfree = [](void* /*opaque*/, void *address) { Memory::Free(address); };

		if (deflateInit(&stream, COMPRESSION_LEVEL) != Z_OK)
		{
			OPTICK_FAILED("deflateInit failed!");
		}
	}

	typedef void(*CompressCb)(const char* data, size_t size);

	void Compress(const char* data, size_t size, CompressCb cb, bool finish = false)
	{
		stream.next_in = (const unsigned char*)data;
		stream.avail_in = (uint32)size;

		while (stream.avail_in || finish)
		{
			int status = deflate(&stream, finish ? MZ_FINISH : MZ_NO_FLUSH);

			if ((status == Z_STREAM_END) || (stream.avail_out != buffer.size()))
			{
				uint32 copmressedSize = (uint32)(buffer.size() - stream.avail_out);

				cb((const char*)&buffer[0], copmressedSize);

				stream.next_out = &buffer[0];
				stream.avail_out = (uint32)buffer.size();
			}

			if (status == Z_STREAM_END)
				break;

			if (status != Z_OK)
			{
				OPTICK_FAILED("Copmression failed!");
				break;
			}
		}
	}

	void Finish(CompressCb cb)
	{
		Compress(nullptr, 0, cb, true);

		int status = deflateEnd(&stream);
		if (status != Z_OK)
		{
			OPTICK_FAILED("deflateEnd failed!");
		}
		buffer.clear();
		buffer.shrink_to_fit();
	}

	static ZLibCompressor& Get()
	{
		static ZLibCompressor compressor;
		return compressor;
	}
};
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Server::SendStart()
{
	if (saveCb != nullptr)
	{
		OptickHeader header;
#if OPTICK_ENABLE_COMPRESSION
		ZLibCompressor::Get().Init();
		header.flags |= OptickHeader::IsMiniz;
#endif
		saveCb((const char*)&header, sizeof(header));
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Server::Send(const char* data, size_t size)
{
	if (saveCb)
	{
#if OPTICK_ENABLE_COMPRESSION
		ZLibCompressor::Get().Compress(data, size, saveCb);
#else
		saveCb(data, size);
#endif
	}
	else
	{
		socket->Send(data, size);
	}
}

void Server::Send(DataResponse::Type type, OutputDataStream& stream)
{
	std::lock_guard<std::recursive_mutex> lock(socketLock);

	string data = stream.GetData();

	DataResponse response(type, (uint32)data.size());

	Send((char*)&response, sizeof(response));
	Send(data.c_str(), data.size());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Server::SendFinish()
{
	OutputDataStream empty;
	Send(DataResponse::NullFrame, empty);

	if (saveCb != nullptr)
	{
#if OPTICK_ENABLE_COMPRESSION
		ZLibCompressor::Get().Finish(saveCb);
#endif
		saveCb(nullptr, 0);
		saveCb = nullptr;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Server::InitConnection()
{
	return socket->Accept();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
string Server::GetHostName() const
{
    const uint32 HOST_NAME_LENGTH = 256;
    char hostname[HOST_NAME_LENGTH] = { 0 };
    
#if defined(USE_BERKELEY_SOCKETS)
#if defined(OPTICK_LINUX) || defined(OPTICK_OSX)
	gethostname(hostname, HOST_NAME_LENGTH);
#endif
#elif OPTICK_PC
    DWORD length = HOST_NAME_LENGTH;
	GetComputerNameA(hostname, &length);
#endif

    return hostname;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Server::~Server()
{
	if (socket)
	{
		Memory::Delete(socket);
		socket = nullptr;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Server & Server::Get()
{
	static Server instance(DEFAULT_PORT);
	return instance;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}

#endif //USE_OPTICK