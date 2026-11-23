#include "Config.h"
#include "Distributor.h"
#include "TcpTransport.h"
#include <cstddef>
#include <iostream>
#include <string>
#include <unordered_set>
int main()
{
    try
    {
        const Config c = Config::load("config.conf");
        Distributor d(c);
        TcpTransport t(c);
        std::unordered_set<std::string> global;
        std::size_t totalUnique = 0, totalReceived = 0, misplaced = 0, duplicates = 0;
        bool ok = true;
        std::cout << "=== STAGE 9 OWNERSHIP VERIFICATION ===\n";
        for (int n = 0; n < c.nodes; ++n)
        {
            auto inventory = t.fetchInventory(n);
            const auto &keys = inventory.keys;
            std::size_t bad = 0, dup = 0;
            for (const auto &k : keys)
            {
                if (d.ownerOf(k) != n)
                    ++bad;
                if (!global.insert(k).second)
                    ++dup;
            }
            totalUnique += keys.size();
            totalReceived += inventory.receivedRecords;
            misplaced += bad;
            duplicates += dup;
            std::cout << "Node " << n << ": received_records=" << inventory.receivedRecords << " unique_keys=" << keys.size() << " misplaced=" << bad << " duplicate_placements=" << dup << " " << ((bad == 0 && dup == 0) ? "PASS" : "FAIL") << "\n";
            if (bad || dup)
                ok = false;
        }
        std::cout << "Total records received across nodes: " << totalReceived << "\n";
        std::cout << "Total unique keys stored: " << totalUnique << "\n";
        std::cout << "Misplaced records: " << misplaced << "\nDuplicate placements: " << duplicates << "\n";
        std::cout << "OWNERSHIP VERIFICATION: " << (ok ? "PASS" : "FAIL") << "\n";
        return ok ? 0 : 2;
    }
    catch (const std::exception &e)
    {
        std::cerr << "VERIFY ERROR: " << e.what() << "\n";
        return 1;
    }
}
