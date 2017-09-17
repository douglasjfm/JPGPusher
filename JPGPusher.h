//============================================================================
// Name        : JPGPusher.h
// Author      : douglasjfm
// Version     : 1.0
// Copyright   : Free
// Description : HTTP Push Server for Motion JPEG streaming.
//============================================================================

#ifndef JPGPUSHER_H_
#define JPGPUSHER_H_
#include <time.h>
#include <string.h>
#include <functional>
#include <string>
#include <thread>
#include <map>

typedef unsigned char uchar;

enum states{
	PAUSED,
	STREAMING,
	WAIT
};

typedef struct jpgsession
{
	int sockcli;
	states state;
	std::thread worker;
	time_t ts;
	int frames;
}jpgsession;

typedef jpgsession* jpgsession_ref;

class JPGPusher {
public:

	JPGPusher(std::function<uchar* (unsigned*)> nextFramePush_);
	virtual ~JPGPusher();
	int init(int porta);
	void start(int port);
	void finish();

private:
	time_t server_id;
	unsigned clientscounter;
	int socket_server;
	bool online;
	std::map<std::string,jpgsession_ref> sids;//Session id's

	void handleSid(std::string sid, int sockcli);
	void setNewSid(int sockcli);
	void checkSids();
};

#endif /* JPGPUSHER_H_ */
