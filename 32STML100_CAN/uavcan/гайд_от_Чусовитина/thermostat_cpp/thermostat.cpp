#include <algorithm>
#include <array>
#include <chrono>
#include <format>
#include <future>
#include <iostream>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <thread>
#include <variant>

#include <nunavut/support/serialization.hpp>
#include <uavcan/node/Heartbeat_1_0.hpp>
#include <uavcan/node/port/List_1_0.hpp>
#include <uavcan/si/sample/temperature/Scalar_1_0.hpp>
#include <uavcan/si/unit/temperature/Scalar_1_0.hpp>
#include <uavcan/si/unit/voltage/Scalar_1_0.hpp>

#include <canard.h>
#include <o1heap.h>
#include <socketcan.hpp>

constexpr CanardPortID temperature_setpoint_port = 2345;
constexpr CanardPortID temperature_sensor_port   = 2346;
constexpr CanardPortID voltage_port              = 2347;

constexpr std::size_t queueCapacity              = 32;

constexpr std::size_t mtuBytes                   = CANARD_MTU_CAN_FD;

class O1ArenaAllocator {
private:
    alignas(O1HEAP_ALIGNMENT) std::array<std::byte, 4096> arena_{};
    O1HeapInstance* allocator_ = o1heapInit(arena_.data(), sizeof(arena_));

public:
    void* allocate(std::size_t amount) {
        return o1heapAllocate(allocator_, amount);
    }
    void deallocate(void* ptr) {
        if (ptr != nullptr) {
            // NOTE: o1heap seems to check for nullptr
            o1heapFree(allocator_, ptr);
        }
    }
};

template <CanardPortID portID, typename T>
class Publisher {
private:
    CanardTransferID transferID_ = 0;
    CanardInstance*  canard_     = nullptr;
    CanardTxQueue*   queue_      = nullptr;

public:
    Publisher(CanardInstance* canard, CanardTxQueue* queue)
        : canard_{canard}
        , queue_{queue} {}

    [[nodiscard]] constexpr CanardPortID port() const noexcept {
        return portID;
    }

    void operator()(const T& t) {
        std::array<std::uint8_t, T::_traits_::SerializationBufferSizeBytes>
             buffer{};
        auto payloadSize = serialize(t, buffer);
        if (!payloadSize) {
            throw std::runtime_error{
                std::format("serialization failed with {}",
                            static_cast<int>(payloadSize.error()))};
        }

        CanardTransferMetadata metadata{
            .priority       = CanardPriorityNominal,
            .transfer_kind  = CanardTransferKindMessage,
            .port_id        = portID,
            .remote_node_id = CANARD_NODE_ID_UNSET,
            .transfer_id    = transferID_};

        if (canardTxPush(queue_,
                         canard_,
                         0,
                         &metadata,
                         {.size = *payloadSize, .data = buffer.data()},
                         0,
                         nullptr)
            < 0) {
            std::cout << "transmission failed" << std::endl;
        }

        ++transferID_;
    };
};

class HeartBeat {
private:
    Publisher<uavcan::node::Heartbeat_1_0::_traits_::FixedPortId,
              uavcan::node::Heartbeat_1_0>
                                publisher_;
    uavcan::node::Heartbeat_1_0 hb_{
        .uptime = 0,
        .health = {uavcan::node::Health_1_0::NOMINAL},
        .mode   = {uavcan::node::Mode_1_0::OPERATIONAL}};

    using clock             = std::chrono::steady_clock;
    clock::time_point next_ = clock::now();

public:
    HeartBeat(CanardInstance* canard, CanardTxQueue* queue)
        : publisher_(canard, queue) {}

    void operator()() {
        publisher_(hb_);
        ++hb_.uptime;
        next_ += std::chrono::seconds{
            uavcan::node::Heartbeat_1_0::MAX_PUBLICATION_PERIOD};
    }

    [[nodiscard]] clock::time_point next() const noexcept { return next_; }
};

template <bool inUse>
class SubjectList;
template <>
class SubjectList<false> {
public:
    constexpr SubjectList(CanardInstance*, CanardTxQueue*) noexcept {};
};

template <>
class SubjectList<true> {
private:
    using list_t = uavcan::node::port::List_1_0;
    using sparse_list =
        uavcan::node::port::SubjectIDList_1_0::_traits_::TypeOf::sparse_list;

