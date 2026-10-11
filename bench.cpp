// salahaDB vs SQLite benchmark
// Build: see bench/Makefile. Both engines run in-process, same machine, same data.
#include <sqlite3.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>
#include "Database.h"
#include "Command.h"

using Clock = std::chrono::steady_clock;
namespace fs = std::filesystem;

// swallow salahaDB's console output so printing to a terminal isn't measured
struct NullBuf : std::streambuf { int overflow(int c) override { return c; } };

template <typename F>
double ms(F&& f) {
    auto t0 = Clock::now();
    f();
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

// median of k runs of a repeatable (read-only) workload
template <typename F>
double medianMs(int k, F&& f) {
    std::vector<double> t;
    for (int i = 0; i < k; i++) t.push_back(ms(f));
    std::sort(t.begin(), t.end());
    return t[k / 2];
}

struct Row { std::string name; int age; };

// ---------- SQLite helpers ----------
struct Lite {
    sqlite3* db = nullptr;
    explicit Lite(const std::string& path, const char* syncMode) {
        fs::remove(path); fs::remove(path + "-journal");
        sqlite3_open(path.c_str(), &db);
        exec(std::string("PRAGMA synchronous=") + syncMode);
        exec("CREATE TABLE records(id INTEGER PRIMARY KEY, name TEXT, age INTEGER)");
        // salahaDB has hash indexes on name and age, so give SQLite the same
        exec("CREATE INDEX idx_name ON records(name)");
        exec("CREATE INDEX idx_age ON records(age)");
    }
    ~Lite() { sqlite3_close(db); }
    void exec(const std::string& s) { sqlite3_exec(db, s.c_str(), 0, 0, 0); }
    sqlite3_stmt* prep(const char* s) {
        sqlite3_stmt* st; sqlite3_prepare_v2(db, s, -1, &st, 0); return st;
    }
};

// step a SELECT to completion, touching every column like a real consumer would
static size_t drain(sqlite3_stmt* st) {
    size_t n = 0, sink = 0;
    while (sqlite3_step(st) == SQLITE_ROW) {
        sink += sqlite3_column_int(st, 0);
        const unsigned char* t = sqlite3_column_text(st, 1);
        sink += t ? t[0] : 0;
        sink += sqlite3_column_int(st, 2);
        n++;
    }
    sqlite3_reset(st);
    volatile size_t keep = sink; (void)keep;
    return n;
}

static void insertAll(Lite& L, const std::vector<Row>& rows, bool oneTxn) {
    sqlite3_stmt* st = L.prep("INSERT INTO records(name,age) VALUES(?,?)");
    if (oneTxn) L.exec("BEGIN");
    for (auto& r : rows) {
        sqlite3_bind_text(st, 1, r.name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(st, 2, r.age);
        sqlite3_step(st);
        sqlite3_reset(st);
    }
    if (oneTxn) L.exec("COMMIT");
    sqlite3_finalize(st);
}

int main(int argc, char** argv) {
    std::vector<size_t> sizes;
    for (int i = 1; i < argc; i++) sizes.push_back(std::stoul(argv[i]));
    if (sizes.empty()) sizes = {1000, 5000, 10000};

    NullBuf nb;
    std::ostream null(&nb);
    auto* realCout = std::cout.rdbuf();

    std::printf("| N | operation | salahaDB | SQLite (sync=OFF, per-op) | SQLite (default fsync, per-op) | SQLite (1 txn) |\n");
    std::printf("|---|---|---|---|---|---|\n");

    for (size_t N : sizes) {
        std::mt19937 rng(42);
        std::vector<Row> rows(N);
        for (size_t i = 0; i < N; i++)
            rows[i] = {"name" + std::to_string(rng() % 1000), 18 + (int)(rng() % 63)};

        auto row = [&](const char* op, const std::string& a, const std::string& b,
                       const std::string& c, const std::string& d) {
            std::printf("| %zu | %s | %s | %s | %s | %s |\n", N, op,
                        a.c_str(), b.c_str(), c.c_str(), d.c_str());
            std::fflush(stdout);
        };
        auto us = [](double totalMs, size_t ops) {
            char buf[64]; std::snprintf(buf, sizeof buf, "%.2f us/op", totalMs * 1000.0 / ops);
            return std::string(buf);
        };
        auto tot = [](double totalMs) {
            char buf[64]; std::snprintf(buf, sizeof buf, "%.2f ms", totalMs);
            return std::string(buf);
        };

        // ================= INSERT =================
        fs::remove("bench_salaha.db");
        std::cout.rdbuf(&nb);
        Database* sdb = new Database("bench_salaha.db");
        double sIns = ms([&] {
            for (auto& r : rows) {
                Command c{"insert", {r.name, std::to_string(r.age)}};
                sdb->execute(c);
            }
        });
        std::cout.rdbuf(realCout);

        Lite* off = new Lite("bench_off.db", "OFF");
        double offIns = ms([&] { insertAll(*off, rows, false); });

        // fsync-per-insert is slow by design; cap it so the run stays short
        size_t fullN = std::min<size_t>(N, 2000);
        double fullIns;
        {
            Lite full("bench_full.db", "FULL");
            std::vector<Row> sub(rows.begin(), rows.begin() + fullN);
            fullIns = ms([&] { insertAll(full, sub, false); });
        }
        double txnIns;
        {
            Lite t("bench_txn.db", "OFF");
            txnIns = ms([&] { insertAll(t, rows, true); });
        }
        row("insert (per row)", us(sIns, N), us(offIns, N), us(fullIns, fullN) + (fullN < N ? " (first 2000)" : ""), us(txnIns, N));

        // ================= FIND BY ID =================
        size_t Q = std::min<size_t>(N, 5000);
        std::vector<int> ids(Q);
        for (auto& x : ids) x = 1 + rng() % N;

        std::cout.rdbuf(&nb);
        double sFind = medianMs(3, [&] {
            for (int id : ids) { Command c{"find", {std::to_string(id)}}; sdb->execute(c); }
        });
        std::cout.rdbuf(realCout);
        sqlite3_stmt* st = off->prep("SELECT id,name,age FROM records WHERE id=?");
        double lFind = medianMs(3, [&] {
            for (int id : ids) { sqlite3_bind_int(st, 1, id); drain(st); }
        });
        sqlite3_finalize(st);
        row("find by id", us(sFind, Q), us(lFind, Q), "-", "-");

        // ================= WHERE name = ? =================
        size_t QN = 100;
        std::vector<std::string> qn(QN);
        for (auto& s : qn) s = "name" + std::to_string(rng() % 1000);
        std::cout.rdbuf(&nb);
        double sName = medianMs(3, [&] {
            for (auto& s : qn) { Command c{"select", {"name", s}}; sdb->execute(c); }
        });
        std::cout.rdbuf(realCout);
        st = off->prep("SELECT id,name,age FROM records WHERE name=?");
        double lName = medianMs(3, [&] {
            for (auto& s : qn) { sqlite3_bind_text(st, 1, s.c_str(), -1, SQLITE_STATIC); drain(st); }
        });
        sqlite3_finalize(st);
        row("WHERE name = ?", us(sName, QN), us(lName, QN), "-", "-");

        // ================= WHERE age = ? =================
        std::vector<int> qa(QN);
        for (auto& a : qa) a = 18 + rng() % 63;
        std::cout.rdbuf(&nb);
        double sAge = medianMs(3, [&] {
            for (int a : qa) { Command c{"select", {"age", std::to_string(a)}}; sdb->execute(c); }
        });
        std::cout.rdbuf(realCout);
        st = off->prep("SELECT id,name,age FROM records WHERE age=?");
        double lAge = medianMs(3, [&] {
            for (int a : qa) { sqlite3_bind_int(st, 1, a); drain(st); }
        });
        sqlite3_finalize(st);
        row("WHERE age = ?", us(sAge, QN), us(lAge, QN), "-", "-");

        // ================= id BETWEEN (width 100) =================
        size_t QR = 200;
        std::vector<int> lo(QR);
        for (auto& x : lo) x = 1 + rng() % (N > 100 ? N - 100 : 1);
        std::cout.rdbuf(&nb);
        double sRange = medianMs(3, [&] {
            for (int l : lo) {
                Command c{"select", {"id", std::to_string(l), std::to_string(l + 99)}};
                sdb->execute(c);
            }
        });
        std::cout.rdbuf(realCout);
        st = off->prep("SELECT id,name,age FROM records WHERE id BETWEEN ? AND ?");
        double lRange = medianMs(3, [&] {
            for (int l : lo) { sqlite3_bind_int(st, 1, l); sqlite3_bind_int(st, 2, l + 99); drain(st); }
        });
        sqlite3_finalize(st);
        row("id BETWEEN (100 rows)", us(sRange, QR), us(lRange, QR), "-", "-");

        // ================= ORDER BY name (full table) =================
        std::cout.rdbuf(&nb);
        double sSort = medianMs(3, [&] { Command c{"select", {"sort", "name"}}; sdb->execute(c); });
        std::cout.rdbuf(realCout);
        st = off->prep("SELECT id,name,age FROM records ORDER BY name COLLATE NOCASE");
        double lSort = medianMs(3, [&] { drain(st); });
        sqlite3_finalize(st);
        row("ORDER BY name (all rows)", tot(sSort), tot(lSort), "-", "-");

        // ================= reopen =================
        delete sdb;
        std::cout.rdbuf(&nb);
        double sOpen = ms([&] { sdb = new Database("bench_salaha.db"); });
        std::cout.rdbuf(realCout);
        delete off;
        double lOpen = ms([&] { off = new Lite("bench_off_reopen.db", "OFF"); }); // trivial open baseline
        delete off;
        // real reopen of populated sqlite file:
        {
            Lite* tmp = new Lite("bench_reopen_src.db", "OFF");
            insertAll(*tmp, rows, true);
            delete tmp;
            sqlite3* h = nullptr;
            lOpen = ms([&] {
                sqlite3_open("bench_reopen_src.db", &h);
                sqlite3_stmt* s2; sqlite3_prepare_v2(h, "SELECT count(*) FROM records", -1, &s2, 0);
                sqlite3_step(s2); sqlite3_finalize(s2);
            });
            sqlite3_close(h);
        }
        row("reopen file", tot(sOpen), tot(lOpen), "-", "-");

        // ================= UPDATE / DELETE =================
        size_t QU = std::min<size_t>(N, 1000);
        std::vector<int> uid(QU);
        for (auto& x : uid) x = 1 + rng() % N;
        std::cout.rdbuf(&nb);
        double sUpd = ms([&] {
            for (int id : uid) {
                Command c{"update", {std::to_string(id), "renamed", "30"}};
                sdb->execute(c);
            }
        });
        std::cout.rdbuf(realCout);
        off = new Lite("bench_off2.db", "OFF");
        insertAll(*off, rows, true);
        st = off->prep("UPDATE records SET name=?,age=? WHERE id=?");
        double lUpd = ms([&] {
            for (int id : uid) {
                sqlite3_bind_text(st, 1, "renamed", -1, SQLITE_STATIC);
                sqlite3_bind_int(st, 2, 30);
                sqlite3_bind_int(st, 3, id);
                sqlite3_step(st); sqlite3_reset(st);
            }
        });
        sqlite3_finalize(st);
        row("update by id", us(sUpd, QU), us(lUpd, QU), "-", "-");

        std::vector<int> did(uid);
        std::sort(did.begin(), did.end());
        did.erase(std::unique(did.begin(), did.end()), did.end());
        std::cout.rdbuf(&nb);
        double sDel = ms([&] {
            for (int id : did) { Command c{"delete", {std::to_string(id)}}; sdb->execute(c); }
        });
        std::cout.rdbuf(realCout);
        st = off->prep("DELETE FROM records WHERE id=?");
        double lDel = ms([&] {
            for (int id : did) { sqlite3_bind_int(st, 1, id); sqlite3_step(st); sqlite3_reset(st); }
        });
        sqlite3_finalize(st);
        row("delete by id", us(sDel, did.size()), us(lDel, did.size()), "-", "-");

        // ================= file size =================
        auto kb = [](const char* p) { char b[32]; std::snprintf(b, sizeof b, "%.0f KB", fs::file_size(p) / 1024.0); return std::string(b); };
        row("file size (after inserts)", "-", "-", "-", "-");
        std::printf("|  | salahaDB file | %s | SQLite file | %s |\n", kb("bench_salaha.db").c_str(), kb("bench_txn.db").c_str());

        delete sdb;
        delete off;
    }
    for (auto f : {"bench_salaha.db","bench_off.db","bench_full.db","bench_txn.db","bench_off2.db","bench_off_reopen.db","bench_reopen_src.db"})
        fs::remove(f);
    return 0;
}
