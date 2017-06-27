# tinyco

[![Build Status](https://travis-ci.org/daoluan/tinyco.svg?branch=master)](https://travis-ci.org/daoluan/tinyco)

Coroutine implemented in ucontext syscall.

tinyco is a framework with high performance and high efficiency focusing on business with which you can build a server in only couple lines of code. It has advantages as follows:

- Make full use of cpu and process requests concurrently which makes server's throughput greatly improve
- Perfect http/dns components
- Small  kernel in size. Only need to link to small static library when using
- Business code can be easy to read and maintain. No more async framework

## How to start?
See [test](https://github.com/daoluan/tinyco/tree/master/test).

