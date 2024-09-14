#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <chrono>
#include <ctime>
#include <cstring>
#include <atomic>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib> // For std::atoi and std::atol
#include <mutex>
#include <semaphore.h> // For POSIX semaphores

// Constants for default values
const int DEFAULT_PORT = 12346; // Default port number
const size_t DEFAULT_LOG_ROTATION_SIZE = 5 * 1024 * 1024; // 5 MB size threshold for log rotation
const int MAX_THREADS = 10; // Maximum number of concurrent client threads

std::atomic<bool> server_running(true); // Atomic flag for server running status
std::mutex thread_pool_mutex; // Mutex for protecting access to thread_pool
sem_t semaphore; // Semaphore to limit the number of concurrent threads

// Function to get current timestamp
std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    char buffer[100];
    struct tm *timeinfo;
    timeinfo = localtime(&now_time_t);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", timeinfo);
    return std::string(buffer);
}

// Function to rotate log file
void rotate_log_file(const std::string& filename) {
    std::string new_filename = filename + "." + get_current_timestamp();
    if (rename(filename.c_str(), new_filename.c_str()) != 0) {
        std::cerr << "Error rotating log file: " << filename << std::endl;
    }
}

// Logging function
void log_message(const std::string& log_file, size_t log_rotation_size, const std::string& client_ip, const std::string& message) {
    // Open log file in append mode
    std::ofstream ofs(log_file, std::ios::app);
    if (!ofs.is_open()) {
        std::cerr << "Unable to open log file: " << log_file << std::endl;
        return;
    }

    // Get current timestamp
    std::string timestamp = get_current_timestamp();

    // Get message size
    size_t message_size = message.size();

    // Write log entry
    ofs << "[" << timestamp << "] "
        << client_ip << " ("
        << message_size << " bytes): "
        << message << std::endl;

    ofs.close();

    // Check for log rotation
    std::ifstream infile(log_file, std::ifstream::ate | std::ifstream::binary);
    size_t file_size = infile.tellg();
    if (file_size >= log_rotation_size) {
        rotate_log_file(log_file);
    }
}

// Client handler function
void client_handler(int client_socket, sockaddr_in client_addr, size_t log_rotation_size) {
    // Signal the semaphore when the thread finishes
    struct SemaphoreGuard {
        SemaphoreGuard(sem_t& sem) : sem_(sem) {}
        ~SemaphoreGuard() {
            sem_post(&sem_);
        }
        sem_t& sem_;
    } guard(semaphore);

    char buffer[1024];
    std::string client_ip = inet_ntoa(client_addr.sin_addr);
    int client_port = ntohs(client_addr.sin_port);

    // Create a unique log file for this client
    std::string log_file = "client_" + client_ip + "_" + std::to_string(client_port) + ".log";

    std::cout << "Logging to file: " << log_file << std::endl;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0) {
            std::cout << "Client " << client_ip << ":" << client_port << " disconnected." << std::endl;
            close(client_socket);
            break;
        }

        std::string message(buffer, bytes_received);
        log_message(log_file, log_rotation_size, client_ip, message);
    }
}

int main(int argc, char* argv[]) {
    // Default values
    int port = DEFAULT_PORT;
    size_t log_rotation_size = DEFAULT_LOG_ROTATION_SIZE;

    // Initialize semaphore
    if (sem_init(&semaphore, 0, MAX_THREADS) != 0) {
        std::cerr << "Semaphore initialization failed." << std::endl;
        return -1;
    }

    // Simple argument parsing
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = std::atoi(argv[++i]);
        } else if (strcmp(argv[i], "--rotation") == 0 && i + 1 < argc) {
            log_rotation_size = std::atol(argv[++i]);
        } else {
            std::cerr << "Unknown argument: " << argv[i] << std::endl;
            std::cerr << "Usage: " << argv[0] << " [--port PORT] [--rotation SIZE]" << std::endl;
            return -1;
        }
    }

    int server_socket;
    sockaddr_in server_addr;

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket creation error." << std::endl;
        sem_destroy(&semaphore);
        return -1;
    }

    // Bind address and port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Bind failed." << std::endl;
        close(server_socket);
        sem_destroy(&semaphore);
        return -1;
    }

    // Listen for connections
    if (listen(server_socket, SOMAXCONN) < 0) {
        std::cerr << "Listen failed." << std::endl;
        close(server_socket);
        sem_destroy(&semaphore);
        return -1;
    }

    std::cout << "Server is listening on port " << port << "..." << std::endl;

    std::vector<std::thread> thread_pool;

    while (server_running) {
        sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        // Accept new connection
        int client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);

        if (client_socket < 0) {
            std::cerr << "Accept failed." << std::endl;
            continue;
        }

        // Wait on semaphore before creating a new thread
        sem_wait(&semaphore);

        std::string client_ip = inet_ntoa(client_addr.sin_addr);
        int client_port = ntohs(client_addr.sin_port);

        std::cout << "Accepted connection from " << client_ip << ":" << client_port << std::endl;

        // Create a thread to handle the client
        std::thread t(client_handler, client_socket, client_addr, log_rotation_size);

        // Add the thread to the thread pool with mutex protection
        {
            std::lock_guard<std::mutex> lock(thread_pool_mutex);
            thread_pool.emplace_back(std::move(t));
        }

        // Clean up finished threads
        {
            std::lock_guard<std::mutex> lock(thread_pool_mutex);
            auto it = thread_pool.begin();
            while (it != thread_pool.end()) {
                if (it->joinable()) {
                    it->join();
                    it = thread_pool.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    // Clean up threads
    {
        std::lock_guard<std::mutex> lock(thread_pool_mutex);
        for (auto& th : thread_pool) {
            if (th.joinable()) {
                th.join();
            }
        }
    }

    sem_destroy(&semaphore);
    close(server_socket);
    return 0;
}
