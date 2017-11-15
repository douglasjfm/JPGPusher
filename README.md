# Simple C++ HTTP Pusher server

HTTP push server implentation for MJPG.

## Usage

This sample show how to stream

```c++
#include "JPGPusher.h"

int main(void)
{
	JPGPusher server_jpg(frame_provider_function);
	server_jpg.start(12340);
	//...
	servjpg.finish();
	return 0;
}
```
The JPGPusher class takes a function for providing the frames. frame_provider_function must be of the form:

```c++
unsigned char* frame_provider_function(unsigned int *size)
```

Where size must retrieve the size os the jpg frame and the return pointer is the frame data itself. At this example the server runs at port 12340. IMPORTANT: this function must also keep the logic for the stream FPS.

## Example using OPENCV

This example show how to stream the webcam using JPGPusher:

```c++
#include <string.h> // memcpy
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "JPGPusher.h"

cv::Mat cv_frame;

uchar* frame_provider_function(unsigned *size)
{
	std::vector<unsigned char> buffer;
	std::vector<int> jpgparams;
	unsigned char* ret;
	jpgparams.push_back(CV_IMWRITE_JPEG_QUALITY);
	jpgparams.push_back(50);//sets jpg quality.
	cv::imencode(".jpg", cv_frame, buffer, jpgparams);
	size[0] = buffer.size();
	ret = (unsigned char*) malloc(size[0]);
	memcpy(ret, buffer.data(), size[0]);
	buffer.clear();
	jpgparams.clear();
	cv::waitKey(67);//FPS 15
	return ret; //JPGPusher will free after.
}

int main(void)
{
	JPGPusher server_jpg(frame_provider_function);
	bool run = true;
	char c;
	cv::VideoCapture cap(0);

	server_jpg.start(12340);

	while (run)
	{
		cap >> cv_frame;
		cv::imshow("cv",cv_frame);
		c = cv::waitKey(1);
		run = c != 27;//Stop if ESC pressed
	}
	server_jpg.finish();
	return 0;
}
```

The client would only need to set a HTML img tag for see the stream:

```html
<img src="http://localhost:12340/stream"/>
```

But a bit more of logic must be added in order to the server "knows" when to stop streaming. The following is a complete example for a front end client:

```html
<!DOCTYPE html>
<html>
<head>
<script src="https://ajax.aspnetcdn.com/ajax/jQuery/jquery-3.2.1.min.js"></script>
<meta charset="UTF-8">
<title>Test MJPG</title>
<script type="text/javascript">
function keep_playing(url)
{
	setInterval(function(){
		var oReq = new XMLHttpRequest();
		oReq.open("get", url, true);
		oReq.send();
	}, 1800); //The interval should be less than 60 * 1/FPS, i.e. the time of 60 frames.
}

function init()
{
	$('#screen').load("http://localhost:12340/stream", function(response, status, xhr){
		$('#screen').attr('src',"http://localhost:12340/" + response);// Response is the Seesion ID
		keep_playing("http://localhost:12340/"+response);
	});
}
</script>
</head>
<body onload="init()">
<img id="screen"/>
</body>
</html> 
```

Here the client starts the streaming request by hitting the /stream url int the JPGPusher server, then JPGPusher will respond with a session id number. At this very point, the streaming hadn't started yet, but the client must keep a regular interval of hitting the url /<session_id> (currently should be less than the time spent for 60 frames).

This front end is already tested in firefox and chrome.

## Dependencies

The code only depends on c++11 and standard posix socket libs.

## ACK

This project uses "jpge.cpp - C++ class for JPEG compression" provided here https://github.com/richgel999/jpeg-compressor. Thanks to @richgel1999.
