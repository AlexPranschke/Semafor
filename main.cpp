#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>
#include <chrono>
#include <cstdlib>

class Restaurant {
private:
    std::queue<int> buffer;
    std::mutex mtx;
    std::condition_variable cv;
    int buffer_size;

    std::atomic<int> Cooks_count{ 0 };
    std::atomic<int> Waiters_count{ 0 };
    std::atomic<int> items_processed{ 0 };

    std::vector<std::thread> threads;
    bool stop_flag = false;

public:
    Restaurant(int size) : buffer_size(size) {}

    ~Restaurant() {
        stop_flag = true;
        cv.notify_all();
        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }
    }

    void Cook(int id) {
        while (!stop_flag) {
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            int item = rand() % 10+10;
            {
                
                std::unique_lock<std::mutex> lock(mtx);
                if (buffer.size() == buffer_size) {
                    std::cout << "BUFFER FULL\n";
                }
                cv.wait(lock, [this]() { return buffer.size() < buffer_size || stop_flag; });
                

                if (stop_flag) break;

                buffer.push(item);
                std::cout << "[Cook-" << id << "] Produced item: " << item << " Buffer size: "<< buffer.size() << std::endl;
                cv.notify_one();
            }
        }
    }

    void Waiter(int id) {
        while (!stop_flag) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            {
                std::unique_lock<std::mutex> lock(mtx);
                if (buffer.empty()) {
                    std::cout << "BUFFER EMPTY\n";
                }
                cv.wait(lock, [this]() {   
                    return !buffer.empty() || stop_flag;
                    });

                if (stop_flag) break;

                int item = buffer.front();
                buffer.pop();
                items_processed++;
                std::cout << "[Waiter-" << id << "] Consumed item: " << item << std::endl;
                cv.notify_one();
            }
        }
    }

    void add_Cook() {
        Cooks_count++;
        int id = Cooks_count.load();
        threads.emplace_back(&Restaurant::Cook, this, id);
        std::cout << "Added Cook-" << id << std::endl;
    }

    void add_Waiter() {
        Waiters_count++;
        int id = Waiters_count.load();
        threads.emplace_back(&Restaurant::Waiter, this, id);
        std::cout << "Added Waiter-" << id << std::endl;
    }

    void show_stats() {
        std::unique_lock<std::mutex> lock(mtx);
        std::cout << "\nStats:\n";
        std::cout << "Cooks: " << Cooks_count.load() << "\n";
        std::cout << "Waiters: " << Waiters_count.load() << "\n";
        std::cout << "Items Processed: " << items_processed.load() << "\n";
    }
};

int main() {
    Restaurant pc(3);

    pc.add_Cook();
    

    while (true) {
      
        std::cout << "1. Add Cook\n";
        std::cout << "2. Add Waiter\n";
        std::cout << "3. Show Stats\n";
        std::cout << "4. Exit\n";
       
        int choice;
        std::cin >> choice;

        switch (choice) {
        case 1:
            pc.add_Cook();
            break;
        case 2:
            pc.add_Waiter();

            break;
        case 3:
            pc.show_stats();
            break;
        case 4:
            std::cout << "Exiting" << std::endl;
            return 0;
        default:
            std::cout << "Invalid input" << std::endl;
        }
    }
}