    Publisher<list_t::_traits_::FixedPortId, list_t> publisher_;
    list_t                                           list_{
        .publishers = {.union_value = {sparse_list{
                           {uavcan::node::Heartbeat_1_0::_traits_::FixedPortId},
                           {list_t::_traits_::FixedPortId}}}}};

    using clock             = std::chrono::steady_clock;
    clock::time_point next_ = clock::now();

public:
    SubjectList(CanardInstance* canard, CanardTxQueue* queue)
        : publisher_(canard, queue) {
        list_.subscribers.set_sparse_list();
    }

    template <CanardPortID port>
    void addSubscriber() {
        if (auto* ptr = list_.subscribers.get_sparse_list_if();
            ptr != nullptr) {
            (*ptr).emplace_back(port);
        } else {
            throw std::runtime_error{
                "Expected subscriber list to be a sparse list"};
        }
    }

    template <CanardPortID port>
    void deleteSubscriber() {
        if (auto* ptr = list_.subscribers.get_sparse_list_if();
            ptr != nullptr) {
            const auto it =
                std::ranges::find(*ptr,
                                  port,
                                  &uavcan::node::port::SubjectID_1_0::value);
            if (it != ptr->end()) {
                ptr->erase(it);
            }
        } else {
            throw std::runtime_error{
                "Expected subscriber list to be a sparse list"};
        }
    }

    template <CanardPortID port>
    void addPublisher() {
        if (auto* ptr = list_.publishers.get_sparse_list_if(); ptr != nullptr) {
            (*ptr).emplace_back(port);
        } else {
            throw std::runtime_error{
                "Expected subscriber list to be a sparse list"};
        }
    }

    template <CanardPortID port>
    void deletePublisher() {
        if (auto* ptr = list_.publishers.get_sparse_list_if(); ptr != nullptr) {
            const auto it =
                std::ranges::find(*ptr,
                                  port,
                                  &uavcan::node::port::SubjectID_1_0::value);
            if (it != ptr->end()) {
                ptr->erase(it);
            }
        } else {
            throw std::runtime_error{
                "Expected subscriber list to be a sparse list"};
        }
    }

    void operator()() {
        publisher_(list_);
        next_ += std::chrono::seconds{
            uavcan::node::port::List_1_0::MAX_PUBLICATION_PERIOD};
    }

    [[nodiscard]] clock::time_point next() const noexcept { return next_; }
};

template <CanardPortID portID, typename T>
class Subscriber {
private:
    CanardInstance*      canard_;
    CanardRxSubscription subscription_;

public:
    explicit Subscriber(CanardInstance* canard)
        : canard_{canard}
        , subscription_{} {
        if (canardRxSubscribe(canard_,
                              CanardTransferKindMessage,
                              portID,
                              T::_traits_::ExtentBytes,
                              CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
                              &subscription_)
            < 1) {
            throw std::runtime_error{"Failed to create subscription"};
        }
    }

    ~Subscriber() {
        canardRxSubscribe(canard_,
                          CanardTransferKindMessage,
                          portID,
                          T::_traits_::ExtentBytes,
                          CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
                          &subscription_);
    }

    [[nodiscard]] T operator()(const CanardRxTransfer& transfer) {
        T           data{0};
        std::size_t size = transfer.payload.size;
        if (!deserialize(
                data,
                {reinterpret_cast<std::uint8_t*>(transfer.payload.data),
                 transfer.payload.size})) {
            throw std::runtime_error{"Deserialization failed"};
        }
        return data;
    }

    [[nodiscard]] constexpr CanardPortID port() const noexcept {
        return portID;
    }
};

template <CanardNodeID nodeID, bool ProvideList>
class CanardNode {
private:
    CanardInstance canard_;
    CanardTxQueue  queue_;
    HeartBeat      hb_;

    [[no_unique_address]] SubjectList<ProvideList> list_;

    using clock              = std::chrono::steady_clock;
    clock::time_point start_ = clock::now();

public:
    explicit CanardNode(CanardMemoryResource memory)
        : canard_{canardInit(memory)}
        , queue_{canardTxInit(queueCapacity, mtuBytes, memory)}
        , hb_{&canard_, &queue_}
        , list_{&canard_, &queue_} {
        canard_.node_id = nodeID;
    }

    [[nodiscard]] auto nextList() const noexcept
    requires ProvideList
    {
        return list_.next();
    };
    void list()
    requires ProvideList
    {
        list_();
    };

