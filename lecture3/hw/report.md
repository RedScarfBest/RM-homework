# 项目理解报告

请尽量使用自己的语言回答以下问题。可以引用少量关键代码或伪代码，但不要只粘贴实现。
完成一节后删除该节末尾的待填写标记；本地检查会拒绝仍有未完成章节的报告。

## 1. 图像生命周期与所有权

解释本项目中图像源为什么会复用缓冲区，以及 `cv::Mat` 的普通复制对底层像素数据
意味着什么。说明你的修改让一个 `Frame` 在进入队列后拥有什么，并解释为何后续读取
不会再改变它。

    因为一开始相机SDK采集是用的一块固定缓冲区，每采集到一帧新图像都把数据写进同一块内存，直接覆盖而不用新的去存。在这个项目里buffer_就是对应这里的相机缓冲区，raw.copyTo(buffer_)会直接复用buffer_已有的内存。
    cv::Mat的普通复制是浅拷贝，cv::Mat包含了描述信息和指向像素数据的指针和引用计数，普通复制只复制了这部分描述信息且让引用计数+1，两块Mat指向的都是同一片像素内存。
    所以我的修改是让每个进入队列的Frame通过clone()来拥有一份独立的像素，因为clone()会分配新内存并把像素完整复制过去，后面Frame::image就不再指向buffer_了。


## 2. 并发处理与恰好一次

结合 `BlockingQueue` 的 `push`、`pop` 和 `close` 行为，解释多个 worker 如何分工。
为什么你的实现既不会漏掉已经入队的帧，也不会重复处理同一帧？输入耗尽时，正在等待
以及仍在处理数据的 worker 分别会怎样？

    BlockingQueue::push 在持锁的状态下检查 closed_，未关闭才把元素放进内部 std::queue，离开临界区后再 notify_one()。因为只多了一份任务，叫醒一个 worker 就够了。

    pop在持锁状态下通过 ready_.wait(lock, predicate) 等待，谓词是 closed_ || !queue_.empty()；谓词成立后，如果队列为空就直接返回 false，说明队列已关闭且已被取空，否则执行 value = std::move(queue_.front()); queue_.pop()。取值和出队是在同一把锁的保护下一次性完成的，就不可能出现两个 worker 拿到同一个元素，也不存在元素被取出却没有出队的情况。所以不会重复处理同一帧。

    不会漏帧是因为谓词是 closed_ || !queue_.empty()，而不是只判断 closed_：即使队列已被关闭，只要里面还有元素，pop 依然会成功把它们交出去；只有真正取空之后才返回 false。生产者在输入全部读完时调用 queue_.close()，它只把 closed_ 置真并 notify_all()，并不会清空队列，所以此前已经入队的帧一定会被消费完。

    输入耗尽时，各 worker 都会正常收工：正在等待的 worker 被 close() 里的 notify_all() 唤醒，谓词成立，队列已空，于是 pop 返回 false，while 循环结束、线程函数返回。正在处理数据的 worker 不受影响，它先把手上这一帧处理、保存、记账完毕，再回到循环去 pop，此时同样会拿到 false 而退出。也就是说每个 worker 都是把手上的事做完再退出，不会因为收到关闭信号就丢掉手里正在处理的帧。


## 3. 共享统计数据

指出哪些线程会读写 `Statistics`。解释原实现中的竞争为什么可能导致错误结果，并说明
你的同步方案提供了什么保证。还应说明取得快照时为什么是安全的。

    写的:producer线程（onProduced()）和所有worker线程（onProcessed()、onSaved()、onCorrupted()）。
    读的是 main 线程通过 pipeline.statistics() 调用 snapshot()。
    原错误在于 deliberatelySlowIncrement 是一个这样工作的：先把当前值读进old，睡100微秒，再写回old + 1。这段时间里别的线程完全可以读到同一个旧值，于是两个线程都写回N + 1，本该发生的两次自增只留下一次。这就是数据竞争导致的丢失更新。测试里8个线程各加50次，本应为400，实际只得到50，说明绝大部分自增都被覆盖掉了。
    我改的是给Statistics加一把互斥量，并让每一次访问都在临界区内完成整段读改写：onXxx() 先 std::lock_guard<std::mutex> lock(mutex_) 拿到锁，再调用自增函数，函数返回时随作用域结束自动解锁。这样同一时刻只有一个线程能修改计数，每次自增都基于最新的值，最终结果必然等于调用次数，不会丢失。这里锁的边界必须包住整个读改写，只锁最后一句赋值是没有意义的，因为竞争只发生在读和写之间。
    取得快照时同样是安全的，因为snapshot()也持有同一把锁，它在临界区内一次性读取四个计数器，因此得到的是同一时刻的一致状态，不会出现produced = 20而processed = 19这种由于"读到别人写了一半的组合"造成的自相矛盾快照。另外，snapshot() 是const成员函数，而加锁需要修改互斥量，所以 mutex_ 被声明为 mutable，表示这把锁不属于对象对外的可观察状态，只是实现细节。

## 4. 线程关闭协议

分别描述以下两条路径中的事件顺序，并解释为什么不会发生 `std::terminate`、悬空访问
或永久等待：

1. 调用者执行 `start()` 后显式调用 `wait()`；
2. 调用者执行 `start()` 后不调用 `wait()`，直接让 `Pipeline` 析构。

如果你的实现允许某个生命周期方法被重复调用，也请说明其行为；如果不允许，请说明前置条件。

1. 调用 start() 之后显式调用 wait()。
    start() 创建 worker_count 个 worker 线程和 1 个 producer 线程。worker 一启动就被堵在 pop 内的条件变量上，等队列非空或已关闭。
    producer 循环读取图像并 push 入队；输入耗尽后 while 结束，调用 queue_.close()。
    所有 worker 被唤醒，把队列中剩余的帧全部取出、处理、保存，直到 pop 取空返回 false，各自结束 while 循环并从线程函数返回。
    wait() 先 producer_.join()，再按顺序对每个 worker join()。返回时所有线程都已真正结束，统计值已经稳定，main 随后读取统计是安全的。
    之后 Pipeline 析构，此时所有 std::thread 都已经 join 过（joinable() 为 false），析构不会有任何问题，再调用一次 wait() 也是空操作。

2. 调用 start() 之后不调用 wait()，直接让 Pipeline 析构。
    析构函数会调用 wait()，因此它走的是与路径一完全相同的关闭流程。这里的关键在于时序：join 发生在析构函数体内，而成员变量的销毁发生在函数体之后。所以一定是线程先全部结束成员才被销毁，任何线程都不可能访问到已经被销毁的 queue_、statistics_ 或 config_，这样既不会出现悬空访问，也不会出现 std::terminate。
    基线版本正是反例：析构函数是空的，于是成员按声明逆序销毁时 workers_ 里还躺着 joinable 的线程，std::thread 的析构函数直接调用 std::terminate，进程被 abort，这正是 shutdown_test 崩溃的原因。这条路径下所有帧仍会被处理完毕，也就是说析构的语义是收尾而不是立刻丢弃。

关于重复调用：wait() 允许重复调用：线程 join 之后 joinable() 变为 false，再次 wait() 只是跳过所有 join，属于幂等操作；如果从未调用 start()，线程容器为空，wait() 与析构同样是安全的空操作。start() 则只允许调用一次，第二次调用时 producer_ 仍处于 joinable 状态，对 joinable 的 std::thread 赋值会触发 std::terminate，同时 workers_ 还会被追加一批线程。因此我在 start() 开头加了 started_ 检查，第二次调用抛出 std::logic_error，把只能启动一次作为明确的前置条件，而不是留给未定义行为。


