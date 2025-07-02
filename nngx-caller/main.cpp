#include <iostream>
#include <format>
#include <thread>
#include <latch>
#include <array>
#include <string>
#include <random>

#include "nngx.h"
#include "nngUtil.h"

class NngTester
{
public:
    static void TestRawMessage_PushPull() {
        using namespace nng;
        class MyPull : public Pull<>
        {
        private:
            virtual bool _On_raw_message(Msg& msg) {
                std::string s;
                s.assign((const char*)msg.body(), msg.len());
                if (s.compare("Exit") == 0) {
                    this->close();
                }
                else {
                    assert(s.compare("StringContent") == 0);
                    m_nCount++;
                }
                return true;
            }

        public:
            size_t m_nCount = 0;
        };

        MyPull pull;
        std::promise<void> proListening;
        auto futListening = proListening.get_future();
        std::thread thPull(
            [&proListening, &pull]
            {
                assert(pull.start(m_szAddr) == NNG_OK);
                proListening.set_value();
                pull.dispatch();
            }
        );

        std::thread thPush(
            [&thPull, &futListening]
            {
                Push<> push;
                futListening.wait();

                assert(push.start(m_szAddr) == NNG_OK);

                const char szData[] = "StringContent";
                for (size_t i(0); i < 10; ++i) {
                    push.async_send(nng_iov{ (void*)szData, strlen(szData) });
                }

                push.async_send(nng_iov{ (void*)"Exit", 4 });

                assert(thPull.joinable());
                thPull.join();
            }
        );

        assert(thPush.joinable());
        thPush.join();
        assert(pull.m_nCount == 10);
        printf("%s -> Passed\r\n", __FUNCTION__);
    }