    [[nodiscard]] auto nextHeartbeat() const noexcept { return hb_.next(); };
    void               heartBeat() { hb_(); }

    template <CanardPortID port, typename T>
    Publisher<port, T> publish() {
        if constexpr (ProvideList) {
            list_.template addPublisher<port>();
        }
        return {&canard_, &queue_};
    }

    template <CanardPortID port, typename T>
    Subscriber<port, T> subscribe() {
        if constexpr (ProvideList) {
            list_.template addSubscriber<port>();
        }
        return Subscriber<port, T>{&canard_};
    }

    template <std::invocable<CanardTxQueueItem*> F>
    void txPoll(F&& f) {
        for (CanardTxQueueItem* item = canardTxPeek(&queue_); item != nullptr;
             item                    = canardTxPeek(&queue_)) {
            // process queue
            f(item);
            // free
            canardTxPop(&queue_, item);
            canardTxFree(&queue_, &canard_, item);
        }
    };

    std::optional<CanardRxTransfer> receive(CanardFrame frame) {
        CanardRxTransfer transfer{};

        const auto res = canardRxAccept(
            &canard_,
            std::chrono::duration_cast<std::chrono::microseconds>(clock::now()
                                                                  - start_)
                .count(),
            &frame,
            0,
            &transfer,
            nullptr);

        if (res != 1) {
            return std::nullopt;
        }
        return transfer;
    }
};

int main() try {
    O1ArenaAllocator allocator;

    const auto canardAllocate = [](void*       allocator,
                                   std::size_t amount) -> void* {
        return reinterpret_cast<O1ArenaAllocator*>(allocator)->allocate(amount);
    };
    const auto canardDeallocate =
        [](void* allocator, std::size_t, void* ptr) -> void {
        return reinterpret_cast<O1ArenaAllocator*>(allocator)->deallocate(ptr);
    };

    SocketCANFD socket = socketcanOpen("vcan0", mtuBytes);

    CanardMemoryResource memory{.user_reference = &allocator,
                                .deallocate     = canardDeallocate,
                                .allocate       = canardAllocate};
    CanardNode<43, true> node{memory};

    std::array<std::byte, mtuBytes> buffer{};

    auto voltagePublisher =
        node.publish<voltage_port, uavcan::si::unit::voltage::Scalar_1_0>();

    auto temperatureSetPointSub =
        node.subscribe<temperature_setpoint_port,
                       uavcan::si::unit::temperature::Scalar_1_0>();
    auto temperatureSensor =
        node.subscribe<temperature_sensor_port,
                       uavcan::si::sample::temperature::Scalar_1_0>();

    float temperatureSetPoint = 0;
    for (;;) {
        using namespace std::chrono_literals;
        if (std::chrono::steady_clock::now() >= node.nextHeartbeat()) {
            node.heartBeat();
        }

        if (std::chrono::steady_clock::now() >= node.nextList()) {
            node.list();
        }

        CanardFrame frame{};
        if (const auto res = socketcanPop(socket,
                                          &frame,
                                          nullptr,
                                          buffer.size(),
                                          buffer.data(),
                                          0,
                                          nullptr);

            res < 0) {
            throw std::runtime_error{"Failed getting can data"};
        } else if (res != 0) {
            auto transfer = node.receive(frame);
            if (transfer) {
                switch (transfer->metadata.port_id) {
                case temperatureSetPointSub.port():
                    temperatureSetPoint =
                        temperatureSetPointSub(*transfer).kelvin;
                    break;
                case temperatureSensor.port():
                    voltagePublisher(uavcan::si::unit::voltage::Scalar_1_0{
                        temperatureSetPoint
                        - temperatureSensor(*transfer).kelvin});
                    break;
                default:
                    throw std::runtime_error{
                        "Received transfer I'm not subscribed to"};
                }
            }
        }
        node.txPoll([&socket](CanardTxQueueItem* item) -> std::int8_t {
            CanardFrame f{
                .extended_can_id = item->frame.extended_can_id,
                .payload         = {.size = item->frame.payload.size,
                                    .data = item->frame.payload.data}
            };
            if (socketcanPush(socket, &f, 0) < 0) {
                printf("Fatal error sending CAN data. Exiting thread.\n");
                return -1;
            }
            return 1;
        });
    }

    return 0;
} catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    return 1;
}
