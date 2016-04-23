#ifndef RAS_H
#define RAS_H

#include <vector>

class RAS
{
public:
    RAS(UINT32 num_entries) 
      : max_entries(num_entries), correct(0), incorrect(0) {};
    ~RAS() {};

    void push_addr(ADDRINT addr) {
        addr_vec.push_back(addr);
        if (addr_vec.size() > max_entries)
            addr_vec.erase(addr_vec.begin());
    }

    void pop_addr(ADDRINT target) {
        if (addr_vec.empty()) {
            incorrect++;
            return;
        }

        ADDRINT ras_ip = *(addr_vec.end() - 1);
        addr_vec.pop_back();

        if (ras_ip == target)
            correct++;
        else
            incorrect++;
    }

    string getNameAndStats() { 
        std::ostringstream stream;
        stream << "RAS (" << max_entries << " entries): " << correct <<
                                                      " " << incorrect;
        return stream.str(); 
    };

private:
    UINT32 max_entries;
    std::vector<ADDRINT> addr_vec;

    unsigned long long correct, incorrect;
};

#endif
