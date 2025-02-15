# Simple C++ HTTP Server Push

HTTP Server push implentation for MJPG (jpeg streaming).

## Usage

The following code sample shows how to stream:

```c++
#include "JPGPusher.h"

int main(void)
{
	JPGPusher server_jpg(frame_provider_function);
	server_jpg.start(12340); // start http streaming on port 12340
	//...
	servjpg.finish(); // finish server
	return 0;
}
```
The JPGPusher class takes a function for providing the frames. frame_provider_function must be of the form:

```c++
unsigned char* frame_provider_function(unsigned int *size)
```

Where size must retrieve the size os the jpg frame and the return pointer is the frame data itself. At this example the server runs at port 12340. IMPORTANT: this function is blocking, so it keeps the logic for the stream FPS.

## Sample code using OPENCV

This example shows how to stream the webcam over http using JPGPusher:

```c++
#include <string.h> // memcpy
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "JPGPusher.h"

cv::Mat cv_frame;

/*
* Description: Will provide the jpeg encoded frames
*              opencv will be storing on the global Mat cv_frame;
* Output:      size - For storing the frame data length (in bytes)
* Return:      The function will return a pointer to the current jpg encoded frame data
*/
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
	cv::waitKey(67); // 15 frames per second
	return ret; //JPGPusher will free after.
}

int main(void)
{
	JPGPusher server_jpg(frame_provider_function);
	bool runnig = true;
	char c;
	cv::VideoCapture cap(0);

	server_jpg.start(12340);

	while (runnig)
	{
		cap >> cv_frame;
		cv::imshow("cv",cv_frame);
		c = cv::waitKey(1);
		runnig = c != 27;//Stop if ESC pressed
	}
	server_jpg.finish();
	return 0;
}
```

The client can set a HTML img tag to see the stream:

```html
<img src="http://localhost:12340/stream"/>
```

A bit more of logic must be added in order to the server "knows" when to stop streaming. The following is a complete example for a front end client:

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
