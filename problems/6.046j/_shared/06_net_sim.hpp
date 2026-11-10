// 06_net_sim.hpp — deterministic network simulators for the distributed exercises in unit 10.
//
// A Topology is an undirected graph whose edge endpoints are numbered ports: process v talks on
// ports 0 .. degree(v)-1 and never learns vertex numbers. Two execution models from 6.046J
// Lectures 19-20 run on it:
//
//  * SyncNetwork: lockstep rounds. Each live process decides what to send, the messages cross
//    the channels, then each live process computes its next state. stats.rounds counts rounds.
//  * AsyncNetwork: discrete events. A process acts only when a message arrives. Each directed
//    channel is a FIFO queue; the delay of every message comes from a policy you pass in, and
//    stats.time is the timestamp of the last delivery.
//
// Same processes + same delay policy => the exact same execution, so failures replay.
#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace netsim {

struct RunStats {
    std::int64_t rounds = 0;    // SyncNetwork: rounds executed
    std::int64_t time = 0;      // AsyncNetwork: timestamp of the last delivered message
    std::int64_t messages = 0;  // messages sent during the run
};

class Topology {
public:
    explicit Topology(int n) : adj_(static_cast<std::size_t>(n)) {}

    // Connects u and v and returns the port on u's side. Parallel edges are allowed (a ring of
    // two nodes has two of them); self-loops are not.
    int add_edge(int u, int v) {
        if (u == v) {
            throw std::invalid_argument("self-loops are not allowed");
        }
        const int port_u = degree(u);
        const int port_v = degree(v);
        adj_[static_cast<std::size_t>(u)].push_back({v, port_v});
        adj_[static_cast<std::size_t>(v)].push_back({u, port_u});
        return port_u;
    }

    int size() const { return static_cast<int>(adj_.size()); }
    int degree(int v) const { return static_cast<int>(adj_[static_cast<std::size_t>(v)].size()); }
    int edge_count() const {
        std::size_t total = 0;
        for (const auto& ports : adj_) {
            total += ports.size();
        }
        return static_cast<int>(total / 2);
    }
    int neighbor(int v, int port) const { return end(v, port).first; }
    // The port number at neighbor(v, port) that leads back to v.
    int reverse_port(int v, int port) const { return end(v, port).second; }

    // Vertex i's port 0 leads clockwise to (i+1) % n and port 1 counterclockwise to (i-1+n) % n.
    static constexpr int kClockwise = 0;
    static constexpr int kCounterclockwise = 1;
    static Topology ring(int n) {
        if (n < 2) {
            throw std::invalid_argument("a ring needs at least two nodes");
        }
        Topology t(n);
        for (int i = 0; i < n; ++i) {
            t.adj_[static_cast<std::size_t>(i)].resize(2);
        }
        for (int i = 0; i < n; ++i) {
            const int next = (i + 1) % n;
            t.adj_[static_cast<std::size_t>(i)][kClockwise] = {next, kCounterclockwise};
            t.adj_[static_cast<std::size_t>(next)][kCounterclockwise] = {i, kClockwise};
        }
        return t;
    }
    static Topology path(int n) {
        Topology t(n);
        for (int i = 0; i + 1 < n; ++i) {
            t.add_edge(i, i + 1);
        }
        return t;
    }
    static Topology clique(int n) {
        Topology t(n);
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                t.add_edge(i, j);
            }
        }
        return t;
    }
    static Topology grid(int rows, int cols) {
        Topology t(rows * cols);
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (c + 1 < cols) {
                    t.add_edge(r * cols + c, r * cols + c + 1);
                }
                if (r + 1 < rows) {
                    t.add_edge(r * cols + c, (r + 1) * cols + c);
                }
            }
        }
        return t;
    }
    // A random spanning tree over shuffled labels plus `extra` further distinct edges.
    static Topology random_connected(int n, int extra, std::uint64_t seed) {
        std::mt19937_64 rng(seed);
        Topology t(n);
        std::vector<int> labels(static_cast<std::size_t>(n));
        std::iota(labels.begin(), labels.end(), 0);
        std::shuffle(labels.begin(), labels.end(), rng);
        const auto seen = [&](int u, int v) {
            for (const auto& [w, _] : t.adj_[static_cast<std::size_t>(u)]) {
                if (w == v) {
                    return true;
                }
            }
            return false;
        };
        for (int i = 1; i < n; ++i) {
            const auto j = static_cast<std::size_t>(
                std::uniform_int_distribution<int>(0, i - 1)(rng));
            t.add_edge(labels[static_cast<std::size_t>(i)], labels[j]);
        }
        const std::int64_t max_extra =
            static_cast<std::int64_t>(n) * (n - 1) / 2 - (n - 1);
        std::int64_t todo = std::min<std::int64_t>(extra, max_extra);
        while (todo > 0) {
            const int u = std::uniform_int_distribution<int>(0, n - 1)(rng);
            const int v = std::uniform_int_distribution<int>(0, n - 1)(rng);
            if (u != v && !seen(u, v)) {
                t.add_edge(u, v);
                --todo;
            }
        }
        return t;
    }