    static void TestMessage_Pair() {
        using namespace nng;
        enum {
            MSG_0 = 0,
            MSG_1 = 1000,
            MSG_OTHER = 23400,
            MSG_REPLY = 5500,
            MSG_QUIT = 56700,
        };
        class ListenerPair : public Pair<Listener>
        {
        private:
            virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg) override {
                if (code == MSG_QUIT) {
                    this->close();
                    return {};
                }

                auto sArg = msg.chop_string();
                printf("Listener[%zu] -> Code:%I64u, Argument:%s\n", ++m_nCount, code, sArg.c_str());
                return {};
            }
        public:
            size_t m_nCount = 0;
        };
        class DialerPair : public Pair<Dialer>
        {
        private:
            virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg) override {
                if (code == MSG_QUIT) {
                    this->async_send(MSG_QUIT, {});
                    return {};
                }
                auto sArg = msg.chop_string();
                printf("Dialer[%zu] -> Code:%I64u, Argument:%s\n", ++m_nCount, code, sArg.c_str());

                msg.realloc(0);
                msg.append_string("DialerReply");
                this->async_send(MSG_REPLY, std::move(msg));
                return {};
            }
        public:
            size_t m_nCount = 0;
        };

        ListenerPair listenerPair;
        std::promise<void> proListening;
        auto futListening = proListening.get_future();
        std::thread thListenerPair(
            [&proListening, &listenerPair]
            {
                assert(listenerPair.start(m_szAddr) == NNG_OK);
                proListening.set_value();

                std::thread thDispatch(
                    [&listenerPair]
                    {
                        listenerPair.dispatch();
                    }
                );

                std::thread thSender(
                    [&listenerPair]
                    {
                        for (size_t i(0); i < 10; ++i) {
                            Msg m(0);
                            m.append_string("Arg0");
                            listenerPair.async_send(MSG_0, std::move(m));

                            m.realloc(0);
                            m.append_string("Arg1");
                            listenerPair.async_send(MSG_1, std::move(m));

                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }

                        listenerPair.async_send(MSG_QUIT, {});

                    }
                );

                assert(thDispatch.joinable());
                thDispatch.join();
                assert(thSender.joinable());
                thSender.join();
            }
        );

        std::thread thDialerPair(
            [&futListening, &thListenerPair]
            {
                DialerPair dialerPair;
                futListening.wait();
                assert(dialerPair.start(m_szAddr) == NNG_OK);

                // 接收线程
                std::thread thDispatch(
                    [&dialerPair]
                    {
                        dialerPair.dispatch();
                    }
                );

                // 发送线程
                std::thread thSender(
                    [&dialerPair]
                    {
                        for (size_t i(0); i < 10; ++i) {
                            Msg m(0);
                            m.append_string("ArgOther");
                            dialerPair.async_send(MSG_OTHER, std::move(m));

                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }

                    }
                );

                // 等待服务端线程退出
                assert(thListenerPair.joinable());
                thListenerPair.join();

                dialerPair.close();

                assert(thDispatch.joinable());
                thDispatch.join();
                assert(thSender.joinable());
                thSender.join();

                assert(dialerPair.m_nCount == 20);
            }
        );

        assert(thDialerPair.joinable());
        thDialerPair.join();
        assert(listenerPair.m_nCount == 30);
        printf("%s -> Passed\r\n", __FUNCTION__);
    }

    static void TestMessage_RequestResponse() {
        using namespace nng;
        enum {
            MSG_CODE0 = 0x345,
        };
        class ListenerRespnose : public Response
        {
        private:
            virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg) override {
                if (code == MSG_CODE0) {
                    assert(msg.chop_u32() == 0x12345678);
                    assert(msg.len() == 0);

                    msg.realloc(0);
                    msg.insert_string("ReplyString");

                    m_nCount++;
                    return 0x87654321;
                }
                else {
                    return 0;
                }
            }
        public:
            size_t m_nCount = 0;
        };

        ListenerRespnose response;
        std::promise<void> proListening;
        auto futListening = proListening.get_future();
        std::thread thResponse(
            [&proListening, &response]
            {
                assert(response.start(m_szAddr) == NNG_OK);
                proListening.set_value();

                response.dispatch();
            }
        );

        futListening.wait();

        Request request;
        assert(request.start(m_szAddr) == NNG_OK);

        // 同步发送请求
        for (size_t i(0); i < 3; ++i) {
            Msg m(0);
            m.append_u32(0x12345678);
            auto result = request.send(MSG_CODE0, m);
            assert(m.trim_string().compare("ReplyString") == 0);
            assert(result == 0x87654321);
        }

        for (size_t i(0); i < 3; ++i) {
            Msg m(0);
            m.append_u32(0x12345678);
            auto future = request.async_send(MSG_CODE0, std::move(m));
            m = future.get();

            auto result = Msg::_Chop_msg_result(m);
            assert(m.trim_string().compare("ReplyString") == 0);
            assert(result == 0x87654321);
        }

        response.close();
        assert(thResponse.joinable());
        thResponse.join();
        assert(response.m_nCount == 6);
        printf("%s -> Passed\r\n", __FUNCTION__);
    }

    static void TestMessage_PublisherSubscriber() {
        using namespace nng;
        enum {
            MSG_CODE0 = 0x345,
            MSG_QUIT,
        };
        class ListenerSubscriber : public Subscriber<>
        {
        private:
            virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg) override {
                if (code == MSG_CODE0) {
                    std::string s = msg.chop_string();
                    assert(s.compare("PublishString") == 0);
                    assert(msg.len() == 0);
                    printf("%s\n", s.c_str());
                    m_nCount++;
                    return {};
                }
                else if (code == MSG_QUIT) {
                    close();
                }

                return 0;
            }
        public:
            size_t m_nCount = 0;
        };

        Publisher publisher;

        enum { MESSAGE_COUNT = 5 };
        enum { SUBCRIBER_COUNT = 3 };
        std::latch latchSubscribers(SUBCRIBER_COUNT);

        std::promise<void> proListening;
        auto futListening = proListening.get_future();
        std::thread thPublisher(
            [&proListening, &publisher, &latchSubscribers]
            {
                assert(publisher.start(m_szAddr) == NNG_OK);
                proListening.set_value();
                latchSubscribers.wait();

                //std::this_thread::sleep_for(std::chrono::milliseconds(200));
                for (size_t i(0); i < MESSAGE_COUNT; ++i) {
                    // Sync
                    Msg m(0);
                    m.append_string("PublishString");
                    publisher.send(MSG_CODE0, std::move(m));
                }

                publisher.send(MSG_QUIT);
            }
        );

        futListening.wait();

        std::array<std::thread, SUBCRIBER_COUNT> arrSubcribers;
        for (size_t i(0); i < SUBCRIBER_COUNT; ++i) {
            arrSubcribers[i] = std::thread(
                [&latchSubscribers]
                {
                    ListenerSubscriber subscriber;
                    assert(subscriber.start(m_szAddr) == NNG_OK);
                    latchSubscribers.count_down();

                    subscriber.socket_subscribe();

                    // 只要连接成功，就会确保能接收到
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));

                    subscriber.dispatch();
                    assert(subscriber.m_nCount == MESSAGE_COUNT);
                }
            );
        }

        for (size_t i(0); i < SUBCRIBER_COUNT; ++i) {
            assert(arrSubcribers[i].joinable());
            arrSubcribers[i].join();
        }

        thPublisher.join();
        printf("%s -> Passed\r\n", __FUNCTION__);
    }

    static void TestMessage_SurveyRespond()
    {
        using namespace nng;
        enum {
            MSG_CODE0 = 0x345,
        };
        class ListenerRespond : public Respond<>
        {
        private:
            virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg) override {
                if (code == MSG_CODE0) {
                    std::string s = msg.chop_string();
                    assert(s.compare("SurveyString") == 0);
                    assert(msg.len() == 0);
                    printf("%s\n", s.c_str());
                    msg.realloc(0);
                    msg.insert_string("ReplyString");
                    return 0x44488800;
                }
                else {
                    return 0;
                }
            }
        };

        Survey survey;
        assert(survey.start(m_szAddr) == NNG_OK);

        enum { RESPOND_COUNT = 5 };
        std::array<std::pair<std::thread, ListenerRespond>, RESPOND_COUNT> responds;
        std::latch latchResponds(RESPOND_COUNT);
        for (auto& respond : responds) {
            respond.first = std::thread(
                [&]
                {
                    assert(respond.second.start(m_szAddr) == NNG_OK);
                    latchResponds.count_down();

                    respond.second.dispatch();
                }
            );
        }

        latchResponds.wait();

        std::vector<Msg> vecMsgs;
        Msg m(0);
        m.append_string("SurveyString");
        survey.send(MSG_CODE0, std::move(m), std::back_inserter(vecMsgs));

        for (auto& respond : responds) {
            respond.second.close();
        }

        for (auto& respond : responds) {
            assert(respond.first.joinable());
            respond.first.join();
        }

        assert(vecMsgs.size() == RESPOND_COUNT);
        for (Msg& m : vecMsgs) {
            Msg::_Ty_msg_result result = Msg::_Chop_msg_result(m);
            assert(result == 0x44488800);

            assert(m.trim_string().compare("ReplyString") == 0);
        }

        printf("%s -> Passed\r\n", __FUNCTION__);
    }

    static void TestMessage_Bus()
    {
        using namespace nng;
        enum {
            MSG_CODE0 = 0x345,
            MSG_QUIT,
        };
        class MyBus : public Bus
        {
        private:
            virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg) override {
                if (code == MSG_CODE0) {
                    std::string s = msg.chop_string();
                    printf("BusReceive[%zu]:%s\n", m_nIdx, s.c_str());
                    assert(s.compare("BusString") == 0);
                    assert(msg.len() == 0);
                    m_fReceived = true;
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    return {};
                }
                else if (code == MSG_QUIT) {
                    printf("BusExit[%zu]\n", m_nIdx);
                    close();
                }

                return {};
            }
        public:
            bool m_fReceived = false;
            size_t m_nIdx = SIZE_MAX;
        };

        enum { BUS_COUNT = 3 };
        std::array<std::pair<std::thread, MyBus>, BUS_COUNT> buses;
        for (size_t i(0); i < BUS_COUNT; ++i) {
            std::string sAddr = m_szAddr + std::to_string(i);
            buses[i].second.m_nIdx = i;
            assert(buses[i].second.start(sAddr) == NNG_OK);

            buses[i].first = std::thread(
                [bus = std::ref(buses[i])]
                {
                    bus.get().second.dispatch();
                }
            );
        }
        for (size_t i(0); i < BUS_COUNT; ++i) {
            for (size_t j(i + 1); j < BUS_COUNT; ++j) {
                if (i != j) {
                    std::string sAddr = m_szAddr + std::to_string(j);
                    assert(buses[i].second.dial(sAddr) == NNG_OK);
                }
            }
        }

        auto randomInt = [](int min, int max)
            {
                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(min, max);
                return dis(gen);
            };

        int idx = randomInt(0, BUS_COUNT - 1);
        Msg m(0);
        m.append_string("BusString");
        buses[idx].second.send(MSG_CODE0, std::move(m));
        buses[idx].second.send(MSG_QUIT); // 其它方退出
        buses[idx].second.close(); // 发送方退出

        for (auto& bus : buses) {
            assert(bus.first.joinable());
            bus.first.join();
        }

        printf("%s -> Passed\r\n", __FUNCTION__);
    }
    static void TestMessage_ResponseParallel()
    {
        using namespace nng;
        enum {
            MSG_CODE0 = 0x345,
            MSG_QUIT,
        };
        class MyResponseParallel : public ResponseParallel
        {
        private:
            virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg) override {
                if (code == MSG_CODE0) {
                    std::string s = msg.chop_string();
                    assert(s.compare("RequestString") == 0);
                    assert(msg.len() == 0);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    msg.realloc(0);
                    msg.append_string("ReplyString");
                    return 0x44448888;
                }
                else if (code == MSG_QUIT) {
                    close();
                }

                return {};
            }
            virtual void _On_close(PWORK_ITEM _Work_item) override {
                printf("ServerClosed.\n");
            }
        };

        enum { PARALLEL = 10 };
        MyResponseParallel mrp;
        assert(mrp.start(m_szAddr, PARALLEL) == NNG_OK);

        Msg m(0);
        m.append_string("RequestString");
        auto result = Request::simple_send(m_szAddr, MSG_CODE0, m);
        assert(result == 0x44448888);
        assert(m.chop_string().compare("ReplyString") == 0);
        mrp.close();
    }

    static void TestMessage_RequestResponse_Service() {
        using namespace nng;
        enum {
            MSG_CODE0 = 0x345,
        };
        class ListenerRespnose : public Service<Response>
        {
        private:
            virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg) override {
                if (code == MSG_CODE0) {
                    assert(msg.chop_u32() == 0x12345678);
                    assert(msg.len() == 0);

                    msg.realloc(0);
                    msg.insert_string("ReplyString");

                    m_nCount++;
                    return 0x87654321;
                }
                else {
                    return 0;
                }
            }
        public:
            size_t m_nCount = 0;
        };

        ListenerRespnose response;
        assert(response.start_dispatch(m_szAddr) == NNG_OK);

        Request request;
        assert(request.start(m_szAddr) == NNG_OK);

        // 同步发送请求
        for (size_t i(0); i < 3; ++i) {
            Msg m(0);
            m.append_u32(0x12345678);
            auto result = request.send(MSG_CODE0, m);
            assert(m.trim_string().compare("ReplyString") == 0);
            assert(result == 0x87654321);
        }

        for (size_t i(0); i < 3; ++i) {
            Msg m(0);
            m.append_u32(0x12345678);
            auto future = request.async_send(MSG_CODE0, std::move(m));
            m = future.get();

            auto result = Msg::_Chop_msg_result(m);
            assert(m.trim_string().compare("ReplyString") == 0);
            assert(result == 0x87654321);
        }

        response.stop_dispatch();
        assert(response.m_nCount == 6);
        printf("%s -> Passed\r\n", __FUNCTION__);
    }

    static void TestMessage_SurveyRespond_Service()
    {
        using namespace nng;
        enum {
            MSG_CODE0 = 0x345,
        };
        class ListenerRespond : public Service<Respond<>>
        {
        private:
            virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg) override {
                if (code == MSG_CODE0) {
                    std::string s = msg.chop_string();
                    assert(s.compare("SurveyString") == 0);
                    assert(msg.len() == 0);
                    printf("%s\n", s.c_str());
                    msg.realloc(0);
                    msg.insert_string("ReplyString");
                    return 0x44488800;
                }
                else {
                    return 0;
                }
            }
        };

        Survey<> survey;
        assert(survey.start(m_szAddr) == NNG_OK);

        enum { RESPOND_COUNT = 5 };
        ListenerRespond respond[RESPOND_COUNT];
        for (size_t i(0); i < RESPOND_COUNT; ++i) {
            respond[i].start_dispatch(m_szAddr);
        }

        std::vector<Msg> vecMsgs;
        Msg m(0);
        m.append_string("SurveyString");
        survey.send(MSG_CODE0, std::move(m), std::back_inserter(vecMsgs));
        assert(vecMsgs.size() == RESPOND_COUNT);
        for (Msg& m : vecMsgs) {
            Msg::_Ty_msg_result result = Msg::_Chop_msg_result(m);
            assert(result == 0x44488800);

            assert(m.trim_string().compare("ReplyString") == 0);
        }

        printf("%s -> Passed\r\n", __FUNCTION__);
    }


    static void TestMessage_Bus_Service()
    {
        using namespace nng;
        enum {
            MSG_CODE0 = 0x345,
            MSG_QUIT,
        };
        class MyBus : public Service<Bus>
        {
        private:
            virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg) override {
                if (code == MSG_CODE0) {
                    std::string s = msg.chop_string();
                    printf("BusReceive[%zu]:%s\n", m_nIdx, s.c_str());
                    assert(s.compare("BusString") == 0);
                    assert(msg.len() == 0);
                    m_fReceived = true;
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    return {};
                }
                else if (code == MSG_QUIT) {
                    printf("BusExit[%zu]\n", m_nIdx);
                    close();
                }

                return {};
            }
        public:
            bool m_fReceived = false;
            size_t m_nIdx = SIZE_MAX;
        };

        enum { BUS_COUNT = 3 };
        std::array<MyBus, BUS_COUNT> buses;
        for (size_t i(0); i < BUS_COUNT; ++i) {
            std::string sAddr = m_szAddr + std::to_string(i);
            buses[i].m_nIdx = i;
            assert(buses[i].start_dispatch(sAddr) == NNG_OK);
        }

        for (size_t i(0); i < BUS_COUNT; ++i) {
            for (size_t j(i + 1); j < BUS_COUNT; ++j) {
                if (i != j) {
                    std::string sAddr = m_szAddr + std::to_string(j);
                    assert(buses[i].dial(sAddr) == NNG_OK);
                }
            }
        }

        auto randomInt = [](int min, int max)
            {
                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(min, max);
                return dis(gen);
            };

        int idx = randomInt(0, BUS_COUNT - 1);
        Msg m(0);
        m.append_string("BusString");
        buses[idx].send(MSG_CODE0, std::move(m));
        buses[idx].send(MSG_QUIT); // 其它方退出
        buses[idx].close(); // 发送方退出

        for (auto& bus : buses) {
            assert(bus.joinable());
            bus.join();
        }

        printf("%s -> Passed\r\n", __FUNCTION__);
    }

    static void TestMessage_RequestResponse_Raw() {
        using namespace nng;
        enum {
            MSG_CODE0 = 0x345,
        };
        class ListenerRespnose : public Service<Response>
        {
        private:
            virtual bool _On_raw_message(Msg& msg) override
            {
                auto code = msg.trim_u32();
                if (code == MSG_CODE0) {
                    assert(msg.trim_u32() == 0x12345678);
                    assert(msg.len() == 0);

                    msg.realloc(0);
                    msg.insert_string("ReplyString");

                    m_nCount++;
                }

                return true;
            }
        public:
            size_t m_nCount = 0;
        };

        ListenerRespnose response;
        response.start_dispatch(m_szAddr);

        Request request;
        assert(request.start(m_szAddr) == NNG_OK);

        // 同步发送请求
        for (size_t i(0); i < 3; ++i) {
            Msg m(0);
            m.append_u32(MSG_CODE0);
            m.append_u32(0x12345678);
            auto result = request.send(m);
            assert(m.trim_string().compare("ReplyString") == 0);
            assert(m.len() == 0);
        }

        // 异步发送请求
        for (size_t i(0); i < 3; ++i) {
            Msg m(0);
            m.append_u32(MSG_CODE0);
            m.append_u32(0x12345678);
            auto future = request.async_send(std::move(m));
            m = future.get();
            assert(m.trim_string().compare("ReplyString") == 0);
            assert(m.len() == 0);
        }

        assert(response.stop_dispatch());
        assert(response.m_nCount == 6);
        printf("%s -> Passed\r\n", __FUNCTION__);
    }

    static void TestMessage_Pair_Service() {
        using namespace nng;
        enum {
            MSG_0 = 0,
            MSG_1 = 1000,
            MSG_OTHER = 23400,
            MSG_REPLY = 5500,
            MSG_QUIT = 56700,
        };
        class ListenerPair : public Service<Pair<Listener>>
        {
        private:
            virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg) override {
                if (code == MSG_QUIT) {
                    this->close();
                    return {};
                }

                auto sArg = msg.chop_string();
                printf("Listener[%zu] -> Code:%I64u, Argument:%s\n", ++m_nCount, code, sArg.c_str());
                return {};
            }
        public:
            size_t m_nCount = 0;
        };
        class DialerPair : public Service<Pair<Dialer>>
        {
        private:
            virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg) override {
                if (code == MSG_QUIT) {
                    this->send(MSG_QUIT);
                    // 同步，上面发送完了再退出
                    this->close();
                    return {};
                }
                auto sArg = msg.chop_string();
                printf("Dialer[%zu] -> Code:%I64u, Argument:%s\n", ++m_nCount, code, sArg.c_str());

                msg.realloc(0);
                msg.append_string("DialerReply");
                this->async_send(MSG_REPLY, std::move(msg));
                return {};
            }
        public:
            size_t m_nCount = 0;
        };

        ListenerPair listenerPair;
        assert(listenerPair.start_dispatch(m_szAddr) == NNG_OK);
        DialerPair dialerPair;
        assert(dialerPair.start_dispatch(m_szAddr) == NNG_OK);

        for (size_t i(0); i < 10; ++i) {
            Msg m(0);
            m.append_string("Arg0");
            listenerPair.async_send(MSG_0, std::move(m));

            m.realloc(0);
            m.append_string("Arg1");
            listenerPair.async_send(MSG_1, std::move(m));

            m.realloc(0);
            m.append_string("ArgOther");
            dialerPair.async_send(MSG_OTHER, std::move(m));
        }

        listenerPair.async_send(MSG_QUIT, {});

        assert(dialerPair.joinable());
        dialerPair.join();

        assert(listenerPair.joinable());
        listenerPair.join();

        assert(listenerPair.m_nCount == 30);
        printf("%s -> Passed\r\n", __FUNCTION__);
    }
    static void TestMessage_PublisherSubscriber_Raw() {
        using namespace nng;
        enum {
            MSG_CODE0 = 0x345,
            MSG_QUIT,
        };
        class ListenerSubscriber : public Service<Subscriber<>>
        {
        private:
            virtual bool _On_raw_message(Msg& msg) override final {
                auto code = msg.chop_u32();
                if (code == MSG_CODE0) {
                    std::string s = msg.chop_string();
                    assert(s.compare("PublishString") == 0);
                    assert(msg.len() == 0);
                    printf("%s\n", s.c_str());
                    m_nCount++;
                }
                else if (code == MSG_QUIT) {
                    close();
                }

                return true;
            }
        public:
            size_t m_nCount = 0;
        };

        Publisher publisher;
        publisher.start(m_szAddr);

        enum { MESSAGE_COUNT = 1 };
        enum { SUBCRIBER_COUNT = 1 };
        ListenerSubscriber subscribers[SUBCRIBER_COUNT];
        for (auto& subscriber : subscribers) {
            assert(subscriber.start_dispatch(m_szAddr) == NNG_OK);
            subscriber.socket_subscribe();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        for (size_t i(0); i < MESSAGE_COUNT; ++i) {
            // Sync
            Msg m(0);
            m.append_u32(MSG_CODE0);
            m.append_string("PublishString");
            assert(publisher.send(std::move(m)) == NNG_OK);
        }

        publisher.send(MSG_QUIT);
        for (auto& subscriber : subscribers) {
            if (subscriber.joinable()) {
                subscriber.join();
            }
        }

        printf("%s -> Passed\r\n", __FUNCTION__);
    }

    static void TestRawMessage_PushPull_HugeMessage() {
        using namespace nng;
        enum {
            MSG_CODE_SYNC = 0x1,
            MSG_CODE_ASYNC
        };
        class MyPull : public Service<Pull<Dialer>>
        {
        private:
            virtual Msg::_Ty_msg_result _On_message(Msg::_Ty_msg_code code, Msg& msg) override final {
                switch (code) {
                case MSG_CODE_SYNC: {
                    std::string sRecv = msg.trim_string();
                    assert(sRecv == "Hello World! Sync!");
                    m_nCountSync++;
                    std::cout << m_nCountSync << " : " << sRecv << std::endl;
                    break;
                }
                case MSG_CODE_ASYNC: {
                    std::string sRecv = msg.trim_string();
                    assert(sRecv == "Hello World! Async!");
                    m_nCountAsync++;
                    std::cout << m_nCountAsync << " : " << sRecv << std::endl;
                    break;
                }
                }
                return {};
            }

        public:
            size_t m_nCountSync = 0;
            size_t m_nCountAsync = 0;
        };

        Push<Listener> pusher;
        assert(pusher.start(m_szAddr) == NNG_OK);

        MyPull puller;
        assert(puller.start_dispatch(m_szAddr) == NNG_OK);

        enum { DATA_COUNT = 10000 };
        for (size_t i(0); i < DATA_COUNT; ++i) {
            nng::Msg m(0);
            m.insert_string("Hello World! Sync!");
            pusher.send(MSG_CODE_SYNC, std::move(m));
        }

        for (size_t i(0); i < DATA_COUNT; ++i) {
            nng::Msg m(0);
            m.insert_string("Hello World! Async!");
            pusher.async_send(MSG_CODE_ASYNC, std::move(m));
        }

        // 等待异步数据发送完成。
        std::this_thread::sleep_for(std::chrono::seconds(10));
        pusher.close();
        puller.stop_dispatch();

        assert(puller.m_nCountSync == DATA_COUNT);
        assert(puller.m_nCountAsync == DATA_COUNT);
        printf("%s -> Passed\r\n", __FUNCTION__);
    }


    static void TestMsg()
    {
        std::string s = "TestString";
        nng::Msg m = nng::Msg::to_msg(s);
        assert(nng::Msg::to_string(m).compare(s) == 0);

        m = nng::Msg::to_msg("");
        assert(nng::Msg::to_string(m).empty());

        nng::Msg msg(0);
        msg.append_string(s);
        assert(msg.chop_string().compare(s) == 0);
        msg.insert_string(s);
        assert(msg.trim_string().compare(s) == 0);
    }
private:
    inline static const char* m_szAddr = "ipc://test.addr";
};

int main()
{
    nng::util::initialize();
    NngTester::TestMsg();
    NngTester::TestRawMessage_PushPull();
    NngTester::TestMessage_Pair();
    NngTester::TestMessage_RequestResponse();
    NngTester::TestMessage_PublisherSubscriber();
    NngTester::TestMessage_SurveyRespond();
    NngTester::TestMessage_Bus();
    NngTester::TestMessage_ResponseParallel();
    NngTester::TestMessage_RequestResponse_Service();
    NngTester::TestMessage_SurveyRespond_Service();
    NngTester::TestMessage_Bus_Service();
    NngTester::TestMessage_RequestResponse_Raw();
    NngTester::TestMessage_Pair_Service();
    NngTester::TestMessage_PublisherSubscriber_Raw();
    NngTester::TestRawMessage_PushPull_HugeMessage();
    nng::util::uninitialize();
    return EXIT_SUCCESS;
}