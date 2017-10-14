### File structure like this: 

```
├── compile.sh
├── include
│   ├── amessage.h
│   ├── init.h
│   ├── poll_wc.h
│   ├── post_wr.h
│   ├── queue.h
│   ├── rdma_socket.h
│   └── receiver.h
├── README.md
├── src
│   ├── amessage.cpp
│   ├── init.cpp
│   ├── poll_wc.cpp
│   ├── post_wr.cpp
│   ├── queue.cpp
│   ├── rdma_socket.cpp
│   └── receiver.cpp
└── test
	├── client.cpp
	├── server.cpp
	├── socket_client.cpp
	└── socket_server.cpp
```

#### In this project, it include a implement of a series of simple RDMA style api which can be used to message transmission similarly like TCP/IP socket api. In rdma_socket.h you can see these c-style function. And in an implement of a simple message queue, in receiver.h .

