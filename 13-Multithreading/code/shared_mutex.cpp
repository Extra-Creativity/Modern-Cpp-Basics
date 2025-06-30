#include <shared_mutex>
#include <thread>
#include <array>
#include <print>
#include <chrono>

using namespace std::literals;

class Database
{
public:
    int Read() {
        std::shared_lock sharedLock{ mutex_ };
        std::this_thread::sleep_for(100ms); // Assuming we need 100ms to read
        return data_;
    }
    
    void Write(int d) {
        std::lock_guard exclusiveLock{ mutex_ };
        std::this_thread::sleep_for(300ms); // Assuming we need 300ms to write.
        data_ = d; 
    }
private:
    std::shared_mutex mutex_;
    int data_ = 0;
};

int main()
{
    constexpr int readerNum = 6, writerNum = 2;
    Database db;
    std::array<std::jthread, readerNum + writerNum> threads; // 6 readers, 2 writers.
    for (int i = 0; i < readerNum; i++)
        threads[i] = std::jthread{ [&db, i] { 
            for (int j = 0; j < 10; j++) {
                std::this_thread::sleep_for(i * 30ms);
                std::println("Read from {}: {}", i, db.Read());
            }
        } 
    };

    for (int i = readerNum; i < readerNum + writerNum; i++)
        threads[i] = std::jthread{ [&db, i] {
            for (int j = 0; j < 3; j++)
            {
                std::this_thread::sleep_for(i * 100ms);
                int result = i * (j + 1);
                db.Write(result);
                std::println("Write from {}: {}", i, result);
            }
        }
    };
    // threads destruct before db, so that threads will join first; otherwise
    // access db will access invalid memory!
    // Notice that output may not be e.g. write 7 -> read 7, instead write 7 -> read 0,
    // since we don't use that mutex on output(you may think about data race here).
    return 0;
}