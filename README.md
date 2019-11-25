# xdev-http-server

> HTTP server based on Boost.Beast library

## Simple example

```cpp
using namespace xdev;

boost::asio::io_context ctx;
using server_type = net::http::plain_server;
server_type srv{ctx, {net::ip::address_v4::loopback(), 4242}};

// returns current file
srv.on("/get_this_file").complete([](server_type::context_type& context) {
    net::error_code ec;
    auto resp = context.make_response<net::http::file_body>();
    net::http::file_body::value_type file;
    file.open(__FILE__, boost::beast::file_mode::read, ec);
    if (ec)
        throw std::runtime_error(ec.message());
    resp.body() = std::move(file);
    return resp;
});

// adds given route parameters
srv.on("/add/<a>/<b>").complete([](double a, double b, server_type::context_type& context) {
    auto response = context.make_response();
    response.body() = std::to_string(a + b);
    response.set(net::http::field::content_type, "text/plain");
    response.content_length(response.body().size());
    return response;
});

ctx.run();

```

## Development

## Building

```bash
mkdir build && cd build
conan install .. -s compiler.cppstd=20
cmake -DCMAKE_TOOLCHAIN_FILE=./conan_paths.cmake ..
make
```

## For usage

> Please note that c++20 is required
This implies using -std=c++20

```bash
conan create . -s compiler.cppstd=20
```
