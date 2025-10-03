#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <stdexcept>

void to_lower(std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
}

bool get_bit(const std::string& key, int bit_index) {
    if (bit_index / 8 >= key.length()) {
        return false;
    }
    char byte = key[bit_index / 8];
    int bit_in_byte = 7 - (bit_index % 8);
    return (byte >> bit_in_byte) & 1;
}

class Patricia {
private:
    struct Node {
        std::string key;
        uint64_t value;
        int bit_index;
        std::shared_ptr<Node> left = nullptr;
        std::shared_ptr<Node> right = nullptr;

        Node(const std::string& k, uint64_t v) : key(k), value(v), bit_index(-1) {}
        Node(int b_idx) : bit_index(b_idx) {}

        bool is_leaf() const {
            return left == nullptr && right == nullptr;
        }
    };

    std::shared_ptr<Node> root = nullptr;
    void save_node_recursive(std::ofstream& out, const std::shared_ptr<Node>& node) const;
    std::shared_ptr<Node> load_node_recursive(std::ifstream& in);

public:
    bool insert(const std::string& key, uint64_t value);
    bool remove(const std::string& key);
    std::pair<bool, uint64_t> find(const std::string& key) const;
    void save(const std::string& path) const;
    void load(const std::string& path);
};

std::pair<bool, uint64_t> Patricia::find(const std::string& key) const {
    if (!root) {
        return {false, 0};
    }

    std::shared_ptr<Node> current = root;
    while (!current->is_leaf()) {
        if (get_bit(key, current->bit_index)) {
            current = current->right;
        } else {
            current = current->left;
        }
    }


    if (current->key == key) {
        return {true, current->value};
    }
    return {false, 0};
}


bool Patricia::insert(const std::string& key, uint64_t value) {
    if (!root) {
        root = std::make_shared<Node>(key, value);
        return true;
    }

    std::shared_ptr<Node> current = root;
    while (!current->is_leaf()) {
        if (get_bit(key, current->bit_index)) {
            current = current->right;
        } else {
            current = current->left;
        }
    }

    if (current->key == key) {
        return false; // "Exist"
    }

    int diff_bit_index = 0;
    while (get_bit(key, diff_bit_index) == get_bit(current->key, diff_bit_index)) {
        diff_bit_index++;
    }

    std::shared_ptr<Node> search_curr = root;
    std::shared_ptr<Node> parent = nullptr;
    while (!search_curr->is_leaf() && search_curr->bit_index < diff_bit_index) {
        parent = search_curr;
        if (get_bit(key, search_curr->bit_index)) {
            search_curr = search_curr->right;
        } else {
            search_curr = search_curr->left;
        }
    }

    auto new_leaf = std::make_shared<Node>(key, value);
    auto new_internal = std::make_shared<Node>(diff_bit_index);

    if (get_bit(key, diff_bit_index)) {
        new_internal->right = new_leaf;
        new_internal->left = search_curr;
    } else {
        new_internal->left = new_leaf;
        new_internal->right = search_curr;
    }

    if (!parent) {
        root = new_internal;
    } else {
        if (parent->right == search_curr) {
            parent->right = new_internal;
        } else {
            parent->left = new_internal;
        }
    }
    return true;
}

bool Patricia::remove(const std::string& key) {
    if (!root) return false;

    std::shared_ptr<Node> current = root;
    std::shared_ptr<Node> parent = nullptr;
    std::shared_ptr<Node> grandparent = nullptr;
    
    while (!current->is_leaf()) {
        grandparent = parent;
        parent = current;
        if (get_bit(key, current->bit_index)) {
            current = current->right;
        } else {
            current = current->left;
        }
    }

    if (current->key != key) {
        return false;
    }

    if (!parent) {
        root = nullptr;
        return true;
    }

    std::shared_ptr<Node> sibling = (parent->left == current) ? parent->right : parent->left;
    if (!grandparent) {
        root = sibling;
    } else {
        if (grandparent->left == parent) {
            grandparent->left = sibling;
        } else {
            grandparent->right = sibling;
        }
    }
    return true;
}

void Patricia::save(const std::string& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error(strerror(errno));
    }
    const char* magic = "PATRICIA";
    out.write(magic, 8);
    save_node_recursive(out, root);
    if (!out.good()) {
        throw std::runtime_error("Failed to write to file.");
    }
}