private:
    const std::pair<int, int>& end(int v, int port) const {
        return adj_.at(static_cast<std::size_t>(v)).at(static_cast<std::size_t>(port));
    }
    std::vector<std::vector<std::pair<int, int>>> adj_;  // (neighbor, port on the neighbor)
};

// --------------------------------------------------------------------------------------------
// Synchronous rounds
// --------------------------------------------------------------------------------------------

template <typename Msg>
class Outbox {
public:
    Outbox(const Topology& topology, int vertex, std::vector<std::vector<std::optional<Msg>>>& inbox)
        : topology_(topology), vertex_(vertex), inbox_(inbox) {}

    int ports() const { return topology_.degree(vertex_); }
    std::int64_t sent() const { return sent_; }

    // Puts m on a port for this round; at most one message per port per round.
    void send(int port, Msg m) {
        if (port < 0 || port >= ports()) {
            throw std::out_of_range("send on a port that does not exist");
        }
        const auto to = static_cast<std::size_t>(topology_.neighbor(vertex_, port));
        const auto at = static_cast<std::size_t>(topology_.reverse_port(vertex_, port));
        auto& slot = inbox_[to][at];
        if (slot.has_value()) {
            throw std::logic_error("two messages on one port in a single round");
        }
        slot = std::move(m);
        ++sent_;
    }
    void broadcast(const Msg& m) {
        for (int port = 0; port < ports(); ++port) {
            send(port, m);
        }
    }

private:
    const Topology& topology_;
    int vertex_;
    std::vector<std::vector<std::optional<Msg>>>& inbox_;
    std::int64_t sent_ = 0;
};

// A process for SyncNetwork. In round r (r = 1, 2, ...) the simulator calls
//   send(r, outbox)  decide this round's messages from the current state only;
//   receive(r, inbox)  inbox has one slot per port, nullopt where nothing arrived;
// and stops calling a process once halted() returns true. The network stops when all halted.
template <typename P>
concept SyncProcess = requires(P p, const P cp, std::int64_t round, Outbox<typename P::Message>& out,
                               std::span<const std::optional<typename P::Message>> inbox) {
    typename P::Message;
    { p.send(round, out) };
    { p.receive(round, inbox) };
    { cp.halted() } -> std::convertible_to<bool>;
};

template <SyncProcess P>
class SyncNetwork {
public:
    using Msg = typename P::Message;

    SyncNetwork(Topology topology, std::vector<P> processes)
        : topology_(std::move(topology)), processes_(std::move(processes)) {
        if (processes_.size() != static_cast<std::size_t>(topology_.size())) {
            throw std::invalid_argument("need exactly one process per vertex");
        }
    }

