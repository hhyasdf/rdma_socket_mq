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



#### RDMA ib verbs 接口分析 ####

RDMA使用基于事件的通信机制，面向连接并且保留消息边界（因为相当于直接进行内存拷贝）。

RDMA的每一个连接是通过一个 event channel 来管理的（下面我们简称 ec）。**rdma_create_event_channel()** 创建一个 ec。**rdma_create_id()** 根据 ec 创建一个 rdma_cm_id（下面我们简称 id），每个 id 对应一个连接（与 POSIX socket 中的文件描述符类似）。接下来的步骤分别以 Sever 和 Client 进行说明。

对于 Sever，我们首先需要使用 **rdma_bind_addr()** 为一个 id 绑定一个 sockaddr。然后使用 **rdma_get_cm_event()** 监听 ec 中的关于连接管理的事件（我们可以认为 ec 本质上是一个 event queue）。每个事件都是一个 rdma_cm_event 结构，（见 RDMA_CM Events ），每个事件的 id 对应了事件关联的连接；每个事件需要用 **rdma_ack_cm_event()** 进行释放并从 event queue 中移除，所以我们一般使用一个拷贝的对象来处理事件。每个事件（下面我们称其为 event）的 event 属性表示了事件的类型:

* 当接收到 event->event == **RDMA_CM_EVENT_CONNECT_REQUEST** 时表示接收到连接请求，此时我们需要初始化一个 queue pair（下面简称为 qp） 。（每个 id 都对应一个连接，如果事件是 RDMA_CM_EVENT_CONNECT_REQUEST，返回的 event 中的 id 是新的 id，否则与原来的 id 相同，默认与原来的 ec 绑定）首先我们以接收到的 event->id->verbs 为参数使用 **ibv_alloc_pd()** 创建一个 protection domain（下面简称 pd） 和 **ibv_create_comp_channel()** 创建一个 completion channel（简称为 cc，一个用来监听cq是否被放入了一个事件的机制） ，并 调用 **ibv_create_cq()** 用 verbs 和 cc（可选，也可以不使用cc监听）创建一个 complete queue（简称为 cq） ，每一个 cc 对应一个 cq ，每一个 qp 中存在一个 send_cq 和 recv_cq（通过设置 qp_attr ），两者可以对应同一个 cq 也可以分别对应不同的 cq （如果对应同一个 cq ，其中就可能会同时存在 send 事件和 recv 事件）。**ibv_req_notify_cq()** 用来开启 cq 的消息提示机制。之后用收到的 event->id 和设置好的 pd、qp_attr 调用 rdma_create_qp() 创建一个 qp，qp 对应的是该连接的 id，需要保存下来。最后调用 **rdma_accept()** 建立连接。
* 收到 event->event == **RDMA_CM_EVENT_ESTABLISHED** 表示连接建立成功。
* 断开连接时，调用 **rdma_disconnect()** 会在连接的两端生成 **RDMA_CM_EVENT_DISCONNECTED** 消息。然后需要释放动态分配的内存空间 **rdma_destroy_id()** 。

对于 Client，我们需要调用 **rdma_resolve_addr()** 将目标地址（addrinfo 结构中的 ai_addr 作为参数）和可选的源地址解析成RDMA地址，当解析完成时，会在 eq 中生成一个 **RDMA_CM_EVENT_ADDR_RESOLVED** 事件。然后同样使用 **rdma_get_cm_event()** 监听 ec 中的关于连接管理的事件：

* 在接收到 **RDMA_CM_EVENT_ADDR_RESOLVED** 事件后同样初始化 qp 和注册内存。然后调用 **rdma_resolve_route()**。(返回的 event 的 id 属性与原 ec 的 id 相同)
* 当收到 **RDMA_CM_EVENT_ROUTE_RESOLVED** 事件之后调用 **rdma_connect()** 发送连接请求，参数为收到的 **event->id** 和 可选的连接参数结构体 **rdma_conn_param** 。
* 收到 **RDMA_CM_EVENT_ESTABLISHED** 事件后请求建立成功。
* 同样可以调用 **rdma_disconnect()** 中断连接。

