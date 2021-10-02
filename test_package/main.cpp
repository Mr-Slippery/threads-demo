#include "compute.h"

#ifdef __unix__
#include <unistd.h>
#include <cstdio>
#endif // __unix__

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <set>
#include <algorithm>
#include <unordered_map>

#include "thread_state.h"

using proc_state = int;

bool is_finished(proc_state state)
{
    constexpr proc_state LOWER_LIMIT = -100;
    constexpr proc_state UPPER_LIMIT = 100;
    return (state < LOWER_LIMIT || state > UPPER_LIMIT);
}

std::tuple<std::string, std::string, std::size_t, std::size_t> read_command(const std::set<std::string>& commands, const std::set<std::string>& no_id_commands) {
#ifdef __unix__
        std::string full_cmd;
#endif // __unix__
        std::string cmd;
        std::size_t supplied_id;
        std::string orig_id;
        std::size_t id;
        for (;;) {
            if (std::cin.eof()) {
                return std::make_tuple(cmd, orig_id, id, supplied_id);
            }
            std::cout << "> " << std::flush;
            std::cin >> cmd;
            if (commands.find(cmd) == commands.end()) {
                std::cerr << "Unknown command: " << cmd << std::endl;
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }
#ifdef __unix__
            full_cmd = cmd;
#endif // __unix__
            if (no_id_commands.find(cmd) != no_id_commands.end()) {
                break;
            }
            if (!(std::cin >> supplied_id)) {
                std::cerr << "Non-numeric worker id for " << cmd << " command." << std::endl;
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }
            orig_id = std::to_string(supplied_id);
#ifdef __unix__
            full_cmd += " " + orig_id;
#endif // __unix__
            id = supplied_id - 1;
            break;
        }
#ifdef __unix__
        if (!isatty(fileno(stdin))) {
            std::cout << full_cmd << std::endl;
        }
#endif // __unix__
        return std::make_tuple(cmd, orig_id, id, supplied_id);
}

std::unordered_map<thread_state, std::string> get_run_state_names() {
    std::unordered_map<thread_state, std::string> run_state_names ({
        { thread_state::PAUSED, "paused" },
        { thread_state::RUNNING, "running" },
        { thread_state::STOPPING, "stopped" },
        { thread_state::TERMINATED, "finished" }
    });
    return run_state_names;
}

constexpr auto get_help_text() {
    constexpr auto help_text = R""""(
  pause <thread_id>
  - pauses the thread with the given id and prints a confirmation message.
  resume <thread_id>
  - resumes the thread with the given id (if not stopped) and prints a confirmation message.
  stop <thread_id>
  - stops the thread with the given id (if not stopped) and prints a confirmation message.
  status
  - prints the id, status (paused, running, stopped, finished) and the current processing step for each thread.
  exit
  - gracefully stops all threads, prints their status and exits the program.
  help
  - prints a help message and instructions.
  sleep <n_seconds>
  - (extra command) puts the controller thread to sleep for <n_seconds>.
    )"""";
    return help_text;
}

void control_thread(
    std::vector<thread_state>& run_states,
    std::vector<std::mutex>& mutexes,
    std::vector<std::condition_variable>& cvs,
    std::vector<proc_state>& states
)
{
    const auto run_state_names = get_run_state_names();
    constexpr auto help_text = get_help_text();

    constexpr auto help = "help";
    constexpr auto resume = "resume";
    constexpr auto pause = "pause";
    constexpr auto stop = "stop";
    constexpr auto status = "status";
    constexpr auto exit = "exit";
    constexpr auto sleep = "sleep"; // extra command for testing
    const std::set<std::string> no_id_commands = {
        help
        , status
        , exit
    };
    const std::set<std::string> id_commands = {
        resume
        , stop
        , pause
        , sleep
    };
    std::set<std::string> commands(id_commands);
    commands.insert(no_id_commands.begin(), no_id_commands.end());
    for (;;) {
        auto [cmd, orig_id, id, supplied_id] = read_command(commands, no_id_commands);
        if (cmd == help) {
            std::cout << help_text << std::endl;
            continue;
        }
        if (cmd == sleep) {
            std::this_thread::sleep_for(std::chrono::seconds(id));
            continue;
        }
        const auto n_threads = run_states.size();
        if (cmd != exit && cmd != status && id >= n_threads) {
            std::cerr << "Invalid worker id: " << orig_id << std::endl;
            continue;
        }
        if (cmd == exit || cmd == status) {
            for (auto& m: mutexes) {
                m.lock();       
            }
            if (cmd == exit) {
                std::size_t count_remaining{0};
                for (auto& r_state: run_states) {
                    if (r_state != thread_state::TERMINATED) {
                        r_state = thread_state::STOPPING;
                        ++count_remaining;
                    }
                }
                for (auto& cv: cvs) {
                    cv.notify_one();
                }
                if (count_remaining) {
                    std::cout << "Stopping " << count_remaining << " remaining worker(s)." << std::endl;
                }
                for (auto& m: mutexes) {
                    m.unlock();       
                }
                return;
            }
            if (cmd == status) {
                for(auto i = 0; i < n_threads; ++i) {
                    std::cout << (i + 1) << " " << run_state_names.find(run_states[i])->second << " " << states[i] << std::endl;                    
                }
                for (auto& m: mutexes) {
                    m.unlock();       
                }
                continue;
            }
        }
        {
            std::unique_lock<std::mutex> l(mutexes[id]);
            auto& run_state = run_states[id];
            if (cmd == pause && run_state == thread_state::PAUSED) {
                continue;
            }
            if (cmd == resume && run_state == thread_state::RUNNING) {
                continue;
            }
            if (cmd == pause) {
                if (run_state != thread_state::TERMINATED &&
                    run_state != thread_state::STOPPING) {
                    run_state = thread_state::PAUSED;
                    cvs[id].notify_one();
                } else {
                    std::cerr << "Thread " << supplied_id << " has been terminated." << std::endl;
                }
                continue;
            }
            if (cmd == resume) {
                if (run_state != thread_state::TERMINATED &&
                    run_state != thread_state::STOPPING) {
                    run_state = thread_state::RUNNING;
                    cvs[id].notify_one();
                } else {
                    std::cerr << "Thread " << supplied_id << " has been terminated." << std::endl;
                }
                continue;
            }
            if (cmd == stop) {
                run_state = thread_state::STOPPING;
                cvs[id].notify_one();
                continue;
            }
        }
    }
}