    // Runs one round. Returns false, doing nothing, if every process has already halted.
    bool step() {
        if (all_halted()) {
            return false;
        }
        const auto n = processes_.size();
        if (inbox_.size() != n) {
            inbox_.resize(n);
            for (std::size_t v = 0; v < n; ++v) {
                inbox_[v].resize(static_cast<std::size_t>(topology_.degree(static_cast<int>(v))));
            }
        }
        const std::int64_t round = ++stats_.rounds;
        for (auto& slots : inbox_) {
            std::fill(slots.begin(), slots.end(), std::nullopt);
        }
        for (std::size_t v = 0; v < n; ++v) {
            if (!processes_[v].halted()) {
                Outbox<Msg> out(topology_, static_cast<int>(v), inbox_);
                processes_[v].send(round, out);
                stats_.messages += out.sent();
            }
        }
        for (std::size_t v = 0; v < n; ++v) {
            if (!processes_[v].halted()) {
                processes_[v].receive(round, std::span<const std::optional<Msg>>(inbox_[v]));
            }
        }
        return true;
    }

    // Runs until every process has halted or the round counter reaches max_rounds.
    RunStats run(std::int64_t max_rounds = 1'000'000) {
        while (stats_.rounds < max_rounds && step()) {
        }
        return stats_;
    }

    const RunStats& stats() const { return stats_; }

    P& process(int v) { return processes_[static_cast<std::size_t>(v)]; }
    const P& process(int v) const { return processes_[static_cast<std::size_t>(v)]; }
    const Topology& topology() const { return topology_; }

private:
    bool all_halted() const {
        for (const P& p : processes_) {
            if (!p.halted()) {
                return false;
            }
        }
        return true;
    }

    Topology topology_;
    std::vector<P> processes_;
    std::vector<std::vector<std::optional<Msg>>> inbox_;
    RunStats stats_;
};

// --------------------------------------------------------------------------------------------
// Asynchronous events
// --------------------------------------------------------------------------------------------

// Delay policy: delay(from, port, seq) >= 1 ticks for the seq-th message of the run, sent by
// vertex `from` on its port `port`. (Ports, unlike vertex pairs, identify a channel even when
// two parallel edges connect the same vertices.)
using DelayFn = std::function<std::int64_t(int from, int port, std::int64_t seq)>;

// Every message takes one tick, which makes the network behave like synchronous rounds.
inline DelayFn unit_delay() {
    return [](int, int, std::int64_t) { return std::int64_t{1}; };
}
// Every message takes a fixed pseudo-random 1..d ticks; the sequence is fixed by the seed.
inline DelayFn random_delay(std::uint64_t seed, std::int64_t d) {
    auto rng = std::make_shared<std::mt19937_64>(seed);
    return [rng, d](int, int, std::int64_t) {
        return std::uniform_int_distribution<std::int64_t>(1, d)(*rng);
    };
}

template <typename Msg>
class Context;

template <typename Msg>
struct AsyncEngine {
    struct Output {
        std::int64_t time;
        std::int64_t value;
    };
    struct Event {
        std::int64_t time;
        std::int64_t seq;
        int from;
        int port;
        Msg message;
    };
    struct Later {
        bool operator()(const Event& a, const Event& b) const {
            return std::tie(a.time, a.seq) > std::tie(b.time, b.seq);
        }
    };

    AsyncEngine(const Topology& t, DelayFn d) : topology(t), delay(std::move(d)) {
        const auto n = static_cast<std::size_t>(t.size());
        last_delivery.resize(n);
        outputs.resize(n);
        for (std::size_t v = 0; v < n; ++v) {
            last_delivery[v].assign(static_cast<std::size_t>(t.degree(static_cast<int>(v))), 0);
        }
    }