对于 RDMA 中消息的收发，每一个消息的 recieve/send/read/write 操作都需要调用 **ibv_post_send()**/**ibv_post_recv()** 或 **rdma_post_recv()**/**rdma_post_send()**/**rdma_post_read()**/**rdma_post_write()** 或 **rdma_post_recvv**/**rdma_post_sendv**/**rdma_post_readv**/**rdma_post_writev** （加v的函数可以一次将几个buffer一起发送）向连接对应的本端 qp 中注册一个操作（work request），操作完成后会在对应的 cq 中生成一个 cqe（complete queue event）。每一次都需要调用 **ibv_req_notify_cq()** 来启用 cq 的事件提醒功能。而后调用 **ibv_get_cq_event()** 从 cc 中获得一个提醒（会返回 cc 对应的 cq ），这个操作会*阻塞*直到出现一个提醒，每个提醒必须使用 **ibv_ack_cq_events()** 确认。然后调用 **ibv_poll_cq()** 从 cq 中获得指定个数以内的 cqe 存入数组中。每个 ibv_wc 结构中的 status 属性表示操作的状态， opcode 表示操作的类型。其中 ibv_post\_* 操作需要额外进行 ibv_send_wr 和 ibv_sge sge 的构造，rdma_post\_* 不需要进行额外的操作。每次操作都需要用 **ibv_reg_mr()** 使用对应连接的 pd 注册一段内存（无论是发送还是接收）。每个 mr（memory region） 中保存了一对 rkey 和 lkey。如果使用的是 ibv_post\_* 接口的话，则需要构造一个 ibv_recv_wr 变量，其中的 wr_id 会作为操作结果的 wc 中的 wr_id 返回，而其中的 sg_list 是一个 sge 数组，每个 sge 元素记录了一个内存空间开始地址、长度和 lkey。使用完 mr 之后要记得使用 **ibv_dereg_mr()** 注销，并释放动态分配的内存空间。



#### rdma_socket 实现细节 ####

**socket_()** 函数创建了一个基本的端点（只有 id 和 event queue）。只能用来建立连接。

**bind_()** 函数是对 **rdma_bind_addr()** 的简单封装。

**listen_()** 函数是对 **rdma_listen()** 的简单封装。

**accept_()** 函数用来监听一个 Socket 结构，并返回接受并建立的连接。在 accept_() 中，会生成一个新的 Socket 结构（对应一个新的连接）并返回。因为每个连接只能通过 event channel 进行管理，所以我们只能使用一个线程来并发地检查在 event channel 中是否有一个event（一般是 RDMA_CM_EVENT_DISCONNECTED）。这里，对于每个连接创建一个 wait_for_close 线程来监听对应的 event channel，查看连接是否关闭（很不优雅。。。最好是使用一个 id 监听所有的连接，但是资源方面好像需要建立一个映射表来释放。。。待优化）。

**connect_()** 用来发起一个连接，同样对于每个连接创建一个 wait_for_close 线程来监听连接是否关闭。

**close_()** 用来关闭连接并释放为连接维护的所有资源，可以双向关闭。关闭操作会发送一个 METADATA_CLOSE 的 MetaData 结构（见下面），使得对端的 send/recv 操作失效（返回 -1，表示连接已关闭），并被动（在 wait_for_close 中）调用 rdma_disconnnect()，关闭连接，从而达到双向关闭的效果。

对于 RDMA 有两种消息的发送和接收方式，一种是 send/recv，一种是 remote read/remote write，而对于 send/recv 方式，在发送方 send 之前必须保证在接收方必须已经注册了 recv 的工作请求，并且对应的 recv 有足够的缓冲区来接收 send 发送的内存块（直接拷贝），这种方式的消息大小是很难变动的，所以只适合发送固定大小的消息。而对于 remote read/remote write 方式，不会对对端有任何的通知或提醒（直接读取到本地内存中或写入对端内存中）。

所以我们结合了 send/recv 和 remote read 建立了一个*简单的通信协议*（当然，这个协议建立在 RDMA 提供了消息的有序和可靠传输的基础上）：即每次在本地注册一个 memory region 并将其信息填充到一个固定大小的 MetaData 结构中（METADATA_NORMAL 类型）并使用 send 操作将其发送到对端（每个端点预先建立了一个 MetaData 池，用来循环接收发送方传过来的 Metadata），接收端使用 remote read 操作将发送方内存中的数据读取到本地，并返回一个 MetaData 结构（METADATA_ACK 类型），发送方通过接收一个 METADATA_ACK 来释放拷贝的已发送成功的内存块。并且我们为连接维护了一个计数器，保证预先注册的 MetaData 缓冲区不被消耗完毕，这意味着，在接受端 recv 处理掉其接收到的 MetaData 之前，发送端只能一次连续发送有限个内存块（如果消耗完接收端的 MetaData 缓冲区之后会阻塞）。

**send_()** 如上所述，会建立一个 MetaData 结构（局部变量，会自动销毁）并 send 到对端， 并且会将用户传入的需要发送的内存块拷贝一份（为了避免用户会在对面读取之前销毁缓冲区），将其注册成一个 memory region。值得注意的是，我们的 send_cq 和 recv_cq 绑定到了同一个 cq 上，并且每次调用 send\_() 和 recv\_() 中会将其清空并处理，释放之前已经发送成功的内存块，（如果既发送也接受的话）并将收到的内存块保存在一个端点维护的队列中，*以提高 recv 的响应时间和吞吐量*。（这里没有限制接收到的总内存块的大小。。。内存块过大可能会爆炸）

**recv_()** 也同样会每次将对应的 complete queue 清空并处理，释放之前已经发送成功的内存块，（如果既发送也接受的话）并将收到的内存块保存在一个端点维护的队列中，如果内存块队列不为空的话就直接从中取出一个内存块返回，否则阻塞。

对于 one-receiver/multi-sender 的消息队列模型，可以使用以下接口：

**receiver_build()** 建立一个 receiver 并返回。

**receiver_bind()** 将 receiver 绑定到目标端口上，并为每个连接创建一个线程不断处理发送过来的消息。

**receiver_recv()** 从目标 receiver 中收取一个消息并返回（保证了对于每个 sender 的消息的有序性），对于一系列 “多帧消息”，会保证被连续接收。（通过将一个连接中连续的带有“多帧消息”的消息缓存在一个辅助队列中，一次放回到 receiver 的总队列中完成）

同样使用 **connect_()** 建立连接。