void print_help(const char* program)
{
    constexpr auto usage_text = R"""(
%s --help
- prints help message and instructions
%s --threads <n_threads>
- starts <n_threads> (at least 1) thread(s) and waits for instructions.

)""";
    std::printf(usage_text, program, program);
}

std::size_t process_args(int argc, char *argv[]) {
    const auto usage = [&argv](){ print_help(argv[0]); };
    constexpr int INVALID_NUMBER_OF_ARGS = 1;
    constexpr int INVALID_NUMBER_OF_WORKERS = 2;
    constexpr int UNKNOWN_ARGUMENT = 3;
    if (argc < 2 || argc > 3) {
        usage();
        std::exit(INVALID_NUMBER_OF_ARGS);
    }
    const std::string help{"--help"};
    const std::string threads{"--threads"};
    if (help == argv[1]) {        
        usage();
        if (argc > 2) {
            std::exit(INVALID_NUMBER_OF_ARGS);
        } else {
            std::exit(0);
        }
    }
    if (threads == argv[1]) {
        if (argc < 3) {
            usage();
            std::exit(INVALID_NUMBER_OF_ARGS);
        }
        try {
            auto n_threads = std::stoi(argv[2]);
            if (n_threads <= 0) {
                usage();
                std::exit(INVALID_NUMBER_OF_WORKERS);
            }
            return n_threads;
        } catch (std::invalid_argument& e) {
            usage();
            std::exit(INVALID_NUMBER_OF_WORKERS);
        }
    }
    usage();
    std::exit(UNKNOWN_ARGUMENT);
}

int main(int argc, char *argv[])
{
    auto n_threads = process_args(argc, argv);
    std::vector<std::thread> threads(n_threads);
    std::vector<thread_state> run_states(n_threads, thread_state::RUNNING);
    std::vector<std::condition_variable> cvs(n_threads);
    std::vector<std::mutex> mutexes(n_threads);
    std::vector<proc_state> states(n_threads);

    for (auto id = 0; id < n_threads; ++id) {
        const auto work = (id % 2) ? increment: decrement;
        auto& run_state = run_states[id];
        auto& cv = cvs[id];
        auto& mtx = mutexes[id];
        auto& state = states[id];
        threads.emplace_back(std::thread([work, &cv, &run_state, &mtx, &state](int id)
        {
            for (;;) {
                constexpr auto SLEEP_TIME = 100u;
                std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
                {
                    std::unique_lock<std::mutex> l(mtx);
                    cv.wait(l, [&run_state]() { 
                        return
                            run_state == thread_state::RUNNING
                            || run_state == thread_state::STOPPING
                        ;
                    });
                    if (run_state == thread_state::RUNNING) { 
                        if (is_finished(state)) {
                            run_state = thread_state::TERMINATED;
                            return;
                        }
                        state = work(state);
                        continue;
                    }
                    if (run_state == thread_state::STOPPING) {
                        run_state = thread_state::TERMINATED;
                        return;
                    }
                }
            }
        }, id));
    }
    auto ct = std::thread([&run_states, &mutexes, &cvs, &states]() {
        control_thread(run_states, mutexes, cvs, states);
    });
    if (ct.joinable()) {
        ct.join();
    }
    for(auto& t: threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}