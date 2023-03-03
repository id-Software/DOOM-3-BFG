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

#pragma once
#include "optick.config.h"

#if USE_OPTICK
#include "optick_message.h"

#include <mutex>
#include <thread>

namespace Optick
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Socket;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Server
{
	InputDataStream networkStream;

	static const int BIFFER_SIZE = 1024;
	char buffer[BIFFER_SIZE];

	Socket* socket;

	std::recursive_mutex socketLock;

	CaptureSaveChunkCb saveCb;

	Server( short port );
	~Server();

	bool InitConnection();

	void Send(const char* data, size_t size);

public:
	void SetSaveCallback(CaptureSaveChunkCb cb);

	void SendStart();
	void Send(DataResponse::Type type, OutputDataStream& stream);
	void SendFinish();

	void Update();

	string GetHostName() const;

	static Server &Get();
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

#endif //USE_OPTICK