void Patricia::save_node_recursive(std::ofstream& out, const std::shared_ptr<Node>& node) const {
    if (!node) {
        char marker = 'N'; out.write(&marker, sizeof(marker)); return;
    }
    if (node->is_leaf()) {
        char marker = 'L'; out.write(&marker, sizeof(marker));
        size_t key_len = node->key.length();
        out.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
        out.write(node->key.c_str(), key_len);
        out.write(reinterpret_cast<const char*>(&node->value), sizeof(node->value));
    } else {
        char marker = 'I'; out.write(&marker, sizeof(marker));
        out.write(reinterpret_cast<const char*>(&node->bit_index), sizeof(node->bit_index));
        save_node_recursive(out, node->left);
        save_node_recursive(out, node->right);
    }
}

void Patricia::load(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error(strerror(errno));
    }
    char magic[8];
    in.read(magic, 8);
    if (in.gcount() != 8 || strncmp(magic, "PATRICIA", 8) != 0) {
        throw std::runtime_error("Invalid file format or not a dictionary file.");
    }
    std::shared_ptr<Node> new_root = load_node_recursive(in);
    in.peek();
    if (!in.eof()) {
        throw std::runtime_error("Corrupted file: trailing data detected.");
    }
    root = new_root;
}

std::shared_ptr<Patricia::Node> Patricia::load_node_recursive(std::ifstream& in) {
    char marker;
    in.read(&marker, sizeof(marker));
    if (in.gcount() != sizeof(marker)) throw std::runtime_error("Invalid file format.");
    if (marker == 'N') {
        return nullptr;
    } else if (marker == 'L') {
        size_t key_len;
        in.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
        std::string key(key_len, '\0');
        in.read(&key[0], key_len);
        uint64_t value;
        in.read(reinterpret_cast<char*>(&value), sizeof(value));
        if(in.fail()) throw std::runtime_error("Invalid file format.");
        return std::make_shared<Node>(key, value);
    } else if (marker == 'I') {
        int bit_index;
        in.read(reinterpret_cast<char*>(&bit_index), sizeof(bit_index));
        if(in.fail()) throw std::runtime_error("Invalid file format.");
        auto node = std::make_shared<Node>(bit_index);
        node->left = load_node_recursive(in);
        node->right = load_node_recursive(in);
        return node;
    } else {
        throw std::runtime_error("Corrupted file: unknown node marker.");
    }
}


void handle_add(Patricia& dict, std::stringstream& ss) {
    std::string word;
    uint64_t num;
    ss >> word >> num;
    to_lower(word);
    if (dict.insert(word, num)) {
        std::cout << "OK\n";
    } else {
        std::cout << "Exist\n";
    }
}

void handle_remove(Patricia& dict, std::stringstream& ss) {
    std::string word;
    ss >> word;
    to_lower(word);
    if (dict.remove(word)) {
        std::cout << "OK\n";
    } else {
        std::cout << "NoSuchWord\n";
    }
}

void handle_search(Patricia& dict, const std::string& word) {
    std::string key = word;
    to_lower(key);
    auto result = dict.find(key);
    if (result.first) {
        std::cout << "OK: " << result.second << "\n";
    } else {
        std::cout << "NoSuchWord\n";
    }
}

void handle_bang_command(Patricia& dict, std::stringstream& ss) {
    std::string sub_command, path;
    ss >> sub_command >> path;
    if (sub_command == "Save") {
        dict.save(path);
        std::cout << "OK\n";
    } else if (sub_command == "Load") {
        Patricia temp_dict;
        temp_dict.load(path);
        dict = temp_dict; 
        std::cout << "OK\n";
    }
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    Patricia dict;
    std::string line;

    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string command;
        ss >> command;

        try {
            if (command == "+") {
                handle_add(dict, ss);
            } else if (command == "-") {
                handle_remove(dict, ss);
            } else if (command == "!") {
                handle_bang_command(dict, ss);
            } else {
                handle_search(dict, command);
            }
        } catch (const std::bad_alloc& e) {
            std::cout << "ERROR: Not enough memory\n";
        } catch (const std::exception& e) {
            std::cout << "ERROR: " << e.what() << "\n";
        }
    }

    return 0;
}