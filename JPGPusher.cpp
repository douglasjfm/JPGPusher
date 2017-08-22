/*
 * CV_JPGPusher.cpp
 *
 *  Created on: 12/08/2017
 *      Author: douglas
 */

#include "JPGPusher.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

std::function<uchar* (unsigned*)> nextFramePush;

JPGPusher::JPGPusher(std::function<uchar* (unsigned*)> nextFramePush_) {
	nextFramePush = nextFramePush_;
	server_id = time(NULL);
	clientscounter = 0;
}

JPGPusher::~JPGPusher() {
	// TODO Auto-generated destructor stub
}

void JPG_stream2Client(int clisock, states *state, int *frames)
{
	unsigned char *jpg;
	char cabhttp[500];
	char resp[600000];//This buffer must be adjusted for the size of the jpg frames
	unsigned jpglen, resplen;

	strcpy(cabhttp,"HTTP/1.0 200 OK\r\n");
	strcat(cabhttp,"Cache-Control: no-cache\r\n");
	strcat(cabhttp,"Pragma: no-cache\r\n");
	strcat(cabhttp,"Connection: close\r\n");
	strcat(cabhttp,"Content-Type: multipart/x-mixed-replace;boundary=--myboundary\r\n\r\n--myboundary\r\n");
	strcat(cabhttp,"Content-Type: image/jpeg\r\nContent-Length: ");

	jpg = nextFramePush(&jpglen);

	if (!jpg)
	{
		write(clisock , "HTTP/1.0 404 NotFound\r\n\r\n" , 25);
		close(clisock);
		return;
	}

	sprintf(resp, "%s%u\r\n\r\n",cabhttp, jpglen);

	resplen = strlen(resp);
	memcpy(resp+resplen,jpg,jpglen);
	sprintf(resp+resplen+jpglen,"\r\n\r\n");

	write(clisock , resp , resplen+jpglen+4);

	free(jpg);

	while((jpg = nextFramePush(&jpglen)) && (state[0] == STREAMING))
	{
		sprintf(resp, "--myboundary\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",jpglen);
		resplen = strlen(resp);
		memcpy(resp+resplen,jpg,jpglen);
		sprintf(resp+resplen+jpglen,"\r\n\r\n");

		write(clisock , resp , resplen+jpglen+4);

		free(jpg);
		frames[0]--;

		if(!frames[0])break;
	}

	state[0] = PAUSED;
	close(clisock);
	printf("Pausing client %d\n", clisock);
}

std::string _getRequest(char *client_get)
{
	//Parsing http request: 'GET /<string>'
	char s[16];
	client_get[20] = '\0';
	sscanf(client_get,"GET /%s",s);
	return s;
}

int JPGPusher::init(int porta)
{
    struct sockaddr_in server , client;
    int c;
    char client_get[500];
    //Create socket
    this->socket_server = socket(AF_INET , SOCK_STREAM , 0);
    if (this->socket_server == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( porta );

    //Bind
    if( bind(this->socket_server,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }

    //Listen
    listen(this->socket_server , 3);

    //Accept and incoming connection
    c = sizeof(struct sockaddr_in);

    this->online = true;

    //accept connection from an incoming client
    while(this->online)
    {
    	int sockcli, read_size;
    	std::string req;

    	sockcli = accept(this->socket_server, (struct sockaddr *)&client, (socklen_t*)&c);

        if (sockcli < 0)
        {
            perror("accept failed");
            return 1;
        }

        //Receive a message from client
		while( (read_size = recv(sockcli , client_get , 499 , 0)) > 0 )
		{
			client_get[read_size] = '\0';
			break;
		}

        req = _getRequest(client_get);

        if (req == "stream")
        	this->setNewSid(sockcli);
        else if(req.size() == 15)
        	this->handleSid(req, sockcli);
        else
        {
        	write(sockcli, "HTTP/1.0 404 NotFound\r\n\r\n",25);
        	close(sockcli);
        }
    }
    close(this->socket_server);
    return 0;
}

void JPGPusher::setNewSid(int sockcli)
{
	jpgsession *ssn;
	char sid[30];
	std::string response;

	this->checkSids();

	if (this->sids.size() > 15)
	{
		write(sockcli,"HTTP/1.0 403 Forbidden\r\n\r\n",26);
		close(sockcli);
		printf("No more clients accepted\n");
		return;
	}

	sprintf(sid,"%015li", server_id + (clientscounter++));

	ssn = (jpgsession*) malloc(sizeof(jpgsession));
	ssn->sockcli = sockcli;
	ssn->state = PAUSED;
	ssn->frames = 60;
	ssn->ts = time(NULL);

	this->sids[sid] = ssn;
	response = "HTTP/1.0 200 OK";
	response += "\r\nAccess-Control-Allow-Origin: *";
	response += "\r\nContent-Type: text/plain\r\nContent-Length: 15\r\n\r\n";
	response += sid;
	write(sockcli,response.c_str(),response.size());
	printf("New client created %s\n", sid);
}

void JPGPusher::checkSids()
{
	time_t t = time(NULL);
	std::map<std::string,jpgsession_ref>::iterator it;

	for (it = this->sids.begin(); it!=this->sids.end(); ++it)
	{
		std::string s = it->first;
	    if (this->sids[s]->state == PAUSED)
	    {
	    	if (t - this->sids[s]->ts > 120)//2min then delete old sid registers
	    	{
	    		printf("Deleting sid %s\n", s.c_str());
	    		free(this->sids[s]);
	    		this->sids.erase(s);
	    	}
	    }
	}
}

void JPGPusher::handleSid(std::string sid, int sockcli)
{
	jpgsession *ssn;
	std::map<std::string,jpgsession_ref>::iterator it;

	it = this->sids.find(sid);

	if(it != this->sids.end())//sid is valid
	{
		ssn = this->sids[sid];

		if (ssn->state == STREAMING && ssn->frames)//If streaming then continue streaming
		{
			ssn->frames = 60;
			write(sockcli,"HTTP/1.0 200 OK\r\n\r\n",19);
			close(sockcli);
			printf("Renew %s\n",sid.c_str());
			return;
		}
		//if not streaming then begin to
		ssn->sockcli = sockcli;
		ssn->state = STREAMING;
		std::thread worker(JPG_stream2Client, ssn->sockcli, &(ssn->state), &(ssn->frames));
		worker.detach();
		printf("Restream %s\n",sid.c_str());
	}
	else//invalid sid
	{
		write(sockcli, "HTTP/1.0 404 NotFound\r\n\r\n", 25);
	}
}


void JPGPusher::start(int port)
{
	JPGPusher *ref = this;
	std::thread service([port, ref]{
		ref->init(port);
	});
	service.detach();
}

void JPGPusher::finish()
{
	this->online = false;
	close(this->socket_server);
}
