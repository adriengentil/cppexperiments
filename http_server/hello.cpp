#include <algorithm>
#include <fstream>
#include <iostream>

#include <cstdio>
#include <cstring>
#include <sys/time.h>                // for gettimeofday()
#include "mongoose.h"

#include <boost/iostreams/filter/counter.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/utility/string_ref.hpp> 
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
namespace fs = boost::filesystem;

static const char *JSON = "application/json";
static const char *JAVASCRIPT = "application/javascript";
static const char *HTML = "text/html";

static const char *OK = "200 OK";
static const char *NOTFOUND = "404 Not Found";

#define CONTENT_SIZE 5 * 1024 * 1024

// This function will be called by mongoose on every new request.
int begin_request_handler(struct mg_connection *conn) {

  timeval t1, t2;
  double elapsedTime;
  // start timer
  gettimeofday(&t1, NULL);

  const struct mg_request_info *request_info = mg_get_request_info(conn);
  char content[CONTENT_SIZE];

  int content_length = 0;
  boost::string_ref contentType(HTML);
  boost::string_ref status(NOTFOUND);
  
//  memset(content, 0, 4 * 1024 * 1024);

  boost::string_ref url(request_info->uri);
  std::cout << "URL: " << url << std::endl;

  if (url.rfind("static") != boost::string_ref::npos)
  {
    boost::string_ref::size_type lastSlash = url.rfind('/');
    std::cout << "file: " << url.substr(lastSlash + 1).data() << std::endl;
    fs::path filename(url.substr(lastSlash + 1).data());
    if (fs::exists(filename))
    {
      std::cout << "Reading file: " << filename.filename() << std::endl;

      fs::ifstream src(filename, std::ios_base::in | std::ios_base::binary);
      //std::istreambuf_iterator<char> begin_source(src);
      //std::istreambuf_iterator<char> end_source;
      //std::copy(begin_source, end_source, content);

      boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
      out.push(boost::iostreams::gzip_compressor());
      out.push(boost::iostreams::counter());
      out.push(boost::iostreams::array_sink(content, CONTENT_SIZE));
      content_length = boost::iostreams::copy(src, out);

      if (filename.extension().filename() == ".js")
      {
        contentType = JAVASCRIPT;
      }

      content_length = out.component<boost::iostreams::counter>(1)->characters();
      status = OK;
    }
  }

  // Send HTTP reply to the client
  mg_printf(conn,
            "HTTP/1.1 %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %d\r\n"        // Always set Content-Length
            "Content-Encoding: gzip\r\n"
            "\r\n",
            status.data(),
            contentType.data(),
            content_length);

  mg_write(conn, content, content_length);

  // stop timer
  gettimeofday(&t2, NULL);

  // compute and print the elapsed time in millisec
  elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
  elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
  std::cout << elapsedTime << " ms." << std::endl;

  // Returning non-zero tells mongoose that our function has replied to
  // the client, and mongoose should not send client any more data.
  return 1;
}

int main(void) {
  struct mg_context *ctx;
  struct mg_callbacks callbacks;

  // List of options. Last element must be NULL.
  const char *options[] = {"listening_ports", "8080", NULL};

  // Prepare callbacks structure. We have only one callback, the rest are NULL.
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.begin_request = begin_request_handler;

  // Start the web server.
  ctx = mg_start(&callbacks, NULL, options);

  // Wait until user hits "enter". Server is running in separate thread.
  // Navigating to http://localhost:8080 will invoke begin_request_handler().
  getchar();

  // Stop the server.
  mg_stop(ctx);

  return 0;
}