    void post(int from, int port, Msg m) {
        const std::int64_t seq = next_seq++;
        const std::int64_t ticks = delay(from, port, seq);
        if (ticks < 1) {
            throw std::logic_error("message delay must be at least one tick");
        }
        auto& last = last_delivery[static_cast<std::size_t>(from)][static_cast<std::size_t>(port)];
        last = std::max(last, now + ticks);  // FIFO: a message never overtakes its predecessor
        queue.push(Event{last, seq, from, port, std::move(m)});
    }

    const Topology& topology;
    DelayFn delay;
    std::vector<std::vector<std::int64_t>> last_delivery;
    std::vector<std::vector<Output>> outputs;
    std::priority_queue<Event, std::vector<Event>, Later> queue;
    std::int64_t now = 0;
    std::int64_t next_seq = 0;
    std::int64_t messages = 0;
};

// What a process may do during a callback.
template <typename Msg>
class Context {
public:
    Context(AsyncEngine<Msg>& engine, int vertex) : engine_(engine), vertex_(vertex) {}

    int ports() const { return engine_.topology.degree(vertex_); }
    std::int64_t now() const { return engine_.now; }

    void send(int port, Msg m) {
        if (port < 0 || port >= ports()) {
            throw std::out_of_range("send on a port that does not exist");
        }
        ++engine_.messages;
        engine_.post(vertex_, port, std::move(m));
    }
    void broadcast(const Msg& m) {
        for (int port = 0; port < ports(); ++port) {
            send(port, m);
        }
    }
    // Records an externally visible output (a decision), with the current time.
    void output(std::int64_t value) {
        engine_.outputs[static_cast<std::size_t>(vertex_)].push_back({engine_.now, value});
    }

private:
    AsyncEngine<Msg>& engine_;
    int vertex_;
};

// A process for AsyncNetwork: on_start(ctx) runs once at time 0, on_message(port, m, ctx) for
// every delivered message.
template <typename P>
concept AsyncProcess = requires(P p, int port, const typename P::Message& m,
                                Context<typename P::Message>& ctx) {
    typename P::Message;
    { p.on_start(ctx) };
    { p.on_message(port, m, ctx) };
};

template <AsyncProcess P>
class AsyncNetwork {
public:
    using Msg = typename P::Message;
    using Output = typename AsyncEngine<Msg>::Output;

    AsyncNetwork(Topology topology, std::vector<P> processes, DelayFn delay = unit_delay())
        : topology_(std::move(topology)),
          processes_(std::move(processes)),
          engine_(std::make_unique<AsyncEngine<Msg>>(topology_, std::move(delay))) {
        if (processes_.size() != static_cast<std::size_t>(topology_.size())) {
            throw std::invalid_argument("need exactly one process per vertex");
        }
    }

    // Runs until the channels are empty.
    RunStats run(std::int64_t max_events = 100'000'000) {
        RunStats stats;
        for (std::size_t v = 0; v < processes_.size(); ++v) {
            Context<Msg> context(*engine_, static_cast<int>(v));
            processes_[v].on_start(context);
        }
        std::int64_t events = 0;
        while (!engine_->queue.empty()) {
            if (++events > max_events) {
                throw std::runtime_error("too many events: the protocol does not seem to stop");
            }
            auto event = engine_->queue.top();
            engine_->queue.pop();
            engine_->now = stats.time = event.time;
            const int to = topology_.neighbor(event.from, event.port);
            Context<Msg> context(*engine_, to);
            processes_[static_cast<std::size_t>(to)].on_message(
                topology_.reverse_port(event.from, event.port), event.message, context);
        }
        stats.messages = engine_->messages;
        return stats;
    }

    P& process(int v) { return processes_[static_cast<std::size_t>(v)]; }
    const P& process(int v) const { return processes_[static_cast<std::size_t>(v)]; }
    const Topology& topology() const { return topology_; }
    const std::vector<Output>& outputs(int v) const {
        return engine_->outputs[static_cast<std::size_t>(v)];
    }

private:
    Topology topology_;
    std::vector<P> processes_;
    std::unique_ptr<AsyncEngine<Msg>> engine_;
};

}  // namespace netsim
